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
*  File              : manage_socket.c                                     *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/5/15                                           *
*--------------------------------------------------------------------------*
*  Description :  A set of procedures to manage sockets.                   *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Francis Dupont      |  94/13/1  | Changes in CreateReceiveSocket to    *
*                       |           | handle multiple interfaces.          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "general_types.h"


/*------------------------ SOCKET CREATION PROCEDURES -----------------------*/

int SetSockTTL(sock, ttl)
     int sock;
     u_int8 ttl;
{

#ifdef MULTICAST
  return(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl,
		    sizeof(ttl)));
#else
  return(0);
#endif
}


int CreateSendSocket(addr, len_addr, port, group, ttl, port_source,
		     UNICAST, my_interface)
     struct sockaddr_in *addr;
     int *len_addr;
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
     u_int8 *ttl;
     u_short *port_source;
     BOOLEAN UNICAST;
     char *my_interface;
{
  struct sockaddr_in addr_local;
  int sock;
  u_int iport, i1, i2, i3, i4;
  BOOLEAN OTHER_INTERFACE;
  struct sockaddr_in my_addr;
  struct hostent *hp;
  int len_my_addr;

  OTHER_INTERFACE  = (strlen(my_interface) ? TRUE : FALSE);

  if(sscanf(group, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
    fprintf(stderr, "CreateSendSocket: Invalid group %s\n", group);
    exit(1);
  }

  iport = atoi(port);
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Cannot create datagram socket");
    exit(1);
  }
  bzero((char *) &addr_local, sizeof(addr_local));
  addr_local.sin_family = AF_INET;
  addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_local.sin_port = htons(0);
  if(bind(sock, (struct sockaddr *) &addr_local, sizeof(addr_local)) < 0){
    perror("bind");
    fprintf(stderr, "Cannot bind socket to address %s port 0\n",
	    inet_ntoa(addr_local.sin_addr), iport);
    exit(1);
  }else{
    struct sockaddr name;
    int namelen=sizeof(name);
	
    getsockname(sock, &name, &namelen);
    bcopy((char *)&name, (char  *)&addr_local, namelen);
    *port_source = ntohs(addr_local.sin_port);
  }
  *len_addr = sizeof(*addr);
  bzero((char *)addr, *len_addr);
  addr->sin_port = htons((u_short)iport);
  addr->sin_addr.s_addr = htonl((i4<<24) | (i3<<16) | (i2<<8) | i1);
  addr->sin_family = AF_INET;

  if(UNICAST == FALSE){
#ifdef MULTICAST
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *) ttl, sizeof(*ttl));
    if(OTHER_INTERFACE) {
	len_my_addr = sizeof(my_addr);
	bzero((char *) &my_addr, len_my_addr);

	if(((hp = gethostbyname(my_interface)) != (struct hostent *)0) &&
	   (hp->h_addrtype == AF_INET)){
	    bcopy(hp->h_addr, (char *)&my_addr.sin_addr, hp->h_length);
	}else{
	    my_addr.sin_addr.s_addr = inet_addr(my_interface);
	    if(my_addr.sin_addr.s_addr == -1){
		fprintf(stderr, "CreateSendSocket: Invalid interface %s\n", 
			my_interface);
		exit(1);
	    }
	}
	if(OTHER_INTERFACE) {
	    len_my_addr = sizeof(my_addr.sin_addr);
	    if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, 
			  &my_addr.sin_addr, len_my_addr) == -1){
		perror("setsockopt'IP_MULTICAST_IF");
	    }
	}
    }
#else
    fprintf(stderr, "MULTICAST mode not supported, use only unicast option\n");
    exit(-1);
#endif /* MULTICAST */
  }

  return(sock);
}



int BindSocket(sock, sin_addr, port_number)
     int sock;
     long sin_addr;
     char *port_number;
{
  struct sockaddr_in addr_local;

  bzero((char *) &addr_local, sizeof(addr_local));
  addr_local.sin_family = AF_INET;
  addr_local.sin_addr.s_addr = htonl(sin_addr);
  addr_local.sin_port = htons(atoi(port_number));
  if(bind(sock, (struct sockaddr *) &addr_local, sizeof(addr_local)) < 0){
    perror("bind");
    fprintf(stderr, "Cannot bind socket to address %s port %s\n",
	    inet_ntoa(addr_local.sin_addr), port_number);
    exit(1);
  }else{
    return(0);
  }
}



