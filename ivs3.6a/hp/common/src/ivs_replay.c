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
*  File              : ivs_replay.c    	                		   *
*  Last modification : 1995/7/13   					   *
*  Author            : Thierry Turletti					   *
*--------------------------------------------------------------------------*
*  Description : Play an ivs encoded audio/video clip or replay the ivs    *
*                session recorded.                                         *
*                                                                          *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Dialog.h>

#include "general_types.h"
#include "protocol.h"
#include "rtp.h"
#include "h261.h"
#include "quantizer.h"

#ifdef SGISTATION
#define DefaultRecord      80
#else
#ifdef SPARCSTATION
#define DefaultRecord      20
#else
#define DefaultRecord      40
#endif /* SPARCSTATION */
#endif /* SGISTATION */

#define DefaultVolume      50
#define MaxVolume         100
#define MaxRecord         100

#define LabelPause      "Pause"
#define LabelCont       "Cont."

#define DefaultTTL         16

#define streq(a, b)        ( strcmp((a), (b)) == 0 )

#define headphone_width 32
#define headphone_height 32
static u_char headphone_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x08, 0x08, 0x00,
   0x00, 0x04, 0x10, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00, 0x02, 0x20, 0x00,
   0x00, 0x01, 0x40, 0x00, 0x80, 0x01, 0xc0, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x0c, 0x98, 0x00,
   0x80, 0x1c, 0x9c, 0x00, 0xc0, 0x1f, 0xfc, 0x01, 0xc0, 0x1f, 0xfc, 0x01,
   0xc0, 0x1f, 0xfc, 0x01, 0xc0, 0x1f, 0xfc, 0x01, 0x00, 0x1c, 0x1c, 0x00,
   0x00, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define speaker_width 32
#define speaker_height 32
static u_char speaker_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x00, 0x00, 0x60, 0x40, 0x00,
   0x00, 0x70, 0x80, 0x00, 0x00, 0x78, 0x08, 0x01, 0x00, 0x7c, 0x10, 0x02,
   0x00, 0x7e, 0x20, 0x02, 0xf0, 0x7f, 0x22, 0x04, 0xf0, 0x7f, 0x44, 0x04,
   0xf0, 0x7f, 0x44, 0x08, 0xf0, 0x7f, 0x88, 0x08, 0xf0, 0x7f, 0x48, 0x08,
   0xf0, 0x7f, 0x44, 0x04, 0xf0, 0x7f, 0x24, 0x04, 0xf0, 0x7f, 0x22, 0x02,
   0x00, 0x7e, 0x10, 0x02, 0x00, 0x7c, 0x08, 0x01, 0x00, 0x78, 0x80, 0x00,
   0x00, 0x70, 0x40, 0x00, 0x00, 0x60, 0x20, 0x00, 0x00, 0x40, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
     
static String fallback_resources[] = {
  "*font: -adobe-courier-bold-r-normal--12-120-75-75-m-70-iso8859-1",
  "*Label.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameBrightness.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameContrast.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameScale.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*ValueScale.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*Form.background:                  Grey",
  "*Command.background:               DarkSlateGrey",
  "*Command.foreground:               MistyRose1",
  "*Label.background:                 Grey",
  "*ButtonQuit.background:            Red",
  "*ButtonQuit.foreground:            White",
  "*ScrollScale.background:           LightSteelBlue4",
  "*ScrollScale.foreground:           White",
  "*MsgError.background:              Red",
  "*NameBrightness.label:             Brightness:",
  "*NameContrast.label:               Contrast:",
  "*ButtonDefault.label:              Default",
  "*ButtonPlay.label:                 Push to play",
  "*ButtonPause.label:                Pause",
  "*ButtonQuit.label:                 Quit",
  "*NameVolAudio.label:               Volume Gain:",
  "*ScrollScale.orientation:          horizontal",
  "*Label.borderWidth:                0",
  "*Form.borderWidth:                 0",
  "*MainForm.borderWidth:             0",
  "*FormScale.borderWidth:            0",
  "*NameScale.borderWidth:            0",
  "*ValueScale.borderWidth:           0",
  "*FormList.borderWidth:             1",
  "*ScrollScale.borderWidth:          4",
  "*ScrollScale.width:                85",
  "*ScrollScale.height:               10",
  "*ScrollScale.minimumThumb:         4",
  "*ScrollScale.thickness:            4",
  NULL,
};


