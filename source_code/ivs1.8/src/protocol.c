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
*  File    : protocol.c                                                    *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
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

void InitTable(t)
     LOCAL_TABLE t;
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    bzero(t[i].name, 50);
    t[i].sin_addr = 0;
    t[i].video_encoding = FALSE;
    t[i].video_decoding = FALSE;
    t[i].audio_encoding = FALSE;
    t[i].audio_decoding = FALSE;
    t[i].valid = FALSE;
  }
}


int AddStation(t, name, sin_addr, video_encoding, audio_encoding, num, myself)
     LOCAL_TABLE t;
     char *name;
     u_long sin_addr;
     BOOLEAN video_encoding, audio_encoding;
     int *num;
     BOOLEAN myself;

{
  register int i;
  int firsti;
  STATION *np, *wp;
  BOOLEAN found=FALSE, free=FALSE;
  static int res;
  struct timeval realtime;
  double time;

  gettimeofday(&realtime, (struct timezone *)NULL);
  time = realtime.tv_sec + realtime.tv_usec/1000000.0;
  res=0;
  firsti=(myself ? 0 : 1);
  for(i=firsti; i<N_MAX_STATIONS; i++){
    np = &(t[i]);
    if((strcmp(np->name, name) == 0) && np->valid){
      wp = np;
      found = TRUE;
      *num = i;
      break;
    }
    if(!free){
      if(np->valid == FALSE){
	*num = i;
	wp = np;
	free = TRUE;
      }
    }
  }
  if(found){
    /*
     *  This participant is known, save if necessary its new parameters.
     */
    wp->lasttime = (int)time;
    if(wp->video_encoding != video_encoding){
      if(!video_encoding)
	res |= UNMAPP_VIDEO;
      if(wp->video_decoding){
	res |= ABORT_VIDEO;
	wp->video_decoding=FALSE;
      }else if(video_encoding)
	res |= MAPP_VIDEO;
      wp->video_encoding = video_encoding;
    }
    if(wp->audio_encoding != audio_encoding){
      if(!audio_encoding)
	res |= UNMAPP_AUDIO;
      if(wp->audio_decoding){
	res |= ABORT_AUDIO;
	wp->audio_decoding=FALSE;
      }else if (audio_encoding)
	res |= MAPP_AUDIO;
      wp->audio_encoding = audio_encoding;
    }
#ifdef DEBUG
      printf("Change %s station parameters on local table\n", wp->name);
#endif

  }else{
    if(free){
      /*
       * It's a new participant, save its parameters.
       */
      strcpy(wp->name, name);
      wp->sin_addr = sin_addr;
      wp->video_encoding = video_encoding;
      wp->audio_encoding = audio_encoding;
      wp->video_decoding = FALSE;
      wp->audio_decoding = FALSE;
      wp->valid = TRUE;
      wp->lasttime = (int)time;
      res = NEW_STATION;
      if(video_encoding)
	res |= MAPP_VIDEO;
      if(audio_encoding)
	res |= MAPP_AUDIO;
#ifdef DEBUG
      printf("Station %s added on local table\n", wp->name);
#endif
    }else{
      /* Local table is full */
      fprintf(stderr, 
	      "Received a call from %s, Couldn't add him on local table\n",
	      name);
    }
  }
  return(res);
}


BOOLEAN ShallIDecode(t, sin_addr)
     LOCAL_TABLE t;
     u_long sin_addr;
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    if(t[i].sin_addr == sin_addr){
      if((t[i].audio_decoding) && (t[i].valid))
	return(TRUE);
    }
  }
  return(FALSE);
}



int RemoveStation(t, sin_addr, num)
     LOCAL_TABLE t;
     u_long sin_addr;
     int *num;

{
  register int i;
  STATION *p;
  static int res;

  res=0;
  for(i=0; i<N_MAX_STATIONS; i++){
    p = &(t[i]);
    if(p->sin_addr == sin_addr){
      *num = i;
      res = KILL_STATION;
      p->valid = FALSE;
      if(p->video_decoding)
	res |= ABORT_VIDEO;
      if(p->audio_decoding)
	res |= ABORT_AUDIO;
#ifdef DEBUG
      printf("Station %s removed from local table\n", p->name);
#endif
      return(res);
    }
  }
  fprintf(stderr, "Received a unknown RemoveHost Packet \n");
  return(0);
}


