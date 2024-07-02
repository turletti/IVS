/**************************************************************************\
*          Copyright (c) 1995 INRIA Sophia Antipolis, FRANCE.              *
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
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  A set of procedures used by a lot of modules...          *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include "general_types.h"
#include "protocol.h"
#include "rtp.h"

#ifdef CRYPTO
#include "ivs_secu.h"
#endif /* CRYPTO */

/*------------------------ LOCAL TABLE FUNCTIONS ----------------------------*/

void ResetStation(station, AUDIO_IGNORED)
     STATION *station;
     BOOLEAN AUDIO_IGNORED;
{
  bzero((char *)station, sizeof(STATION));
  return;
}



void ResetTable(t, ALL_AUDIO_IGNORED)
     LOCAL_TABLE t;
     BOOLEAN ALL_AUDIO_IGNORED;
{
  register int i;
  

  bzero((char *)t, N_MAX_STATIONS*sizeof(STATION));
  return;
}



void InitStation(p, name, sin_addr, sin_addr_coder, identifier, feedback_port,
		 AUDIO_ENCODING, VIDEO_ENCODING, AUDIO_IGNORED, lasttime)
     STATION *p;
     char *name;
     u_long sin_addr, sin_addr_coder;
     u_short identifier, feedback_port;
     BOOLEAN AUDIO_ENCODING, VIDEO_ENCODING, AUDIO_IGNORED;
     long lasttime;
{
  p->valid = TRUE;
  p->sin_addr = sin_addr;
  p->sin_addr_coder = sin_addr_coder;
  p->identifier = identifier;
  p->feedback_port = feedback_port;
  strcpy(p->name, name);
  p->video_encoding = VIDEO_ENCODING;
  p->video_decoding = FALSE;
  p->lasttime = lasttime;
/*  NewStation(sin_addr,identifier,AUDIO_ENCODING,FALSE,AUDIO_IGNORED);*/
  p->dv_fd[0] = p->dv_fd[1] = 0;
  return;
}



void CopyStation(out, in)
     STATION *out, *in;
{
  
  out->valid = in->valid;
  out->sin_addr = in->sin_addr;
  out->sin_addr_coder = in->sin_addr_coder;
  out->identifier = in->identifier;
  out->feedback_port = in->feedback_port;
  strcpy(out->name, in->name);
  out->video_encoding = in->video_encoding;
  out->video_decoding = in->video_decoding;
  out->lasttime = in->lasttime;
  out->dv_fd[0] = in->dv_fd[0];
  out->dv_fd[1] = in->dv_fd[1];
  return;
}

void PrintStation(F, p, i)
     FILE *F;
     STATION *p;
     int i;
{
  fprintf(F, "\nSTATION %d ::\n", i);
  fprintf(F, "Name: %s\n", p->name);
  fprintf(F, "sin_addr_from: %d\n", p->sin_addr);
  fprintf(F, "sin_addr_coder: %d\n", p->sin_addr_coder);
  fprintf(F, "identifier: %d\n", p->identifier);
  fprintf(F, "video_encoding : %s\n", (p->video_encoding ? "Yes" : "No"));
  fprintf(F, "video_decoding : %s\n", (p->video_decoding ? "Yes" : "No"));
  fprintf(F, "valid : %s\n", (p->valid ? "Yes" : "No"));
  fprintf(F, "lasttime: %u\n", p->lasttime);
  return;
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
  return;
}


/*--------------------- SESSION PACKETS SENDING PROCEDURES ------------------*/

