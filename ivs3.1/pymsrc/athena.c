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
*  File              : athena.c    	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 93/19/1                                             *
*--------------------------------------------------------------------------*
*  Description       : Athena toolkit.                                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/



#include <stdio.h>
#include <string.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Dialog.h>

#define BOOLEAN     unsigned char
#ifndef TRUE
#define TRUE                    1
#endif /* TRUE */
#ifndef FALSE                   
#define FALSE                   0
#endif /* FALSE */

/*
**		IMPORTED VARIABLES
*/
/*----- from display.c ---------*/
 
extern int	cur_brght;
extern float	cur_ctrst;
extern int	cur_int_ctrst;

void 	        rebuild_map_lut();


static XtCallbackProc brightnessCB(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  float top = *((float *) call_data);
  int L ;
  Arg args[1];
  char value[5];
  
  L = (int)(128.0*top - 64);
  if (L == cur_brght) return ;
  sprintf(value, "%d", L);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  cur_brght = L ;
  rebuild_map_lut() ;
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
  if (C == cur_int_ctrst) return ;
  cur_int_ctrst = C ;
  cur_ctrst = (float) pow ((double)2.0, (double) ((double) (cur_int_ctrst) 
						  / 25.0));
  sprintf(value, "%.2f", cur_ctrst);
  XtSetArg (args[0], XtNlabel, value); 
  XtSetValues ((Widget)closure, args, 1);
  rebuild_map_lut() ;
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
  Widget      popup, dialog;
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


void MarkMenu(widget)
     Widget widget;
{
  Arg args[1];
  extern Pixmap mark;

  XtSetArg(args[0], XtNleftBitmap, mark);
  XtSetValues(widget, args, 1);
}


void UnMarkMenu(widget)
     Widget widget;
{
  Arg args[1];

  XtSetArg(args[0], XtNleftBitmap, None);
  XtSetValues(widget, args, 1);
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


void ReverseColor(widget)
     Widget widget;
{
  Arg args[10];
  Pixel bg_color, fg_color;
  int n;

  n = 0;
  XtSetArg (args[n], XtNbackground, &bg_color); n++;
  XtSetArg (args[n], XtNforeground, &fg_color); n++;
  XtGetValues (widget, args, n);
  
  n = 0;
  XtSetArg (args[n], XtNbackground, fg_color); n++;
  XtSetArg (args[n], XtNforeground, bg_color); n++;
  XtSetValues (widget, args, n);
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
  int n;
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
  *value = XtCreateManagedWidget("ValueScale", labelWidgetClass, *form, 
				 args, n);

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



void CreatePanel(title, panel, parent)
     char	*title;
     Widget	*panel;
     Widget	parent;
{
  Arg         	args[15];
  Dimension   	width, height;
  Cardinal    	n;
  char		str_default[6];
  XColor	couleur[4], bidon ;
  Display	*dpy = XtDisplay (parent) ;
  int		screen = DefaultScreen(dpy) ;
  Colormap	palette = DefaultColormap(dpy, screen);
  static Widget form1, form_brightness, scroll_brightness, name_brightness;
  static Widget value_brightness, form_contrast, scroll_contrast;
  static Widget name_contrast, value_contrast;

  if(!(XAllocNamedColor(dpy, palette, "LightSteelBlue", &couleur[0], 
      &bidon) && XAllocNamedColor(dpy, palette, "LightSteelBlue4", 
				  &couleur[1], &bidon))){
    couleur[0].pixel = WhitePixel (dpy, screen) ;
    couleur[1].pixel = WhitePixel (dpy, screen) ;
  }

  n = 0;
  XtSetArg(args[n], XtNwidth, 400); n++;
  XtSetArg(args[n], XtNheight, 60); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;

  *panel = XtCreatePopupShell(title, transientShellWidgetClass, 
			       parent, args, n);

  /*
  *  Form 1.
  */
  n = 0;
  XtSetArg(args[n], XtNfromVert, *panel); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  form1 = XtCreateManagedWidget("Form1", formWidgetClass, *panel,
				args, n);

  /*
  *  Create the Brightness and contrast utilities scales.
  */
  CreateScale(&form_brightness, &scroll_brightness, &name_brightness,
	      &value_brightness, form1, "FormBrightness", 
	      "Brightness", cur_brght);

  sprintf(str_default, "%d", cur_brght);
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  XtSetArg(args[n], XtNforeground, WhitePixel (dpy, screen)); n++;
  XtSetArg(args[n], XtNlabel, str_default); n++;
  XtSetArg(args[n], XtNresize, TRUE); n++;
  XtSetValues(name_brightness, args, 2); 
  XtSetValues(value_brightness, args, n); 
  n = 0;
  XtSetArg(args[n], XtNborderWidth, 0); n++;
  XtSetArg(args[n], XtNbackground, couleur[0].pixel); n++;
  XtSetValues(form_brightness, args, n); 
  n = 0;
  XtSetArg(args[n], XtNorientation, XtorientHorizontal); n++;
  XtSetArg(args[n], XtNwidth, 100); n++;
  XtSetArg(args[n], XtNheight, 4); n++;
  XtSetArg(args[n], XtNborderWidth, 4); n++;
  XtSetArg(args[n], XtNminimumThumb, 4); n++;
  XtSetArg(args[n], XtNthickness, 4); n++;
  XtSetArg(args[n], XtNbackground, couleur[1].pixel); n++;
  XtSetValues(scroll_brightness, args, n); 

  XawScrollbarSetThumb(scroll_brightness, 0.5, 0.05);

  XtAddCallback(scroll_brightness, XtNjumpProc, brightnessCB, 
		(XtPointer)value_brightness);
  
  CreateScale(&form_contrast, &scroll_contrast, &name_contrast,
	      &value_contrast, form1, "FormContrast", "Contrast", 
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
  XtSetArg(args[n], XtNforeground, WhitePixel (dpy, screen)); n++;
  XtSetArg(args[n], XtNlabel, str_default); n++;
  XtSetValues(name_contrast, args, 2); 
  XtSetValues(value_contrast, args, n); 

  XawScrollbarSetThumb(scroll_contrast, 0.5, 0.05);

  XtAddCallback(scroll_contrast, XtNjumpProc, contrastCB, 
		(XtPointer)value_contrast);

}



