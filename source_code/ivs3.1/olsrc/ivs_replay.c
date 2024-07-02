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
*  File              : ivs_replay.c    	                		   *
*  Last modification : 93/3/23   					   *
*  Author            : Thierry Turletti					   *
*--------------------------------------------------------------------------*
*  Description : Play an ivs encoded audio/video clip.                     *
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
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Dialog.h>

#include "h261.h"
#include "protocol.h"

#ifdef INDIGO
#define DefaultRecord      80
#else
#ifdef SPARCSTATION
#define DefaultRecord      20
#else
#ifdef DECSTATION
#define DefaultRecord      15
#else
#define DefaultRecord      40
#endif /* DECSTATION */
#endif /* SPARCSTATION */
#endif /* INDIGO */

#define DefaultVolume      50

#ifdef DECSTATION
#define MaxVolume          41
#define MaxRecord          36
#else
#define MaxVolume         100
#define MaxRecord         100
#endif

#define LabelSpeakerOn  "Loudspeaker"
#define LabelSpeakerOff "Headphones"

String fallback_resources[] = {
  "IVS_REPLAY*font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "IVS_REPLAY*Label.borderWidth:            0",
  "IVS_REPLAY*Label.background:             LightSteelBlue",
  "IVS_REPLAY*Form.borderWidth:             0",
  "IVS_REPLAY*Form.background:              LightSteelBlue",
  "IVS_REPLAY*Form1.borderWidth:            1",
  "IVS_REPLAY*Form2.borderWidth:            1",
  "IVS_REPLAY*MainForm.borderWidth:         0",
  "IVS_REPLAY*Command.background:           DarkSlateGrey",
  "IVS_REPLAY*Command.foreground:           MistyRose1",
  "IVS_REPLAY*LabelTitle.label:             IVS_REPLAY (3.1)",
  "IVS_REPLAY*LabelTitle.background:        Black",
  "IVS_REPLAY*LabelTitle.foreground:        MistyRose1",
  "IVS_REPLAY*LabelTitle.font: -adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
  "IVS_REPLAY*ButtonQuit.label:             Quit",
  "IVS_REPLAY*ButtonQuit.background:        Red",
  "IVS_REPLAY*ButtonQuit.foreground:        White",
  "IVS_REPLAY*ButtonPlay.label:             Push to play",
  "IVS_REPLAY*NameVolAudio.label:           Volume Gain:",
  "IVS_REPLAY*FormScale.borderWidth:        0",
  "IVS_REPLAY*ScrollScale.orientation:      horizontal",
  "IVS_REPLAY*ScrollScale.width:            100",
  "IVS_REPLAY*ScrollScale.height:           10",
  "IVS_REPLAY*ScrollScale.borderWidth:      4",
  "IVS_REPLAY*ScrollScale.minimumThumb:     4",
  "IVS_REPLAY*ScrollScale.thickness:        4",
  "IVS_REPLAY*ScrollScale.background:       LightSteelBlue4",
  "IVS_REPLAY*ScrollScale.foreground:       White",
  "IVS_REPLAY*NameScale.borderWidth:        0",
  "IVS_REPLAY*ValueScale.borderWidth:       0",
  "IVS_REPLAY*MsgError.background:          Red",
  "IVS_REPLAY*FormList.borderWidth:         1",
  NULL,
};




/* Global Widgets */

Widget main_form;
Widget menu_video[3]; /* not used here */
Pixmap mark; /* not used here */
Widget form_brightness, form_contrast, button_default; /* Not used */
Widget msg_error;

static XtAppContext app_con;
static Widget toplevel, form0, form1;
static Widget label_title;
static Widget form_v_audio, scroll_v_audio, name_v_audio, value_v_audio;
static Widget output_audio, button_play, button_quit;


LOCAL_TABLE t;
struct sockaddr_in addr_audio, addr_video;
int sock_audio, sock_video; 
int len_addr_audio, len_addr_video;
int da_fd[2], dv_fd[2], fd_s2f[2], fd_f2s[2];
u_char luth[256];
FILE *F_in;
BOOLEAN SPEAKER;
BOOLEAN QCIF2CIF=FALSE;
BOOLEAN AUDIO_DECODING=FALSE;
BOOLEAN VIDEO_DECODING=FALSE;
BOOLEAN UNICAST=TRUE;
struct sockaddr_in my_sin_addr;
char IP_addr[16];
char display[50], fullname[256], filename[256];
XtInputId id_reinit;

