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
*  File              : ivs_record.c    	                		   *
*  Last modification : 1995/7/13   					   *
*  Author            : Thierry Turletti					   *
*--------------------------------------------------------------------------*
*  Description : Perform an ivs encoded audio/video clip.                  *
*     Recording starts as soon as the first full intra encoded image is    *
*     received using the -fir option or at the first packet received.      *
*                                                                          *
*     Audio and video packets are encoded in a file as :                   *
*                                                                          *
*    1               16                                                    *
*     ----------------                                                     *
*    |  data length   |                                                    *
*    |________________|                                                    *
*    |   timestamp    |                                                    *
*    |________________|                                                    *
*    |  Header data   |                                                    *
*    |       &        |                                                    *
*    |     data       |                                                    *
*    |  ...........   |                                                    *
*    |________________|                                                    *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Dialog.h>

#include "general_types.h"
#define DATA_MODULE
#include "protocol.h"
#include "h261.h"
#include "quantizer.h"
#include "rtp.h"

#define LabelStart        "Push to start recording"
#define LabelStop         "Push to stop recording"
#define LabelClearWarning "This operation will overwrite the clip"
#define streq(a, b)        (strcmp((a), (b)) == 0)

/* From identifier.c */
extern u_short FindIdentifier();
extern u_short ManageIdentifier();
extern BOOLEAN AddIdentifier();
extern void AddCSRCOption();


String fallback_resources[] = {
  "IVS_RECORD*font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "IVS_RECORD*MainForm.background:             Grey",
  "IVS_RECORD*MsgWarning.background:           Red",
  "IVS_RECORD*MsgShowWarning.background:       Grey",
  "IVS_RECORD*MsgShowWarning.label.foreground: Black",
  "IVS_RECORD*MsgShowWarning.label.background: MistyRose",
  "IVS_RECORD*Command.background:              Black",
  "IVS_RECORD*Command.foreground:              MistyRose",
  "IVS_RECORD*ButtonQuit.background:           Red",
  "IVS_RECORD*ButtonQuit.foreground:           White",
  "IVS_RECORD*ButtonQuit.label:                Quit",
  "IVS_RECORD*ButtonClear.label:               Clear",
  NULL,
};


static XtAppContext app_con;
static Widget toplevel, button_command, button_clear, button_quit, popup_clear;
static u_long tab_sin_addr[N_MAX_STATIONS];
static char hostname[256];
static u_long hostname_target;
static lw=0; /* length of the output file */
static struct sockaddr_in addr_video, addr_audio, addr_control, addr_coder;
static u_long sin_addr_from;
static char IP_GROUP[256], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
static int sock_video, sock_audio, sock_control;
static int len_addr_video, len_addr_audio, len_addr_control;
static int mask_video, mask_audio, mask_control;
static char version[100];
static FILE *F_out;
static BOOLEAN RECORDING=FALSE;
static BOOLEAN RECORDING_REQUEST=FALSE;
static BOOLEAN LOCAL_MODE=FALSE;
static BOOLEAN FIR_USELESS=FALSE; /* True if Full Intra Request not required
				    to start recording */
static BOOLEAN FIRST_P;
static char filename[256];


/* Global variable */
Widget main_form;
BOOLEAN DEBUG_MODE=FALSE;
BOOLEAN UNICAST=FALSE;
/* Following global variables widgets not used here */
Widget msg_error;
Widget video_coder_panel;
Widget video_grabber_panel;
Widget rate_control_panel;
Widget info_panel;
Widget form_control, scroll_control, value_control;
Widget button_popup1, button_popup2, button_popup3, button_popup4;
Widget form_frms, form_format;
Widget label_band2, label_ref2;
Pixmap mark;
S_PARAM param;



/***************************************************************************
 * Procedures
 **************************************************************************/


static void Usage(name)
     char *name;
{
  fprintf(stderr, 
 "Usage: %s addr_from[/video_port] [-nofir] [-only hostname] -clip filename\n",
	  name);
  fprintf(stderr, "\t addr_from is a multicast address or a hostname\n");
  fprintf(stderr, "\t video_port is by default %s\n", MULTI_VIDEO_PORT);
  fprintf(stderr,
	  "\t -nofir: Do not request Full INTRA video image to start\n");
  fprintf(stderr,
	  "\t -only hostname: In multicast mode, only record hostname\n");
  fprintf(stderr, "\t filename is the recorded clip\n");
  exit(1);
}


