/**************************************************************************\
*          Copyright (c) 1994 INRIA Sophia Antipolis, FRANCE.              *
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
*  File              : audio.c	                             		   *
*  Author            : Hugues CREPIN					   *
*  Last Modification : 1995/07/13                                          *
*--------------------------------------------------------------------------*
*  Description       : The procedure what receive and play the sound.      *
*                      Version 1.0                                         *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#include <stdio.h>

#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

#include "general_types.h"
#include "protocol.h"
#include "audio.h"
#include "rtp.h"

#ifdef CRYPTO
#include "ivs_secu.h"
#endif /* CRYPTO */

#define P_MAX 2000

#ifndef NO_AUDIO

static int nb_stations=0;
static LONGUEUR_DEMANDE=2000;

static BOOLEAN DEBUT = TRUE;
static BOOLEAN AUDIO_SET;
static BOOLEAN AUDIO_DROPPED;
static BOOLEAN ORDO_REQUESTED = FALSE;

static BOOLEAN MICRO_MUTED=TRUE;
static BOOLEAN Audio_Quit=FALSE;


static int encoding,squelch;
static int8 REDONDANCE=/*-1;*/PCM_64;/*RTP_LPC;*/
static int8 REDONDANCY_TYPE=PCM_64;
static u_char ttl;

static int alen_addr_r;
static struct sockaddr_in addr_r;
static int asock_r;

static BOOLEAN UNICAST=TRUE;
static BOOLEAN DEFAULT_DECODE=TRUE;

static char IP_GROUP[256], AUDIO_PORT[5];

static int VOLUME=0;
static BOOLEAN SPEAKER;

extern BOOLEAN DEBUG_MODE;
extern S_PARAM param;



#define NB_MAX_ST 10000

enum mode_t {NORMAL,DETECTION_PIC};

typedef struct
{
  double debut,var,tps_init,ni,ni1,ni2,di,dvi,pi,ni_start;
  enum mode_t mode;
  u_int32 nb_dvi;
} Param_Algo_t;

typedef struct paquet_p
{
  u_int8   validaudio:1;          /* set if audio if still valid   */
  u_int8   beginoftalkspurt:1;    /* set if the next packet is a   */
                                  /* begin of a talkspurt          */
  u_int8   reserved:6;
  int8   redondancy_type;             /* for 32 bits frontiere         */
  u_int16  lastarrived;           /* The last paquet arrived       */
  u_int32    actualplay;            /* the actual playing position   */
  double   sendtime;              /* The time when the packet was sent */
  double   arrivaltime;           /* The time when the packet was arrived */
  u_int32    playout;               /* The playout in byte*/
  u_int16    longueur;              /* Length of frame*/
  u_int16    longueur_data_prec;
  u_char   * data;                /* data of the frame */
  u_char   * data_prec;           /* data of the precedent frame */
  struct paquet_p *suivant;         /* the next paquet */
} paquet_t;

typedef struct
{
  u_long  sin_addr;        /* packet has been received from this address  */
  u_short identifier;         /* source identifier, set to 0 if no bridge */
  u_int8   audio_encoding:1;           /* set if the station encodes audio */
  u_int8   audio_decoding:1;             /* set if audio decoding selected */
  u_int8   audio_ignored:1;                     /* set if audio is ignored */
  u_int8   alwaysinit:1;             /* set if the station was always init */
  u_int8   reserved:4;
  u_int8   reserved2;
  Param_Algo_t parameter;
  paquet_t * p_paquet;
} STATION_AUDIO;

static STATION_AUDIO table[NB_MAX_ST];

void recoit();
static void Timeout();
static void joue();
static int ordonnance();
static short pcmu_to_l14();
static u_char l14_to_pcmu();
static void mix_init();
static void mix2_pcmu();
static paquet_t * Insere();
static paquet_t * Remove();
static paquet_t * RemoveEnTete();
static paquet_t * InitList();
static paquet_t * PurgeList();


static int  encode(outbuf,inbuf,encoding,length)
     char * outbuf, * inbuf;
     int encoding,length;

{
  int longueur=0;
  
  if (encoding==-1)
    return 0;

  switch(encoding){
  case RTP_PCM_CONTENT:
    memcpy(outbuf, inbuf, length);
    longueur=length;
    break;
  case RTP_ADPCM_4:
    longueur = adpcm4_coder(inbuf, outbuf, length);
    break;
  case RTP_VADPCM_CONTENT:
    longueur = vadpcm_coder(inbuf, outbuf, length);
    break;
  case RTP_ADPCM_6:
    longueur = adpcm6_coder(inbuf, outbuf, length);
    break;
  case RTP_ADPCM_5:
    longueur = adpcm5_coder(inbuf, outbuf, length);
    break;
  case RTP_ADPCM_3:
    longueur = adpcm3_coder(inbuf, outbuf, length);
    break;
  case RTP_ADPCM_2:
    longueur = adpcm2_coder(inbuf, outbuf, length);
    break;
  case RTP_LPC:
    longueur = coder_lpc(inbuf, outbuf, length);
    break;
  case RTP_GSM_13:
    longueur=gsm_coder(inbuf, outbuf, length);
    break;
  }
  return longueur;
}



/* return the length of the decoded frame */
static unsigned int DecodeAudio(inbuf, soundel, format, outbuf)
     unsigned char *inbuf, *outbuf;
     int soundel;
     u_char format;
{
    int len;

    if (soundel<=0)
      return 0;
    
    switch(format){
    case RTP_PCM_CONTENT:
      memcpy((char *)outbuf,(char *)inbuf,soundel);
      len=soundel;
      break;
    case RTP_ADPCM_4:
      len=adpcm4_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_VADPCM_CONTENT:
      len=vadpcm_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_ADPCM_6:
      len=adpcm_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_ADPCM_5:
      len=adpcm_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_ADPCM_3:
      len=adpcm3_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_ADPCM_2:
      len=adpcm2_decoder(inbuf, outbuf, soundel);
      break;
    case RTP_LPC:
      len=decoder_lpc(inbuf, outbuf, soundel);
      break;
    case RTP_GSM_13:
      len=gsm_decoder(inbuf, outbuf, soundel);
      break;
    default:
      return 0;
    }
    return len;
  }



void SetRedondancy(redon)
     int redon;
{
  REDONDANCE=(int8)redon;
}

void SetMicroMuted()
{
  MICRO_MUTED=TRUE;
}


void UnSetMicroMuted()
{
  MICRO_MUTED=FALSE;
}

BOOLEAN IsMicroMuted()
{
  return MICRO_MUTED;
}

void SetEncoding(encod)
     int encod;
{
  encoding=encod;
}

int Encoding()
{
  return encoding;
}

void SetSquelch(squ)
     int squ;
{
  squelch=squ;
}

int Squelch()
{
  return squelch;
}

void AudioQuit()
{
  Audio_Quit=TRUE;
}



static void ToggleMicroOn()
{
  soundplayvol(VOLUME);  
}

static void ToggleMicroOff()
{
  soundplayvol(0);  
}

