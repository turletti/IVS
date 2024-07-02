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
*  File    : stat.c                                                        *
*  Date    : September 1992                                                *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  rate statistics.                                         *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "protocol.h"

#define T_MAX 10000

int a_lr=0, v_lr=0;
int a_t=0, v_t=0;
int n=0;
FILE *F_audio, *F_video;
BOOLEAN UNICAST=FALSE;

average()
{
  printf("Average audio socket : %3.2f kbits/s\n", (8.0*a_lr)/a_t);
  printf("Average video socket : %3.2f kbits/s\n", (8.0*v_lr)/v_t);
  printf("Video Frequency : %2.2f packets/s\n", (1000.0*n)/v_t);
  fclose(F_audio);
  fclose(F_video);
  exit(0);
}


main()
{
  struct timeval realtime;
  double inittime, synctime, a_inittime, v_inittime;
  struct sockaddr_in addr, from;
  int lr, fmax, sock_audio, sock_video, len_addr, fromlen;
  int mask, mask0, mask_audio, mask_video;
  FILE *F_aux;
  char ntemp[256];
  char buf[T_MAX];
  char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
  BOOLEAN first_audio=TRUE, first_video=TRUE;
  double t, r, last_r=0.0;
  int tt=0;


  if((F_aux=fopen(".videoconf.default", "r")) != NULL){
    fscanf(F_aux, "%s%s", ntemp, AUDIO_PORT);
    fscanf(F_aux, "%s%s", ntemp, VIDEO_PORT);
    fscanf(F_aux, "%s%s", ntemp, CONTROL_PORT);
    fscanf(F_aux, "%s%s", ntemp, IP_GROUP);
  }else{
    strcpy(AUDIO_PORT, DEF_AUDIO_PORT);
    strcpy(VIDEO_PORT, DEF_VIDEO_PORT);
    strcpy(CONTROL_PORT, DEF_CONTROL_PORT);
    strcpy(IP_GROUP, DEF_IP_GROUP);
    fclose(F_aux);
  }
  if((F_audio=fopen("audio_rate", "w")) == NULL){
    fprintf(stderr, "Cannot open audio_rate file\n");
    exit(1);
  }
  if((F_video=fopen("video_rate", "w")) == NULL){
    fprintf(stderr, "Cannot open video_rate file\n");
    exit(1);
  }

  sock_audio = CreateReceiveSocket(&addr, &len_addr, AUDIO_PORT, IP_GROUP);
  sock_video = CreateReceiveSocket(&addr, &len_addr, VIDEO_PORT, IP_GROUP);
  mask_audio = (1 << sock_audio);
  mask_video = (1 << sock_video);
  mask0 = mask_audio | mask_video;
  fmax = (sock_audio>sock_video ? sock_audio+1 : sock_video+1);
  fromlen = sizeof(from);
  signal(SIGINT, average);

  do{
    mask = mask0;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)0) <= 0){
      fprintf(stderr, "rate: select returned -1\n");
      exit(1);
    }
    gettimeofday(&realtime, (struct timezone *)NULL);
    synctime = realtime.tv_sec +realtime.tv_usec/1000000.0;
    if(mask & mask_audio){
      if((lr=recvfrom(sock_audio, (char *)buf, T_MAX, 0, &from, &fromlen)) 
	 < 0){
	fprintf("rate: error while receiving audio packet\n");
	continue;
      }
      if(first_audio){
	first_audio=FALSE;
      }else{
	t = 1000.0*(synctime-a_inittime);
	a_t += t;
	a_lr += lr;
	r = 8.0*lr/t;
	printf("audio socket: received %d bytes in %d ms --> %3.1f kbits/s\n",
	       lr, (int)t, r);
	fprintf(F_audio, "%d \t %3.1f\n", a_t, r);
      }
      a_inittime = synctime;
    }else{
      if((lr=recvfrom(sock_video, (char *)buf, T_MAX, 0, &from, &fromlen)) 
	 < 0){
	fprintf("rate: error while receiving video packet\n");
	continue;
      }
      if(first_video){
	first_video=FALSE;
      }else{
	n ++;
	t = 1000.0*(synctime-v_inittime);
	r = 8.0*lr/t;
	v_t += t;
	v_lr += lr;
	printf("video socket: received %d bytes in %d ms --> %3.1f kbits/s\n", 
	       lr, (int)t, r);
	fprintf(F_video, "%d \t %3.1f\n", tt++, r);
/*	fprintf(F_video, "%3.1f\t %3.1f\n", last_r, r);*/
	last_r = r;
      }
      v_inittime = synctime;
    }
    inittime = synctime;
  }while(1);
}
  
