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
*  File    : h261_encode.c    	                			   *
*  Date    : May 1992       						   *
*  Author  : Thierry Turletti						   *
*  Release : 1.0.0						           *
*--------------------------------------------------------------------------*
*  Description : H.261 coder program.                                      *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/Text.h>

#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "quantizer.h"

#define COM_STOP 1

#define INPUT_H261       0
#define INPUT_FILE       1
#define INPUT_CAMERA     2

#define NO_SOUND         0
#define UNCOMPRESS_SOUND 1
#define COMPRESS_SOUND   2

static Widget input_f_box, Hostname_box, Port_number_box, output_f_box,
    mode_cif, mode_qcif, output_network, input_file, output_file, input_camera,
    input_h261, input_h261_box, button_r, para_rate, para_quality,
    para_ucsound, para_csound, para_nosound;


int TCP=1; /* Boolean : true if TCP output */
int SNR=0; /* Boolean : true if snr process */
int num_port; /* TCP port number */
char port_number[5]; /*TCP port number in ASCII */
char host[20]; /* Decoder Host */
int lig=288, col=352; /* Default is CIF format */
static char file_in[40]="VIDEOPIX", file_out[40], file_h261[40];
int START=0; /* Boolean : true if coding is on the way */
int id;
int fd[2];
int QUANT_FIX=3, seuil=13, rate=2, snr=0;
int mode;
int quality=1; /* Boolean: true if we privilege quality and not rate */
int audio=UNCOMPRESS_SOUND; 

/***************************************************************************
 * Procedures
 **************************************************************************/

usage(name)
char *name;
  {
    fprintf(stderr, "Usage : %s [-h hostname] [-p port_number]\n", name);
    exit(1);
  }

static void ChangeButton (widget, name)
    Widget         widget;
    char *name;
  {
    Arg   args[10];
    XmString xms;
    int  n = 0;

    n = 0;
    xms = XmStringCreate(name, XmSTRING_DEFAULT_CHARSET);
    XtSetArg (args[n], XmNlabelString, xms); n++;
    XtSetValues (widget, args, n);
  }


