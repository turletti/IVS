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
*  File              : athena.c    	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/4/5                                            *
*--------------------------------------------------------------------------*
*  Description       : Athena toolkit.                                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Pierre de la Motte  |  93/1/6   |   Add to Display Control Panel :     *
*                       |           |    - color and grey buttons          *
*                       |           |    - double, regular and half buttons*
*                       |           |                                      *
\**************************************************************************/



#include <stdio.h>
#include <string.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/SimpleMenu.h>

#include "general_types.h"
#include "protocol.h"
#include "athena.h"
#include "rtp.h"

#ifdef CRYPTO
#include "ivs_secu.h"
#endif

#define MODCOLOR	4
#define MODGREY		2
#define MODNB		1

#define streq(a, b)        ( strcmp((a), (b)) == 0 )

#define LabelReleaseAudio      "Release \n audio "
#define LabelAntiLarsenOn      "Echo \n off  "
#define LabelAntiLarsenOff     "Echo \n on "

/*
**		EXPORTED VARIABLES
*/

Widget main_form;
Widget msg_error;
Widget video_coder_panel;
Widget video_grabber_panel;
Widget audio_panel;
Widget control_panel;
Widget info_panel;
Widget form_control, scroll_control, value_control;
Widget form_frms, form_format;
Widget label_band2, label_ref2;
Widget button_popup1, button_popup2, button_popup3, button_popup4;
Widget button_popup5;
Widget colorButton, greyButton;
Widget doubleButton, regularButton, halfButton;
Widget value_fps, value_kbps, value_loss;
BOOLEAN RECV_ONLY; /* True if only reception is allowed */
BOOLEAN MICRO_MUTED;
BOOLEAN AUDIO_DROPPED=FALSE;
BOOLEAN SPEAKER, MICROPHONE;
BOOLEAN AUTO_MUTING; /* True if loudspeaker is muted while speaking */
BOOLEAN AUDIO_INIT_FAILED=FALSE;
BOOLEAN WITHOUT_ANY_AUDIO;
Widget button_r_audio, button_larsen;
Widget input_audio, output_audio;
Widget button_ptt;
int output_speaker=A_SPEAKER; /* A_SPEAKER, A_HEADPHONE or A_EXTERNAL */
int input_microphone=A_MICROPHONE; /* A_MICROPHONE or A_LINE_IN */
int volume_gain;
int record_gain;
S_PARAM param;
Pixmap LabelSpeaker, LabelHeadphone, LabelSpeakerExt;
Pixmap LabelMicrophone, LabelLineIn;
#ifdef SCREENMACHINE
int current_sm; /* Current ScreenMachine II board number selected */
int nb_sm; /* Number of ScreenMachine II in the platform */
#endif

/*
**		IMPORTED VARIABLES
*/
/*----- from display.c ---------*/
 
extern int cur_brght;
extern float cur_ctrst;
extern int cur_int_ctrst;

extern void rebuild_map_lut();
extern XtCallbackProc affColorCB();
extern XtCallbackProc affGreyCB();
extern XtCallbackProc doubleCB();
extern XtCallbackProc regularCB();
extern XtCallbackProc halfCB();

extern XtCallbackProc display_panel();

/*
**		GLOBAL VARIABLES
*/

static Widget mainform_coder, form_scales;
static Widget button_default;
static Widget form_brightness, scroll_brightness, name_brightness;
static Widget value_brightness, form_contrast, scroll_contrast;
static Widget name_contrast, value_contrast;
static Widget form_color, button_color, button_grey;
static Widget form_size, button_qcif, button_cif, button_scif;
static Widget form_freeze, button_freeze, button_unfreeze;
static Widget form_local, button_local, button_nolocal;
static Widget mainform_grabber;
static Widget form_board, button_board1, button_board2, button_board3;
static Widget button_board4;
static Widget form_port, button_port[6];
static Widget button_format[8];

static Widget mainform_control;
static Widget form_audio, button_audio[AUDIO_FORMAT_LEN];
static Widget form_redondance, button_redondance[AUDIO_REDONDANCE_LEN];
static Widget form_rate, button_rate[3];
static Widget form_frame, button_frame[2];
static Widget form_mode, button_mode[2];
static Widget form_bw;
static Widget name_control;
static Widget form_fr;
static Widget scroll_frms, name_frms, value_frms;
static Widget mainform_audio, form_audio_tuning; 
static Widget form_v_audio, scroll_v_audio, name_v_audio, value_v_audio;
static Widget form_r_audio, scroll_r_audio, name_r_audio, value_r_audio;
#ifdef SCREENMACHINE
static Widget form_field, button_field[4];
static Widget form_sm, button_sm[4];
#endif /* SCREENMACHINE */
#ifdef CRYPTO
extern void NewKey();
Widget form_key, label_key, key_value;

static char defaultTranslations[] = 
      "Ctrl<Key>J:   no-op() \n \
       Ctrl<Key>M:   no-op() \n \
       Ctrl<Key>N:   no-op() \n \
       <Key>Return:  NewKey()";
#endif /* CRYPTO */
static BOOLEAN AUTO_FREEZE_CONTROL=FALSE;
static void CallbackFrameControl();

/******************************************\
*                                          *
*   CALL BACKS AND WIDGET MANAGEMENT       *
*                                          *
\******************************************/


static XtCallbackProc brightnessCB(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int L;
  Arg args[1];
  char value[5];
  
  L = (int)(128.0*top - 64);
  if (L == cur_brght) return;
  sprintf(value, "%d", L);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  cur_brght = L;
  rebuild_map_lut();
}


static XtCallbackProc contrastCB(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int C;
  Arg args[1];
  char value[6];
  
  C = (int)(100*top - 50);
  if (C == cur_int_ctrst) return;
  cur_int_ctrst = C;
  cur_ctrst = (float) pow ((double)2.0, (double) ((double) (cur_int_ctrst) 
						  / 25.0));
  sprintf(value, "%.2f", cur_ctrst);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  rebuild_map_lut();
}

void DestroyWarning(w, client_data, call_data)
     Widget w;
     XtPointer client_data, call_data;
{
  Widget popup;

  popup = XtParent((Widget) client_data);
  XtDestroyWidget(popup);
}
  


void ShowWarning(msg)
     char *msg;
{
  Arg         args[5];
  Widget      popup;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  extern Widget main_form, msg_error;

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);

  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;

  popup = XtCreatePopupShell("Warning", transientShellWidgetClass, main_form,
			     args, n);

  n = 0;
  XtSetArg(args[n], XtNlabel, msg); n++;
  msg_error = XtCreateManagedWidget("MsgError", dialogWidgetClass, popup,
				    args, n);

  XawDialogAddButton(msg_error, "OK", DestroyWarning, 
		     (XtPointer) msg_error);
  XtPopup(popup, XtGrabNone);
}


void ReverseColor(widget)
     Widget widget;
{
  Arg args[10];
  Pixel bg_color, fg_color;
  Cardinal n;

  n = 0;
  XtSetArg(args[n], XtNbackground, &bg_color); n++;
  XtSetArg(args[n], XtNforeground, &fg_color); n++;
  XtGetValues(widget, args, n);
  
  n = 0;
  XtSetArg(args[n], XtNbackground, fg_color); n++;
  XtSetArg(args[n], XtNforeground, bg_color); n++;
  XtSetValues(widget, args, n);
}


void Valid(widget)
     Widget widget;
{
  ReverseColor(widget);
}


void Unvalid(widget)
     Widget widget;
{
  ReverseColor(widget);
}


void ChangeLabel(widget, name)
     Widget widget;
     char *name;
{
  Arg args[1];

  XtSetArg (args[0], XtNlabel, name);
  XtSetValues (widget, args, 1);
}


void ChangeAndMappLabel(widget, name)
     Widget widget;
     char *name;
{
  Arg args[2];
  
  XtSetArg (args[0], XtNlabel, name);
  XtSetArg (args[1], XtNmappedWhenManaged, TRUE);
  XtSetValues (widget, args, 2);
}


void ChangeAndMappPixmap(widget, name)
     Widget widget;
     Pixmap name;
{
  Arg args[2];
  
  XtSetArg (args[0], XtNbitmap, name);
  XtSetArg (args[1], XtNmappedWhenManaged, TRUE);
  XtSetValues (widget, args, 2);
}


void ChangePixmap(widget, name)
     Widget widget;
     Pixmap name;
{
  Arg args[1];

  XtSetArg (args[0], XtNbitmap, name);
  XtSetValues (widget, args, 1);
}

void SetSensitiveVideoFormat(board, value)
    int board, value;
{
#if defined(PARALLAX) || defined (VIGRAPIX)
    register int i;

    switch(board) {
    case PARALLAX_BOARD:
	for(i=0; i<PX_FORMAT_LEN-3; i++) 
	    XtSetSensitive(button_format[i], value);
	break;
    case VIGRAPIX_BOARD:
	for(i=0; i<PX_FORMAT_LEN-3; i++) 
	    XtSetSensitive(button_format[i], FALSE);
	XtSetSensitive(button_format[AUTO_PAL-3], value);
	XtSetSensitive(button_format[SECAM-3], value);
	break;
    default:
	for(i=0; i<PX_FORMAT_LEN-3; i++)
	    XtSetSensitive(button_format[i], FALSE);
	break;
    }
#endif /* PARALLAX || VIGRAPIX */
    return;
}



void SetSensitiveVideoOptions(value)
     BOOLEAN value;
{

#if defined(VIDEOPIX) || defined(SCREENMACHINE)
  if(param.board_selected == VIDEOPIX_BOARD || 
     param.board_selected == SCREENMACHINE_BOARD){
    XtSetSensitive(form_scales, !value);
  }else{
    XtSetSensitive(form_scales, FALSE);
  }
#endif
  XtSetSensitive(form_size, value);
  XtSetSensitive(form_color, value);
  XtSetSensitive(form_board, value);
  switch(param.board_selected){
  case PARALLAX_BOARD:
  case VIGRAPIX_BOARD:
    SetSensitiveVideoFormat(param.board_selected, value);
    break;
  case INDIGO_BOARD:
  case GALILEO_BOARD:
  case INDY_BOARD:
/*    XtSetSensitive(form_port, value);*/
    XtSetSensitive(button_scif, FALSE);
    break;
  default:
    XtSetSensitive(button_scif, FALSE);
  }
}



void SetSensitiveVideoTuning(value)
     BOOLEAN value;
{
  extern Widget form_brightness, form_contrast, button_default;

  XtSetSensitive(form_brightness, value);
  XtSetSensitive(form_contrast, value);
  XtSetSensitive(button_default, value);
}



void CreateScale(form, scroll, name, value, parent, FormScale, 
		 NameScale, val_default)
     Widget *form, *scroll, *name, *value, parent;
     char *FormScale, *NameScale;
     int val_default;
{
  Cardinal n;
  Arg args[10];
  float top;
  XtArgVal *l_top;
  char value_default[4];

  sprintf(value_default, "%d", val_default);
  /*
  * Create the form for the scale.
  */
  *form = XtCreateManagedWidget(FormScale, formWidgetClass, parent, NULL, 0);

  n = 0;
  top = 0.5;
  if(sizeof(float) > sizeof(XtArgVal)){
    XtSetArg(args[n], XtNtopOfThumb, &top); n++;
  }else{
    l_top = (XtArgVal *) &top;
    XtSetArg(args[n], XtNtopOfThumb, *l_top); n++;
  }
  *scroll = XtCreateManagedWidget("ScrollScale", scrollbarWidgetClass,
				  *form, args, n);
  top = (float)val_default / 100.0;
  XawScrollbarSetThumb(*scroll, top, 0.05);

  n = 0;
  XtSetArg(args[n], XtNfromVert, *scroll); n++;
  *name = XtCreateManagedWidget(NameScale, labelWidgetClass, *form, 
				     args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, value_default); n++;
  XtSetArg(args[n], XtNfromHoriz, *name); n++;
  XtSetArg(args[n], XtNfromVert, *scroll); n++;
  XtSetArg(args[n], XtNjustify, XtJustifyLeft); n++;
  XtSetArg(args[n], XtNinternalWidth, 6); n++;
  *value = XtCreateManagedWidget("ValueScale", labelWidgetClass, *form, 
				 args, n);

}
  
void Dismiss(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  XtSetMappedWhenManaged(XtParent(XtParent(XtParent(w))), FALSE);
}



