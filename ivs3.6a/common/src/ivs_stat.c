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
*  File    : ivs_stat.c                                                    *
*  Date    : 1995/2/15                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  video stat measurement.                                  *
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

#include "general_types.h"
#define DATA_MODULE
#include "protocol.h"
#include "rtp.h"

#define old_packet(recvd, expctd) \
    ((int)(expctd - recvd + 65536) % 65536 < 32768 ? TRUE : FALSE)

#define T_MAX 10000

int a_lr=0, v_lr=0;
int a_t=0, v_t=0;
int a_n=0, v_n =0;
FILE *F_audio, *F_video;
BOOLEAN UNICAST=FALSE;
BOOLEAN bad_packet;
double a_first_synctime, v_first_synctime;
unsigned short a_first_pckt, v_first_pckt;

void average(null)
     int null;
{
  fprintf(stderr, "Average audio socket : %3.2f kbits/s\n", (8.0*a_lr)/a_t);
  fprintf(stderr, "Audio Frequency : %2.2f packets/s\n", (1000.0*a_n)/a_t);
  fprintf(stderr, "Average video socket : %3.2f kbits/s\n", (8.0*v_lr)/v_t);
  fprintf(stderr, "Video Frequency : %2.2f packets/s\n", (1000.0*v_n)/v_t);
  fclose(F_audio);
  fclose(F_video);
  exit(0);
}

void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-I @IP] [-u] hostname[/video_port]\n", name);
  fprintf(stderr, "\t -I: listen to a new multicast address\n");
  fprintf(stderr, "\t -u: enable the unicast mode\n");
  exit(1);
}