CheckStations(t, time)
     LOCAL_TABLE t;
     int time;
{
  register int i;
  STATION *p;
  BOOLEAN StationKilled=FALSE;
  extern void AbortScalesVideo(), DestroyTabForm(), CreateTabForm();
  extern SendPP_QUIT(), SendPP_NEW_TABLE();
  extern int da_fd[2], dv_fd[N_MAX_STATIONS][2];
  
  for(i=1; i<N_MAX_STATIONS; i++){
    p = &(t[i]);
    if(p->valid){
      if((time - p->lasttime) > 60){
	printf("Station %s not responding, removed it from local table\n",
	       p->name);
	StationKilled=TRUE;
	p->valid = FALSE;
	if(p->video_decoding){
	  AbortScalesVideo();
	  SendPP_QUIT(dv_fd[i]);
	}
	if(p->audio_decoding)
	  SendPP_NEW_TABLE(t, da_fd);
      }
    }
  }
  if(StationKilled){
    DestroyTabForm();
    CreateTabForm();
  }
}



InitStation(p, name, enc_audio, enc_video)
     STATION *p;
     char *name;
     BOOLEAN enc_audio, enc_video;
{
  strcpy(p->name, name);
  p->audio_encoding = enc_audio;
  p->video_encoding = enc_video;
  p->sin_addr = 0;
}


PrintTable(F, t)
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

PrintStation(F, p, i)
     FILE *F;
     STATION *p;
     int i;
{
  fprintf(F, "\nSTATION %d ::\n", i);
  fprintf(F, "Name: %s\n", p->name);
  fprintf(F, "audio_encoding : %s\n", (p->audio_encoding ? "Yes" : "No"));
  fprintf(F, "video_encoding : %s\n", (p->video_encoding ? "Yes" : "No"));
  fprintf(F, "valid : %s\n", (p->valid ? "Yes" : "No"));
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
    fprintf(stderr, "Cannot create datagram socket\n");
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
  int sock;
  unsigned int iport, i1, i2, i3, i4;
  struct hostent *hp;
  char hostname[64];
  extern BOOLEAN UNICAST;
#ifdef MULTICAST
  int one=1;
  struct ip_mreq imr;
#endif 

  if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
    fprintf(stderr, "CreateReceiveSocket: Invalid group %s\n", group);
    return(-1);
  }

  *len_addr = sizeof(*addr);
  iport = atoi(port);
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    fprintf(stderr, "Cannot create datagram socket\n");
    exit(-1);
  }
  gethostname(hostname, 64);
  if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "Unknown host %s\n", hostname);
      exit(0);
  }
  if(UNICAST){
    bzero((char *) addr, *len_addr);
    bcopy(hp->h_addr, (char *)addr, sizeof(addr));
  }else{
#ifdef MULTICAST
    bcopy(hp->h_addr, (char *)&imr.imr_interface.s_addr, hp->h_length);
    imr.imr_multiaddr.s_addr = (i4<<24) | (i3<<16) | (i2<<8) | i1;
    imr.imr_multiaddr.s_addr = htonl(imr.imr_multiaddr.s_addr);
/*
    imr.imr_interface.s_addr = htonl(imr.imr_interface.s_addr);
*/
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr,
		  sizeof(struct ip_mreq)) == -1){
      fprintf(stderr, "Cannot join group %s\n", group);
      perror("setsockopt()");
      exit(1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#else
    fprintf(stderr, "MULTICAST mode not implemented, use unicast option\n");
    exit(-1);
#endif /* MULTICAST */
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons((u_short)iport);
  addr->sin_addr.s_addr = INADDR_ANY;
  if (bind(sock, (struct sockaddr *) addr, *len_addr) < 0){
    fprintf(stderr, "Cannot bind socket to %d port\n", iport);
    exit(1);
  }
  return(sock);
}


int SendDeclarePacket(s, addr, len_addr, video_encoding, audio_encoding, name)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
     BOOLEAN video_encoding, audio_encoding;
     char *name;
{
  unsigned char buf[10];
  int len, lw;

  buf[0] = 0xc0;
  if(video_encoding)
    buf[0] |= 2;
  if(audio_encoding)
    buf[0] |= 1;
  strcpy((char *) &(buf[1]), name);
  len = strlen(name) + 1;
  buf[len++] = 0;
  if((lw=sendto(s, (char *)buf, len, 0, (struct sockaddr *)addr, len_addr)) 
     != len){
    fprintf(stderr, "Cannot send Declaration Packet on socket\n");
    fprintf(stderr, "sendto returns %d instead of %d\n", lw, len);
    return(0);
  }
  return(1);
}

SendAskInfoPacket(s, addr, len_addr)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
{
  unsigned char buf[2];
  int lw;

  buf[0] = 0xc0;
  buf[0] |= (P_ASK_INFO << 2);
  if((lw=sendto(s, (char *)buf, 1, 0, (struct sockaddr *)addr, len_addr)) 
     != 1){
    fprintf(stderr, "Cannot send AskInfo Packet on socket\n");
    fprintf(stderr, "sendto returns %d instead of 1\n", lw);
    return(0);
  }
  return(1);
  
}


SendAbortPacket(s, addr, len_addr)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
{
  unsigned char buf[2];
  int lw;

  buf[0] = 0xc0;
  buf[0] |= (P_ABORT << 2);
  if((lw=sendto(s, (char *)buf, 1, 0, (struct sockaddr *)addr, len_addr)) 
     != 1){
    fprintf(stderr, "Cannot send Abort Packet on socket\n");
    fprintf(stderr, "sendto returns %d instead of 1\n", lw);
    return(0);
  }
  return(1);
  
}


int ItIsTheEnd(fd)
     int fd[2];
  {
    struct timeval tim_out;
    int mask;

    mask = 1<<fd[0];
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
	      &tim_out) != 0)
      return(1);
    else
      return(0);
  }
    