int AudioEncode(coding, MICRO_MUTE)
     int coding;
     BOOLEAN MICRO_MUTE;
{
  static int sock, len_addr;
  static struct sockaddr_in addr;
  unsigned int inbuf[P_MAX/4], outbuf[P_MAX/4];
  static unsigned int redondancy_buf[P_MAX/4];
  static unsigned char *ptr_redondancy_buf=(unsigned char *)redondancy_buf;
  static unsigned char grabber[P_MAX*2];
  static unsigned char *ptr_grabber=grabber;
  static int16 redondancy_length=0;
  
  static int counter=0; /* the number of sample */
  static unsigned int *buf;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t * r; /* RTP */
  rtp_t * s;
  static u_int16 cpt=1;
  static u_short port_audio_source;
  static u_int8 end_of_talkspurt = 0 ; /* this is the mark of end of 
					  talkspurt */
  static u_short end_emmit = 2;
  static BOOLEAN FIRST=TRUE, SOUND_RECORD=FALSE;
  int format=0;
  struct timeval send_time;
  static BOOLEAN Precedent_state=TRUE; /* this is the precedent_state of the
					  micro mute*/
  int len_packet;
  char *buf_to_send;
#ifdef CRYPTO
  u_int buf_crypt[P_MAX/4 + 2];
#endif

/* we fill the RTP Header */
  
  h = (rtp_hdr_t *)outbuf;
  h->ver = 1;
  h->channel = 0;
  h->p = 1;
  h->s = 0; /* now */
  h->format = (u_int8)(0x0003F & encoding);
  h->seq = cpt;

  r = (rtp_t *) &outbuf[2];
  r->app_ivs_audio.final = 1;
  r->app_ivs_audio.type = RTP_APP;
  r->app_ivs_audio.length = 3;
  r->app_ivs_audio.subtype = 0;
  strncpy((char *) &(r->app_ivs.name), "IVS ", 4);
  r->app_ivs_audio.index=0;
  r->app_ivs_audio.valpred=0;

/* we detect if it is the first time what the procedure is called */
  
  if (FIRST)
    {
      FIRST=FALSE;
      Audio_Quit=FALSE;
      switch (coding) {
      case PCM_64:
	format = RTP_PCM_CONTENT;
	break;
      case ADPCM_4:
	format = RTP_ADPCM_4;
	break;
      case VADPCM:
	format = RTP_VADPCM_CONTENT;
	break;
      case ADPCM_6:
	format = RTP_ADPCM_6;
	break;
      case ADPCM_5:
	format = RTP_ADPCM_5;
	break;
      case ADPCM_3:
	format = RTP_ADPCM_3;
	break;
      case ADPCM_2:
	format = RTP_ADPCM_2;
	break;
      case GSM_13:
	format = RTP_GSM_13;
	break;
      case LPC:
	format = RTP_LPC;
	break;
      }
      
/* We set the different parameters for the audio encoding */

      SetEncoding(format);
      SetSquelch(squelch);
      Audio_Quit=FALSE;
      sock = CreateSendSocket(&addr, &len_addr, AUDIO_PORT, IP_GROUP, &ttl,
			      &port_audio_source, UNICAST, param.ni);

#if defined(SPARCSTATION) || defined(SGISTATION)
      if(issoundrecordinit() == FALSE){
	if(!soundrecordinit()){
	  fprintf(stderr, "audio_encode: Cannot open audio device.\n");
/* if we haven't succed we close the socket */
	  
	  close(sock);
	  FIRST=TRUE;
	  return 0;
	}
      }
#ifdef SPARCSTATION
      soundplayflush();
      soundrecflush();
#endif
      
#endif
#ifdef AF_AUDIO
      if(!audioInit()){   
	fprintf(stderr,"audio_coder: cannot connect to AF AudioServer\n");
      }else{
	sounddest(0);
      }
#endif /* AF_AUDIO */
      
      if (MICRO_MUTE)
	SetMicroMuted();
      else
	UnSetMicroMuted();

      if (soundrecord())  
	{
	  SOUND_RECORD=TRUE;
	  if(encoding == RTP_PCM_CONTENT){
	    buf = inbuf;
	  }
	}
      ptr_grabber=grabber;
      counter=0;
      
#if defined(SPARCSTATION) || defined(SGISTATION)
      soundrecordterm();
#endif
    }/* end of if(FIRST) */
  if (SOUND_RECORD)
    {
      int soundel;

      if (Audio_Quit)
	{
	  Audio_Quit=FALSE;
	  SOUND_RECORD=FALSE;
	  FIRST=TRUE;
	  close(sock);
	  SetMicroMuted();
	  soundstoprecord();
#if defined(SPARCSTATION) || defined(SGISTATION)	  
	  soundrecordterm();
#endif
	  ToggleMicroOn();
	  Precedent_state=TRUE;
#ifdef SPARCSTATION
	  soundrecflush();
#endif
	  return 0;
	}

      h->format=(u_int8)(0x0003F & encoding);
      if(encoding == RTP_PCM_CONTENT)
	{
	  buf = inbuf;
	}
      
      if(MICRO_MUTED){
	if (end_emmit<2)
	  {
	    /*il faut flusher si il reste qq chose dans la file*/
	    /* we must send if anything is present in the queue */
	    {
	      int i;
	      while(counter>=frame_length){
		soundel=frame_length;
		memcpy((char *)buf, (char *)grabber, frame_length);
		if (redondancy_length)
		  {
		    s=(rtp_t *)&outbuf[2];
		    s->app_ivs_audio_redondancy.type=RTP_APP;
		    s->app_ivs_audio_redondancy.final = 1;
		    if (redondancy_length%4)
		      s->app_ivs_audio_redondancy.length=
			redondancy_length/4+5;
		    else
		      s->app_ivs_audio_redondancy.length=redondancy_length/4+4;
		    memcpy((char *)&(s->app_ivs_audio_redondancy.data),
			   (char *)ptr_redondancy_buf,
			   redondancy_length);
		    h->p = 1;
		    s->app_ivs_audio_redondancy.subtype = 1;
		    s->app_ivs_audio_redondancy.encoding_type =
		      REDONDANCY_TYPE;
		    strncpy((char *) &(s->app_ivs.name), "IVS ", 4);
		    s->app_ivs_audio_redondancy.index= 0x33;
		    s->app_ivs_audio_redondancy.length_data= redondancy_length;
		    soundel=encode((char *)&outbuf[2+
					   s->app_ivs_audio_redondancy
					   .length],
			       (char *)buf,encoding,soundel);
		  }
		else
		  {
		    h->p = 0;
		    soundel=encode((char *)&outbuf[2],
				   (char *)buf,encoding,soundel);
		  }
		end_emmit=0;
		h->seq = cpt;
		h->s=end_of_talkspurt;
		end_of_talkspurt=0;
		if (redondancy_length)
		  {
		    gettimeofday(&send_time, (struct timezone *)NULL);
		    h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
		    h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

		    len_packet = soundel + 8 + 
		      s->app_ivs_audio_redondancy.length*4;
		    buf_to_send = (char *)outbuf;
#ifdef CRYPTO
		    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		      test_key("audio_coder");
#endif
		      len_packet = method[current_method].crypt_rtp(
				  (char *)outbuf, len_packet, s_key, len_key, 
				  (char *)buf_crypt);
		      buf_to_send = (char *)buf_crypt;
		    }	    
#endif /* CRYPTO */	    
   
		    if(sendto(sock, buf_to_send, len_packet, 0,
			      (struct sockaddr *) &addr, len_addr) < 0)
		      {
			perror("audio_coder: when sending datagram packet");
		      }
		  }
		else
		  {
		    gettimeofday(&send_time, (struct timezone *)NULL);
		    h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
		    h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

		    len_packet = soundel + 8;
		    buf_to_send = (char *)outbuf;
#ifdef CRYPTO
		    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		      test_key("audio_coder");
#endif
		      len_packet = method[current_method].crypt_rtp(
				  (char *)outbuf, len_packet, s_key, len_key, 
				  (char *)buf_crypt);
		      buf_to_send = (char *)buf_crypt;
		    }	    
#endif /* CRYPTO */	    
		    
		    if(sendto(sock, buf_to_send, len_packet, 0,
			      (struct sockaddr *) &addr, len_addr) < 0)
		      {
			perror("audio_coder: when sending datagram packet");
		      }
		  }
		if (REDONDANCE!=-1)
		  {
		    REDONDANCY_TYPE=REDONDANCE;
		    redondancy_length=
		      encode((char *)ptr_redondancy_buf,(char *)buf,
			     REDONDANCY_TYPE,frame_length);
		  }
		else
		  redondancy_length=0;
		cpt = (u_int16)(cpt +1) % 0xFFFF;
		if (counter>frame_length)
		  {
		    for (i=0;i<counter-frame_length;i++)
		      grabber[i]=grabber[i+frame_length];
		  }
		counter-=frame_length;
	      }
	    }
	    
	    if (redondancy_length)
	      {
		s=(rtp_t *)&outbuf[2];
		s->app_ivs_audio_redondancy.type=RTP_APP;
		s->app_ivs_audio_redondancy.final = 1;
		if (redondancy_length%4)
		  s->app_ivs_audio_redondancy.length=redondancy_length/4+5;
		else
		  s->app_ivs_audio_redondancy.length=redondancy_length/4+4;
		memcpy((char *)&(s->app_ivs_audio_redondancy.data),
		       (char *)ptr_redondancy_buf,
		       redondancy_length);

		s->app_ivs_audio_redondancy.subtype = 1;
		s->app_ivs_audio_redondancy.encoding_type = REDONDANCY_TYPE;
		strncpy((char *) &(s->app_ivs.name), "IVS ", 4);
		s->app_ivs_audio_redondancy.index= 0x33;
		s->app_ivs_audio_redondancy.length_data= redondancy_length;
	      }
	    else
	      {
		h->p = 0;
		soundel=encode((char *)&outbuf[2],
			       (char *)buf,encoding,soundel);
	      }
	    h->seq = cpt;
	    h->s=1;
	    end_of_talkspurt=0;
	    gettimeofday(&send_time, (struct timezone *)NULL);
	    h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
	    h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);
	    h->p=0;
	    if (redondancy_length)
	      {
		h->p=1;
		len_packet = 8 + s->app_ivs_audio_redondancy.length*4;
		/******** to check ********************/
		buf_to_send = (char *)outbuf;
#ifdef CRYPTO
		if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		    test_key("audio_coder");
#endif
		    len_packet = method[current_method].crypt_rtp(
			(char *)outbuf, len_packet, s_key, len_key, 
			(char *)buf_crypt);
		    buf_to_send = (char *)buf_crypt;
		}	    
#endif /* CRYPTO */	    	
		if(sendto(sock, buf_to_send, len_packet, 0,
			  (struct sockaddr *) &addr, len_addr) < 0)
		  {
		    perror("audio_coder: when sending datagram packet");
		  }
	      }
	    else {
	      len_packet = 8; /***************** to check ****************/
	      buf_to_send = (char *)outbuf;
#ifdef CRYPTO
	      if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		test_key("audio_coder");
#endif
		len_packet = method[current_method].crypt_rtp(
				  (char *)outbuf, len_packet, s_key, len_key, 
				  (char *)buf_crypt);
		buf_to_send = (char *)buf_crypt;
	      }	    
#endif /* CRYPTO */	    
	      
	      if(sendto(sock, buf_to_send, len_packet, 0,
			(struct sockaddr *) &addr, len_addr) < 0)
		{
		  perror("audio_coder: when sending datagram packet");
		}
	    }
	    redondancy_length=0;
	    cpt = (u_int16)(cpt +1) % 0xFFFF;
	    end_emmit=2;
	    ptr_grabber=grabber;
	    counter=0;
