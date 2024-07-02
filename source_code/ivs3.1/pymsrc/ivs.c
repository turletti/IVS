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
*  File              : ivs.c       	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 4/26/1993                                           *
*--------------------------------------------------------------------------*
*  Description       : Main file.                                          *
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
#include <arpa/inet.h>
#include <fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Dialog.h>

#ifdef VIDEOPIX
#include <vfc_lib.h>
#endif

#ifdef HPUX
char audioserver[50]=NULL;
#endif

#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "quantizer.h"
#include "protocol.h"

#define ecreteL(x) ((x)>255 ? 255 : (u_char)(x))
#define ecreteC(x) ((x)>255.0 ? 255 : ((x)<0 ? 0 :(u_char)(x)))
#define streq(a, b)        ( strcmp((a), (b)) == 0 )
#define maxi(x,y) ((x)>(y) ? (x) : (y))

#ifdef VIDEOPIX
#define DefaultBrightness  50
#else
#define DefaultBrightness  71
#endif
#define DefaultContrast    50

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

#define DEFAULT_QUANTIZER   3

#define DefaultVolume      50

#ifdef DECSTATION
#define MaxVolume          41
#define MaxRecord          36
#else

#define MaxVolume         100
#define MaxRecord         100
#endif

#define LoginFileName ".ivs.login"

#define LabelHOSTS   "List of participants"
#define LabelVIDEO   "Video"
#define LabelAUDIO   "Audio"

#define LabelHiden   "Hiding"
#define LabelListen  "Listening"
#define LabelShown   "Showing"
#define LabelMuted   "Muting"

#define LabelCallOn  "Call up"
#define LabelCallOff "Abort call"

#define LabelLocalOn  "Local display"
#define LabelLocalOff "No local display"

#define LabelFreezeOn  "Push to unfreeze"
#define LabelFreezeOff "Push to freeze"

#define LabelSpeakerOn  "Loudspeaker"
#define LabelSpeakerOff "Headphones"

#define LabelPushToTalk  "Push to talk"
#define LabelMicroOn     "Micro on "
#define LabelMicroOff    "Micro off"


#define Mark_width 15
#define Mark_height 15



static char Mark_bits[] = {
   0x00, 0x00, 0x80, 0x00, 0xc0, 0x01, 0xe0, 0x03, 0xf0, 0x07, 0xf8, 0x0f,
   0xfc, 0x1f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03,
   0xc0, 0x01, 0x80, 0x00, 0x00, 0x00};


void ActionMicro();

XtActionsRec actions[] = {
  {"ActionMicro",          (XtActionProc) ActionMicro},
};


String fallback_resources[] = {
  "*Label.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*Label.borderWidth:             0",
  "*Label.background:              LightSteelBlue",
  "*Form.borderWidth:              0",
  "*Form.background:               LightSteelBlue",
  "*Form1.borderWidth:             1",
  "*Form2.borderWidth:             1",
  "*Form3.borderWidth:             1",
  "*Form4.borderWidth:             1",
  "*Form4.background:              Black",
  "*MainForm.borderWidth:          0",
  "*menu.background:               MistyRose1",
  "*Command.background:            DarkSlateGrey",
  "*Command.foreground:            MistyRose1",
  "*MenuButton.background:         MistyRose1",
  "*MenuButton.foreground:         DarkSlateGrey",
  "*LabelTitle.background:         Black",
  "*LabelTitle.foreground:         MistyRose1",
  "*LabelTitle.font: -adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
  "*NameBrightness.label:          Brightness:",
  "*NameBrightness.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*NameContrast.label:            Contrast:",
  "*NameContrast.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*ButtonDefault.label:           Default",
  "*ButtonLocal.label:             No Local display",
  "*ButtonFreeze.label:            Push to freeze",
  "*ButtonFreeze.width:            120",
  "*ButtonControl.label:           Rate control",
  "*ButtonEVideo.label:            Encode video",
  "*ButtonEAudio.label:            Encode audio",
  "*ButtonQuit.label:              Quit",
  "*ButtonQuit.background:         Red",
  "*ButtonQuit.foreground:         White",
  "*ButtonMicro.Translations:      #override   \\n\
          <EnterWindow>:        	highlight() \\n\
          <LeaveWindow>:        	reset()     \\n\
          <Btn1Down>:	  		ActionMicro() unset() \\n\
          <Btn1Up>: 	 		ActionMicro() unset()",
  "*ButtonMicro.width:             95",
  "*ButtonOutAudio.width:          90",
  "*NameControl.label:             Max bandwidth:",
  "*NameControl.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*FormBandwidth.background:      White",
  "*FormBandwidth.resizable:       True",
  "*LabelB1.label:                 Bandwidth :",
  "*LabelB2.label:                 _____",
  "*LabelB3.label:                 Kbit/s",
  "*LabelR1.label:                 Frame rate:",
  "*LabelR2.label:                 ____",
  "*LabelR3.label:                 image/s",
  "*LabelB*.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*NameVolAudio.label:            Volume Gain:",
  "*NameRecAudio.label:            Record Gain:",
  "*ButtonOptAudio.label:          Audio encoding",
  "*ButtonPortVideo.label:         Video input port",
  "*ButtonFormatVideo.label:       Video format",
  "*FormScale.borderWidth:         0",
  "*ScrollScale.orientation:       horizontal",
  "*ScrollScale.width:             100",
  "*ScrollScale.height:            10",
  "*ScrollScale.borderWidth:       4",
  "*ScrollScale.minimumThumb:      4",
  "*ScrollScale.thickness:         4",
  "*ScrollScale.background:        LightSteelBlue4",
  "*ScrollScale.foreground:        White",
  "*NameScale.borderWidth:         0",
  "*NameScale.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*ValueScale.borderWidth:       0",
  "*ValueScale.font: -adobe-courier-bold-r-normal--11-80-100-100-m-60-iso8859-1",
  "*LabelHosts.background:        Black",
  "*LabelHosts.foreground:        MistyRose1",
  "*LabelHosts.font: -adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
  "*LabelHosts.width:             278",
  "*LabelVideo.background:        Black",
  "*LabelVideo.foreground:        MistyRose1",
  "*LabelVideo.font: -adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
  "*LabelVideo.width:             60",
  "*LabelAudio.background:        Black",
  "*LabelAudio.foreground:        MistyRose1",
  "*LabelAudio.font: -adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
  "*LabelAudio.width:             70",
  "*LabelConf.font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "*SimpleMenu*leftMargin:        15",
  "*MsgError.background:          Red",
  "*FormList.borderWidth:         1",
  "*HostName.width:               270",
  "*HostName.font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "*HostName.background:          White",
  "*MyHostName.width:             270",
  "*MyHostName.font: -adobe-courier-bold-r-normal--14-100-100-100-m-90-iso8859-1",
  "*MyHostName.background:        MistyRose1",
  "*ButtonDAudio.internalWidth:   2",
  "*ButtonDVideo.internalWidth:   2",
  "*ButtonDAudio.internalHeight:  2",
  "*ButtonDVideo.internalHeight:  2",
  "*ButtonDAudio.width:           70",
  "*ButtonDVideo.width:           60",
  "*Showing.background:           Green",
  NULL,
};


#ifdef VIDEOPIX
static char *VideoPixMenuItems[] = {
  "Video port 1", "Video port 2", "Video port SVideo", 
  "Automatic selection"
  };
#endif

static char *VideoFormatMenuItems[] = {
 "Small size (QCIF)", "Normal size (CIF)", "Large size (SCIF)",
 "Grey mode", "Color mode"
  };

static char *ControlMenuItems[] = {
  "Without any control", "Privilege quality", "Privilege frame rate",
  "Accept feedback", "Avoid feedback"
  };

static char *AudioMenuItems[] = {
  "PCM (64 kb/s)", "ADPCM (32 kb/s)", "ADPCM (10-30 kb/s)"
  };



/* Global Variables */

Widget main_form;
Widget form_brightness, form_contrast, button_default;
Widget menu_video[5];
Widget msg_error;
BOOLEAN UNICAST=FALSE;
Pixmap mark;



static XtAppContext app_con;

static Widget toplevel, form0, form1, form2, form3, form4, form5;
static Widget form2_1, form3_1;

static Widget label_title;
static Widget scroll_brightness, name_brightness, value_brightness, 
  scroll_contrast, name_contrast, value_contrast, button_local, button_freeze;
static Widget form_control, scroll_control, name_control,
  value_control;
static Widget form_bandwidth, label_band1, label_band2, label_band3;
static Widget form_refresh, label_ref1, label_ref2, label_ref3;
static Widget form_v_audio, scroll_v_audio, name_v_audio, value_v_audio,
  form_r_audio, scroll_r_audio, name_r_audio, value_r_audio, output_audio;
static Widget button_e_audio, button_e_video;
static Widget button_port, menu_port, button_format, menu_format, 
  button_control, menu_control, button_audio, menu_audio;
static Widget menu_videopix[4], menu_rate[5], menu_eaudio[3];
static Widget button_micro, button_quit, button_ring;
static Widget label_conf, label_hosts, label_video, label_audio;
static Widget view_list, form_list;
static Widget host_name[N_MAX_STATIONS];
static Widget button_d_video[N_MAX_STATIONS], button_d_audio[N_MAX_STATIONS];

static LOCAL_TABLE t;
static char display[50];
static char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
static char name_user[L_cuserid];
static struct sockaddr_in addr_s, addr_r, addr_dist;
static int csock_s, csock_r; 
static int len_addr_s, len_addr_r;
static int ea_fd[2], evf2s_fd[2], evs2f_fd[2], da_fd[2], 
           dv_fd[N_MAX_STATIONS][2];
static int fd_answer[2], fd_call[2];
static int *scaledv_fd;
static u_char luth[256];
static u_char ttl=DEFAULT_TTL;
static FILE *F_log;
static int statusp;
static int T_ALARM=10;     /* a Declaration packet is sent every T_ALARM 
			      seconds */
static long my_sin_addr;
static u_short my_feedback_port=PORT_VIDEO_CONTROL;
static int size;           /* Image size: CIF_SIZE, CIF4_SIZE or QCIF_SIZE */
static int audio_format;
static int rate_control;   /* WITHOUT_CONTROL or REFRESH_CONTROL or 
			      QUALITY_CONTROL */
static int vfc_port_video; /* Videopix input port video */
static int rate_max=100;   /* Maximum bandwidth allowed (kb/s) */
static XtInputId id_input; /* ID returned from the video coder 
			      XtAppAddInput call. */
static XtInputId id_call_input; /* ID returned from the Call up 
				   XtAppAddInput call. */
static int nb_stations=0;  /* Current number of stations in the group */
static int pip[N_MAX_STATIONS]; 
static int first_passive=1; /* Index of the first passive station in the 
			       list */
static int brightness=50, contrast=50; /* Current encoding tuning values */
static int default_quantizer=DEFAULT_QUANTIZER;

#ifdef SPARCSTATION
static int squelch_level=75; /* Current squelch level for audio rec */
#else
static int squelch_level=30; /* Current squelch level for audio rec */
#endif
static BOOLEAN START_EAUDIO=FALSE, START_EVIDEO=FALSE, SPEAKER;
static BOOLEAN AUDIO_SET=FALSE;
static BOOLEAN QCIF2CIF=FALSE;
static BOOLEAN COLOR; /* True if color encoding */
static BOOLEAN LOCAL_DISPLAY=FALSE;
static BOOLEAN IMPLICIT_AUDIO_DECODING=TRUE; /* True if implicite audio 
						decoding. */
static BOOLEAN IMPLICIT_VIDEO_DECODING=TRUE; /* True if implicite video 
						decoding. */