static void PreviewButton()
  {
    switch (mode){
      case INPUT_FILE:
        ChangeButton(button_r, "Read&Code&Write");
	break;
      case INPUT_H261:
        ChangeButton(button_r, "Read&Write");
        break;
      default:
        ChangeButton(button_r, "Record&Code&Write");
        break;
    }
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



static void CallbackInputFile(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    XtManageChild(input_f_box);
  }

static void CallbackH261InputFile(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    XtManageChild(input_h261_box);
  }

static void GetInputFilename(w, null, reason)
    Widget w;
    char *null;
    XmSelectionBoxCallbackStruct *reason;
  {
    char *pt;

    pt = (char *)malloc (reason->length + 1);
    XmStringGetLtoR(reason->value, XmSTRING_DEFAULT_CHARSET, &pt);
    strcpy(file_in, pt);
    switch (mode){
      case INPUT_H261:
        ChangeColor(input_h261); ChangeColor(input_file);
	break;
      case INPUT_CAMERA:
	ChangeColor(input_camera); ChangeColor(input_file);
	break;
      default:
	break;
    }
    mode = INPUT_FILE;
    ChangeButton(button_r, "Read&Code&Write");
  }
    

static void GetH261Filename(w, null, reason)
    Widget w;
    char *null;
    XmSelectionBoxCallbackStruct *reason;
  {
    char *pt;

    pt = (char *)malloc (reason->length + 1);
    XmStringGetLtoR(reason->value, XmSTRING_DEFAULT_CHARSET, &pt);
    strcpy(file_h261, pt);
    switch (mode){
      case INPUT_FILE:
        ChangeColor(input_file); ChangeColor(input_h261);
	break;
      case INPUT_CAMERA:
	ChangeColor(input_camera); ChangeColor(input_h261);
	break;
      default:
	break;
    }
    mode = INPUT_H261;
    ChangeButton(button_r, "Read&Write");
  }
    

static void CallbackCamera(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {

    strcpy(file_in, "VIDEOPIX");
    switch (mode){
      case INPUT_FILE:
        ChangeColor(input_file); ChangeColor(input_camera);
	break;
      case INPUT_H261:
	ChangeColor(input_camera); ChangeColor(input_h261);
	break;
      default:
	break;
    }
    mode = INPUT_CAMERA;
    ChangeButton(button_r, "Record&Code&Write");
  }

static void GetHostname(w, null, reason)
    Widget w;
    char *null;
    XmSelectionBoxCallbackStruct *reason;
  {
    char *pt;

    pt = (char *)malloc (reason->length + 1);
    XmStringGetLtoR(reason->value, XmSTRING_DEFAULT_CHARSET, &pt);
    strcpy(host, pt);
  }
    

static void GetPortNumber(w, null, reason)
    Widget w;
    char *null;
    XmSelectionBoxCallbackStruct *reason;
  {
    char *pt;

    pt = (char *)malloc (reason->length + 1);
    XmStringGetLtoR(reason->value, XmSTRING_DEFAULT_CHARSET, &pt);
    num_port = atoi(pt);
  }
    

static void CallbackNetwork(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(!TCP){
      TCP=1;
      ChangeColor(output_network);
      ChangeColor(output_file);
    }
  }

static void CallbackOutputFile(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(mode != INPUT_H261){
      XtManageChild(output_f_box);
      if(TCP){
	ChangeColor(output_network);
	ChangeColor(output_file);
	TCP=0;
      }
    }
  }

static void GetOutputFilename(w, null, reason)
    Widget w;
    char *null;
    XmSelectionBoxCallbackStruct *reason;
  {
    char *pt;

    pt = (char *)malloc (reason->length + 1);
    XmStringGetLtoR(reason->value, XmSTRING_DEFAULT_CHARSET, &pt);
    strcpy(file_out, pt);
    TCP=0;
  }
    

static void CallbackCIF(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(lig != 288){
      ChangeColor(mode_cif);
      ChangeColor(mode_qcif);
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
      ChangeColor(mode_cif);
      ChangeColor(mode_qcif);
      lig=144;
      col=176;
    }
  }


static void CallbackUCSound(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    switch(audio){
    case COMPRESS_SOUND : ChangeColor(para_csound); ChangeColor(para_ucsound);
                          audio=UNCOMPRESS_SOUND; break;
    case NO_SOUND       : ChangeColor(para_nosound);
                          ChangeColor(para_ucsound); audio=UNCOMPRESS_SOUND;
    }
  }


static void CallbackCSound(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    switch(audio){
    case UNCOMPRESS_SOUND :ChangeColor(para_ucsound);ChangeColor(para_csound);
                           audio=COMPRESS_SOUND; break;
    case NO_SOUND         :ChangeColor(para_nosound);
                           ChangeColor(para_csound); audio=COMPRESS_SOUND;
    }
  }


static void CallbackNoSound(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    switch(audio){
    case UNCOMPRESS_SOUND : ChangeColor(para_ucsound);
                            ChangeColor(para_nosound); audio=NO_SOUND; break;
    case COMPRESS_SOUND :   ChangeColor(para_csound);
                            ChangeColor(para_nosound); audio=NO_SOUND;
    }
  }


static void CallbackQuality(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(!quality){
      ChangeColor(para_rate);
      ChangeColor(para_quality);
      quality = 1;
    }
  }


static void CallbackRate(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    if(quality){
      ChangeColor(para_rate);
      ChangeColor(para_quality);
      quality = 0;
    }
  }


static void CallbackHost(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    XtManageChild(Hostname_box);
  }


static void CallbackPort(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    XtManageChild(Port_number_box);
  }



static void CallbackRecord(w, argv, reason)
    Widget w;
    char * argv;
    XmPushButtonCallbackStruct *reason;
  {
    u_char COM=0x01;

    if(!START){
      START=1;
      if(pipe(fd) < 0){
	fprintf(stderr, "menu : Cannot create pipe\n");
	exit(1);
      }
      if((id=fork()) == -1){
	fprintf(stderr,"%s : Cannot fork !\n");
	exit(1);
      }
      /* fathers's process goes on , son's process starts encoding */
      if(id){
	close(fd[0]);
	ChangeButton(button_r, "Stop");
      }else{
	close(fd[1]);
	if(mode == INPUT_H261){

	  FILE *F_h261;
	  char buf[1024];
	  int lread, mask, sock;
	  struct timeval tim_out;

	  tim_out.tv_sec = 0;
	  tim_out.tv_usec = 0;
/*	  sock = tcp_connect(host, num_port);*/
	  if((F_h261=fopen(file_h261, "r"))==NULL){
	    fprintf(stderr, "Cannot open file %s\n", file_h261);
	    exit(1);
	  }
	  do{
	    if((lread=fread(buf, 1, 1024, F_h261)) <= 0)
		break;
	    if(send(sock, buf, lread, 0) != lread){
	      perror("send returns ");
	      exit(1);
	    }
	    mask = 1 << fd[0];
	    if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, 
		      (fd_set *)0, &tim_out) != 0)
		break;
	  }while(lread==1024);
	  close(sock);
	  close(F_h261);
	  exit(0);

	}else
	    encode_sequence(file_in, file_out, host, num_port, col, lig, 
			    seuil, QUANT_FIX, TCP, snr, rate, quality, audio);
      }
    }else{
      write(fd[1],(char *)&COM, 1);
      START=0;
      PreviewButton();
    }

  }
    
    