void InitScale(scroll, value, val_default)
     Widget scroll, value;
     int val_default;
{
  Arg args[1];
  float top;
  char str_default[4];

  sprintf(str_default, "%d", val_default);
  XtSetArg(args[0], XtNlabel, str_default);
  XtSetValues(value, args, 1);

  top = (float)val_default / 100.0;
  XawScrollbarSetThumb(scroll, top, 0.05);
}

void InitScaleScreenMachine(scroll, value, val_default)
     Widget scroll, value;
     int val_default;
{
  Arg args[1];
  float top;
  char str_default[4];

  sprintf(str_default, "%d", val_default);
  XtSetArg(args[0], XtNlabel, str_default);
  XtSetValues(value, args, 1);

#ifndef PC
  top = (float)(val_default) / 100.0;
  XawScrollbarSetThumb(scroll, top, 0.05);
#endif
}


void CreatePanel(title, panel, parent)
     char *title;
     Widget *panel;
     Widget parent;
{
  Arg args[15];
  Cardinal n;
  char str_default[6];
  XColor couleur[5], bidon;
  Display *dpy = XtDisplay (parent);
  int screen = DefaultScreen(dpy);
  Colormap palette = DefaultColormap(dpy, screen);
  Widget mainform;
  Widget form_brightness, scroll_brightness, name_brightness;
  Widget value_brightness, form_contrast, scroll_contrast;
  Widget name_contrast, value_contrast;
  Widget button_dismiss;
  Widget form_tuning, form_display, form_zoom, form_color;
  Widget label_fps, label_kbps, label_loss;
  BOOLEAN LOCAL_DISPLAY;
  XFontStruct *font_small, *font_button;

  if((font_small = XLoadQueryFont(dpy, "-adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1")) == NULL){
    font_small = XLoadQueryFont(dpy, "-*-*-bold-r-normal--10-*-*-*-*-*-*-*");
  }
  if((font_button = XLoadQueryFont(dpy, "-adobe-courier-bold-r-normal--12-120-75-75-m-70-iso8859-1")) == NULL){
    font_button = XLoadQueryFont(dpy, "-*-*-bold-r-normal--12-*-*-*-*-*-*-*");
  }
    

  if(streq(title, TitleLocalDisplay)){
    LOCAL_DISPLAY = TRUE;
  }else{
    LOCAL_DISPLAY = FALSE;
  }
  if(!XAllocNamedColor(dpy, palette, "Grey", &couleur[0], &bidon))
    couleur[0].pixel = WhitePixel (dpy, screen);
  if(! XAllocNamedColor(dpy, palette, "LightSteelBlue4", &couleur[1], &bidon))
    couleur[1].pixel = WhitePixel (dpy, screen);
  if(! XAllocNamedColor(dpy, palette, "MistyRose1", &couleur[2], &bidon))
    couleur[2].pixel = WhitePixel (dpy, screen);
  if(! XAllocNamedColor(dpy, palette, "Red", &couleur[3], &bidon))
    couleur[3].pixel = WhitePixel (dpy, screen);
  if(! XAllocNamedColor(dpy, palette, "White", &couleur[4], &bidon))
    couleur[4].pixel = WhitePixel (dpy, screen);

  n = 0;
  XtSetArg(args[n], XtNwidth, 250); n++;
  if(LOCAL_DISPLAY){
    XtSetArg(args[n], XtNheight, 145); n++;
  }else{
    XtSetArg(args[n], XtNheight, 165); n++;
  }
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;

  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			       parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  mainform = XtCreateManagedWidget("MainForm", formWidgetClass, *panel,
				args, n);


  /*
  *  Form for brightness/contrast tuning.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  form_tuning = XtCreateManagedWidget("Form_Tuning", formWidgetClass, mainform,
				args, n);

  /*
  *  Create the Brightness and contrast utilities scales.
  */
  CreateScale(&form_brightness, &scroll_brightness, &name_brightness,
	      &value_brightness, form_tuning, "FormBrightness", 
	      "Brightness", cur_brght);

  sprintf(str_default, "%d", cur_brght);
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  if(font_small != NULL)
    XtSetArg(args[n], XtNfont, font_small); n++;
  XtSetArg(args[n], XtNlabel, str_default); n++;
  XtSetValues(name_brightness, args, n-1); 
  XtSetValues(value_brightness, args, n); 
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  XtSetValues(form_brightness, args, n); 
  n = 0;
  XtSetArg(args[n], XtNorientation, XtorientHorizontal); n++;
  XtSetArg(args[n], XtNwidth, 85); n++;
  XtSetArg(args[n], XtNheight, 10); n++;
  XtSetArg(args[n], XtNborderWidth, 4); n++;
  XtSetArg(args[n], XtNminimumThumb, 4); n++;
  XtSetArg(args[n], XtNthickness, 4); n++;
  XtSetArg(args[n], XtNbackground, couleur[1].pixel); n++;
  XtSetValues(scroll_brightness, args, n); 

  XawScrollbarSetThumb(scroll_brightness, 0.5, 0.05);

  XtAddCallback(scroll_brightness, XtNjumpProc, (XtCallbackProc)brightnessCB, 
		(XtPointer)value_brightness);
  
  CreateScale(&form_contrast, &scroll_contrast, &name_contrast,
	      &value_contrast, form_tuning, "FormContrast", "Contrast", 
	      cur_int_ctrst);

  XtSetValues(scroll_contrast, args, n); 
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_brightness); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  XtSetValues(form_contrast, args, n); 

  sprintf(str_default, "%.2f", cur_ctrst);
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  if(font_small != NULL)
    XtSetArg(args[n], XtNfont, font_small); n++;
  XtSetArg(args[n], XtNlabel, str_default); n++;
  XtSetValues(name_contrast, args, n-1); 
  XtSetValues(value_contrast, args, n); 

  XawScrollbarSetThumb(scroll_contrast, 0.5, 0.05);

  XtAddCallback(scroll_contrast, XtNjumpProc, (XtCallbackProc) contrastCB, 
		(XtPointer)value_contrast);

  if(!LOCAL_DISPLAY){
    /*
     *  Form for bw/fr/loss display.
     */
    n = 0;
    XtSetArg(args[n], XtNfromVert, form_tuning); n++;
    XtSetArg(args[n], XtNtop, XtChainTop); n++;
    XtSetArg(args[n], XtNbottom, XtChainTop); n++;
    XtSetArg(args[n], XtNleft, XtChainLeft); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    form_display = XtCreateManagedWidget("FormDisplay", formWidgetClass,
					 mainform, args, n);
    n = 0;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "  0.0"); n++;
    XtSetArg(args[n], XtNhorizDistance, 3); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    value_fps = XtCreateManagedWidget("ValueFps", labelWidgetClass, 
				      form_display, args, (Cardinal) n);
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, value_fps); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "f/s"); n++;
    XtSetArg(args[n], XtNhorizDistance, 1); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    label_fps = XtCreateManagedWidget("ButtonLabelFps", labelWidgetClass, 
				      form_display, args, n);
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, label_fps); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "  0.0"); n++;
    XtSetArg(args[n], XtNhorizDistance, 7); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    value_kbps = XtCreateManagedWidget("ValueKbps", labelWidgetClass, 
				       form_display, args, (Cardinal) n);
    
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, value_kbps); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "kbps"); n++;
    XtSetArg(args[n], XtNhorizDistance, 1); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    label_kbps = XtCreateManagedWidget("ButtonLabelKbps", labelWidgetClass, 
				       form_display, args, (Cardinal) n);
    
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, label_kbps); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "  0.0"); n++;
    XtSetArg(args[n], XtNhorizDistance, 7); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    value_loss = XtCreateManagedWidget("ValueLoss", labelWidgetClass, 
				       form_display, args, (Cardinal) n);
    
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, value_loss); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNinternalWidth, 1); n++;
    XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
    XtSetArg(args[n], XtNlabel, "% loss"); n++;
    XtSetArg(args[n], XtNhorizDistance, 1); n++;
    if(font_small != NULL)
      XtSetArg(args[n], XtNfont, font_small); n++;
    label_loss = XtCreateManagedWidget("ButtonLabelLoss", labelWidgetClass, 
				       form_display, args, (Cardinal) n);
  }
  /*
  *  Form for zoom buttons.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert,
	   LOCAL_DISPLAY ? form_tuning : form_display); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  form_zoom = XtCreateManagedWidget("FormZoom", formWidgetClass, mainform,
				    args, n);

  n = 0;
  XtSetArg(args[n], XtNlabel, LabelZoomx2); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[2].pixel); n++;
  doubleButton = XtCreateManagedWidget("DoubleButton", commandWidgetClass,
				      form_zoom, args, n);
  XtAddCallback(doubleButton, XtNcallback, (XtCallbackProc) doubleCB, 
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, doubleButton); n++;
  XtSetArg(args[n], XtNlabel, LabelZoomx1); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[2].pixel); n++;
  regularButton = XtCreateManagedWidget("RegularButton", commandWidgetClass,
				      form_zoom, args, n);
  XtAddCallback(regularButton, XtNcallback, (XtCallbackProc) regularCB, 
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, regularButton); n++;
  XtSetArg(args[n], XtNlabel, LabelZooms2); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[2].pixel); n++;
  halfButton = XtCreateManagedWidget("HalfButton", commandWidgetClass,
				      form_zoom, args, n);
  XtAddCallback(halfButton, XtNcallback, (XtCallbackProc) halfCB, 
		(XtPointer)NULL);

  /*
  *  Form for color/grey/dismiss buttons.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert,
	   LOCAL_DISPLAY ? form_tuning :form_display); n++;
  XtSetArg(args[n], XtNfromHoriz, form_zoom); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  form_color = XtCreateManagedWidget("FormColor", formWidgetClass, mainform,
				     args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelColor); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[2].pixel); n++;
  colorButton = XtCreateManagedWidget("ColorButton", commandWidgetClass,
				      form_color, args, n);
  XtAddCallback(colorButton, XtNcallback, (XtCallbackProc)affColorCB, 
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, colorButton); n++;
  XtSetArg(args[n], XtNlabel, LabelGrey); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[2].pixel); n++;
  greyButton = XtCreateManagedWidget("GreyButton", commandWidgetClass,
				      form_color, args, n);
  XtAddCallback(greyButton, XtNcallback, (XtCallbackProc) affGreyCB, 
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, greyButton); n++;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  if(font_button != NULL)
    XtSetArg(args[n], XtNfont, font_button); n++;
  XtSetArg(args[n], XtNbackground, couleur[3].pixel); n++;
  XtSetArg(args[n], XtNforeground, couleur[4].pixel); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_color, args, n);
  XtAddCallback(button_dismiss, XtNcallback, (XtCallbackProc) display_panel,
		(XtPointer)NULL);
}


/*--------------------------- Video tuning callbacks ------------------------*/

void CallbackBri(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int lw;
  Arg args[1];
  char value[4];
  u_char command[257];
  
#ifdef SCREENMACHINE
  /* From -50 to 50 */
  param.brightness = (int)(100*top) - 50;
#ifndef NO_AUDIO
  smSetLumaIntensity(current_sm, param.brightness);
#endif
#else
  param.brightness = (int)(100*top);
#endif
  sprintf(value, "%d", param.brightness);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);

#ifdef VIDEOPIX
  /* Send to video_encode process the new tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", param.brightness);
  sprintf((char *)&command[5], "%d", param.contrast);
  if((lw=write(param.f2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackBri'write ");
    return;
  }
#endif /* VIDEOPIX */
}



void CallbackCon(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int lw;
  Arg args[1];
  char value[4];
  u_char command[257];
  
#ifdef SCREENMACHINE
  /* From -50 to 50 */
  param.contrast = (int)(100*top) - 50;
#ifndef NO_AUDIO
  smSetChromaIntensity(current_sm, param.contrast);
#endif
#else
  param.contrast = (int)(100*top);
#endif
  sprintf(value, "%d", param.contrast);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);

#ifdef VIDEOPIX
  /* Send to video_encode process the new tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", param.brightness);
  sprintf((char *)&command[5], "%d", param.contrast);
  if((lw=write(param.f2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackCon'write ");
    return;
  }
#endif /* VIDEOPIX */
}