#if defined(SPARCSTATION) || defined(SGISTATION)
	    soundrecordterm();
#endif
	    ToggleMicroOn();
#ifdef SPARCSTATION
	    soundplayflush();
	    soundrecflush();
#endif

	  }
	if (!Precedent_state)
	  ToggleMicroOn();
	Precedent_state=TRUE;
	return 1;
      }/* if Micromuted*/
      if (Precedent_state)
	{
	  if (SPEAKER)
	    ToggleMicroOff();
#ifdef SPARCSTATION
	  soundplayflush();
	  soundrecflush();
#endif
	  Precedent_state=FALSE;
	}

      /* Get the sound ... */
      {
#if defined(SPARCSTATION) || defined(SGISTATION)
	if(issoundrecordinit() == FALSE)
	  {
	    if(!soundrecordinit(O_RDONLY))
	      {
		fprintf(stderr,
			"audio_encode: 2nd call Cannot open audio device.\n");
	      }
	  }
#endif
	ptr_grabber=grabber+counter;
	counter += soundgrab(ptr_grabber, 2*P_MAX-counter);
      }
      if (counter<frame_length){
	return 1;
      }
      soundel=frame_length;
      memcpy((char *)buf, (char *)grabber, frame_length);
      
      if (soundel > 0) {
	register u_char *start = (u_char *)buf;
	register int i;
	int squelched = (squelch > 0);
	
	
	/* If entire buffer is less than squelch, ditch it. */
	if (squelch > 0) {
	  for (i=0; i<soundel; i++) {
	    if ((int)((*start++ & 0x7F) ^ 0x7F) > squelch) {
	      squelched = 0;
	      break;
	    }
	  }
	}
	if (!squelched){
	  end_emmit=0;
	  h->seq = cpt;
	  h->s=end_of_talkspurt;
	  end_of_talkspurt=0;

	  if (redondancy_length)
	    {
	      s=(rtp_t *)&outbuf[2];
	      s->app_ivs_audio_redondancy.type=RTP_APP;
	      s->app_ivs_audio_redondancy.final = 1;
	      if (redondancy_length%4)
		s->app_ivs_audio_redondancy.length=redondancy_length/4+5;
	      else
		s->app_ivs_audio_redondancy.length=redondancy_length/4+4;
	      memcpy((char *)&(s->app_ivs_audio_redondancy.data),
		     (char *)ptr_redondancy_buf,
		     redondancy_length);
	      h->p = 1;
	      s->app_ivs_audio_redondancy.subtype = 1;
	      s->app_ivs_audio_redondancy.encoding_type = REDONDANCY_TYPE;
	      strncpy((char *) &(s->app_ivs.name), "IVS ", 4);
	      s->app_ivs_audio_redondancy.index= 0x33;
	      s->app_ivs_audio_redondancy.length_data= redondancy_length;
	      soundel=encode((char *)&outbuf[2+
				     s->app_ivs_audio_redondancy.length],
			     (char *)buf,encoding,soundel);
	      gettimeofday(&send_time, (struct timezone *)NULL);
	      h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
	      h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

	      len_packet = soundel + 8 + s->app_ivs_audio_redondancy.length*4;
	      buf_to_send = (char *)outbuf;
#ifdef CRYPTO
	      if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		test_key("audio_coder");
#endif
		len_packet = method[current_method].crypt_rtp(
				  (char *)outbuf, len_packet, s_key, len_key, 
				  (char *)buf_crypt);
		buf_to_send = (char *)buf_crypt;
		}	    
#endif /* CRYPTO */	    
	      if(sendto(sock, buf_to_send, len_packet, 0,
			(struct sockaddr *) &addr, len_addr) < 0)
		{
		  perror("audio_coder: when sending datagram packet");
		}

	    }
	  else
	    {
	      h->p = 0;
	      soundel=encode((char *)&outbuf[2],
			     (char *)buf,encoding,soundel);
	      gettimeofday(&send_time, (struct timezone *)NULL);
	      h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
	      h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

	      len_packet = soundel + 8;
	      buf_to_send = (char *)outbuf;
#ifdef CRYPTO
	      if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		test_key("audio_coder");
#endif
		len_packet = method[current_method].crypt_rtp(
				   (char *)outbuf, len_packet, s_key, len_key, 
				   (char *)buf_crypt);
		buf_to_send = (char *)buf_crypt;
		}	    
#endif /* CRYPTO */	   
	      if(sendto(sock, buf_to_send, len_packet, 0,
			(struct sockaddr *) &addr, len_addr) < 0)
		{
		  perror("audio_coder: when sending datagram packet");
		}

	    }

	  
	  if (REDONDANCE!=-1)
	    {
	      REDONDANCY_TYPE=REDONDANCE;
	      redondancy_length=
		encode((char *)ptr_redondancy_buf,(char *)buf,
		       REDONDANCY_TYPE,frame_length);
	    }
	  else
	    redondancy_length=0;
	  cpt = (u_int16)(cpt +1) % 0xFFFF;
	}
	else
	  {
	    if (end_emmit < 2)
	      {
		end_emmit++;
		if (end_emmit==2)
		  {
		    end_of_talkspurt = 1;
		    Precedent_state=TRUE;
		  }
		h->seq = cpt;
		h->s=end_of_talkspurt;
		end_of_talkspurt=0;
		if (redondancy_length)
		  {
		    s=(rtp_t *)&outbuf[2];
		    s->app_ivs_audio_redondancy.type=RTP_APP;
		    s->app_ivs_audio_redondancy.final = 1;
		    if (redondancy_length%4)
		      s->app_ivs_audio_redondancy.length=
			redondancy_length/4+5;
		    else
		      s->app_ivs_audio_redondancy.length=redondancy_length/4+4;
		    memcpy((char *)&(s->app_ivs_audio_redondancy.data),
			   (char *)ptr_redondancy_buf,
			   redondancy_length);
		    h->p = 1;
		    s->app_ivs_audio_redondancy.subtype = 1;
		    s->app_ivs_audio_redondancy.encoding_type =
		      REDONDANCY_TYPE;
		    strncpy((char *) &(s->app_ivs.name), "IVS ", 4);
		    s->app_ivs_audio_redondancy.index= 0x33;
		    s->app_ivs_audio_redondancy.length_data= redondancy_length;
		    soundel=encode((char *)&outbuf[2+
					   s->app_ivs_audio_redondancy.length],
				   (char *)buf,encoding,soundel);
		    gettimeofday(&send_time, (struct timezone *)NULL);
		    h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
		    h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

		    len_packet = soundel + 8 +
		      s->app_ivs_audio_redondancy.length*4;
		    buf_to_send = (char *)outbuf;
#ifdef CRYPTO
		    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		      test_key("audio_coder");
#endif
		      len_packet = method[current_method].crypt_rtp(
				  (char *)outbuf, len_packet, s_key, len_key, 
				  (char *)buf_crypt);
		      buf_to_send = (char *)buf_crypt;
		    }	    
#endif /* CRYPTO */	    

		    if(sendto(sock, buf_to_send, len_packet, 0,
			      (struct sockaddr *) &addr, len_addr) < 0)
		      {
			perror("audio_coder: when sending datagram packet");
		      }
		    
		  }
		else
		  {
		    h->p = 0;
		    soundel=encode((char *)&outbuf[2],
				   (char *)buf,encoding,soundel);
		    gettimeofday(&send_time, (struct timezone *)NULL);
		    h->ts_sec = (u_short) (send_time.tv_sec & 0xffff);
		    h->ts_usec = (u_short) ((send_time.tv_usec >> 4) & 0xffff);

		    len_packet = soundel + 8;
		    buf_to_send = (char *)outbuf;
#ifdef CRYPTO
		    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		      test_key("audio_coder");
#endif
		      len_packet = method[current_method].crypt_rtp(
				   (char *)outbuf, len_packet, s_key, len_key, 
				   (char *)buf_crypt);
		      buf_to_send = (char *)buf_crypt;
		    }	    
#endif /* CRYPTO */	    
		    
		    if(sendto(sock, buf_to_send, len_packet, 0,
			      (struct sockaddr *) &addr, len_addr) < 0)
		      {
			perror("audio_coder: when sending datagram packet");
		      }
		  }
		if (end_emmit==2)
		  {
		    end_of_talkspurt = 1;
		    redondancy_length = 0;
		  }
		else
		  if (REDONDANCE!=-1)
		    {
		      REDONDANCY_TYPE=REDONDANCE;
		      redondancy_length=
			encode((char *)ptr_redondancy_buf,(char *)buf,
			       REDONDANCY_TYPE,
			       frame_length);
		    }
		  else
		    redondancy_length=0;
		
		cpt = (u_int16)(cpt +1) % 0xFFFF;
	      }
	  }
	if (counter>frame_length)
	  {
	    for (i=0;i<counter-frame_length;i++)
	      grabber[i]=grabber[i+frame_length];
	  }
	counter -= frame_length;
      }
    }
return 1;
}



/* --------------------Function for the receiver-------------------------*/

void restoreinitialstate()
{
  int i;
  for (i=0;i<nb_stations;i++)
    table[i].audio_decoding=!table[i].audio_ignored;
  
}

