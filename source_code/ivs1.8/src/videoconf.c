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
*  File    : videoconf.c    	                			   *
*  Date    : September 1992   						   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description : CODEC audio + video.                                      *
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
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Scale.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/Label.h>


#ifdef VIDEOPIX
#include <vfc_lib.h>
#endif

#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "quantizer.h"
#include "protocol.h"

#define ecreteL(x) ((x)>255 ? 255 : (u_char)(x))
#define ecreteC(x) ((x)>255.0 ? 255 : ((x)<0 ? 0 :(u_char)(x)))

static Widget video_cif, video_qcif,  ButtonVideo, ButtonAudio, 
  video_rate, video_quality, ScaleVol, ScaleRec, ScaleBri, ScaleCon, 
  ButtonDefault, label[3], MainForm, LabelsForm, form[N_MAX_STATIONS], 
  BVideo[N_MAX_STATIONS], BAudio[N_MAX_STATIONS], LabelHost[N_MAX_STATIONS], 
  audio_entry, video_entry, audio_pcm64, audio_adpcm32, audio_vadpcm,
  video_cutting, video_nocutting, temp, msg_err;


LOCAL_TABLE t;
char display[50];
int lig=288, col=352; /* Default is CIF format */
int quality=1; /* Boolean: true if we privilege quality and not rate */
char IP_GROUP[16], AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
char name_user[L_cuserid];
struct sockaddr_in addr_s, addr_r;
int csock_s, csock_r; 
int len_addr_s, len_addr_r;
BOOLEAN START_EAUDIO=FALSE, START_EVIDEO=FALSE;
BOOLEAN QCIF2CIF=TRUE;
BOOLEAN CUTTING=FALSE;
int AUDIO_ENCODING=ADPCM_32;
int ea_fd[2], ev_fd[2], da_fd[2], dv_fd[N_MAX_STATIONS][2], *scaledv_fd;
u_char luth[256];
u_char ttl=DEFAULT_TTL;
FILE *F_log;
int statusp;
BOOLEAN audio_set=FALSE;
BOOLEAN UNICAST=FALSE;
int T_ALARM=20; /* a Declaration packet is sent every T_ALARM seconds */
struct sockaddr_in my_sin_addr;
BOOLEAN STAT_MODE=FALSE;




/***************************************************************************
 * Procedures
 **************************************************************************/

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
  fprintf(stderr, 
"Usage: %s [-i @IP] [-n user] [-t ttl] [-d display] [-o] [-stat] [hostname]\n"
	  , name);
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


void AbortScalesVideo()
{
  
  UnMappWidget(ScaleBri);
  UnMappWidget(ScaleCon);
  UnMappWidget(ButtonDefault);
  
}



static void CreateForm(number, LastWidget, LastForm, name, video_encoding, 
		       audio_encoding, video_decoding, audio_decoding)
     int number;
     Widget *LastWidget, *LastForm;
     char *name;
     BOOLEAN video_encoding, audio_encoding;
     BOOLEAN video_decoding, audio_decoding;
{
  int n;
  XmString xms;
  Arg args[10];
  extern void CallbackDVideo(), CallbackDAudio();
  
  n = 0;
  XtSetArg (args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopWidget, *LastWidget); n++;
  XtSetArg (args[n], XmNtopOffset, 10); n++;
  XtSetArg (args[n], XmNbottomOffset, 5); n++;
  XtSetArg (args[n], XmNleftOffset, 2); n++;
  XtSetArg (args[n], XmNrightOffset, 2); n++;
  form[number] = XmCreateForm(*LastForm, "form", args, n);
  XtManageChild(form[number]);
  
  n = 0;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg (args[n], XmNrightPosition, 70); n++;
  xms = XmStringCreate(name, XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNlabelString, xms); n++;
  LabelHost[number] = XmCreateLabel(form[number], "texte", args, n);
  XmStringFree(xms);
  XtManageChild(LabelHost[number]);
  
  n = 0;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++; 
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftWidget, LabelHost[number]); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg (args[n], XmNrightPosition, 85); n++;
  if(!video_encoding){
    XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  }
  BVideo[number] = XmCreatePushButton (form[number], (video_decoding ? "Yes" :
						      "No"), args, n);
  XtAddCallback (BVideo[number], XmNactivateCallback, CallbackDVideo, NULL);
  
  
  n = 0;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftWidget, BVideo[number]); n++;
  if(!audio_encoding){
    XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  }
  BAudio[number] = XmCreatePushButton (form[number], (audio_decoding ? "Yes" :
						      "No"), args, n);
  XtAddCallback (BAudio[number], XmNactivateCallback, CallbackDAudio, NULL);
  
  XtManageChild(BVideo[number]);
  XtManageChild(BAudio[number]);
  XtManageChild(form[number]);
}


