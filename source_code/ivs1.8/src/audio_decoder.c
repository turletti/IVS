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
*  File    : audio_decoder.c                                               *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Audio decoder.                                           *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"


#define maxi(x,y) ((x)>(y) ? (x) : (y))
 


static void PlayAudio(inbuf, soundel, compression)
     unsigned char *inbuf;
     int soundel;
     char compression;
{
    unsigned char *val;
    int len;
    unsigned char outbuf[P_MAX];
    static char encoding[20];


#ifdef DEBUG
    switch(compression){
    case PCM_64: strcpy(encoding, "PCM 64 kb/s"); break;
    case ADPCM_32: strcpy(encoding, "ADPCM 32 kb/s"); break;
    case VADPCM: strcpy(encoding, "ADPCM variable"); break;
    }
    printf("PlayAudio: Received %d bytes (%s).\n", soundel,
	   encoding);
#endif /*DEBUG*/

    switch(compression){
    case PCM_64:
      val=inbuf;
      len=soundel;
      break;
    case ADPCM_32:
      len = soundel*2;
      adpcm_decoder(inbuf, outbuf, len, FALSE);
      val = outbuf;
      break;
    case VADPCM:
      len=vadpcm_decoder(inbuf, outbuf, soundel, FALSE);
      val = outbuf;
      break;
    }
#ifdef SPARCSTATION
    soundplay(len, val);
#endif
}




void Audio_Decode(fd, port, group)
     int fd[2];
     char *port, *group;
{
  struct sockaddr_in addr, from;
  int len_addr, fromlen;
  int sock;
  int mask_pipe, mask_sock, mask, smask, mask0, fmax;
  unsigned char buf[P_MAX];
  int lr, n;
  char header;
  int type;
  struct timeval tim;
  LOCAL_TABLE table;

  /*
  *  Initializations
  */
  InitTable(table);
  sock = CreateReceiveSocket(&addr, &len_addr, port, group);
  tim.tv_sec = 0;
  tim.tv_usec = 0;
  mask_pipe = 1 << fd[0];
  mask_sock = 1 << sock;
  mask0 = mask_pipe | mask_sock;
  fmax = maxi(fd[0], sock) + 1;
  fromlen = sizeof(from);

#ifdef SPARCSTATION
  if(!soundinit(O_WRONLY)) {
    fprintf(stderr, "cannot opening audio output device");
    exit(-1);
  }
#endif 
  do{
    mask = mask0;
    smask = mask_sock;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		  (struct timeval *)0) > 0){

      if((n=select(sock+1, (fd_set *)&smask, (fd_set *)0, (fd_set *)0, 
		  (struct timeval *)&tim)) > 0){ 
	do{
	  bzero((char *)&from, fromlen);
	  if((lr=recvfrom(sock, (char *)buf, P_MAX, 0, &from, &fromlen)) < 0){
	    perror("audio_decode (recvfrom) :");
	  }
	  if((type=GetType(buf)) != AUDIO_TYPE){
	    fprintf(stderr, 
		    "audio_decode: received and dropped a packet type %d\n", 
		    type);
	  }else{
	    if(ShallIDecode(table, from.sin_addr.s_addr)){
	      PlayAudio(&buf[2], lr-2, buf[1]);
	    }
	  }
	}while(--n);
      }
      if(mask & mask_pipe){
	if((lr=read(fd[0], &header, 1)) != 1){
	  fprintf(stderr, 
		  "audio_decode: Cannot read on pipe, Abort decoding\n");
	  exit(1);
	}else{
	  switch((int)(header)){
	    case PP_QUIT:
#ifdef SPARCSTATION
	      soundterm();
#endif
	      exit(0);
	    case PP_NEW_TABLE:
	      if ((lr=read(fd[0], table, sizeof(table))) < 0) {
		fprintf(stderr, 
			"audio_decode : Cannot read on pipe, Abort decoding");
	      }else{
#ifdef DEBUG
		printf("audio_decode: received new audio table %d bytes\n", 
		       lr);
#endif
	      }
	      break;
	    default:
	      fprintf(stderr, 
		 "audio_decode: received on pipe an unknown packet type %d\n",
		      (u_long)buf[0]);
          }
	}
      }
    }
  }while(1);
}










    