static int SinToIndice(address,ident)
     u_long  address;        /* packet has been received from this address  */
     u_short ident;             /* source identifier, set to 0 if no bridge */
{
  register int i;
  for (i=0;i<nb_stations;i++)
    if (table[i].sin_addr==address && table[i].identifier==ident)
      return i;
  return -1;
}

static void IndiceToSin(indice,address,ident)
     int indice;                                  /* the indice in the table */
     u_long  * address;       /* packet has been received from this address  */
     u_short * ident;            /* source identifier, set to 0 if no bridge */
{
  if (indice<=nb_stations)
    {
      *address=table[indice].sin_addr;
      *ident=table[indice].identifier;
    }
  else
    {
      *address=0;
      *ident=0;
    }
}
     
void SetAudioIgnored(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_ignored=TRUE;
}

void UnSetAudioIgnored(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_ignored=FALSE;
}


BOOLEAN IsAudioIgnored(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    return table[indice].audio_ignored;
  else
    return DEFAULT_DECODE;
}

void SetAudioDecoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_decoding=TRUE;
}

void UnSetAudioDecoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;     /* source identifier, set to 0 if no bridge */
			
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_decoding=FALSE;
}  

BOOLEAN IsAudioDecoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;     /* source identifier, set to 0 if no bridge */
			
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    return table[indice].audio_decoding;
  return FALSE;
}  

void SetAudioEncoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_encoding=TRUE;
}

void UnSetAudioEncoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    table[indice].audio_encoding=FALSE;
}

BOOLEAN IsAudioEncoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    return table[indice].audio_encoding;
  return FALSE;
}

BOOLEAN IsStationDeclare(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    return TRUE;
  else
    return FALSE;
}


void Listen(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    {
      table[indice].audio_ignored=FALSE;
      table[indice].audio_decoding=TRUE;
    }
}


void Mute(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    {
      table[indice].audio_ignored=TRUE;
      table[indice].audio_decoding=FALSE;
    }
}


BOOLEAN NewAudioStation(address,ident,audio_encod,audio_decod,audio_igno)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
     u_int8   audio_encod;               /* set if the station encodes audio */
     u_int8   audio_decod;                 /* set if audio decoding selected */
     u_int8   audio_igno;                         /* set if audio is ignored */
     
{
  int indice;
  paquet_t paquet;

  indice=SinToIndice(address,ident);
  if (indice>=0)
    {
      table[indice].audio_encoding=audio_encod;
      table[indice].audio_decoding=audio_decod;
      table[indice].audio_ignored=audio_igno;
      return TRUE;
    }
  else
    if (nb_stations<NB_MAX_ST)
      {
	table[nb_stations].sin_addr=address;
	table[nb_stations].identifier=ident;
	table[nb_stations].audio_encoding=audio_encod;
	table[nb_stations].audio_decoding=audio_decod;
	table[nb_stations].audio_ignored=audio_igno;
	table[nb_stations].alwaysinit=TRUE;
	paquet.beginoftalkspurt = 1;
	paquet.validaudio = 0;
	paquet.redondancy_type=(int8)-1;
	paquet.actualplay = 0;
	paquet.sendtime = 0;
	paquet.lastarrived = 0;
	paquet.arrivaltime = 0;
	paquet.playout = 0;
	paquet.longueur = 0;
	paquet.data = 0;
	paquet.data_prec = 0;
	table[nb_stations].p_paquet = (paquet_t *)InitList();
	table[nb_stations].p_paquet = (paquet_t *)Insere(table[nb_stations].p_paquet,&paquet);
	table[nb_stations].parameter.debut=0;
	table[nb_stations].parameter.var=0;
	table[nb_stations].parameter.tps_init=0;
	table[nb_stations].parameter.ni=0;
	table[nb_stations].parameter.ni1=0;
	table[nb_stations].parameter.ni2=0;
	table[nb_stations].parameter.di=0;
	table[nb_stations].parameter.dvi=0;
	table[nb_stations].parameter.pi=0;
	table[nb_stations].parameter.ni_start=0;
	table[nb_stations].parameter.nb_dvi=1;
	nb_stations++;
	return TRUE;
      }
    return FALSE;
}

BOOLEAN RemoveAudioStation(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  int indice;
  indice=SinToIndice(address,ident);
  if (indice>=0)
    {
      PurgeList(table[indice].p_paquet);
      for (;indice<nb_stations-1;indice++)
	table[indice]=table[indice+1];
      nb_stations--;
      return TRUE;
    }
  return FALSE;
}

void RemoveAudioAll()
{
  int i;
  for (i=0;i<nb_stations;i++)
    {
      PurgeList(table[i].p_paquet);
    }
  bzero((char *)table,sizeof(table));
  nb_stations=0;
}

void InitAudio(default_decoding)
     BOOLEAN default_decoding;
{
 bzero((char *)table,sizeof(table));
 DEFAULT_DECODE=default_decoding;
}




void InitAudioSocket(port,group,Ttl,Squelch,Unicast)
     char *port;
     char *group; /* IP address style "225.23.23.6" = "a4.a3.a2.a1" */
     u_char Ttl;
     int Squelch;
     BOOLEAN Unicast;
{
  strcpy(AUDIO_PORT,port);
  strcpy(IP_GROUP,group);
  UNICAST=Unicast;
  ttl=Ttl;
  squelch=Squelch;
}


void AudioSocketReceive(audio_socket)
     int *audio_socket;
{
  *audio_socket=asock_r = CreateReceiveSocket(&addr_r, &alen_addr_r,
					      AUDIO_PORT, IP_GROUP, UNICAST,
					      param.ni);
}



static BOOLEAN ShallIDecode(sin_addr, identifier, i_station)
     u_long sin_addr;
     u_short identifier;
     int *i_station;
{
  int indice;
 
  indice=SinToIndice(sin_addr,identifier);
  *i_station=indice;
  if (indice>=0)
    {
      if(table[indice].audio_decoding)
	return(TRUE);
      else
	return(FALSE);
    }
  return(FALSE);  
}


BOOLEAN AnyAudioDecoding()
{
  register int i;

  for(i=0; i<nb_stations; i++){
    if(table[i].audio_decoding && !table[i].audio_ignored)
      return(TRUE);
  }
  return(FALSE);
}

void SetVolume(vol)
     int vol;
{
  VOLUME=vol;
}

void SetSpeaker(speak)
     BOOLEAN speak;
{
  SPEAKER=speak;
}


/*-------------------------ManageAudio--------------------------------------*/


int ManageAudio(audio_set, audio_dropped)
     BOOLEAN audio_set;
     BOOLEAN audio_dropped;
{
  int NOSTATION;
  static u_char BUFFER_AUDIO[BUFFER_AUDIO_LENGTH];
  static int OLDLOCALPLAY=0, GLOBALPLAY=0;
  AUDIO_SET=audio_set;
  AUDIO_DROPPED=audio_dropped;

  if(ORDO_REQUESTED || !DEBUT){
    ORDO_REQUESTED = FALSE;
    NOSTATION=ordonnance(BUFFER_AUDIO,&OLDLOCALPLAY,&GLOBALPLAY);
    Timeout(NOSTATION,OLDLOCALPLAY,GLOBALPLAY);
    joue(BUFFER_AUDIO,&OLDLOCALPLAY,&GLOBALPLAY);
  }
  return DEBUT;
}



/* This procedure return the time where the songs must be play */
static double algo(parametre,recepteur,emetteur,bot)
     Param_Algo_t * parametre;
     double recepteur;
     double emetteur;
     int bot;
/* cette procedure renvoie le temps auquel doit etre joue le morceau */
     

{
/* The first time we set the parameter at the adapted value */
  if (parametre->debut==0.0)
    {
      parametre->ni=recepteur-emetteur;
      parametre->ni1=parametre->ni;
      parametre->di=parametre->ni;
      parametre->dvi=0.0;
      parametre->pi=recepteur;
      parametre->debut=emetteur;
      parametre->tps_init=parametre->pi;
      parametre->ni_start=parametre->ni;
      parametre->nb_dvi=1;
      parametre->mode=NORMAL;
      parametre->ni2=0.0;
    }
  else
    {
/* After we run the algorithm */      
      parametre->ni=recepteur-emetteur;
/* We detect if it is a pike */      
      if (parametre->mode==NORMAL)
	{
	  if (parametre->ni>parametre->ni1)
	    parametre->mode=DETECTION_PIC;
	}
      else
	{
	  if (parametre->ni1>parametre->ni)
	    {
	      parametre->mode=NORMAL;
	      (parametre->nb_dvi)++;
	      if (parametre->nb_dvi==0)
		parametre->dvi=0;
	      else
		parametre->dvi=parametre->dvi*
		  (1-1/(float)(parametre->nb_dvi))+
		  ((parametre->ni1-parametre->ni
		    +parametre->ni1-parametre->ni2)/2.0)/
		    (float)(parametre->nb_dvi);
	    }
	  else
	    {
	      parametre->ni_start=parametre->ni;
	    }
	}
      parametre->ni2=parametre->ni1;
      parametre->ni1=parametre->ni;

      if (bot==1)
	{
	  parametre->debut=emetteur;
	  parametre->pi=parametre->dvi+parametre->ni_start+emetteur;
	  parametre->tps_init=parametre->pi;
	}
      else
	parametre->pi=parametre->tps_init+emetteur-parametre->debut;
    }
  return parametre->pi;
}