static BOOLEAN RESTRICT_SAVING=FALSE; /* True if only encoders stations 
					 are saved in the local table. */
static BOOLEAN FREEZE=FALSE; /* True if image frozen */
static BOOLEAN RING_UP=FALSE;
static BOOLEAN FEEDBACK=TRUE; /* True if feedback from video decoder is
				 allowed. */
static BOOLEAN PUSH_TO_TALK=TRUE;
static BOOLEAN MICRO_MUTED;
static BOOLEAN STAT_MODE=FALSE; /* True if STAT mode set */
static BOOLEAN FORCE_GREY=FALSE; /* True if only grey display */

/* from athena.c module */

extern void DestroyWarning(), ShowWarning(), MarkMenu(), UnMarkMenu(),
  ChangeLabel(), ChangeAndMappLabel(), ReverseColor(), 
  SetSensitiveVideoTuning(), CreateScale(), InitScale(); 
  


/***************************************************************************
 * Procedures
 **************************************************************************/


void Usage(name)
     char *name;
{
  fprintf(stderr, IVS_VERSION);
  fprintf(stderr, "\n\n");
#ifndef HPUX
  fprintf(stderr, "Usage: %s [-I @IP] [-N user] [-T ttl] [-S squelch_level] [-d display] [-double] [-r] [-a] [-v] [-dp] [-stat] [-grey] [-q default_quantizer] [hostname]\n", name);
#else
  fprintf(stderr, 
"Usage: %s [-I @IP] [-N user] [-T ttl] [-as audio] [-d display] [-double] [-r] [-a] [-stat] [hostname]\n", name);
#endif  
  fprintf(stderr, "\t -I Multicast Internet group address style 224.8.8.8\n");
  fprintf(stderr, 
"\t -N user name: this name will appear on the control window panel\n");
  fprintf(stderr, "\t -T time-to-live value\n");
  fprintf(stderr, "\t -S squelch_level\n");
#ifdef HPUX
  fprintf(stderr, "\t -as audio: set audioserver to audio\n");
#endif
  fprintf(stderr, "\t -d display as machine:0.0\n");
  fprintf(stderr, 
	  "\t -double: a QCIF image will be display in a CIF display.\n");
  fprintf(stderr, "\t -r: only the encoders stations will appear on the control panel window\n");
  fprintf(stderr, "\t -a: disable the implicit audio decoding mode.\n");
  fprintf(stderr, "\t -v: disable the implicit video decoding mode.\n");
  fprintf(stderr, "\t -dp: disable the Push To Talk button.\n");
  fprintf(stderr, "\t -stat: set ivs in a statistic mode\n");
  fprintf(stderr, "\t -grey: force grey decoding display\n");
  fprintf(stderr, 
	  "\t -q default_quantizer: force the default quantizer value\n");
  fprintf(stderr, "\t hostname: set ivs in unicast mode;\n");
  fprintf(stderr, "hostname can be a IP address style 138.3.3.3\n");
  exit(1);
}


static void SetVolumeGain(value)
     int value;
{

#ifdef DECSTATION
#ifdef DECAUDIO
  char com[100];

  sprintf(com, "/usr/bin/aset -gain output %d", value);
  system(com);
#endif /* DECAUDIO */
#else
  soundplayvol(value);
#endif

}


static void SetRecordGain(value)
     int value;
{

#ifdef DECSTATION
#ifdef DECAUDIO
  char com[100];

  sprintf(com, "/usr/bin/aset -gain input %d", value);
  system(com);
#endif /* DECAUDIO */
#else
  soundrecgain(value);
#endif

}


static void SetSensitiveVideoMenu(value)
     BOOLEAN value;
{
  register int i;

#ifdef VIDEOPIX
  for(i=0; i<5; i++)
#else
  for(i=0; i<2; i++)
#endif
    XtSetSensitive(menu_video[i], value);
}





static BOOLEAN InitDAudio(fd)
     int fd[2];
{
  int id;

#ifndef DECSTATION
#ifndef NOHPAUDIO

#if defined(SPARCSTATION) || defined(INDIGO)
  if(!soundinit(O_WRONLY)){
    /* If the audio device is busy, don't fork any process */
    ShowWarning("audio_decode: Cannot open audio device");
    return(FALSE);
  }
  /* Close the output audio device, it will be open by the son process. */
  soundterm();
#endif /* SPARCSTATION OR INDIGO */
  if(pipe(fd) < 0){
    ShowWarning("audio_decode: Cannot create pipe");
    return(FALSE);
  }
  if((id=fork()) == -1){
    ShowWarning("Too many processes, cannot fork audio decoding process");
    return(FALSE);
  }
  if(!id){
    close(fd[1]);
    Audio_Decode(fd, AUDIO_PORT, IP_GROUP);
  }else{
    close(fd[0]);
    return(TRUE);
  }
#else
  ShowWarning("Sorry, no hp audio server support");
#endif /* NOT NOHPAUDIO */
#else
  ShowWarning("Sorry, DEC audio doesn't work yet ...");
#endif /* NOT DECSTATION */
  return(FALSE);
}



static BOOLEAN InitDVideo(fd, i)
     int fd[2];
     int i; /* Station's index in the local table */

{
  int idv;
  register int n;

  if(pipe(fd) < 0){
    ShowWarning("InitDVideo: Cannot create pipe");
    return;
  }
  if((idv=fork()) == -1){
    ShowWarning("InitDVideo: Cannot fork decoding video process");
    return;
  }
  if(!idv){
    close(fd[1]);
    VideoDecode(fd, VIDEO_PORT, IP_GROUP, t[i].name, t[i].sin_addr, display, 
		QCIF2CIF, STAT_MODE, FORCE_GREY, t[i].feedback_port);
  }else{
    close(fd[0]);
    t[i].video_decoding=TRUE;
    ChangeLabel(button_d_video[i], LabelShown);
    scaledv_fd = fd;

    for(n=0; n<256; n++)
      luth[n] = ecreteL((int)((255*n)/150));
  }
}



static void AddLoginName(name)
     char *name;
{
  if((F_log = fopen(LoginFileName, "a")) == NULL){
    fprintf(stderr, "cannot open login file\n");
    STAT_MODE = FALSE;
  }else{
    fprintf(F_log, "\t %s\n", name);
    fclose(F_log);
  }
}



static void CheckStations(t, time)
     LOCAL_TABLE t;
     int time;
{
  register int i;
  STATION *p;
  
  for(i=1; i<nb_stations; i++){
    p = &(t[i]);
    if(p->valid){
      if((time - p->lasttime) > 100){
#ifdef DEBUG
	fprintf(stderr, 
		"Station %s not responding, removed it from local table\n",
		p->name);
#endif
	p->valid = FALSE;
	XtSetSensitive(host_name[i], FALSE);
	if(p->video_encoding){
	  p->video_encoding = FALSE;
	  XtSetMappedWhenManaged(button_d_video[i], FALSE);
	}
	if(p->audio_encoding){
	  p->audio_encoding = FALSE;
	  XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	}
	if(p->video_decoding){
	  p->video_decoding = FALSE;
	  SendPIP_QUIT(dv_fd[pip[i]]);
	  close(dv_fd[pip[i]][1]);
	}
	if(p->audio_decoding){
	  p->audio_decoding = FALSE;
	  SendPIP_NEW_TABLE(t, first_passive, da_fd);
	}
      }
    }
  }
}



static void LoopDeclare()
{
  struct timeval realtime;
  double time;
  static int cpt=0;
  
  gettimeofday(&realtime, (struct timezone *)NULL);
  time = realtime.tv_sec;
  if(cpt % 6 == 5)
      CheckStations(t, (int)time); /* Every minute */
  SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
	     my_feedback_port, my_sin_addr, name_user);
  alarm((u_int)T_ALARM);
  signal(SIGALRM, LoopDeclare);
  cpt ++;
}



