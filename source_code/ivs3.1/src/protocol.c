/**************************************************************************\
*          Copyright (c) 1992 INRIA Sophia Antipolis, FRANCE.              *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
\**************************************************************************/
/**************************************************************************\
*                                                                          *
*  File              : protocol.c                                          *
*  Author            : Thierry Turletti                                    *
*  Last modification : 12/21/92                                            *
*--------------------------------------------------------------------------*
*  Description :  Protocol between stations.                               *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include "protocol.h"



/***************************************************************************\
*                        LOCAL TABLE FUNCTIONS                              *
\***************************************************************************/


void ResetStation(station)
     STATION *station;
{
  station->valid = FALSE;
  station->sin_addr = 0;
  station->feedback_port = 0;
  bzero(station->name, LEN_MAX_NAME);
  station->video_encoding = FALSE;
  station->audio_encoding = FALSE;
  station->video_decoding = FALSE;
  station->audio_decoding = FALSE;
}



void InitTable(t)
     LOCAL_TABLE t;
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    ResetStation(&t[i]);
  }
}


void InitStation(p, name, sin_addr, feedback_port, enc_audio, enc_video)
     STATION *p;
     char *name;
     u_long sin_addr;
     u_short feedback_port;
     BOOLEAN enc_audio, enc_video;
{
  p->valid = TRUE;
  p->sin_addr = sin_addr;
  p->feedback_port = feedback_port;
  strcpy(p->name, name);
  p->video_encoding = enc_video;
  p->audio_encoding = enc_audio;
  p->video_decoding = FALSE;
  p->audio_decoding = FALSE;
}


void CopyStation(out, in)
     STATION *out, *in;
{
  
  out->valid = in->valid;
  out->sin_addr = in->sin_addr;
  out->feedback_port = in->feedback_port;
  strcpy(out->name, in->name);
  out->video_encoding = in->video_encoding;
  out->audio_encoding = in->audio_encoding;
  out->video_decoding = in->video_decoding;
  out->audio_decoding = in->audio_decoding;
  out->lasttime = in->lasttime;
}




BOOLEAN ShallIDecode(t, nb_stations, sin_addr)
     AUDIO_TABLE t;
     int nb_stations;
     u_long sin_addr;
{
  register int i;

  for(i=0; i<nb_stations; i++){
    if(t[i].sin_addr == sin_addr){
      if(t[i].audio_decoding)
	return(TRUE);
      break;
    }
  }
  return(FALSE);
}


BOOLEAN AnyAudioDecoding(t, nb_stations)
     AUDIO_TABLE t;
     int nb_stations;
{
  register int i;

  for(i=0; i<nb_stations; i++){
    if(t[i].audio_decoding)
      return(TRUE);
  }
  return(FALSE);
}



void PrintStation(F, p, i)
     FILE *F;
     STATION *p;
     int i;
{
  fprintf(F, "\nSTATION %d ::\n", i);
  fprintf(F, "Name: %s\n", p->name);
  fprintf(F, "sin_addr: %d\n", p->sin_addr);
  fprintf(F, "audio_encoding : %s\n", (p->audio_encoding ? "Yes" : "No"));
  fprintf(F, "video_encoding : %s\n", (p->video_encoding ? "Yes" : "No"));
  fprintf(F, "valid : %s\n", (p->valid ? "Yes" : "No"));
}


void PrintTable(F, t)
     FILE *F;
     LOCAL_TABLE t;
{
  register int i;

  fprintf(F, "\n\n\t\t*** LOCAL TABLE ***");
  for(i=0; i<N_MAX_STATIONS; i++){
    if(t[i].valid)
      PrintStation(F, &(t[i]), i);
  }
}



/****************************************************************************\
*                  SENDING & RECEIVING CONTROL PACKETS                       *
\****************************************************************************/