static void CallbackQuit(w, null, reason)
    Widget w;
    char*  null;
    XmPushButtonCallbackStruct *reason;
  {
    u_char com=0x01;

    if(write(fd[1],(char *)&com, 1) < 0)
	exit(1);
    else
	exit(0);
  }






/**************************************************
 *  Main program
 **************************************************/

main(arg, argv)
int arg;
char **argv;
  {
    int argc=1;
    Widget  appShell, main_win, form, dwa, menubar, input_menu, output_menu,
            mode_menu, input_entry, output_entry, mode_entry, 
            button_q, dialog_Shell, child, para_menu, 
            para_entry, para_host, para_port;
    Arg     args[10];
    int  n = 0;
    FILE *F_aux;
    static char name[50]="H.261 Coder Panel";
    char temp[40];
    char *pt = name;
    register int i;
    XmString xms;
    int nb_arg=1;
    int file_def=0, host_def=0, port_def=0;
 

    if(arg > 5)
	usage(argv[0]);

    if((F_aux=fopen("h261_default.var","r"))!=NULL){
      file_def=1;
      fscanf(F_aux, "%s%s", temp, port_number);
      fscanf(F_aux, "%s%s", temp, host);
      fclose(F_aux);
    }

    while(arg > nb_arg){
      if(strcmp(argv[nb_arg], "-h") == 0){
	strcpy(host, argv[nb_arg+1]);
	host_def=1;
      }else{
	if(strcmp(argv[nb_arg], "-p") == 0){
	  strcpy(port_number, argv[nb_arg+1]);
	  port_def=1;
	}else
	    usage(argv[0]);
      }
      nb_arg += 2;
    }
    if(!file_def){
      if(!host_def)
	  fprintf(stderr, 
   	     "%s Warning : hostname destination isn't initialized\n", argv[0]);
      if(!port_def)
	  fprintf(stderr, 
          "%s Warning : port number destination isn't initialized\n", argv[0]);
    }
    num_port = atoi (port_number);

    appShell = XtInitialize("net", "H261", NULL, 0, &argc, &pt);

    main_win = XmCreateMainWindow(appShell, "main_app", NULL, 0);

    n = 0;
    dialog_Shell = XmCreateDialogShell(main_win, "dialog", args, n);
    
    n = 0;
    XtSetArg (args[n], XmNwidth, 170); n++;
    XtSetArg (args[n], XmNheight, 100); n++;
    form = XmCreateForm (main_win, "main", args, n);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    dwa = XmCreateDrawingArea (form, "dwa", args, n);

    n = 0;
    menubar = XmCreateMenuBar(main_win, "nom du menu bar", args, n);

    input_menu = XmCreatePulldownMenu(menubar, "input", args, 0);
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, input_menu); n++;
    input_entry = XmCreateCascadeButton (menubar, "Input", args, n);
    n = 0;
