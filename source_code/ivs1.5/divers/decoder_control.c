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
*                                                                          *
*  File    : decoder_control.c                                             *
*  Date    : May 1992                                                      *
*  Author  : Thierry Turletti                                              *
*  Release : 1.0.0                                                         *
*--------------------------------------------------------------------------*
*  Description : Modifies brightness and contrast with two Motif's scales. *
*                 Commands are "Play", "Loop", "Stop" and "Quit".          *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/Scale.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include "h261.h"

#define ecreteL(x) ((x)>255 ? 255 : (x))
#define ecreteC(x) ((x)>255.0 ? 255 : ((x)<0 ? 0 :(int)(x)))


extern XColor colors[256];
extern u_char map_lut[256];
extern int fd[2];
extern int fdd[2];

Widget scaleL, scaleC;
int memo[256];


/******************************************************************************
 * Callbacks
 *****************************************************************************/

static void CallbackL(w, null, reason)
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
	memo[i] = (int)(L*aux);
	map_lut[i] = (u_char) colors[memo[i]].pixel;
      }
    }else{
      if(L<100){
	int aux1=coeff3*L;

	for(i=0; i<256; i++){
	  memo[i] = ecreteL((int)((255*i)/(512.0-aux1)));
	    map_lut[i] = (u_char) colors[memo[i]].pixel;
	}
      }else
	  for(i=0; i<256; i++){
	    memo[i] = 255;
	    map_lut[i] = (u_char) colors[255].pixel;
	  }
    }

    if((lwrite=write(fd[1], (char *)map_lut, 256)) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
  }

	      