/*Reception*/
/* This procedure take data and insert them at the good place */
/* for each station */
/* asock_r is the audio socket recieve number */

static char *RedondanceItems[] = {
  " PCM ", "ADPCM-6", "ADPCM-5", "ADPCM-3", 
  "ADPCM-2", "ADPCM-4", "LPC", "ADPCM-42", 
  "LPC2", "GSM-13", "", "", "", "", "", "VADPCM","NONE"
};

void reception(sin_addr,identifi)
     u_long * sin_addr;
     u_short * identifi;
{
  static struct sockaddr_in from;
  static int fromlen;
  u_int pbuf[P_MAX/4],poutbuf[BUFFER_AUDIO_LENGTH/4];
  u_char * outbuf = (u_char *)poutbuf;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *option; /* RTP option (CRSC) */
  u_long * pt;
  u_short identifier;
  int lr, i_station=-1;
  u_short seq;
  BOOLEAN DECODING_REQUIRED;
  static struct timeval realtime, sendtime;
  u_int16 longueur;
  static paquet_t paquet;
  static BOOLEAN FIRST = TRUE;
  static int compt=0;
  double proba;

  static char last_redondancy=-1;

#ifdef DEBUG
  struct trace {
    unsigned int numero;
    int begin_of_t;
    int longueur;
    double emetteur;
    double recepteur;
    u_long adress;
  } TRACE;
  static FILE *file_trace;


#endif

  *sin_addr=*identifi=0;
  
  if (FIRST)
    {
      fromlen = sizeof(from);
      FIRST = FALSE;
      mix_init();
#ifdef DEBUG      
      if ((file_trace=fopen("mesure.dat","w"))==0)
	{
	  fprintf(stderr,"Impossible d'ouvrir le fichier de mesure \n");
	  exit(8);
	}
#endif      
    }
  
  bzero((char *)&from, fromlen);
  /* READS THE INCOMING PACKET */
  if((lr=recvfrom(asock_r, (char *)pbuf, P_MAX, 0, 
		  (struct sockaddr *)&from, &fromlen)) < 0) {
#ifdef DEBUG
    {
      perror("audio_decode (recvfrom) :");
    }
#endif
    return;
  }
#ifdef LOSS
  {
      static int cpt=0;

      cpt ++;
      if(cpt % 5 == 1) /* 20 % loss */
	  return;
  }
#endif /* LOSS */
#ifdef CRYPTO
  if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
      test_key("audio_decoder");
#endif
      lr = method[current_method].uncrypt_rtp((char *)pbuf, lr, s_key, 
					      len_key, (char *)pbuf);
  }
#endif /* CRYPTO */

  
  h = (rtp_hdr_t *)pbuf;
  pt = (u_long *) (h+1);/*pbuf[8]*/
  identifier=0;
  paquet.redondancy_type=(int8)-1;
  paquet.data_prec=0;
  paquet.longueur_data_prec=0;
  if(h->p)
    {
      /* An option field is added (SSRC) */
      do
	{	
  option = (rtp_t *)pt;
	  pt += option->generic.length;
	  if ((char *)pt -(char *)h>lr || option->generic.length == 0)
	    {
#ifdef DEBUG
	      {
		fprintf(stderr, "Bad audio RTP fields (perhaps too long) : final:%d, l:%d\n",
			option->generic.final, option->generic.length);
	      }
#endif
	      return ;
	    }
	  switch (option->generic.type)
	    {
	    case RTP_SSRC:
	      identifier = htons(option->ssrc.id);
	      break;
	    case RTP_APP:
	      /* subtype = 0 <- devries pour sa valeur precedente*/
	      /* subtype = 1 <- crepin pour la redondance */
	      if (htons(option->app_ivs_audio.subtype)==1)
		{
		  paquet.redondancy_type=
		    (int8)
		    htons(option->app_ivs_audio_redondancy.encoding_type);
		  longueur = (u_int16)
		    htons(option->app_ivs_audio_redondancy.length_data);
		  paquet.data_prec = (u_char *)malloc (4*(longueur/4+1));
		  
		  paquet.longueur_data_prec=longueur;
		  if (paquet.data_prec)
		    {
		      memcpy((char *)paquet.data_prec,(char *)
			     &(option->app_ivs_audio_redondancy.data),
			     longueur);
		    }    
		  else  
		    {
		      fprintf(stderr,
			      "Impossible to allocate memory paquet lost\n");
		    }
		  if (paquet.redondancy_type != last_redondancy) {
		      last_redondancy=paquet.redondancy_type;
		  }
		}
	      /* No option defined */
	      break;
	    default:
	      break;
	    }
	} while (!option->generic.final);
    }
  h->seq=htons(h->seq);
  h->ts_sec=htons(h->ts_sec);
  h->ts_usec=htons(h->ts_usec);
  
  seq = h->seq;
  *sin_addr=htonl((u_long)from.sin_addr.s_addr);
  *identifi=identifier; 
  DECODING_REQUIRED = ShallIDecode(*sin_addr,
				   identifier, &i_station);

  sendtime.tv_sec=(long)(0x0000ffff & (u_short)h->ts_sec);
  sendtime.tv_usec=(long)(0x00ffff0 & ((u_int)h->ts_usec << 4));
  
  gettimeofday(&realtime, (struct timezone *)NULL);
  sendtime.tv_sec=(long)((realtime.tv_sec & 0xffff0000)
			 | (0x0000ffff & sendtime.tv_sec ));
  

/* We test if we must decode and stock the audio packet */
  if (DECODING_REQUIRED && !AUDIO_DROPPED &&
      i_station!=-1 &&
      (table[i_station].p_paquet==0 ||
       table[i_station].p_paquet->sendtime <
       sendtime.tv_sec+sendtime.tv_usec/1000000.0
       ))
    {
      longueur=DecodeAudio((unsigned char *)pt,lr-((char *)pt-(char *)h),
			   h->format,outbuf);
      paquet.data = (u_char *)malloc ((u_int16) (4*((longueur / 4) + 4)));
/* if we can do the allocation we include the packet in the list */
      if (paquet.data==0)
	{
	  fprintf(stderr,"Impossible to allocate memory paquet lost\n");
	}
      else
	{
	  memcpy((char *)paquet.data, (char *)outbuf, longueur);
	  paquet.beginoftalkspurt = h->s;
	  paquet.validaudio=1;
	  paquet.reserved=0;
	  paquet.actualplay=0;
	  paquet.lastarrived=seq;
	  paquet.sendtime=sendtime.tv_sec+1/1000000.0*sendtime.tv_usec;
	  paquet.arrivaltime=realtime.tv_sec+realtime.tv_usec/1000000.0;
	  paquet.playout=0;
	  paquet.longueur=longueur;
	  table[i_station].p_paquet=(paquet_t *)
	    Insere(table[i_station].p_paquet, &paquet);
	  ORDO_REQUESTED = TRUE;
#ifdef DEBUG
	  TRACE.emetteur=paquet.sendtime;
	  TRACE.recepteur=paquet.arrivaltime;
	  TRACE.begin_of_t=paquet.beginoftalkspurt;
	  TRACE.longueur=paquet.longueur;
	  TRACE.numero=paquet.lastarrived;
	  TRACE.adress=*sin_addr;
	  fwrite((char *)(&TRACE),sizeof(struct trace),1,file_trace);
#endif	      
	}
    }
  else
    if (paquet.data_prec && paquet.longueur_data_prec)
      free(paquet.data_prec);
} 






/* This procedure take data and insert them at the good place */
/* for each station */
/* asock_r is the audio socket recieve number */
void recoit(sin_addr,identifier)
     u_long * sin_addr;
     u_short * identifier;
{
  fd_set fdset;
  static struct timeval timeout={0,0};

  FD_ZERO(&fdset);
  FD_SET(asock_r, &fdset);
  if (select(asock_r+1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &timeout)
      < 0)
    fprintf(stderr,"Pbeme select\n");
  if (FD_ISSET(asock_r, &fdset))
    {
      reception(sin_addr,identifier);
    }
}


