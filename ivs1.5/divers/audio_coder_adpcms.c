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
*  Fichier : audio_coder.c                                                 *
*  Date    : July 1992                                                     *
*  Auteur  : Thierry Turletti                                              *
*  Release : 1.1                                                           *
*--------------------------------------------------------------------------*
*  Description :  Audio coder.                                             *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "in.h"
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"
#include "audio_encode.h"
#include "audio_hdr.h"



void audio_encode(group, port, ttl, encoding, fd)
     char *group, *port;
     u_char ttl;
     int encoding;
     int fd[2];
{
  int sock, len_addr;
  struct sockaddr_in addr;
  int squelch = 40; 	/* Squelch level if > 0 */
  struct soundbuf netbuf;
#define abuf netbuf.buffer.buffer_val
  static struct{
    int len;
    struct audio_g72x_state state;
  } g721, g723;
  int len;
  static Audio_hdr audio_hdr = {8000, 1, 1, 1, AUDIO_ENCODING_ULAW, 
				  AUDIO_UNKNOWN_SIZE};

  
  sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
  netbuf.header = 0x80;
  netbuf.compression = (u_char)encoding;

  if (!soundinit(O_RDONLY /* | O_NDELAY */ )) {
    fprintf(stderr, "Unable to initialise audio.\n");
    exit(1);
  }

  if(encoding == G_721){
    g721_init_state(&g721.state);
  }else if(encoding == G_723){
    g723_init_state(&g723.state);
  }

  if (soundrecord()) {
    struct timeval tim;
    int mask, mask0;
    
    mask0 = 1 << fd[0];
    tim.tv_sec = 0;
    tim.tv_usec = 0;
    
    do{
      int soundel = soundgrab(abuf, sizeof abuf);
      unsigned char *bs = (unsigned char *) abuf;
      
      mask = mask0;
      if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim) > 0){
	printf("son: Received End of encoding\n");
	exit(0);
      }
      if (soundel > 0) {
	register unsigned char *start = bs;
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
	  case PCM_32:
	    soundel /= 2;
	    for (i=1; i<soundel; i++) {
	      abuf[i] = abuf[i * 2];
	    }
	    break;
	  case G_721:
	    printf("G.721 encoding, soundel= %d ...\n", soundel);
	    g721_encode(abuf, soundel, &audio_hdr, 
			(u_char *)abuf, &g721.len, &g721.state);
	    soundel = g721.len;
	    printf("len after encoding : %d\n", soundel);
	    break;
	  case G_723:
	    printf("G.723 encoding, soundel= %d ...\n", soundel);
	    g723_encode(abuf, soundel, &audio_hdr, 
			(u_char *)abuf, &g723.len, &g723.state);
	    soundel = g723.len;
	    printf("len after encoding : %d\n", soundel);
	    break;
	  }
	  netbuf.buffer.buffer_len = soundel;
	  if (sendto(sock, (char *)&netbuf, (sizeof(netbuf))- 
		     (8000 - soundel), 0, (struct sockaddr *) &addr, 
		     len_addr) < 0) {
	    perror("audio_coder: when sending datagram packet");
	  }
	}
      } 
    }while(1);
  }
}


