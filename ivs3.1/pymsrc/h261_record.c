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
* 	                						   *
*  File              : h261_record.c  	                		   *
*  Last modification : 4/1/1993   					   *
*  Author            : Thierry Turletti					   *
*--------------------------------------------------------------------------*
*  Description : Perform an ivs ancoded audio/video clip.                  *
*     Recording starts as soon as the first full intra encoded image is    *
*     received.                                                            *
*                                                                          *
*     Audio and video packets are encoded in a file as :                   *
*                                                                          *
*    1               16                                                    *
*     ________________                                                     *
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

#include "h261.h"
#include "protocol.h"

#define LabelStart        "Push to start recording"
#define LabelStop         "Push to stop recording"
#define LabelClearWarning "This operation will overwrite the clip"

String fallback_resources[] = {
  "IVS_RECORD*font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "IVS_RECORD*MainForm.background:             LightSteelBlue",
  "IVS_RECORD*MsgWarning.background:           Red",
  "IVS_RECORD*MsgShowWarning.background:       LightSteelBlue",
  "IVS_RECORD*MsgShowWarning.label.background: MistyRose",
  "IVS_RECORD*Command.background:              DarkSlateGrey",
  "IVS_RECORD*Command.foreground:              MistyRose",
  "IVS_RECORD*ButtonQuit.background:           Red",
  "IVS_RECORD*ButtonQuit.foreground:           White",
  "IVS_RECORD*ButtonQuit.label:                Quit",
  "IVS_RECORD*ButtonClear.label:               Clear",
  NULL,
};

Widget main_form, msg_error;
static XtAppContext app_con;
static Widget toplevel, button_command, button_clear, button_quit, popup_clear;
static lwrite=0; /* length of the output file */
char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
struct sockaddr_in addr_audio, addr_video;
int sock_audio, sock_video; 
int len_addr_audio, len_addr_video;
FILE *F_out;
struct sockaddr_in target;
BOOLEAN RECORDING=FALSE;
BOOLEAN RECORDING_REQUEST=FALSE;
char filename[256], hostname[256];
int mask_audio, mask_video;
BOOLEAN UNICAST=FALSE;

/* Following widgets not used here */
Widget menu_video[3], form_brightness, form_contrast, button_default;
Pixmap mark;



/***************************************************************************
 * Procedures
 **************************************************************************/


static void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-I @IP] [{-m|-u} hostname] filename\n", name);
  exit(1);
}


static void Quit()
{
  fprintf(stderr, "ivs_record: %d bytes written on %s \n", lwrite, filename);
  fclose(F_out);
  XtDestroyApplicationContext(app_con);
  exit(0);
}


/**************************************************
 * Callbacks
 **************************************************/


static XtCallbackProc CallbackCommand(w, call_data, client_data)
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
      XtSetMappedWhenManaged(button_clear, FALSE);
      XtSetMappedWhenManaged(button_command, FALSE);
    }
  }



static XtCallbackProc ClearClip(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;

  fclose(F_out);
  lwrite = 0;
  if((F_out=fopen(filename, "w")) == NULL)
    ShowWarning("Cannot clear ivs clip");
  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}



static XtCallbackProc AbortClear(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;

  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}
 