int SendDESCRIPTOR(s, addr, len_addr, video_encoding, audio_encoding, 
		   feedback_port, conf_size, sin_addr, name, class_name)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
     BOOLEAN video_encoding, audio_encoding;
     u_short feedback_port;
     int conf_size;
     u_long sin_addr;
     char *name;
     u_short class_name;
{
  static u_int buf[((LEN_MAX_NAME+38) / 4) + 1];
  static int total_length;
  int len_name, lw;
  rtp_hdr_t *h;
  rtp_t *r;
  static BOOLEAN FIRST=TRUE;
  int len_packet;
  char *buf_to_send;
#ifdef CRYPTO
  u_int buf_crypt[((LEN_MAX_NAME+38) / 4) + 3];
#endif /* CRYPTO */
  
  if(FIRST){
    /* Set the fixed RTP header */
    h = (rtp_hdr_t *) buf;
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    /* Set the APP_IVS RTP option */
    r = (rtp_t *) &buf[2];
    r->app_ivs.final = 0;
    r->app_ivs.type = RTP_APP;
    r->app_ivs.length = 3;
    r->app_ivs.subtype = 0;
    strncpy((char *) &(r->app_ivs.name), "IVS ", 4);
    r->app_ivs.ver = RTP_IVS_VERSION;
    r->app_ivs.audio = audio_encoding;
    r->app_ivs.video = video_encoding;
    r->app_ivs.conf_size = conf_size;
    /* Set the SDES RTP option */
    r = (rtp_t *) &buf[5];
    len_name = strlen(name);
    r->sdes.final = 1;
    r->sdes.type = RTP_SDES;
    r->sdes.length = (u_short)((19+len_name)%4 == 0 ? (19+len_name)/4 :
			       (19+len_name)/4 +1);
    total_length = 4*(5 + r->sdes.length);
    r->sdes.id = (u_short)0;
    r->sdes.ptype = RTP_TYPE_PORT;
    r->sdes.plength = 1;
    r->sdes.port = feedback_port;
    r->sdes.atype = RTP_TYPE_ADDR;
    r->sdes.alength = 2;
    r->sdes.adtype = RTP_IPv4;
    r->sdes.addr = sin_addr;
    r->sdes.ctype = class_name;
    r->sdes.clength = r->sdes.length - 4;
    strcpy((char *) r->sdes.name, name);
    FIRST = FALSE;
  }else{
    /* Set the APP_IVS RTP option */
    r = (rtp_t *) &buf[2];
    r->app_ivs.audio = audio_encoding;
    r->app_ivs.video = video_encoding;
    r->app_ivs.conf_size = conf_size;
  }
  len_packet = total_length;
  buf_to_send = (char *)buf;
#ifdef CRYPTO
  if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
    test_key("SendDESCRIPTOR");
#endif
    len_packet = method[current_method].crypt_rtcp(
	(char *)buf, len_packet, s_key, len_key, (char *)buf_crypt);
    buf_to_send = (char *)buf_crypt;
  }
#endif /* CRYPTO */  
  if((lw=sendto(s, buf_to_send, len_packet, 0, (struct sockaddr *)addr,
		len_addr)) != len_packet){
    perror("send DESCRIPTOR");
    return(0);
  }
  return(1);
}



SendBYE(s, addr, len_addr)
     int s;
     struct sockaddr_in *addr;
     int len_addr;
{
  static u_int buf[3];
  static BOOLEAN FIRST=TRUE;
  rtp_hdr_t *h;
  rtp_t *r;
  int lw;
  int len_packet;
  char *buf_to_send;
#ifdef CRYPTO
  u_int buf_crypt[5];
#endif /* CRYPTO */  

  if(FIRST){
    /* Set the fixed RTP header */
    h = (rtp_hdr_t *) buf;
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    /* Set the BYE RTP option */
    r = (rtp_t *) &buf[2];
    r->bye.final = 1;
    r->bye.type = RTP_BYE;
    r->bye.length = 1;
    r->bye.id = 0;
    FIRST = FALSE;
  }
  len_packet = 12;
  buf_to_send = (char *) buf;
#ifdef CRYPTO
  if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
    test_key("SendBYE");
#endif 
    len_packet = method[current_method].crypt_rtcp(
	(char *)buf, len_packet, s_key, len_key, (char *)buf_crypt);
    buf_to_send = (char *)buf_crypt;
  }
#endif /* CRYPTO */  
  if((lw=sendto(s, buf_to_send, len_packet, 0, (struct sockaddr *)addr, 
		len_addr)) != len_packet){
    perror("send BYE");
    return(0);
  }
  return(1);
}


/*-------------------- REVERSE PACKETS SENDING PROCEDURES -------------------*/
  
void SendFIR(addr, len_addr)
     struct sockaddr_in *addr;
     int len_addr;
{
  static int sock_report;
  static BOOLEAN FIRST=TRUE;
  static u_int buf[3];
  rtp_hdr_t *h;
  rtp_t *r;
  int lw;

  if(FIRST){
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    /* Set the fixed RTP header */
    h = (rtp_hdr_t *) buf;
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    /* Set the FIR RTP option */
    r = (rtp_t *) &buf[2];
    r->fir.final = 1;
    r->fir.type = RTP_FIR;
    r->fir.length = 1;
    FIRST = FALSE;
  }    
  if((lw=sendto(sock_report, (char *)buf, 12, 0,
		(struct sockaddr *)addr, len_addr)) != 12){
    perror("Send FIR");
  }
  return;
}