#ifdef HPUX
char audioserver[50]=NULL;
#endif


/***************************************************************************
 * Procedures
 **************************************************************************/

static void Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-double] clip\n", name);
  exit(1);
}



static void SetVolumeGain(value)
     int value;
{

#ifdef DECSTATION
  char com[100];

  sprintf(com, "/usr/bin/aset -gain output %d", value);
  system(com);
#else
  soundplayvol(value);
#endif

}



static void Quit()
{
  if(AUDIO_DECODING)
    SendPIP_QUIT(da_fd);
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
     int delay;
{
   struct timeval timer;
   timer.tv_sec = delay / 1000;
   timer.tv_usec = (delay % 1000) * 1000;
   (void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timer);
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
  
  val = (int)(MaxVolume*top);
  sprintf(value, "%d", val);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  SetVolumeGain(val);
}


#if defined(SPARCSTATION)  || defined(HPUX)
static XtCallbackProc CallbackAudioPort(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{
  if(SPEAKER==TRUE){
    SPEAKER=FALSE;
    ChangeLabel(output_audio, LabelSpeakerOff);
    sounddest(0);
    return;
  }else{
    SPEAKER=TRUE; 	
    ChangeLabel(output_audio, LabelSpeakerOn); 
    sounddest(1);
    return;
  }
}
#endif /* SPARCSTATION OR HP */


/*------------------------------------------------*\
*                 Others  callbacks                *
\*------------------------------------------------*/

static XtCallbackProc ReInit(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int statusp;

  XtRemoveInput(id_reinit);
  VIDEO_DECODING = FALSE;
  SendPIP_QUIT(dv_fd);
  close(fd_f2s[1]);
  close(fd_s2f[0]);
  waitpid(0, &statusp, WNOHANG);
  XtSetMappedWhenManaged(button_play, TRUE);
  return;
}


static XtCallbackProc CallbackPlay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
  {

    int id, len_data, delay;
    char data[P_MAX];
    int l1, l2, lread, aux;
    int type_packet;
    struct timeval timer;
    int mask, mask0;
    BOOLEAN STAT_MODE=FALSE;

    if(!AUDIO_DECODING){
      /* fork the audio decoding process */
      
#ifndef DECSTATION
#if defined(SPARCSTATION) || defined(INDIGO)
      /* If the audio device is busy, don't fork any process */
      if(!soundinit(O_WRONLY)){
	fprintf(stderr, "audio_decode: Cannot open audio device\n");
      }else{
	soundterm();
#else
      {
#endif
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
	    t[0].sin_addr = (u_long)my_sin_addr.sin_addr.s_addr;
	    t[0].audio_decoding = TRUE;
	    SendPIP_NEW_TABLE(t, 1, da_fd);
	    AUDIO_DECODING = TRUE;
	  }
	}
      }
#endif /* NOT DECSTATION */
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
			my_sin_addr.sin_addr.s_addr, display, QCIF2CIF,
			STAT_MODE, FALSE, 0);
	  }
	  VIDEO_DECODING=TRUE;
	  close(dv_fd[0]);
	}
      }
    }
    if(AUDIO_DECODING || VIDEO_DECODING){

      if(pipe(fd_f2s) < 0){
	fprintf(stderr, "Cannot create pipe\n");
	Quit();
      }
      
      if(pipe(fd_s2f) < 0){
	fprintf(stderr, "Cannot create pipe\n");
	Quit();
      }
      id_reinit = XtAppAddInput(app_con, fd_s2f[0], XtInputReadMask, 
				ReInit, NULL);
      if((id=fork()) == -1){
	fprintf(stderr,
		"Too many processes, Cannot fork decoding ivs process\n");
	Quit();
      }else{
	if(!id){
	  close(fd_f2s[1]);
	  close(fd_s2f[0]);
	  mask0 = 1 << fd_f2s[0];
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
	      ShowWarning("clip is not complete");
	      break;
	    }
	    if(((data[0] >> 6) == RTP_VERSION) && 
	       ((data[1] & 0x3f) == RTP_H261_CONTENT))
	      type_packet = VIDEO_TYPE;
	    else
	      type_packet = AUDIO_TYPE;

	    mask = mask0;
	    if(select(fd_f2s[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		      &timer) > 0){
	      /* it's a PIP_QUIT request */
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
	  }while(TRUE);
	  SendPIP_QUIT(fd_s2f);
	  close(fd_s2f[1]);
	  fclose(F_in);
	  exit(0);
	}else{
	  close(fd_f2s[0]);
	  close(fd_s2f[1]);
	  XtSetMappedWhenManaged(button_play, FALSE);
	}
      }
    }
    return;
  }