void CreateTabForm()
{
  register int i, lasti=0;
  Widget *LastWidget, *LastForm;
  Arg args[1];
  
  LastWidget = &(label[0]);
  LastForm = &(LabelsForm);
  for(i=0; i<N_MAX_STATIONS; i++){
    if(t[i].valid){
      CreateForm(i, LastWidget, LastForm, t[i].name, t[i].video_encoding, 
		 t[i].audio_encoding, t[i].video_decoding, t[i].audio_decoding);
      lasti = i;
      LastWidget = &(LabelHost[lasti]);
      LastForm = &(form[lasti]);
    }
  }
  XtSetArg(args[0], XmNbottomAttachment, XmATTACH_FORM);
  XtSetValues(form[lasti], args, 1);
  XtSetValues(LabelHost[lasti], args, 1);
  XtSetValues(BVideo[lasti], args, 1);
  XtSetValues(BAudio[lasti], args, 1);
}


void DestroyTabForm()
{
  register int i;
  
  for(i=0; i<N_MAX_STATIONS; i++){
    if(t[i].valid){
      if(BAudio[i])
	XtUnmanageChild(BAudio[i]);
      if(BVideo[i])
	XtUnmanageChild(BVideo[i]);
      if(LabelHost[i])
	XtUnmanageChild(LabelHost[i]);
      if(form[i])
	XtUnmanageChild(form[i]);
    }
  }
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

    if((lwrite=write(scaledv_fd[1], (char *)luth, 256)) < 0){
      fprintf(stderr, "CallbackBri: error when writing on pipe\n");
      exit(1);
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

    if((lwrite=write(scaledv_fd[1], (char *)luth, 256)) < 0){
      fprintf(stderr, "CallbackCon: error when writing on pipe\n");
      exit(1);
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
    if((lwrite=write(scaledv_fd[1], (char *)luth, 256)) < 0){
      fprintf(stderr, "CallbackDefault: error when writing on pipe\n");
      exit(1);
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


static void CallbackRec(w, null, reason)
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
  soundrecgain(value);
#elif DECSTATION
  sprintf(com, "/usr/bin/aset -gain input %d", value);
  system(com);
#endif
}





static void CallbackCutting(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(!CUTTING){
      ChangeColor(video_cutting);
      ChangeColor(video_nocutting);
      CUTTING=TRUE;
    }
  }

static void CallbackNoCutting(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(CUTTING){
      ChangeColor(video_cutting);
      ChangeColor(video_nocutting);
      CUTTING=FALSE;
    }
  }

static void CallbackCIF(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(lig != 288){
      ChangeColor(video_cif);
      ChangeColor(video_qcif);
      lig=288;
      col=352;
    }
  }

static void CallbackQCIF(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(lig == 288){
      ChangeColor(video_cif);
      ChangeColor(video_qcif);
      lig=144;
      col=176;
    }
  }


static void CallbackPCM64(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(AUDIO_ENCODING != PCM_64){
      switch(AUDIO_ENCODING){
      case ADPCM_32:
	ChangeColor(audio_adpcm32);
	break;
      case VADPCM:
	ChangeColor(audio_vadpcm);
	break;
      }
      ChangeColor(audio_pcm64);
      AUDIO_ENCODING = PCM_64;
    }
  }


static void CallbackADPCM32(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(AUDIO_ENCODING != ADPCM_32){
      switch(AUDIO_ENCODING){
      case PCM_64:
	ChangeColor(audio_pcm64);
	break;
      case VADPCM:
	ChangeColor(audio_vadpcm);
	break;
      }
      ChangeColor(audio_adpcm32);
      AUDIO_ENCODING = ADPCM_32;
    }
  }


static void CallbackVADPCM(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(AUDIO_ENCODING != VADPCM){
      switch(AUDIO_ENCODING){
      case PCM_64:
	ChangeColor(audio_pcm64);
	break;
      case ADPCM_32:
	ChangeColor(audio_adpcm32);
	break;
      }
      ChangeColor(audio_vadpcm);
      AUDIO_ENCODING = VADPCM;
    }
  }



static void CallbackQuality(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(!quality){
      ChangeColor(video_rate);
      ChangeColor(video_quality);
      quality = 1;
    }
  }


static void CallbackRate(w, null, reason)
    Widget w;
    char * null;
    XmPushButtonCallbackStruct *reason;
  {
    if(quality){
      ChangeColor(video_rate);
      ChangeColor(video_quality);
      quality = 0;
    }
  }

 
/*static*/ void CallbackDVideo(w, null, reason)
     Widget w;
     char *null;
     XmPushButtonCallbackStruct *reason;
{
  register int i;
  int idv;
  
  for(i=0; i<N_MAX_STATIONS; i++){
    if((t[i].valid) && (w==BVideo[i])){
      break;
    }
  }
  if(t[i].video_decoding == FALSE){
    if(pipe(dv_fd[i]) < 0){
      DisplayMsgError("CallbackDVideo: Cannot create pipe");
      return;
    }
    if((idv=fork()) == -1){
      DisplayMsgError("CallbackDVideo: Cannot fork decoding video process");
      return;
    }
    if(!idv){
      close(dv_fd[i][1]);
      VideoDecode(dv_fd[i], VIDEO_PORT, IP_GROUP, t[i].name, t[i].sin_addr,
		  display, QCIF2CIF);
    }else{
      t[i].video_decoding=TRUE;
      ChangeButton(BVideo[i], "Yes");
      scaledv_fd = dv_fd[i];
      InitScalesVideo();
    }
  }else{
    SendPP_QUIT(dv_fd[i]);
    t[i].video_decoding=FALSE;
    ChangeButton(BVideo[i], "No");
    AbortScalesVideo();
    waitpid(0, &statusp, WNOHANG);
  }
}



/*static*/ void CallbackDAudio(w, null, reason)
     Widget w;
     char *null;
     XmPushButtonCallbackStruct *reason;
{
  register int i;
  int id;
  
  if(audio_set==FALSE){
#ifdef SPARCSTATION
    /* If the audio device is busy, don't fork any process */
    if(!soundinit(O_WRONLY)){
      DisplayMsgError("audio_decode: Cannot open audio device");
      return;
    }
    soundterm();
#endif /* SPARCSTATION */
    audio_set=TRUE;
    if(pipe(da_fd) < 0){
      DisplayMsgError("audio_decode: Cannot create pipe");
      return;
    }
    if((id=fork()) == -1){
      DisplayMsgError("Too many processes, cannot fork audio decoding process"
		      );
      return;
    }
    if(!id){
      close(da_fd[1]);
      Audio_Decode(da_fd, AUDIO_PORT, IP_GROUP);
    }else
      close(da_fd[0]);
  }
  for(i=0; i<N_MAX_STATIONS; i++){
    if((t[i].valid) && (w==BAudio[i])){
      break;
    }
  }
  if(t[i].audio_decoding == FALSE){
    t[i].audio_decoding=TRUE;
    ChangeButton(BAudio[i], "Yes");
  }else{
    t[i].audio_decoding=FALSE;
    ChangeButton(BAudio[i], "No");
  }
  SendPP_NEW_TABLE(t, da_fd);
}




static void CallbackVideo(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    int id;
    int ok;
#ifdef VIDEOPIX
    int format;
    int video_port;
    extern int vfc_scan_ports();
    VfcDev *vfc_dev;
#endif

    if(!START_EVIDEO){
      if(pipe(ev_fd) < 0){
	DisplayMsgError("CallbackVideo: Cannot create pipe");
	return;
      }

      ok = 0;
#ifdef VIDEOPIX
      if((vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV)) == NULL){
	DisplayMsgError("CallbackVideo: Cannot open hardware VideoPix");
	return;
      }
      else ok = 1;
#endif
#ifdef INDIGOVIDEO
      if (!sgi_checkvideo()) {
	DisplayMsgError("Sorry, this Indigo has no video capture");
	return;
      }
      else ok = 1;
#endif
      if (!ok) {
	DisplayMsgError("Sorry, this IVS is configured without video capture");
	return;
      }

      /* XXX if you move this statement after the fork (in the parent),
	 XXX you don't have to UnMapp on all the error exits: */
      UnMappWidget(ButtonVideo);
#ifdef VIDEOPIX
      if ((video_port=vfc_scan_ports(vfc_dev, &format)) == -1) {
	DisplayMsgError(
		       "CallbackVideo: No input video available on any port");
	MappWidget(ButtonVideo);
	vfc_destroy(vfc_dev);
	return;
      }
#endif

      if((id=fork()) == -1){
	DisplayMsgError(
     "CallbackVideo: Too many processes, Cannot fork video decoding process");
	MappWidget(ButtonVideo);
#ifdef VIDEOPIX
	vfc_destroy(vfc_dev);
#endif
	return;
      }
      /* fathers's process goes on , son's process starts encoding */
      if(id){
	START_EVIDEO=TRUE;
	close(ev_fd[0]);
	UnMappWidget(video_entry);
	SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
			  START_EAUDIO, name_user);
	ChangeButton(ButtonVideo, "Stop Encoding Video");
	MappWidget(ButtonVideo);
      }else{
	close(ev_fd[1]);
#ifdef VIDEOPIX
	video_encode(IP_GROUP, VIDEO_PORT, ttl, col, lig, quality, ev_fd, 
		     CUTTING, video_port, vfc_dev, STAT_MODE);
#endif
#ifdef INDIGOVIDEO
	sgi_video_encode(IP_GROUP, VIDEO_PORT, ttl, col, lig, quality, ev_fd, 
		     CUTTING, STAT_MODE);
#endif
      }
    }else{
      SendPP_QUIT(ev_fd);
      START_EVIDEO=FALSE;
      MappWidget(video_entry);
      SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
			START_EAUDIO, name_user);
      ChangeButton(ButtonVideo, "Start Encoding Video");
      waitpid(0, &statusp, WNOHANG);
    }
  }

    
    