static void ManageStation(t, name, sin_addr, feedback_port, VIDEO_ENCODING, 
			  AUDIO_ENCODING, MYSELF)
     LOCAL_TABLE t;
     char *name;
     u_long sin_addr;
     u_short feedback_port;
     BOOLEAN VIDEO_ENCODING, AUDIO_ENCODING, MYSELF;
{
  static BOOLEAN FIRST_CALL=TRUE;
  STATION *np, np_memo;
  struct timeval realtime;
  double time;
  BOOLEAN FOUND=FALSE;
  BOOLEAN DECALAGE=TRUE;
  BOOLEAN ABORT_DAUDIO=FALSE;
  BOOLEAN ABORT_DVIDEO=FALSE;
  BOOLEAN ACTIVE;
  register int i;
  int i_found;
  
  if(FIRST_CALL  && !MYSELF){
    /* The first station should be myself ... */
    return;
  }
  if(MYSELF){
    np = &t[0];
    if(FIRST_CALL){
      nb_stations++;
      InitStation(np, name, sin_addr, feedback_port, FALSE, FALSE);
      ChangeAndMappLabel(host_name[0], name);
      FIRST_CALL = FALSE;
      if(STAT_MODE)
	AddLoginName(name);
    }else{
      if(np->audio_encoding != AUDIO_ENCODING){
	if(!AUDIO_ENCODING){
	  if(np->audio_decoding){
	    np->audio_decoding = FALSE;
	    SendPIP_NEW_TABLE(t, first_passive, da_fd);
	  }
	  XtSetMappedWhenManaged(button_d_audio[0], FALSE);
	}else{
	  ChangeAndMappLabel(button_d_audio[0], LabelMuted);
	}/* NOT AUDIO_ENCODING */
	np->audio_encoding = AUDIO_ENCODING;
      }/* AUDIO_ENCODING CHANGE */
      if(feedback_port != t[0].feedback_port){
	t[0].feedback_port = feedback_port;
	if(np->video_decoding){
	  SendPIP_FEEDBACK(dv_fd[0], feedback_port);
	}
      }
      if(np->video_encoding != VIDEO_ENCODING){
	if(!VIDEO_ENCODING){
	  if(np->video_decoding){
	    np->video_decoding = FALSE;
	    SendPIP_QUIT(dv_fd[0]);
	    close(dv_fd[0][1]);
	  }
	  XtSetMappedWhenManaged(button_d_video[0], FALSE);
	}else{
	  ChangeAndMappLabel(button_d_video[0], LabelHiden);
	}/* NOT VIDEO_ENCODING */
	np->video_encoding = VIDEO_ENCODING;
      }/* VIDEO_ENCODING CHANGE */
    }/* FIRST_CALL */
  }else{
    /* It is not my declaration */
    gettimeofday(&realtime, (struct timezone *)NULL);
    time = realtime.tv_sec;
    ACTIVE = VIDEO_ENCODING | AUDIO_ENCODING;
    if(ACTIVE){
      /*---------------------------*\
      *  Active station declaration *
      \*---------------------------*/
      for(i=1; i<first_passive; i++){
	np = &(t[i]);
	if(streq(np->name, name)){
	  /* This station is a known active station */
	  FOUND=TRUE;
	  if(!np->valid){
	    np->valid = TRUE;
	    XtSetSensitive(host_name[i], TRUE);
	  }
	  np->lasttime = (int) time;
	  if(np->audio_encoding != AUDIO_ENCODING){
	    if(!AUDIO_ENCODING){
	      /* The station stopped its audio emission */
	      if(np->audio_decoding){
		np->audio_decoding = FALSE;
		SendPIP_NEW_TABLE(t, first_passive, da_fd);
	      }
	      XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	    }else{
	      /* The station started an audio emission */
	      if(!IMPLICIT_AUDIO_DECODING){
		ChangeAndMappLabel(button_d_audio[i], LabelMuted);
	      }else{
		if(!AUDIO_SET){
		  AUDIO_SET=InitDAudio(da_fd);
		}
		if(AUDIO_SET){
		  np->audio_decoding = TRUE;
		  SendPIP_NEW_TABLE(t, first_passive+1, da_fd);
		  ChangeAndMappLabel(button_d_audio[i], LabelListen);
		}
	      }
	    }/* NOT AUDIO ENCODING */
	    np->audio_encoding = AUDIO_ENCODING;
	  }/* AUDIO_ENCODING CHANGE */
	  if(feedback_port != t[i].feedback_port){
	    t[i].feedback_port = feedback_port;
	    if(np->video_decoding){
              SendPIP_FEEDBACK(dv_fd[pip[i]], feedback_port);
	    }
	  }
	  if(np->video_encoding != VIDEO_ENCODING){
	    if(!VIDEO_ENCODING){
	      /* The station stopped its video emission */
	      if(np->video_decoding){
		np->video_decoding = FALSE;
		SendPIP_QUIT(dv_fd[pip[i]]);
		close(dv_fd[pip[i]][1]);
	      }
	      XtSetMappedWhenManaged(button_d_video[i], FALSE);
	    }else{
	      if(IMPLICIT_VIDEO_DECODING){
		pip[i] = i;
		if(InitDVideo(dv_fd[i], i)){
		  ChangeAndMappLabel(button_d_video[i], LabelShown);
		  np->video_decoding = TRUE;
		}else
		  ChangeAndMappLabel(button_d_video[i], LabelHiden); 
	      }else{
		ChangeAndMappLabel(button_d_video[i], LabelHiden);
	      }		
	    }/* NOT VIDEO_DECODING */
	    np->video_encoding = VIDEO_ENCODING;
	  }/* VIDEO_ENCODING CHANGE */
	  return;
	}/* Known active station declaration */
      }/* for() */
      if(!FOUND){
	/* This is an unknown active station declaration, i==first_passive. */
	np = &(t[i]);
	if(i == nb_stations){
	  /* There is no passive station, so decalage is useless. */
	  DECALAGE = FALSE;
	  nb_stations ++;
	  if(STAT_MODE)
	    AddLoginName(name);
	}else{
	  if(strcmp(np->name, name) != 0){
	    /* Decalage is necessary */
	    DECALAGE=TRUE;
	    nb_stations ++;
	    CopyStation(&np_memo, np);
	  }else{
	    /* Decalage is not necessary */
	    DECALAGE=FALSE;
	  }
	}
	/* Copy the station at this place */
	pip[i] = i;
	ChangeAndMappLabel(host_name[i], name);
	if(np->valid == FALSE)
	  XtSetSensitive(host_name[i], TRUE);
	InitStation(np, name, sin_addr, feedback_port, AUDIO_ENCODING, 
		    VIDEO_ENCODING);
	np->lasttime = (int)time;
	if(VIDEO_ENCODING){
	  if(IMPLICIT_VIDEO_DECODING){
	    if(InitDVideo(dv_fd[i], i)){
	      ChangeAndMappLabel(button_d_video[i], LabelShown);
	      np->video_decoding = TRUE;
	    }else
	      ChangeAndMappLabel(button_d_video[i], LabelHiden); 
	  }else{
	    ChangeAndMappLabel(button_d_video[i], LabelHiden);
	  }		
	}
	if(AUDIO_ENCODING){
	  if(IMPLICIT_AUDIO_DECODING){
	    if(!AUDIO_SET){
	      AUDIO_SET=InitDAudio(da_fd);
	    }
	    if(AUDIO_SET){
	      np->audio_decoding = TRUE;
	      SendPIP_NEW_TABLE(t, first_passive+1, da_fd);
	      ChangeAndMappLabel(button_d_audio[i], LabelListen);
	    }
	  }else{
	    ChangeAndMappLabel(button_d_audio[i], LabelMuted);
	  }
	}/* AUDIO_ENCODING */
	first_passive ++;
	if(DECALAGE){
	  /* Insert the np_memo station either at the end of the list
          *  or at the position of the passive station if existed.
	  */
	  for(++i; i<nb_stations-1; i++){
	    np = &(t[i]);
	    if(streq(np->name, name)){
	      FOUND=TRUE;
	      break;
	    }
	  }
	  ChangeAndMappLabel(host_name[i], np_memo.name);
	  CopyStation(&t[i], &np_memo);
	  pip[i] = i;
	  if(np_memo.valid == FALSE)
	    XtSetSensitive(host_name[i], FALSE);
	  if(FOUND)
	    nb_stations --;
	  else
	    if(STAT_MODE)
	      AddLoginName(name);
	}/* DECALAGE */
      }/* NOT FOUND */
    }else{
      /*----------------------------*\
      *  Passive station declaration *
      \*----------------------------*/
      for(i=1; i<first_passive; i++){
	np = &t[i];
	if(streq(np->name, name)){
	  /* This station becomes now a passive station.
	   *  We have to put it first after the last active station.
	   *  The following active stations have to move upstairs...
	   */
	  FOUND=TRUE;
	  i_found = pip[i];
	  break;
	}
      }/* for() */
      if(FOUND){
	/* Move upstairs the following active stations */
	first_passive --;
	for(; i<first_passive; i++){
	  if(t[i].valid != t[i+1].valid){
	    if(t[i+1].valid)
	      XtSetSensitive(host_name[i], TRUE);
	    else
	      XtSetSensitive(host_name[i], FALSE);
	  }
	  CopyStation(&t[i], &t[i+1]);
	  pip[i] = pip[i+1];
	  ChangeAndMappLabel(host_name[i], t[i].name);
	  if(t[i].audio_encoding){
	    if(t[i].audio_decoding)
	      ChangeAndMappLabel(button_d_audio[i], LabelListen);
	    else
	      ChangeAndMappLabel(button_d_audio[i], LabelMuted);
	    XtSetMappedWhenManaged(button_d_audio[i], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	  if(t[i].video_encoding){
	    if(t[i].video_decoding)
	      ChangeAndMappLabel(button_d_video[i], LabelShown);
	    else
	      ChangeAndMappLabel(button_d_video[i], LabelHiden);
	    XtSetMappedWhenManaged(button_d_video[i], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_video[i], FALSE);
	}/* for() */
	/* Insert here the passive station */
	ABORT_DAUDIO = t[i].audio_decoding;
	ABORT_DVIDEO = t[i].video_decoding;
	InitStation(&t[i], name, sin_addr, feedback_port, FALSE, FALSE);
	np->lasttime = (int)time;
	pip[i] = i;
	if(ABORT_DAUDIO){
	  /* The station stopped its audio emission and we have to
	   *  stop its decoding.
	   */
	  SendPIP_NEW_TABLE(t, first_passive, da_fd);
	}
	if(ABORT_DVIDEO){
	  /* The station stopped its video emission and we have to
	   *  stop its decoding.
	   */
	  SendPIP_QUIT(dv_fd[i_found]);
	  close(dv_fd[i_found][1]);
	}	  
	XtSetSensitive(host_name[i], TRUE);
	XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	XtSetMappedWhenManaged(button_d_video[i], FALSE);
	ChangeAndMappLabel(host_name[i], name);
	return;
      }else{
	/* This station was not a known active station. */ 
	for(; i<nb_stations; i++){
	  np = &(t[i]);
	  if(streq(np->name, name)){
	    FOUND=TRUE;
	    break;
	  }
	}
	if(FOUND){
	  /* This station is a known passive station. */
	  if(!np->valid){
	    np->valid = TRUE;
	    XtSetSensitive(host_name[i], TRUE);
	  }
	  np->lasttime = (int)time;
	  return;
	}else{
	  /* This station is a new passive station */
	  np = &(t[nb_stations]);
	  InitStation(np, name, sin_addr, feedback_port, FALSE, FALSE);
	  np->lasttime = (int)time;
	  XtSetSensitive(host_name[i], TRUE);
	  ChangeAndMappLabel(host_name[nb_stations], name);
	  nb_stations ++;
	  if(STAT_MODE)
	    AddLoginName(name);
	  return;
	}/* FOUND */
      }/* FOUND */
    }/* active station */
  }/* MYSELF */
  return;
}




static void RemoveStation(t, sin_addr)
     LOCAL_TABLE t;
     u_long sin_addr;
     
{
  register int i;
  STATION *p;
  BOOLEAN FOUND=FALSE;
  
  for(i=1; i<nb_stations; i++){
    p = &(t[i]);
    if(p->sin_addr == sin_addr){
      FOUND = TRUE;
      if(p->video_decoding){
	SendPIP_QUIT(dv_fd[pip[i]]);
	close(dv_fd[pip[i]][1]);
      }
      if(p->audio_decoding){
	p->audio_decoding = FALSE;
	SendPIP_NEW_TABLE(t, first_passive, da_fd);
      }
      break;
    }
  }
  if(i < first_passive)
    first_passive --;
  if(FOUND){
    nb_stations --;
    for(; i<nb_stations; i++){
      if(t[i].valid != t[i+1].valid){
	if(t[i+1].valid)
	  XtSetSensitive(host_name[i], TRUE);
	else
	  XtSetSensitive(host_name[i], FALSE);
      }
      CopyStation(&t[i], &t[i+1]);
      pip[i] = pip[i+1];
      ChangeAndMappLabel(host_name[i], t[i].name);
      if(t[i].audio_encoding){
	if(t[i].audio_decoding)
	  ChangeAndMappLabel(button_d_audio[i], LabelListen);
	else
	  ChangeAndMappLabel(button_d_audio[i], LabelMuted);
	XtSetMappedWhenManaged(button_d_audio[i], TRUE);
      }else
	XtSetMappedWhenManaged(button_d_audio[i], FALSE);
      if(t[i].video_encoding){
	if(t[i].video_decoding)
	  ChangeAndMappLabel(button_d_video[i], LabelShown);
	else
	  ChangeAndMappLabel(button_d_video[i], LabelHiden);
	XtSetMappedWhenManaged(button_d_video[i], TRUE);
      }else
	XtSetMappedWhenManaged(button_d_video[i], FALSE);
    }
    ResetStation(&t[i]);
    XtSetMappedWhenManaged(host_name[i], FALSE);
    XtSetMappedWhenManaged(button_d_audio[i], FALSE);
    XtSetMappedWhenManaged(button_d_video[i], FALSE);
  }else
    fprintf(stderr, "Received a unknown RemoveHost Packet \n");
}



static void Quit()
{
  register int i;

  if(STAT_MODE){
    char date[30];
    time_t a_date;

    if((F_log = fopen(LoginFileName, "a")) == NULL){
      fprintf(stderr, "cannot open login file\n");
    }else{
      a_date = time((time_t *)0);
      strcpy(date, ctime(&a_date)); 
      fprintf(F_log, "\nEnd of conference : %s\n", date);
      fclose(F_log);
    }
  }
  /* Inform the group that we abort our ivs session and Kill all
  *  the son processes.
  */
  SendBYE(csock_s, &addr_s, len_addr_s);
  if(AUDIO_SET)
    SendPIP_QUIT(da_fd);
  for(i=0; i<nb_stations; i++)
    if(t[i].video_decoding)
      SendPIP_QUIT(dv_fd[pip[i]]);
  if(START_EAUDIO)
    SendPIP_QUIT(ea_fd);
  if(START_EVIDEO){
    XtRemoveInput(id_input);
    SendPIP_QUIT(evf2s_fd);
  }
#ifdef HPUX
    audioTerm();
#endif /* HPUX */
  XtDestroyApplicationContext(app_con);
  exit(0);
}




/**************************************************
 * Callbacks
 **************************************************/


/*------------------------------------------------*\
*               IVS-TALK  callbacks                *
\*------------------------------------------------*/

static XtCallbackProc ShowAnswerCall(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  Arg         args[5];
  Widget      popup, dialog;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  char        msg[100];
  int         lr;

  RING_UP = FALSE;
  ChangeAndMappLabel(button_ring, LabelCallOn);
  XtRemoveInput(id_call_input);
  bzero(msg, 100);
  if((lr=read(fd_answer[0], msg, 100)) < 0){
    ShowWarning("ShowAnswerCall: read failed");
    return;
  }

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);
  
  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;
  
  popup = XtCreatePopupShell("Answer", transientShellWidgetClass, main_form,
			     args, n);
  
  n = 0;
  XtSetArg(args[n], XtNlabel, msg); n++;
  msg_error = XtCreateManagedWidget("MsgError", dialogWidgetClass, popup,
				    args, n);
  
  XawDialogAddButton(msg_error, "OK", DestroyWarning, (XtPointer) msg_error);
  XtPopup(popup, XtGrabNone);
}



static XtCallbackProc CallbackRing(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  int id;
  
  if(!RING_UP){

    if(pipe(fd_call) < 0){
      ShowWarning("CallbackRing: Cannot create pipe");
      return;
    }
    if(pipe(fd_answer) < 0){
      ShowWarning("CallbackRing: Cannot create pipe");
      return;
    }
    if((id=fork()) == -1){
      ShowWarning("CallbackRing: Cannot fork Ring up process");
      return;
    }

    if(!id){
      char data[10];
      char msg[100];
      int lr, sock_cx;
      int fmax, mask, mask_sock, mask_pipe;

      close(fd_call[1]);
      close(fd_answer[0]);

      /* Create the TCP socket */
      if((sock_cx = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	strcpy(msg, "CallbackRing: Cannot create socket");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
      /* Try to connect to the distant ivs_daemon */
      if(connect(sock_cx, (struct sockaddr *)&addr_dist, sizeof(addr_dist)) 
	 < 0){
	strcpy(msg, "Cannot connect to the distant ivs daemon");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
      
      mask_pipe = (1 << fd_call[0]);
      mask_sock = (1 << sock_cx);
      fmax = maxi(fd_call[0], sock_cx) + 1;
      if(!SendTalkRequest(sock_cx, name_user)){
	strcpy(msg, "CallbackRing: SendTalkRequest failed");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }

      mask = mask_pipe | mask_sock;
      if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
		(struct timeval *)0) > 0){

	if(mask & mask_pipe){
	  /* Abort the call */
	  if(!SendTalkAbort(sock_cx)){
	    strcpy(msg, "CallbackRing: SendTalkAbort failed");
	    if(write(fd_answer[1], msg, strlen(msg)) < 0){
	      perror("CallbackRing'write");
	    }
	  }
	  exit(0);
	}else{
	  if(mask & mask_sock){
	    /* A message is arrived */
	    if((lr=read(sock_cx, data, 10)) <= 0){
	      perror("CallbackRing'read");
	      if(write(fd_answer[1], msg, 1+strlen(&msg[1])) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    }
	    switch(data[0]){
	    case CALL_ACCEPTED:
	      strcpy(msg, "IVS talk accepted by your party");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    case CALL_REFUSED:
	      strcpy(msg, "IVS talk refused. Please, retry later ...");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    default:
	      strcpy(msg, 
                    "CallbackRing: Incorrect answer received from your party");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(1);
	    }
	  }
	}
      }else{
	/* The distant ivs_daemon aborts ... */
	strcpy(msg, "CallbackRing: select failed");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
    }else{
      close(fd_call[0]);
      close(fd_answer[1]);
      id_call_input = XtAppAddInput(app_con, fd_answer[0], XtInputReadMask, 
				    ShowAnswerCall, NULL);
      ChangeAndMappLabel(button_ring, LabelCallOff);
      RING_UP = TRUE;
    }
  }else{
    XtRemoveInput(id_call_input);
    SendPIP_QUIT(fd_call);
    close(fd_call[1]);
    close(fd_answer[0]);
    ChangeAndMappLabel(button_ring, LabelCallOn);
    RING_UP = FALSE;
    waitpid(0, &statusp, WNOHANG);
  }
}



/*------------------------------------------------*\
*              Display rate callback               *
\*------------------------------------------------*/

static XtCallbackProc DisplayRate(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  static BOOLEAN FIRST=TRUE;
  static struct timeval tim_out;
  static int len_data=11;
  int fmax, mask0;
  int mask, lr;
  char data[11];
  Arg args[1];
  
  if(FIRST){
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    FIRST=FALSE;
  }
  
  mask0 = 1 << evs2f_fd[0];
  fmax = evs2f_fd[0] + 1;
  
  do{
    if((lr=read(evs2f_fd[0], data, len_data)) != len_data){
      ShowWarning(
		 "DisplayRate: Could not read all rate data from video coder");
      XtRemoveInput(id_input);
      break;
    }
    
    XtSetArg(args[0], XtNlabel, (String)data);
    XtSetValues(label_band2, args, 1);
    XtSetArg(args[0], XtNlabel, (String)&data[6]);
    XtSetValues(label_ref2, args, 1);
    mask = mask0;
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) <= 0)
      break;
  }while(TRUE);
  
}



/*------------------------------------------------*\
*                 Video  callbacks                 *
\*------------------------------------------------*/

static XtCallbackProc CallbackBri(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int lwrite;
  Arg args[1];
  char value[4];
  u_char command[257];
  
  brightness = (int)(100*top);
  sprintf(value, "%d", brightness);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);

#ifdef VIDEOPIX
  /* Send to video_encode process the new tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", brightness);
  sprintf((char *)&command[5], "%d", contrast);
  if((lwrite=write(evf2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackBri'write ");
    return;
  }
#endif /* VIDEOPIX */
}



static XtCallbackProc CallbackCon(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int lwrite;
  Arg args[1];
  char value[4];
  u_char command[257];
  
  contrast = (int)(100*top);
  sprintf(value, "%d", contrast);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);

#ifdef VIDEOPIX
  /* Send to video_encode process the new tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", brightness);
  sprintf((char *)&command[5], "%d", contrast);
  if((lwrite=write(evf2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackCon'write ");
    return;
  }
#endif /* VIDEOPIX */
}



static XtCallbackProc CallbackDefault(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int lwrite;
  Arg args[1];
  u_char command[257];

#ifdef VIDEOPIX
  brightness = 50;
  contrast = 50;
  InitScale(scroll_brightness, value_brightness, brightness);
  InitScale(scroll_contrast, value_contrast, contrast);

  /* Send to video_encode process the deafult tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", brightness);
  sprintf((char *)&command[5], "%d", contrast);
  if((lwrite=write(evf2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackBri'write ");
    return;
  }
#endif /* VIDEOPIX */
}


static XtCallbackProc CallbackLocalDisplay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data; 
{
  int lwrite;
  u_char command;
  
  if(!LOCAL_DISPLAY){
    LOCAL_DISPLAY = TRUE;
    ChangeLabel(button_local, LabelLocalOn);
    if(START_EVIDEO){
      command=PIP_LDISP;
      if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackLocalDisplay'write ");
	return;
      }
    }
  }else{
    LOCAL_DISPLAY = FALSE;
    ChangeLabel(button_local, LabelLocalOff);
    if(START_EVIDEO){
      command=PIP_NOLDISP;
      if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackLocalDisplay'write ");
	return;
      }
    }
  }
}



