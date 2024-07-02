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
*  File    : ivs_replay.c    	                			   *
*  Date    : October 1992   						   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description : Play an ivs encoded audio/video clip.                     *
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
#include <arpa/inet.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/Scale.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>


#include "h261.h"
#include "protocol.h"

#define ecreteL(x) ((x)>255 ? 255 : (u_char)(x))
#define ecreteC(x) ((x)>255.0 ? 255 : ((x)<0 ? 0 :(u_char)(x)))

static Widget ButtonPort, ButtonPlay, ButtonQuit, ScaleVol, ScaleBri, 
              ScaleCon, ButtonDefault, msg_err;

LOCAL_TABLE t;
struct sockaddr_in addr_audio, addr_video;
int sock_audio, sock_video; 
int len_addr_audio, len_addr_video;
int da_fd[2], dv_fd[2], fd[2];
u_char luth[256];
FILE *F_in;
BOOLEAN SPEAKER=TRUE;
BOOLEAN QCIF2CIF=TRUE;
BOOLEAN AUDIO_DECODING=FALSE;
BOOLEAN VIDEO_DECODING=FALSE;
BOOLEAN UNICAST=TRUE;
struct sockaddr_in my_sin_addr;
char IP_addr[16];
char display[50], fullname[256], filename[256];




/***************************************************************************
 * Procedures
 **************************************************************************/

static void Quit()
{
  if(AUDIO_DECODING)
    SendPP_QUIT(da_fd);
  if(VIDEO_DECODING)
    SendPP_QUIT(dv_fd);
  SendPP_QUIT(fd);
  exit(0);
}


static void DisplayMsgError(msg)
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


static void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-o] clip\n", name);
  Quit();
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


static void InitScalesVideo()
{
  register int n;
  
  for(n=0; n<256; n++)
    luth[n] = ecreteL((int)((255*n)/150));
  
  XmScaleSetValue(ScaleBri, 71);
  XmScaleSetValue(ScaleCon, 50);
  MappWidget(ScaleBri);
  MappWidget(ScaleCon);
  MappWidget(ButtonDefault);
}


static void AbortScalesVideo()
{
  UnMappWidget(ScaleBri);
  UnMappWidget(ScaleCon);
  UnMappWidget(ButtonDefault);
}


static void Wait(delay)
     int delay;
{
  usleep(1000*delay);
}


/**************************************************
 * Callbacks
 **************************************************/


static void CallbackBri(w, null, reason)
    Widget w;
    char*  null;
    XmScaleCallbackStruct *reason;
  {
    int L;
    register int i;
    static float coeff1 = 0.0201;
    static float coeff3 = 5.1;
    float aux;
    int lwrite;

    XmScaleGetValue(w, &L);
    if(L<=50){
      for(i=0, aux=0; i<256; i++, aux+=coeff1){
	luth[i] = (int)(L*aux);
      }
    }else{
      if(L<100){
	int aux1=coeff3*L;

	for(i=0; i<256; i++){
	  luth[i] = ecreteL((int)((255*i)/(512.0-aux1)));
	}
      }else
	  for(i=0; i<256; i++){
	    luth[i] = 255;
	  }
    }

    if((lwrite=write(dv_fd[1], (char *)luth, 256)) < 0){
      DisplayMsgError("CallbackBri : error when writing on pipe");
    }
  }

	      
static void CallbackCon(w, null, reason)
    Widget w;
    char*  null;
    XmScaleCallbackStruct *reason;
  {
    int C;
    register int i;
    float aux, aux1;
    int lwrite;

    XmScaleGetValue(w, &C);
    aux = (float)C / 50.0;
    aux1 = 127.5 - 2.55*(float)C;
    for(i=0; i<256; i++){
      luth[i] = ecreteC(aux*luth[i]+aux1);
    }

    if((lwrite=write(dv_fd[1], (char *)luth, 256)) < 0){
      DisplayMsgError("CallbackCon : error when writing on pipe");
    }
  }

	      
static void CallbackDefault(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    register int i;
    int lwrite;

    XmScaleSetValue(ScaleBri, 71);
    XmScaleSetValue(ScaleCon, 50);
    for(i=0; i<256; i++){
      luth[i] = ecreteL((int)((255*i)/150));
    }
    if((lwrite=write(dv_fd[1], (char *)luth, 256)) < 0){
      DisplayMsgError("CallbackDefault : error when writing on pipe");
    }
  }



    