void CallbackDefault(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int lw;
  u_char command[257];

#if defined(VIDEOPIX) || defined(SCREENMACHINE)
#ifdef VIDEOPIX
  param.brightness = 50;
  param.contrast = 50;
  InitScale(scroll_brightness, value_brightness, param.brightness);
  InitScale(scroll_contrast, value_contrast, param.contrast);
#else
  param.brightness = 0;
  param.contrast = 0;
  InitScaleScreenMachine(scroll_brightness, value_brightness, 
			 param.brightness);
  InitScaleScreenMachine(scroll_contrast, value_contrast, param.contrast);
#endif

#ifdef VIDEOPIX
  /* Send to video_encode process the default tuning */
  command[0] = PIP_TUNING;
  sprintf((char *)&command[1], "%d", param.brightness);
  sprintf((char *)&command[5], "%d", param.contrast);
  if((lw=write(param.f2s_fd[1], (char *)command, 9)) != 9){
    perror("CallbackDefault'write ");
  }
#endif /* VIDEOPIX */
#endif /* VIDEOPIX || SCREENMACHINE */
  return;
}





/*---------------------------------------------------------------------------*/
void PopVideoGrabberPanel (w, unused, evt)
     Widget w;
     XtPointer unused;
     XEvent evt;
{
  static BOOLEAN HIDDEN=TRUE;
  if(HIDDEN){
    XtPopup(video_grabber_panel, XtGrabNone);
    Valid(button_popup1);
    HIDDEN = FALSE;
  }else{
    XtPopdown(video_grabber_panel);
    Unvalid(button_popup1);
    HIDDEN = TRUE;
  }
}


void PopVideoCoderPanel (w, unused, evt)
     Widget w;
     XtPointer unused;
     XEvent evt;
{
  static BOOLEAN HIDDEN=TRUE;
  if(HIDDEN){
    XtPopup(video_coder_panel, XtGrabNone);
    Valid(button_popup2);
    HIDDEN = FALSE;
  }else{
    XtPopdown(video_coder_panel);
    Unvalid(button_popup2);
    HIDDEN = TRUE;
  }
}


void PopAudioPanel(w, unused, evt)
     Widget w;
     XtPointer unused;
     XEvent evt;
{
  static BOOLEAN HIDDEN=TRUE;
  if(HIDDEN){
    XtPopup(audio_panel, XtGrabNone);
    Valid(button_popup3);
    HIDDEN = FALSE;
  }else{
    XtPopdown(audio_panel);
    Unvalid(button_popup3);
    HIDDEN = TRUE;
  }
}


void PopControlPanel(w, unused, evt)
     Widget w;
     XtPointer unused;
     XEvent evt;
{
  static BOOLEAN HIDDEN=TRUE;
  if(HIDDEN){
    XtPopup(control_panel, XtGrabNone);
    Valid(button_popup4);
    HIDDEN = FALSE;
  }else{
    XtPopdown(control_panel);
    Unvalid(button_popup4);
    HIDDEN = TRUE;
  }
}


void PopInfoPanel(w, unused, evt)
     Widget w;
     XtPointer unused;
     XEvent evt;
{
  static BOOLEAN HIDDEN=TRUE;
  if(HIDDEN){
    XtPopup(info_panel, XtGrabNone);
    Valid(button_popup5);
    HIDDEN = FALSE;
  }else{
    XtPopdown(info_panel);
    Unvalid(button_popup5);
    HIDDEN = TRUE;
  }
}


void CallbackColorEncoding(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  if(!param.COLOR){
    param.COLOR = TRUE;
    Unvalid(button_grey);
    Valid(button_color);
  }
  return;
}



void CallbackGreyEncoding(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  if(param.COLOR){
    param.COLOR = FALSE;
    Unvalid(button_color);
    Valid(button_grey);
  }
  return;
}



void CallbackQCIFEncoding(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.video_size){
  case CIF_SIZE:
    Unvalid(button_cif);
    break;
  case CIF4_SIZE:
    Unvalid(button_scif);
    break;
  default:
    return;
  }
  param.video_size = QCIF_SIZE;
  Valid(button_qcif);
  return;
}
    
void CallbackCIFEncoding(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.video_size){
  case QCIF_SIZE:
    Unvalid(button_qcif);
    break;
  case CIF4_SIZE:
    Unvalid(button_scif);
    break;
  default:
    return;
  }
  param.video_size = CIF_SIZE;
  Valid(button_cif);
  return;
}
    
void CallbackSCIFEncoding(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.video_size){
  case QCIF_SIZE:
    Unvalid(button_qcif);
    break;
  case CIF_SIZE:
    Unvalid(button_cif);
    break;
  default:
    return;
  }
  param.video_size = CIF4_SIZE;
  Valid(button_scif);
  return;
}
    
void CallbackFreeze(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int lw;
  u_char command;
  
  if(!param.FREEZE){
    param.FREEZE=TRUE;
    if(! param.MAX_FR_RATE_TUNED){
      AUTO_FREEZE_CONTROL = TRUE;
      param.MAX_FR_RATE_TUNED = TRUE;
      XtSetSensitive(form_frms, TRUE);
      Unvalid(button_frame[0]);
      Valid(button_frame[1]);
      if(param.START_EVIDEO){
	command = PIP_MAX_FR_RATE;
	if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackFrameControl'write ");
	  return;
	}
      }
    }
    command = PIP_FREEZE;
    Unvalid(button_unfreeze);
    Valid(button_freeze);
    if(param.START_EVIDEO){
      if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackFreeze'write ");
	return;
      }
    }
  }
  return;
}

void CallbackUnfreeze(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int lw;
  u_char command;
  
  if(param.FREEZE){
    param.FREEZE=FALSE;
    if(AUTO_FREEZE_CONTROL && param.MAX_FR_RATE_TUNED){
      AUTO_FREEZE_CONTROL = FALSE;
      param.MAX_FR_RATE_TUNED = FALSE;
      XtSetSensitive(form_frms, FALSE);
      Unvalid(button_frame[1]);
      Valid(button_frame[0]);
      if(param.START_EVIDEO){
	command = PIP_NO_MAX_FR_RATE;
	if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	  perror("CallbackFrameControl'write ");
	  return;
	}
      }
    }
    command = PIP_UNFREEZE;
    Unvalid(button_freeze);
    Valid(button_unfreeze);
    if(param.START_EVIDEO){
      if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackUnfreeze'write ");
	return;
      }
    }
  }
}

void CallbackLocalDisplay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data; 
{
  int lw;
  u_char command;

  if(!param.LOCAL_DISPLAY){
    param.LOCAL_DISPLAY = TRUE;
    Unvalid(button_nolocal);
    Valid(button_local);
    if(param.START_EVIDEO){
      command=PIP_LDISP;
      if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackLocalDisplay'write ");
	return;
      }
    }
  }
  return;
}



void CallbackNoLocalDisplay(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data; 
{
  int lw;
  u_char command;

  if(param.LOCAL_DISPLAY){
    param.LOCAL_DISPLAY = FALSE;
    Unvalid(button_local);
    Valid(button_nolocal);
    if(param.START_EVIDEO){
      command=PIP_NOLDISP;
      if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
	perror("CallbackLocalDisplay'write ");
	return;
      }
    }
  }
  return;
}




void CreateVideoCoderPanel(title, panel, parent)
     char	*title;
     Widget	*panel;
     Widget	parent;
{
  Arg args[15];
  Cardinal n;
  Widget form_dismiss, button_dismiss;
  
  n = 0;
  XtSetArg(args[n], XtNwidth, 320); n++;
#if defined(VIDEOPIX) || defined(SCREENMACHINE)
  XtSetArg(args[n], XtNheight, 210); n++;
#else
  XtSetArg(args[n], XtNheight, 145); n++;
#endif  

  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			       parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  mainform_coder = XtCreateManagedWidget("MainForm", formWidgetClass,
					 *panel, args, n);
#if defined(VIDEOPIX) || defined(SCREENMACHINE)
  /*
  *  Form for scales.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_scales = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				      args, n);
  /*
   *  Create the Brightness and contrast utilities scales.
   */
  CreateScale(&form_brightness, &scroll_brightness, &name_brightness,
	      &value_brightness, form_scales, "FormBrightness", 
	      "NameBrightness", param.brightness);
  XtAddCallback(scroll_brightness, XtNjumpProc, CallbackBri, 
		(XtPointer)value_brightness);
  
  CreateScale(&form_contrast, &scroll_contrast, &name_contrast,
	      &value_contrast, form_scales, "FormContrast", "NameContrast", 
	      param.contrast);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_brightness); n++;
  XtSetValues(form_contrast, args, (Cardinal) n); 
  XtAddCallback(scroll_contrast, XtNjumpProc, CallbackCon, 
		(XtPointer)value_contrast);
  
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_contrast); n++;
  button_default = XtCreateManagedWidget("ButtonDefault", commandWidgetClass,
					 form_scales, args, (Cardinal) n);
  XtAddCallback(button_default, XtNcallback, CallbackDefault,
		(XtPointer) NULL);
#endif /* VIDEOPIX || SCREENMACHINE */  

  /*
  *  Form for color/grey.
  */
  n = 0;
#if defined(VIDEOPIX) || defined(SCREENMACHINE)
  XtSetArg(args[n], XtNfromVert, form_scales); n++;
#endif  
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_color = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				args, n);
  n = 0;
  button_color = XtCreateManagedWidget("ButtonColor", commandWidgetClass,
				       form_color, args, (Cardinal) n);
  XtAddCallback(button_color, XtNcallback, CallbackColorEncoding,
		(XtPointer)NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_color); n++;
  button_grey = XtCreateManagedWidget("ButtonGrey", commandWidgetClass,
				       form_color, args, (Cardinal) n);
  XtAddCallback(button_grey, XtNcallback, CallbackGreyEncoding,
		(XtPointer)NULL);
  
  /*
  *  Form for size choice.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNfromVert, form_color); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_size = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				args, n);
  n = 0;
  button_qcif = XtCreateManagedWidget("ButtonQCIF", commandWidgetClass,
				       form_size, args, (Cardinal) n);
  XtAddCallback(button_qcif, XtNcallback, CallbackQCIFEncoding,
		(XtPointer)NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_qcif); n++;
  button_cif = XtCreateManagedWidget("ButtonCIF", commandWidgetClass,
				       form_size, args, (Cardinal) n);
  XtAddCallback(button_cif, XtNcallback, CallbackCIFEncoding,
		(XtPointer)NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_cif); n++;
  button_scif = XtCreateManagedWidget("ButtonSCIF", commandWidgetClass,
				       form_size, args, (Cardinal) n);
  XtAddCallback(button_scif, XtNcallback, CallbackSCIFEncoding,
		(XtPointer)NULL);
  
  /*
  *  Form for freeze choice.
  */
  n = 0;
#if defined(VIDEOPIX) || defined(SCREENMACHINE)
  XtSetArg(args[n], XtNfromVert, form_scales); n++;
#endif  
  XtSetArg(args[n], XtNfromHoriz, form_color); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_freeze = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				args, n);
  n = 0;
  button_freeze = XtCreateManagedWidget("ButtonFreeze", commandWidgetClass,
					form_freeze, args, (Cardinal) n);
  XtAddCallback(button_freeze, XtNcallback, CallbackFreeze,
		(XtPointer)NULL);
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_freeze); n++;
  button_unfreeze = XtCreateManagedWidget("ButtonUnfreeze", commandWidgetClass,
					form_freeze, args, (Cardinal) n);
  XtAddCallback(button_unfreeze, XtNcallback, CallbackUnfreeze,
		(XtPointer)NULL);
  /*
  *  Form for local image  choice.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_freeze); n++;
  XtSetArg(args[n], XtNfromHoriz, form_size); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_local = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				     args, n);
  n = 0;
  button_local = XtCreateManagedWidget("ButtonLocal", commandWidgetClass,
				       form_local, args, (Cardinal) n);
  XtAddCallback(button_local, XtNcallback, CallbackLocalDisplay,
		(XtPointer)NULL);
  
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_local); n++;
  button_nolocal = XtCreateManagedWidget("ButtonNoLocal", commandWidgetClass,
					 form_local, args, (Cardinal) n);
  XtAddCallback(button_nolocal, XtNcallback, CallbackNoLocalDisplay,
		(XtPointer)NULL);
  
  /*
  *  Form for dismiss button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_local); n++;
  XtSetArg(args[n], XtNfromHoriz, form_size); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  form_dismiss = XtCreateManagedWidget("Form", formWidgetClass, mainform_coder,
				       args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_dismiss, args, n);
  XtAddCallback(button_dismiss, XtNcallback,
		(XtCallbackProc)PopVideoCoderPanel, (XtPointer)NULL);
}
/*---------------------------------------------------------------------------*/



void ChangeButtonPort(board)
     int board;
{
  register int i;
  
  switch(board){
#ifdef SUNVIDEO    
  case SUNVIDEO_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], SunVideoBoardItems[i]);
    break;
#endif
#ifdef VIGRAPIX
  case VIGRAPIX_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], VigraPixBoardItems[i]);
    break;
