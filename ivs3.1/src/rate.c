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
*  File    : rate.c                                                        *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  rate measurement.                                        *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "protocol.h"

#define T_MAX 10000

int a_lr=0, v_lr=0;
int a_t=0, v_t=0;
int n=0;
FILE *F_audio, *F_video;
BOOLEAN UNICAST=FALSE;
BOOLEAN bad_packet;

void average(sig)
     int sig;
{
  fprintf(stderr, "Average audio socket : %3.2f kbits/s\n", (8.0*a_lr)/a_t);
  fprintf(stderr, "Average video socket : %3.2f kbits/s\n", (8.0*v_lr)/v_t);
  fprintf(stderr, "Video Frequency : %2.2f packets/s\n", (1000.0*n)/v_t);
  fclose(F_audio);
  fclose(F_video);
  exit(0);
}

void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-u] hostname\n", name);
  exit(1);
}

main(arg, argv)
     int arg;
     char **argv;
{
  struct timeval realtime;
  double inittime, synctime, a_inittime, v_inittime;
  struct sockaddr_in addr, from, target;
  int lr, fmax, sock_audio, sock_video, len_addr, fromlen, argc, narg;
  int mask, mask0, mask_audio, mask_video;
  FILE *F_aux;
  char ntemp[256];
  char buf[T_MAX];
  char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
  BOOLEAN first_audio=TRUE, first_video=TRUE;
  double t, r, last_r=0.0;
  char hostname[256];
  u_short sequence_number;

  if((arg < 2) || (arg > 4))
    Usage(argv[0]);
  narg = 1;
  while(narg != arg){
    if(strcmp(argv[narg], "-u") == 0){
      UNICAST = TRUE;
      narg ++;
      continue;
    }else{
      strcpy(hostname, argv[narg]);
      narg ++;
      continue;
    }
  }

  if(UNICAST){
    strcpy(AUDIO_PORT, UNI_AUDIO_PORT);
    strcpy(VIDEO_PORT, UNI_VIDEO_PORT);
    strcpy(CONTROL_PORT, UNI_CONTROL_PORT);
  }else{
    if((F_aux=fopen(".videoconf.default", "r")) != NULL){
      fscanf(F_aux, "%s%s", ntemp, AUDIO_PORT);
      fscanf(F_aux, "%s%s", ntemp, VIDEO_PORT);
      fscanf(F_aux, "%s%s", ntemp, CONTROL_PORT);
      fscanf(F_aux, "%s%s", ntemp, IP_GROUP);
      fclose(F_aux);
    }else{
      strcpy(AUDIO_PORT, MULTI_AUDIO_PORT);
      strcpy(VIDEO_PORT, MULTI_VIDEO_PORT);
      strcpy(CONTROL_PORT, MULTI_CONTROL_PORT);
      strcpy(IP_GROUP, MULTI_IP_GROUP);
      fclose(F_aux);
    }
  }

  if((F_audio=fopen("audio_rate", "w")) == NULL){
    fprintf(stderr, "Cannot open audio_rate file\n");
    exit(1);
  }
  if((F_video=fopen("video_rate", "w")) == NULL){
    fprintf(stderr, "Cannot open video_rate file\n");
    exit(1);
  }
  {
    /* getting the target sin_addr */

    struct hostent *hp;
    int namelen=100;
    unsigned int i1, i2, i3, i4;

    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), hostname);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&target.sin_addr, 4);
    }else{
      target.sin_addr.s_addr = (u_long)inet_addr(hostname);
    }
    if(UNICAST)
      strcpy(IP_GROUP, inet_ntoa(target.sin_addr));
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
    bad_packet = TRUE;
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
	fprintf(stderr, "rate: error while receiving audio packet\n");
	continue;
      }
      if(from.sin_addr.s_addr == target.sin_addr.s_addr){
	bad_packet = FALSE;
	if(first_audio){
	  first_audio=FALSE;
	}else{
	  t = 1000.0*(synctime-a_inittime);
	  a_t += t;
	  a_lr += lr;
	  r = 8.0*lr/t;
	  fprintf(stderr,
              "audio socket: received %d bytes in %d ms --> %3.1f kbits/s\n",
		  lr, (int)t, r);
	  fprintf(F_audio, "%d \t %3.1f\n", a_t, r);
	}
	a_inittime = synctime;
      }
    }else{
      if((lr=recvfrom(sock_video, (char *)buf, T_MAX, 0, &from, &fromlen)) 
	 < 0){
	fprintf(stderr, "rate: error while receiving video packet\n");
	continue;
      }
      if(from.sin_addr.s_addr == target.sin_addr.s_addr){
	bad_packet = FALSE;
	sequence_number = ((u_char)buf[2] << 8) | (u_char)buf[3];
	if(first_video){
	  first_video=FALSE;
	}else{
	  n ++;
	  t = 1000.0*(synctime-v_inittime);
	  r = 8.0*lr/t;
	  v_t += t;
	  v_lr += lr;
	  fprintf(stderr,
           "video socket: %d: received %d bytes in %d ms --> %3.1f kbits/s\n", 
		  sequence_number, lr, (int)t, r);
	  fprintf(F_video, "%d%3.1f\t %3.1f\n", last_r, r);
	  last_r = r;
	}
	v_inittime = synctime;
      }
    }
    if(!bad_packet)
      inittime = synctime;
  }while(1);
}
  
