/**************************************************************************
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
*  File              : video_coder0.c                                      *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 video coder.                                       *
*                                                                          *
*--------------------------------------------------------------------------*
* Modified by Winston Dang  wkd@Hawaii.Edu	92/12/8                    *
* alterations were made for speed purposes.                                *
\**************************************************************************/

#include <math.h>
#include <memory.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "idx_huffman_coder.h"
#include "video_coder.h"
#include "rtp.h"

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int DiffTime(told, tnew)
     struct timeval *told, *tnew;
{
  double timeold, timenew;
  int deltat; /* msec */
  
  timeold = (double)1000.0 * told->tv_sec + (double)told->tv_usec/1000.0;
  timenew = (double)1000.0 * tnew->tv_sec + (double)tnew->tv_usec/1000.0;
  deltat = (int)(timenew - timeold);
  return(deltat);
}

/*-------------------------------------------------------------------------*/


void ToggleCoderDebug(null)
     int null;
{
  DEBUG_MODE = (DEBUG_MODE ? FALSE : TRUE);
  if(DEBUG_MODE)
    fprintf(stderr, "video_coder: Debug mode turned on\n");
  else
    fprintf(stderr, "video_coder: Debug mode turned off\n");
  signal(SIGUSR1, ToggleCoderDebug);  
}

/*-------------------------------------------------------------------------*/


void ToggleCoderStat(null)
     int null;
{
  MODE_STAT = (MODE_STAT ? FALSE : TRUE);
  if(MODE_STAT)
    fprintf(stderr, "video_coder: STAT mode turned on\n");
  else
    fprintf(stderr, "video_coder: STAT mode turned off\n");
  signal(SIGUSR2, ToggleCoderStat);  
}

/*-------------------------------------------------------------------------*/


declare_listen(num_port)
    int num_port;

  {
    int size_of_ad_in;
    struct sockaddr_in ad_in;
    int sock;

    bzero((char *)&ad_in, sizeof(ad_in));
    ad_in.sin_family = AF_INET;
    ad_in.sin_port = htons((u_short)num_port);
    ad_in.sin_addr.s_addr = INADDR_ANY;

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      fprintf(stderr,"declare_listen : cannot create socket-controle UDP\n");
      exit(1);
    }
    size_of_ad_in = sizeof(ad_in);
    if (bind(sock, (struct sockaddr *) &ad_in, size_of_ad_in) < 0){
      perror("declare_listen'bind");
      close(sock);
      exit(1);
    }
    if(!num_port){
      getsockname(sock, (struct sockaddr *)&ad_in, &size_of_ad_in);
      fprintf(stderr, "Listening at %d port number\n", ad_in.sin_port);
    }
    return(sock);
  }    

/*-------------------------------------------------------------------------*/


int how_long_ago(sec, usec, timestamps, cpt)
     u_short sec, usec;
     TIMESTAMP timestamps[256];
     int cpt;
{
  /*
  *  Return n if (n-1) images have been encoded since the lost image.
  */
  register int i, j;

  for(i=0; i<255; i++){
    j = (cpt + 256 - i) % 256;
    if(equal_time(sec, usec, timestamps[j].sec, timestamps[j].usec))
      break;
  }
  if(DEBUG_MODE){
    if(i==255){
      printf("looking for sec: %d, usec:%d in the following table failed\n",
	     sec, usec);
      for(i=0; i<255; i++){
	printf("%d:\tsec:%d\tusec:%d\n", i, timestamps[i].sec, 
	       timestamps[i].usec);
      }
    }
  }
  return(i+1);
}

/*-------------------------------------------------------------------------*/


void CheckControlPacket(sock, cpt, timestamps, ForceGobEncoding,
			INC_NG, FULL_INTRA, SUPER_CIF, NACK_FIR_ALLOWED,
			current_time, TEMPO)
     int sock, cpt;
     TIMESTAMP timestamps[256];
     u_char *ForceGobEncoding;
     int INC_NG;
     BOOLEAN *FULL_INTRA, SUPER_CIF, NACK_FIR_ALLOWED;
     struct timeval *current_time;
     BOOLEAN TEMPO;
     
