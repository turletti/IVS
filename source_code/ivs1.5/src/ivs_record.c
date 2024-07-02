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
*  File    : ivs_record.c    	                			   *
*  Date    : October 1992   						   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description : Perform an ivs ancoded audio/video clip.                  *
*     Recording starts as soon as the first full intra encoded image is    *
*     received.                                                            *
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
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>

#include "h261.h"
#include "protocol.h"


static Widget ButtonClear, ButtonCommand, ButtonQuit, temp, msg_err, msg_info;
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


/***************************************************************************
 * Procedures
 **************************************************************************/

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
#ifdef MULTICAST
  int one=1;
  struct ip_mreq imr;
#endif 

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
#ifdef MULTICAST
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
#else
    fprintf(stderr, "MULTICAST mode not implemented, use unicast option\n");
    exit(-1);
#endif /* MULTICAST */
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



static int GetType(buf)
     u_char *buf;
{
  return(buf[0] >> 6);
}


void DisplayMsgError(msg)
     char *msg;
{
  Arg args[1];
  XmString xms;
  
  xms = XmStringCreate(msg, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[0], XmNmessageString, xms);
  XtSetValues(msg_err, args, 1);
  XmStringFree(xms);
  XtManageChild(msg_err);
}


void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-i @IP] [-h hostname] filename\n", name);
  exit(1);
}



static void MappWidget(w)
     Widget w;
{
  Arg args[1];
  
  XtSetArg (args[0], XmNmappedWhenManaged, TRUE);
  XtSetValues(w, args, 1);
}


static void UnMappWidget(w)
     Widget w;
{
  Arg args[1];
  
  XtSetArg (args[0], XmNmappedWhenManaged, FALSE);
  XtSetValues(w, args, 1);
}



static void ChangeButton (widget, name)
     Widget         widget;
     char *name;
{
  Arg   args[1];
  XmString xms;
  
  xms = XmStringCreate(name, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[0], XmNlabelString, xms);
  XtSetValues (widget, args, 1);
  XmStringFree(xms);
}


static void ChangeColor (widget)
     Widget         widget;
{
  Arg   args[10];
  Pixel bg_color, fg_color;
  int  n = 0;
  
  XtSetArg (args[n], XmNbackground, &bg_color); n++;
  XtSetArg (args[n], XmNforeground, &fg_color); n++;
  XtGetValues (widget, args, n);
  
  n = 0;
  XtSetArg (args[n], XmNbackground, fg_color); n++;
  XtSetArg (args[n], XmNforeground, bg_color); n++;
  XtSetValues (widget, args, n);
}



/**************************************************
 * Callbacks
 **************************************************/

static void CallbackCommand(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(RECORDING){
      /* We must stop recording */
      RECORDING = RECORDING_REQUEST = FALSE;
      MappWidget(ButtonClear);
      ChangeButton(ButtonCommand, "Push to start recording");
      printf("**** %d bytes written\n", lwrite);
    }else{
      RECORDING_REQUEST = TRUE;
      UnMappWidget(ButtonClear);
      UnMappWidget(ButtonCommand);
    }
  }


static void CallbackErase(w, null, reason)
     Widget w;
     char*  null;
     XmAnyCallbackStruct *reason;
{
  fclose(F_out);
  lwrite = 0;
  if((F_out=fopen(filename, "w")) == NULL)
    DisplayMsgError("Cannot clear ivs clip");
}  


static void CallbackClear(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    XtManageChild(msg_info);
  }


static void CallbackQuit(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    fclose(F_out);
    exit(0);
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
    u_char temp;

    /* First save the buffer length */
    temp = (u_char)(lbuffer >> 8);
    if(putc(temp, File) == EOF){
      perror("putc lbuffer");
      exit(1);
    }
    temp = (u_char)(lbuffer & 0x00ff);
    if(putc(temp, File) == EOF){
      perror("putc lbuffer");
      exit(1);
    }

    /* save the delay value */
    temp = (u_char)(delay >> 8);
    if(putc(temp, File) == EOF){
      perror("putc delay");
      exit(1);
    }
    temp = (u_char)(delay & 0x00ff);
    if(putc(temp, File) == EOF){
      perror("putc delay");
      exit(1);
    }

    /* Now save the buffer (audio or video) */
    if(fwrite(buffer, 1, lbuffer, File) != lbuffer){
      perror("fwrite buffer");
      exit(1);
    }
    lwrite += (lbuffer+4);
  }