#endif
#ifdef PARALLAX
  case PARALLAX_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], ParallaxBoardItems[i]);
    break;
#endif
#ifdef VIDEOPIX
  case VIDEOPIX_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], VideoPixBoardItems[i]);
    break;
#endif
#ifdef VIDEOTX
  case VIDEOTX_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], VideoTXBoardItems[i]);
    break;
#endif
#ifdef J300
  case J300_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], J300BoardItems[i]);
    break;
#endif
#ifdef HP_ROPS_VIDEO
  case RASTEROPS_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], HPRopsBoardItems[i]);
    break;
#endif
#if defined(INDIGOVIDEO) || defined(GALILEOVIDEO)   
  case INDY_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], IndyBoardItems[i]);
    for(i=3; i<6; i++)
      XtSetSensitive(button_port[i], FALSE);
    break;
  case GALILEO_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], SGIBoardItems[i]);
    for(i=3; i<6; i++)
      XtSetSensitive(button_port[i], TRUE);
    break;
  case INDIGO_BOARD:
    for(i=0; i<3; i++)
      ChangeLabel(button_port[i], SGIBoardItems[i]);
    for(i=3; i<6; i++)
      XtSetSensitive(button_port[i], TRUE);
    break;
#endif /* INDIGO_VIDEO || GALILEOVIDEO */    
  }
  return;
}



void CallbackBoard1(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.board_selected){
  case SUNVIDEO_BOARD:
    break;
  case VIGRAPIX_BOARD:
    Unvalid(button_board2);
    Valid(button_board1);
    param.board_selected = SUNVIDEO_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(SUNVIDEO_BOARD, FALSE);
#ifdef VIGRAPIX
    Unvalid(button_format[param.secam_format ? 2 : 3]);
#endif
    break;
  case PARALLAX_BOARD:
    Unvalid(button_board3);
    Valid(button_board1);
    param.board_selected = SUNVIDEO_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(SUNVIDEO_BOARD, FALSE);
#ifdef PARALLAX
    Unvalid(button_format[param.px_port_video.standard]);
#endif
    XtSetSensitive(button_scif, FALSE);
    break;
  case VIDEOPIX_BOARD:
    Unvalid(button_board4);
    Valid(button_board1);
    param.board_selected = SUNVIDEO_BOARD;
    ChangeButtonPort(param.board_selected);
    XtSetSensitive(button_scif, FALSE);
    break;
  case INDY_BOARD:
    break;
  case GALILEO_BOARD:
    Unvalid(button_board2);
    Valid(button_board1);
    param.board_selected = INDY_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  case INDIGO_BOARD:
    Unvalid(button_board3);
    Valid(button_board1);
    param.board_selected = INDY_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  default:
    break;
  }
}
    


void CallbackBoard2(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.board_selected){
  case SUNVIDEO_BOARD:
    Unvalid(button_board1);
    Valid(button_board2);
    param.board_selected = VIGRAPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(VIGRAPIX_BOARD, TRUE);
#ifdef VIGRAPIX
    Valid(button_format[param.secam_format ? 2 : 3]);
#endif
    XtSetSensitive(button_scif, FALSE);
    break;
  case VIGRAPIX_BOARD:
    break;
  case PARALLAX_BOARD:
    Unvalid(button_board3);
    Valid(button_board2);
    param.board_selected = VIGRAPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(VIGRAPIX_BOARD, TRUE);
    XtSetSensitive(button_scif, FALSE);
#ifdef PARALLAX
    Unvalid(button_format[param.px_port_video.standard]);
#endif
#ifdef VIGRAPIX
    Valid(button_format[param.secam_format ? 2 : 3]);
#endif
    break;
  case VIDEOPIX_BOARD:
    Unvalid(button_board4);
    Valid(button_board2);
    param.board_selected = VIGRAPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(VIGRAPIX_BOARD, TRUE);
    XtSetSensitive(button_scif, FALSE);
#ifdef VIGRAPIX
    Valid(button_format[param.secam_format ? 2 : 3]);
#endif
    break;
  case INDY_BOARD:
    Unvalid(button_board1);
    Valid(button_board2);
    param.board_selected = GALILEO_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  case GALILEO_BOARD:
    break;
  case INDIGO_BOARD:
    Unvalid(button_board3);
    Valid(button_board2);
    param.board_selected = GALILEO_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  default:
    break;
  }
}
    

void CallbackBoard3(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.board_selected){
  case SUNVIDEO_BOARD:
    Unvalid(button_board1);
    Valid(button_board3);
    param.board_selected = PARALLAX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(PARALLAX_BOARD, TRUE);
#ifdef PARALLAX
    Valid(button_format[param.px_port_video.standard]);
#endif
    XtSetSensitive(button_scif, TRUE);
    break;
  case VIGRAPIX_BOARD:
    Unvalid(button_board2);
    Valid(button_board3);
    param.board_selected = PARALLAX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(PARALLAX_BOARD, TRUE);
#ifdef VIGRAPIX
    Unvalid(button_format[param.secam_format ? 2 : 3]);
#endif
#ifdef PARALLAX
    Valid(button_format[param.px_port_video.standard]);
#endif
    XtSetSensitive(button_scif, TRUE);
    break;
  case PARALLAX_BOARD:
    break;
  case VIDEOPIX_BOARD:
    Unvalid(button_board4);
    Valid(button_board3);
    param.board_selected = PARALLAX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(PARALLAX_BOARD, TRUE);
#ifdef PARALLAX
    Valid(button_format[param.px_port_video.standard]);
#endif
    break;
  case INDY_BOARD:
    Unvalid(button_board1);
    Valid(button_board3);
    param.board_selected = INDIGO_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  case GALILEO_BOARD:
    Unvalid(button_board2);
    Valid(button_board3);
    param.board_selected = INDIGO_BOARD;
    ChangeButtonPort(param.board_selected);
    break;
  case INDIGO_BOARD:
    break;
  default:
    break;
  }
}



void CallbackBoard4(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  switch(param.board_selected){
  case SUNVIDEO_BOARD:
    Unvalid(button_board1);
    Valid(button_board4);
    param.board_selected = VIDEOPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    XtSetSensitive(button_scif, TRUE);
    break;
  case VIGRAPIX_BOARD:
    Unvalid(button_board2);
    Valid(button_board4);
    param.board_selected = VIDEOPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(VIDEOPIX_BOARD, FALSE);
#ifdef VIGRAPIX
    Unvalid(button_format[param.secam_format ? 2 : 3]);
#endif
    XtSetSensitive(button_scif, TRUE);
    break;
  case PARALLAX_BOARD:
    Unvalid(button_board3);
    Valid(button_board4);
    param.board_selected = VIDEOPIX_BOARD;
    ChangeButtonPort(param.board_selected);
    SetSensitiveVideoFormat(VIDEOPIX_BOARD, FALSE);
#ifdef PARALLAX
    Unvalid(button_format[param.px_port_video.standard]);
#endif    
    break;
  case VIDEOPIX_BOARD:
  default:
    break;
  }
}