#ifdef VIDEOPIX
    input_camera = XtCreateManagedWidget ("Camera", xmPushButtonWidgetClass,
					  input_menu, args, n);
#endif
    input_file = XtCreateManagedWidget ("Input file ..", 
				xmPushButtonWidgetClass, input_menu, args, n);
    input_h261 = XtCreateManagedWidget ("H.261 file ..", 
				 xmPushButtonWidgetClass, input_menu, args, n);

    output_menu = XmCreatePulldownMenu(menubar, "output", args, 0);
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, output_menu); n++;
    output_entry = XmCreateCascadeButton (menubar, "Output", args, n);
    n = 0;
    output_network = XtCreateManagedWidget ("Network", xmPushButtonWidgetClass,
					    output_menu, args, n);
    output_file = XtCreateManagedWidget ("Output file ..", 
			        xmPushButtonWidgetClass, output_menu, args, n);

    mode_menu = XmCreatePulldownMenu(menubar, "mode", args, 0);
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, mode_menu); n++;
    mode_entry = XmCreateCascadeButton (menubar, "Format", args, n);
    n = 0;
    mode_cif = XtCreateManagedWidget ("CIF", xmPushButtonWidgetClass,
					  mode_menu, args, n);
    mode_qcif = XtCreateManagedWidget ("Quarter CIF", xmPushButtonWidgetClass,
					  mode_menu, args, n);

    para_menu = XmCreatePulldownMenu(menubar, "parameters", args, 0);
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, para_menu); n++;
    para_entry = XmCreateCascadeButton (menubar, "Parameters", args, n);
    n = 0;
    para_host = XtCreateManagedWidget ("Host Name ..", xmPushButtonWidgetClass,
				       para_menu, args, n);
    para_port = XtCreateManagedWidget ("Port Number ..", 
				  xmPushButtonWidgetClass, para_menu, args, n);
    para_quality = XtCreateManagedWidget ("Privilege Quality", 
 			       xmPushButtonWidgetClass, para_menu, args, n);
    para_rate = XtCreateManagedWidget ("Privilege Rate", 
 			       xmPushButtonWidgetClass, para_menu, args, n);
    para_ucsound = XtCreateManagedWidget ("Uncompressed Audio", 
				xmPushButtonWidgetClass, para_menu, args, n);
    para_csound = XtCreateManagedWidget ("Compressed Audio", 
				xmPushButtonWidgetClass, para_menu, args, n);
    para_nosound = XtCreateManagedWidget ("Only Video", 
		               xmPushButtonWidgetClass, para_menu, args, n);


    XtAddCallback (input_h261, XmNactivateCallback, CallbackH261InputFile, 
		   NULL);
    XtAddCallback (input_file, XmNactivateCallback, CallbackInputFile, NULL);
#ifdef VIDEOPIX
    XtAddCallback (input_camera, XmNactivateCallback, CallbackCamera, NULL);
