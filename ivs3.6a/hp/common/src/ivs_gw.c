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
* 	                						   *
*  File              : ivs_gw.c       	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Main file.                                          *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "general_types.h"
#define DATA_MODULE
#include "protocol.h"
#include "rtp.h"

#define DEFAULT_TTL   32

#define streq(a, b)   (strcmp((a), (b)) == 0)
#define maxi(x,y)     ((x)>(y) ? (x) : (y))


/* From identifier.c */
extern u_short ManageIdentifier();
extern void ChangeIdValue();
extern void AddSSRCOption();

/* Global variable */
BOOLEAN DEBUG_MODE=FALSE;
BOOLEAN UNICAST=FALSE;


static u_char ttl=DEFAULT_TTL;
static int rsockA[3], rsockB[3], wsockA[3], wsockB[3];
static u_long tab_sin_addr[N_MAX_STATIONS][N_MAX_STATIONS];
static int null=0;


static void ToggleDebug(null)
     int null;
{
  DEBUG_MODE = (DEBUG_MODE ? FALSE : TRUE);
  if(DEBUG_MODE)
    fprintf(stderr, "ivs: Debug mode turned on\n");
  else
    fprintf(stderr, "ivs: Debug mode turned off\n");
  signal(SIGUSR1, ToggleDebug);
  return;
}



static void Quit(null)
     int null;
{
  register int i;

  for(i=0; i<3; i++){
    close(rsockA[i]);
    close(wsockA[i]);
    close(rsockB[i]);
    close(wsockB[i]);
  }
  exit(0);
}



static void Usage(name)
     char *name;
{
  fprintf(stderr, 
	  "Usage: %s sourceA[/video_port] sourceB[/video_port] [-t ttl]\n", 
	  name);
  exit(1);
}