static XtCallbackProc CallbackFreeze(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int lwrite;
  u_char command;
  
  if(FREEZE){
    FREEZE=FALSE;
    command = PIP_UNFREEZE;
    ChangeLabel(button_freeze, LabelFreezeOff);
    if(START_EVIDEO){
      if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackFreeze'write ");
	return;
      }
    }
  }else{
    FREEZE=TRUE;
    command = PIP_FREEZE;
    ChangeLabel(button_freeze, LabelFreezeOn);
    if(START_EVIDEO){
      if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackFreeze'write ");
	return;
      }
    }
  }
}




/*--------------------------------------------------*\
*             Control output rate callback           *
\*--------------------------------------------------*/

static XtCallbackProc CallbackRate(w, closure, call_data) 
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  u_char command[5];
  char value[4];
  int lwrite;
  Arg args[1];
  
  
  rate_max = (int)(300*top);
  if(rate_max < 1)
    rate_max = 1;
  sprintf(value, "%d", rate_max);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  if(START_EVIDEO){
    command[0] = PIP_MAX_RATE;
    sprintf((char *)&command[1], "%d", rate_max);
    if((lwrite=write(evf2s_fd[1], (char *)command, 5)) != 5){
      perror("CallbackRate'write ");
      return;
    }
  }
}



/*------------------------------------------------*\
*                 Audio  callbacks                 *
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


static XtCallbackProc CallbackRec(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int val;
  char value[4];
  Arg args[1];
  
  val = (int)(MaxRecord*top);
  sprintf(value, "%d", val);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  SetRecordGain(val);
}



#ifdef SPARCSTATION

static XtCallbackProc CallbackAudioPort(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{
  int lwrite;
  u_char command;

  if(SPEAKER==TRUE){
    SPEAKER=FALSE;
    ChangeLabel(output_audio, LabelSpeakerOff);
    sounddest(0);
    MICRO_MUTED = FALSE;
    if(PUSH_TO_TALK){
      ChangeLabel(button_micro, LabelMicroOn); 
      if(START_EAUDIO){
	command = PIP_MIC_ENABLE;
	if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to coder");
	}
      }
      if(AUDIO_SET){
	command = PIP_CONT;
	if((lwrite=write(da_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to decoder");
	}
      }
    }
    return;
  }else{
    SPEAKER=TRUE; 	
    ChangeLabel(output_audio, LabelSpeakerOn); 
    sounddest(1);
    if(PUSH_TO_TALK){
      MICRO_MUTED = TRUE;
      ChangeLabel(button_micro, LabelPushToTalk); 
      if(START_EAUDIO){
	command = PIP_MIC_DISABLE;
	if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to coder");
	}
      }
      if(AUDIO_SET){
	command = PIP_CONT;
	if((lwrite=write(da_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to decoder");
	}
      }
    }else
      MICRO_MUTED = FALSE;
    return;
  }
}

#endif /* SPARCSTATION */



static void ActionMicro(widget, event, params, num_params)
     Widget   widget;
     XEvent   *event;
     String   *params;
     Cardinal *num_params;
{
  Arg args[5];
  Cardinal n=0;
  u_char command;
  int lwrite;
  
  if(SPEAKER){
    /* Push-button mode */
    n = 0;
    if(event -> type == ButtonPress){
      if(START_EAUDIO){
	command = PIP_MIC_ENABLE;
	if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to coder");
	}
	ReverseColor(button_micro);
      }
      if(AUDIO_SET){
	command = PIP_DROP;
	if((lwrite=write(da_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to decoder");
	}
      }
      return;
    }
    if(event -> type == ButtonRelease){
      if(START_EAUDIO){
	command = PIP_MIC_DISABLE;
	if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to coder");
	}
	ReverseColor(button_micro);
      }
      if(AUDIO_SET){
	command = PIP_CONT;
	if((lwrite=write(da_fd[1], (char *)&command, 1)) != 1){
	  perror("ActionMicro'write to decoder");
	}
      }
    }
    return;
  }else{
    /* flip-flop mode */
    if(event -> type == ButtonRelease){
      if(MICRO_MUTED){
	MICRO_MUTED = FALSE;
	ChangeLabel(button_micro, LabelMicroOn); 
	if(START_EAUDIO){
	  command = PIP_MIC_ENABLE;
	  if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	    perror("ActionMicro'write to coder");
	  }
	}
      }else{
	MICRO_MUTED = TRUE;
	ChangeLabel(button_micro, LabelMicroOff); 
	if(START_EAUDIO){
	  command = PIP_MIC_DISABLE;
	  if((lwrite=write(ea_fd[1], (char *)&command, 1)) != 1){
	    perror("ActionMicro'write to coder");
	  }
	}
      }/* MICRO MUTED */
    } /* Button released */
  } /* flip-flop mode */
}