int CreateSendSocket(addr, len_addr, port, group, ttl)
     struct sockaddr_in *addr;
     int *len_addr;
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
     unsigned char *ttl;
{
  int sock;
  unsigned int iport, i1, i2, i3, i4;
  extern BOOLEAN UNICAST;

  if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
    fprintf(stderr, "CreateSendSocket: Invalid group %s\n", group);
    return(-1);
  }

  iport = atoi(port);
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Cannot create datagram socket");
    return(-1);
  }
  *len_addr = sizeof(*addr);
  bzero((char *)addr, *len_addr);
  addr->sin_port = htons((u_short)iport);
  addr->sin_addr.s_addr = htonl((i4<<24) | (i3<<16) | (i2<<8) | i1);
  addr->sin_family = AF_INET;
  if(UNICAST == FALSE){
#ifdef MULTICAST
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, ttl, sizeof(*ttl));
#else
    fprintf(stderr, "MULTICAST mode not supported, use only unicast option\n");
    exit(-1);
#endif /* MULTICAST */
  }
  return(sock);
}


int CreateReceiveSocket(addr, len_addr, port, group)
     struct sockaddr_in *addr;
     int *len_addr;
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
{
  int sock, optlen, optval;
  unsigned int iport, i1, i2, i3, i4;
  struct hostent *hp;
  char hostname[64];
  extern BOOLEAN UNICAST;
#ifdef MULTICAST
  int one=1;
  struct ip_mreq imr;
#endif 

  if(!UNICAST){
    if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      fprintf(stderr, "CreateReceiveSocket: Invalid group %s\n", group);
      return(-1);
    }
  }

  *len_addr = sizeof(*addr);
  iport = atoi(port);
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Cannot create datagram socket");
    exit(-1);
  }
  gethostname(hostname, 64);
  if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "Unknown host %s\n", hostname);
      exit(0);
  }
  if(UNICAST){
    bzero((char *) addr, *len_addr);
    bcopy(hp->h_addr, (char *)&addr->sin_addr, 4);
  }else{
#ifdef MULTICAST
    bcopy(hp->h_addr, (char *)&imr.imr_interface.s_addr, hp->h_length);
    imr.imr_multiaddr.s_addr = (i4<<24) | (i3<<16) | (i2<<8) | i1;
    imr.imr_multiaddr.s_addr = htonl(imr.imr_multiaddr.s_addr);
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr,
		  sizeof(struct ip_mreq)) == -1){
      fprintf(stderr, "Cannot join group %s\n", group);
      perror("setsockopt()");
      fprintf(stderr, "Does your kernel support IP multicast extensions ?\n");
      exit(1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef INDIGO
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
#else
    fprintf(stderr, "MULTICAST mode not implemented, use unicast option\n");
    exit(-1);
#endif /* MULTICAST */
  }
  addr->sin_addr.s_addr = INADDR_ANY;
  addr->sin_family = AF_INET;
  addr->sin_port = htons((u_short)iport);
  if (bind(sock, (struct sockaddr *) addr, *len_addr) < 0){
    fprintf(stderr, "Cannot bind socket to %d port\n", iport);
    exit(1);
  }
  optlen = 4;
  optval = 50000;
  if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, optlen) != 0){
    perror("setsockopt failed");
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen);
    fprintf(stderr,"SO_RCVBUF = %d\n", optval); 
  }
  return(sock);
}


/*
*  Real Time Control (RTP) packets.
*/