{
  static struct timeval tim_out, last_qos_check;
  static BOOLEAN FIRST=TRUE;
  static int mask0;
  static u_char loss_rate[101];
  struct sockaddr_in from;
  struct hostent *host;
  char name[LEN_MAX_NAME];
  BOOLEAN ERROR;
  int lr, len_from, mask, force_value, new_rate_max;
  int type, CN, FGOBL, LGOBL;
  u_int buf[64];
  u_short ts_sec, ts_usec;
  float c_loss;
  u_int32 *pt;
  rtp_hdr_t *h;
  rtp_t *r;

  /* Set up the select mask.  Keep the loss_rate stuff for the moment
   * but don't act on it.
   */
  
  if(FIRST){
    mask0 = 1 << sock;
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    bzero((char *)loss_rate, 101);
    last_qos_check.tv_sec = current_time->tv_sec;
    last_qos_check.tv_usec = current_time->tv_usec;
    FIRST = FALSE;
  }
  if(TEMPO)
    tim_out.tv_usec = 1000; /* 1ms polling tempo in PQ/control mode */
  else
    tim_out.tv_usec = 0;
  
  /* Zero out the GoBs that we want to be intra coded */
  if(SUPER_CIF){
    bzero((char *) ForceGobEncoding, 52);
  }else{
    bzero((char *) ForceGobEncoding, 13);
  }
  mask = mask0;
  len_from = sizeof(from);
  
  while(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) 
	!= 0){
    ERROR = FALSE;
    if ((lr=recvfrom(sock, (char *)buf, 255, 0, (struct sockaddr *)&from,
		     &len_from)) < 0){
      perror("recvfrom control packet");
      return;
    }
#ifdef MCDEBUG
    fprintf(stderr,"Got a packet\n");
#endif
    /* First sort out the fixed rtp header */          
    h = (rtp_hdr_t *) buf;
    if(h->ver != RTP_VERSION || h->format != RTP_H261_CONTENT){
      if(DEBUG_MODE){
	fprintf(stderr, "ParseRTCP: ver: %d, format: %d",
		h->ver, h->format);
      }
      ERROR = TRUE;
      goto after_parsing;
    }

    /* Check that there are some options present */
    if(!h->p){
      if(DEBUG_MODE)
	fprintf(stderr, "ParseRTCP: RTCP without option");
      ERROR = TRUE;
      goto after_parsing;
    }

    /* Start parsing the options */
    pt = (u_int32 *) (h+1);

    do{
      r = (rtp_t *) pt;
      pt += r->generic.length; /* point to end of option */

      /* 
      * Error if length of option goes beyond end of packet or zero
      * length option
      */
      
      if((((char *)pt - (char *)h) > lr) || (r->generic.length == 0)){
	if(DEBUG_MODE){
	  fprintf(stderr, "ParseRTCP: Invalid length field\n");
	  fprintf(stderr, "pt: %u, h: %u, lr: %d, length:%d",
		  (char *)pt, (char *)h, lr, r->generic.length);
	}
	ERROR=TRUE;
	goto after_parsing;
      }
      
      switch(type=r->generic.type) {
	/*
	* Add in the necessary code to detect MC rx returns and for
	* processing them
	*/
      case RTP_APP: 
	/* ignore anything except for the MCCP receive option */
	if(htonl(r->app_mccr.name) != RTP_APP_MCCR_VAL) {
	  if(DEBUG_MODE)
	    fprintf(stderr, "ParseRTCP: Invalid APP field\n");

	  ERROR=TRUE;
	  goto after_parsing;
	}
	/* 
	* Hand it off to the mccp processing machinery, with necessary
	* state and relevant stuff from rtp header
	*/
	mc_recvpkt(mcTxState, h, r);
	break;

	/* Full intra coded frame requested */
      case RTP_FIR:
	if(DEBUG_MODE){
	  fprintf(stderr, "FIR received\n");
	}
	if(!NACK_FIR_ALLOWED || !FULL_INTRA)
	  break;
	*FULL_INTRA = TRUE;
	break;

      case RTP_NACK:
	if(!NACK_FIR_ALLOWED || !FULL_INTRA)
	  break;
	FGOBL = r->nack.fgobl;
	LGOBL = r->nack.lgobl;
	CN = r->nack.size & 0x03;
	ts_sec = r->nack.ts_sec;
	ts_usec = r->nack.ts_usec;
	force_value = how_long_ago(ts_sec, ts_usec, timestamps, cpt);
	if(force_value > 255){
	  if(DEBUG_MODE){
	    fprintf(stderr, " Couln't find %ds, %dus timestamps image\n",
		    ts_sec, ts_usec);
	  }
	}else{
	  register int n;
	  
	  if(SUPER_CIF){
	    int aux=CN*13;
	    
	    for(n=FGOBL; n<=LGOBL; n+=INC_NG){
	      if((int)ForceGobEncoding[aux+n] < force_value)
		ForceGobEncoding[aux+n] = force_value;
	    }      
	  }else{
	    for(n=FGOBL; n<=LGOBL; n+=INC_NG){
	      if((int)ForceGobEncoding[n] < force_value)
		ForceGobEncoding[n] = force_value;
	    }
	  }
	}
	if(DEBUG_MODE){
	  if(FGOBL != LGOBL){
	    fprintf(stderr, 
		    "**NACK CIF %d, GOB %d to %d, %ds, %dus are lost\n", 
		    CN, FGOBL, LGOBL, ts_sec, ts_usec);
	  }else{
	    fprintf(stderr, 
		    "*NACK CIF %d, GOB %d, %ds, %dus is lost\n",
		    CN, FGOBL, ts_sec, ts_usec);
	  }
	}
	break;

      default:
	if(DEBUG_MODE)
	  fprintf(stderr, "Bad Type control packet: %d", type);
	ERROR = TRUE;
	goto after_parsing;
      }
    }while(!r->generic.final);
    
  after_parsing:  
    if(ERROR){
      if((host=gethostbyaddr((char *)(&(from.sin_addr)), 
			     sizeof(struct in_addr), from.sin_family))
	 == NULL){
	strcpy(name, (char *)inet_ntoa(from.sin_addr));
      }else{
	strcpy(name, host->h_name);
      }
      if(DEBUG_MODE)
	fprintf(stderr, " from %s\n", name);
    } 
    mask = mask0;
  } /* while data on socket */
  
  return;
}