/* Exported  Widgets */
Widget main_form;
Widget msg_error;
BOOLEAN UNICAST=FALSE;
/* Global not used variables (for athena module) */
S_PARAM param;


/* Global Widgets */
static XtAppContext app_con;
static Widget toplevel, form;
static Widget form_v_audio, scroll_v_audio, name_v_audio, value_v_audio;
static Widget output_audio, button_play, button_pause, button_quit;

static Pixmap LabelSpeakerOn, LabelSpeakerOff;
static struct sockaddr_in addr_video, addr_audio, addr_control;
static int sock_video, sock_audio, sock_control;
static int len_addr_video, len_addr_audio, len_addr_control;
static int dv_fd[2], fd_s2f[2], fd_f2s[2];
static FILE *F_in;
static BOOLEAN DEBUG_MODE=FALSE;
static BOOLEAN SPEAKER;
static BOOLEAN QCIF2CIF=FALSE;
static BOOLEAN AUDIO_DECODING=FALSE;
static BOOLEAN VIDEO_DECODING=FALSE;
static BOOLEAN AUDIO_INIT_FAILED=FALSE;
static BOOLEAN LOCAL_REPLAY=TRUE;
static BOOLEAN NEW_PORTS=FALSE;
static BOOLEAN AUTO_SEND=FALSE;
static BOOLEAN PAUSE=FALSE;
static struct sockaddr_in my_sin_addr;
static char IP_GROUP[256], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
static char IP_addr[16];
static char display[50], fullname[256], filename[256];
static u_short port_source;
static XtInputId id_reinit;
static int null=0;

#ifdef HPUX
char audioserver[50]=NULL;
#endif

static XtCallbackProc CallbackLocalPlay();
static XtCallbackProc CallbackRemotePlay();


/***************************************************************************
 * Procedures
 **************************************************************************/


#ifndef NO_AUDIO