#ifdef VIDEOPIX
static void CallbackVfcPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<VIDEOPIX_BOARD_LEN; sel++){
    if(streq(choice, VideoPixBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackVideoPixPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* VIDEOPIX */



#ifdef VIGRAPIX
static void CallbackVigraPixPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<VIGRAPIX_BOARD_LEN; sel++){
    if(streq(choice, VigraPixBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackVigraPixPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* VIGRAPIX */



#ifdef SUNVIDEO
static void CallbackSunVideoPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<SUNVIDEO_BOARD_LEN; sel++){
    if(streq(choice, SunVideoBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackSunVideoPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* SUNVIDEO */
#ifdef PARALLAX
static void CallbackParallaxPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, last;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<PX_BOARD_LEN; sel++){
    if(streq(choice, ParallaxBoardItems[sel])){
      switch(sel){
      case CH_1:
      case CH_2:
      case CH_AUTO:
	last = param.px_port_video.channel;
	if(sel == last){
	  return;
	}
	param.px_port_video.channel = sel; 
	Unvalid(button_port[last]);
	Valid(button_port[sel]);
	break;
      }
    }
  }
}
#endif /* PARALLAX */
#if defined(INDIGOVIDEO) || defined(GALILEOVIDEO) || defined(INDYVIDEO)
static void CallbackSGIPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;

  strcpy(choice, XtName(w));
  if(param.board_selected == INDY_BOARD) {
    for(sel=0; sel<INDY_BOARD_LEN; sel++){
      if(streq(choice, IndyBoardItems[sel]))
	break;
    }
  }else{
    for(sel=0; sel<SGI_BOARD_LEN; sel++){
      if(streq(choice, SGIBoardItems[sel]))
	break;
    }
  }

  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackSGIPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* INDIGOVIDEO or GALILEOVIDEO or INDYVIDEO */
#ifdef VIDEOTX
static void CallbackVideoTXPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;

  strcpy(choice, XtName(w));
  for(sel=0; sel<VIDEOTX_BOARD_LEN; sel++){
    if(streq(choice, VideoTXBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackVideoTXPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* VIDEOTX */
#ifdef J300
static void CallbackJ300Port(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;

  strcpy(choice, XtName(w));
  for(sel=0; sel<J300_BOARD_LEN; sel++){
    if(streq(choice, J300BoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackJ300Port'write ");
	return;
      }
    }
    return;
  }
}
#endif /* VIDEOTX */
#ifdef HP_ROPS_VIDEO
static void CallbackHPPort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, lw;

  strcpy(choice, XtName(w));
  for(sel=0; sel<HP_ROPS_VIDEO_BOARD_LEN; sel++){
    if(streq(choice, HPRopsBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
    if(param.START_EVIDEO){
      command[0] = PIP_VIDEO_PORT;
      command[1] = param.video_port;
      if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
	perror("CallbackHPRopsPort'write ");
	return;
      }
    }
    return;
  }
}
#endif /* HP_ROPS_VIDEO */
#ifdef SCREENMACHINE
static void CallbackScreenMachinePort(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel;

  strcpy(choice, XtName(w));
  for(sel=0; sel<SCREENMACHINE_BOARD_LEN; sel++){
    if(streq(choice, ScreenMachineBoardItems[sel]))
      break;
  }
  if(param.video_port != sel){
    Unvalid(button_port[param.video_port]);
    Valid(button_port[sel]);
    param.video_port = sel;
#ifndef NO_AUDIO
    if(param.START_EVIDEO){
      smSetInput(current_sm, sel);
    }
#endif
    return;
  }
}
static void CallbackScreenMachineFormat(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel;

  strcpy(choice, XtName(w));
  for(sel=0; sel<SCREENMACHINE_FORMAT_LEN; sel++){
    if(streq(choice, ScreenMachineFormatItems[sel]))
      break;
  }
  if(param.video_format != sel){
    Unvalid(button_format[param.video_format]);
    Valid(button_format[sel]);
    param.video_format = sel;
#ifndef NO_AUDIO
    if(param.START_EVIDEO){
      smSetSystem(current_sm, sel);
    }
#endif
    return;
  }
}
static void CallbackScreenMachineField(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel;

  strcpy(choice, XtName(w));
  for(sel=0; sel<SCREENMACHINE_FIELD_LEN; sel++){
    if(streq(choice, ScreenMachineFieldItems[sel]))
      break;
  }
  if(param.video_field != sel){
    Unvalid(button_field[param.video_field]);
    Valid(button_field[sel]);
    param.video_field = sel;
#ifndef NO_AUDIO
    if(param.START_EVIDEO){
      smSetInputFields(current_sm, sel);
    }
#endif
    return;
  }
}
static void CallbackScreenMachineScreen(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel;

  strcpy(choice, XtName(w));
  for(sel=0; sel<SCREENMACHINE_SCREEN_LEN; sel++){
    if(streq(choice, ScreenMachineScreenItems[sel]))
      break;
  }
  if(current_sm != sel){
    Unvalid(button_sm[current_sm]);
    Valid(button_sm[sel]);
    current_sm = sel;
  }
}
#endif /* SCREENMACHINE */
#if defined(PARALLAX) || defined(VIGRAPIX)
static void CallbackFormat(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[1];
  int lw, sel, last, cc;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<PX_FORMAT_LEN; sel++){
    if(streq(choice, ParallaxFormatItems[sel])){
      switch(sel){
      case NTSC:
      case PAL:
      case SECAM:
      case AUTO_PAL:
      case AUTO_SECAM:
#ifdef PARALLAX
	if(param.board_selected == PARALLAX_BOARD)
	  last = param.px_port_video.standard + NTSC;
#endif
#ifdef VIGRAPIX
	if(param.board_selected == VIGRAPIX_BOARD)
	  last = 6 - param.secam_format;
#endif
	if(last == sel)
	  return;
#ifdef PARALLAX
	if(param.board_selected == PARALLAX_BOARD)
	  param.px_port_video.standard = sel-NTSC;
#endif
#ifdef VIGRAPIX
	if(param.board_selected == VIGRAPIX_BOARD)
	  param.secam_format = !param.secam_format;
#endif
	break;
#ifdef PARALLAX
      case SQUEEZE:
	cc = param.px_port_video.squeeze =
	  !param.px_port_video.squeeze;
	goto l_toggle;
      case MIRROR_X:
	cc = param.px_port_video.mirror_x =
	  !param.px_port_video.mirror_x;
	goto l_toggle;
      case MIRROR_Y:
	cc = param.px_port_video.mirror_y =
	  !param.px_port_video.mirror_y;
      l_toggle:
	if(cc)
	  Valid(button_format[sel-3]);
	else
	  Unvalid(button_format[sel-3]);
	return;
#endif
      }
      Unvalid(button_format[last-3]);
      Valid(button_format[sel-3]);
      if((param.board_selected == VIGRAPIX_BOARD) && param.START_EVIDEO) {
	command[0] = PIP_VIDEO_FORMAT;
	if((lw=write(param.f2s_fd[1], (char *)command, 1)) != 1){
	  perror("CallbackFormat'write ");
	  return;
	}
      }
    }
  }
}
#endif /* PARALLAX || VIGRAPIX */



void CreateVideoGrabberPanel(title, panel, parent)
     char	*title;
     Widget	*panel;
     Widget	parent;
{
  Arg args[15];
  Cardinal n;
  BoardName6 *BoardMenuItem;
  void (*CallbackBoardPort)();
  Widget form_dismiss, button_dismiss;

  switch(param.board_selected){
#ifdef VIDEOPIX
  case VIDEOPIX_BOARD:
    BoardMenuItem = (BoardName6 *)VideoPixBoardItems;
    CallbackBoardPort = CallbackVfcPort;
    break;
#endif
#ifdef SUNVIDEO  
  case SUNVIDEO_BOARD:
    BoardMenuItem = (BoardName6 *)SunVideoBoardItems;
    CallbackBoardPort = CallbackSunVideoPort;
    break;
#endif
#ifdef VIGRAPIX
  case VIGRAPIX_BOARD:
    BoardMenuItem = (BoardName6 *)VigraPixBoardItems;
    CallbackBoardPort = CallbackVigraPixPort;
    break;
#endif
#ifdef PARALLAX
  case PARALLAX_BOARD:
    BoardMenuItem = (BoardName6 *)ParallaxBoardItems;
    CallbackBoardPort = CallbackParallaxPort;
    break;
#endif
#if defined(INDIGOVIDEO) ||  defined(GALILEOVIDEO) || defined(INDYVIDEO)
  case INDIGO_BOARD:
  case GALILEO_BOARD:
    BoardMenuItem = (BoardName6 *)SGIBoardItems;
    CallbackBoardPort = CallbackSGIPort;
    break;
  case INDY_BOARD:
    BoardMenuItem = (BoardName6 *)IndyBoardItems;
    CallbackBoardPort = CallbackSGIPort;
    break;
#endif
#ifdef VIDEOTX
  case VIDEOTX_BOARD:
    BoardMenuItem = (BoardName6 *)VideoTXBoardItems;
    CallbackBoardPort = CallbackVideoTXPort;
    break;
#endif
#ifdef J300
  case J300_BOARD:
    BoardMenuItem = (BoardName6 *)J300BoardItems;
    CallbackBoardPort = CallbackJ300Port;
#endif
#if defined(HP_ROPS_VIDEO)
  case RASTEROPS_BOARD:
    BoardMenuItem = (BoardName6 *)HPRopsBoardItems;
    CallbackBoardPort = CallbackHPPort;
    break;
#endif
#ifdef SCREENMACHINE
  case SCREENMACHINE_BOARD:
    BoardMenuItem = (BoardName6 *)ScreenMachineBoardItems;
    CallbackBoardPort = CallbackScreenMachinePort;
    break;
#endif
  }
  n = 0;
  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			      parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  mainform_grabber = XtCreateManagedWidget("MainForm", formWidgetClass,
					 *panel, args, n);
  /*
   *  Form for boards selection.
   */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_board = XtCreateManagedWidget("Form", formWidgetClass,
				     mainform_grabber, args, n);
  /*
   *  Create the Board selection
   */
  
  n = 0;
#if !defined(SUNVIDEO) && !defined(INDYVIDEO) && !defined(VIDEOTX) && !defined(SCREENMACHINE) && !defined(HP_ROPS_VIDEO)
  XtSetArg(args[n], XtNsensitive, FALSE); n++;
#endif  
  button_board1 = XtCreateManagedWidget("ButtonBoard1", commandWidgetClass,
					form_board, args, (Cardinal) n);
  XtAddCallback(button_board1, XtNcallback, CallbackBoard1,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_board1); n++;
#if !defined(VIGRAPIX) && !defined(GALILEOVIDEO) && !defined(J300) && !defined(PC_VIDEO2) && !defined(HP_VIDEO2)
  XtSetArg(args[n], XtNsensitive, FALSE); n++;
#endif  
  button_board2 = XtCreateManagedWidget("ButtonBoard2", commandWidgetClass,
					form_board, args, (Cardinal) n);
  XtAddCallback(button_board2, XtNcallback, CallbackBoard2,
		(XtPointer) NULL);
#if !defined(DECSTATION) && !defined(HPUX) && !defined(PC)  
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_board2); n++;
#if !defined(PARALLAX) && !defined(INDIGOVIDEO) 
  XtSetArg(args[n], XtNsensitive, FALSE); n++;
#endif  
  button_board3 = XtCreateManagedWidget("ButtonBoard3", commandWidgetClass,
					form_board, args, (Cardinal) n);
  XtAddCallback(button_board3, XtNcallback, CallbackBoard3,
		  (XtPointer) NULL);
#ifdef SPARCSTATION
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_board3); n++;
#ifndef VIDEOPIX
  XtSetArg(args[n], XtNsensitive, FALSE); n++;
#endif
  button_board4 = XtCreateManagedWidget("ButtonBoard4", commandWidgetClass,
					form_board, args, (Cardinal) n);
  XtAddCallback(button_board4, XtNcallback, CallbackBoard4, (XtPointer) NULL);
#endif /* SPARCSTATION */
#endif /* !defined(DECSTATION) && !defined(HPUX) && !defined(PC) */

#ifdef SCREENMACHINE
  /*
   *  Form for video ScreenMachine Selection 
   */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_board); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_sm = XtCreateManagedWidget("Form", formWidgetClass,
				      mainform_grabber, args, n);
  n = 0;
  button_sm[0] = XtCreateManagedWidget(ScreenMachineScreenItems[0],
					   commandWidgetClass,
					   form_sm, args, (Cardinal) n);
  XtAddCallback(button_sm[0], XtNcallback, CallbackScreenMachineScreen,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_sm[0]); n++;
  button_sm[1] = XtCreateManagedWidget(ScreenMachineScreenItems[1],
					   commandWidgetClass,
					   form_sm, args, (Cardinal) n);
  XtAddCallback(button_sm[1], XtNcallback, CallbackScreenMachineScreen,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_sm[1]); n++;
  button_sm[2] = XtCreateManagedWidget(ScreenMachineScreenItems[2],
					   commandWidgetClass,
					   form_sm, args, (Cardinal) n);
  XtAddCallback(button_sm[2], XtNcallback, CallbackScreenMachineScreen,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_sm[2]); n++;
  button_sm[3] = XtCreateManagedWidget(ScreenMachineScreenItems[3],
					   commandWidgetClass,
					   form_sm, args, (Cardinal) n);
  XtAddCallback(button_sm[3], XtNcallback, CallbackScreenMachineScreen,
		(XtPointer)NULL);

#endif /* SCREENMACHINE */

  /*
   *  Form for video port inputs.
   */
  n = 0;
#ifdef SCREENMACHINE
  XtSetArg(args[n], XtNfromVert, form_sm); n++;
#else
  XtSetArg(args[n], XtNfromVert, form_board); n++;
#endif
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_port = XtCreateManagedWidget("Form", formWidgetClass, mainform_grabber,
				    args, n);
  n = 0;
  button_port[0] = XtCreateManagedWidget((*BoardMenuItem)[0],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[0], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_port[0]); n++;
  button_port[1] = XtCreateManagedWidget((*BoardMenuItem)[1],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[1], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_port[1]); n++;
  button_port[2] = XtCreateManagedWidget((*BoardMenuItem)[2],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[2], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);
#if defined(GALILEOVIDEO) || defined(INDIGOVIDEO) || defined(SCREENMACHINE)
  n = 0;
#ifdef SCREENMACHINE
  XtSetArg(args[n], XtNfromHoriz, button_port[2]); n++;
#else
  XtSetArg(args[n], XtNfromVert, button_port[0]); n++;
#endif
  if(param.board_selected == INDY_BOARD)
    XtSetArg(args[n], XtNsensitive, FALSE); n++;
  button_port[3] = XtCreateManagedWidget((*BoardMenuItem)[3],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[3], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);
#ifndef SCREENMACHINE
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_port[1]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_port[3]); n++;
  if(param.board_selected == INDY_BOARD)
    XtSetArg(args[n], XtNsensitive, FALSE); n++;
  button_port[4] = XtCreateManagedWidget((*BoardMenuItem)[4],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[4], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_port[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_port[4]); n++;
  if(param.board_selected == INDY_BOARD)
    XtSetArg(args[n], XtNsensitive, FALSE); n++;
  button_port[5] = XtCreateManagedWidget((*BoardMenuItem)[5],
					 commandWidgetClass,
					 form_port, args, (Cardinal) n);
  XtAddCallback(button_port[5], XtNcallback, (*CallbackBoardPort),
		(XtPointer)NULL);
#endif /* ! SCREENMACHINE */
#endif /* GALILEOVIDEO ||  INDIGOVIDEO || SCREENMACHINE */

#if defined(PARALLAX) || defined(VIGRAPIX)
  /*
   *  Form for video format.
   */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_port); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_format = XtCreateManagedWidget("Form", formWidgetClass,
				      mainform_grabber, args, n);
  n = 0;
  button_format[0] = XtCreateManagedWidget(ParallaxFormatItems[3],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[0], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[0]); n++;
  button_format[1] = XtCreateManagedWidget(ParallaxFormatItems[4],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[1], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[1]); n++;
  button_format[2] = XtCreateManagedWidget(ParallaxFormatItems[5],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[2], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[2]); n++;
  button_format[3] = XtCreateManagedWidget(ParallaxFormatItems[6],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[3], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[3]); n++;
  button_format[4] = XtCreateManagedWidget(ParallaxFormatItems[7],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[4], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_format[0]); n++;
  button_format[5] = XtCreateManagedWidget(ParallaxFormatItems[8],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[5], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_format[1]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_format[5]); n++;
  button_format[6] = XtCreateManagedWidget(ParallaxFormatItems[9],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[6], XtNcallback, CallbackFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_format[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_format[6]); n++;
  button_format[7] = XtCreateManagedWidget(ParallaxFormatItems[10],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[7], XtNcallback, CallbackFormat, 
		(XtPointer)NULL);
#endif /* PARALLAX || VIGRAPIX */
#ifdef SCREENMACHINE
  /*
   *  Form for video format.
   */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_port); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_format = XtCreateManagedWidget("Form", formWidgetClass,
				      mainform_grabber, args, n);
  n = 0;
  button_format[0] = XtCreateManagedWidget(ScreenMachineFormatItems[0],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[0], XtNcallback, CallbackScreenMachineFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[0]); n++;
  button_format[1] = XtCreateManagedWidget(ScreenMachineFormatItems[1],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[1], XtNcallback, CallbackScreenMachineFormat,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_format[1]); n++;
  button_format[2] = XtCreateManagedWidget(ScreenMachineFormatItems[2],
					   commandWidgetClass,
					   form_format, args, (Cardinal) n);
  XtAddCallback(button_format[2], XtNcallback, CallbackScreenMachineFormat,
		(XtPointer)NULL);

  /*
   *  Form for video field.
   */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_format); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_field = XtCreateManagedWidget("Form", formWidgetClass,
				      mainform_grabber, args, n);
  n = 0;
  button_field[0] = XtCreateManagedWidget(ScreenMachineFieldItems[0],
					   commandWidgetClass,
					   form_field, args, (Cardinal) n);
  XtAddCallback(button_field[0], XtNcallback, CallbackScreenMachineField,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_field[0]); n++;
  button_field[1] = XtCreateManagedWidget(ScreenMachineFieldItems[1],
					   commandWidgetClass,
					   form_field, args, (Cardinal) n);
  XtAddCallback(button_field[1], XtNcallback, CallbackScreenMachineField,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_field[1]); n++;
  button_field[2] = XtCreateManagedWidget(ScreenMachineFieldItems[2],
					   commandWidgetClass,
					   form_field, args, (Cardinal) n);
  XtAddCallback(button_field[2], XtNcallback, CallbackScreenMachineField,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_field[2]); n++;
  button_field[3] = XtCreateManagedWidget(ScreenMachineFieldItems[3],
					   commandWidgetClass,
					   form_field, args, (Cardinal) n);
  XtAddCallback(button_field[3], XtNcallback, CallbackScreenMachineField,
		(XtPointer)NULL);
#endif /* SCREENMACHINE */

  /*
  *  Form for dismiss button.
  */
  n = 0;
#if defined(PARALLAX) || defined(VIGRAPIX)
  XtSetArg(args[n], XtNfromVert, form_format); n++;
#else 
#ifdef SCREENMACHINE
  XtSetArg(args[n], XtNfromVert, form_field); n++;
#else 
  XtSetArg(args[n], XtNfromVert, form_port); n++;
#endif
#endif
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  form_dismiss = XtCreateManagedWidget("Form", formWidgetClass,
				       mainform_grabber, args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_dismiss, args, n);
  XtAddCallback(button_dismiss, XtNcallback,
		(XtCallbackProc)PopVideoGrabberPanel, (XtPointer)NULL);

}
/*---------------------------------------------------------------------------*/


#ifndef NO_AUDIO
static void CallbackAudio(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int sel, last, format;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<AUDIO_FORMAT_LEN; sel++){
    if(streq(choice, AudioFormatItems[sel])){
      last = param.audio_format;
      if(last == sel)
	return;
      param.audio_format = sel;
      Unvalid(button_audio[last]);
      Valid(button_audio[sel]);
    }
  }
  if(param.START_EAUDIO){
    switch (param.audio_format) {
    case PCM_64:
      format = RTP_PCM_CONTENT;
      break;
    case ADPCM_4:
      format = RTP_ADPCM_4;
      break;
    case VADPCM:
      format = RTP_VADPCM_CONTENT;
      break;
    case ADPCM_6:
      format = RTP_ADPCM_6;
      break;
    case ADPCM_5:
      format = RTP_ADPCM_5;
      break;
    case ADPCM_3:
      format = RTP_ADPCM_3;
      break;
    case ADPCM_2:
      format = RTP_ADPCM_2;
      break;
    case LPC:
      format = RTP_LPC;
      break;
    case GSM_13:
      format = RTP_GSM_13;
      break;
    }
    SetEncoding(format);
  }
}


static void CallbackRedondance(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lw, sel, last, format;
  
  strcpy(choice, XtName(w));
  for(sel=0; sel<AUDIO_REDONDANCE_LEN; sel++){
    if(streq(choice, AudioRedondanceItems[sel])){
      last = param.audio_redundancy;
      if(last == sel)
	return;
      param.audio_redundancy = sel;
      Unvalid(button_redondance[last]);
      Valid(button_redondance[sel]);
    }
  }
  if(param.START_EAUDIO){
    switch (param.audio_redundancy) {
    case PCM_64:
      format = RTP_PCM_CONTENT;
      break;
    case ADPCM_4:
      format = RTP_ADPCM_4;
      break;
    case VADPCM:
      format = RTP_VADPCM_CONTENT;
      break;
    case ADPCM_6:
      format = RTP_ADPCM_6;
      break;
    case ADPCM_5:
      format = RTP_ADPCM_5;
      break;
    case ADPCM_3:
      format = RTP_ADPCM_3;
      break;
    case ADPCM_2:
      format = RTP_ADPCM_2;
      break;
    case LPC:
      format = RTP_LPC;
      break;
    case GSM_13:
      format = RTP_GSM_13;
      break;
    case NONE:
      format = -1;
      break;
    }
    SetRedondancy(format);
  }
}


#endif /* ! NO_AUDIO */


static void CallbackControl(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lw, last, new;
  
  strcpy(choice, XtName(w));
  for(new=0; new<RATE_CONTROL_FORMAT_LEN; new++){
    if(streq(choice, ControlFormatItems[new])){
      last = param.rate_control;
      if(last == new)
	return;
      param.rate_control = new;
      switch(new){
      case AUTOMATIC_CONTROL:
	XtSetSensitive(scroll_control, FALSE);
	XtSetSensitive(form_control, TRUE);
	break;
      case MANUAL_CONTROL:
	XtSetSensitive(form_control, TRUE);
	XtSetSensitive(scroll_control, TRUE);
	break;
      case WITHOUT_CONTROL:
	XtSetSensitive(form_control, FALSE);
	XtSetSensitive(scroll_control, FALSE);
	break;
      }
      Unvalid(button_rate[last]);
      Valid(button_rate[new]);
    }
  }
  if(param.START_EVIDEO){
    command[0] = PIP_RATE_CONTROL;
    command[1] = param.rate_control;
    if((lw=write(param.f2s_fd[1], (char *)command, 2)) != 2){
      perror("CallbackControl'write ");
      return;
    }
  }
}



static void CallbackMode(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command[2];
  int lw, last, new;
  
  strcpy(choice, XtName(w));
  for(new=0; new<MODE_FORMAT_LEN; new++){
    if(streq(choice, ModeFormatItems[new])){
      last = !param.PRIVILEGE_QUALITY;
      if(last == new)
	return;
      param.PRIVILEGE_QUALITY = !new;
      Unvalid(button_mode[last]);
      Valid(button_mode[new]);
      break;
    }
  }
  if(param.START_EVIDEO){
    command[0] = new ? PIP_PRIVILEGE_FR : PIP_PRIVILEGE_QUALITY;
    if((lw=write(param.f2s_fd[1], (char *)command, 1)) != 1){
      perror("CallbackMode'write ");
      return;
    }
  }
}



static void CallbackFrameControl(w, junk, garbage)
     Widget w;
     XtPointer junk, garbage;
{
  char choice[100];
  u_char command;
  int lw, last, new;
  
  strcpy(choice, XtName(w));
  for(new=0; new<FR_CONTROL_FORMAT_LEN; new++){
    if(streq(choice, FrameControlFormatItems[new])){
      last = param.MAX_FR_RATE_TUNED;
      if(last == new)
	return;
      param.MAX_FR_RATE_TUNED = new;
      if(new)
	XtSetSensitive(form_frms, TRUE);
      else
	XtSetSensitive(form_frms, FALSE);
      Unvalid(button_frame[last]);
      Valid(button_frame[new]);
      break;
    }
  }
  if(param.START_EVIDEO){
    command = new ? PIP_MAX_FR_RATE : PIP_NO_MAX_FR_RATE;
    if((lw=write(param.f2s_fd[1], (char *)&command, 1)) != 1){
      perror("CallbackFrameControl'write ");
      return;
    }
  }
}


void CallbackRate(w, closure, call_data) 
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  u_char command[5];
  char value[4];
  int lw;
  Arg args[1];
  
  
  param.memo_rate_max = param.rate_max = (int)(param.max_rate_max*top);
  if(param.rate_max < param.rate_min)
    param.memo_rate_max = param.rate_max = param.rate_min;
  sprintf(value, "%d", param.rate_max);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  if(param.START_EVIDEO){
    command[0] = PIP_MAX_RATE;
    sprintf((char *)&command[1], "%d", param.rate_max);
    if((lw=write(param.f2s_fd[1], (char *)command, 5)) != 5){
      perror("CallbackRate'write ");
      return;
    }
  }
}