static int ordonnance(BUFFER_AUDIO, OLDLOCALPLAY, GLOBALPLAY)
     u_char * BUFFER_AUDIO;
     int  * OLDLOCALPLAY;
     int * GLOBALPLAY;
{
  
  int i;
  int BESTYOUNG,NOSTATION,NOBODY;
  double temps;
  int ajoue,reste;
  BOOLEAN PLACE=FALSE;
  static BOOLEAN FIRST=TRUE;
  static u_char vide;
  int LOCALPLAY;
  struct timeval realtime;

  if (FIRST)
    {
      vide = l14_to_pcmu(0);
      memset((char *)BUFFER_AUDIO, (int)vide,BUFFER_AUDIO_LENGTH);
      FIRST=FALSE;
    }
  
  BESTYOUNG=-1;
  NOSTATION=-1;

  for (i=0;i<nb_stations;i++)
    {
      while (
	     table[i].p_paquet!=0 &&
	     table[i].p_paquet->suivant!=0 &&
	     ((table[i].p_paquet->suivant->lastarrived
	       ==
	       (u_int16)(table[i].p_paquet->lastarrived +1) % (u_int16)0xFFFF)
	      ||
	      (table[i].p_paquet->validaudio==0)))
	{
	  temps=algo(&(table[i].parameter),
		     table[i].p_paquet->suivant->arrivaltime,
		     table[i].p_paquet->suivant->sendtime,
		     (int)table[i].p_paquet->beginoftalkspurt);

	  if (table[i].p_paquet->beginoftalkspurt)
	    {
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      ajoue =(int) ((temps-
			     (realtime.tv_sec+realtime.tv_usec/1000000.0))*
			    8000.0);
	      if (ajoue<0)
/* if the delay is too small we keep the packet but play immediatly */
		table[i].p_paquet->suivant->actualplay=
		  *OLDLOCALPLAY % BUFFER_AUDIO_LENGTH;
	      else
		if (ajoue+(int)table[i].p_paquet->suivant->longueur>=
		    BUFFER_AUDIO_LENGTH)
		  ajoue=BUFFER_AUDIO_LENGTH-
		    table[i].p_paquet->suivant->longueur-1;
	      if (AUDIO_SET && ajoue>0)
		if (ajoue<soundplayqueue())
		  table[i].p_paquet->suivant->actualplay=*OLDLOCALPLAY %
		    BUFFER_AUDIO_LENGTH;
		else
		  table[i].p_paquet->suivant->actualplay=
		    ((ajoue-soundplayqueue())+*OLDLOCALPLAY) %
		    BUFFER_AUDIO_LENGTH;
	      else
		table[i].p_paquet->suivant->actualplay=
		  (*OLDLOCALPLAY) % BUFFER_AUDIO_LENGTH;
	      LOCALPLAY=table[i].p_paquet->suivant->actualplay;
	      PLACE=TRUE;
	    }
	  else
	    {
	      if (((table[i].p_paquet->actualplay-*OLDLOCALPLAY+
		    BUFFER_AUDIO_LENGTH)%BUFFER_AUDIO_LENGTH)+
		  table[i].p_paquet->suivant->longueur*
		  (table[i].p_paquet->suivant->lastarrived-
		   table[i].p_paquet->lastarrived)
		  < BUFFER_AUDIO_LENGTH)
		{
		  LOCALPLAY=(table[i].p_paquet->actualplay
			     +table[i].p_paquet->suivant->longueur*
			     (table[i].p_paquet->suivant->lastarrived-
			      table[i].p_paquet->lastarrived-1))%
			      BUFFER_AUDIO_LENGTH;
		  PLACE=TRUE;
		}
	      else
		{
		  if (table[i].p_paquet->validaudio==FALSE)
		    {
		      LOCALPLAY=*OLDLOCALPLAY;
		      PLACE=TRUE;
		    }
		  else
		    {
		      PLACE=FALSE;
		      break;
		    }
		}
	    }
/* if the variable PLACE is TRUE we must keep the packet an then place it */
	  if (PLACE)
	    {
/* we see if we are at the end of the buffer */
	      reste=BUFFER_AUDIO_LENGTH-
		(table[i].p_paquet->suivant->longueur+LOCALPLAY);
	      if (reste < 0)
		{
		  mix2_pcmu((size_t)(BUFFER_AUDIO_LENGTH-LOCALPLAY),
			    &(BUFFER_AUDIO[LOCALPLAY]),
			    table[i].p_paquet->suivant->data);
		  mix2_pcmu((size_t)((table[i].p_paquet->suivant->longueur+
				   LOCALPLAY)-BUFFER_AUDIO_LENGTH),
			    BUFFER_AUDIO,
			&(table[i].p_paquet->suivant->data[BUFFER_AUDIO_LENGTH
							   -LOCALPLAY]));
		}/* end of if(reste<0) */
	      else
		mix2_pcmu((size_t)table[i].p_paquet->suivant->longueur,
			  &(BUFFER_AUDIO[LOCALPLAY]),
			  table[i].p_paquet->suivant->data);
	      LOCALPLAY = (LOCALPLAY+(int)table[i].p_paquet->suivant->
			   longueur) %
		BUFFER_AUDIO_LENGTH;
	      table[i].p_paquet->suivant->actualplay=LOCALPLAY;
	      /* the next packet was played so we remove the prevent packet */
	      table[i].p_paquet=(paquet_t *)RemoveEnTete(table[i].p_paquet);
	    } /* end of if (place) */
	}/* end of while */
    }/* end of for */
  

/* we detect if station is active*/
  for (i=0;i<nb_stations;i++)
    {
      if (table[i].audio_decoding && table[i].p_paquet!=0 &&
	  table[i].p_paquet->validaudio)
	{
	  if (BESTYOUNG<0)
	    {
	      BESTYOUNG=table[i].p_paquet->actualplay;
	      NOSTATION=i;
	    }/* it is the first time */
	  else
	    if ((table[i].p_paquet->actualplay-
		 *OLDLOCALPLAY+BUFFER_AUDIO_LENGTH)%
		BUFFER_AUDIO_LENGTH <=
		(BESTYOUNG-*OLDLOCALPLAY+BUFFER_AUDIO_LENGTH)%
		BUFFER_AUDIO_LENGTH)
	      {
		BESTYOUNG=table[i].p_paquet->actualplay;
		NOSTATION=i;
	      }
	}
    }
  
  if (BESTYOUNG<0)
    *GLOBALPLAY=*OLDLOCALPLAY;
  else
    {
      *GLOBALPLAY=BESTYOUNG;
      if (table[NOSTATION].p_paquet->beginoftalkspurt)
	table[NOSTATION].p_paquet->validaudio=0;
    }

  NOBODY=1;
  for (i=0;i<nb_stations;i++)
    {
      if (table[i].p_paquet!=0 && table[i].p_paquet->validaudio)
	{
	  NOBODY=0;
	  break;
	}
    }
  /* si il n'y aplus de donnees valide alors on remplit de blanc*/
  /* if no data are alive where fill with blank the next begin of talkspurt*/
  if (NOBODY)
    {
      if (soundplayqueue()<=0)
	DEBUT=TRUE;
    }
  for (i=0;i<nb_stations;i++)
    {
      if (table[i].p_paquet!=0 && table[i].p_paquet->beginoftalkspurt)
	table[i].p_paquet->validaudio=0;
    }  
  return NOSTATION;
}/* end of procedure ordonnancement*/