main(argc, argv)
     int argc;
     char **argv;
{
  struct timeval realtime;
  double synctime, a_inittime, v_inittime;
  struct sockaddr_in addr, from, target;
  int i, lr, fmax, sock_audio, sock_video, len_addr, fromlen, narg;
  int mask, mask0, mask_audio, mask_video;
  char buf[T_MAX], *ptr;
  char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
  double t, r, a_last_r=0.0, v_last_r=0.0;
  char hostname[256];
  u_short sequence_number, a_expected, v_expected;
  BOOLEAN first_audio=TRUE, first_video=TRUE;
  BOOLEAN NEW_PORTS=FALSE;
  BOOLEAN NEW_IP_GROUP=FALSE;
  rtp_hdr_t *h;
  char null=0;

  
  if((argc < 2) || (argc > 5))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(strcmp(argv[narg], "-I") == 0){
      strcpy(IP_GROUP, argv[++narg]);
      narg ++;
      NEW_IP_GROUP = TRUE;
      continue;
    }else{
      if(strcmp(argv[narg], "-u") == 0){
	UNICAST = TRUE;
	narg ++;
	continue;
      }else{
	if(argv[narg][0] == '-')
	  Usage(argv[0]);
	else{ 
	  strcpy(hostname, argv[narg]);
	  narg ++;
	  continue;
	}
      }
    }
  }
  if((ptr=(char *)strstr(hostname, "/")) != NULL){
    *ptr++ = (char) 0;
    strcpy(VIDEO_PORT, ptr);
    i = atoi(VIDEO_PORT);
    sprintf(AUDIO_PORT, "%d", ++i);
    sprintf(CONTROL_PORT, "%d", ++i);
    NEW_PORTS = TRUE;
  }
  if(UNICAST){
    if(! NEW_PORTS){
      strcpy(VIDEO_PORT, UNI_VIDEO_PORT);
      strcpy(AUDIO_PORT, UNI_AUDIO_PORT);
      strcpy(CONTROL_PORT, UNI_CONTROL_PORT);
    }
  }else{
    if(! NEW_PORTS){
      strcpy(VIDEO_PORT, MULTI_VIDEO_PORT);
      strcpy(AUDIO_PORT, MULTI_AUDIO_PORT);
      strcpy(CONTROL_PORT, MULTI_CONTROL_PORT);
    }
    if(!NEW_IP_GROUP)
      strcpy(IP_GROUP, MULTI_IP_GROUP);
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

  sock_audio = CreateReceiveSocket(&addr, &len_addr, AUDIO_PORT, IP_GROUP,
				   UNICAST, &null);
  sock_video = CreateReceiveSocket(&addr, &len_addr, VIDEO_PORT, IP_GROUP,
				   UNICAST, &null);
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
      perror("select");
      exit(1);
    }
    gettimeofday(&realtime, (struct timezone *)NULL);
    synctime = realtime.tv_sec +realtime.tv_usec/1000000.0;
    if(mask & mask_audio){
      if((lr=recvfrom(sock_audio, (char *)buf, T_MAX, 0,
		      (struct sockaddr *)&from, &fromlen)) < 0){
	perror("recvfrom audio");
	continue;
      }
      if(from.sin_addr.s_addr == target.sin_addr.s_addr){
	h = (rtp_hdr_t *) buf;
	bad_packet = FALSE;
	sequence_number = ntohs(h->seq);
	if(first_audio){
	  a_first_synctime = synctime;
	  a_first_pckt = sequence_number;
	  a_expected = (int)(sequence_number + 1) % 65536;
	  fprintf(stderr,
		  "ivs_stat: First audio packet received #%d, len:%d\n",
		  sequence_number, lr);
	  first_audio=FALSE;
	}else{
	  a_n ++;
	  t = 1000.0*(synctime-a_inittime);
	  r = 8.0*lr/t;
	  a_t += t;
	  a_lr += lr;
	  if(sequence_number != a_expected){
	    if(!old_packet(sequence_number, a_expected)){
	      a_expected = (int)(sequence_number + 1) % 65536;
	      fprintf(F_audio,
		      ">>>#%d, len:%d, dt (ms):%f, rate (kbps):%f\n",
		      sequence_number, lr, t, r);
	    }else{
	      fprintf(F_audio,
		  "<<<#%d, expected: %d, len:%d, dt (ms):%f, rate (kbps):%f\n",
		      sequence_number, a_expected, lr, t, r);
	    }
	  }else{
	    a_expected = (int)(sequence_number + 1) % 65536;
	    fprintf(F_audio,
		    "#%d, len:%d, dt (ms):%f, rate (kbps):%f\n",
		    sequence_number, lr, t, r);
	  }
	  a_last_r = r;
	}
	a_inittime = synctime;
      }
    }else{
      if((lr=recvfrom(sock_video, (char *)buf, T_MAX, 0,
		      (struct sockaddr *)&from, &fromlen)) < 0){
	perror("recvfrom video");
	continue;
      }
      if(from.sin_addr.s_addr == target.sin_addr.s_addr){
	h = (rtp_hdr_t *) buf;
	bad_packet = FALSE;
	sequence_number = ntohs(h->seq);
#ifdef TEST_TN
	fprintf(F_video, "%d\t%d\n", sequence_number, lr);
#endif	
	if(first_video){
	  v_first_synctime = synctime;
	  v_first_pckt = sequence_number;
	  v_expected = (int)(sequence_number + 1) % 65536;
	  fprintf(stderr,
		  "ivs_stat: First video packet received #%d, len:%d\n",
		  sequence_number, lr);
	  first_video=FALSE;
	}else{
	  v_n ++;
	  t = 1000.0*(synctime-v_inittime);
	  r = 8.0*lr/t;
	  v_t += t;
	  v_lr += lr;
#ifndef TEST_TN
	  if(sequence_number != v_expected){
	    if(!old_packet(sequence_number, v_expected)){
	      v_expected = (int)(sequence_number + 1) % 65536;
	      fprintf(F_video,
		      ">>>#%d, len:%d, dt (ms):%f, rate (kbps) :%f\n",
		      sequence_number, lr, t, r);
	    }else{
	      fprintf(F_video,
		  "<<<#%d, expected: %d, len:%d, dt (ms):%f, rate (kbps):%f\n",
		      sequence_number, a_expected, lr, t, r);
	    }
	  }else{
	    v_expected = (int)(sequence_number + 1) % 65536;
	    fprintf(F_video,
		    "#%d, len:%d, dt (ms):%f, rate (kbps):%f\n",
		    sequence_number, lr, t, r);
	  }
#endif
	  v_last_r = r;
	}
	v_inittime = synctime;
      }
    }
  }while(1);
}
  