void PlayAudio(inbuf, soundel, format)
     unsigned char *inbuf;
     int soundel;
     u_char format;
{
    u_char *val;
    u_char outbuf[P_MAX];
    int len;

    switch(format){
    case RTP_PCM_CONTENT:
      len=soundel;
      val=inbuf;
      break;
    case RTP_ADPCM_4:
      len = adpcm4_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_VADPCM_CONTENT:
      len=vadpcm_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_ADPCM_6:
      len=adpcm_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_ADPCM_5:
      len=adpcm_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_ADPCM_3:
      len=adpcm3_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_ADPCM_2:
      len=adpcm2_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_LPC:
      len=decoder_lpc(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    case RTP_GSM_13:
      len=gsm_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    default:
      return;
    }
    soundplay(len, val);
    return;
  }
#endif /* NO_AUDIO */



static void Usage(name)
     char *name;
{
  fprintf(stderr,
	  "Usage: %s [-autostart] [addr/port] [-t ttl] -clip <filename>\n",
	  name);
  exit(1);
}



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




static void Start(null)
     int null;
{
  Widget w;
  XtPointer closure, call_data;
  
  if(LOCAL_REPLAY){
    CallbackLocalPlay(w, closure, call_data);
  }else{
    CallbackRemotePlay(w, closure, call_data);
  }
  return;
}



static void SetVolumeGain(value)
     int value;
{
#ifndef NO_AUDIO  
  soundplayvol(value);
#endif
  return;
}



static BOOLEAN SetAudio()
{
  BOOLEAN SET=FALSE;
  static BOOLEAN FIRST=TRUE;

#ifndef NO_AUDIO
#if defined(SPARCSTATION) || defined(SGISTATION)
  if(!soundinit(O_WRONLY)){
    if(DEBUG_MODE){
      fprintf(stderr,"audio_decoder: cannot open audio driver\n");
    }
  }else{
    SET = TRUE;
  }
#endif /* SPARCSTATION OR SGISTATION */
#ifdef AF_AUDIO
  if(!audioInit()){
    if(DEBUG_MODE){
      fprintf(stderr,"audio_decoder: cannot connect to AF AudioServer\n");
    }
  }else{
    SET = TRUE;
    sounddest(A_SPEAKER);
  }
#endif /* AF_AUDIO */
  if(SET && !FIRST){
#if defined(SPARCSTATION) || defined(AF_AUDIO)
    if(SPEAKER)
      sounddest(A_SPEAKER);
    else
      sounddest(A_HEADPHONE);
#endif
  }
  FIRST = FALSE;
  if(SET && DEBUG_MODE){
    fprintf(stderr, "Open the audio device\n");
  }
#endif /* NO_AUDIO */
  return(SET);
}



static void UnSetAudio()
{
#ifndef NO_AUDIO  
#if defined(SPARCSTATION) || defined(SGISTATION)
  if(DEBUG_MODE){
    fprintf(stderr, "Close the audio device\n");
  }
  soundterm();
#endif /* SPARCSTATION OR SGISTATION */
#endif /* NO_AUDIO */
  return;
}




static void Quit(null)
     int null;
{
  if(AUDIO_DECODING)
    UnSetAudio();
  if(VIDEO_DECODING)
    SendPIP_QUIT(dv_fd);
  SendPIP_QUIT(fd_f2s);
#ifdef HPUX 
  audioTerm();
#endif
  XtDestroyApplicationContext(app_con);
  exit(0);
}


static void Wait(delay)
     int delay; /* in ms */
{
  struct timeval timer;
  static int nb=0;

  if(nb < 15){
    delay += 100;
  }else{
    delay -= 7;
    if(delay < 0)
      delay = 0;
  }
  timer.tv_sec = delay / 1000;
  timer.tv_usec = (delay % 1000) * 1000;
  (void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timer);
  nb ++;
  return;
}


/**************************************************
 * Callbacks
 **************************************************/


/*------------------------------------------------*\
*                  Audio  callbacks                *
\*------------------------------------------------*/

static XtCallbackProc CallbackVol(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int val;
  char value[4];
  Arg args[1];
#ifndef NO_AUDIO  
  val = (int)(MaxVolume*top);
  sprintf(value, "%d", val);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  SetVolumeGain(val);
#endif
  return;
}

#ifndef NO_AUDIO
#if defined(SPARCSTATION)  || defined(HPUX) || defined(AF_AUDIO)
static XtCallbackProc CallbackAudioPort(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{
  if(SPEAKER==TRUE){
    SPEAKER=FALSE;
    ChangePixmap(output_audio, LabelSpeakerOff);
    sounddest(A_HEADPHONE);
    return;
  }else{
    SPEAKER=TRUE; 	
    ChangePixmap(output_audio, LabelSpeakerOn); 
    sounddest(A_SPEAKER);
    return;
  }
}
#endif /* SPARCSTATION OR HP OR AF_AUDIO */
#endif /* NO_AUDIO */


/*------------------------------------------------*\
*                 Others  callbacks                *
\*------------------------------------------------*/

static XtCallbackProc ReInit(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int statusp;

  if(AUTO_SEND){
    Quit(null);
  }else{
    XtRemoveInput(id_reinit);
    VIDEO_DECODING = FALSE;
    SendPIP_QUIT(dv_fd);
    close(fd_f2s[1]);
    close(fd_s2f[0]);
    waitpid(0, &statusp, WNOHANG);
    XtSetMappedWhenManaged(button_play, TRUE);
    XtSetMappedWhenManaged(button_pause, FALSE);
    return;
  }
}


static XtCallbackProc CallbackLocalPlay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
  {

    struct timeval timer;
    int id, len_data, delay, lread;
    int lpipe, type_packet, next_option, mask, mask0;
    u_int data[P_MAX/4];
    char command;
    BOOLEAN STAT_MODE=FALSE;
    rtp_hdr_t *h; /* fixed RTP header */
    rtp_t *r; /* RTP options */
    ivs_record_hdr *head; /* fixed IVS RECORD header */

#ifndef NO_AUDIO    
    if(!AUDIO_INIT_FAILED && !AUDIO_DECODING){
      AUDIO_DECODING = SetAudio();
    }
#endif /* AUDIO */    
    if(!VIDEO_DECODING){
      /* fork the video decoding process */
      
      if(pipe(dv_fd) < 0){
	fprintf(stderr,
		"Cannot create pipe towards video decoding process\n");
      }else{
	if((id=fork()) == -1){
	  fprintf(stderr,
		  "Too many processes, Cannot fork decoding video process\n");
	}else{
	  if(!id){
	    close(dv_fd[1]);
	    VideoDecode(dv_fd, VIDEO_REPLAY_PORT, IP_addr, fullname, 
			my_sin_addr.sin_addr.s_addr, 0, 0, (char *)&null, 0,
			display, QCIF2CIF, STAT_MODE, FALSE, UNICAST);
	  }
	  VIDEO_DECODING=TRUE;
	  close(dv_fd[0]);
	}
      }
    }
    
    if(AUDIO_DECODING || VIDEO_DECODING){
      if(pipe(fd_f2s) < 0){
	fprintf(stderr, "Cannot create pipe\n");
	Quit(null);
      }
      
      if(pipe(fd_s2f) < 0){
	fprintf(stderr, "Cannot create pipe\n");
	Quit(null);
      }
      id_reinit = XtAppAddInput(app_con, fd_s2f[0],
				(XtPointer)XtInputReadMask, 
				(XtInputCallbackProc)ReInit, NULL);
      if((id=fork()) == -1){
	fprintf(stderr,
		"Too many processes, Cannot fork decoding ivs process\n");
	Quit(null);
      }else{
	if(!id){
	  close(fd_f2s[1]);
	  close(fd_s2f[0]);
	  mask0 = 1 << fd_f2s[0];
	  timer.tv_sec = timer.tv_usec = 0;

	  /* Open the input ivs_file */
	  if((F_in=fopen(filename, "r")) == NULL){
	    fprintf(stderr, "Cannot open input file %s\n", filename);
	    Quit(null);
	  }
	  if((lread=fread((char *)data, 1, 14, F_in)) != 14){
	    fprintf(stderr, "Cannot read clip version\n");
	    Quit(null);
	  }
	  if(!streq((char *)data, IVS_CLIP_VERSION)){
	    fprintf(stderr, "Bad clip version: %s\n", (char *)data);
	    Quit(null);
	  }

	  sleep((unsigned int)3); /* Waiting for son process running */

	  do{
	    /* Get next header packet */
	    if((lread=fread((char *)data, 1, 4, F_in)) != 4){
	      break;
	    }
	    head = (ivs_record_hdr *)data;
	    /* its length */
	    len_data = ntohs(head->len);
	    /* delay before sending it */
	    delay = ntohs(head->delay);
	    /* read the packet */

	    if((lread=fread((char *)data, 1, len_data, F_in)) != len_data){
	      fprintf(stderr, "clip is not complete");
	      break;
	    }

	    h = (rtp_hdr_t *)data;
	    if(h->ver != RTP_VERSION){
	      if(DEBUG_MODE){
		fprintf(stderr, "Bad RTP version %d\n", h->ver);
	      }
	      continue;
	    }
	    switch(h->format){

	    case RTP_H261_CONTENT:
	      type_packet = VIDEO_TYPE;
#ifdef DEBUG	      
	      fprintf(stderr, "delay: %d, VIDEO, #%d\n", delay, h->seq);
#endif	      
	      break;
	    case RTP_PCM_CONTENT:
	    case RTP_ADPCM_4:
	    case RTP_VADPCM_CONTENT:
	    case RTP_ADPCM_6:
	    case RTP_ADPCM_5:
	    case RTP_ADPCM_3:
	    case RTP_ADPCM_2:
	    case RTP_LPC:
	    case RTP_GSM_13:
#ifdef DEBUG	      
	      fprintf(stderr, "delay: %d, AUDIO, #%d\n", delay, h->seq);
#endif
	      next_option = 2;
	      if(h->p){
		r = (rtp_t *) &data[next_option];
		while(!r->generic.final){
		  next_option += r->generic.final;
		  r = (rtp_t *) &data[next_option];
		}
	      }
	      type_packet = AUDIO_TYPE;
	      break;
	    default:
	      fprintf(stderr,
		 "ivs_replay: Use the addr/port option to replay this clip\n");
	      SendPIP_QUIT(fd_s2f);
	      close(fd_s2f[1]);
	      fclose(F_in);
	      exit(1);
	    }
	    mask = mask0;
	    if(select(fd_f2s[0]+1, (fd_set *)&mask, (fd_set *)0,
			 (fd_set *)0, &timer) < 0){
	      if(errno !=  EINTR){
		perror("ivs-replay-select");
	      }
	    }
	    if(mask & mask0){
	      if((lpipe=read(fd_f2s[0], (char *)&command, 1)) < 0){
		SendPIP_QUIT(fd_s2f);
		close(fd_s2f[1]);
		fclose(F_in);
		exit(1);
	      }else{
		switch(command){
		case PIP_PAUSE:
		  mask = mask0;
		  if((lpipe=read(fd_f2s[0], (char *)&command, 1)) < 0){
		    SendPIP_QUIT(fd_s2f);
		    close(fd_s2f[1]);
		    fclose(F_in);
		    exit(1);
		  }else{
		    switch(command){
		    case PIP_CONT:
		      break;
		    default:
		      SendPIP_QUIT(fd_s2f);
		      close(fd_s2f[1]);
		      fclose(F_in);
		      exit(1);
		    }
		  }
		  break;
		case PIP_QUIT:	    
		default:
		  SendPIP_QUIT(fd_s2f);
		  close(fd_s2f[1]);
		  fclose(F_in);
		  exit(0);
		}
	      }
	    }
	    Wait(delay);
#ifndef NO_AUDIO
	    if(AUDIO_DECODING && (type_packet == AUDIO_TYPE)){
	      PlayAudio(&data[next_option], len_data-(next_option*4),
			h->format);
	    }
#endif	    
	    if(VIDEO_DECODING && (type_packet == VIDEO_TYPE)){
	      if(sendto(sock_video, (char *)data, len_data, 0, 
			(struct sockaddr *) &addr_video, len_addr_video)
		 < 0){
		fprintf(stderr, "sendto video packet failed");
		Quit(null);
	      }
	    }
	  }while(TRUE);
	  SendPIP_QUIT(fd_s2f);
	  close(fd_s2f[1]);
	  fclose(F_in);
	  exit(0);
	}else{
	  close(fd_f2s[0]);
	  close(fd_s2f[1]);
	  XtSetMappedWhenManaged(button_play, FALSE);
	  XtSetMappedWhenManaged(button_pause, TRUE);
	}
      }
    }
    return;
  }


static XtCallbackProc CallbackRemotePlay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
  {

    struct timeval timer;
    int lread, lw, mask, mask0;
    int lpipe, id, len_data, delay;
    u_int data[P_MAX/4];
    char command;
    rtp_hdr_t *h; /* fixed RTP header */
    ivs_record_hdr *head; /*fixed IVS RECORD header */

    if(pipe(fd_f2s) < 0){
      fprintf(stderr, "Cannot create pipe\n");
      Quit(null);
    }
      
    if(pipe(fd_s2f) < 0){
      fprintf(stderr, "Cannot create pipe\n");
      Quit(null);
    }
    id_reinit = XtAppAddInput(app_con, fd_s2f[0], (XtPointer)XtInputReadMask, 
			      (XtInputCallbackProc)ReInit, NULL);
    if((id=fork()) == -1){
      fprintf(stderr,
	      "Too many processes, Cannot fork\n");
      Quit(null);
    }else{
      if(!id){
	close(fd_f2s[1]);
	close(fd_s2f[0]);
	mask0 = 1 << fd_f2s[0];
	timer.tv_sec = timer.tv_usec = 0;

	/* Open the input ivs_file */
	if((F_in=fopen(filename, "r")) == NULL){
	  fprintf(stderr, "Cannot open input file %s\n", filename);
	  Quit(null);
	}
	if((lread=fread((char *)data, 1, 14, F_in)) != 14){
	  fprintf(stderr, "Cannot read clip version\n");
	  Quit(null);
	}
	if(!streq((char *)data, IVS_CLIP_VERSION)){
	  fprintf(stderr, "Bad clip version: %s\n", (char *)data);
	  Quit(null);
	}
	sleep((unsigned)3); /* Waiting for son process running */
	do{
	    /* Get next header packet */
	    if((lread=fread((char *)data, 1, 4, F_in)) != 4){
	      break;
	    }
	    head = (ivs_record_hdr *)data;
	    /* its length */
	    len_data = ntohs(head->len);
	    /* delay before sending it */
	    delay = ntohs(head->delay);

	  /* read the packet */

	  if((lread=fread((char *)data, 1, len_data, F_in)) != len_data){
	    ShowWarning("clip is not complete");
	    break;
	  }
	  h = (rtp_hdr_t *)data;
	  if(h->ver != RTP_VERSION){
	    if(DEBUG_MODE){
	      fprintf(stderr, "Bad RTP version %d\n", h->ver);
	    }
	    continue;
	  }
	  Wait(delay);
	  switch(h->format){
	  case RTP_CONTROL_CONTENT:
	    h->format = RTP_H261_CONTENT; /* Reset the correct format */
	    if((lw=sendto(sock_control, (char *)data, len_data, 0, 
		      (struct sockaddr *)&addr_control, len_addr_control))
	       != len_data){
	      perror("sendto control");
	      continue;
	    }
	    break;
	  case RTP_H261_CONTENT:
	    if((lw=sendto(sock_video, (char *)data, len_data, 0, 
		      (struct sockaddr *)&addr_video, len_addr_video))
	       != len_data){
	      perror("sendto video");
	      continue;
	    }
	    break;
	  case RTP_PCM_CONTENT:
	  case RTP_ADPCM_4:
	  case RTP_VADPCM_CONTENT:
	  case RTP_ADPCM_6:
	  case RTP_ADPCM_5:
	  case RTP_ADPCM_3:
	  case RTP_ADPCM_2:
	  case RTP_LPC:
	  case RTP_GSM_13:
	    if((lw=sendto(sock_audio, (char *)data, len_data, 0, 
		      (struct sockaddr *)&addr_audio, len_addr_audio))
	       != len_data){
	      perror("sendto audio");
	      continue;
	    }
	    break;
	  default:
	    if(DEBUG_MODE){
	      fprintf(stderr, "Bad RTP content type %d\n", h->format);
	    }
	    continue;
	  }
          mask = mask0;
	  if(select(fd_f2s[0]+1, (fd_set *)&mask, (fd_set *)0,
		    (fd_set *)0, &timer) < 0){
	    if(errno !=  EINTR)
	      perror("ivs-replay-select");
	  }
	  if(mask & mask0){
	    if((lpipe=read(fd_f2s[0], (char *)&command, 1)) < 0){
	      SendPIP_QUIT(fd_s2f);
	      close(fd_s2f[1]);
	      fclose(F_in);
	      exit(1);
	    }else{
	      switch(command){
	      case PIP_PAUSE:
		mask = mask0;
		if((lpipe=read(fd_f2s[0], (char *)&command, 1)) < 0){
		  SendPIP_QUIT(fd_s2f);
		  close(fd_s2f[1]);
		  fclose(F_in);
		  exit(1);
		}else{
		  switch(command){
		  case PIP_CONT:
		    break;
		  default:
		    SendPIP_QUIT(fd_s2f);
		    close(fd_s2f[1]);
		    fclose(F_in);
		    exit(1);
		  }
		}
		break;
	      case PIP_QUIT:	    
	      default:
		SendPIP_QUIT(fd_s2f);
		close(fd_s2f[1]);
		fclose(F_in);
		exit(0);
	      }
	    }
	  }
	}while(TRUE);
	SendPIP_QUIT(fd_s2f);
	close(fd_s2f[1]);
	fclose(F_in);
	exit(0);
      }else{/* father process */
	close(fd_f2s[0]);
	close(fd_s2f[1]);
	XtSetMappedWhenManaged(button_play, FALSE);
	XtSetMappedWhenManaged(button_pause, TRUE);
      }
    }/* fork success */
    return;
  }


static XtCallbackProc CallbackPause(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  if(PAUSE){
    SendPIP_CONT(fd_f2s);
    ChangeLabel(button_pause, LabelPause); 
    PAUSE = FALSE;
  }else{
    SendPIP_PAUSE(fd_f2s);
    ChangeLabel(button_pause, LabelCont); 
    PAUSE = TRUE;
  }
  return;
}



static XtCallbackProc CallbackQuit(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  Quit(null);
}







/**************************************************
 *  Main program
 **************************************************/

main(argc, argv)
     int argc;
     char **argv;
{
  Arg args[10];
  int n, narg;
  char *ptr;
  char title[256];
  u_char ttl=DefaultTTL;
  char null=0;

  if((argc < 3) || (argc > 7))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(strcmp(argv[narg], "-clip") == 0){
      if(++narg == argc)
	Usage(argv[0]);
      strcpy(filename, argv[narg]);
      narg ++;
      continue;
    }
    if(strcmp(argv[narg], "-autostart") == 0){
      AUTO_SEND = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-t") || streq(argv[narg], "-T")){
      if(++narg == argc)
	Usage(argv[0]);
      ttl = atoi(argv[narg++]);
      continue;
    }
    if(argv[narg][0] == '-'){
      Usage(argv[0]);
    }
    LOCAL_REPLAY = FALSE;
    if((ptr=strstr(argv[narg], "/")) == NULL){
      strcpy(IP_GROUP, argv[narg]);
      narg ++;
    }else{
      *ptr++ = (char) 0;
      strcpy(IP_GROUP, argv[narg]);
      narg ++;
      strcpy(VIDEO_PORT, ptr);
      n = atoi(VIDEO_PORT);
      sprintf(AUDIO_PORT, "%d", ++n);
      sprintf(CONTROL_PORT, "%d", ++n);
      NEW_PORTS = TRUE;
    }
  }
  {
    /* Check if the file exists and the version */
    FILE *F_in;
    char data[14];
    int lread;
    
    if((F_in=fopen(filename, "r")) == NULL){
      fprintf(stderr, "Cannot open input file %s\n", filename);
      exit(1);
    }
    if((lread=fread((char *)data, 1, 14, F_in)) != 14){
      fprintf(stderr, "Cannot read clip version\n");
      exit(1);
    }
    if(!streq((char *)data, IVS_CLIP_VERSION)){
      fprintf(stderr, "Bad clip version: %s\n", data);
      exit(1);
    }
    fclose(F_in);
  }
  {
    unsigned int i1, i2, i3, i4;
    struct hostent *hp;
    struct sockaddr_in addr_dist;
    
    if(sscanf(IP_GROUP, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
      if((i4 < 224) || (i4 > 239)){
	UNICAST = TRUE;
      }
    }else{
      UNICAST = TRUE;
      if(!LOCAL_REPLAY){
	if((hp=gethostbyname(IP_GROUP)) == 0){
	  fprintf(stderr, "ivs_replay: %d-Unknown host %s\n", (int)getpid(),
		  IP_GROUP);
	  exit(1);
	}
	bcopy(hp->h_addr, (char *)&addr_dist.sin_addr, 4);
	strcpy(IP_GROUP, inet_ntoa(addr_dist.sin_addr));
      }
    }
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
  }

  { /* Getting my sin_addr and my IP_addr */

    struct hostent *hp;
    int namelen=256;
    char hostname[256];

    if(gethostname(hostname, namelen) != 0){
      perror("gethostname");
      exit(1);
    }
    if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "%d-Unknown host\n", (int)getpid());
      exit(1);
    }
    bcopy(hp->h_addr, (char *)&my_sin_addr.sin_addr, 4);
    strcpy(IP_addr, inet_ntoa(my_sin_addr.sin_addr));
    strcpy(fullname, "ivs_replay@");
    strcat(fullname, hostname);
  }


  /* Initializations */

  strcpy(title, "ivs_replay (3.5v) ");
  strcat(title, filename);

  if(LOCAL_REPLAY){

    /* Audio initialization */
#ifndef NO_AUDIO
#ifdef SPARCSTATION
    if(!sounddevinit()){
      fprintf(stderr, "cannot opening audioctl device");
      AUDIO_INIT_FAILED = TRUE;
    }else{
      SPEAKER = (soundgetdest() == A_HEADPHONE ? FALSE : TRUE);
    }
#else
    SPEAKER = TRUE;
#endif
#ifdef HPUX
    if(!audioInit(audioserver)){
      fprintf(stderr,"cannot connect to AudioServer\n");
      AUDIO_INIT_FAILED = TRUE;
    }
#endif
#ifdef AF_AUDIO
    if(!audioInit()){
      fprintf(stderr,"cannot connect to AF audio server\n");
      AUDIO_INIT_FAILED = TRUE;
    }else{
      sounddest(A_SPEAKER);
    }
#endif
#endif /* NO_AUDIO */
    sock_video = CreateSendSocket(&addr_video, &len_addr_video, 
				  VIDEO_REPLAY_PORT, IP_addr, &ttl,
				  &port_source, UNICAST, &null);
  }else{
    sock_video = CreateSendSocket(&addr_video, &len_addr_video, 
				  VIDEO_PORT, IP_GROUP, &ttl, &port_source,
				  UNICAST, &null);
    sock_audio = CreateSendSocket(&addr_audio, &len_addr_audio, 
				  AUDIO_PORT, IP_GROUP, &ttl, &port_source,
				  UNICAST, &null);
    sock_control = CreateSendSocket(&addr_control, &len_addr_control, 
				    CONTROL_PORT, IP_GROUP, &ttl,
				    &port_source, UNICAST, &null);
  }
  n = 0;
  if(AUTO_SEND){
    XtSetArg(args[n], XtNmappedWhenManaged, False); n++; 
  }
  toplevel = XtAppInitialize(&app_con, "IVS_REPLAY", NULL, 0, &argc, argv, 
			     fallback_resources, args, n);


  /*
  *  Create all the bitmaps ...
  */
  LabelSpeakerOn = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				speaker_bits, speaker_width, speaker_height);

  LabelSpeakerOff = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
			        headphone_bits, headphone_width,
				headphone_height);

  /* 
  * Create a Form widget and put all children in that form widget.
  */
  n = 0;
  main_form = XtCreateManagedWidget("MainForm", formWidgetClass, toplevel,
				    args, n);

  /*
  *  Form.
  */
  n = 0;
  form= XtCreateManagedWidget("Form", formWidgetClass, main_form,
				args, n);
  /*
  *  Create the audio control utilities.
  */
  CreateScale(&form_v_audio, &scroll_v_audio, &name_v_audio, &value_v_audio,
	      form, "FormVolAudio", "NameVolAudio", DefaultVolume);