main(argc, argv)
     int argc;
     char **argv;
{
  struct sockaddr_in addr, from, targetA, targetB;
  struct sockaddr_in waddrA[3], waddrB[3];
  int len_waddrA[3], len_waddrB[3];
  int mask, mask0, rmaskA[3], rmaskB[3];
  int lr, lw, len_addr, fromlen, fmax, narg, next_option;
  static u_short last_identifier[N_MAX_STATIONS];
  u_int i1, i2, i3, i4;
  u_short identifier;
  u_long sin_addr_from;
  u_int buf[P_MAX/4];
  char *ptr;
  char AUDIO_PORTA[5], VIDEO_PORTA[5], CONTROL_PORTA[5];
  char AUDIO_PORTB[5], VIDEO_PORTB[5], CONTROL_PORTB[5];
  char IP_GROUPA[256], IP_GROUPB[256];
  BOOLEAN NEW_PORTSA=FALSE;
  BOOLEAN NEW_PORTSB=FALSE;
  BOOLEAN UNICASTA=FALSE;
  BOOLEAN UNICASTB=FALSE;
  BOOLEAN OPTION_PRESENT;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *r; /* RTP options */
  u_short id_from, port_source;
  register int i;
  char null=0;

  if((argc < 3) || (argc > 5))
    Usage(argv[0]);
  
  narg = 1;
  while(narg != argc){
    if(argv[narg][0] == '-'){
      if(streq(argv[narg], "-t")){
	ttl = atoi(argv[++narg]);
	narg ++;
	continue;
      }else
	Usage(argv[0]);
    }
    narg++;
  }
  if((ptr=(char *)strstr((char *)argv[1], "/")) != NULL){
    *ptr++ = (char) 0;
    strcpy(IP_GROUPA, argv[1]);
    strcpy(VIDEO_PORTA, ptr);
    i = atoi(VIDEO_PORTA);
    sprintf(AUDIO_PORTA, "%d", ++i);
    sprintf(CONTROL_PORTA, "%d", ++i);
    NEW_PORTSA = TRUE;
  }else{
    strcpy(IP_GROUPA, argv[1]);
  }
  if(sscanf(IP_GROUPA, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
    if((i4 < 224) || (i4 > 239)){
      UNICASTA = TRUE;
    }
  }else
    UNICASTA = TRUE;
  if(UNICASTA){
    if(! NEW_PORTSA){
      strcpy(VIDEO_PORTA, UNI_VIDEO_PORT);
      strcpy(AUDIO_PORTA, UNI_AUDIO_PORT);
      strcpy(CONTROL_PORTA, UNI_CONTROL_PORT);
    }
  }else{
    if(! NEW_PORTSA){
      strcpy(VIDEO_PORTA, MULTI_VIDEO_PORT);
      strcpy(AUDIO_PORTA, MULTI_AUDIO_PORT);
      strcpy(CONTROL_PORTA, MULTI_CONTROL_PORT);
    }
  }

  if((ptr=(char *)strstr((char *)argv[2], "/")) != NULL){
    *ptr++ = (char) 0;
    strcpy(IP_GROUPB, argv[2]);
    strcpy(VIDEO_PORTB, ptr);
    i = atoi(VIDEO_PORTB); 
    sprintf(AUDIO_PORTB, "%d", ++i);
    sprintf(CONTROL_PORTB, "%d", ++i);
    NEW_PORTSB = TRUE;
  }else
    strcpy(IP_GROUPB, argv[2]);
  if(sscanf(IP_GROUPB, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
    if((i4 < 224) || (i4 > 239)){
      UNICASTB = TRUE;
    }
  }else
    UNICASTB = TRUE;

  if(UNICASTB){
    if(! NEW_PORTSB){
      strcpy(VIDEO_PORTB, UNI_VIDEO_PORT);
      strcpy(AUDIO_PORTB, UNI_AUDIO_PORT);
      strcpy(CONTROL_PORTB, UNI_CONTROL_PORT);
    }
  }else{
    if(! NEW_PORTSB){
      strcpy(VIDEO_PORTB, MULTI_VIDEO_PORT);
      strcpy(AUDIO_PORTB, MULTI_AUDIO_PORT);
      strcpy(CONTROL_PORTB, MULTI_CONTROL_PORT);
    }
  }

  {
    /* getting the targets sin_addr A and B */

    struct hostent *hp;

    if(sscanf(IP_GROUPA, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(IP_GROUPA)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), IP_GROUPA);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&targetA.sin_addr, 4);
    }else{
      targetA.sin_addr.s_addr = (u_long)inet_addr(IP_GROUPA);
    }
    if(UNICASTA)
      strcpy(IP_GROUPA, (char *)inet_ntoa(targetA.sin_addr));

    if(sscanf(IP_GROUPB, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(IP_GROUPB)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), IP_GROUPB);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&targetB.sin_addr, 4);
    }else{
      targetB.sin_addr.s_addr = (u_long)inet_addr(IP_GROUPB);
    }
    if(UNICASTB)
      strcpy(IP_GROUPB, (char *)inet_ntoa(targetB.sin_addr));
  }

  /* Create sockets for source A */
  UNICAST = UNICASTA;
  rsockA[0] = CreateReceiveSocket(&addr, &len_addr, VIDEO_PORTA, IP_GROUPA,
				  UNICAST, &null);
  rsockA[1] = CreateReceiveSocket(&addr, &len_addr, AUDIO_PORTA, IP_GROUPA,
				  UNICAST, &null);
  rsockA[2] = CreateReceiveSocket(&addr, &len_addr, CONTROL_PORTA, IP_GROUPA,
				  UNICAST, &null);
  wsockA[0] = CreateSendSocket(&waddrA[0], &len_waddrA[0], VIDEO_PORTA,
			       IP_GROUPA, &ttl, &port_source, UNICAST, 
			       &null);
  wsockA[1] = CreateSendSocket(&waddrA[1], &len_waddrA[1], AUDIO_PORTA,
			       IP_GROUPA, &ttl, &port_source, UNICAST, 
			       &null);
  wsockA[2] = CreateSendSocket(&waddrA[2], &len_waddrA[2], CONTROL_PORTA, 
			       IP_GROUPA, &ttl, &port_source, UNICAST, 
			       &null);
  if(!UNICAST){
#ifdef MULTICAST
    /* Disable loop back */
    u_char loop=0;
    for(i=0; i<3; i++){
      setsockopt(wsockA[i], IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, 
		 sizeof(loop));
    }
#else
    fprintf(stderr, "ivs_gw: multicast not implemented\n");
    exit(1);
#endif
  }
  /* Create sockets for source B */
  UNICAST = UNICASTB;
  rsockB[0] = CreateReceiveSocket(&addr, &len_addr, VIDEO_PORTB, IP_GROUPB,
				  UNICAST, &null);
  rsockB[1] = CreateReceiveSocket(&addr, &len_addr, AUDIO_PORTB, IP_GROUPB,
				  UNICAST, &null);
  rsockB[2] = CreateReceiveSocket(&addr, &len_addr, CONTROL_PORTB, IP_GROUPB,
				  UNICAST, &null);
  wsockB[0] = CreateSendSocket(&waddrB[0], &len_waddrB[0], VIDEO_PORTB, 
			       IP_GROUPB, &ttl, &port_source, UNICAST, 
			       &null);
  wsockB[1] = CreateSendSocket(&waddrB[1], &len_waddrB[1], AUDIO_PORTB, 
			       IP_GROUPB, &ttl, &port_source, UNICAST, 
			       &null);
  wsockB[2] = CreateSendSocket(&waddrB[2], &len_waddrB[2], CONTROL_PORTB,
			       IP_GROUPB, &ttl, &port_source, UNICAST, 
			       &null);
  if(!UNICAST){
#ifdef MULTICAST    
    /* Disable loop back */
    u_char loop=0;
    for(i=0; i<3; i++){
      setsockopt(wsockB[i], IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, 
		 sizeof(loop));
    }
#else
    fprintf(stderr, "ivs_gw: multicast not implemented\n");
    exit(1);
#endif
  }

  bzero((char *)tab_sin_addr, N_MAX_STATIONS*sizeof(u_long));
  mask0 = 0;
  fmax = 0;
  for(i=0; i<3; i++){
    rmaskA[i] = 1 << rsockA[i];
    rmaskB[i] = 1 << rsockB[i];
    fmax = maxi(fmax, rsockA[i]);
    fmax = maxi(fmax, rsockB[i]);
    mask0 |= (rmaskA[i] | rmaskB[i]);
  }
  fmax += 1;
  bzero((char *)last_identifier, N_MAX_STATIONS*sizeof(u_short));
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);
  signal(SIGUSR1, ToggleDebug);

  do{
    mask = mask0;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)0) <= 0){
      perror("select");
      Quit(null);
    }
    for(i=0; i<3; i++){
      if(mask & rmaskA[i]){
	bzero((char *)&from, 4);
	fromlen = sizeof(from);
	if((lr=recvfrom(rsockA[i], (char *)&buf[1], P_MAX, 0,
			(struct sockaddr *)&from, &fromlen)) < 0){
	  perror("recvfrom");
	  fprintf(stderr, "recvfrom failed on rsockA[%d]\n", i);
	  continue;
	}
	sin_addr_from = (u_long)from.sin_addr.s_addr;
	h = (rtp_hdr_t *) &buf[1];
	id_from = 0;
	next_option=3;
	OPTION_PRESENT = h->p;
	while(OPTION_PRESENT){
	  /* There is a RTP option */
	  r = (rtp_t *) &buf[next_option];
	  OPTION_PRESENT = (r->generic.final == 0);
	  if(r->generic.type == RTP_SSRC){
	    /* There is a SSRC option, get the identifier */
	    id_from = ntohs(r->ssrc.id);
	  }else{
	    r->generic.final = 0;
	    next_option += r->generic.length;
	  }
	}
	if((identifier = ManageIdentifier(tab_sin_addr, sin_addr_from, id_from,
					  last_identifier)) == 0){
	  if(DEBUG_MODE){
	    fprintf(stderr,
		    "Too many participants, last_identifier[%d] = %d\n",
		    id_from, last_identifier[id_from]);
	    /* Drop this new participant */
	  }
	  continue;
	}
	if(DEBUG_MODE){
	  fprintf(stderr, "id_from%d, id_out%d\n", id_from, identifier);
	}
	if(!id_from){
	  /* The SSRC option must be added after the RTP timestamp */
	  AddSSRCOption(buf, identifier, next_option, &lr);
	}else{
	  /* Set the new identifier in the SSRC field */
	  ChangeIdValue(buf, identifier, next_option);
	}
	if((lw=sendto(wsockB[i], (char *)(id_from ? &buf[1] : buf), lr, 0, 
		      (struct sockaddr *)&waddrB[i], len_waddrB[i])) != lr){
	  perror("sendto");
	  fprintf(stderr, "sendto failed on wsockB[%d]\n", i);
	  continue;
	}
      }
      if(mask & rmaskB[i]){
	bzero((char *)&from, 4);
	fromlen = sizeof(from);
	if((lr=recvfrom(rsockB[i], (char *)&buf[1], P_MAX, 0,
			(struct sockaddr *)&from, &fromlen)) < 0){
	  perror("recvfrom");
	  fprintf(stderr, "recvfrom failed on rsockB[%d]\n", i);
	  continue;
	}
	sin_addr_from = (u_long)from.sin_addr.s_addr;
	h = (rtp_hdr_t *) &buf[1];
	id_from = 0;
	next_option=3;
	OPTION_PRESENT = h->p;
	while(OPTION_PRESENT){
	  /* There is a RTP option */
	  r = (rtp_t *) &buf[next_option];
	  OPTION_PRESENT = (r->generic.final == 0);
	  if(r->generic.type == RTP_SSRC){
	    /* There is a SSRC option, get the identifier */
	    id_from = ntohs(r->ssrc.id);
	  }else{
	    r->generic.final = 0;
	    next_option += r->generic.length;
	  }
	}

	if((identifier = ManageIdentifier(tab_sin_addr, sin_addr_from, id_from,
					  last_identifier)) == 0){
	  if(DEBUG_MODE){
	    fprintf(stderr,
		    "Too many participants, last_identifier[%d] = %d\n",
		    id_from, last_identifier[id_from]);
	    /* Drop this new participant */
	  }
	  continue;
	}
	if(DEBUG_MODE){
	  fprintf(stderr, "id_from%d, id_out%d\n", id_from, identifier);
	}
	if(!id_from){
	  /* The SSRC option must be added after the RTP timestamp */
	  AddSSRCOption(buf, identifier, next_option, &lr);
	}else{
	  /* Set the new identifier in the SSRC field */
	  ChangeIdValue(buf, identifier, next_option);
	  if(DEBUG_MODE)
	    fprintf(stderr,
		    "%d flow, changed ssrc, next_option=%d, new id %d\n", i,
		    next_option, identifier);
	}

	if((lw=sendto(wsockA[i], (char *)(id_from ? &buf[1] : buf), lr, 0, 
		      (struct sockaddr *) &waddrA[i], len_waddrA[i])) != lr){
	  perror("sendto");
	  fprintf(stderr, "sendto failed on wsockA[%d]\n", i);
	  continue;
	}
      }
    } /* for() */
  }while(1);
}