static void Timeout(NOSTATION, OLDLOCALPLAY, GLOBALPLAY)
     int NOSTATION, OLDLOCALPLAY, GLOBALPLAY;
{

  int i;
  paquet_t paquet;
  u_int16 longu;
  unsigned int buffer[2000];

#ifdef HUGUES

  static FILE *fichier;
  static BOOLEAN FIRST=TRUE;

  if (FIRST)
    {
      fichier=fopen("trace_substitution","w");
      if ((fichier=fopen("trace_substitution","w"))==0)
	{
	  fprintf(stderr,"Impossible d'ouvrir le fichier de mesure \n");
	  exit(8);
	}
      FIRST=FALSE;
    }
#endif
  
  
/* a timeout is possible only if the audio is available*/  
  if(AUDIO_SET)
    {
      if ((i=soundplayqueue()) <= LONGUEUR_DEMANDE*2/5
	  && i>=0 && NOSTATION>=0 && GLOBALPLAY==OLDLOCALPLAY)
	{
	  /* we try to have the lesser number of blank */

	  for (i=0;i<nb_stations;i++)
	    {
	      if (table[i].p_paquet!=0 && table[i].p_paquet->actualplay==
		  OLDLOCALPLAY)
		{
		  if (table[i].p_paquet->suivant &&
		      ((u_int16)(table[i].p_paquet->lastarrived+2)% 0xFFFF==
		       table[i].p_paquet->suivant->lastarrived))
		    /*we insert a false packet  */
		    {
		      table[i].p_paquet->beginoftalkspurt=0;
		      paquet.validaudio=1;
		      paquet.beginoftalkspurt=0;
		      paquet.reserved=0;
		      paquet.actualplay=0;
		      paquet.longueur_data_prec=0;
		      paquet.data_prec=0;
		      paquet.redondancy_type=(int8)-1;
		      paquet.longueur=table[i].p_paquet->longueur;
		      paquet.lastarrived=(u_int16)
			(table[i].p_paquet->lastarrived+1) % 0xFFFF;
		      paquet.sendtime=table[i].p_paquet->sendtime+
			(double)1.0/8000.0;
		      paquet.arrivaltime=table[i].p_paquet->arrivaltime+
			(double)paquet.longueur/8000.0;
		      paquet.playout=0;
		      if (table[i].p_paquet->suivant->data_prec)
			{
			  longu=DecodeAudio((unsigned char *)
					    table[i].p_paquet->
					    suivant->data_prec,
					    table[i].p_paquet->
					    suivant->longueur_data_prec,
					    table[i].p_paquet->
					    suivant->redondancy_type
					    ,(unsigned char *)buffer);
			  paquet.data = (u_char *) malloc (4*((unsigned)
							   longu / 4) + 4);
			  if (paquet.data==0)
			    {
			      fprintf(stderr,
				  "Impossible to allocate memory paquet lost\n"
				      );
			    }
			  else
			    {
			      memcpy((char *)paquet.data,
				     (char *)buffer,longu);
			      paquet.longueur=longu;
			    }
			}
		      else
			{
			  paquet.data = (u_char *) malloc (4*((unsigned)
							   paquet.longueur
							      / 4) + 4);
			  if (paquet.data==0)
			    {
			      fprintf(stderr,
				      "Impossible to allocate memory paquet lost\n"
				      );
			    }
			  else
			    memcpy((char *)paquet.data,
				   (char *)table[i].p_paquet->data,
				   (int)(paquet.longueur));
			}
		      table[i].p_paquet=(paquet_t *)
			Insere(table[i].p_paquet, &paquet);
#ifdef HUGUES
		      fprintf(fichier,"substitution d'un paquet apres le paquet : %d\n",table[i].p_paquet->lastarrived);
#endif

		    }/* end of if (table[i]...) */
		  else
		    {
		      if (table[i].p_paquet->suivant &&
		      ((u_int16)(table[i].p_paquet->lastarrived+3)% 0xFFFF==
		       table[i].p_paquet->suivant->lastarrived) &&
			  table[i].p_paquet->suivant->data_prec)
			{
			  paquet_t * pointer= table[i].p_paquet->suivant;
			  table[i].p_paquet->beginoftalkspurt=0;
			  paquet.validaudio=1;
			  paquet.beginoftalkspurt=0;
			  paquet.reserved=0;
			  paquet.actualplay=0;
			  paquet.longueur_data_prec=0;
			  paquet.data_prec=0;
			  paquet.redondancy_type=(int8)-1;
			  paquet.longueur=table[i].p_paquet->longueur;
			  paquet.lastarrived=(u_int16)
			    (table[i].p_paquet->lastarrived+1) % 0xFFFF;
			  paquet.sendtime=table[i].p_paquet->sendtime+
			    (double)1.0/8000.0;
			  paquet.arrivaltime=table[i].p_paquet->arrivaltime+
			    (double)paquet.longueur/8000.0;
			  paquet.playout=0;
			  paquet.data = (u_char *) malloc (4*((unsigned)
							      paquet.longueur
							      / 4) + 4);
			  if (paquet.data==0)
			    {
			      fprintf(stderr,
				      "Impossible to allocate memory paquet lost\n"
				      );
			    }
			  else
			    memcpy((char *)paquet.data,
				   (char *)table[i].p_paquet->data,
				   (int)(paquet.longueur));
			  table[i].p_paquet=(paquet_t *)
			    Insere(table[i].p_paquet, &paquet);
			  paquet.lastarrived=(u_int16)
			    (table[i].p_paquet->lastarrived+2) % 0xFFFF;
			  paquet.sendtime=table[i].p_paquet->sendtime+
			    (double)2.0/8000.0;

			  longu=DecodeAudio((unsigned char *)
					    pointer->data_prec,
					    pointer->longueur_data_prec,
					    pointer->redondancy_type
					    ,(unsigned char *)buffer);
			  paquet.data = (u_char *) malloc (4*((unsigned)
							   longu / 4) + 4);
			  if (paquet.data==0)
			    {
			      fprintf(stderr,
				      "Impossible to allocate memory paquet lost\n"
				      );
			    }
			  else
			    {
			      memcpy((char *)paquet.data,
				     (char *)buffer,longu);
			      paquet.longueur=longu;
			    }
			  table[i].p_paquet=(paquet_t *)
			    Insere(table[i].p_paquet, &paquet);
#ifdef HUGUES
			  fprintf(fichier,"substitution de 2 paquets apres le paquet : %d\n",table[i].p_paquet->lastarrived);
#endif
			}
		      else
			{
			  table[i].p_paquet->beginoftalkspurt=1;
			}
		    }/* end of else */
		  table[i].p_paquet->validaudio=0;
		}/*end of if*/
	    }/*end of for*/
	}/* end of if soundplayqueue()<LONGUEUR_DEMANDE*2/5... */
    }/* end of if (AUDIO_SET)*/
  else
    {
      for (i=0;i<nb_stations;i++)
	{
	  if (table[i].p_paquet)
	    {
	      table[i].p_paquet->validaudio=0;
	      table[i].p_paquet->beginoftalkspurt=1;
	    }
	}
      /* we say that all packet stocked are loss*/
      DEBUT=TRUE;
    }
}/* end of procedure Timeout*/


      
/* This procedure cut a buffer in small part */
static void soundplayh(len, buf)
     int len;
     unsigned char *buf;

{

  static len_trans=160;
  int i;
  
  for (i=0;i<(len-len_trans);i+=len_trans)
      soundplay(len_trans,(unsigned char *)&(buf[i]));
  soundplay(len-i,(unsigned char *)&(buf[i]));
  
/*  soundplay(len,buf);*/

}/*end of procedure soundplayh()*/



/* This procedure play the BUFFER_AUDIO */

static void joue(BUFFER_AUDIO,oldlocalplay, globalplay)
     u_char * BUFFER_AUDIO;
     int * oldlocalplay;
     int * globalplay;
{
  int lastpos;
  int OLDLOCALPLAY=*oldlocalplay;
  int GLOBALPLAY=*globalplay;
  static BOOLEAN FIRST=TRUE;
  static unsigned char vide;
  static unsigned char BLANC[2000];

  if (FIRST)
    {
      vide = l14_to_pcmu(0);
      memset((char *)BLANC, (int)vide,2000);
      FIRST=FALSE;
    }

  if (GLOBALPLAY!=OLDLOCALPLAY)
    {
      if (DEBUT && AUDIO_SET)
	{
	  DEBUT=FALSE;
	  lastpos=LONGUEUR_DEMANDE -
		   (GLOBALPLAY-OLDLOCALPLAY+
		    BUFFER_AUDIO_LENGTH)
		   %BUFFER_AUDIO_LENGTH;
#if defined(SPARCSTATION) || defined(SGISTATION)
	  if (soundplayqueue()< lastpos)
	    {
	      soundplayh(lastpos,BLANC);
	    }
#endif	  
	}/*end of if (DEBUT) */
      lastpos=GLOBALPLAY;
      if (lastpos-OLDLOCALPLAY<0)
	{
	  if(AUDIO_SET)
	    {
	      soundplayh(BUFFER_AUDIO_LENGTH-OLDLOCALPLAY,
			 &(BUFFER_AUDIO[OLDLOCALPLAY]));
	      soundplayh(lastpos,BUFFER_AUDIO);
	    }
	  memset((char *)&(BUFFER_AUDIO[OLDLOCALPLAY]), (int)vide,
		 BUFFER_AUDIO_LENGTH-OLDLOCALPLAY);
	  memset((char *)BUFFER_AUDIO, (int)vide, lastpos);
	}
      else
	{
	  if(AUDIO_SET)
	    {
	      soundplayh(lastpos-OLDLOCALPLAY,&(BUFFER_AUDIO[OLDLOCALPLAY]));
	    }
	  memset((char *)&(BUFFER_AUDIO[OLDLOCALPLAY]), (int)vide,
		 lastpos-OLDLOCALPLAY);
	}
      if (DEBUT && AUDIO_SET)
	soundplayh(1,&vide);
      OLDLOCALPLAY=GLOBALPLAY;
    }/* fin de if (GLOBALPLAY!= .... */
  else
    {
      if (DEBUT && AUDIO_SET)
	soundplayh(1,&vide);
    }
  *oldlocalplay=OLDLOCALPLAY;
} /* end of procedure joue()*/


      



/*----------------- procedure for mix --------------------*/

/* this procedure become from Nevot */
/* and i have keep only the part what's interest me */

/********************************
***                           ***
***   p c m u _ t o _ l 1 4   ***
***                           ***
*********************************
Convert 8-bit mu-law to 14-bit signed linear (-8031 .. 8031).
*/
static short pcmu_to_l14(pcm)
register int pcm;	/* u-Law PCM value to convert to linear */
{
  register sign;        /* save sign of PCM value */
  register segment;     /* segment of piecewise linear curve (0..7) */
  register step;	/* step along segment (0..15) */
  register short lin;	/* return linear value */

  pcm = ~pcm & 0xFF;                  /* undo 1's complement of PCM */
  sign = pcm & 0x80;                  /* extract sign bit */
  pcm = pcm & 0x7F;                   /* retain only magnitude */
  segment = pcm >> 4;                 /* separate into segment */
  step = pcm & 0xF;                   /*  and step within segment */
  lin = ((2*step+33)<<segment)-33;    /* calculate linear value by formula */
  if (sign) lin = -lin;               /* apply the sign */
  return lin;
} /* pcmu_to_l14 */


/********************************
***                           ***
***   l 1 4 _ t o _ p c m u   ***
***                           ***
*********************************
Convert 14-bit linear code (+/- 8158) to 8-bit mu-law.
*/
static u_char l14_to_pcmu(lin)
register short lin;		/* Linear value to convert to u-Law PCM */
{
    register decision;			/* Threshold for segment, step */
    register segment;			/* Segment of piecewise linear curve */
    register step;			/* Step along segment */
    register sign = 0;			/* Assume positive sign */

    if (lin < 0) {			/* If negative linear value, */
	lin = -lin;			/*  make positive and */
	sign = 0x80;			/*  get sign to apply later */
    }
    if (lin > 8158) lin = 8158;		/* Limit to maximum linear value */
    lin += 33;				/* Shift up against 8192 */
    for (segment=7, decision=1<<(7+5); segment>=0; segment--, decision>>=1) 
	if (decision < lin) break;	/* Find first seg the value is above */
    lin -= decision;			/* Get offset from segment base */
    step = lin / (decision>>4);		/* Calculate step along segment */
    return(~(sign + (segment<<4) + step) & 0xFF);   /* Assemble PCM value */
} /* l14_to_pcmu */