static void CallbackVol(w, null, reason)
    Widget w;
    char*  null;
    XmScaleCallbackStruct *reason;
{
  int value;
#ifdef DECSTATION
  char com[100];
#endif

  XmScaleGetValue(w, &value);
#ifdef SPARCSTATION
  soundplayvol(value);
#elif DECSTATION
  sprintf(com, "/usr/bin/aset -gain output %d", value);
  system(com);
#endif
}



static void CallbackPort(w, null, reason) 
     Widget w; 
     char *null;
     XmPushButtonCallbackStruct *reason;
 { 
   if(SPEAKER==TRUE){
     SPEAKER=FALSE;
     UnMappWidget(ButtonPort);
     ChangeButton(ButtonPort, "Headphones");
     sounddest(0);
     MappWidget(ButtonPort);
     return;
   }else{
     SPEAKER=TRUE; 	
     UnMappWidget(ButtonPort);
     ChangeButton(ButtonPort, "Speaker"); 
     sounddest(1);
     MappWidget(ButtonPort);
     return;
   }
 }
  

static void CallbackPlay(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {

    int id, len_data, delay;
    char data[P_MAX];
    int l1, l2, lread, aux;
    int type_packet;
    struct timeval timer;
    int mask, mask0;

    if(!AUDIO_DECODING){
      /* fork the audio decoding process */
      
#ifdef SPARCSTATION
      /* If the audio device is busy, don't fork any process */
      if(!soundinit(O_WRONLY)){
	fprintf(stderr, "audio_decode: Cannot open audio device\n");
      }else{
	soundterm();
#endif /* SPARCSTATION */
	if(pipe(da_fd) < 0){
	  fprintf(stderr, "audio_decode: Cannot create audio pipe\n");
	}else{
	  if((id=fork()) == -1){
	    fprintf(stderr,
		"Too many processes, Cannot fork audio decoding process\n");
	  }else{
	    if(!id){
	      close(da_fd[1]);
	      Audio_Decode(da_fd, AUDIO_REPLAY_PORT, IP_addr);
	    }
	    close(da_fd[0]);
	    AddStation(t, fullname, my_sin_addr.sin_addr.s_addr, 0, 0, &aux,
		       1);
	    t[0].audio_decoding = TRUE;
	    SendPP_NEW_TABLE(t, da_fd);
	    AUDIO_DECODING = TRUE;
	  }
	}
      }
    }
    if(!VIDEO_DECODING){
      /* fork the video decoding process */
      
      if(pipe(dv_fd) < 0){
	fprintf(stderr, "Cannot create pipe towards video decoding process\n");
      }else{
	if((id=fork()) == -1){
	  fprintf(stderr,
		  "Too many processes, Cannot fork decoding video process\n");
	}else{
	  if(!id){
	    close(dv_fd[1]);
	    VideoDecode(dv_fd, VIDEO_REPLAY_PORT, IP_addr, fullname, 
			my_sin_addr.sin_addr.s_addr, display, QCIF2CIF);
	  }
	  VIDEO_DECODING=TRUE;
	  InitScalesVideo();
	  close(dv_fd[0]);
	}
      }
    }
    if(AUDIO_DECODING || VIDEO_DECODING){

      if(pipe(fd) < 0){
	fprintf(stderr, "Cannot create pipe\n");
	Quit();
      }
      
      if((id=fork()) == -1){
	fprintf(stderr,
		"Too many processes, Cannot fork decoding ivs process\n");
	Quit();
      }else{
	if(!id){
	  close(fd[1]);
	  mask0 = 1 << fd[0];
	  timer.tv_sec = timer.tv_usec = 0;

	  /* Open the input ivs_file */
	  if((F_in=fopen(filename, "r")) == NULL){
	    fprintf(stderr, "Cannot open input file %s\n", filename);
	    Quit();
	  }
	  
	  do{
	    /* Get next header packet */

	    /* its length */
	    if((l1=getc(F_in)) == EOF)
	      break;
	    if((l2=getc(F_in)) == EOF)
	      break;
	    len_data = ((l1 & 0xff) << 8) + (l2 & 0xff);
	    /* delay before sending it */
	    if((l1=getc(F_in)) == EOF)
	      break;
	    if((l2=getc(F_in)) == EOF)
	      break;
	    delay = ((l1 & 0xff) << 8) + (l2 & 0xff);

	    /* read the packet */

	    if((lread=fread(data, 1, len_data, F_in)) != len_data){
	      DisplayMsgError("clip is not complete");
	      break;
	    }
	    type_packet = GetType(&data[0]);
	    mask = mask0;
	    if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		      &timer) > 0){
	      /* it's a PP_QUIT request */
	      exit(0);
	    }
	    Wait(delay);
	    if(AUDIO_DECODING && (type_packet == AUDIO_TYPE)){
	      if(sendto(sock_audio, data, len_data, 0, 
			(struct sockaddr *) &addr_audio, len_addr_audio) < 0){
		fprintf(stderr, "sendto audio packet failed");
		Quit();
	      }
	    }
	    if(VIDEO_DECODING && (type_packet == VIDEO_TYPE)){
	      if(sendto(sock_video, data, len_data, 0, 
			(struct sockaddr *) &addr_video, len_addr_video) < 0){
		fprintf(stderr, "sendto video packet failed");
		Quit();
	      }
	    }
	  }while(1);
	  fclose(F_in);
	  exit(0);
	}else{
	  close(fd[0]);
	  UnMappWidget(ButtonPlay);
	}
      }
    }
    return;
  }