#ifndef NO_AUDIO
  if(!AUDIO_INIT_FAILED){
    XtAddCallback(scroll_v_audio, XtNjumpProc, (XtCallbackProc)CallbackVol, 
		  (XtPointer)value_v_audio);
    SetVolumeGain(DefaultVolume);
  }
#endif  
#ifndef NO_AUDIO
#if defined(SPARCSTATION) || defined(HPUX) || defined(AF_AUDIO)
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_v_audio); n++;
  if(SPEAKER){
    XtSetArg(args[n], XtNbitmap, LabelSpeakerOn); n++;
  }else{
    XtSetArg(args[n], XtNbitmap, LabelSpeakerOff); n++;
  }
  output_audio = XtCreateManagedWidget("ButtonOutAudio", commandWidgetClass, 
				       form, args, n);
  if(!AUDIO_INIT_FAILED){
    XtAddCallback(output_audio, XtNcallback,
		  (XtCallbackProc)CallbackAudioPort, NULL);
  }
#endif 
#endif /* NO_AUDIO */

  /*
  *  Create the Play and Quit buttons.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form); n++;
  button_play = XtCreateManagedWidget("ButtonPlay", commandWidgetClass,
				      main_form, args, n);
  if(LOCAL_REPLAY)
    XtAddCallback(button_play, XtNcallback, (XtCallbackProc)CallbackLocalPlay,
		  (XtPointer)NULL);
  else
    XtAddCallback(button_play, XtNcallback, (XtCallbackProc)CallbackRemotePlay,
		  (XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_play); n++;
  XtSetArg(args[n], XtNfromHoriz, form); n++;
  button_pause = XtCreateManagedWidget("ButtonPause", commandWidgetClass,
				       main_form, args, n);
  XtAddCallback(button_pause, XtNcallback, (XtCallbackProc)CallbackPause,
		(XtPointer)NULL);
  XtSetMappedWhenManaged(button_pause, FALSE);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_play); n++;
  XtSetArg(args[n], XtNfromHoriz, button_pause); n++;
  button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
				      main_form, args, n);
  XtAddCallback(button_quit, XtNcallback, (XtCallbackProc)CallbackQuit,
		(XtPointer)NULL);
  if(AUTO_SEND){
    alarm((u_int)1);
    signal(SIGALRM, Start);
  }
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);
  signal(SIGUSR1, ToggleDebug);
  XtRealizeWidget(toplevel);
  XStoreName(XtDisplay(toplevel), XtWindow(toplevel), title);
  XtAppMainLoop(app_con);

}

