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
*  Fichier : audio_decoder.c                                               *
*  Date    : July 1992                                                     *
*  Auteur  : Thierry Turletti                                              *
*  Release : 1.1                                                           *
*--------------------------------------------------------------------------*
*  Description :  Audio decoder.                                           *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "in.h"
#include <netdb.h>
#include <fcntl.h>
/*#include "ulaw2linear.h"*/
#include "protocol.h"
#include "audio_encode.h"
#include "audio_hdr.h"

#define maxi(x,y) ((x)>(y) ? (x) : (y))
#define T_MAX 10000

 


static void PlayAudio(msg)
  struct soundbuf *msg;
{
    char *val;
    int len;
    char auxbuf[8002];
    static char encoding[20];
    register int i;
    register char *ab;
    static struct{
      int len;
      struct audio_g72x_state state;
    } g721, g723;
    static Audio_hdr audio_hdr = {8000, 1, 1, 1, AUDIO_ENCODING_ULAW, 
				  AUDIO_UNKNOWN_SIZE};
    static BOOLEAN FIRST=TRUE;

    if(FIRST){
      g721_init_state(&g721.state);
      g723_init_state(&g723.state);
      FIRST=FALSE;
    }

#ifdef DEBUG
    switch(msg->compression){
    case PCM_64: strcpy(encoding, "PCM 64 kb/s");break;
    case PCM_32: strcpy(encoding, "PCM 32 kb/s");break;
    case G_721: strcpy(encoding, "ADPCM 32 kb/s");break;
    case G_723: strcpy(encoding, "ADPCM 24 kb/s");break;
    }
    printf("PlayAudio: Playing %d bytes (%s).\n", msg->buffer.buffer_len,
	   encoding);
#endif DEBUG

    len = msg->buffer.buffer_len;
    val = msg->buffer.buffer_val;

    /* If the buffer contains compressed sound samples, reconstruct an
       approximation of the original  sound  by  performing  a	linear
       interpolation between each pair of adjacent compressed samples.
       Note that since the samples are logarithmically scaled, we must
       transform  them	to  linear  before interpolating, then back to
       log before storing the calculated sample. */

    switch(msg->compression){
    case PCM_64:
      break;
    case PCM_32:
      ab = auxbuf;
      for (i=0; i<len; i++){
	*ab++ = (i==0 ? *val : 
		 (audio_s2u((audio_u2s(*val) + audio_u2s(val[-1])) / 2)));
	*ab++ = *val++;
      }
      len *= 2;
      val = auxbuf;
      break;
    case G_721:
      g721_decode(auxbuf, len, &audio_hdr, auxbuf, &g721.len, &g721.state);
      len = g721.len;
      val = auxbuf;
      break;
    case G_723:
      g723_decode(auxbuf, len, &audio_hdr, auxbuf, &g723.len, &g723.state);
      len = g723.len;
      val = auxbuf;
      break;
    }
    soundplay(len, val);
}




void Audio_Decode(fd, port, group)
     int fd[2];
     char *port, *group;
{
  struct sockaddr_in addr;
  int len_addr;
  int sock;
  int mask_pipe, mask_sock, mask, smask, mask0, fmax;
  char buf[T_MAX];
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

  if(!soundinit(O_WRONLY)) {
    fprintf(stderr, "cannot opening audio output device");
    exit(-1);
  }

  do{
    mask = mask0;
    smask = mask_sock;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		  (struct timeval *)0) > 0){

      if((n=select(sock+1, (fd_set *)&smask, (fd_set *)0, (fd_set *)0, 
		  (struct timeval *)&tim)) > 0){ 
	do{
	  if ((lr=recvfrom(sock, (char *)buf, T_MAX, 0, &addr, &len_addr)) < 0)
	    fprintf(stderr, "audio_decode: error recvfrom on sock");
	  if((type=GetType(buf)) != AUDIO_TYPE){
	    fprintf(stderr, 
		    "audio_decode: received and dropped a packet type %d\n", 
		    type);
	  }else{
	    if(ShallIDecode(table, addr.sin_addr.S_un.S_addr)){
	      PlayAudio((struct soundbuf *) buf);
#ifdef DEBUG
	      printf("audio_decode: Playing %d bytes\n", lr);
#endif
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
#ifdef DEBUG
	      printf("audio_decode: Received a PP_QUIT\n");
#endif
	      soundterm();
	      exit(0);
	    case PP_NEW_TABLE:
	      if ((lr=read(fd[0], table, sizeof(table))) < 0) {
		fprintf(stderr, 
			"audio_decode : Cannot read on pipe, Abort decoding");
	      }else{
#ifdef DEBUG
		printf("audio_decode: received new audio table %d bytes\n", 
		       lr);
#endif          ;
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










    