/*-----------------------------------------*\
*              Menus Callbacks              *
\*-----------------------------------------*/

#ifdef VIDEOPIX

static void CallbackMenuVideoPix(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lwrite;
  VfcDev *vfc_dev;
  int format, video_port;
  
  strcpy(choice, XtName(w));
  if(streq(choice, VideoPixMenuItems[0])){
    /* Video Port 1 selected */
    if(vfc_port_video != VFC_PORT1){
      switch(vfc_port_video){
      case VFC_PORT2:
	UnMarkMenu(menu_videopix[1]);
	break;
      case VFC_SVIDEO:
	UnMarkMenu(menu_videopix[2]);
	break;
      }
      MarkMenu(menu_videopix[0]);
      vfc_port_video = VFC_PORT1;
      if(START_EVIDEO){
	command[0] = PIP_VFC_PORT;
	command[1] = VFC_PORT1;
	if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	  perror("CallbackMenuVideoPix'write ");
	  return;
	}
      }
      return;
    }
  }
  if(streq(choice, VideoPixMenuItems[1])){
    /* Video Port 2 selected */
    if(vfc_port_video != VFC_PORT2){
      switch(vfc_port_video){
      case VFC_PORT1:
	UnMarkMenu(menu_videopix[0]);
	break;
      case VFC_SVIDEO:
	UnMarkMenu(menu_videopix[2]);
	break;
      }
      MarkMenu(menu_videopix[1]);
      vfc_port_video = VFC_PORT2;
      if(START_EVIDEO){
	command[0] = PIP_VFC_PORT;
	command[1] = VFC_PORT2;
	if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	  perror("CallbackMenuVideoPix'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, VideoPixMenuItems[2])){
    /* Port SVideo selected */
    if(vfc_port_video != VFC_SVIDEO){
      switch(vfc_port_video){
      case VFC_PORT1:
	UnMarkMenu(menu_videopix[0]);
	break;
      case VFC_PORT2:
	UnMarkMenu(menu_videopix[1]);
	break;
      }
      MarkMenu(menu_videopix[2]);
      vfc_port_video = VFC_SVIDEO;
      if(START_EVIDEO){
	command[0] = PIP_VFC_PORT;
	command[1] = VFC_SVIDEO;
	if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	  perror("CallbackMenuVideoPix'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, VideoPixMenuItems[3])){
    /* Automatic video port selection */
    if((vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV)) == NULL){
      ShowWarning("CallbackVideo: Cannot open hardware VideoPix");
      return;
    }
    if((video_port=VfcScanPorts(vfc_dev, &format)) == -1){
      ShowWarning(
		  "CallbackVideo: No input video available on any port");
      vfc_destroy(vfc_dev);
      return;
    }else{
      vfc_destroy(vfc_dev);
      if(video_port != vfc_port_video){
	switch(vfc_port_video){
	case VFC_PORT1:
	  UnMarkMenu(menu_videopix[0]);
	  break;
	case VFC_PORT2:
	  UnMarkMenu(menu_videopix[1]);
	  break;
	case VFC_SVIDEO:
	  UnMarkMenu(menu_videopix[2]);
	  break;
	}
	switch(video_port){
	case VFC_PORT1:
	  MarkMenu(menu_videopix[0]);
	  break;
	case VFC_PORT2:
	  MarkMenu(menu_videopix[1]);
	  break;
	case VFC_SVIDEO:
	  MarkMenu(menu_videopix[2]);
	  break;
	}
	vfc_port_video = video_port;
	if(START_EVIDEO){
	  command[0] = PIP_VFC_PORT;
	  command[1] = video_port;
	  if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	    perror("CallbackScanVideoPort'write ");
	    return;
	  }
	}
      }
    }
    return;
  }
}
#endif /* VIDEOPIX */



static void CallbackMenuVideoFormat(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lwrite;
  
  strcpy(choice, XtName(w));
  if(streq(choice, VideoFormatMenuItems[0])){
    /* QCIF selected */
    switch(size){
    case CIF4_SIZE:
      UnMarkMenu(menu_video[2]);
      MarkMenu(menu_video[0]);
      size = QCIF_SIZE;
      break;
    case CIF_SIZE:
      UnMarkMenu(menu_video[1]);
      MarkMenu(menu_video[0]);
      size = QCIF_SIZE;
      break;
    case QCIF_SIZE:
      return;
    }
    if(START_EVIDEO){
      command[0] = PIP_SIZE;
      command[1] = QCIF_SIZE;
      if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackMenuVideoFormat'write ");
	return;
      }
    }
    return;
  }
  if(streq(choice, VideoFormatMenuItems[1])){
    /* CIF selected */
    switch(size){
    case CIF4_SIZE:
      UnMarkMenu(menu_video[2]);
      MarkMenu(menu_video[1]);
      size = CIF_SIZE;
      break;
    case QCIF_SIZE:
      UnMarkMenu(menu_video[0]);
      MarkMenu(menu_video[1]);
      size = CIF_SIZE;
      break;
    case CIF_SIZE:
      return;
    }
    if(START_EVIDEO){
      command[0] = PIP_SIZE;
      command[1] = CIF_SIZE;
      if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackMenuVideoFormat'write ");
	return;
      }
    }
    return;
  }
  if(streq(choice, VideoFormatMenuItems[2])){
    /* Super CIF selected */
    switch(size){
    case CIF_SIZE:
      UnMarkMenu(menu_video[1]);
      MarkMenu(menu_video[2]);
      size = CIF4_SIZE;
      break;
    case QCIF_SIZE:
      UnMarkMenu(menu_video[0]);
      MarkMenu(menu_video[2]);
      size = CIF4_SIZE;
      break;
    case CIF4_SIZE:
      return;
    }
    if(START_EVIDEO){
      command[0] = PIP_SIZE;
      command[1] = CIF4_SIZE;
      if((lwrite=write(evf2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackMenuVideoFormat'write ");
	return;
      }
    }
    return;
  }
  if(streq(choice, VideoFormatMenuItems[3])){
    /* Grey mode selected */
    if(COLOR){
      COLOR = FALSE ;
      UnMarkMenu(menu_video[4]);
      MarkMenu(menu_video[3]);
    }
    return;
  }
  if(streq(choice, VideoFormatMenuItems[4])){
    /* Color mode selected */
    if(!COLOR){
      COLOR = TRUE ;
      UnMarkMenu(menu_video[3]);
      MarkMenu(menu_video[4]);
    }
    return;
  }
}



static void CallbackMenuRateControl(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command;
  int lwrite;
  
  strcpy(choice, XtName(w));
  if(streq(choice, ControlMenuItems[0])){
    /* Without any control option selected */
    if(rate_control != WITHOUT_CONTROL){
      XtSetSensitive(form_control, FALSE);
      switch(rate_control){
      case REFRESH_CONTROL:
	UnMarkMenu(menu_rate[1]);
	break;
      case QUALITY_CONTROL:
	UnMarkMenu(menu_rate[2]);
	break;
      }
      MarkMenu(menu_rate[0]);
      rate_control = WITHOUT_CONTROL;
      if(START_EVIDEO){
	command=PIP_WCONTROL;
	if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackControlRate'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, ControlMenuItems[1])){
    /* Refresh control option selected */
    if(rate_control != REFRESH_CONTROL){
      XtSetSensitive(form_control, TRUE);
      switch(rate_control){
      case WITHOUT_CONTROL:
	UnMarkMenu(menu_rate[0]);
	break;
      case QUALITY_CONTROL:
	UnMarkMenu(menu_rate[2]);
	break;
      }
      MarkMenu(menu_rate[1]);
      rate_control = REFRESH_CONTROL;
      if(START_EVIDEO){
	command=PIP_RCONTROL;
	if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackControlRate'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, ControlMenuItems[2])){
    /* Quality control option selected */
    if(audio_format != QUALITY_CONTROL){
      XtSetSensitive(form_control, TRUE);
      switch(rate_control){
      case WITHOUT_CONTROL:
	UnMarkMenu(menu_rate[0]);
	break;
      case REFRESH_CONTROL:
	UnMarkMenu(menu_rate[1]);
	break;
      }
      MarkMenu(menu_rate[2]);
      rate_control = QUALITY_CONTROL;
      if(START_EVIDEO){
	command=PIP_QCONTROL;
	if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackControlRate'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, ControlMenuItems[3])){
    /* Feedback allowed mode */
    if(!FEEDBACK){
      MarkMenu(menu_rate[3]);
      UnMarkMenu(menu_rate[4]);
      FEEDBACK=TRUE;
      my_feedback_port = PORT_VIDEO_CONTROL;
      if(START_EVIDEO){
	command=PIP_FEEDBACK;
	if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackFeedback'write ");
	  return;
	}
      }
    }
    return;
  }
  if(streq(choice, ControlMenuItems[4])){
    /* No feedback allowed mode */
    if(FEEDBACK){
      MarkMenu(menu_rate[4]);
      UnMarkMenu(menu_rate[3]);
      FEEDBACK=FALSE;
      my_feedback_port = 0;
      if(START_EVIDEO){
	command=PIP_NOFEEDBACK;
	if((lwrite=write(evf2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackFeedback'write ");
	  return;
	}
      }
    }
    return;
  }
}



static void CallbackMenuAudio(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lwrite;
  
  strcpy(choice, XtName(w));
  if(streq(choice, AudioMenuItems[0])){
    /* PCM encoding selected */
    if(audio_format != PCM_64){
      switch(audio_format){
      case ADPCM_32:
	UnMarkMenu(menu_eaudio[1]);
	break;
      case VADPCM:
	UnMarkMenu(menu_eaudio[2]);
	break;
      }
      MarkMenu(menu_eaudio[0]);
      audio_format = PCM_64;
    }
  }else{
    if(streq(choice, AudioMenuItems[1])){
      /* ADPCM encoding selected */
      if(audio_format != ADPCM_32){
	switch(audio_format){
	case PCM_64:
	  UnMarkMenu(menu_eaudio[0]);
	  break;
	case VADPCM:
	  UnMarkMenu(menu_eaudio[2]);
	  break;
	}
	MarkMenu(menu_eaudio[1]);
	audio_format = ADPCM_32;
      }
    }else{
      if(streq(choice, AudioMenuItems[2])){
	/* VADPCM encoding selected */
	if(audio_format != VADPCM){
	  switch(audio_format){
	  case PCM_64:
	    UnMarkMenu(menu_eaudio[0]);
	    break;
	  case ADPCM_32:
	    UnMarkMenu(menu_eaudio[1]);
	    break;
	  }
	  MarkMenu(menu_eaudio[2]);
	  audio_format = VADPCM;
	}
      }
    }
  }
  if(START_EAUDIO){
    command[0] = (u_char) PIP_FORMAT;
    command[1] = audio_format;
    if((lwrite=write(ea_fd[1], (char *)command, 2)) != 2){
      perror("CallbackMenuAudio'write ");
      return;
    }
  }
}



/*-----------------------------------------------------*\
*    Video and audio encoding procedures callbacks      *
\*-----------------------------------------------------*/


static XtCallbackProc CallbackEVideo(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int id;
  BOOLEAN ok;
#ifdef VIDEOPIX
  int vfc_format;
  VfcDev *vfc_dev;
#endif
  
  if(!START_EVIDEO){
    XtSetMappedWhenManaged(button_e_video, FALSE);
    if(pipe(evf2s_fd) < 0){
      ShowWarning("CallbackEVideo: Cannot create father to son pipe");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    if(pipe(evs2f_fd) < 0){
      ShowWarning("CallbackEVideo: Cannot create son to father pipe");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    ok = FALSE;
#ifdef VIDEOPIX
    if((vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV)) == NULL){
      ShowWarning("CallbackEVideo: Cannot open hardware VideoPix");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    vfc_set_port(vfc_dev, vfc_port_video);
    if(vfc_set_format(vfc_dev, VFC_AUTO, &vfc_format) < 0){
      ShowWarning("CallbackEVideo: No video signal detected");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      vfc_destroy(vfc_dev);
      return;
    }else{
      vfc_format = vfc_video_format(vfc_format);
      ok = TRUE;
    }
#else
#ifdef INDIGOVIDEO
    if(!sgi_checkvideo()){
      ShowWarning("Sorry, this Indigo has no video capture");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }else 
      ok = TRUE;
#else
#ifdef HP_ROPS_VIDEO
    if(!hp_rops_checkvideo()){
      ShowWarning("Sorry, this HP has no video capture");
      return;
    }else
      ok = TRUE;
#else
#ifdef SUN_PX_VIDEO
    if(!sun_px_checkvideo()){
      ShowWarning("Sorry, this SPARCstation has no video capture");
      return;
    }else
      ok = TRUE;
#else
#ifdef VIDEOTX
    ok = TRUE;
#endif /* VIDEOTX */
#endif /* SUN_PX_VIDEO */
#endif /* HP_ROPS_VIDEO */
#endif /* INDIGOVIDEO */
#endif /* VIDEOPIX */
    if(!ok){
      ShowWarning("Sorry, this IVS is configured without video capture");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    if((id=fork()) == -1){
      ShowWarning(
     "CallbackEVideo: Too many processes, Cannot fork video encoding process");
      XtSetMappedWhenManaged(button_e_video, TRUE);
#ifdef VIDEOPIX
      vfc_destroy(vfc_dev);
#endif
      return;
    }
    /* fathers's process goes on , son's process starts encoding */
    if(id){
      START_EVIDEO=TRUE;
      close(evf2s_fd[0]);
      close(evs2f_fd[1]);
      id_input = XtAppAddInput(app_con, evs2f_fd[0], XtInputReadMask, 
			       DisplayRate, NULL);
      SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
		 my_feedback_port, my_sin_addr, name_user);
      ChangeLabel(button_e_video, "Stop video");
      SetSensitiveVideoMenu(FALSE);
      XtSetMappedWhenManaged(button_e_video, TRUE);
#ifdef VIDEOPIX
      SetSensitiveVideoTuning(TRUE);
#endif
    }else{
      close(evf2s_fd[1]);
      close(evs2f_fd[0]);
#ifdef VIDEOPIX
      sun_vfc_video_encode(IP_GROUP, VIDEO_PORT, ttl, size, evf2s_fd, evs2f_fd,
			   COLOR, STAT_MODE, FREEZE, LOCAL_DISPLAY, FEEDBACK,
			   display, rate_control, rate_max, vfc_port_video, 
			   vfc_format, vfc_dev, brightness, contrast, 
			   default_quantizer);
#else
#ifdef INDIGOVIDEO
      sgi_video_encode(IP_GROUP, VIDEO_PORT, ttl, size, evf2s_fd, evs2f_fd, 
		       COLOR, STAT_MODE, FREEZE, LOCAL_DISPLAY, FEEDBACK, 
		       display, rate_control, rate_max, default_quantizer);
#else
#ifdef HP_ROPS_VIDEO
      hp_rops_video_encode(IP_GROUP, VIDEO_PORT, ttl, size, evf2s_fd,
			   evs2f_fd, STAT_MODE, FREEZE, LOCAL_DISPLAY, 
			   FEEDBACK, display, rate_control, rate_max, 
			   default_quantizer); 
#else
#ifdef SUN_PX_VIDEO
      sun_px_video_encode(IP_GROUP, VIDEO_PORT, ttl, size, evf2s_fd, evs2f_fd,
			  STAT_MODE, FREEZE, FEEDBACK, display, rate_control, 
			  rate_max, default_quantizer);
#else
#ifdef VIDEOTX
      dec_video_encode(IP_GROUP, VIDEO_PORT, ttl, size, evf2s_fd, evs2f_fd,
		       STAT_MODE, FREEZE, FEEDBACK, display, rate_control,
		       rate_max, default_quantizer);
#endif /* VIDEOTX */
#endif /* SUN_PX_VIDEO */
#endif /* HP_ROPS_VIDEO */
#endif /* INDIGOVIDEO */
#endif /* VIDEOPIX */
    }
  }else{
    SetSensitiveVideoMenu(TRUE);
    XtRemoveInput(id_input);
    SendPIP_QUIT(evf2s_fd);
    close(evf2s_fd[1]);
    close(evs2f_fd[0]);
    START_EVIDEO=FALSE;
    SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
	       my_feedback_port, my_sin_addr, name_user);
    ChangeLabel(button_e_video, "Encode video");
#ifdef VIDEOPIX
    SetSensitiveVideoTuning(FALSE);
#endif
    waitpid(0, &statusp, WNOHANG);
  }
}



static XtCallbackProc CallbackEAudio(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int id;
  
#ifndef DECSTATION
  if(!START_EAUDIO){
#if defined(SPARCSTATION) || defined(INDIGO)
    /* If the audio device is busy, don't fork any process */
    if(!soundinit(O_RDONLY)){
      ShowWarning("CallbackEAudio: Cannot initialize audio device");
      return;
    }
    soundterm();
#endif /* SPARCSTATION OR INDIGO */
    if(pipe(ea_fd) < 0){
      ShowWarning("CallbackEAudio: Cannot create pipe");
      return;
    }
    if((id=fork()) == -1){
      ShowWarning(
     "CallbackEAudio: Too many processes, Cannot fork audio encoding process");
      return;
    }
    /* fathers's process goes on , son's process starts encoding */
    if(id){
      START_EAUDIO=TRUE;
      close(ea_fd[0]);
      SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
		 my_feedback_port, my_sin_addr, name_user);
      ChangeLabel(button_e_audio, "Stop audio");
    }else{
      close(ea_fd[1]);
      audio_encode(IP_GROUP, AUDIO_PORT, ttl, audio_format,
		   MICRO_MUTED, ea_fd, squelch_level);
    }
  }else{
    SendPIP_QUIT(ea_fd);
    close(ea_fd[1]);
    START_EAUDIO=FALSE;
    SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
	       my_feedback_port, my_sin_addr, name_user);
    ChangeLabel(button_e_audio, "Encode audio");
    waitpid(0, &statusp, WNOHANG);
  }
#else
  ShowWarning("Sorry, DEC audio doesn't work yet ...");
#endif /* NOT DECSTATION */
}


/*-----------------------------------------------------*\
 *    Video and audio decoding procedures callbacks     *
\*-----------------------------------------------------*/



void CallbackDVideo(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  register int i;
  int i_pip;
  
  for(i=0; i<nb_stations; i++){
    if(w == button_d_video[i]){
      break;
    }
  }
  i_pip = pip[i];
  
  if(t[i].video_decoding == FALSE){
    InitDVideo(dv_fd[i_pip], i);
  }else{
    SendPIP_QUIT(dv_fd[i_pip]);
    close(dv_fd[i_pip][1]);
    t[i].video_decoding=FALSE;
    ChangeLabel(button_d_video[i], LabelHiden);
    waitpid(0, &statusp, WNOHANG);
  }
}



void CallbackDAudio(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  register int i;
  
  if(AUDIO_SET==FALSE){
    AUDIO_SET=InitDAudio(da_fd);
  }/* NOT AUDIO_SET */
  if(AUDIO_SET){
    for(i=0; i<nb_stations; i++){
      if(w==button_d_audio[i]){
	break;
      }
    }
    if(t[i].audio_decoding == FALSE){
      t[i].audio_decoding=TRUE;
      ChangeLabel(button_d_audio[i], LabelListen);
    }else{
      t[i].audio_decoding=FALSE;
      ChangeLabel(button_d_audio[i], LabelMuted);
    }
    SendPIP_NEW_TABLE(t, first_passive, da_fd);
  }
}



/*-----------------------------------------------------*\
 *                  Others callbacks                    *       
\*-----------------------------------------------------*/

static XtCallbackProc GetControlInfo(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  
  int fromlen, type, lr, l;
  struct hostent *host;
  u_char buf[1000];
  struct sockaddr_in from;
  u_long sin_addr_recv, ip_long;
  u_short feedback_port;
  char name[LEN_MAX_NAME];
  BOOLEAN VIDEO_ENCODING, AUDIO_ENCODING;
  BOOLEAN ERROR=FALSE, MYSELF=FALSE;
  BOOLEAN CONTINUE;
    
  waitpid(0, &statusp, WNOHANG);
  fromlen=sizeof(from);
  bzero((char *)&from, fromlen);
  if ((lr = recvfrom(csock_r, (char *)buf, 1000, 0, (struct sockaddr *)&from, 
		     &fromlen)) < 0){
    perror("receive from");
    return;
  }
  sin_addr_recv = (u_long)from.sin_addr.s_addr;

  l = 0;
  do{
    type = (int)(buf[l] & 0x7f);
    switch(type){
    case RTP_CDESC:
      if(buf[l+2] != RTP_H261_CONTENT){
	fprintf(stderr, 
		"GetControlInfo: Bad content RTCP packet %d\n", buf[l+2]);
        ERROR = TRUE;
        break;
      }
      if(buf[l+12] != RTP_IVS_VERSION){
	fprintf(stderr,
		"GetControlInfo: Bad IVS option %d.%d received from %s\n",
		buf[l+12]>>4, buf[l+12]&0x0f, &(buf[l+24]));
	ERROR = TRUE;
	break;
      }
      feedback_port = (u_short)((buf[l+4] << 8) + buf[l+5]);
      AUDIO_ENCODING = buf[13];
      VIDEO_ENCODING = buf[14];
      if((buf[l+16] & 0x7f) != RTP_SDESC){
	fprintf(stderr, "GetControlInfo: SDESC missing after CDESC\n");
        ERROR = TRUE;
        break;
      }
      bcopy((char *)&buf[20], (char *)&ip_long, 4);
      strcpy(name, (char *) &(buf[l+24]));
      strcat(name, "@");
      if((host=gethostbyaddr((char *)(&(from.sin_addr)), 
			     sizeof(struct in_addr), from.sin_family)) ==NULL){
	strcat(name, inet_ntoa(from.sin_addr));
      }else{
	strcat(name, host->h_name);
      }
      if(my_sin_addr == sin_addr_recv){
	if((nb_stations == 0) || streq(name, t[0].name))
	  MYSELF=TRUE;
      }
      if(RESTRICT_SAVING){
	if(!MYSELF && !VIDEO_ENCODING && !AUDIO_ENCODING){
	  /* Do not save in the local table this read-only station */
	  CONTINUE = (buf[l+16] & 0x80) ? FALSE : TRUE;
	  l += ((4 + buf[l+17]) << 2);
	  break;
	}
      }
      ManageStation(t, name, sin_addr_recv, feedback_port, VIDEO_ENCODING, 
		    AUDIO_ENCODING, MYSELF);
      CONTINUE = (buf[l+16] & 0x80) ? FALSE : TRUE;
      l += ((4 + buf[l+17]) << 2);
      break;
    
    case RTP_HELLO:
      if(FEEDBACK)
	  SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
		     my_feedback_port, my_sin_addr, name_user);
      CONTINUE = (buf[l] & 0x80) ? FALSE : TRUE;
      l += 4;
      break;

    case RTP_BYE:
      CONTINUE = (buf[l] & 0x80) ? FALSE : TRUE;
      RemoveStation(t, sin_addr_recv);
      l += 4;
      break;
    default:
      fprintf(stderr, "GetControlInfo: Received a packet type %d\n", type);
      ERROR = TRUE;
      break;
    } /* switch() */
  }while(CONTINUE && (ERROR==FALSE));
  waitpid(0, &statusp, WNOHANG);
}


static XtCallbackProc CallbackQuit(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  Quit();
}




/***************************************************************************
 * Main Program
 **************************************************************************/

main(argc, argv)
     int argc;
     char **argv;
{
  Widget entry;
  int narg, i, n;
  Arg args[10];
  FILE *F_aux;
  BOOLEAN NEW_IP_GROUP=FALSE;
  char ntemp[256], hostname[256], data[256], data_ttl[5];
  
  strcpy(display, "");
  cuserid(name_user);
  if(argc > 20)
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(streq(argv[narg], "-I")){
      if(++narg == argc)
	Usage(argv[0]);
      strcpy(IP_GROUP, argv[narg]);
      narg ++;
      NEW_IP_GROUP = TRUE;
      continue;
    }else{
      if(streq(argv[narg], "-N")){
	if(++narg == argc)
	  Usage(argv[0]);
	strncpy(name_user, argv[narg], L_cuserid);
	narg ++;
	continue;
      }else{
	if(streq(argv[narg], "-d")){
	  if(++narg == argc)
	    Usage(argv[0]);
	  strcpy(display, argv[narg]);
	  narg ++;
	  continue;
	}else{
	  if(streq(argv[narg], "-double")){
	    QCIF2CIF=TRUE;
	    narg ++;
	    continue;
	  }else{
	    if(streq(argv[narg], "-T")){
	      ttl = atoi(argv[++narg]);
	      narg ++;
	      continue;
	    }else{
	      if(streq(argv[narg], "-r")){
		RESTRICT_SAVING = TRUE;
		narg ++;
		continue;
	      }else{
		if(streq(argv[narg], "-a")){
		  IMPLICIT_AUDIO_DECODING = FALSE;
		  narg ++;
		  continue;
		}else{
		  if(streq(argv[narg], "-v")){
		    IMPLICIT_VIDEO_DECODING = FALSE;
		    narg ++;
		    continue;
		  }else{
		    if(streq(argv[narg], "-dp")){
		      PUSH_TO_TALK = FALSE;
		      narg ++;
		      continue;
		    }else{
		      if(streq(argv[narg], "-stat")){
			STAT_MODE = TRUE;
			narg ++;
			continue;
		      }else{
			if(streq(argv[narg], "-grey")){
			  FORCE_GREY = TRUE;
			  narg ++;
			  continue;
			}else{
			  if(streq(argv[narg], "-q")){
			    default_quantizer = atoi(argv[++narg]);
			    narg ++;
			    continue;
			  }else{
			    if(streq(argv[narg], "-S")){
			      squelch_level = atoi(argv[++narg]);
			      narg ++;
			      continue;
			    }else{
			      if(argv[narg][0] == '-')
				Usage(argv[0]);
			      else{
				UNICAST=TRUE;
				strcpy(hostname, argv[narg]);
				narg ++;
				continue;
			      }
			    }
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
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
      if((F_aux=fopen(".videoconf.default","w")) != NULL){
	fprintf(F_aux, "AUDIO_PORT:\t\t%s\n", AUDIO_PORT);
	fprintf(F_aux, "VIDEO_PORT:\t\t%s\n", VIDEO_PORT);
	fprintf(F_aux, "CONTROL_PORT:\t\t%s\n", CONTROL_PORT);
	fprintf(F_aux, "IP_GROUP:\t\t%s\n", IP_GROUP);
      }
      fclose(F_aux);
    }
  }
  {
    /* getting my sin_addr */

    struct hostent *hp;
    struct sockaddr_in my_sock_addr;
    int namelen=100;
    char name[100];
    register int i;

    if(gethostname(name, namelen) != 0){
      perror("gethostname");
      exit(1);
    }
    if((hp=gethostbyname(name)) == 0){
	fprintf(stderr, "%d-Unknown host\n", getpid());
	exit(-1);
      }
    bcopy(hp->h_addr, (char *)&my_sock_addr.sin_addr, 4);
    my_sin_addr = my_sock_addr.sin_addr.s_addr;
    for(i=0; i<N_MAX_STATIONS; i++)
      pip[i] = i;
  }
  if(UNICAST){
    unsigned int i1, i2, i3, i4;
    struct hostent *hp;

    if(RESTRICT_SAVING){
      fprintf(stderr, "Unicast mode, -r option turned off\n");
      RESTRICT_SAVING = FALSE;
    }
    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), hostname);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&addr_dist.sin_addr, 4);
      addr_dist.sin_family = AF_INET;
      addr_dist.sin_port = htons((u_short)IVSD_PORT);
      strcpy(IP_GROUP, inet_ntoa(addr_dist.sin_addr));
    }else{
      strcpy(IP_GROUP, hostname);
      addr_dist.sin_family = AF_INET;
      addr_dist.sin_addr.s_addr = (u_long)inet_addr(hostname);
      addr_dist.sin_port = htons((u_short)IVSD_PORT);
    }
  }

  /* Initializations */

  InitTable(t);
  csock_r=CreateReceiveSocket(&addr_r, &len_addr_r, CONTROL_PORT, IP_GROUP);
  csock_s=CreateSendSocket(&addr_s, &len_addr_s, CONTROL_PORT, IP_GROUP, &ttl);
  
  /* Audio initialization */
#ifdef SPARCSTATION
  if(!sounddevinit()){
    fprintf(stderr, "cannot opening audioctl device");
  }else{
    if(soundgetdest()){
      SPEAKER = FALSE;
      MICRO_MUTED = FALSE;
    }else{
      SPEAKER = TRUE;
      MICRO_MUTED = (PUSH_TO_TALK ? TRUE : FALSE);
    }
  }
#else
  SPEAKER = TRUE;
  MICRO_MUTED = FALSE;
#endif
#ifdef HPUX
  if(!audioInit(audioserver)){
    fprintf(stderr,"cannot connect to AudioServer\n");
  }
#endif

  if(STAT_MODE){
    char date[30];
    time_t a_date;
    if((F_log=fopen(LoginFileName, "w")) == NULL){
      STAT_MODE = FALSE;
      fprintf(stderr, "cannot create login file\n");
    }else{
      a_date = time((time_t *)0);
      strcpy(date, ctime(&a_date)); 
      fprintf(F_log, "\t\t *** IVS Participants list ***\n\n");
      fprintf(F_log, "Beginning of conference: %s\n", date);
      fclose(F_log);
    }
  }

  toplevel = XtAppInitialize(&app_con, "IVS", NULL, 0, &argc, argv, 
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
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form0 = XtCreateManagedWidget("Form0", formWidgetClass, main_form,
				args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, IVS_VERSION); n++;
  label_title = XtCreateManagedWidget("LabelTitle", labelWidgetClass, 
				      form0, args, n);

  /*
  *  Form 1.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form0); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form1 = XtCreateManagedWidget("Form1", formWidgetClass, main_form,
				   args, n);

  /*
  *  Create the Brightness and contrast utilities scales.
  */
  CreateScale(&form_brightness, &scroll_brightness, &name_brightness,
	      &value_brightness, form1, "FormBrightness", 
	      "NameBrightness", DefaultBrightness);
  XtSetSensitive(form_brightness, FALSE);
  XtAddCallback(scroll_brightness, XtNjumpProc, CallbackBri, 
		(XtPointer)value_brightness);
  
  CreateScale(&form_contrast, &scroll_contrast, &name_contrast,
	      &value_contrast, form1, "FormContrast", "NameContrast", 
	      DefaultContrast);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_brightness); n++;
  XtSetValues(form_contrast, args, n); 
  XtSetSensitive(form_contrast, FALSE);
  XtAddCallback(scroll_contrast, XtNjumpProc, CallbackCon, 
		(XtPointer)value_contrast);
  
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_contrast); n++;
  XtSetArg(args[n], XtNsensitive, FALSE); n++;
  button_default = XtCreateManagedWidget("ButtonDefault", commandWidgetClass,
					   form1, args, n);
  XtAddCallback(button_default, XtNcallback, CallbackDefault, NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_default); n++;
  button_local = XtCreateManagedWidget("ButtonLocal", commandWidgetClass,
				       form1, args, n);
  XtAddCallback(button_local, XtNcallback, CallbackLocalDisplay, NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_default); n++;
  XtSetArg(args[n], XtNfromHoriz, form_contrast); n++;
  button_freeze = XtCreateManagedWidget("ButtonFreeze", commandWidgetClass,
					form1, args, n);
  XtAddCallback(button_freeze, XtNcallback, CallbackFreeze, NULL);

  /*
  *  Form 2.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form1); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form2 = XtCreateManagedWidget("Form2", formWidgetClass, main_form,
				   args, n);
  /*
  *  Create the control rate functions.
  */
  CreateScale(&form_control, &scroll_control, &name_control, &value_control,
	      form2, "FormControl", "NameControl", 100);
  XtAddCallback(scroll_control, XtNjumpProc, CallbackRate, 
		(XtPointer)value_control);

  /*
  * Create the Bandwidth display.
  */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_control); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  form_bandwidth = XtCreateManagedWidget("FormBandwidth", formWidgetClass, 
					 form2, args, n);
  n = 0;
  label_band1 = XtCreateManagedWidget("LabelB1", labelWidgetClass, 
				      form_bandwidth, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_band1); n++;
  label_band2 = XtCreateManagedWidget("LabelB2", labelWidgetClass, 
				      form_bandwidth, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_band2); n++;
  label_band3 = XtCreateManagedWidget("LabelB3", labelWidgetClass, 
				      form_bandwidth, args, n);
  
  /*
  * Create the Refresh rate display.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_bandwidth); n++;
  XtSetArg(args[n], XtNfromHoriz, form_control); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  form_refresh = XtCreateManagedWidget("FormBandwidth", formWidgetClass, 
				       form2, args, n);
  label_ref1 = XtCreateManagedWidget("LabelR1", labelWidgetClass, 
				     form_refresh, NULL, 0);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_ref1); n++;
  label_ref2 = XtCreateManagedWidget("LabelR2", labelWidgetClass, 
				     form_refresh, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_ref2); n++;
  label_ref3 = XtCreateManagedWidget("LabelR3", labelWidgetClass, 
				     form_refresh, args, n);
  
  /*
  *  Form 2_1.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form1); n++;
  XtSetArg(args[n], XtNfromHoriz, form2); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  form2_1 = XtCreateManagedWidget("Form2_1", formWidgetClass, main_form,
				  args, n);
  /*
  *  Create the start encoding buttons.
  */
  n = 0;
  button_e_video = XtCreateManagedWidget("ButtonEVideo", commandWidgetClass,
					 form2_1, args, n);
  XtAddCallback(button_e_video, XtNcallback, CallbackEVideo, NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_e_video); n++;
  button_e_audio = XtCreateManagedWidget("ButtonEAudio", commandWidgetClass,
					 form2_1, args, n);
  XtAddCallback(button_e_audio, XtNcallback, CallbackEAudio, NULL);


  /*
  *  Form 3.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form2); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form3 = XtCreateManagedWidget("Form3", formWidgetClass, main_form,
				   args, n);
  /*
  *  Create the audio control utilities.
  */
  CreateScale(&form_v_audio, &scroll_v_audio, &name_v_audio, &value_v_audio,
	      form3, "FormVolAudio", "NameVolAudio", DefaultVolume);
  n = 0;
  XtSetValues(form_v_audio, args, n);
  XtAddCallback(scroll_v_audio, XtNjumpProc, CallbackVol, 
		(XtPointer)value_v_audio);
  SetVolumeGain(DefaultVolume);
  
  CreateScale(&form_r_audio, &scroll_r_audio, &name_r_audio, &value_r_audio,
	      form3, "FormRecAudio", "NameRecAudio", DefaultRecord);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_v_audio); n++;
  XtSetValues(form_r_audio, args, n);
  XtAddCallback(scroll_r_audio, XtNjumpProc, CallbackRec, 
		(XtPointer)value_r_audio);
  SetRecordGain(DefaultRecord);
  
#ifdef SPARCSTATION
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_r_audio); n++;
  if(SPEAKER){
    XtSetArg(args[n], XtNlabel, LabelSpeakerOn); n++;
  }else{
    XtSetArg(args[n], XtNlabel, LabelSpeakerOff); n++;
  }
  output_audio = XtCreateManagedWidget("ButtonOutAudio", commandWidgetClass, 
				       form3, args, n);
  XtAddCallback(output_audio, XtNcallback, CallbackAudioPort, NULL);
    
  if(PUSH_TO_TALK){
    n = 0;
    if(SPEAKER){
      XtSetArg(args[n], XtNlabel, LabelPushToTalk); n++;
    }else{
      XtSetArg(args[n], XtNlabel, LabelMicroOn); n++;
    }
    XtSetArg(args[n], XtNfromHoriz, form_r_audio); n++;
    XtSetArg(args[n], XtNfromVert, output_audio); n++;
    button_micro = XtCreateManagedWidget("ButtonMicro", commandWidgetClass, 
					 form3, args, n);
    XtAppAddActions(app_con, actions, XtNumber(actions));
  }
#endif /* SPARCSTATION */


  /*
  *  Form 3_1.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form2); n++;
  XtSetArg(args[n], XtNfromHoriz, form3); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  form3_1 = XtCreateManagedWidget("Form3_1", formWidgetClass, main_form,
				  args, n);
  button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
				      form3_1, NULL, 0);
  XtAddCallback(button_quit, XtNcallback, CallbackQuit, NULL);
  
  if(UNICAST){
    n = 0;
    XtSetArg(args[n], XtNfromVert, button_quit); n++;
    XtSetArg(args[n], XtNlabel, LabelCallOn); n++;
    XtSetArg(args[n], XtNwidth, 80); n++;
    button_ring = XtCreateManagedWidget("ButtonRing", commandWidgetClass,
					form3_1, args, n);
    XtAddCallback(button_ring, XtNcallback, CallbackRing, NULL);
  }

  /*
  *  Form 4.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form3); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form4 = XtCreateManagedWidget("Form4", formWidgetClass, main_form,
				args, n);

#ifdef VIDEOPIX

  /* VideoPix input menu */
  n = 0;
  button_port = XtCreateManagedWidget("ButtonPortVideo", 
				      menuButtonWidgetClass, form4, args, n);
  menu_port = XtCreatePopupShell("menu", simpleMenuWidgetClass,
				 button_port, NULL, 0);
  for(n=0; n<4; n++){
    menu_videopix[n] = XtCreateManagedWidget(VideoPixMenuItems[n], 
				        smeBSBObjectClass, menu_port, NULL, 0);
    XtAddCallback(menu_videopix[n], XtNcallback, CallbackMenuVideoPix, NULL);
    if(n==2)
      entry = XtCreateManagedWidget("Line", smeLineObjectClass, menu_port,
				    NULL, 0);
  }
#endif /* VIDEOPIX */

  /* Video format menu */
  n = 0;
#ifdef VIDEOPIX
  XtSetArg(args[n], XtNfromHoriz, button_port); n++;
#endif /* VIDEOPIX */
  button_format = XtCreateManagedWidget("ButtonFormatVideo", 
					menuButtonWidgetClass, form4, args, n);
  menu_format = XtCreatePopupShell("menu", simpleMenuWidgetClass,
				   button_format, NULL, 0);
#ifdef VIDEOPIX
  for(n=0; n<5; n++){
#else
  for(n=0; n<2; n++){
#endif
    menu_video[n] = XtCreateManagedWidget(VideoFormatMenuItems[n], 
				    smeBSBObjectClass, menu_format, NULL, 0);
    XtAddCallback(menu_video[n], XtNcallback, CallbackMenuVideoFormat, NULL);
    if(n==2)
      entry = XtCreateManagedWidget("Line", smeLineObjectClass, menu_format,
				    NULL, 0);
  }
    
  /* Video rate control menu */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format); n++;
  button_control = XtCreateManagedWidget("ButtonControl", 
					menuButtonWidgetClass, form4, args, n);
  menu_control = XtCreatePopupShell("menu", simpleMenuWidgetClass,
				    button_control, NULL, 0);
  for(n=0; n<5; n++){
    menu_rate[n] = XtCreateManagedWidget(ControlMenuItems[n], 
                                    smeBSBObjectClass, menu_control, NULL, 0);
    XtAddCallback(menu_rate[n], XtNcallback, CallbackMenuRateControl, NULL);
    if(n==2)
      entry = XtCreateManagedWidget("Line", smeLineObjectClass, menu_control,
				    NULL, 0);
  }

  /* Audio encoding options menu */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_control); n++;
  button_audio = XtCreateManagedWidget("ButtonOptAudio", 
				       menuButtonWidgetClass, form4, args, n);
  menu_audio = XtCreatePopupShell("menu", simpleMenuWidgetClass,
				  button_audio, NULL, 0);
  for(n=0; n<3; n++){
    menu_eaudio[n] = XtCreateManagedWidget(AudioMenuItems[n], 
				      smeBSBObjectClass, menu_audio, NULL, 0);
    XtAddCallback(menu_eaudio[n], XtNcallback, CallbackMenuAudio, NULL);
  }

  /*
  *  Form 5.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form4); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form5 = XtCreateManagedWidget("Form5", formWidgetClass, main_form,
				args, n);

  n = 0;
  if(UNICAST){
    strcpy(data, "-UNICAST MODE-  Correspondant:");
    strcat(data, hostname);
  }else{
    strcpy(data, "-MULTICAST MODE-  Address:");
    strcat(data, IP_GROUP);
    strcat(data, "   TTL:");
    sprintf(data_ttl, "%d", ttl);
    strcat(data, data_ttl);
  }
  XtSetArg(args[n], XtNlabel, data); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  label_conf = XtCreateManagedWidget("LabelConf", labelWidgetClass,
				     form5, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromVert, label_conf); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNlabel, LabelHOSTS); n++;
  label_hosts = XtCreateManagedWidget("LabelHosts", labelWidgetClass, 
				      form5, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_hosts); n++;
  XtSetArg(args[n], XtNfromVert, label_conf); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNlabel, LabelVIDEO); n++;
  label_video = XtCreateManagedWidget("LabelVideo", labelWidgetClass, 
				      form5, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_video); n++;
  XtSetArg(args[n], XtNfromVert, label_conf); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNlabel, LabelAUDIO); n++;
  label_audio = XtCreateManagedWidget("LabelAudio", labelWidgetClass, 
				      form5, args, n);

  /*
  *  Create the Scroll Bar Window.
  */
  n = 0;
  XtSetArg(args[n], XtNright, main_form); n++;
  XtSetArg(args[n], XtNallowVert, TRUE); n++;
  if(UNICAST){
    XtSetArg(args[n], XtNheight, 50); n++;
  }else{
    XtSetArg(args[n], XtNheight, 200); n++;
  }
  XtSetArg(args[n], XtNfromVert, label_hosts); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainBottom); n++;
  view_list = XtCreateManagedWidget("ViewList", viewportWidgetClass,
				        form5, args, n);
  n = 0;
  form_list = XtCreateManagedWidget("FormList", formWidgetClass,
				    view_list, args, n);
  
  n = 0;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  host_name[0] = XtCreateManagedWidget("MyHostName", labelWidgetClass, 
				       form_list, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, host_name[0]); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  button_d_video[0] = XtCreateManagedWidget("ButtonDVideo", 
				      commandWidgetClass, form_list, args, n);
  XtAddCallback(button_d_video[0], XtNcallback, CallbackDVideo, NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_d_video[0]); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  button_d_audio[0] = XtCreateManagedWidget("ButtonDAudio", 
				       commandWidgetClass, form_list, args, n);
  XtAddCallback(button_d_audio[0], XtNcallback, CallbackDAudio, NULL);

  if(UNICAST){
    char head[100];
    char my_hostname[80];

    strcpy(head, name_user);
    strcat(head, "@");
    if(gethostname(my_hostname, 80) < 0)
      perror("gethostname");
    else
      strcat(head, my_hostname);
    ManageStation(t, head, 0, (FEEDBACK ? PORT_VIDEO_CONTROL : 0), FALSE, 
		  FALSE, TRUE);
    XtSetArg(args[0], XtNwidth, 270); 
    XtSetValues(host_name[0], args, 1);
  }

  for(i=1; i<N_MAX_STATIONS; i++){
    n = 0;
    XtSetArg(args[n], XtNfromVert, host_name[i-1]); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    host_name[i] = XtCreateManagedWidget("HostName", labelWidgetClass, 
					 form_list, args, n);
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, host_name[i]); n++;
    XtSetArg(args[n], XtNfromVert, button_d_video[i-1]); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNleft, XtChainRight); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    button_d_video[i] = XtCreateManagedWidget("ButtonDVideo", 
				      commandWidgetClass, form_list, args, n);
    XtAddCallback(button_d_video[i], XtNcallback, CallbackDVideo, NULL);

    n = 0;
    XtSetArg(args[n], XtNfromHoriz, button_d_video[i]); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNleft, XtChainRight); n++;
    XtSetArg(args[n], XtNfromVert, button_d_audio[i-1]); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    button_d_audio[i] = XtCreateManagedWidget("ButtonDAudio", 
				       commandWidgetClass, form_list, args, n);
    XtAddCallback(button_d_audio[i], XtNcallback, CallbackDAudio, NULL);
  }
    
  /*
  * Create the bitmap for marking selected items. 
  */

  mark = XCreateBitmapFromData(XtDisplay(toplevel),
			       RootWindowOfScreen(XtScreen(toplevel)),
			       Mark_bits, Mark_width, Mark_height);


  XtAppAddInput(app_con, csock_r, XtInputReadMask, GetControlInfo, NULL);

  SendCSDESC(csock_s, &addr_s, len_addr_s, START_EVIDEO, START_EAUDIO, 
	     my_feedback_port, my_sin_addr, name_user);
  if(FEEDBACK){
      SendHELLO(csock_s, &addr_s, len_addr_s);
  }

  /* By default ... */

  size = CIF_SIZE; MarkMenu(menu_video[1]);
  COLOR=FALSE; 
#ifdef VIDEOPIX
  MarkMenu(menu_video[3]);
#endif
  rate_control = WITHOUT_CONTROL; MarkMenu(menu_rate[0]);
  XtSetSensitive(form_control, FALSE);
  MarkMenu(menu_rate[3]);
#ifdef VIDEOPIX
  vfc_port_video = VFC_PORT1; MarkMenu(menu_videopix[0]);
#endif
#ifdef INDIGO  
  audio_format = ADPCM_32; MarkMenu(menu_eaudio[1]);
#else
#ifdef HPUX
  audio_format = PCM_64; MarkMenu(menu_eaudio[0]);
#else
  audio_format = VADPCM; MarkMenu(menu_eaudio[2]);
#endif /* HPUX */
#endif /* INDIGO */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);

  alarm((u_int)T_ALARM);
  signal(SIGALRM, LoopDeclare);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app_con);
}