static void Quit(null)
     int null;
{
  fprintf(stderr, "ivs_record: %d bytes written on %s \n", lw, filename);
  fclose(F_out);
  XtDestroyApplicationContext(app_con);
  exit(0);
}


static void FlushSock(sock)
     int sock;
{
  int mask, mask0, lr;
  struct sockaddr_in from;
  char buf[P_MAX];
  static struct timeval timer;
  static int fromlen;
  static FIRST=TRUE;

  if(FIRST){
    fromlen = sizeof(from);
    timer.tv_sec = 0;
    timer.tv_usec = 0;
    FIRST = FALSE;
  }

  mask = mask0 = 1 << sock;

  while(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	       (struct timeval *)&timer) > 0){
    if((lr=recvfrom(sock, (char *)buf, P_MAX, 0, (struct sockaddr *)&from, 
		    &fromlen)) < 0){
      perror("FlushSock'recvfrom");
      return;
    }
    mask = mask0;
  }
}



/**************************************************
 * Callbacks
 **************************************************/


static void CallbackCommand(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
  {
    if(RECORDING){
      /* We must stop recording */
      RECORDING = RECORDING_REQUEST = FALSE;
      XtSetMappedWhenManaged(button_clear, TRUE);
      ChangeLabel(button_command, LabelStart);
    }else{
      RECORDING_REQUEST = TRUE;
      FIRST_P = TRUE;
      if(!LOCAL_MODE){
	FlushSock(sock_audio);
	FlushSock(sock_video);
      }
      XtSetMappedWhenManaged(button_clear, FALSE);
      XtSetMappedWhenManaged(button_command, FALSE);
    }
  }



static void ClearClip(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;

  fclose(F_out);
  lw = 0;
  if((F_out=fopen(filename, "w")) == NULL)
    ShowWarning("Cannot clear ivs clip");
  if(fwrite(version, 1, 14, F_out) != 14){
    ShowWarning("Cannot write clip version");
  }
  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}



static void AbortClear(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;

  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}
 


static void CallbackClear(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data; 
{
  Arg         args[5];
  Widget      msg_from;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup_clear = XtCreatePopupShell("ClearWarning", transientShellWidgetClass, 
				   main_form, args, n);

  n = 0;
  XtSetArg(args[n], XtNlabel, LabelClearWarning); n++;
  msg_from = XtCreateManagedWidget("MsgShowWarning", dialogWidgetClass, 
				   popup_clear, args, n);

  XawDialogAddButton(msg_from, "OK", ClearClip, (XtPointer) msg_from);

  XawDialogAddButton(msg_from, "Cancel", AbortClear, (XtPointer) msg_from);

  XtPopup(popup_clear, XtGrabNone);
  XtRealizeWidget(popup_clear);
}


static void CallbackQuit(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data; 
{
  int null=0;
  
  Quit(null);
}


static int NewDelay(FIRST)
     BOOLEAN FIRST;
{
  struct timeval timer;
  static double lasttime;
  double diff, time;
  u_long deltat;

  gettimeofday(&timer, (struct timezone *)NULL);
  time = (double)1000.0*timer.tv_sec + (double)timer.tv_usec/1000.0;
  if(FIRST){
    lasttime = time;
    deltat = 0;
  }else{
    diff = time - lasttime;
    deltat = (u_long)(diff);
#ifdef DEBUG
    fprintf(stderr, "time -%f-%f- %d\n", time, lasttime, deltat);
#endif
    lasttime = time;
  }
  return((int)deltat);
}



static void save(File, buffer, lbuffer, delay)
     FILE *File;
     u_int *buffer;
     int lbuffer, delay;
{
  u_int data;
  ivs_record_hdr *head; /*fixed IVS RECORD header */
  
  /* First save the buffer length and the delay value */
  head = (ivs_record_hdr *)&data;
  head->len = htons((u_short) lbuffer);
  head->delay = htons((u_short) delay);
  if(fwrite((char *)&data, 1, 4, File) != 4){
    perror("fwrite header");
    exit(1);
  }
  
  /* Now save the buffer (audio or video) */
  if(fwrite((char *)buffer, 1, lbuffer, File) != lbuffer){
    perror("fwrite buffer");
    exit(1);
  }
  lw += (lbuffer+4);
}



static void LocalRecord()
{

  struct sockaddr_in from;
  struct timeval timer;
  u_int buf[P_MAX/4];
  int lr, mask, delay, fromlen, next_option;
  static BOOLEAN FIRST=TRUE;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *r; /* RTP options */

  if(RECORDING_REQUEST){
    timer.tv_sec = 0;
    timer.tv_usec = 0;
    mask = mask_audio;
    while(select(sock_audio+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_audio, (char *)buf, P_MAX, 0,
		      (struct sockaddr *) &from, &fromlen)) < 0){
	perror("recvfrom audio socket");
	return;
      }
      sin_addr_from = (u_long)from.sin_addr.s_addr;
      if(sin_addr_from != hostname_target)
	continue;
      if(FIRST_P){
	if(FIR_USELESS){
	  ChangeAndMappLabel(button_command, LabelStop);
	  delay = NewDelay(TRUE);
	  RECORDING=TRUE;
	}
	FIRST_P = FALSE;
      }
      if(RECORDING){
	h = (rtp_hdr_t *)buf;
	if(h->ver != RTP_VERSION){
	  if(DEBUG_MODE){
	    fprintf(stderr, "Bad RTP version %d\n", h->ver);
	  }
	  mask = mask_audio;
	  continue;
	}	  
	if(h->p){
	  next_option = 2;
	  r = (rtp_t *) &buf[next_option];
	  while(!r->generic.final){
	    if(r->generic.type == RTP_SSRC){
	      /* A SSRC already added */
	      if(DEBUG_MODE){
		fprintf(stderr, "SSRC already added, reset the identifier\n");
	      }
	      r->ssrc.id = 0;
	    }
	    next_option += r->generic.length;
	    r = (rtp_t *) &buf[next_option];
	  }
	}
	switch(h->format) {
	case RTP_PCM_CONTENT:
	case RTP_ADPCM_4:
	case RTP_VADPCM_CONTENT:
	case RTP_ADPCM_6:
	case RTP_ADPCM_5:
	case RTP_ADPCM_3:
	case RTP_ADPCM_2:
	case RTP_LPC:
	case RTP_GSM_13:
	    break;
	default:
	  if(DEBUG_MODE){
	    fprintf(stderr, "Bad RTP content type received: %d\n",
		    h->format);
	  }
	  mask = mask_audio;
	  continue;
	}
	delay = NewDelay(FALSE);
#ifdef DEBUG	
	fprintf(stderr, "recorded %d bytes audio, %d packet\n", lr, 
		ntohs(h->seq));
#endif	
	save(F_out, buf, lr, delay);
      }
      mask = mask_audio;
    }

    mask = mask_video;
    while(select(sock_video+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_video, (char *)buf, P_MAX, 0, (struct sockaddr *)
		      &from, &fromlen)) < 0){
	perror("recvfrom video socket");
	return;
      }
      sin_addr_from = (u_long)from.sin_addr.s_addr;
      if(sin_addr_from != hostname_target)
	continue;
      else{
	
	if(FIRST){
	  bzero((char *)&addr_coder, sizeof(addr_coder));
	  addr_coder.sin_addr.s_addr = hostname_target;
	  addr_coder.sin_port = (u_short)from.sin_port;
	  addr_coder.sin_family = AF_INET;
	  FIRST =FALSE;
	}
      }      
      h = (rtp_hdr_t *)buf;
      if(h->ver != RTP_VERSION){
	if(DEBUG_MODE){
	  fprintf(stderr, "Bad RTP version %d\n", h->ver);
	}
	mask = mask_video;
	continue;
      }	  
      if(h->format != RTP_H261_CONTENT){
	if(DEBUG_MODE){
	  fprintf(stderr, "Bad RTP video content: %d\n", h->format);
	}
	mask = mask_video;
	continue;
      }
      next_option = 2;
      if(h->p){
	do{
	  r = (rtp_t *) &buf[next_option];
	  if(r->generic.type == RTP_SSRC){
	    r->ssrc.id = 0;
	  }
	  next_option += r->generic.length;
	}while(!r->generic.final);
      }
      r = (rtp_t *) &buf[next_option];
      if(!RECORDING){
	/* Check if it's the first full INTRA image */
	if(FIR_USELESS || (r->h261.F == 0) || r->h261.I){
	  RECORDING = TRUE;
	  ChangeAndMappLabel(button_command, LabelStop);
	  FlushSock(sock_audio);
	  delay = NewDelay(TRUE);
	  r->h261.F = 0;
	  r->h261.Q = 0;
	  save(F_out, buf, lr, delay);
	}else{
	  /* A full INTRA image encoded is required */
	  SendFIR(&addr_coder, len_addr_video);
	}
      }else{
#ifdef DEBUG	
	fprintf(stderr, "recorded %d bytes video, %d packet\n", lr, 
		ntohs(h->seq));
#endif	
	delay = NewDelay(FALSE);
	r->h261.F = 0;
	r->h261.Q = 0;
	save(F_out, buf, lr, delay);
      }
      mask = mask_video;
    }/* while */
  } /* RECORDING_REQUEST */
}