int SendDESCRIPTOR(s, addr, len_addr, video_encoding, audio_encoding, 
		   feedback_port, sin_addr, name, class_name)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
     BOOLEAN video_encoding, audio_encoding;
     u_short feedback_port;
     u_long sin_addr;
     char *name;
     u_short class_name;
{
  static u_char buf[128];
  static int len;
  int lw;
  u_char length_sdes;
  static BOOLEAN FIRST=TRUE;

  /*
  *  0                   1                   2                   3
  *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |0|   FMT=32    |  length = 2   |0|0|  format   | clock quality |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |    version    |audio_encoding |video_encoding |      MBZ      |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |0|   SDES=33   |    length     |       source identifier       |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |          class = 1            |  length = 2   |      MBZ      |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |                        IP v4 address                          |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |      class = class_name       |    length     | text         ...
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * | ... describing the source ...                                ...
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |0|  RNA=36     |    length=3   |    format     |   interval=0  |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |      address class = 1        |      MBZ      |      MBZ      |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |                        network-address                       ...
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |1|  RTA=37     |  length = 2   |    format     | protocol = 17 |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |      MBZ      |      MBZ      |        UDP port number        |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * If UDP port number = 0, RTA and RNA are not sent.
  */

  if(FIRST){
    buf[0] = RTP_FMT;
    buf[1] = 0x02;
    buf[2] = RTP_H261_CONTENT;
    buf[3] = 0x00; /* clock quality ?? */
    buf[4] = RTP_IVS_VERSION;
    buf[7] = 0x00;
    buf[12] = 0x00;
    buf[13] = 0x01;
    buf[14] = 0x02;
    buf[15] = 0x00;
    bcopy((char *)&sin_addr, (char *)&buf[16], 4);
    FIRST=FALSE;
  }
  buf[5] = audio_encoding;
  buf[6] = video_encoding;
  buf[8] = (feedback_port ? 0x00 : 0x80) | RTP_SDES;
  length_sdes = 5 + ((strlen(name)) / 4);
  buf[9] = length_sdes;
  buf[20] = (u_char)(class_name >> 8);
  buf[21] = (u_char)(class_name & 0xff);
  buf[22] = length_sdes - 3;
  strcpy((char *)&buf[23], name);
  len = 8 + (length_sdes << 2);
  if(feedback_port){
    buf[len++] = RTP_RNA;
    buf[len++] = 0x03;
    buf[len++] = RTP_H261_CONTENT;
    buf[len++] = 0x00; /* QOS interval */
    buf[len++] = 0x00; /* address class */
    buf[len++] = 0x01; /* address class */
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    bcopy((char *)&sin_addr, (char *)&buf[len], 4);
    len += 4;
    buf[len++] = 0x80 | RTP_RTA;
    buf[len++] = 0x02;
    buf[len++] = RTP_H261_CONTENT;
    buf[len++] = (u_char)17; /* protocol UDP */
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    buf[len++] = (u_char)(feedback_port >> 8);
    buf[len++] = (u_char)(feedback_port & 0xff);
  }
  if((lw=sendto(s, (char *)buf, len, 0, (struct sockaddr *)addr, len_addr)) 
     != len){
    perror("Cannot send CSDESC packet on socket");
    return(0);
  }
  return(1);
}