static XtCallbackProc CallbackQuit(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  Quit();
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

  if((argc < 2) || (argc> 3))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(strcmp(argv[narg], "-double") == 0){
      QCIF2CIF = TRUE;
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
      exit(1);
    }
    if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "%d-Unknown host\n", getpid());
      exit(1);
    }
    bcopy(hp->h_addr, (char *)&my_sin_addr.sin_addr, 4);
    strcpy(IP_addr, inet_ntoa(my_sin_addr.sin_addr));
    strcpy(fullname, "ivs_replay@");
    strcat(fullname, hostname);
  }


  /* Initializations */

  /* Audio initialization */

#ifdef SPARCSTATION
  if(!sounddevinit()){
    fprintf(stderr, "cannot opening audioctl device");
  }else{
    SPEAKER = soundgetdest() ? FALSE : TRUE;
  }
#else
  SPEAKER = TRUE;
#endif
#ifdef HPUX
  if(!audioInit(audioserver)){
    fprintf(stderr,"cannot connect to AudioServer\n");
  }
#endif


  InitTable(t);
  sock_audio = CreateSendSocket(&addr_audio, &len_addr_audio, 
				AUDIO_REPLAY_PORT, IP_addr, 0);
  sock_video = CreateSendSocket(&addr_video, &len_addr_video, 
				VIDEO_REPLAY_PORT, IP_addr, 0);

  toplevel = XtAppInitialize(&app_con, "IVS_REPLAY", NULL, 0, &argc, argv, 
			     fallback_resources, NULL, 0);


  /* 
  * Create a Form widget and put all children in that form widget.
  */

  main_form = XtCreateManagedWidget("MainForm", formWidgetClass, toplevel,
				    NULL, 0);
  /*
  *  Form 0.
  */
  n = 0;
  form0 = XtCreateManagedWidget("Form0", formWidgetClass, main_form,
				args, n);
  
  label_title = XtCreateManagedWidget("LabelTitle", labelWidgetClass, 
				      form0, NULL, 0);

  /*
  *  Form 1.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form0); n++;
  form1 = XtCreateManagedWidget("Form1", formWidgetClass, main_form,
				   args, n);
  /*
  *  Create the audio control utilities.
  */
  CreateScale(&form_v_audio, &scroll_v_audio, &name_v_audio, &value_v_audio,
	      form1, "FormVolAudio", "NameVolAudio", DefaultVolume);
  n = 0;
  XtSetValues(form_v_audio, args, n);
  XtAddCallback(scroll_v_audio, XtNjumpProc, CallbackVol, 
		(XtPointer)value_v_audio);
  SetVolumeGain(DefaultVolume);
  
#if defined(SPARCSTATION) || defined(HPUX)
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_v_audio); n++;
  if(SPEAKER){
    XtSetArg(args[n], XtNlabel, LabelSpeakerOn); n++;
  }else{
    XtSetArg(args[n], XtNlabel, LabelSpeakerOff); n++;
  }
  output_audio = XtCreateManagedWidget("ButtonOutAudio", commandWidgetClass, 
				       form1, args, n);
  XtAddCallback(output_audio, XtNcallback, CallbackAudioPort, NULL);
#endif /* SPARCSTATION OR HP */

  /*
  *  Create the Play and Quit buttons.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form0); n++;
  XtSetArg(args[n], XtNfromHoriz, form1); n++;
  button_play = XtCreateManagedWidget("ButtonPlay", commandWidgetClass,
				      main_form, args, n);
  XtAddCallback(button_play, XtNcallback, CallbackPlay, NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_play); n++;
  XtSetArg(args[n], XtNfromHoriz, form1); n++;
  button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
				      main_form, args, n);
  XtAddCallback(button_quit, XtNcallback, CallbackQuit, NULL);


  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app_con);

}