static void CallbackQuit(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    Quit();
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
  Widget  appShell, main_win, MainForm, DialogShell, dwa, temp;
  Arg     args[10];
  int  n = 0;
  static char name[50]="IVS_REPLAY (1.0)";
  char *pt = name;
  XmString xms;
  int file_def=0, host_def=0, port_def=0;
  int narg;
  

  strcpy(display, "");

  if((arg < 2) || (arg > 3))
    Usage(argv[0]);
  narg = 1;
  while(narg != arg){
    if(strcmp(argv[narg], "-o") == 0){
      QCIF2CIF=FALSE;
      narg ++;
      continue;
    }else{
      strcpy(filename, argv[narg]);
      narg ++;
      continue;
    }
  }
    
  { /* Getting my sin_addr and my IP_addr */

    struct hostent *hp;
    int namelen=256;
    char hostname[256];

    if(gethostname(hostname, namelen) != 0){
      perror("gethostname");
      Quit();
    }
    if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "%d-Unknown host\n", getpid());
      Quit();
    }
    bcopy(hp->h_addr, (char *)&my_sin_addr.sin_addr, 4);
    strcpy(IP_addr, inet_ntoa(my_sin_addr.sin_addr));
    strcpy(fullname, "ivs_replay@");
    strcat(fullname, hostname);
  }


  /* Initializations */

#ifdef SPARCSTATION
  if(!sounddevinit()){
    fprintf(stderr, "Cannot opening audioctl device\n");
    Quit();
  }