static void CallbackAudio(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    u_char COM=0x01;
    int id;

    if(!START_EAUDIO){
#ifdef SPARCSTATION
      /* If the audio device is busy, don't fork any process */
      if(!soundinit(O_RDONLY)){
	DisplayMsgError("CallbackAudio: Cannot initialize audio device");
	return;
      }
      soundterm();
#endif /* SPARCSTATION */
      if(pipe(ea_fd) < 0){
	DisplayMsgError("CallbackAudio: Cannot create pipe");
	return;
      }
      if((id=fork()) == -1){
	DisplayMsgError(
      "CallbackAudio: Too many processes, Cannot fork audio encoding process");
	return;
      }
      /* fathers's process goes on , son's process starts encoding */
      if(id){
	START_EAUDIO=TRUE;
	close(ea_fd[0]);
	SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
			  START_EAUDIO, name_user);

	ChangeButton(ButtonAudio, "Stop Encoding Audio");
	MappWidget(ScaleRec);
	UnMappWidget(audio_entry);
	XmScaleSetValue(ScaleRec, 80);
#ifdef SPARCSTATION
	soundrecgain(80);
#endif
      }else{
	close(ea_fd[1]);
	audio_encode(IP_GROUP, AUDIO_PORT, ttl, AUDIO_ENCODING, ea_fd);
      }
    }else{
      SendPP_QUIT(ea_fd);
      START_EAUDIO=FALSE;
      SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
			START_EAUDIO, name_user);
      ChangeButton(ButtonAudio, "Start Encoding Audio");
      UnMappWidget(ScaleRec);
      MappWidget(audio_entry);
      waitpid(0, &statusp, WNOHANG);
    }
  }

    
    