static void BroadcastRecord()
{

  static u_long tab_sin_addr[N_MAX_STATIONS][N_MAX_STATIONS];
  static u_short last_identifier[N_MAX_STATIONS];
  struct sockaddr_in from;
  struct timeval timer;
  u_int buf[P_MAX/4];
  int lr, mask, delay, fromlen, next_option;
  u_short identifier, id_from;
  BOOLEAN OPTION_PRESENT;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *r; /* RTP options */

  
  if(RECORDING_REQUEST){
    timer.tv_sec = 0;
    timer.tv_usec = 0;
    mask = mask_audio;
    while(select(sock_audio+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_audio, (char *)&buf[1], P_MAX, 0,
		      (struct sockaddr *) &from, &fromlen)) < 0){
	perror("recvfrom audio socket");
	return;
      }
      sin_addr_from = (u_long)from.sin_addr.s_addr;
      if(RECORDING){
	h = (rtp_hdr_t *)&buf[1];
	if(h->ver != RTP_VERSION){
	  if(DEBUG_MODE){
	    fprintf(stderr, "Bad RTP version %d\n", h->ver);
	  }
	  mask = mask_audio;
	  continue;
	}	  
	switch(h->format) {
	case RTP_PCM_CONTENT:
	case RTP_ADPCM_4:
	case RTP_VADPCM_CONTENT:
	case RTP_ADPCM_6:
	case RTP_ADPCM_5:
	case RTP_ADPCM_3:
	case RTP_ADPCM_2:
	case RTP_LPC:
	case RTP_GSM_13:
	    break;
	default:
	  if(DEBUG_MODE){
	    fprintf(stderr, "Bad RTP content type received: %d\n",
		    h->format);
	  }
	  mask = mask_audio;
	  continue;
	}
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
	if(!id_from){
	  /* The SSRC option must be added after the RTP timestamp */
	  AddSSRCOption(buf, identifier, next_option, &lr);
	}else{
	  /* Set the new identifier in the SSRC field */
	  ChangeIdValue(buf, identifier, next_option);
	}
	delay = NewDelay(FALSE);
#ifdef DEBUG	
	fprintf(stderr, "recorded %d bytes audio\n", lr);
#endif	
	save(F_out, buf, lr, delay);
      }
      mask = mask_audio;
    }

    mask = mask_video;
    while(select(sock_video+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_video, (char *)&buf[1], P_MAX, 0,
		      (struct sockaddr *) &from, &fromlen)) < 0){
	perror("recvfrom video socket");
	return;
      }
      sin_addr_from = (u_long)from.sin_addr.s_addr;
      if(RECORDING){
	h = (rtp_hdr_t *)&buf[1];
	if(h->ver != RTP_VERSION){
	  if(DEBUG_MODE){
	    fprintf(stderr, "Bad RTP version %d\n", h->ver);
	  }
	  mask = mask_video;
	  continue;
	}	  
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
	if(!id_from){
	  /* The SSRC option must be added after the RTP timestamp */
	  AddSSRCOption(buf, identifier, next_option, &lr);
	  next_option --;
	}else{
	  /* Set the new identifier in the SSRC field */
	  ChangeIdValue(buf, identifier, next_option);
	}
	r = (rtp_t *) &buf[next_option];
	r = (rtp_t *) &buf[next_option+r->generic.length];
	r->h261.F = 0;
	r->h261.Q = 0;
	delay = NewDelay(FALSE);
#ifdef DEBUG      
	fprintf(stderr, "recorded %d bytes video\n", lr);
#endif      
	save(F_out, buf, lr, delay);
      } /* if RECORDING */
      mask = mask_video;
    }/* while */
    
    mask = mask_control;
    while(select(sock_control+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_control, (char *)&buf[1], P_MAX, 0,
		      (struct sockaddr *) &from, &fromlen)) < 0){
	perror("recvfrom control socket");
	return;
      }
      sin_addr_from = (u_long)from.sin_addr.s_addr;
      if(FIRST_P){
	ChangeAndMappLabel(button_command, LabelStop);
	delay = NewDelay(TRUE);
	FlushSock(sock_audio);
	FlushSock(sock_video);
	RECORDING=TRUE;
	FIRST_P = FALSE;
      }
      h = (rtp_hdr_t *)&buf[1];
      if(h->ver != RTP_VERSION){
	if(DEBUG_MODE){
	  fprintf(stderr, "p_control: bad RTP version %d\n", h->ver);
	}
	mask = mask_control;
	continue;
      }	  
      if(h->format != RTP_H261_CONTENT){
	if(DEBUG_MODE){
	  fprintf(stderr, "p_control: bad RTP video content: %d\n", h->format);
	}
	mask = mask_control;
	continue;
      }
      h->format = RTP_CONTROL_CONTENT; /* To differenciate control/data */
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
	  if(r->generic.type == RTP_H261_CONTENT){
	    /* Set the Large conference mode to avoid any feedback */
	    r->app_ivs.conf_size = LARGE_SIZE;
	  }
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
      if(!id_from){
	/* The SSRC option must be added after the RTP timestamp */
	AddSSRCOption(buf, identifier, next_option, &lr);
      }else{
	/* Set the new identifier in the SSRC field */
	ChangeIdValue(buf, identifier, next_option);
      }
      delay = NewDelay(FALSE);