void SendNACK(addr, len_addr, F, fgobl, lgobl, size, ts_sec, ts_usec)
     struct sockaddr_in *addr;
     int len_addr;
     BOOLEAN F; /* False is many NACKs are encoded in the same packet. */
     int fgobl; /* First GOB Lost */
     int lgobl; /* Last GOB Lost */
     int size;   /* Image format */
     u_short ts_sec, ts_usec;

{
  static int sock_report;
  static BOOLEAN FIRST=TRUE;
  static u_int32 buf[64];
  static int k=2;
  rtp_hdr_t *h;
  rtp_t *r;
  int lw;

  if(FIRST){
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    /* Set the fixed RTP header */
    h = (rtp_hdr_t *) buf;
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    FIRST = FALSE;
  }
  /* Set the NACK RTP option */
  r = (rtp_t *) &buf[k];
  r->nack.final = F;
  r->nack.type = RTP_NACK;
  r->nack.length = 3;
  r->nack.fgobl = fgobl;
  r->nack.lgobl = lgobl;
  r->nack.size = size;
  r->nack.ts_sec = htons(ts_sec);
  r->nack.ts_usec = htons(ts_usec);
  if(F){
    if((lw=sendto(sock_report, (char *)buf, (4*k)+12, 0, (struct sockaddr *)
		  addr, len_addr)) != ((4*k)+12)){
      perror("Send NACK");
      k = 2;
      return;
    }
#ifdef DEBUG    
    printf("Sending NACK: fgobl:%d, lgobl:%d, lw=%d, ts=%u\n", fgobl, lgobl,
	   lw, ts_usec);
#endif    
    k = 2;
  }else{
    k += 3;
  }
  return;
}





/*---------------------------- PIPE UTILITIES -------------------------------*/

BOOLEAN HandleCommands(param, TUNING)
     S_PARAM *param;
     BOOLEAN *TUNING;
     
{
  struct timeval tim_out;
  int mask;
  u_int8 command[500];
  u_int16 nb_stations;
  int lr, i=0;
  int fd = param->f2s_fd[0];
#ifdef CRYPTO
  register int j;
#endif

  mask = 1 << fd;
  tim_out.tv_sec = 0;
  tim_out.tv_usec = 0;
  if(select(fd+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
	    &tim_out) != 0){
    if((lr=read(fd, (char *)command, 500)) <= 0){
      perror("HandleCommands'read ");
      return FALSE;
    }
    do{
      switch(command[i]){
      case PIP_QUIT: /* One byte message */
	return FALSE;
      case PIP_SIZE: /* Two bytes message */
	param->video_size = (int)command[++i];
	i ++;
	break;
      case PIP_FREEZE: /* One byte message */
	param->FREEZE = TRUE;
	i ++;
	break;
      case PIP_UNFREEZE: /* One byte message */
	param->FREEZE = FALSE;
	i ++;
	break;
      case PIP_VIDEO_PORT: /* Two bytes message */
	param->video_port = (int)command[++i];
	i ++;
	break;
#ifdef VIGRAPIX
      case PIP_VIDEO_FORMAT: /* One byte message */
	param->secam_format = !param->secam_format;
	i ++;
	break;
#endif /* VIGRAPIX */
      case PIP_MAX_RATE: /* Five bytes message */
	sscanf((char *) &command[++i], "%d", &(param->rate_max));
	i += 4;
	break;
      case PIP_RATE_CONTROL: /* Two bytes message */
	param->rate_control = (int)command[++i];
	i ++;
	break;
      case PIP_PRIVILEGE_QUALITY: /* One byte message */
	param->PRIVILEGE_QUALITY = TRUE;
	i ++;
	break;
      case PIP_PRIVILEGE_FR: /* One byte message */
	param->PRIVILEGE_QUALITY = FALSE;
	i ++;
	break;
      case PIP_MAX_FR_RATE: /* One byte message */
	param->MAX_FR_RATE_TUNED = TRUE;
	i ++;
	break;
      case PIP_NO_MAX_FR_RATE: /* One byte message */
	param->MAX_FR_RATE_TUNED = FALSE;
	i ++;
	break;
      case PIP_FRAME_RATE: /* Five bytes message */
	sscanf((char *) &command[++i], "%d", &(param->max_frame_rate));
	i += 4;
	break;
      case PIP_CONF_SIZE: /* Two bytes message */
	param->conf_size = (int)command[++i];
	i ++;
	break;
      case PIP_NB_STATIONS: /* Three bytes message (last byte is 0) */
	nb_stations = command[++i] << 8;
	nb_stations += command[++i];
	param->nb_stations = nb_stations;
	i ++;
	break;
      case PIP_LDISP: /* One byte message */
	param->LOCAL_DISPLAY = TRUE;
	i ++;
	break;
      case PIP_NOLDISP: /* One byte message */
	param->LOCAL_DISPLAY = FALSE;
	i ++;
	break;
      case PIP_TUNING: /* 9 bytes message */
	*TUNING = TRUE;
	sscanf((char *) &command[++i], "%d", &(param->brightness));
	i += 4; /* 4 bytes */ 
	sscanf((char *) &command[i], "%d", &(param->contrast)); 
	i += 4; /* 4 bytes */
	break;
#ifdef CRYPTO
      case PIP_ENCRYPTION: /* variable length message */
	current_method = (int) command[++i];
	len_key = (int) command[++i];
	for(j=0; j<len_key; j++)
	    s_key[j] = command[++i];
	i ++;
#ifdef DEBUG_CRYPTO
	test_key("HandleCommand");
#endif 
	break;
#endif /* CRYPTO */
      }
    }while(lr>i);
  }
  return(TRUE);
}
      