static void CallbackQuit(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    register int i;

    if(STAT_MODE)
      fclose(F_log);
    SendAbortPacket(csock_s, &addr_s, len_addr_s);
    SendPP_QUIT(da_fd);
    for(i=0; i<N_MAX_STATIONS; i++)
      if(t[i].valid && t[i].video_decoding)
	SendPP_QUIT(dv_fd[i]);
    if(START_EAUDIO)
      SendPP_QUIT(ea_fd);
    if(START_EVIDEO)
      SendPP_QUIT(ev_fd);
    exit(0);
  }


static void CallbackDeclare()
{
  struct timeval realtime;
  double time;

  gettimeofday(&realtime, (struct timezone *)NULL);
  time = realtime.tv_sec + realtime.tv_usec/1000000.0;
  CheckStations(t, (int)time);
  SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
		    START_EAUDIO, name_user);
  alarm(T_ALARM);
  signal(SIGALRM, CallbackDeclare);
}

static void GetControlInfo(t)
     LOCAL_TABLE t;
{

  int type, lr;
  struct hostent *host;
  char buf[1000];
  struct sockaddr_in from;
  int fromlen;

  fromlen=sizeof(from);
  bzero((char *)&from, fromlen);
  if ((lr = recvfrom(csock_r, (char *)buf, 1000, 0, &from, &fromlen)) < 0){
    perror("receive from");
    return;
  }
  if((type=GetType(buf)) != CONTROL_TYPE){
      host=gethostbyaddr((char *)(&(from.sin_addr)), fromlen, from.sin_family);
#ifdef DEBUG
    fprintf(stderr, "Received a packet type %d from %s\n", type, host->h_name);
#endif
    return;
  }else{
    int subtype;
    BOOLEAN video_encoding, audio_encoding;
    char name[100];
    u_short *pt;
    int res;
    int num;
    BOOLEAN myself=FALSE;
    
    subtype = GetSubType(buf);
    switch(subtype){
    case P_DECLARATION:
      host=gethostbyaddr((char *)(&(from.sin_addr)), fromlen, from.sin_family);
      video_encoding = ((buf[0] & 0x02) ? TRUE : FALSE);
      audio_encoding = ((buf[0] & 0x01) ? TRUE : FALSE);
      strcpy(name, (char *) &(buf[1]));
      if(my_sin_addr.sin_addr.s_addr == from.sin_addr.s_addr)
	myself=TRUE;
      strcat(name, "@");
      strcat(name, host->h_name);
      if(res=AddStation(t, name, from.sin_addr.s_addr, video_encoding, 
		    audio_encoding, &num, myself)){
	if(res && STAT_MODE)
	  PrintTable(F_log, t);
	if(res & NEW_STATION){
	  DestroyTabForm();
	  CreateTabForm(t);
	}
	if(res & MAPP_AUDIO){
	  MappWidget(BAudio[num]);
	  if(myself){
	    ChangeButton(BAudio[num], "No");
	  }else{
	    if(audio_set==FALSE){
	      int id;

#ifdef SPARCSTATION
	      /* If the audio device is busy, don't fork any process */
	      if(!soundinit(O_WRONLY)){
		DisplayMsgError("audio_decode: Cannot open audio device");
		return;
	      }
	      soundterm();
#endif /* SPARCSTATION */
	      audio_set=TRUE;
	      if(pipe(da_fd) < 0){
		DisplayMsgError("audio_decode: Cannot create pipe");
		return;
	      }
	      if((id=fork()) == -1){
		DisplayMsgError(
       "audio_decode: Too many processes, Cannot fork audio decoding process");
		return;
	      }
	      if(!id){
		close(da_fd[1]);
		Audio_Decode(da_fd, AUDIO_PORT, IP_GROUP);
	      }else
		close(da_fd[0]);
	    }
	    t[num].audio_decoding=TRUE;
	    ChangeButton(BAudio[num], "Yes");
	    SendPP_NEW_TABLE(t, da_fd);
	  }
	}
	if((res & NEW_STATION) == 0){
	  if(res & UNMAPP_AUDIO)
	    UnMappWidget(BAudio[num]);
	  if(res & ABORT_AUDIO)
	    SendPP_NEW_TABLE(t, da_fd);
	  if(res & UNMAPP_VIDEO)
	    UnMappWidget(BVideo[num]);
	  if(res & ABORT_VIDEO){
	    AbortScalesVideo();
	    SendPP_QUIT(dv_fd[num]);
	  }else{
	    if(res & MAPP_VIDEO){
	      MappWidget(BVideo[num]);
	      ChangeButton(BVideo[num], "No");
	    }
	  }
	}
      }
      break; 

    case P_ASK_INFO:
#ifdef DEBUG
      printf("Received an askInfo packet\n");
#endif
      SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
			START_EAUDIO, name_user);
      break;
      
    case P_ABORT:
      if(res=RemoveStation(t, from.sin_addr.s_addr, &num)){
	if(res & ABORT_AUDIO)
	  SendPP_NEW_TABLE(t, da_fd);
	if(res & ABORT_VIDEO){
	  AbortScalesVideo();
	  SendPP_QUIT(dv_fd[num]);
	}
	if(res & KILL_STATION){
	  DestroyTabForm();
	  CreateTabForm(t);
	}
      }
      break;

      default: ;
#ifdef DEBUG
      fprintf(stderr, "GetControlInfo: Received a control packet type: %d\n",
	      subtype);
#endif
    }
  }
  waitpid(0, &statusp, WNOHANG);
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
  Widget  appShell, main_win, DialogShell, dwa, menubar, input_menu, 
  output_menu, video_menu, input_entry, output_entry, ButtonQuit, 
  audio_menu, audio_host, audio_port;
  Arg     args[10];
  int  n = 0;
  FILE *F_aux;
  static char name[50]=VIDEOCONF_VERSION;
  char *pt = name;
  register int i;
  XmString xms;
  int nb_arg=1;
  int file_def=0, host_def=0, port_def=0;
  int narg;
  int lr;
  BOOLEAN new_ip_group=FALSE;
  char ntemp[256], hostname[256];
  

  strcpy(display, "");
  cuserid(name_user);
  if(argc > 10)
    Usage(argv[0]);
  narg = 1;
  while(narg != arg){
    if(strcmp(argv[narg], "-i") == 0){
      strcpy(IP_GROUP, argv[++narg]);
      narg ++;
      new_ip_group = TRUE;
      continue;
    }else{
      if(strcmp(argv[narg], "-n") == 0){
	strncpy(name_user, argv[++narg], L_cuserid);
	narg ++;
	continue;
      }else{
	if(strcmp(argv[narg], "-d") == 0){
	  strcpy(display, argv[++narg]);
	  narg ++;
	  continue;
	}else{
	  if(strcmp(argv[narg], "-o") == 0){
	    QCIF2CIF=FALSE;
	    narg ++;
	    continue;
	  }else{
	    if(strcmp(argv[narg], "-t") == 0){
	      ttl = atoi(argv[++narg]);
	      narg ++;
	      continue;
	    }else{
	      if(strcmp(argv[narg], "-stat") == 0){
		STAT_MODE=TRUE;
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
    if((F_aux=fopen(".videoconf.default","w")) != NULL){
      fprintf(F_aux, "AUDIO_PORT:\t\t%s\n", AUDIO_PORT);
      fprintf(F_aux, "VIDEO_PORT:\t\t%s\n", VIDEO_PORT);
      fprintf(F_aux, "CONTROL_PORT:\t\t%s\n", CONTROL_PORT);
      fprintf(F_aux, "IP_GROUP:\t\t%s\n", IP_GROUP);
    }
    fclose(F_aux);
  }
  {
    /* getting my sin_addr */

    struct hostent *hp;
    int namelen=100;
    char name[100];

    if(gethostname(name, namelen) != 0){
      perror("gethostname");
      exit(1);
    }
    if((hp=gethostbyname(name)) == 0){
	fprintf(stderr, "%d-Unknown host\n", getpid());
	exit(-1);
      }
    bcopy(hp->h_addr, (char *)&my_sin_addr.sin_addr, 4);
  }
  if(UNICAST){
    unsigned int i1, i2, i3, i4;
    struct hostent *hp;
    struct sockaddr_in taddr;

    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "%d-Unknown host %s\n", getpid(), hostname);
	exit(-1);
      }
      bcopy(hp->h_addr, (char *)&taddr.sin_addr, 4);
      strcpy(IP_GROUP, inet_ntoa(taddr.sin_addr));
    }else
      strcpy(IP_GROUP, hostname);
  }

  /* Initializations */

  
  InitTable(t);
  csock_r=CreateReceiveSocket(&addr_r, &len_addr_r, CONTROL_PORT, IP_GROUP);
  csock_s=CreateSendSocket(&addr_s, &len_addr_s, CONTROL_PORT, IP_GROUP, &ttl);
#ifdef SPARCSTATION
  if(!sounddevinit()){
    fprintf(stderr, "cannot opening audioctl device");
  }
#endif
  if(STAT_MODE){
    if((F_log=fopen(".videoconf.log","w")) == NULL)
      fprintf(stderr, "cannot create videoconf.log file\n");
  }
  XtToolkitInitialize();
  app = XtCreateApplicationContext();
  disp = XtOpenDisplay(app, display,
		       "VideoConf",    /* application name */
		       "VIDEOCONF",    /* application class */
		       NULL, 0,        /* options */
		       &arg, argv);    /* command line parameters */
  if(!disp){
    fprintf(stderr, "Unable to open display %s, trying unix:0\n", display);
    strcpy(display, "unix:0");
    disp = XtOpenDisplay(app, display,
			 "VideoConf",    /* application name */
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
/*XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;*/
  dwa = XmCreateDrawingArea (MainForm, "dwa", args, n);

  n = 0;
  menubar = XmCreateMenuBar(main_win, "nom du menu bar", args, n);
  
  video_menu = XmCreatePulldownMenu(menubar, "video", args, 0);
  n = 0;
  XtSetArg(args[n], XmNsubMenuId, video_menu); n++;
  video_entry = XmCreateCascadeButton (menubar, "Video Encoding", args, n);
  n = 0;
  video_cif = XtCreateManagedWidget ("CIF", xmPushButtonWidgetClass,
				     video_menu, args, n);
  video_qcif = XtCreateManagedWidget ("Quarter CIF", xmPushButtonWidgetClass,
				      video_menu, args, n);
  video_quality = XtCreateManagedWidget ("Privilege Quality", 
				xmPushButtonWidgetClass, video_menu, args, n);
  video_rate = XtCreateManagedWidget ("Privilege Rate", 
				xmPushButtonWidgetClass, video_menu, args, n);
  video_cutting = XtCreateManagedWidget ("One packet <-> One GOB", 
				xmPushButtonWidgetClass, video_menu, args, n);
  video_nocutting = XtCreateManagedWidget ("Two packets <-> One Image", 
				xmPushButtonWidgetClass, video_menu, args, n);

  audio_menu = XmCreatePulldownMenu(menubar, "audio", args, 0);
  n = 0;
  XtSetArg(args[n], XmNsubMenuId, audio_menu); n++;
  audio_entry = XmCreateCascadeButton (menubar, "Audio Encoding", args, n);
  n = 0;
  audio_pcm64 = XtCreateManagedWidget ("PCM (64 kb/s)", 
				xmPushButtonWidgetClass, audio_menu, args, n);
  audio_adpcm32 = XtCreateManagedWidget ("ADPCM (32 kb/s)", 
				xmPushButtonWidgetClass, audio_menu, args, n);
  audio_vadpcm = XtCreateManagedWidget ("ADPCM (variable)", 
				xmPushButtonWidgetClass, audio_menu, args, n);


  XtAddCallback (video_cif, XmNactivateCallback, CallbackCIF, NULL);
  XtAddCallback (video_qcif, XmNactivateCallback, CallbackQCIF, NULL);
  XtAddCallback (video_quality, XmNactivateCallback, CallbackQuality, NULL);
  XtAddCallback (video_rate, XmNactivateCallback, CallbackRate, NULL);
  XtAddCallback (video_cutting, XmNactivateCallback, CallbackCutting, NULL);
  XtAddCallback (video_nocutting, XmNactivateCallback, CallbackNoCutting, 
		 NULL);
  XtAddCallback (audio_pcm64, XmNactivateCallback, CallbackPCM64, NULL);
  XtAddCallback (audio_adpcm32, XmNactivateCallback, CallbackADPCM32, NULL);
  XtAddCallback (audio_vadpcm, XmNactivateCallback, CallbackVADPCM, NULL);

  n = 0;
  XtSetArg (args[n], XmNdialogType, XmDIALOG_ERROR); n++;
  msg_err = XmCreateMessageBox(DialogShell, "MSG_ERR", args, n);
  temp = (Widget)XmMessageBoxGetChild(msg_err, XmDIALOG_CANCEL_BUTTON);
  XtUnmanageChild(temp);
  temp = XmMessageBoxGetChild(msg_err, XmDIALOG_HELP_BUTTON);
  XtUnmanageChild(temp);

  n = 0;
  XtSetArg (args[n], XmNx, 300); n++;
  XtSetArg (args[n], XmNy, 220); n++;
  ButtonQuit = XmCreatePushButton (dwa, "Quit", args, n);
  XtAddCallback (ButtonQuit, XmNactivateCallback, CallbackQuit, NULL);


  n = 0;
  XtSetArg (args[n], XmNx, 10); n++;
  XtSetArg (args[n], XmNy, 170); n++;
  ButtonVideo = XmCreatePushButton (dwa, "Start Encoding Video", args, n);
  XtAddCallback (ButtonVideo, XmNactivateCallback, CallbackVideo, NULL);

  n = 0;
  XtSetArg (args[n], XmNx, 190); n++;
  XtSetArg (args[n], XmNy, 170); n++;
  ButtonAudio = XmCreatePushButton (dwa, "Start Encoding Audio", args, n);
  XtAddCallback (ButtonAudio, XmNactivateCallback, CallbackAudio, NULL);

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
  xms = XmStringCreate("Record Gain", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNtitleString, xms); n++;
  XtSetArg (args[n], XmNx, 220); n++;
  XtSetArg (args[n], XmNy, 70); n++;
  XtSetArg (args[n], XmNwidth, 100); n++;
#ifdef DECSTATION
  XtSetArg (args[n], XmNmaximum, 36); n++;
#endif
  XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg (args[n], XmNshowValue, True); n++;
  XtSetArg (args[n], XmNmappedWhenManaged, FALSE); n++;
  ScaleRec = XmCreateScale (dwa, "scaleRec", args, n);
  XmStringFree(xms);
  XtAddCallback (ScaleRec, XmNdragCallback, CallbackRec, NULL);

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

  n = 0;
  XtSetArg (args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopWidget, dwa); n++;
  XtSetArg (args[n], XmNtopOffset, 2); n++;
  XtSetArg (args[n], XmNbottomOffset, 20); n++;
  XtSetArg (args[n], XmNleftOffset, 2); n++;
  XtSetArg (args[n], XmNrightOffset, 2); n++;
  LabelsForm = XmCreateForm(MainForm, "formtitle", args, n);

  n = 0;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg (args[n], XmNrightPosition, 70); n++;
  xms = XmStringCreate("CONFERENCE HOSTS DECODING", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNlabelString, xms); n++;
  label[0] = XmCreateLabel(LabelsForm, "texte", args, n);
  XmStringFree(xms);
    
  n = 0;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftWidget, label[0]); n++;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg (args[n], XmNrightPosition, 85); n++;
  xms = XmStringCreate("VIDEO", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNlabelString, xms); n++;
  label[1] = XmCreateLabel(LabelsForm, "texte", args, n);
  XmStringFree(xms);

  n = 0;
  XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg (args[n], XmNleftWidget, label[1]); n++;
  xms = XmStringCreate("AUDIO", XmSTRING_DEFAULT_CHARSET);
  XtSetArg (args[n], XmNlabelString, xms); n++;
  label[2] = XmCreateLabel(LabelsForm, "texte", args, n);
  XmStringFree(xms);

  XtAppAddInput(app, csock_r, XtInputReadMask, GetControlInfo, t);

  SendDeclarePacket(csock_s, &addr_s, len_addr_s, START_EVIDEO, 
		    START_EAUDIO, name_user);
  SendAskInfoPacket(csock_s, &addr_s, len_addr_s);

  if(UNICAST){
    int num;
    char head[100];

    strcpy(head, " UNICAST MODE : sending to ");
    strcat(head, hostname);
    AddStation(t, head, 0, FALSE, FALSE, &num, TRUE);
    if(STAT_MODE)
      PrintTable(F_log, t);
    DestroyTabForm();
    CreateTabForm(t);
  }

  XtManageChild (label[2]);
  XtManageChild (label[1]);
  XtManageChild (label[0]);
  XtManageChild (LabelsForm);
  XtManageChild (ButtonDefault);
  XtManageChild (ScaleCon);
  XtManageChild (ScaleBri);
  XtManageChild (ScaleRec);
  XtManageChild (ScaleVol);
  XtManageChild (ButtonAudio);
  XtManageChild (ButtonVideo);
  XtManageChild (ButtonQuit);
  XtManageChild (audio_entry);
  XtManageChild (video_entry);
  XtManageChild (menubar);
  XtManageChild (dwa);
  XtManageChild (MainForm);
  XtManageChild (DialogShell);
  XtManageChild(main_win);
  XtRealizeWidget(appShell);

  ChangeColor(video_cif);
  ChangeColor(video_nocutting);
  ChangeColor(audio_adpcm32);
  ChangeColor(video_quality);

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, CallbackQuit);
  signal(SIGQUIT, CallbackQuit);

  alarm(T_ALARM);
  signal(SIGALRM, CallbackDeclare);

  XtAppMainLoop(app);

}