SendPP_QUIT(fd)
     int fd[2];
{
  u_char buf=0;

  if(write(fd[1], (char *)&buf, 1) < 0){
#ifdef DEBUG
    fprintf(stderr, "Cannot send PP_QUIT\n");
#endif
  }
}

SendPP_NEW_TABLE(t, fd)
     LOCAL_TABLE t;
     int fd[2];
{
  u_char *buf;
  int lbuf;
  int lw;

  lbuf = N_MAX_STATIONS*sizeof(STATION);
  buf = (u_char *)malloc(lbuf+1);
  buf[0] = PP_NEW_TABLE;
  bcopy((char *)t, (char *)&buf[1], lbuf);
  if((lw=write(fd[1], (char *)buf, lbuf+1)) < 0){
    fprintf(stderr, "Cannot send PP_NEW_TABLE\n");
  }
  free(buf);
}


int GetType(buf)
     u_char *buf;
{
  return(buf[0] >> 6);
}


int GetSubType(buf)
     u_char *buf;
{
  return((buf[0] >> 2) & 0x0f);
}


BOOLEAN ChangeAudioDecoding(t, name, audio_decoding)
     LOCAL_TABLE t;
     char *name;
     BOOLEAN audio_decoding;
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    if((strcmp(name, t[i].name) == 0) && t[i].valid){
      if(t[i].audio_decoding != audio_decoding){
	t[i].audio_decoding = audio_decoding;
	return(TRUE);
      }else
	break;
    }
  }
  return(FALSE);
}


BOOLEAN ChangeVideoDecoding(t, name, video_decoding)
     LOCAL_TABLE t;
     char *name;
     BOOLEAN video_decoding;
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    if((strcmp(name, t[i].name) == 0) && (t[i].valid)){
      if(t[i].video_decoding != video_decoding){
	t[i].video_decoding = video_decoding;
	return(TRUE);
      }else
	break;
    }
  }
  return(FALSE);
}

