/**************************************************************************
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
*  File              : videosparc_vfc.c                                    *
*  Last modification : November 1992                                       *
*  Author            : Thierry Turletti                                    *
*--------------------------------------------------------------------------*
*  Description :  Video interface for VIDEOPIX framegrabber.               *
*                                                                          *
* Added support for NTSC, and auto format detection,                       *
* David Berry, Sun Microsystems,                                           *
* david.berry@sun.com                                                      *
*                                                                          *
* The VideoPix specific code is originally from work that is               *
* Copyright (c) 1990-1992 by Sun Microsystems, Inc.                        *
* Permission to use, copy, modify, and distribute this software and        * 
* its documentation for any purpose and without fee is hereby granted,     *
* provided that the above copyright notice appear in all copies            * 
* and that both that copyright notice and this permission notice appear    * 
* in supporting documentation.                                             *
*                                                                          *
* This file is provided AS IS with no warranties of any kind.              * 
* The author shall have no liability with respect to the infringement      *
* of copyrights, trade secrets or any patents by this file or any part     * 
* thereof.  In no event will the author be liable for any lost revenue     *
* or profits or other special, indirect and consequential damages.         *
*--------------------------------------------------------------------------*
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
* Winston Dang            |  92/10/1  | alterations were made for speed    *
* wkd@Hawaii.Edu          |           | purposes.                          *
*--------------------------------------------------------------------------*
* Pierre Delamotte        |  93/2/27  | COMPYTOCCIR added to grabbing      *
* delamot@wagner.inria.fr |           | procedures.                        *
*--------------------------------------------------------------------------*
* Thierry Turletti        |  93/3/4   | Now image's format are the same in *
*                         |           | PAL and NTSC formats.              *
*--------------------------------------------------------------------------*
* Pierre Delamotte        |  93/3/20  | Color grabbing procedures added.   *
\**************************************************************************/

#ifdef VIDEOPIX

#include "h261.h"
#include "protocol.h"
#include <vfc_lib.h>

#include "grab_vfc.h"
/*-------------------------------------------*/
typedef union {
  double a;
  u_char Y[4][LIG_MAX][COL_MAX];
} TYPE_M;
typedef union {
  double a;
  u_char C[4][LIG_MAX_DIV2][COL_MAX_DIV2];
} TYPE_C;

#define new_Y Mnew_Y.Y
#define new_Cr Cnew_Cr.C
#define new_Cb Cnew_Cb.C

/*-------------------------------------------*/

/*
**		IMPORTED VARIABLES
*/

extern FILE *F_loss;
extern BOOLEAN STAT_MODE;
/*--------- from display.c --------------*/
extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;

/*-------------------------------------------*/

XtAppContext	appCtxt;
Widget		appShell;
Display 	*dpy;
XImage 		*ximage;
GC 		gc;
Window 		win;
XColor 		colors[256];
int 		depth;

XImage	*alloc_shared_ximage();
void 	destroy_shared_ximage();
void	build_image();

  struct { 
  unsigned int user;
  unsigned int system; 
  } utime; 
/*---------------------------------------------------------------*/

int VfcScanPorts(vfc_dev, format)
     VfcDev  *vfc_dev;
     int     *format;
{
  int in_port;

  for(in_port=VFC_PORT1; in_port<VFC_SVIDEO; in_port++){
    vfc_set_port(vfc_dev, in_port);
    if(vfc_set_format(vfc_dev, VFC_AUTO, format) >= 0)
      return(in_port);
  }
  /* in_port == VFC_SVIDEO */
  vfc_set_port(vfc_dev, in_port);
  if(vfc_set_format(vfc_dev, VFC_AUTO, format) >= 0){
    *format = VFC_NTSC;
    return(in_port);
  }else{
    /* No video available */
    return(-1);
  }
}
/*---------------------------------------------------------------*/

void sun_vfc_video_encode(group, port, ttl, size, fd_f2s, fd_s2f, COLOR, STAT, 
			  FREEZE, LOCAL_DISPLAY, FEEDBACK,
			  display, rate_control, rate_max,
			  vfc_port, vfc_format, vfc_dev, 
			  brightness, contrast, default_quantizer)

     char *group, *port;      /* Address socket */
     u_char ttl;              /* ttl value */
     int size;                /* image's size */
     int fd_f2s[2];           /* pipe descriptor father to son */
     int fd_s2f[2];           /* pipe descriptor son to father*/
     BOOLEAN COLOR;           /* True if color encoding */
     BOOLEAN STAT;            /* True if statistical mode */
     BOOLEAN FREEZE;          /* True if image frozen */
     BOOLEAN LOCAL_DISPLAY;   /* True if local display selected */
     BOOLEAN FEEDBACK;        /* True if feedback allowed */
     char *display;           /* Display name */
     int rate_control;        /* WITHOUT_CONTROL or REFRESH_CONTROL 
				 or QUALITY_CONTROL */
     int rate_max;            /* Maximum bandwidth allowed (kb/s) */
     int vfc_port;            /* VFC input port selected */
     int vfc_format;          /* VFC_PAL or VFC_NTSC */
     VfcDev *vfc_dev;         /* VFC device (already opened) */
     int brightness, contrast;
     int default_quantizer;