void CallbackFrameRate(w, closure, call_data) 
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  u_char command[5];
  char value[5];
  int lw;
  Arg args[1];
  
  
  param.max_frame_rate = (int)(30*top);
  if(param.max_frame_rate < 1)
    param.max_frame_rate = 1;
  sprintf(value, "%d", (param.max_frame_rate));
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  if(param.START_EVIDEO){
    command[0] = PIP_FRAME_RATE;
    sprintf((char *)&command[1], "%d", param.max_frame_rate);
    if((lw=write(param.f2s_fd[1], (char *)command, 5)) != 5){
      perror("CallbackFrameRate'write ");
      return;
    }
  }
}


void CreateControlPanel(title, panel, parent)
     char *title;
     Widget *panel;
     Widget parent;
{
  Arg args[15];
  Cardinal n;
  Widget label_ref1, label_ref3;
  Widget label_band1, label_band3;
  Widget label_rate, label_frame, label_mode;
  Widget form_dismiss, button_dismiss;
#ifdef CRYPTO
  XtTranslations trans_table;
#endif
  float top;
  int rate;

  rate = param.floor_mode == FLOOR_RATE_MIN_MAX ?
    param.rate_min : param.rate_max;
  
  n = 0;
  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			      parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  mainform_control = XtCreateManagedWidget("MainForm", formWidgetClass,
					   *panel, args, n);

#ifdef CRYPTO
  /* 
   * Form for the key control.
   */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_key = XtCreateManagedWidget("Form", formWidgetClass,
				    mainform_control, args, n);

  trans_table = XtParseTranslationTable(defaultTranslations);

  n = 0;
  if(!ENCRYPTION_ENABLED()){
    XtSetArg(args[n], XtNsensitive, False); n++;
  }
  label_key = XtCreateManagedWidget("LabelKey", labelWidgetClass, 
				    form_key, args, n);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_key); n++;
  XtSetArg(args[n], XtNeditType, XawtextEdit); n++;
  XtSetArg(args[n], XtNwidth, 235); n++;
  key_value = XtCreateManagedWidget("Key", asciiTextWidgetClass, 
				    form_key, args, n);

  XtOverrideTranslations(key_value, trans_table);
