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
*  File              : audio_decoder.c                                     *
*  Author            : Thierry Turletti                                    *
*  Last Modification : 3/31/1993                                           *
*--------------------------------------------------------------------------*
*  Description :  Audio decoder.                                           *
*                                                                          *
\**************************************************************************/

#ifndef DECSTATION

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"


#define maxi(x, y) ((x) > (y) ? (x) : (y))

#define DiffTime(new, old)  \
  (1000*((new).tv_sec - (old).tv_sec) + ((new).tv_usec - (old).tv_usec)/1000)
 


static void PlayAudio(inbuf, soundel, compression)
     unsigned char *inbuf;
     int soundel;
     u_char compression;
{
    unsigned char *val;
    int len;
    unsigned char outbuf[P_MAX];
    static char encoding[20];
    u_long queue_length;


#ifdef DEBUG
    switch(compression){
    case PCM_64: strcpy(encoding, "PCM 64 kb/s"); break;
    case ADPCM_32: strcpy(encoding, "ADPCM 32 kb/s"); break;
    case VADPCM: strcpy(encoding, "ADPCM variable"); break;
    }
    fprintf(stderr, "PlayAudio: Received %d bytes (%s).\n", soundel,
	   encoding);
#endif /* DEBUG */

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
    soundplay(len, val);
/*
    queue_length = soundplayqueue();
    if(queue_length > 8000)
#ifndef HPUX
      usleep(500000);
#else
      hp_usleep(500000);
#endif
    else{
      if(queue_length > 1000)

#ifndef HPUX
	usleep(125*len);
#else
      hp_usleep(125*len);
#endif
     }
*/
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
  int lr, n, lsent;
  u_long last_from;
  char header;
  int type;
  struct timeval tim, realtime, lasttime;
  AUDIO_TABLE table;
  u_char nb_stations = 0;
  BOOLEAN AUDIO_SET=FALSE;
  BOOLEAN DROPPED=FALSE;


  /*
  *  Initializations
  */
  sock = CreateReceiveSocket(&addr, &len_addr, port, group);
  tim.tv_sec = 0;
  tim.tv_usec = 0;
  mask_pipe = 1 << fd[0];
  mask_sock = 1 << sock;
  mask0 = mask_pipe | mask_sock;
  fmax = maxi(fd[0], sock) + 1;
  fromlen = sizeof(from);

  gettimeofday(&lasttime, (struct timezone *)NULL);
  lasttime.tv_sec -= 1;

  do{
    mask = mask0;
    smask = mask_sock;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		  (struct timeval *)0) > 0){
      if(mask & mask_sock){
	bzero((char *)&from, fromlen);
	if((lr=recvfrom(sock, (char *)buf, P_MAX, 0, 
			(struct sockaddr *)&from, &fromlen)) < 0){
	  perror("audio_decode (recvfrom) :");
	}
	if((type=GetType(buf)) != AUDIO_TYPE){
	  fprintf(stderr, 
		  "audio_decode: received and dropped a packet type %d\n", 
		  type);
	}else{
	  if(AUDIO_SET && (DROPPED == FALSE)){
	    if(ShallIDecode(table, nb_stations, from.sin_addr.s_addr)){
	      if(from.sin_addr.s_addr != last_from){
		gettimeofday(&realtime, (struct timezone *)NULL);
		if(DiffTime(realtime, lasttime) > 500){
		  lasttime.tv_sec = realtime.tv_sec;
		  lasttime.tv_usec = realtime.tv_usec;
		}else{
		  continue;
		}
	      }else{
		gettimeofday(&lasttime, (struct timezone *)NULL);
	      }
	      last_from = (u_long)from.sin_addr.s_addr;
	      PlayAudio(&buf[2], lr-2, buf[1]);
	    }
	  }
	}
      } /* fi mask_sock */
      if(mask & mask_pipe){
	if((lr=read(fd[0], &header, 1)) != 1){
#ifdef DEBUG
	  fprintf(stderr, 
		  "audio_decode: Cannot read on pipe, Abort decoding\n");
#endif
	  exit(1);
	}else{
	  switch((int)(header)){

	  case PIP_QUIT:
#if defined(SPARCSTATION) || defined(INDIGO)
#ifdef DEBUG
	    fprintf(stderr, "Receive a PIP_QUIT\n");
#endif
	    soundterm();
#endif /* SPARCSTATION OR INDIGO */
	    exit(0);

	  case PIP_NEW_TABLE:
	    if((lr=read(fd[0], (char *)&nb_stations, 1)) != 1){
#ifdef DEBUG
	      fprintf(stderr, 
		      "audio_decode: Cannot read on pipe, Abort decoding\n");
#endif
	      exit(1);
	    }
	    lsent = nb_stations*sizeof(AUDIO_STATION);

	    if((lr=read(fd[0], (char *)table, lsent)) != lsent) {
#ifdef DEBUG	      
	      fprintf(stderr, 
		      "audio_decode : Cannot read on pipe, Abort decoding\n");
#endif
	      exit(1);
	    }else{
#ifdef DEBUG
	      fprintf(stderr, 
		      "audio_decode: received new audio table %d bytes\n", 
		     lr);
	      for(n=0; n<nb_stations; n++)
		printf("Station %d: sin_addr:%d, decoding:%d\n", n,
		       table[n].sin_addr, table[n].audio_decoding);
	      fprintf(stderr, "\n");
#endif
	      if(!AnyAudioDecoding(table, nb_stations)){
		if(AUDIO_SET){
#ifdef DEBUG
		  fprintf(stderr, "audio device closed\n");		  
#endif
#if defined(SPARCSTATION) || defined(INDIGO)
		  soundterm();
		  AUDIO_SET = FALSE;
#endif
		}
	      }else{
		if(!AUDIO_SET){
#if defined(SPARCSTATION) || defined(INDIGO)
		  if(!soundinit(O_WRONLY)) {
		    fprintf(stderr, "cannot opening audio output device");
		    exit(-1);
		  }
#ifdef DEBUG
		  fprintf(stderr, "audio device opened\n");
#endif
#endif /* SPARCSTATION OR INDIGO */
		  AUDIO_SET = TRUE;
		}
	      }/* AnyAudioDecoding() ? */
	    }
	    break;

	  case PIP_DROP:
	    DROPPED = TRUE;
	    break;

	  case PIP_CONT:
	    DROPPED = FALSE;
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
#endif /* NOT DECSTATION */