/*-------------------------------------------------------------------------*/


void coffee_break(delay)
     long delay;
{
  struct timeval timer;
  static BOOLEAN FIRST=TRUE;
  static int sock;
  long sec, usec;

  if(FIRST){
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("coffee_break creation socket ");
      return;
    }
    FIRST = FALSE;
  }
  sec = delay / 1000000;
  usec = delay - 1000000*sec;
  timer.tv_sec = sec;
  timer.tv_usec = usec;
  select(sock+1, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timer);
  return;

}

/*-------------------------------------------------------------------------*/
void idx_code_bloc_8x8(bloc,  nb_coeff,inter, pbit_state)
 
    /*
    *  H.261 encoding of a 8x8 block of DCT coefficients.
    */
    char *bloc;
    int nb_coeff;
    int inter; /* Boolean: True if INTER mode*/
    TYPE_bit_state *pbit_state;
     
  {
    int run = 0; 
    register int level; 
    register int val, a;
    int plus, N=0;
    int first_coeff = 1; /* Boolean */
    register unsigned char *zig_zag_ptr;
    register int jj;
    register u_char *ptr_chaine_codee;
    register u_int byte4_codee;

    GET_bit_state((*pbit_state), ptr_chaine_codee, byte4_codee, jj);
    
    zig_zag_ptr = &ZIG_ZAG[0];
   
    for(a=0; a<64; a++){
      if(N==nb_coeff)
	  break;

      if(!(val= *(int *)(bloc+(*zig_zag_ptr++)))){
	run ++;
	continue;
      }else
	  N ++;
      if(val>0){
	level = val;
	plus = 1;
      }else{
	level = -val;
	plus = 0;
      }
      val = (run<<4) + level; /* run is encoded with 5 bits (to the left)
			      *  and level with 4 bits (to the right)
			      */
      if(first_coeff){
	first_coeff = 0;
	if(inter){
	  if((level == 1)&&(!run)){
	    /* Peculiar case : First coeff is 1 */
	    Mput_bit_ptr(ptr_chaine_codee,1,jj)
	    if(plus)
	      Mput_bit_ptr(ptr_chaine_codee,0,jj)
	    else
	      Mput_bit_ptr(ptr_chaine_codee,1,jj)
	    continue; 
	  }
	}else{
	  /* INTRA mode and CC --> encoding with an uniform quantizer */
	  if(level == 128)
	      level = 255;
	  Mput_value_ptr(ptr_chaine_codee, level, 8, jj)
	  continue;
	}
      }

      if(level<=15){ /*@@@@*/
	  register int _n;
	  register int _val;
	  register TYPE_HUFF_TAB *_ptr;

	  if((_n = val -  OFFSET_tab_tcoeff)<0 
	     || (val >=  MAX_tab_tcoeff) 
	     || !(_n = ((_ptr = (idx_tab_tcoeff + _n))->len)) ){
	    ; 
	  }else{
	    _val = _ptr->val;
	    Mput_value_ptr(ptr_chaine_codee, _val, _n, jj);
	    /* Sign encoding */
	    if(plus)
		Mput_bit_ptr(ptr_chaine_codee,0,jj)
	    else
		Mput_bit_ptr(ptr_chaine_codee,1,jj)
	  run = 0;
	  continue;
	  }
	}
      /*
      *  This code is not a Variable Length Code, so we have to encode it 
      *  with 20 bits.
      *  - 6 bits for ESCAPE (000001)
      *  - 6 bits for RUN
      *  - 8 bits for LEVEL
      */

      Mput_value_ptr(ptr_chaine_codee, 1, 6, jj)
      Mput_value_ptr(ptr_chaine_codee, run, 6, jj)


      if(plus)
	Mput_value_ptr(ptr_chaine_codee, level, 8, jj)
      else{
	Mput_bit_ptr(ptr_chaine_codee,1,jj)
	  level = 128 - level;
	Mput_value_ptr(ptr_chaine_codee, level, 7, jj)
      }
      run = 0;
    }
    /* EOB encoding (10) */

      Mput_value_ptr(ptr_chaine_codee, 2, 2, jj);

    PUT_bit_state((*pbit_state), ptr_chaine_codee, byte4_codee, jj);
  }
	