#endif /* CRYPTO */

  if(!RECV_ONLY){  
  /*
   *  Form for video output rate control.
   */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
#ifdef CRYPTO
  XtSetArg(args[n], XtNfromVert, form_key); n++;
#endif
  form_rate = XtCreateManagedWidget("Form", formWidgetClass,
				    mainform_control, args, n);

  /*
   *  Create the rate control selection
   */
  n = 0;
  label_rate = XtCreateManagedWidget(LabelRate, labelWidgetClass,
				     form_rate, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_rate); n++;
  button_rate[0] = XtCreateManagedWidget(ControlFormatItems[0],
					 commandWidgetClass, form_rate, args,
					 (Cardinal) n);
  XtAddCallback(button_rate[0], XtNcallback, CallbackControl,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_rate[0]); n++;
  button_rate[1] = XtCreateManagedWidget(ControlFormatItems[1],
					 commandWidgetClass,form_rate, args,
					 (Cardinal) n);
  XtAddCallback(button_rate[1], XtNcallback, CallbackControl,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_rate[1]); n++;
  button_rate[2] = XtCreateManagedWidget(ControlFormatItems[2],
					 commandWidgetClass, form_rate, args,
					 (Cardinal) n);
  XtAddCallback(button_rate[2], XtNcallback, CallbackControl,
		(XtPointer) NULL);
  
  /*
  *  Form for video frame rate control.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  XtSetArg(args[n], XtNfromVert, form_rate); n++;
  form_frame = XtCreateManagedWidget("Form", formWidgetClass,
				     mainform_control, args, n);

  n = 0;
  label_frame = XtCreateManagedWidget(LabelFrame, labelWidgetClass,
				      form_frame, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_frame); n++;
  button_frame[0] = XtCreateManagedWidget(FrameControlFormatItems[0],
					  commandWidgetClass, form_frame,
					  args, (Cardinal) n);
  XtAddCallback(button_frame[0], XtNcallback, CallbackFrameControl,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_frame[0]); n++;
  button_frame[1] = XtCreateManagedWidget(FrameControlFormatItems[1],
					  commandWidgetClass, form_frame,
					  args, (Cardinal) n);
  XtAddCallback(button_frame[1], XtNcallback, CallbackFrameControl,
		(XtPointer) NULL);
    
  /*
  *  Form for video control mode.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  XtSetArg(args[n], XtNfromVert, form_frame); n++;
  form_mode = XtCreateManagedWidget("Form", formWidgetClass,
				    mainform_control, args, n);

  n = 0;
  label_mode = XtCreateManagedWidget(LabelMode, labelWidgetClass,
				     form_mode, args, n);

  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop);  n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  XtSetArg(args[n], XtNfromHoriz, label_mode); n++;
  button_mode[0] = XtCreateManagedWidget(ModeFormatItems[0],
					  commandWidgetClass, form_mode,
					  args, (Cardinal) n);
  XtAddCallback(button_mode[0], XtNcallback, CallbackMode,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_mode[0]); n++;
  button_mode[1] = XtCreateManagedWidget(ModeFormatItems[1],
					  commandWidgetClass, form_mode,
					  args, (Cardinal) n);
  XtAddCallback(button_mode[1], XtNcallback, CallbackMode,
		(XtPointer) NULL);
 
  /*
  *  Form for bandwidth section.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  XtSetArg(args[n], XtNfromVert, form_mode); n++;
  form_bw = XtCreateManagedWidget("Form", formWidgetClass,
				  mainform_control, args, n);
   
  CreateScale(&form_control, &scroll_control, &name_control,
	      &value_control, form_bw, "FormControl", "NameControl", rate);
  XtAddCallback(scroll_control, XtNjumpProc, CallbackRate, 
		(XtPointer)value_control);
  top = (float) rate / (float)param.max_rate_max;
  XawScrollbarSetThumb(scroll_control, top, 0.05);

  n = 0;
  XtSetArg(args[n], XtNfromVert, form_control); n++;
  label_band1 = XtCreateManagedWidget("LabelB1", labelWidgetClass, 
				      form_bw, args, (Cardinal) n);
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_control); n++;
  XtSetArg(args[n], XtNfromHoriz, label_band1); n++;
  XtSetArg(args[n], XtNinternalWidth, 1); n++;
  label_band2 = XtCreateManagedWidget("LabelB2", labelWidgetClass, 
				      form_bw, args, (Cardinal) n);
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_control); n++;
  XtSetArg(args[n], XtNfromHoriz, label_band2); n++;
  XtSetArg(args[n], XtNinternalWidth, 1); n++;
  label_band3 = XtCreateManagedWidget("LabelB3", labelWidgetClass, 
				      form_bw, args, (Cardinal) n);
  
  /*
  *  Form for frame rate section.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  XtSetArg(args[n], XtNfromVert, form_mode); n++;
  XtSetArg(args[n], XtNfromHoriz, form_bw); n++;
  form_fr = XtCreateManagedWidget("Form", formWidgetClass,
				  mainform_control, args, n);
  
  CreateScale(&form_frms, &scroll_frms, &name_frms, &value_frms,
	      form_fr, "FormFrms", "NameFrms", param.max_frame_rate);
  
  XtAddCallback(scroll_frms, XtNjumpProc, CallbackFrameRate, 
		(XtPointer)value_frms);
  top = (float)param.max_frame_rate / 30.0;
  XawScrollbarSetThumb(scroll_frms, top, 0.05);

  n = 0;
  XtSetArg(args[n], XtNfromVert, form_frms); n++;
  label_ref1 = XtCreateManagedWidget("LabelR1", labelWidgetClass, 
				     form_fr, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_frms); n++;
  XtSetArg(args[n], XtNfromHoriz, label_ref1); n++;
  XtSetArg(args[n], XtNinternalWidth, 1); n++;
  label_ref2 = XtCreateManagedWidget("LabelR2", labelWidgetClass, 
				     form_fr, args, (Cardinal) n);
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_frms); n++;
  XtSetArg(args[n], XtNfromHoriz, label_ref2); n++;
  XtSetArg(args[n], XtNinternalWidth, 1); n++;
  label_ref3 = XtCreateManagedWidget("LabelR3", labelWidgetClass, 
				     form_fr, args, (Cardinal) n);

  }/* ! RECV_ONLY */
  /*
  *  Form for dismiss button.
  */
  n = 0;
  if(!RECV_ONLY){
    XtSetArg(args[n], XtNfromVert, form_bw); n++;
  }
#ifdef CRYPTO
  else{
    XtSetArg(args[n], XtNfromVert, form_key); n++;
  }
#endif /* CRYPTO */
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  form_dismiss = XtCreateManagedWidget("Form", formWidgetClass,
				       mainform_control, args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_dismiss, args, n);
  XtAddCallback(button_dismiss, XtNcallback,
		(XtCallbackProc)PopControlPanel, (XtPointer)NULL);
  
}
/*---------------------------------------------------------------------------*/

/*----------------------- Audio tuning callbacks ----------------------------*/

#ifndef NO_AUDIO
void SetVolumeGain(value)
     int value;
{
  if(! WITHOUT_ANY_AUDIO)
    soundplayvol(value);
  return;
}



void SetRecordGain(value)
     int value;
{
  if(! WITHOUT_ANY_AUDIO)
    soundrecgain(value);
  return;
}


void CallbackVol(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  char value[4];
  Arg args[1];
  
  volume_gain = (int)(MaxVolume*top);
  sprintf(value, "%d", volume_gain);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  SetVolumeGain(volume_gain);
  SetVolume(volume_gain);
}

void CallbackRec(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  char value[4];
  Arg args[1];
  
  record_gain = (int)(MaxRecord*top);
  sprintf(value, "%d", record_gain);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  SetRecordGain(record_gain);
}

void CallbackAudioOutput(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{

  switch(output_speaker){
  case A_SPEAKER:
    SPEAKER = FALSE;
    SetSpeaker(FALSE); 	
    output_speaker = A_HEADPHONE;
    ChangePixmap(output_audio, LabelHeadphone);
    sounddest(A_HEADPHONE);
    break;
  case A_HEADPHONE:
    SPEAKER = TRUE;
    SetSpeaker(AUTO_MUTING ? TRUE : FALSE);
#ifdef SPARCSTATION
    output_speaker = A_EXTERNAL;
    ChangePixmap(output_audio, LabelSpeakerExt); 
    sounddest(A_EXTERNAL);
#else
    output_speaker = A_SPEAKER;
    ChangePixmap(output_audio, LabelSpeaker); 
    sounddest(A_SPEAKER);
#endif
    break;
  case A_EXTERNAL:
    SPEAKER = TRUE;
    SetSpeaker(AUTO_MUTING ? TRUE : FALSE);
    output_speaker = A_SPEAKER;
    ChangePixmap(output_audio, LabelSpeaker); 
    sounddest(A_SPEAKER);
    break;
  }
  return;
}

#ifdef SPARCSTATION
void CallbackAudioInput(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{

  switch(input_microphone){
  case A_MICROPHONE:
    input_microphone = A_LINE_IN;
    ChangePixmap(input_audio, LabelLineIn);
    soundfrom(A_LINE_IN);
    break;
  default:
    input_microphone = A_MICROPHONE;
    ChangePixmap(input_audio, LabelMicrophone);
    soundfrom(A_MICROPHONE);
    break;
  }
  return;
}
#endif /* SPARCSTATION */

void CallbackAntiLarsen(w, closure, call_data)
     Widget w; 
     XtPointer closure, call_data;
{
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
  if(AUTO_MUTING){
    AUTO_MUTING = FALSE;
    ChangeLabel(button_larsen, LabelAntiLarsenOff);
  }else{
    AUTO_MUTING = TRUE;
    ChangeLabel(button_larsen, LabelAntiLarsenOn);
  }
  if(SPEAKER){
    SetSpeaker(AUTO_MUTING ? TRUE : FALSE);
  }else{
    SetSpeaker(FALSE);
  }
  return;
#endif
}
#endif /* NO_AUDIO

/*---------------------------------------------------------------------------*/
#ifndef NO_AUDIO
void CreateAudioPanel(title, panel, parent)
     char *title;
     Widget *panel;
     Widget parent;
{
  Arg args[15];
  Cardinal n;
  Widget form_dismiss, button_dismiss, form_audio_buttons;
  Widget label_audio, label_redondance;
  
  n = 0;
  XtSetArg(args[n], XtNwidth, 300); n++;
  XtSetArg(args[n], XtNheight, 310); n++;

  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			       parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  mainform_audio = XtCreateManagedWidget("MainForm", formWidgetClass,
					 *panel, args, n);

  /*
  * Form for the audio tuning.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form_audio_tuning = XtCreateManagedWidget("FormAudio", formWidgetClass, 
				     mainform_audio,
				     args, (Cardinal) n);
  /*
  *  Create the audio control utilities.
  */
  CreateScale(&form_v_audio, &scroll_v_audio, &name_v_audio, &value_v_audio,
	      form_audio_tuning, "FormVolAudio", "NameVolAudio", volume_gain);
  n = 0;
  XtSetValues(form_v_audio, args, (Cardinal) n);
  if(! AUDIO_INIT_FAILED){
    XtAddCallback(scroll_v_audio, XtNjumpProc, CallbackVol, 
		  (XtPointer)value_v_audio);
    SetVolumeGain(volume_gain);
    SetVolume(volume_gain);
  }
  
  CreateScale(&form_r_audio, &scroll_r_audio, &name_r_audio, &value_r_audio,
	      form_audio_tuning, "FormRecAudio", "NameRecAudio", record_gain);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, form_v_audio); n++;
  XtSetValues(form_r_audio, args, (Cardinal) n);
  if(! AUDIO_INIT_FAILED){
    XtAddCallback(scroll_r_audio, XtNjumpProc, CallbackRec, 
		  (XtPointer)value_r_audio);
    SetRecordGain(record_gain);
  }
  
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
  /*
  * Form for the audio icones & buttons.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_audio_tuning); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  form_audio_buttons = XtCreateManagedWidget("FormAudio", formWidgetClass, 
					     mainform_audio, args, 
					     (Cardinal) n);
  n = 0;
  switch(output_speaker){
  case A_SPEAKER:
     XtSetArg(args[n], XtNbitmap, LabelSpeaker); n++;
     break;
  case A_HEADPHONE:
     XtSetArg(args[n], XtNbitmap, LabelHeadphone); n++;
     break;
  case A_EXTERNAL:
     XtSetArg(args[n], XtNbitmap, LabelSpeakerExt); n++;
     break;
  }
  output_audio = XtCreateManagedWidget("ButtonOutAudio", commandWidgetClass, 
				       form_audio_buttons, args, (Cardinal) n);
  if(! AUDIO_INIT_FAILED){
    XtAddCallback(output_audio, XtNcallback, CallbackAudioOutput,
		  (XtPointer)NULL);
  }

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, output_audio); n++;
  switch(input_microphone){
  case A_MICROPHONE:
     XtSetArg(args[n], XtNbitmap, LabelMicrophone); n++;
     break;
  default:
     XtSetArg(args[n], XtNbitmap, LabelLineIn); n++;
     break;
  }
  input_audio = XtCreateManagedWidget("ButtonInAudio", commandWidgetClass, 
				       form_audio_buttons, args, (Cardinal) n);
#ifdef SPARCSTATION
  if(! AUDIO_INIT_FAILED){
    XtAddCallback(input_audio, XtNcallback, CallbackAudioInput,
		  (XtPointer)NULL);
  }
#endif /* SPARCSTATION */

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, input_audio); n++;
  XtSetArg(args[n], XtNlabel, AUTO_MUTING ? LabelAntiLarsenOn : 
	   LabelAntiLarsenOff); n++;
  button_larsen = XtCreateManagedWidget("ButtonLarsen", commandWidgetClass,
					form_audio_buttons, args, 
					(Cardinal) n);
  XtAddCallback(button_larsen, XtNcallback, CallbackAntiLarsen,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNlabel, LabelReleaseAudio); n++;
  XtSetArg(args[n], XtNfromHoriz, button_larsen); n++;
  XtSetArg(args[n], XtNinternalWidth, 1); n++;
  button_r_audio = XtCreateManagedWidget("ButtonRAudio", commandWidgetClass,
					 form_audio_buttons, args, 
					 (Cardinal) n);
#endif /* SPARCSTATION || AF_AUDIO || SGISTATION */
  
  /*
   *  Form for audio channel.
   */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
  XtSetArg(args[n], XtNfromVert, form_audio_buttons); n++;
#else
  XtSetArg(args[n], XtNfromVert, form_audio_tuning); n++;
#endif
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_audio = XtCreateManagedWidget("Form", formWidgetClass,
				     mainform_audio, args, n);

  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNfromVert, form_audio); n++;
  XtSetArg(args[n], XtNborderWidth, 1); n++;
  form_redondance = XtCreateManagedWidget("Form_redondance", formWidgetClass,
					  mainform_audio, args, n);

  /*
   *  Create the audio selection
   */