#ifdef DEBUG      
      fprintf(stderr, "recorded %d bytes control\n", lr);
#endif      
      save(F_out, buf, lr, delay);
      mask = mask_control;
    }/* while */
  } /* RECORDING_REQUEST */
}



/**************************************************
 *  Main program
 **************************************************/

main(argc, argv)
     int argc;
     char **argv;
{

  int n, narg, i;
  Arg args[10];
  char *ptr;
  char title[256];
  BOOLEAN NEW_PORTS=FALSE;
  char null=0;

  if((argc < 3) || (argc > 7))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(streq(argv[narg], "-clip")){
      strcpy(filename, argv[++narg]);
      if((ptr=strstr(argv[narg], ".ivs")) == NULL){
	strcat(filename, ".ivs");
      }
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-only")){
      strcpy(hostname, argv[++narg]);
      LOCAL_MODE = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-nofir")){
      FIR_USELESS = TRUE;
      narg ++;
      continue;
    }
    if(argv[narg][0] == '-'){
      Usage(argv[0]);
    }
    if((ptr=strstr(argv[narg], "/")) == NULL){
      strcpy(IP_GROUP, argv[narg]);
      narg ++;
    }else{
      *ptr++ = (char) 0;
      strcpy(IP_GROUP, argv[narg]);
      narg ++;
      strcpy(VIDEO_PORT, ptr);
      i = atoi(VIDEO_PORT);
      sprintf(AUDIO_PORT, "%d", ++i);
      sprintf(CONTROL_PORT, "%d", ++i);
      NEW_PORTS = TRUE;
    }
  }
  {
/* MULTICAST or UNICAST ? */
    unsigned int i1, i2, i3, i4;

    if(sscanf(IP_GROUP, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
      if(i4 < 224){
	UNICAST = LOCAL_MODE = TRUE;
      }else{
	/* MULTICAST MODE */
	if(!LOCAL_MODE)
	  FIR_USELESS = TRUE;
      }
    }else
      UNICAST = LOCAL_MODE = TRUE;
  }
  if(LOCAL_MODE){
    /* Getting the target sin_addr.s_addr */
    unsigned int i1, i2, i3, i4;
    struct hostent *hp;
    
    if(UNICAST)
      strcpy(hostname, IP_GROUP);
    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "ivs: %d-Unknown host %s\n", getpid(), hostname);
	exit(1);
      }
      bcopy(hp->h_addr, (char *)&hostname_target, 4);
    }else{
      hostname_target = (u_long)inet_addr(hostname);
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

  /* Initializations */

  strcpy(version, IVS_CLIP_VERSION);
  if((F_out=fopen(filename, "r")) != NULL){
    /* There is already such a clip */
    fclose(F_out);
    if((F_out=fopen(filename, "a")) == NULL){
      fprintf(stderr, "%s: Cannot open %s file in 'a' mode\n",
	      argv[0], filename);
      exit(1);
    }
  }else{
    /* Create the new file and the IVS clip header */
    if((F_out=fopen(filename, "w")) == NULL){
      fprintf(stderr, "%s: Cannot open %s file in 'w' mode\n",
	      argv[0], filename);
      exit(1);
    }
    if(fwrite(version, 1, 14, F_out) != 14){
      perror("fwrite clip version");
      exit(1);
    }
  }

  strcpy(title, "ivs_record (3.5v) ");
  strcat(title, filename);
  
  sock_video=
    CreateReceiveSocket(&addr_video, &len_addr_video, VIDEO_PORT, IP_GROUP,
			UNICAST, &null);
  mask_video = 1 << sock_video;

  sock_audio=
    CreateReceiveSocket(&addr_audio, &len_addr_audio, AUDIO_PORT, IP_GROUP,
			UNICAST, &null);
  mask_audio = 1 << sock_audio;
  if(!UNICAST){
    sock_control=
      CreateReceiveSocket(&addr_control, &len_addr_control, CONTROL_PORT,
			  IP_GROUP, UNICAST, &null);
      mask_control = 1 << sock_control;
  }else{
    bzero((char *)tab_sin_addr, N_MAX_STATIONS*sizeof(u_long));
  }

  toplevel = XtAppInitialize(&app_con, "IVS_RECORD", NULL, 0, &argc, argv, 
			     fallback_resources, NULL, 0);


  /* 
  * Create a Form widget and put all children in that form widget.
  */

  main_form = XtCreateManagedWidget("MainForm", formWidgetClass, toplevel,
				    (ArgList)NULL, 0);

  /*
  *  Create the Start/Stop command button.
  */
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelStart); n++;
  button_command = XtCreateManagedWidget("ButtonCommand", commandWidgetClass,
					 main_form, args, (Cardinal)n);
  XtAddCallback(button_command, XtNcallback, CallbackCommand, NULL);


  /*
  *  Create the clear clip button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_command); n++;
  button_clear = XtCreateManagedWidget("ButtonClear", commandWidgetClass,
				       main_form, args, (Cardinal)n);
  XtAddCallback(button_clear, XtNcallback, CallbackClear, NULL);


  /*
  *  Create the Quit button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_clear); n++;
  button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
					 main_form, args, (Cardinal)n);
  XtAddCallback(button_quit, XtNcallback, CallbackQuit, NULL);

  if(LOCAL_MODE){
    XtAppAddInput(app_con, sock_video, (XtPointer)XtInputReadMask,
		  (XtInputCallbackProc)LocalRecord, NULL);
    XtAppAddInput(app_con, sock_audio, (XtPointer)XtInputReadMask,
		  (XtInputCallbackProc)LocalRecord, NULL);
  }else{
    XtAppAddInput(app_con, sock_video, (XtPointer)XtInputReadMask,
		  (XtInputCallbackProc)BroadcastRecord, NULL);
    XtAppAddInput(app_con, sock_audio, (XtPointer)XtInputReadMask,
		  (XtInputCallbackProc)BroadcastRecord, NULL);
    XtAppAddInput(app_con, sock_control, (XtPointer)XtInputReadMask,
		  (XtInputCallbackProc)BroadcastRecord, NULL);
  }
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);

  XtRealizeWidget(toplevel);
  XStoreName(XtDisplay(toplevel), XtWindow(toplevel), title);
  XtAppMainLoop(app_con);

}