SendBYE(s, addr, len_addr)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
{
  unsigned char buf[4];
  int lw;

  /*
  *  0                   1                   2                   3
  *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |1|    BYE=35   |   length=1    |       0       |       0       |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  
  buf[0] = 0xa3;
  buf[1] = 0x01;
  buf[2] = 0x00;
  buf[3] = 0x00;
  if((lw=sendto(s, (char *)buf, 4, 0, (struct sockaddr *)addr, len_addr)) 
     != 4){
    perror("Cannot send HELLO packet on socket");
    return(0);
  }
  return(1);
  
}



/*
*  Reverse Control packets.
*/
  
SendFIR(host_coder, port)
     char *host_coder;
     u_short port;
{
  struct hostent *hp;
  unsigned long add_long;
  unsigned char buf[4];
  int lw;
  static struct sockaddr_in addr;
  static int sock_report, len_addr;
  static BOOLEAN FIRST=TRUE;

  if(FIRST){
    bzero((char *)&addr, sizeof(addr));
    if((hp=gethostbyname(host_coder)) == 0){
      if((add_long=inet_addr(host_coder)) == -1){
	fprintf(stderr, "Unknown host : %s\n", host_coder);
	exit(1);
      }else
	addr.sin_addr.s_addr = add_long;
    }else
      bcopy(hp->h_addr, (char *)&addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    len_addr = sizeof(addr);
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    FIRST = FALSE;
  }

  /*
  *
  *   0                   1                   2                   3
  *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *  |F|    RAD      |  Length = 1   |      Type     |MBZ|   Flow    |
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  * With:
  *       F = [1]
  *       RAD = [65] Reverse Application Data.
  *       Flow = [0] Only one video flow per station.
  *       MBZ = [0]  Reserved.
  */
  buf[0] = 0x80 | RTP_RAD;
  buf[1] = 0x01;
  buf[2] = RTP_RAD_FIR;
  buf[3] = 0x00;
  if((lw=sendto(sock_report, (char *)buf, 4, 0, (struct sockaddr *)&addr, 
		len_addr)) != 4){
    perror("Cannot send FIR control packet on socket");
    return(0);
  }
  return(1);
  
}


void SendNACK(host_coder, port, F, FGOBL, LGOBL, FMT, sec_up, sec_down,
	 usec_up, usec_down)
     char *host_coder;
     u_short port;
     BOOLEAN F; /* False is many NACKs are encoded in the same packet. */
     int FGOBL; /* First GOB Lost */
     int LGOBL; /* Last GOB Lost */
     int FMT;   /* Image format */
     long sec_up, sec_down, usec_up, usec_down;

{
  struct hostent *hp;
  unsigned long add_long;
  static unsigned char buf[200];
  int lw;
  static struct sockaddr_in addr;
  static int sock_report, len_addr;
  static BOOLEAN FIRST=TRUE;
  static int k=0;

  if(FIRST){
    bzero((char *)&addr, sizeof(addr));
    if((hp=gethostbyname(host_coder)) == 0){
      if((add_long=inet_addr(host_coder)) == -1){
	fprintf(stderr, "Unknown host : %s\n", host_coder);
	exit(1);
      }else
	addr.sin_addr.s_addr = add_long;
    }else
      bcopy(hp->h_addr, (char *)&addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    len_addr = sizeof(addr);
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    FIRST = FALSE;
  }
  /*
  *   0                   1                   2                   3
  *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *  |F|    RAD      |  Length = 3   |      Type     |MBZ|   Flow    |
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *  |     FGOBL     |     LGOBL     |         MBZ             | FMT |
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *  | timestamp (seconds)           | timestamp (fraction)          |
  *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  * With:
  *       F = [0] if another NACK is following; [1] else.
  *       FGOBL = First GOB number lost.
  *       LGOBL = Last GOB number lost.
  *       FMT = Lost image format.
  *       timestamp: timestamp of the lost image.
  */
  buf[k++] = (F << 7) | RTP_RAD;
  buf[k++] = 0x03;
  buf[k++] = RTP_RAD_NACK;
  buf[k++] = 0x00; /* flow */
  buf[k++] = FGOBL;
  buf[k++] = LGOBL;
  buf[k++] = 0x00;
  buf[k++] = FMT;
  buf[k++] = sec_up;
  buf[k++] = sec_down;
  buf[k++] = usec_up;
  buf[k++] = usec_down;
  if(F){
    if((lw=sendto(sock_report, (char *)buf, k, 0, (struct sockaddr *)&addr, 
		  len_addr)) != k){
      perror("Cannot send Send Info Packet Lost on socket");
    }
    k = 0;
  }/* else, another NACK follows... */
}



/****************************************************************************\
*                               PIPE  UTILITIES                              *
\****************************************************************************/


BOOLEAN HandleCommands(fd, size, rate_control, port_vfc, rate_max, FREEZE,
		       LOCAL_DISPLAY, TUNING, FEEDBACK, brightness, contrast)
     int fd[2];
     int *size;
     int *rate_control;
     int *port_vfc;
     int *rate_max;
     BOOLEAN *FREEZE;
     BOOLEAN *LOCAL_DISPLAY;
     BOOLEAN *TUNING;
     BOOLEAN *FEEDBACK;
     int *brightness, *contrast;
{
  struct timeval tim_out;
  int mask;
  u_char command[500];
  int lr, i=0;

  mask = 1<<fd[0];
  tim_out.tv_sec = 0;
  tim_out.tv_usec = 0;
  if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
	    &tim_out) != 0){
    if((lr=read(fd[0], (char *)command, 500)) <= 0){
      perror("HandleCommands'read ");
      return FALSE;
    }
    do{
      switch(command[i]){
      case PIP_QUIT: /* One byte message */
	return FALSE;
      case PIP_SIZE: /* Two bytes message */
	*size = (int)command[++i];
	i ++;
	break;
      case PIP_FREEZE: /* One byte message */
	*FREEZE = TRUE;
	i ++;
	break;
      case PIP_UNFREEZE: /* One byte message */
	*FREEZE = FALSE;
	i ++;
	break;
      case PIP_WCONTROL: /* One byte message */
	*rate_control=WITHOUT_CONTROL;
	i ++;
	break;
      case PIP_RCONTROL: /* One byte message */
	*rate_control = REFRESH_CONTROL;
	i ++;
	break;
      case PIP_QCONTROL: /* One byte message */
	*rate_control = QUALITY_CONTROL;
	i ++;
	break;
      case PIP_VFC_PORT: /* Two bytes message */
	*port_vfc = (int)command[++i];
	i ++;
	break;
      case PIP_MAX_RATE: /* Five bytes message */
	sscanf((char *) &command[++i], "%d", rate_max);
	i += 4;
	break;
      case PIP_LDISP: /* One byte message */
	*LOCAL_DISPLAY = TRUE;
	i ++;
	break;
      case PIP_NOLDISP: /* One byte message */
	*LOCAL_DISPLAY = FALSE;
	i ++;
	break;
      case PIP_TUNING: /* 9 bytes message */
	*TUNING = TRUE;
	sscanf((char *) &command[++i], "%d", brightness); /* 4 bytes */
	i += 4;
	sscanf((char *) &command[i], "%d", contrast); /* 4 bytes */
	i += 4;
	break;
      case PIP_FEEDBACK: /* One byte message */
	*FEEDBACK = TRUE;
	i ++;
	break;
      case PIP_NOFEEDBACK: /* One byte message */
	*FEEDBACK = FALSE;
	i ++;
	break;
      }
    }while(lr>i);
  }
  return(TRUE);
}
      


SendPIP_QUIT(fd)
     int fd[2];
{
  u_char buf=PIP_QUIT;

  if(write(fd[1], (char *)&buf, 1) != 1){
#ifdef DEBUG
    perror("SendPIP_QUIT'write");
#endif
  }
}



BOOLEAN SendPIP_FORK(fd)
     int fd[2];
{
  u_char buf=PIP_FORK;

  if(write(fd[1], (char *)&buf, 1) != 1){
    perror("SendPIP_FORK'write");
    return(FALSE);
  }
  return(TRUE);
}



BOOLEAN SendPIP_FEEDBACK(fd, feedback_port)
     int fd[2];
     u_short feedback_port;
{
  u_char buf[3];

  buf[0] = PIP_FEEDBACK;
  buf[1] = (u_char)(feedback_port >> 8);
  buf[2] = (u_char)(feedback_port & 0xff);

  if(write(fd[1], (char *)buf, 3) != 3){
    perror("SendPIP_FEEDBACK'write");
    return(FALSE);
  }
  return(TRUE);
}



SendPIP_NEW_TABLE(t, nb_stations, fd)
     LOCAL_TABLE t;
     int nb_stations; /* less than 255 .. */
     int fd[2];
{
  u_char *buf;
  int lw, lbuf;
  register int i;
  AUDIO_TABLE t_audio;

  lbuf = nb_stations*sizeof(AUDIO_STATION);
  buf = (u_char *)malloc((unsigned)(lbuf+2));
  buf[0] = (u_char) PIP_NEW_TABLE;
  buf[1] = (u_char) nb_stations;
  for(i=0; i<nb_stations; i++){
    t_audio[i].sin_addr = t[i].sin_addr;
    t_audio[i].audio_decoding = t[i].audio_decoding;
  }
  bcopy((char *)t_audio, (char *)&buf[2], lbuf);
  if((lw=write(fd[1], (char *)buf, lbuf+2)) < 0){
    perror("SendPIP_NEW_TABLE'write");
  }
  free((char *)buf);
}


int GetType(buf)
     u_char *buf;
{
  return(buf[0] >> 6);
}




/****************************************************************************\
*                         IVS DAEMON  UTILITIES                              *
\****************************************************************************/


int SendTalkRequest(sock, name)
     int sock;
     char *name;
{
  int lwrite, len;
  char data[10];

  len = strlen(name) + 1;
  bzero(data, 10);
  data[0] = (u_char) CALL_REQUEST;
  strcpy(&data[1], name);
  if((lwrite = write(sock, data, len)) != len)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkAbort(sock)
     int sock;
{
  int lwrite;
  char data;

  data = (u_char) CALL_ABORT;
  if((lwrite = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkAccept(sock)
     int sock;
{
  int lwrite;
  char data;

  data = (u_char) CALL_ACCEPTED;
  if((lwrite = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkRefuse(sock)
     int sock;
{
  int lwrite;
  char data;

  data = (u_char) CALL_REFUSED;
  if((lwrite = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