#ifndef NO_AUDIO
/***/
  n = 0;
  label_audio = XtCreateManagedWidget(LabelAudio, labelWidgetClass,
				      form_audio, args, n);
/* B0 */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_audio); n++;
  button_audio[0] = XtCreateManagedWidget(AudioFormatItems[0],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[0], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/* B1 */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_audio[0]); n++;
  button_audio[1] = XtCreateManagedWidget(AudioFormatItems[1],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[1], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/* B2 */
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_audio[1]); n++;
  button_audio[2] = XtCreateManagedWidget(AudioFormatItems[2],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[2], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/* B3 */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[0]); n++;
  button_audio[3] = XtCreateManagedWidget(AudioFormatItems[3],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[3], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/*** B4 ***/
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[1]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_audio[3]); n++;
  button_audio[4] = XtCreateManagedWidget(AudioFormatItems[4],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[4], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/* B5 */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_audio[4]); n++;
  button_audio[5] = XtCreateManagedWidget(AudioFormatItems[5],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[5], XtNcallback, CallbackAudio,
		(XtPointer) NULL);

/* B6 */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_audio[5]); n++;
  button_audio[6] = XtCreateManagedWidget(AudioFormatItems[6],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[6], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/* B7 */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[3]); n++;
  button_audio[7] = XtCreateManagedWidget(AudioFormatItems[7],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[7], XtNcallback, CallbackAudio,
		(XtPointer) NULL);
/*** B8 ***/
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_audio[4]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_audio[7]); n++;
  button_audio[8] = XtCreateManagedWidget(AudioFormatItems[8],
					  commandWidgetClass, form_audio,
					  args, (Cardinal) n);
  XtAddCallback(button_audio[8], XtNcallback, CallbackAudio,
		(XtPointer) NULL);


/*------------------------------------------------------------------*/

  n = 0;
  label_redondance = XtCreateManagedWidget(LabelRedondance, labelWidgetClass,
					   form_redondance, args, n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, label_redondance); n++;
  button_redondance[0] = XtCreateManagedWidget(AudioRedondanceItems[0],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[0], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[0]); n++;
  button_redondance[1] = XtCreateManagedWidget(AudioRedondanceItems[1],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[1], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[1]); n++;
  button_redondance[2] = XtCreateManagedWidget(AudioRedondanceItems[2],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[2], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

  /* second line */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[0]); n++;
  
  button_redondance[3] = XtCreateManagedWidget(AudioRedondanceItems[3],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[3], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[1]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[3]); n++;
  button_redondance[4] = XtCreateManagedWidget(AudioRedondanceItems[4],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[4], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[4]); n++;
  button_redondance[5] = XtCreateManagedWidget(AudioRedondanceItems[5],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[5], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[2]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[5]); n++;
  button_redondance[6] = XtCreateManagedWidget(AudioRedondanceItems[6],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[6], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

  /* third line */
  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[3]); n++;
  button_redondance[7] = XtCreateManagedWidget(AudioRedondanceItems[7],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[7], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[4]); n++;
  XtSetArg(args[n], XtNfromHoriz,  button_redondance[7]); n++;
  button_redondance[8] = XtCreateManagedWidget(AudioRedondanceItems[8],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[8], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

  n = 0;
  XtSetArg(args[n], XtNfromVert, button_redondance[5]); n++;
  XtSetArg(args[n], XtNfromHoriz, button_redondance[8]); n++;
  button_redondance[9] = XtCreateManagedWidget(AudioRedondanceItems[9],
					  commandWidgetClass, form_redondance,
					  args, (Cardinal) n);
  XtAddCallback(button_redondance[9], XtNcallback, CallbackRedondance,
		(XtPointer) NULL);

#endif /* ! NO_AUDIO */

  /*
  *  Form for dismiss button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_redondance); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  form_dismiss = XtCreateManagedWidget("Form", formWidgetClass,
				       mainform_audio, args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_dismiss, args, n);
  XtAddCallback(button_dismiss, XtNcallback,
		(XtCallbackProc)PopAudioPanel, (XtPointer)NULL);
}
#endif /* NO_AUDIO */


/*---------------------------------------------------------------------------*/


void CreateInfoPanel(title, panel, parent)
     char *title;
     Widget *panel;
     Widget parent;
{
  Arg args[15];
  Cardinal n;
  Widget mainform, form_text, text_info;
  Widget form_dismiss, button_dismiss;
  static char buffer_text[] = {
    "\tIVS (INRIA Videoconferencing System) \n\
\t\t   version 3.6a\n\
    by Thierry Turletti, <turletti@sophia.inria.fr>  \n\n\
- To talk, click on the <Push to talk> button with the \n\
  right or the left mouse button.  The mike is unmuted \n\
  only when the <Speak now!!> label appears.  The left \n\
  mouse  button works  as a  toggle.  The right  mouse \n\
  button works in ``Push and speak'' mode. \n\
- Mute / hide  individual  sites  by  clicking  on the \n\
  corresponding icons next to the site name. \n\
- Select audio output line by clicking on speaker icon.\n\
- If  the audio  device is  already  taken  by another \n\
  application, the ``Get audio'' label  appears. Click \n\
  on this button once the audio device is free.        \n\
- In unicast mode,  click on the  <Call up>  button to \n\
  ring up your correspondant.                          \n\
- Read the man page.                                 \n\n\
- To receive the last updates of ivs, send a subscribe \n\
  message to ivs-users-request@sophia.inria.fr. You can\n\
  also look at the following URL which includes a lot  \n\
  of information about ivs, real time protocols and    \n\
  pointers to others videoconferencing tools:          \n\
  http://www.inria.fr/rodeo/ivs.html.\n\n\n"};

  n = 0;
  XtSetArg(args[n], XtNwidth, 400); n++;
  XtSetArg(args[n], XtNheight, 355); n++;
  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			      parent, args, n);

  /*
  *  Main Form.
  */
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  mainform = XtCreateManagedWidget("MainForm", formWidgetClass, *panel,
				   args, n);
  /*
  *  Form for dismiss button.
  */
  n = 0;
  XtSetArg(args[n], XtNwidth, 399); n++;
  XtSetArg(args[n], XtNheight, 354); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  form_text = XtCreateManagedWidget("Form", viewportWidgetClass,
				    mainform, args, n);
  n = 0;
  XtSetArg(args[n], XtNx, 100); n++;
  XtSetArg(args[n], XtNstring, buffer_text); n++;

  XtSetArg(args[n], XtNtype, XawAsciiString); n++;
  XtSetArg(args[n], XtNinsertPosition, sizeof(buffer_text)); n++;
  text_info = XtCreateManagedWidget("TextInfo", asciiTextWidgetClass,
                                    form_text, args, n);

  /*
  *  Form for dismiss button.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, form_text); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  form_dismiss = XtCreateManagedWidget("Form", formWidgetClass,
				       mainform, args, n);
  n = 0;
  XtSetArg(args[n], XtNlabel, LabelDismiss); n++;
  XtSetArg(args[n], XtNx, 320); n++;
  XtSetArg(args[n], XtNy, 330); n++;
  button_dismiss = XtCreateManagedWidget("ButtonDismiss", commandWidgetClass,
					 form_text, args, n);
  XtAddCallback(button_dismiss, XtNcallback,
		(XtCallbackProc)PopInfoPanel, (XtPointer)NULL);
  
}
/*---------------------------------------------------------------------------*/
void SetDefaultButtons()
{
  int format=-1;

  if(!RECV_ONLY){
    if(param.COLOR){
      Valid(button_color);
    }else{
      Valid(button_grey);
    }    
    if(param.FREEZE){
      Valid(button_freeze);
    }else{
      Valid(button_unfreeze);
    }    
    if(param.LOCAL_DISPLAY){
      Valid(button_local);
    }else{
      Valid(button_nolocal);
    }    
    switch(param.video_size){
    case QCIF_SIZE:
      Valid(button_qcif);
      break;
    case CIF_SIZE:
      Valid(button_cif);
      break;
    case CIF4_SIZE:
      Valid(button_scif);
      break;
    }    
    switch(param.board_selected){
    case SUNVIDEO_BOARD:
      Valid(button_board1);
      XtSetSensitive(button_scif, FALSE);
      SetSensitiveVideoFormat(SUNVIDEO_BOARD, FALSE);
      break;
    case VIGRAPIX_BOARD:
      Valid(button_board2);
      XtSetSensitive(button_scif, FALSE);
      SetSensitiveVideoFormat(VIGRAPIX_BOARD, TRUE);
#ifdef VIGRAPIX
      if(param.secam_format)
	Valid(button_format[2]);
      else
	Valid(button_format[3]);
#endif
      break;
    case PARALLAX_BOARD:
      Valid(button_board3);
      XtSetSensitive(button_scif, TRUE);
      SetSensitiveVideoFormat(PARALLAX_BOARD, TRUE);
#ifdef PARALLAX
      Valid(button_format[param.px_port_video.standard]);
#endif
      break;
    case VIDEOPIX_BOARD:
      Valid(button_board4);
      XtSetSensitive(button_scif, TRUE);
      SetSensitiveVideoFormat(VIDEOPIX_BOARD, FALSE);
      break;
    case INDY_BOARD:
      Valid(button_board1);
      XtSetSensitive(button_scif, FALSE);
      break;
    case GALILEO_BOARD:
      Valid(button_board2);
      XtSetSensitive(button_scif, FALSE);
      break;
    case INDIGO_BOARD:
      Valid(button_board3);
      XtSetSensitive(button_scif, FALSE);
      break;
    default:
      XtSetSensitive(button_scif, FALSE);
      break;
    }
#ifdef PARALLAX
    if(param.px_port_video.squeeze)
      Valid(button_format[5]);
    if(param.board_selected == PARALLAX_BOARD)
      Valid(button_port[param.px_port_video.channel]);
    else
      Valid(button_port[param.video_port]);
#else
    Valid(button_port[param.video_port]);
#endif

    Valid(button_rate[param.rate_control]);
    Valid(button_mode[!param.PRIVILEGE_QUALITY]);
    Valid(button_frame[param.MAX_FR_RATE_TUNED]);
    if(!param.MAX_FR_RATE_TUNED)
      XtSetSensitive(form_frms, FALSE);
    switch(param.rate_control){
    case AUTOMATIC_CONTROL:
      XtSetSensitive(scroll_control, FALSE);
      break;
    case WITHOUT_CONTROL:
      XtSetSensitive(form_control, FALSE);
      break;
    }
  }/* ! RECV_ONLY */
#ifndef NO_AUDIO  
  if(! WITHOUT_ANY_AUDIO) {
    Valid(button_audio[param.audio_format]);
    Valid(button_redondance[param.audio_redundancy]);
    XtSetSensitive(button_audio[7], FALSE);
    switch (param.audio_redundancy) {
    case PCM_64:
	format = RTP_PCM_CONTENT;
	break;
    case ADPCM_4:
	format = RTP_ADPCM_4;
	break;
    case VADPCM:
	format = RTP_VADPCM_CONTENT;
	break;
    case ADPCM_6:
	format = RTP_ADPCM_6;
	break;
    case ADPCM_5:
	format = RTP_ADPCM_5;
	break;
    case ADPCM_3:
	format = RTP_ADPCM_3;
	break;
    case ADPCM_2:
	format = RTP_ADPCM_2;
	break;
    case LPC:
	format = RTP_LPC;
	break;
    case GSM_13:
	format = RTP_GSM_13;
	break;
    case NONE:
	format = -1;
	break;
    }
    SetRedondancy(format);
  }
#endif  
#ifdef SCREENMACHINE
  Valid(button_board1);
  current_sm = param.current_sm;
  Valid(button_sm[param.current_sm]);
  Valid(button_format[param.video_format]);
  Valid(button_field[param.video_field]);
  switch(nb_sm) {
  case -1:
  case 0:
    XtSetSensitive(button_sm[0], FALSE);
    XtSetSensitive(button_sm[1], FALSE);
    XtSetSensitive(button_sm[2], FALSE);
    XtSetSensitive(button_sm[3], FALSE);
    break;
  case 1: 
    XtSetSensitive(button_sm[1], FALSE);
    XtSetSensitive(button_sm[2], FALSE);
    XtSetSensitive(button_sm[3], FALSE);
    break;
  case 2: 
    XtSetSensitive(button_sm[2], FALSE);
    XtSetSensitive(button_sm[3], FALSE);
    break;
  case 3: 	
    XtSetSensitive(button_sm[3], FALSE);
    break;
  }
#endif /* SCREENMACHINE */
  return;
}