static XtCallbackProc CallbackClear(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data; 
{
  Arg         args[5];
  Widget      dialog, msg_from;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  char        data[100];


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


static XtCallbackProc CallbackQuit(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data; 
{
  Quit();
}


static int NewDelay(first)
     BOOLEAN first;
{
  struct timeval timer;
  static double lasttime, time;
  int deltat;

  gettimeofday(&timer, (struct timezone *)NULL);
  time = timer.tv_sec + timer.tv_usec/1000000.0;
  if(first){
    lasttime = time;
    first = FALSE;
    return(0);
  }else{
    deltat = (int)(1000.0*(time - lasttime));
    lasttime = time;
    return(deltat);
  }
}



static void save(File, buffer, lbuffer, delay)
  FILE *File;
  char *buffer;
  int lbuffer, delay;
  {
    if(fwrite(buffer, 1, lbuffer, File) != lbuffer){
      perror("fwrite buffer");
      exit(1);
    }
    lwrite += (lbuffer+4);
  }




static void Record()
{

  int lr, mask, delay;
  struct hostent *host;
  char buf[P_MAX];
  struct sockaddr_in from;
  int fromlen, temp;
  struct timeval timer;
  static u_short feedback_port=PORT_VIDEO_CONTROL;

  if(RECORDING_REQUEST){
    timer.tv_sec = 0;
    timer.tv_usec = 0;
    mask = mask_audio;
    if(select(sock_audio+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_audio, (char *)buf, P_MAX, 0, &from, 
		      &fromlen)) < 0){
	perror("recvfrom audio socket");
	return;
      }
      if((RECORDING && (from.sin_addr.s_addr == target.sin_addr.s_addr) &&
	  GetType(buf) == AUDIO_TYPE)){
	delay = NewDelay(FALSE);
	save(F_out, buf, lr, delay);
      }
    }
    mask = mask_video;
    if(select(sock_video+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
	      (struct timeval *)&timer) > 0){
      fromlen=sizeof(from);
      bzero((char *)&from, fromlen);
      if((lr=recvfrom(sock_video, (char *)buf, P_MAX, 0, &from, 
		      &fromlen)) < 0){
	perror("recvfrom video socket");
	return;
      }
      if(from.sin_addr.s_addr == target.sin_addr.s_addr){
	if((temp = buf[0] >> 6) != RTP_VERSION){
	  fprintf(stderr, "Bad RTP version %d\n", temp);
	  return;
	}
	if((temp = buf[1] & 0x3f) != RTP_H261_CONTENT){
	  fprintf(stderr, "Bad RTP content type %d\n", temp);
	  return;
	}
	RECORDING = TRUE;
	ChangeAndMappLabel(button_command, LabelStop);
	delay = NewDelay(TRUE);
	save(F_out, buf, lr, delay);
      }
    }
  }
}



/**************************************************
 *  Main program
 **************************************************/

main(argc, argv)
     int argc;
     char **argv;
{

  int n, narg;
  Arg args[10];
  FILE *F_aux;
  BOOLEAN NEW_IP_GROUP=FALSE;
  BOOLEAN ANOTHER_HOSTNAME=FALSE;
  char display[50], ntemp[256];

  if((argc < 2) || (argc > 6))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(strcmp(argv[narg], "-I") == 0){
      strcpy(IP_GROUP, argv[++narg]);
      narg ++;
      NEW_IP_GROUP = TRUE;
      continue;
    }else{
      if(strcmp(argv[narg], "-m") == 0){
	ANOTHER_HOSTNAME = TRUE;
	strcpy(hostname, argv[++narg]);
	narg ++;
	continue;
      }else{
	if(strcmp(argv[narg], "-u") == 0){
	  UNICAST = TRUE;
	  ANOTHER_HOSTNAME = TRUE;
	  strcpy(hostname, argv[++narg]);
	  narg ++;
	  continue;
	}else{
	  strcpy(filename, argv[narg]);
	  narg ++;
	  continue;
	}
      }
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
      if(!NEW_IP_GROUP)
	fscanf(F_aux, "%s%s", ntemp, IP_GROUP);
      fclose(F_aux);
    }else{
      strcpy(AUDIO_PORT, MULTI_AUDIO_PORT);
      strcpy(VIDEO_PORT, MULTI_VIDEO_PORT);
      strcpy(CONTROL_PORT, MULTI_CONTROL_PORT);
      if(!NEW_IP_GROUP)
	strcpy(IP_GROUP, MULTI_IP_GROUP);
    }
  }

  {
    /* getting the target sin_addr */

    struct hostent *hp;
    int namelen=100;
    unsigned int i1, i2, i3, i4;

    if(!ANOTHER_HOSTNAME)
      if(gethostname(hostname, namelen) != 0){
	perror("gethostname");
	exit(1);
      }
    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), hostname);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&target.sin_addr, 4);
    }else
      target.sin_addr.s_addr = (u_long)inet_addr(hostname);
  }

  /* Initializations */

  if((F_out=fopen(filename, "a")) == NULL){
    fprintf(stderr, "Cannot open %s output file\n", filename);
    exit(1);
  }

  sock_audio=
    CreateReceiveSocket(&addr_audio, &len_addr_audio, AUDIO_PORT, IP_GROUP);
  sock_video=
    CreateReceiveSocket(&addr_video, &len_addr_video, VIDEO_PORT, IP_GROUP);

  mask_audio = 1 << sock_audio;
  mask_video = 1 << sock_video;

  toplevel = XtAppInitialize(&app_con, "IVS_RECORD", NULL, 0, &argc, argv, 
			     fallback_resources, NULL, 0);


  /* 
  * Create a Form widget and put all children in that form widget.
  */

  main_form = XtCreateManagedWidget("MainForm", formWidgetClass, toplevel,
				    NULL, 0);

  /*
  *  Create the Start/Stop command button.
  */
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelStart); n++;
  button_command = XtCreateManagedWidget("ButtonCommand", commandWidgetClass,
					 main_form, args, n);
  XtAddCallback(button_command, XtNcallback, CallbackCommand, NULL);


  /*
  *  Create the clear clip button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_command); n++;
  button_clear = XtCreateManagedWidget("ButtonClear", commandWidgetClass,
				       main_form, args, n);
  XtAddCallback(button_clear, XtNcallback, CallbackClear, NULL);


  /*
  *  Create the Quit button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_clear); n++;
  button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
					 main_form, args, n);
  XtAddCallback(button_quit, XtNcallback, CallbackQuit, NULL);


  XtAppAddInput(app_con, sock_audio, XtInputReadMask, Record, NULL);
  XtAppAddInput(app_con, sock_video, XtInputReadMask, Record, NULL);

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app_con);

}

