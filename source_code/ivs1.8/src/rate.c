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
#include "protocol.h"

#define T_MAX 10000

int a_lr=0, v_lr=0;
int a_t=0, v_t=0;
int n=0;
BOOLEAN UNICAST=FALSE;

void
average(sig)
     int sig;
{
  printf("Average audio socket : %3.2f kbits/s\n", (8.0*a_lr)/a_t);
  printf("Average video socket : %3.2f kbits/s\n", (8.0*v_lr)/v_t);
  printf("Video Frequency : %2.2f packets/s\n", (1000.0*n)/v_t);
  exit(0);
}

int CreateReceiveSocket(addr, len_addr, port, group)
     struct sockaddr_in *addr;
     int *len_addr;
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
{
  int sock;
  unsigned int iport, i1, i2, i3, i4;
  struct hostent *hp;
  char hostname[64];
  extern BOOLEAN UNICAST;
  int one=1;
  struct ip_mreq imr;

  if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
    fprintf(stderr, "CreateReceiveSocket: Invalid group %s\n", group);
    return(-1);
  }

  *len_addr = sizeof(*addr);
  iport = atoi(port);
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    fprintf(stderr, "Cannot create datagram socket\n");
    exit(-1);
  }
  gethostname(hostname, 64);
  if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "Unknown host %s\n", hostname);
      exit(0);
  }
  if(UNICAST){
    bzero((char *) addr, *len_addr);
    bcopy(hp->h_addr, (char *)addr, sizeof(addr));
  }else{
    bcopy(hp->h_addr, (char *)&imr.imr_interface.s_addr, hp->h_length);
    imr.imr_multiaddr.s_addr = (i4<<24) | (i3<<16) | (i2<<8) | i1;
    imr.imr_multiaddr.s_addr = htonl(imr.imr_multiaddr.s_addr);
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr,
		  sizeof(struct ip_mreq)) == -1){
      fprintf(stderr, "Cannot join group %s\n", group);
      perror("setsockopt()");
      exit(1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons((u_short)iport);
  addr->sin_addr.s_addr = INADDR_ANY;
  if (bind(sock, (struct sockaddr *) addr, *len_addr) < 0){
    fprintf(stderr, "Cannot bind socket to %d port\n", iport);
    exit(1);
  }
  return(sock);
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
	fprintf(stderr, "rate: error while receiving audio packet\n");
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
      }
      a_inittime = synctime;
    }else{
      if((lr=recvfrom(sock_video, (char *)buf, T_MAX, 0, &from, &fromlen)) 
	 < 0){
	fprintf(stderr, "rate: error while receiving video packet\n");
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
	last_r = r;
      }
      v_inittime = synctime;
    }
    inittime = synctime;
  }while(1);
}
  
