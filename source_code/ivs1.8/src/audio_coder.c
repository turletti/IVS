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
*  File    : audio_coder.c                                                 *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Audio coder.                                             *
*                                                                          *
\**************************************************************************/
#ifdef SPARCSTATION

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"



void audio_encode(group, port, ttl, encoding, fd)
     char *group, *port;
     u_char ttl;
     int encoding;
     int fd[2];
{
  int sock, len_addr;
  struct sockaddr_in addr;
  int squelch = 30; 	/* Squelch level if > 0 */
  int len;
  unsigned char inbuf[P_MAX], outbuf[P_MAX];
  unsigned char *buf;
  int lbuf=1024;


  sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
  outbuf[0] = 0x80;
  outbuf[1] = (u_char)encoding;

  if (!soundinit(O_RDONLY)) {
    fprintf(stderr, "audio_decode: Cannot open audio device.\n");
    exit(1);
  }

  if (soundrecord()) 
  {
    struct timeval tim;
    int mask, mask0;
    
    mask0 = 1 << fd[0];
    tim.tv_sec = 0;
    tim.tv_usec = 0;

    if(encoding == PCM_64){
      buf = &outbuf[2];
    }else{
      buf = inbuf;
    }

    do{
      int soundel;
      
      /* Get the sound ... */
      soundel = soundgrab(buf, lbuf);

      mask = mask0;
      if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim) > 0){
	exit(0);
      }
      if (soundel > 0) {
	register unsigned char *start = buf;
	register int i;
	int squelched = (squelch > 0);

	/* If entire buffer is less than squelch, ditch it. */
	if (squelch > 0) {
	  for (i=0; i<soundel; i++) {
	    if (((*start++ & 0x7F) ^ 0x7F) > squelch) {
	      squelched = 0;
	      break;
	    }
	  }
	}
	
	if (!squelched){
	  switch(encoding){
	  case PCM_64:
	    break;
	  case ADPCM_32:
	    adpcm_coder(inbuf, &outbuf[2], soundel);
	    soundel /= 2;
	    break;
	  case VADPCM:
	    soundel = vadpcm_coder(inbuf, &outbuf[2], soundel);
	    break;
	  }
	  if (sendto(sock, outbuf, soundel+2, 0, (struct sockaddr *) &addr, 
		     len_addr) < 0) {
	    perror("audio_coder: when sending datagram packet");
	  }
	}
      } 
    }while(1);
  }
}


#endif /* SPARCSTATION */