static void FirstImageRequest()
  {
    static BOOLEAN first_request=TRUE;
    static struct sockaddr_in addr_coder;
    static struct hostent *hp;
    static int sock, len_addr;
    u_char buf[1];
    u_long add_long;

    if(first_request){
      first_request = FALSE;
      bzero((char *)&addr_coder, sizeof(addr_coder));
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "Unknown host : %s\n", hostname);
	exit(1);
      }else
	bcopy(hp->h_addr, (char *)&addr_coder.sin_addr, hp->h_length);
      addr_coder.sin_port = htons(PORT_VIDEO_CONTROL);
      addr_coder.sin_family = AF_INET;
      len_addr = sizeof(addr_coder);
      if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	fprintf(stderr, "Cannot create datagram socket\n");
	exit(1);
      }
    }
    buf[0] = (u_char)0;
    if(sendto(sock, (char *)buf, 1, 0, &addr_coder, len_addr) < 0){
      fprintf(stderr, "sendto: Cannot send First Image Request\n");
      exit(1);
    }
  }



static void Record()
{

  int lr, mask, delay;
  struct hostent *host;
  char buf[P_MAX];
  struct sockaddr_in from;
  int fromlen;
  struct timeval timer;

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
      if((from.sin_addr.s_addr == target.sin_addr.s_addr) &&
	 (GetType(buf) == VIDEO_TYPE)){
	if(!RECORDING){
	  /* Check if it's the first full INTRA image */
	  if((buf[0] & 0x3f) == 0){
	    RECORDING = TRUE;
	    ChangeButton(ButtonCommand, "Push to stop recording");
	    MappWidget(ButtonCommand);
	    delay = NewDelay(TRUE);
	    save(F_out, buf, lr, delay);
	  }else{
	    /* A full INTRA image encoded is required */
	    FirstImageRequest();
	  }
	}else{
	  delay = NewDelay(FALSE);
	  save(F_out, buf, lr, delay);
	}
      }
    }
  }
}



/**************************************************
 *  Main program
 **************************************************/