int CreateReceiveSocket(addr, len_addr, port, group, UNICAST, my_interface)
     struct sockaddr_in *addr;
     int *len_addr;
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
     BOOLEAN UNICAST;
     char *my_interface;
{
  int sock, optlen, optval;
  u_int iport;
  struct hostent *hp;
#ifdef MULTICAST
  int one=1;
  struct ip_mreq imr;
#endif 
  BOOLEAN OTHER_INTERFACE;
  struct sockaddr_in my_addr;
  int len_my_addr;

  OTHER_INTERFACE  = (strlen(my_interface) ? TRUE : FALSE);
  *len_addr = sizeof(*addr);
  bzero((char *) addr, *len_addr);
  iport = atoi(port);
  addr->sin_family = AF_INET;
  addr->sin_port = htons((u_short)iport);

  if(!UNICAST){
    if(((hp = gethostbyname(group)) != (struct hostent *)0) &&
       (hp->h_addrtype == AF_INET)){
      bcopy(hp->h_addr, (char *)&addr->sin_addr, hp->h_length);
    }else{
      addr->sin_addr.s_addr = inet_addr(group);
      if(addr->sin_addr.s_addr == -1){
        fprintf(stderr, "CreateReceiveSocket: Invalid group %s\n", group);
        exit(1);
      }
    }
  }

  if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
    perror("Cannot create datagram socket");
    exit(-1);
  }

#ifdef MULTICAST
  if(OTHER_INTERFACE && ! UNICAST) {
      if(((hp = gethostbyname(my_interface)) != (struct hostent *)0) &&
	 (hp->h_addrtype == AF_INET)){
	  bcopy(hp->h_addr, (char *)&my_addr.sin_addr, hp->h_length);
      }else{
	  my_addr.sin_addr.s_addr = inet_addr(my_interface);
	  if(my_addr.sin_addr.s_addr == -1){
	      fprintf(stderr, "CreateReceiveSocket: Invalid interface %s\n", 
		      my_interface);
	      exit(1);
	  }
      }
  }
#endif
  if(!UNICAST){
#ifdef MULTICAST
    imr.imr_multiaddr.s_addr = addr->sin_addr.s_addr;
    imr.imr_interface.s_addr = OTHER_INTERFACE ?
	my_addr.sin_addr.s_addr : INADDR_ANY;
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&imr,
		  sizeof(struct ip_mreq)) == -1){
      fprintf(stderr, "Cannot join group %s\n", group);
      if(OTHER_INTERFACE) {
	  fprintf(stderr, "Are you sure %s is the right interface ?\n", 
		  my_interface);
      }
      perror("setsockopt()");
      if(errno == EADDRNOTAVAIL){
	fprintf(stderr, "Try: /usr/etc/route add 224.0.0.0 192.1.1.1 0\n");
      }else{
        fprintf(stderr,
		"Does your kernel support IP multicast extensions ?\n");
      }
      exit(1);
    }
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, 
		  sizeof(one)) == -1) {
	perror("setsockopt'SO_REUSEADDR"); 
    }
#else
    fprintf(stderr, "MULTICAST mode not implemented, use unicast option\n");
    exit(-1);
#endif /* MULTICAST */
  }

#if defined(SOLARIS) || defined(HPUX) || defined(DECSTATION)
  addr->sin_addr.s_addr = INADDR_ANY;
#endif
  if (bind(sock, (struct sockaddr *) addr, *len_addr) < 0){
    perror("bind");
    fprintf(stderr, "Cannot bind socket to address %s port %d\n",
	    inet_ntoa(addr->sin_addr), iport);
    exit(1);
  }
  optlen = 4;
  optval = 25000; /* 50000 in 3.5 version of ivs */
  if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, optlen) != 0){
    perror("setsockopt failed");
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen);
    fprintf(stderr,"SO_RCVBUF = %d\n", optval); 
  }
  return(sock);
}