#endif
    XtAddCallback (output_network, XmNactivateCallback, CallbackNetwork, NULL);
    XtAddCallback (output_file, XmNactivateCallback, CallbackOutputFile, NULL);
    XtAddCallback (mode_cif, XmNactivateCallback, CallbackCIF, NULL);
    XtAddCallback (mode_qcif, XmNactivateCallback, CallbackQCIF, NULL);
    XtAddCallback (para_host, XmNactivateCallback, CallbackHost, NULL);
    XtAddCallback (para_port, XmNactivateCallback, CallbackPort, NULL);
    XtAddCallback (para_quality, XmNactivateCallback, CallbackQuality, NULL);
    XtAddCallback (para_rate, XmNactivateCallback, CallbackRate, NULL);
    XtAddCallback (para_ucsound, XmNactivateCallback, CallbackUCSound, NULL);
    XtAddCallback (para_csound, XmNactivateCallback, CallbackCSound, NULL);
    XtAddCallback (para_nosound, XmNactivateCallback, CallbackNoSound, NULL);


    n = 0;
    xms = XmStringCreate("Input Filename :", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNselectionLabelString, xms); n++;
    input_f_box = (Widget)XmCreatePromptDialog(dialog_Shell, 
				       "Inputfilename", args, n);
    child = (Widget)XmSelectionBoxGetChild(input_f_box, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(child);
    XtAddCallback (input_f_box, XmNokCallback, GetInputFilename, NULL);
    

    n = 0;
    xms = XmStringCreate("H.261 Filename :", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNselectionLabelString, xms); n++;
    input_h261_box = (Widget)XmCreatePromptDialog(dialog_Shell, 
				       "H261filename", args, n);
    child = (Widget)XmSelectionBoxGetChild(input_h261_box,
					   XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(child);
    XtAddCallback (input_h261_box, XmNokCallback, GetH261Filename, NULL);
    

    n = 0;
    xms = XmStringCreate("Output Filename :", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNselectionLabelString, xms); n++;
    output_f_box = (Widget)XmCreatePromptDialog(dialog_Shell, 
					       "Outputfilename", args, n);
    child = (Widget)XmSelectionBoxGetChild(output_f_box, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(child);
    XtAddCallback (output_f_box, XmNokCallback, GetOutputFilename, NULL);
    

    n = 0;
    xms = XmStringCreate("TCP Port Number :", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNselectionLabelString, xms); n++;
    xms = XmStringCreate(port_number, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNtextString, xms); n++;
    Port_number_box = (Widget)XmCreatePromptDialog(dialog_Shell, 
					       "TCPPortNumber", args, n);
    child = (Widget)XmSelectionBoxGetChild(Port_number_box, 
					   XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(child);
    XtAddCallback (Port_number_box, XmNokCallback, GetPortNumber, NULL);
    

    n = 0;
    xms = XmStringCreate("Destination Hostname :", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNselectionLabelString, xms); n++;
    xms = XmStringCreate(host, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(args[n], XmNtextString, xms); n++;
    Hostname_box = (Widget)XmCreatePromptDialog(dialog_Shell, 
					       "Hostname", args, n);
    child = (Widget)XmSelectionBoxGetChild(Hostname_box, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(child);
    XtAddCallback (Hostname_box, XmNokCallback, GetHostname, NULL);
    

    n = 0;
    XtSetArg (args[n], XmNx, 200); n++;
    XtSetArg (args[n], XmNy, 60); n++;
    button_q = XmCreatePushButton (dwa, "Quit", args, n);
    XtAddCallback (button_q, XmNactivateCallback, CallbackQuit, NULL);


    n = 0;
    XtSetArg (args[n], XmNx, 10); n++;
    XtSetArg (args[n], XmNy, 60); n++;
    button_r = XmCreatePushButton (dwa, "Record&Code&Write", args, n);
    XtAddCallback (button_r, XmNactivateCallback, CallbackRecord, NULL);

    XtManageChild (button_r);
    XtManageChild (button_q);
    XtManageChild (para_entry);
    XtManageChild (mode_entry);
    XtManageChild (output_entry);
    XtManageChild (input_entry);
    XtManageChild (menubar);
    XtManageChild (dwa);
    XtManageChild (form);
    XtManageChild(dialog_Shell);
    XtManageChild(main_win);
    XtRealizeWidget(appShell);

    /* Defaults are ... */
#ifdef VIDEOPIX
    mode = INPUT_CAMERA;
    ChangeColor(input_camera);
#else
    mode = INPUT_H261;
    ChangeColor(input_h261);
#endif
    ChangeColor(mode_cif);
    ChangeColor(para_ucsound);
    ChangeColor(output_network);
    ChangeColor(para_quality);

    signal(SIGPIPE, SIG_IGN);
    XtMainLoop();
}