{
    extern int n_mba, n_tcoeff, n_cbc, n_dvm;
    extern int tab_rec1[][255], tab_rec2[][255]; /* for quantization */
    IMAGE_CIF image_coeff[4];
static TYPE_M Mnew_Y;
static TYPE_C Cnew_Cb;
static TYPE_C Cnew_Cr;
    int NG_MAX;
    int sock_recv=0;
    BOOLEAN FIRST=TRUE; /* True if it is the first image */
    BOOLEAN CONTINUE=TRUE;
    BOOLEAN NEW_LOCAL_DISPLAY, NEW_SIZE, TUNING;
    BOOLEAN DOUBLE = FALSE ;
    int i;
    int new_size, new_vfc_port;
    void (*get_image)();
    int L_lig, L_col, L_lig2, L_col2, Lcoldiv2 ;

    XEvent	evt_mouse;
#ifdef INPUT_FILE
    FILE *F_input;
    extern int n_rate;
    extern double dump_rate;
#endif

    /* Video control socket declaration */
    sock_recv=declare_listen(PORT_VIDEO_CONTROL);
    if(STAT){
      if((F_loss=fopen(".videoconf.loss", "w")) == NULL){
	fprintf(stderr, "cannot create videoconf.loss file\n");
	STAT_MODE=FALSE;
      }else
	STAT_MODE=TRUE;
    }
#ifdef INPUT_FILE
    if((F_input=fopen("input_file", "r")) == NULL){
      fprintf(stderr, "cannot open input_file\n");
      exit(0);
    }
#endif
    if(vfc_format == VFC_NTSC) 
      COLOR = FALSE ;
    Initialize(vfc_format, size, COLOR, &NG_MAX, &L_lig, &L_col, &Lcoldiv2,
	       &L_col2, &L_lig2, &get_image);

    InitCOMPY2CCIR(COMPY2CCIR, brightness, contrast);

    if(LOCAL_DISPLAY){
      if(size == CIF4_SIZE){
	init_window(L_lig2, L_col2, "Local display", display, &appCtxt, 
		    &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
      }else{
	init_window(L_lig, L_col, "Local display", display, &appCtxt, 
		    &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
        if (affCOLOR == MODNB) DOUBLE = TRUE ;
#ifdef BW_DEBUG
  fprintf(stderr, "VIDEOSPARC_VFC.C-video_encode() : retour init_window() - depth = %d, DOUBLE = %d\n", depth, DOUBLE) ;
  fflush(stderr) ;
#endif
      }
    }

    do{
      if(size != CIF4_SIZE){
#ifdef INPUT_FILE
	if(size == CIF_SIZE){
	  if(fread(new_Y[0], 1, L_lig*L_col, F_input) <= 0)
	    break;
	}else{
	  register int l;
	  for(l=0; l<L_lig; l++)
	    if(fread(&new_Y[0][l][0], 1, L_col, F_input) <= 0)
	      break;
	}
#else
	if(!FREEZE){
          if (COLOR)
	    (*get_image)(vfc_dev, new_Y[0], new_Cb[0], new_Cr[0] );
          else
	    (*get_image)(vfc_dev, new_Y[0]);

	}
#endif
        encode_h261(group, port, ttl, new_Y[0], new_Cb[0], new_Cr[0],
		    image_coeff[0], NG_MAX, sock_recv, FIRST, COLOR, 
		    FEEDBACK, rate_control, rate_max, default_quantizer,
		    fd_s2f[1]);

	if(LOCAL_DISPLAY){

          build_image((u_char *) &new_Y[0][0][0], 
                      (u_char *) &new_Cb[0][0][0], 
                      (u_char *) &new_Cr[0][0][0], 
                      DOUBLE, ximage, 
                      COL_MAX, 0, L_col, L_lig ) ;


	  show(dpy, ximage, ximage->width, ximage->height) ;
	}

      } else { 
/*
** Super CIF image encoding 
*/
        if(!FREEZE)
          if (COLOR)
	    (*get_image)(vfc_dev, new_Y, new_Cb, new_Cr);
          else
	    (*get_image)(vfc_dev, new_Y);

	for(i=0; i<4; i++)
	  encode_h261_cif4(group, port, ttl, new_Y[i], new_Cb[i], 
			     new_Cr[i], (GOB *)image_coeff[i], sock_recv, 
			     FIRST, COLOR, FEEDBACK, i, rate_control, 
			     rate_max, default_quantizer, fd_s2f[1]);

        if(LOCAL_DISPLAY){

          build_image((u_char *) &new_Y[0][0][0], 
                      (u_char *) &new_Cb[0][0][0], 
                      (u_char *) &new_Cr[0][0][0], 
                      FALSE, ximage, 
                      COL_MAX, 0, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[1][0][0], 
                      (u_char *) &new_Cb[1][0][0], 
                      (u_char *) &new_Cr[1][0][0], 
                      FALSE, ximage, 
                      COL_MAX, L_col, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[2][0][0], 
                      (u_char *) &new_Cb[2][0][0], 
                      (u_char *) &new_Cr[2][0][0], 
                      FALSE, ximage, 
                      COL_MAX, L_lig*L_col2, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[3][0][0], 
                      (u_char *) &new_Cb[3][0][0], 
                      (u_char *) &new_Cr[3][0][0], 
                      FALSE, ximage, 
                      COL_MAX, (L_lig*L_col2 + L_col), L_col, L_lig ) ;

	  show(dpy, ximage, L_col2, L_lig2);
        } /* if(local display) */
      } /* else{} Super CIF encoding */

      FIRST = FALSE;
      new_size = size;
      new_vfc_port = vfc_port;
      NEW_LOCAL_DISPLAY = LOCAL_DISPLAY;
      NEW_SIZE = FALSE;
      if(CONTINUE = HandleCommands(fd_f2s, &new_size, &rate_control, 
				   &new_vfc_port, &rate_max, &FREEZE, 
				   &NEW_LOCAL_DISPLAY, &TUNING, &FEEDBACK,
				   &brightness, &contrast)){
	if(new_vfc_port != vfc_port){
	  vfc_port = new_vfc_port;
	  vfc_set_port(vfc_dev, vfc_port);
	}

	if(new_size != size){
          register int w , h ;

	  NEW_SIZE = TRUE;
	  FIRST = TRUE;
	  size = new_size;
	  Initialize(vfc_format, size, COLOR, &NG_MAX, &L_lig, &L_col, 
		     &Lcoldiv2, &L_col2, &L_lig2, &get_image);
          w = (size == CIF4_SIZE) ? L_col2 : L_col ;
          h = (size == CIF4_SIZE) ? L_lig2 : L_lig ;
          if (CONTEXTEXIST) {
            destroy_shared_ximage (dpy, ximage) ;
            ximage = alloc_shared_ximage (dpy, w, h, depth );
            XtResizeWidget (appShell, w, h, 0 ) ;
          }
        }

	if( NEW_LOCAL_DISPLAY != LOCAL_DISPLAY ){
	  LOCAL_DISPLAY = NEW_LOCAL_DISPLAY;
	  if(LOCAL_DISPLAY){
            if (CONTEXTEXIST) XtMapWidget(appShell);
            else
	      if(size == CIF4_SIZE){
	        init_window(L_lig2, L_col2, "Local display", display, 
			    &appCtxt, &appShell, &depth, &dpy, &win, &gc, 
			    &ximage, FALSE);
	      }else{
	        init_window(L_lig, L_col, "Local display", display, &appCtxt, 
			  &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
                if (affCOLOR == MODNB) DOUBLE = TRUE ;
#ifdef BW_DEBUG
  fprintf(stderr, "VIDEOSPARC_VFC.C-video_encode() : retour init_window() - depth = %d, DOUBLE = %d\n", depth, DOUBLE) ;
  fflush(stderr) ;
#endif
	      }
	  }
	  else
            if(CONTEXTEXIST){
              XtUnmapWidget(appShell);
              while ( XtAppPending (appCtxt))
                XtAppProcessEvent (appCtxt, XtIMAll);	
            }
	}

	if(TUNING){
	  InitCOMPY2CCIR(COMPY2CCIR, brightness, contrast);
	  TUNING=FALSE;
	}
      }
/******************
** looking for X events on win
*/
      if (LOCAL_DISPLAY) {
        while ( XtAppPending (appCtxt) )
          XtAppProcessEvent (appCtxt, XtIMAll ) ;	
      } /* fi LOCAL_DISPLAY */

    }while(CONTINUE);

    /* Clean up and exit */
    close(fd_f2s[0]);
    close(fd_s2f[1]);
    if(LOCAL_DISPLAY)
      XCloseDisplay(dpy);
    if(STAT_MODE)
      fclose(F_loss);
#ifdef INPUT_FILE
    fprintf(stderr, "average rate: %fkbps\n", dump_rate/(double)n_rate);
#endif
    vfc_destroy(vfc_dev);
    exit(0);
    /* NOT REACHED */
  }

#endif /* VIDEOPIX */