void SendPIP_QUIT(fd)
     int fd[2];
{
  u_int8 buf=PIP_QUIT;

  if(write(fd[1], (char *)&buf, 1) != 1){
#ifdef DEBUG
    perror("SendPIP_QUIT'write");
#endif
  }
  return;
}



BOOLEAN SendPIP_FORK(fd)
     int fd[2];
{
  u_int8 buf=PIP_FORK;

  if(write(fd[1], (char *)&buf, 1) != 1){
    perror("SendPIP_FORK'write");
    return(FALSE);
  }
  return(TRUE);
}



BOOLEAN SendPIP_PAUSE(fd)
     int fd[2];
{
  u_int8 buf=PIP_PAUSE;

  if(write(fd[1], (char *)&buf, 1) != 1){
    perror("SendPIP_PAUSE'write");
    return(FALSE);
  }
  return(TRUE);
}



BOOLEAN SendPIP_CONT(fd)
     int fd[2];
{
  u_int8 buf=PIP_CONT;

  if(write(fd[1], (char *)&buf, 1) != 1){
    perror("SendPIP_CONT'write");
    return(FALSE);
  }
  return(TRUE);
}


#ifdef CRYPTO
BOOLEAN SendPIP_ENCRYPTION(fd)
     int fd[2];
{
  u_int8 buf[34];
  register u_int8 *pt_buf;
  register int i;
  int len_msg;

  pt_buf = (u_int8 *) buf;
  *pt_buf++ = PIP_ENCRYPTION;
  *pt_buf++ = (u_int8) current_method;
  *pt_buf++ = (u_int8) len_key;
  for(i=0; i<len_key; i++)
    *pt_buf++ = s_key[i];
  len_msg = len_key + 3;
  if(write(fd[1], buf, len_msg) != len_msg){
    perror("SendPIP_CONT'write");
    return(FALSE);
  }
  return(TRUE);
}
#endif /* CRYPTO */


/*--------------------------- IVS DAEMON  UTILITIES -------------------------*/


int SendTalkRequest(sock)
     int sock;
{
#if defined(POSIX_SOURCE) || defined(__NetBSD__)
  const char *getlogin();
#endif
  int lw, len;
  char data[10];
  char name[9];

  bzero(data, 10);
  data[0] = (u_int8) CALL_REQUESTED;
#if defined(POSIX_SOURCE) || defined(__NetBSD__)
  strcpy(name, getlogin());
#else
  cuserid(name);
#endif
  len = strlen(name) + 1;
  strcpy(&data[1], name);
  if((lw = write(sock, data, len)) != len)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkAbort(sock)
     int sock;
{
  int lw;
  char data;

  data = (u_int8) CALL_ABORTED;
  if((lw = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkAccept(sock)
     int sock;
{
  int lw;
  char data;

  data = (u_int8) CALL_ACCEPTED;
  if((lw = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


int SendTalkRefuse(sock)
     int sock;
{
  int lw;
  char data;

  data = (u_int8) CALL_REFUSED;
  if((lw = write(sock, &data, 1)) != 1)
    return(FALSE);
  else
    return(TRUE);
}
     


/* --------------------- Audio Stuff .... ---------------------------*/