main(arg, argv)
     int arg;
     char **argv;
{
  int argc=1;
  XtAppContext app;
  Display  *disp;
  Widget  appShell, main_win, DialogShell1, DialogShell2, MainForm, dwa;
  Arg     args[10];
  int  n = 0;
  FILE *F_aux;
  static char name[50]="IVS_RECORD (1.0)";
  char *pt = name;
  register int i;
  XmString xms;
  int nb_arg=1;
  int file_def=0, host_def=0, port_def=0;
  int narg;
  int lr;
  BOOLEAN new_ip_group=FALSE;
  BOOLEAN another_hostname=FALSE;
  char display[1], ntemp[256];

  if((arg<2) || (arg>6))
    Usage(argv[0]);
  narg = 1;
  while(narg != arg){
    if(strcmp(argv[narg], "-i") == 0){
      strcpy(IP_GROUP, argv[++narg]);
      narg ++;
      new_ip_group = TRUE;
      continue;
    }else{
      if(strcmp(argv[narg], "-h") == 0){
	another_hostname = TRUE;
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
  if((F_aux=fopen(".videoconf.default", "r")) != NULL){
    fscanf(F_aux, "%s%s", ntemp, AUDIO_PORT);
    fscanf(F_aux, "%s%s", ntemp, VIDEO_PORT);
    fscanf(F_aux, "%s%s", ntemp, CONTROL_PORT);
    if(!new_ip_group)
      fscanf(F_aux, "%s%s", ntemp, IP_GROUP);
    fclose(F_aux);
  }else{
    strcpy(AUDIO_PORT, DEF_AUDIO_PORT);
    strcpy(VIDEO_PORT, DEF_VIDEO_PORT);
    strcpy(CONTROL_PORT, DEF_CONTROL_PORT);
    if(!new_ip_group)
      strcpy(IP_GROUP, DEF_IP_GROUP);
  }
  {
    /* getting the target sin_addr */

    struct hostent *hp;
    int namelen=100;

    if(!another_hostname)
      if(gethostname(hostname, namelen) != 0){
	perror("gethostname");
	exit(1);
      }
    if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "%d-Unknown host\n", getpid());
	exit(1);
      }
    bcopy(hp->h_addr, (char *)&target.sin_addr, 4);
  }

  /* Initializations */

  strcpy(display, "");
  sock_audio=
    CreateReceiveSocket(&addr_audio, &len_addr_audio, AUDIO_PORT, IP_GROUP);
  mask_audio = 1 << sock_audio;
  sock_video=
    CreateReceiveSocket(&addr_video, &len_addr_video, VIDEO_PORT, IP_GROUP);
  mask_video = 1 << sock_video;

  if((F_out=fopen(filename, "w")) == NULL){
    fprintf(stderr, "Cannot open %s output file\n", filename);
    exit(1);
  }

  XtToolkitInitialize();
  app = XtCreateApplicationContext();
  disp = XtOpenDisplay(app, display,
		       "ivs_record",   /* application name */
		       "VIDEOCONF",    /* application class */
		       NULL, 0,        /* options */
		       &arg, argv);    /* command line parameters */
  if(!disp){
    fprintf(stderr, "Unable to open display %s, trying unix:0\n", display);
    strcpy(display, "unix:0");
    disp = XtOpenDisplay(app, display,
			 "ivs_record",   /* application name */
			 "VIDEOCONF",    /* application class */
			 NULL, 0,        /* options */
			 &arg, argv);    /* command line parameters */
    if(!disp){
      fprintf(stderr, "Cannot open display unix:0\n");
      exit(0);
     }
  }
  n = 0;
  XtSetArg (args[n], XmNtitle, pt); n++;
  XtSetArg (args[n], XmNallowShellResize, TRUE); n++;
  appShell = XtAppCreateShell("ivs_record", "VIDEOCONF", 
			      applicationShellWidgetClass, disp, args, n);
    
  main_win = XmCreateMainWindow(appShell, "main_app", NULL, 0);

  n = 0;
  MainForm = XmCreateForm (main_win, "main", args, n);

  n = 0;
  DialogShell1 = XmCreateDialogShell(appShell, "Error", args, n);

  n = 0;
  DialogShell2 = XmCreateDialogShell(appShell, "Warning", args, n);

  n = 0;
  XtSetArg (args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  dwa = XmCreateDrawingArea (MainForm, "dwa", args, n);

  n = 0;
  XtSetArg (args[n], XmNdialogType, XmDIALOG_ERROR); n++;
  msg_err = XmCreateMessageBox(DialogShell1, "MSG_ERR", args, n);
  temp = (Widget)XmMessageBoxGetChild(msg_err, XmDIALOG_CANCEL_BUTTON);
  XtUnmanageChild(temp);
  temp = XmMessageBoxGetChild(msg_err, XmDIALOG_HELP_BUTTON);
  XtUnmanageChild(temp);

  n = 0;
  xms = XmStringCreate("This operation will overwrite the clip", 
		       XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNmessageString, xms); n ++;
  XtSetArg (args[n], XmNdialogType, XmDIALOG_WARNING); n++;
  msg_info = XmCreateMessageBox(DialogShell2, "MSG_INFO", args, n);
  XmStringFree(xms);
  temp = XmMessageBoxGetChild(msg_info, XmDIALOG_HELP_BUTTON);
  XtUnmanageChild(temp);
  XtAddCallback (msg_info, XmNokCallback, CallbackErase, NULL);

  n = 0;
  XtSetArg (args[n], XmNx, 240); n++;
  XtSetArg (args[n], XmNy, 10); n++;
  ButtonQuit = XmCreatePushButton (dwa, "Quit", args, n);
  XtAddCallback (ButtonQuit, XmNactivateCallback, CallbackQuit, NULL);


  n = 0;
  XtSetArg (args[n], XmNx, 185); n++;
  XtSetArg (args[n], XmNy, 10); n++;
  ButtonClear = XmCreatePushButton (dwa, "Clear", args, n);
  XtAddCallback (ButtonClear, XmNactivateCallback, CallbackClear, NULL);

  n = 0;
  XtSetArg (args[n], XmNx, 10); n++;
  XtSetArg (args[n], XmNy, 10); n++;
  ButtonCommand = XmCreatePushButton (dwa, "Push to start recording", args, n);
  XtAddCallback (ButtonCommand, XmNactivateCallback, CallbackCommand, NULL);

  XtAppAddInput(app, sock_audio, XtInputReadMask, Record, NULL);
  XtAppAddInput(app, sock_video, XtInputReadMask, Record, NULL);

  XtManageChild (ButtonCommand);
  XtManageChild (ButtonClear);
  XtManageChild (ButtonQuit);
  XtManageChild (dwa);
  XtManageChild (MainForm);
  XtManageChild (DialogShell2);
  XtManageChild (DialogShell1);
  XtManageChild(main_win);
  XtRealizeWidget(appShell);

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, CallbackQuit);
  signal(SIGQUIT, CallbackQuit);

  XtAppMainLoop(app);

}