static u_char sum_table[256*256];		/* table of sums */

/**************************
***                     ***
***   m i x _ i n i t   ***
***                     ***
***************************
Initialize mixing table.
*/
static void mix_init()
{
  register pcm0,pcm1;			/* Input PCM values */

  /* Fill in the sum table with the sum for each possible combination
     of PCM inputs. Can't use macro since values of > 8192 are
     possible. */
      
  for (pcm0 = 0; pcm0 < 256; pcm0++) {
    for (pcm1 = 0; pcm1 < 256; pcm1++) {
      sum_table[pcm0*256 + pcm1] = 
        l14_to_pcmu(pcmu_to_l14(pcm0) + pcmu_to_l14(pcm1));
    }
  }
}

/*****************************
***                        ***
***   m i x 2 _ p c m u    ***
***                        ***
******************************
Mix two PCM u-law streams.
*/
static void mix2_pcmu(len,v0,v1)
size_t len;   /* number of bytes */
u_char *v0;   /* destination */ 
u_char *v1;   /* stream to be mixed in */
{
  register i;                             /* Loop index */
   
  for (i = len; i > 0; i--, v0++)
    *v0 = sum_table[(((*v0) << 8) | (*v1++))];
} /* mix2_pcmu */




/*--------------------------- IVS LISTE  UTILITIES -------------------------*/

/* This procedure Insere inserte a packet in a ordered linked liste */
/* it classify the paquet with the sendtime */
static paquet_t * Insere(ptr_tete, a_inserer)
     paquet_t * ptr_tete;
     paquet_t * a_inserer;
{
  paquet_t * ptr, * ptr_aux;
  ptr=ptr_tete;
  /*if it is the List is blank the head pointer is null */
  if (ptr==0)
    {
      ptr=(paquet_t *)malloc((sizeof(paquet_t)/4+1)*4);
      if (ptr==0)
	{
	  fprintf(stderr,"Impossible to allocate memory\n");
	  return ptr;
	}

      ptr->validaudio=a_inserer->validaudio;
      ptr->beginoftalkspurt=a_inserer->beginoftalkspurt;
      ptr->reserved=a_inserer->reserved;
      ptr->redondancy_type=(int8)a_inserer->redondancy_type;
      ptr->actualplay=a_inserer->actualplay;
      ptr->lastarrived=a_inserer->lastarrived;
      ptr->sendtime=a_inserer->sendtime;
      ptr->arrivaltime=a_inserer->arrivaltime;
      ptr->playout=a_inserer->playout;
      ptr->longueur=a_inserer->longueur;
      ptr->longueur_data_prec=a_inserer->longueur_data_prec;
      ptr->data=a_inserer->data;
      ptr->data_prec=a_inserer->data_prec;
      ptr->suivant=0;
      return ptr;
    }
  /* if it is the same paquet we kill it */
  if (ptr->sendtime==a_inserer->sendtime)
    {
      free(a_inserer->data);
      return ptr_tete;
    }
  /* if the packet is the first of the list */
  if ((ptr->sendtime > a_inserer->sendtime))
    {
      ptr_aux=(paquet_t *)malloc((sizeof(paquet_t)/4+1)*4);
      if (ptr_aux==0)
	{
	  fprintf(stderr,"Impossible to allocate memory\n");
	  return ptr;
	}      
      ptr_aux->validaudio=a_inserer->validaudio;
      ptr_aux->beginoftalkspurt=a_inserer->beginoftalkspurt;
      ptr_aux->reserved=a_inserer->reserved;
      ptr_aux->redondancy_type=(int8)a_inserer->redondancy_type;
      ptr_aux->actualplay=a_inserer->actualplay;
      ptr_aux->lastarrived=a_inserer->lastarrived;
      ptr_aux->sendtime=a_inserer->sendtime;
      ptr_aux->arrivaltime=a_inserer->arrivaltime;
      ptr_aux->playout=a_inserer->playout;
      ptr_aux->longueur=a_inserer->longueur;
      ptr_aux->longueur_data_prec=a_inserer->longueur_data_prec;
      ptr_aux->data=a_inserer->data;
      ptr_aux->data_prec=a_inserer->data_prec;
      ptr_aux->suivant=ptr;
      return ptr_aux;
    }
  /* else we're looking for is place */
  while (ptr->suivant!=0)
    {
      if (ptr->suivant->sendtime==a_inserer->sendtime)
	{
	  free(a_inserer->data);
	  return ptr_tete;
	}
      if ((ptr->suivant->sendtime > a_inserer->sendtime))
	{
	  ptr_aux=(paquet_t *)malloc((sizeof(paquet_t)/4+1)*4);
	  if (ptr_aux==0)
	    {
	      fprintf(stderr,"Impossible to allocate memory\n");
	      return ptr_tete;
	    }
	  ptr_aux->validaudio=a_inserer->validaudio;
	  ptr_aux->beginoftalkspurt=a_inserer->beginoftalkspurt;
	  ptr_aux->reserved=a_inserer->reserved;
	  ptr_aux->redondancy_type=(int8)a_inserer->redondancy_type;
	  ptr_aux->actualplay=a_inserer->actualplay;
	  ptr_aux->lastarrived=a_inserer->lastarrived;
	  ptr_aux->sendtime=a_inserer->sendtime;
	  ptr_aux->arrivaltime=a_inserer->arrivaltime;
	  ptr_aux->playout=a_inserer->playout;
	  ptr_aux->longueur=a_inserer->longueur;
	  ptr_aux->longueur_data_prec=a_inserer->longueur_data_prec;
	  ptr_aux->data=a_inserer->data;
	  ptr_aux->data_prec=a_inserer->data_prec;
	  ptr_aux->suivant=ptr->suivant;
	  ptr->suivant=ptr_aux;
	  return ptr_tete;
	}
      ptr=ptr->suivant;
    }
/* The packet is the last */
  ptr_aux=(paquet_t *)malloc((sizeof(paquet_t)/4+1)*4);
  if (ptr_aux==0)
    {
      fprintf(stderr,"Impossible to allocate memory\n");
      return ptr_tete;
    }
  ptr_aux->validaudio=a_inserer->validaudio;
  ptr_aux->beginoftalkspurt=a_inserer->beginoftalkspurt;
  ptr_aux->reserved=a_inserer->reserved;
  ptr_aux->redondancy_type=(int8)a_inserer->redondancy_type;
  ptr_aux->actualplay=a_inserer->actualplay;
  ptr_aux->lastarrived=a_inserer->lastarrived;
  ptr_aux->sendtime=a_inserer->sendtime;
  ptr_aux->arrivaltime=a_inserer->arrivaltime;
  ptr_aux->playout=a_inserer->playout;
  ptr_aux->longueur=a_inserer->longueur;
  ptr_aux->longueur_data_prec=a_inserer->longueur_data_prec;
  ptr_aux->data=a_inserer->data;
  ptr_aux->data_prec=a_inserer->data_prec;
  ptr_aux->suivant=0;
  ptr->suivant=ptr_aux;
  return ptr_tete;
}      

/* This function remove the first packet of the list */
/* it return the new Head pointer */
static paquet_t * RemoveEnTete(ptr_tete)
     paquet_t * ptr_tete;
{
  return (paquet_t *)Remove(ptr_tete,ptr_tete);
}/*end of RemoveEnTete*/


/* this function remove the packet pointed by ptr_paquet in the list */
/* where ptr_tete is the head pointer */
/* it return the new Head pointer */
static paquet_t * Remove(ptr_tete,ptr_paquet)
     paquet_t * ptr_tete;
     paquet_t * ptr_paquet;
{
  paquet_t * ptr;
  ptr=ptr_tete;

  /* if the list is empty we remove nothing */
  if (ptr==0)
    return 0;
  /* if it is the head */
  if (ptr==ptr_paquet)
    {
      ptr=ptr_tete->suivant;
      free(ptr_tete->data);
      free(ptr_tete->data_prec);
      free(ptr_tete);
      return ptr;
    }
  /* we research the packet and destroy him */
  while (ptr->suivant!=0)
    {
      if (ptr->suivant==ptr_paquet)
	{
	  ptr->suivant=ptr_paquet->suivant;
	  free(ptr_paquet->data);
	  free(ptr_paquet->data_prec);
	  free(ptr_paquet);
	  return ptr_tete;
	}
      ptr=ptr->suivant;
    }
  return ptr_tete;
}


/* this function initialize the list*/
static paquet_t * InitList()
{
  return (paquet_t *)0;
}


/* This function destroy all the packet in the list */
static paquet_t * PurgeList(ptr_tete)
     paquet_t * ptr_tete;
{
  paquet_t * aux=ptr_tete;

  while ((aux=RemoveEnTete(aux))!=0);

  return 0; 
}/*end of function PurgeList*/



#else
BOOLEAN IsAudioEncoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  return FALSE;
}


BOOLEAN IsAudioDecoding(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;     /* source identifier, set to 0 if no bridge */
			
{
  return FALSE;
}


BOOLEAN IsAudioIgnored(address,ident)
     u_long  address;         /* packet has been received from this address  */
     u_short ident;              /* source identifier, set to 0 if no bridge */
{
  return TRUE;
}


#endif /* ! NO_AUDIO */
