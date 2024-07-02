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
*  Date    : September 1992                                                *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Audio decoder.                                           *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"
#include "ulaw2linear.h"

#define maxi(x,y) ((x)>(y) ? (x) : (y))


unsigned char *pt_out, outbuf[P_MAX];
BOOLEAN SOUND_ARRIVED=FALSE; 

#ifdef SPARCSTATION

BOOLEAN SUM_RUNNING=FALSE; 
BOOLEAN PLAY_AFTER_SUM=FALSE; 

int outlenmax;

#endif


void Audio_Decode(fd, port, group)
     int fd[2];
     char *port, *group;
{
  struct sockaddr_in addr, from;
  int len_addr, fromlen, outlen;
  int sock;
  int mask_pipe, mask_sock, mask, smask, mask0, fmax;
  unsigned char buf[P_MAX];
  int lr, n;
  char header;
  int type;
  struct timeval tim;
  LOCAL_TABLE table;
  int lpt=0;
#ifdef DECSTATION
  FILE *F_out;
  int lwrite;
  int cpt=0;
#endif /* DECSTATION */

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

  /* Initialize audio device */
  if(!soundinit(O_WRONLY)) {
    fprintf(stderr, "audio_decode: Cannot open audio device.\n");
    exit(-1);
  }

  /* Initialize CheckIfPlay() running procedure */
  signal(SIGALRM, CheckIfPlay);
  ualarm(100000, 100000); /* each second */

#elif DECSTATION

  /* Open F_out */
  if((F_out = popen("/usr/bin/play -f -", "w")) == NULL){
    fprintf(stderr, "Cannot make play -f -\n");
    perror("popen");
    exit(1);
  }

#endif /* SPARCSTATION */  

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
	    if(ShallIDecode(table, from.sin_addr.S_un.S_addr)){
#ifdef SPARCSTATION
	      if(SOUND_ARRIVED){
		SUM_RUNNING=TRUE;
		switch(buf[1]){
		case PCM_64:
		  {
		    register int i;
		    register unsigned char *pt_from=&buf[2];
		    register unsigned char *pt_to=outbuf;
		    int max=lr-2;

		    for(i=0; i<max; i++){
		      *pt_to = audio_s2u(audio_u2s(*pt_to) 
					 + audio_u2s(*pt_from));
		      pt_to++;
		      pt_from ++;
		    }
		    outlen = lr-2;
		    break;
		  }
		case ADPCM_32:
		  outlen = (lr-2)*2;
		  adpcm_decoder(&buf[2], outbuf, outlen, SOUND_ARRIVED);
		  pt_out = outbuf;
		  break;
		case VADPCM:
		  outlen = vadpcm_decoder(&buf[2], outbuf, lr-2, 
					  SOUND_ARRIVED);
		  pt_out = outbuf;
		  break;
		}
		outlenmax = maxi(outlen, outlenmax);
		SUM_RUNNING=FALSE;
	      }else{
#endif SPARCSTATION
		switch(buf[1]){
		case PCM_64:
		  /* We have to copy data */
		  bcopy(&buf[2], (char *)&outbuf[lpt], lr-2);
		  outlen = lr-2;
		  break;
		case ADPCM_32:
		  outlen = (lr-2)*2;
		  adpcm_decoder(&buf[2], &outbuf[lpt], outlen, SOUND_ARRIVED);
		  break;
		case VADPCM:
		  outlen = vadpcm_decoder(&buf[2], &outbuf[lpt], lr-2, 
					  SOUND_ARRIVED);
		  break;
		}
		pt_out = outbuf;
#ifdef SPARCSTATION
		outlenmax=outlen;
		SOUND_ARRIVED=TRUE;
	      }
	      if(PLAY_AFTER_SUM){
		soundplay(outlenmax, pt_out);
		PLAY_AFTER_SUM=SOUND_ARRIVED=FALSE;
		outlenmax=0;
	      }
#elif DECSTATION
	      lpt += outlen;
	      if(cpt==0){
		cpt = 0;
		lwrite=fwrite(pt_out, 1, lpt, F_out);
		fflush(F_out);
		if(outlen != lwrite)
		  fprintf(stderr, 
                   "received %d bytes, write only %d bytes\n", outlen, lwrite);
		lpt = 0;
	      }else{
		cpt ++;
	      }
 
#endif
	    }
	  }
	}while(--n);
      }
      if(mask & mask_pipe){
	if((lr = read(fd[0], &header, 1)) != 1){
	  fprintf(stderr, 
		  "audio_decode: Cannot read on pipe, Abort decoding\n");
	  exit(1);
	}else{
	  switch((int)(header)){
	    case PP_QUIT:
#ifdef SPARCSTATION
	      soundterm();
#elif DECSTATION
	      pclose(F_out);
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










    