#endif
  InitTable(t);
  sock_audio=CreateSendSocket(&addr_audio, &len_addr_audio, AUDIO_REPLAY_PORT, 
			      IP_addr, 0);
  sock_video=CreateSendSocket(&addr_video, &len_addr_video, VIDEO_REPLAY_PORT, 
			      IP_addr, 0);
  
  XtToolkitInitialize();
  app = XtCreateApplicationContext();
  disp = XtOpenDisplay(app, display,
		       "ivs_replay",   /* application name */
		       "VIDEOCONF",    /* application class */
		       NULL, 0,        /* options */
		       &arg, argv);    /* command line parameters */
  if(!disp){
    fprintf(stderr, "Unable to open display %s, trying unix:0\n", display);
    strcpy(display, "unix:0");
    disp = XtOpenDisplay(app, display,
			 "ivs_replay",   /* application name */
			 "VIDEOCONF",    /* application class */
			 NULL, 0,        /* options */
			 &arg, argv);    /* command line parameters */
    if(!disp){
      fprintf(stderr, "Cannot open display unix:0\n");
      Quit();
     }
  }
  n = 0;
  XtSetArg (args[n], XmNtitle, pt); n++;
  XtSetArg (args[n], XmNallowShellResize, TRUE); n++;
  appShell = XtAppCreateShell("VideoConf", "VIDEOCONF", 
			      applicationShellWidgetClass, disp, args, n);
    
  main_win = XmCreateMainWindow(appShell, "main_app", NULL, 0);

  n = 0;
  MainForm = XmCreateForm (main_win, "main", args, n);

  n = 0;
  DialogShell = XmCreateDialogShell(appShell, "Warning", args, n);

  n = 0;
  XtSetArg (args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  dwa = XmCreateDrawingArea (MainForm, "dwa", args, n);

  n = 0;
  XtSetArg (args[n], XmNdialogType, XmDIALOG_ERROR); n++;
  msg_err = XmCreateMessageBox(DialogShell, "MSG_ERR", args, n);
  temp = (Widget)XmMessageBoxGetChild(msg_err, XmDIALOG_CANCEL_BUTTON);
  XtUnmanageChild(temp);
  temp = XmMessageBoxGetChild(msg_err, XmDIALOG_HELP_BUTTON);
  XtUnmanageChild(temp);

  n = 0;
  XtSetArg (args[n], XmNx, 300); n++;
  XtSetArg (args[n], XmNy, 170); n++;
  ButtonQuit = XmCreatePushButton (dwa, "Quit", args, n);
  XtAddCallback (ButtonQuit, XmNactivateCallback, CallbackQuit, NULL);

  n = 0;
  XtSetArg (args[n], XmNx, 220); n++;
  XtSetArg (args[n], XmNy, 80); n++;
  ButtonPort = XmCreatePushButton (dwa, "Speaker", args, n);
  XtAddCallback (ButtonPort, XmNactivateCallback, CallbackPort, NULL);

  n = 0;
  XtSetArg (args[n], XmNx, 10); n++;
  XtSetArg (args[n], XmNy, 170); n++;
  ButtonPlay = XmCreatePushButton (dwa, "Push to play", args, n);
  XtAddCallback (ButtonPlay, XmNactivateCallback, CallbackPlay, NULL);

  n = 0;
  xms = XmStringCreate("Play Gain", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNtitleString, xms); n++;
  XtSetArg (args[n], XmNx, 220); n++;
  XtSetArg (args[n], XmNy, 10); n++;
  XtSetArg (args[n], XmNwidth, 100); n++;
#ifdef DECSTATION
  XtSetArg (args[n], XmNmaximum, 41); n++;
#endif
  XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg (args[n], XmNshowValue, True); n++;
  ScaleVol = XmCreateScale (dwa, "scaleVol", args, n);
  XmStringFree(xms);
  XtAddCallback (ScaleVol, XmNdragCallback, CallbackVol, NULL);
#ifdef SPARCSTATION
  XmScaleSetValue(ScaleVol, 50);
  soundplayvol(50);
#elif DECSTATION
  XmScaleSetValue(ScaleVol, 20);
#endif

  n = 0;
  xms = XmStringCreate("Brightness", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNtitleString, xms); n++;
  XtSetArg (args[n], XmNx, 40); n++;
  XtSetArg (args[n], XmNy, 10); n++;
  XtSetArg (args[n], XmNwidth, 100); n++;
  XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg (args[n], XmNshowValue, True); n++;
  XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  ScaleBri = XmCreateScale (dwa, "scaleBri", args, n);
  XmStringFree(xms);
  XtAddCallback (ScaleBri, XmNdragCallback, CallbackBri, NULL);

  n = 0;
  xms = XmStringCreate("Contrast", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNtitleString, xms); n++;
  XtSetArg (args[n], XmNx, 40); n++;
  XtSetArg (args[n], XmNy, 70); n++;
  XtSetArg (args[n], XmNwidth, 100); n++;
  XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg (args[n], XmNshowValue, True); n++;
  XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  ScaleCon = XmCreateScale (dwa, "scaleCon", args, n);
  XmStringFree(xms);
  XtAddCallback (ScaleCon, XmNdragCallback, CallbackCon, NULL);
    
  n = 0;
  xms = XmStringCreate("Default Colormap", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNlabelString, xms); n++;
  XtSetArg (args[n], XmNx, 15); n++;
  XtSetArg (args[n], XmNy, 130); n++;
  XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  ButtonDefault = XmCreatePushButton (dwa, "Default", args, n);
  XmStringFree(xms);
  XtAddCallback (ButtonDefault, XmNactivateCallback, CallbackDefault, NULL);


  XtManageChild (ButtonDefault);
  XtManageChild (ScaleCon);
  XtManageChild (ScaleBri);
  XtManageChild (ScaleVol);
  XtManageChild (ButtonPlay);
  XtManageChild (ButtonPort);
  XtManageChild (ButtonQuit);
  XtManageChild (dwa);
  XtManageChild (MainForm);
  XtManageChild (DialogShell);
  XtManageChild(main_win);
  XtRealizeWidget(appShell);

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, CallbackQuit);
  signal(SIGQUIT, CallbackQuit);

  XtAppMainLoop(app);

}