static void CallbackC(w, null, reason)
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
      memo[i] = ecreteC(aux*memo[i]+aux1);
      map_lut[i] = (u_char) colors[memo[i]].pixel;
    }

    if((lwrite=write(fd[1], (char *)map_lut, 256)) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
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

    XmScaleSetValue(scaleL, 71);
    XmScaleSetValue(scaleC, 50);
    for(i=0; i<256; i++){
      memo[i] = ecreteL((int)((255*i)/150));
      map_lut[i] = (u_char) colors[memo[i]].pixel;
    }
    if((lwrite=write(fd[1], (char *)map_lut, 256)) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
  }
    
static void CallbackQuit(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    u_char com=QUIT;
    
    if(write(fdd[1], (char *)&com, 1) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
    exit(0);
  }


static void CallbackPlay(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    u_char com=PLAY;

    if(write(fdd[1], (char *)&com, 1) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
  }
    
static void CallbackLoop(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    u_char com=LOOP;

    if(write(fdd[1], (char *)&com, 1) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
  }
    
static void CallbackStop(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    u_char com=STOP;

    if(write(fdd[1], (char *)&com, 1) < 0){
      printf(stderr, "scale_nb : error lwrite < 0\n");
      exit(1);
    }
  }
    
    

/**************************************************
 *  Main program
 **************************************************/

int New_Luth(TCP, server)
    int TCP; /* Boolean: true if mode TCP */
    char *server;
  {
    int argc=1;
    Widget  appShell, form, dwa, button_q, button_d, button_l, button_r,
            button_s, label_1, label_2, label_3, label_4, main_win, 
            dialog_Shell;
    Arg     args[10];
    int  n = 0;
    static char name[50]="VideoControl Panel";
    char *pt = name;
    register int i;

    for(i=0; i<256; i++)
	memo[i] = ecreteL((int)((255*i)/150));
    appShell = XtInitialize("net", "H261", NULL, 0, &argc, &pt);

    main_win = XmCreateMainWindow(appShell, "main_app", NULL, 0);

    n = 0;
    dialog_Shell = XmCreateDialogShell(appShell, "dialog", args, n);

    n = 0;
    XtSetArg (args[n], XmNwidth, 210); n++;
    XtSetArg (args[n], XmNheight, 170); n++;
    form = XmCreateForm (main_win, "main", args, n);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    dwa = XmCreateDrawingArea (form, "dwa", args, n);

    n = 0;
    XtSetArg (args[n], XmNx, 100); n++;
    XtSetArg (args[n], XmNy, 40); n++;
    XtSetArg (args[n], XmNwidth, 100); n++;
    XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg (args[n], XmNshowValue, True); n++;
    scaleL = XmCreateScale (dwa, "scaleL", args, n);
    XtAddCallback (scaleL, XmNdragCallback, CallbackL, map_lut);
    XmScaleSetValue(scaleL, 71);

    n = 0;
    XtSetArg (args[n], XmNx, 100); n++;
    XtSetArg (args[n], XmNy, 75); n++;
    XtSetArg (args[n], XmNwidth, 100); n++;
    XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg (args[n], XmNshowValue, True); n++;
    scaleC = XmCreateScale (dwa, "scaleC", args, n);
    XtAddCallback (scaleC, XmNdragCallback, CallbackC, map_lut);
    XmScaleSetValue(scaleC, 50);

    n = 0;
    XtSetArg (args[n], XmNx, 160); n++;
    XtSetArg (args[n], XmNy, 140); n++;
    button_q = XmCreatePushButton (dwa, "Quit", args, n);
    XtAddCallback (button_q, XmNactivateCallback, CallbackQuit, NULL);

    n = 0;
    XtSetArg (args[n], XmNx, 120); n++;
    XtSetArg (args[n], XmNy, 0); n++;
    button_d = XmCreatePushButton (dwa, "Default", args, n);
    XtAddCallback (button_d, XmNactivateCallback, CallbackDefault, NULL);

    if(!TCP){
      n = 0;
      XtSetArg (args[n], XmNx, 10); n++;
      XtSetArg (args[n], XmNy, 140); n++;
      button_r = XmCreatePushButton (dwa, "Play", args, n);
      XtAddCallback (button_r, XmNactivateCallback, CallbackPlay, NULL);
      
      n = 0;
      XtSetArg (args[n], XmNx, 60); n++;
      XtSetArg (args[n], XmNy, 140); n++;
      button_l = XmCreatePushButton (dwa, "Loop", args, n);
      XtAddCallback (button_l, XmNactivateCallback, CallbackLoop, NULL);

      n = 0;
      XtSetArg (args[n], XmNx, 110); n++;
      XtSetArg (args[n], XmNy, 140); n++;
      button_s = XmCreatePushButton (dwa, "Stop", args, n);
      XtAddCallback (button_s, XmNactivateCallback, CallbackStop, NULL);

    }else{
      static char first[]="Video from ";
      char name[50];
      XmString xms;
      Widget mesg, child;

      strcpy(name, first);
      strcat(name, server);
      n = 0;
      xms = XmStringCreate(name, XmSTRING_DEFAULT_CHARSET);
      XtSetArg (args[n], XmNmessageString, xms); n++;
      XtSetArg (args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
      mesg = (Widget)XmCreateMessageBox(dialog_Shell, "mesg", args, n);
      child = (Widget)XmMessageBoxGetChild(mesg, XmDIALOG_CANCEL_BUTTON);
      XtUnmanageChild(child);
      child = (Widget)XmMessageBoxGetChild(mesg, XmDIALOG_HELP_BUTTON);
      XtUnmanageChild(child);
      XtManageChild(mesg);
    }

    n = 0;
    XtSetArg (args[n], XmNx, 20); n++;
    XtSetArg (args[n], XmNy, 12); n++;
    label_1= XmCreateLabel (dwa, "TUNING", args, n);

    n = 0;
    XtSetArg (args[n], XmNx, 10); n++;
    XtSetArg (args[n], XmNy, 55); n++;
    label_2= XmCreateLabel (dwa, "Brightness", args, n);

    n = 0;
    XtSetArg (args[n], XmNx, 20); n++;
    XtSetArg (args[n], XmNy, 90); n++;
    label_3= XmCreateLabel (dwa, "Contrast", args, n);

    n = 0;
    XtSetArg (args[n], XmNx, 65); n++;
    XtSetArg (args[n], XmNy, 117); n++;
    label_4= XmCreateLabel (dwa, "COMMANDS", args, n);

    XtManageChild (label_4);
    XtManageChild (label_3);
    XtManageChild (label_2);
    XtManageChild (label_1);
    if(!TCP){
      XtManageChild (button_s);
      XtManageChild (button_l);
      XtManageChild (button_r);
    }
    XtManageChild (button_d);
    XtManageChild (button_q);
    XtManageChild (scaleC);
    XtManageChild (scaleL);
    XtManageChild (dwa);
    XtManageChild (form);
    XtManageChild (dialog_Shell);
    XtManageChild (main_win);
    XtRealizeWidget(appShell);
    
    XtMainLoop();
}


