/**************************************************************************
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
*                                                                          *
*  File              : videosparc_vfc.c                                    *
*  Last modification : 1995/2/15                                           *
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
*--------------------------------------------------------------------------*
* Andrzej Wozniak         |  93/6/18  | grab moved to grab_vfc.c           *
* wozniak@inria.fr        |           |                                    *
\**************************************************************************/

#ifdef VIDEOPIX

#include <sys/time.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include <vfc_lib.h>

#include "grab_vfc.h"
/*-------------------------------------------*/

/*
**		IMPORTED VARIABLES
*/

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
Window 		icon;
XColor 		colors[256];
int 		depth;

XImage	*alloc_shared_ximage();
void 	destroy_shared_ximage();
void	build_image();
void	build_icon_image();


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

void sun_vfc_video_encode(param)
     S_PARAM *param;

{
  IMAGE_CIF image_coeff[4];
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  static int cif_size=101376;
  static int qcif_size=25344;
  BOOLEAN CONTINUE=TRUE;
  BOOLEAN LOCAL_DISPLAY, TUNING=FALSE;
  BOOLEAN DOUBLE=FALSE ;
  BOOLEAN ICONIC_DISPLAY=FALSE;
  int NG_MAX, i, video_size, video_port;
  void (*get_image)();
  int L_lig, L_col, L_lig2, L_col2, Lcoldiv2;
#ifdef INPUT_FILE
  FILE *F_input;
  extern int n_rate;
  extern double dump_rate;
  int yuv411_size, yuv411_size_div4;
#endif
#ifdef OUTPUT_FILE
  FILE *F_original_file;
  int dim_imageY;
  int dim_imageC;
#endif    
  
  if((new_Y  = (u_char *) malloc(sizeof(char)*4*LIG_MAX*COL_MAX)) == NULL){
    perror("vfc_encode'malloc");
    exit(1);
  }
  if((new_Cb  = (u_char *) malloc(sizeof(char)*4*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("vfc_encode'malloc");
    exit(1);
  }
  if((new_Cr  = (u_char *) malloc(sizeof(char)*4*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("vfc_encode'malloc");
    exit(1);
  }

  VfcInitialize(param->vfc_format, param->video_size,
		param->COLOR, &NG_MAX, &L_lig, &L_col, &Lcoldiv2, &L_col2,
		&L_lig2, &get_image);
  
#ifdef INPUT_FILE
  if((F_input=fopen("input_file", "r")) == NULL){
    fprintf(stderr, "cannot open input_file\n");
    exit(0);
  }
#ifdef YUV_4_1_1
  /* Throw away 182 bytes */
  fread((char *) new_Y, 1, 182, F_input);
  yuv411_size = L_lig * L_col;
  yuv411_size_div4 = yuv411_size / 4;
#endif /* YUV_4_1_1 */    
#endif /* INPUT_FILE */
#ifdef OUTPUT_FILE
  dim_imageY = L_lig*L_col;
  dim_imageC = L_lig*L_col/4;
  if((F_original_file=fopen("original_file", "w")) == NULL){
    fprintf(stderr, "cannot open original_file\n");
    exit(0);
  }
#endif    
  
  InitCOMPY2CCIR(COMPY2CCIR, param->brightness, param->contrast);
  
  if(param->LOCAL_DISPLAY){
    if(param->video_size == CIF4_SIZE){
      init_window(L_lig2, L_col2, "Local display", param->display, &appCtxt, 
		  &appShell, &depth, &dpy, &win, &icon, &gc, &ximage, FALSE);
    }else{
      init_window(L_lig, L_col, "Local display", param->display, &appCtxt, 
		  &appShell, &depth, &dpy, &win, &icon, &gc, &ximage, FALSE);
      if(affCOLOR == MODNB) 
	DOUBLE = TRUE ;
#ifdef BW_DEBUG
      fprintf(stderr, "VIDEOSPARC_VFC.C-video_encode() : retour init_window() - depth = %d, DOUBLE = %d\n", depth, DOUBLE) ;
      fflush(stderr) ;
#endif
    }
  }
  
#define NTSC_FIELD_TIME        33333
#define PAL_SECAM_FIELD_TIME   (40000)
#define NTSC_TURN_TIME (NTSC_FIELD_TIME)
#define PAL_SECAM__TURN_TIME (PAL_SECAM_FIELD_TIME)
#define FREEZE_FRAME_RATE 10
  
#ifdef EXT_FRAME_RATE
#define MAX_FRAME_RATE (EXT_FRAME_RATE)
#else
#define MAX_FRAME_RATE 1
#endif
  
  
  do{
    
    if(param->video_size != CIF4_SIZE){
#ifdef INPUT_FILE
#ifdef YUV_4_1_1
      {
	if(fread(new_Y, 1, yuv411_size, F_input) <= 0)
	  goto stop;
	if(fread(new_Cb, 1, yuv411_size_div4, F_input) <= 0)
	  goto stop;
	if(fread(new_Cr, 1, yuv411_size_div4, F_input) <= 0)
	  goto stop;
      }
#else
      if(param->video_size == CIF_SIZE){
	if(fread(new_Y, 1, L_lig*L_col, F_input) <= 0)
	  goto stop;
	usleep(200000);
      }else{
	register int l;
	for(l=0; l<L_lig; l++)
	  if(fread((new_Y+l*COL_MAX), 1, L_col, F_input) <= 0)
	    goto stop;
	usleep(100000);
	if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
	    static int ccpt=0;
	  build_image(new_Y, new_Cb, new_Cr, DOUBLE, ximage, COL_MAX, 0, L_col,
		      L_lig ) ;
	    ccpt++;
	    show(dpy, win, ximage);
	    if(ccpt==15)
		while(1);
	}
      }
#endif /* YUV_4_1_1 */	
#else
      if(!param->FREEZE){
	if (param->COLOR)
	  (*get_image)(param->vfc_dev, new_Y, new_Cb, new_Cr);
	else
	  (*get_image)(param->vfc_dev, new_Y);
      }
      
#endif /* INPUT_FILE */
#ifdef OUTPUT_FILE
#ifdef GRAB_COLOR
      if(param->video_size == CIF_SIZE){
	if(fwrite((char *) new_Y, 1, dim_imageY, F_original_file) !=
	   dim_imageY){
	  perror("fwrite output file");
	}
	if(fwrite((char *) new_Cb, 1, dim_imageC, F_original_file) !=
	   dim_imageC){
	  perror("fwrite output file");
	}
	if(fwrite((char *) new_Cr, 1, dim_imageC, F_original_file) !=
	   dim_imageC){
	  perror("fwrite output file");
	}
      }else{
	register int l;
	int LigDiv2=L_lig/2;
	int ColDiv2=L_col/2;

	for(l=0; l<L_lig; l++){
	  if(fwrite((char *) (new_Y+l*cif_size), 1, L_col, F_original_file) !=
	     L_col)
	    perror("fwrite output file");
	}
	for(l=0; l<LigDiv2; l++){
	  if(fwrite((char *) (new_Cb+l*qcif_size), 1, ColDiv2,
		    F_original_file) != ColDiv2)
	    perror("fwrite output file");
	}
	for(l=0; l<LigDiv2; l++){
	  if(fwrite((char *) (new_Cr+l*qcif_size), 1, ColDiv2,
		    F_original_file) != ColDiv2)
	    perror("fwrite output file");
	}
      }
#else
      if(param->video_size == CIF_SIZE){
	if(fwrite((char *) new_Y, 1, dim_imageY, F_original_file) !=
	   dim_imageY){
	  perror("fwrite output file");
	}
      }else{
	register int l;
	for(l=0; l<L_lig; l++)
	  if(fwrite((char *) (new_Y+l*COL_MAX), 1, L_col, F_original_file) !=
	     L_col)
	    perror("fwrite output file");
      }
#endif /* GRAB_COLOR */      
#endif /* OUTPUT_FILE */
      
      encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff[0], NG_MAX);
      
      if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
	
	build_image(new_Y, new_Cb, new_Cr, DOUBLE, ximage, COL_MAX, 0, L_col,
		    L_lig ) ;
	show(dpy, win, ximage);
      }
      
    } else { 
/*
** Super CIF image encoding 
*/
      if(!param->FREEZE)
	if(param->COLOR)
	  (*get_image)(param->vfc_dev, new_Y, new_Cb, new_Cr);
	else
	  (*get_image)(param->vfc_dev, new_Y);

      for(i=0; i<4; i++)
	encode_h261_cif4(param, (new_Y+i*cif_size), (new_Cb+i*qcif_size),
			 (new_Cr+i*qcif_size), (GOB *)image_coeff[i], i);
      
      if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY ){
	
	build_image(new_Y, new_Cb, new_Cr, FALSE, ximage, COL_MAX, 0, L_col,
		    L_lig ) ;
	
	build_image((u_char *) (new_Y+cif_size),
		    (u_char *) (new_Cb+qcif_size),
		    (u_char *) (new_Cr+qcif_size),
		    FALSE, ximage, COL_MAX, L_col, L_col, L_lig ) ;
	
	build_image((u_char *) (new_Y+2*cif_size),
		    (u_char *) (new_Cb+2*qcif_size),
		    (u_char *) (new_Cr+2*qcif_size),
		    FALSE, ximage, COL_MAX, (ximage->height>>1)*L_col2,
		    L_col, L_lig);
	
	build_image((u_char *) (new_Y+3*cif_size),
		    (u_char *) (new_Cb+3*qcif_size),
		    (u_char *) (new_Cr+3*qcif_size),
		    FALSE, ximage, COL_MAX,
		    ((ximage->height>>1)*L_col2 + L_col), L_col, L_lig);
	
	show(dpy, win, ximage);
      } /* if(local display) */
    } /* else{} Super CIF encoding */
    
    video_size = param->video_size;
    video_port = param->video_port;
    LOCAL_DISPLAY = param->LOCAL_DISPLAY;
    if(CONTINUE = HandleCommands(param, &TUNING)){
      if(video_port != param->video_port){
	vfc_set_port(param->vfc_dev, param->video_port+1);
      }
      
      if(video_size != param->video_size){
	register int w , h ;
	
	VfcInitialize(param->vfc_format, param->video_size,
		      param->COLOR, &NG_MAX, &L_lig, &L_col, &Lcoldiv2,
		      &L_col2, &L_lig2, &get_image);
	w = (param->video_size == CIF4_SIZE) ? L_col2 : L_col ;
	h = (param->video_size == CIF4_SIZE) ? L_lig2 : L_lig ;
	if (CONTEXTEXIST) {
	  destroy_shared_ximage (dpy, ximage) ;
	  ximage = alloc_shared_ximage (dpy, w, h, depth );
	  XtResizeWidget (appShell, w, h, 0 ) ;
	}
      }
      
      if(LOCAL_DISPLAY != param->LOCAL_DISPLAY){
	if(param->LOCAL_DISPLAY){
	  if (CONTEXTEXIST) XtMapWidget(appShell);
	  else
	    if(param->video_size == CIF4_SIZE){
	      init_window(L_lig2, L_col2, "Local display", param->display, 
			  &appCtxt, &appShell, &depth, &dpy, &win, &icon, 
			  &gc, &ximage, FALSE);
	    }else{
	      init_window(L_lig, L_col, "Local display", param->display,
			  &appCtxt, &appShell, &depth, &dpy, &win, &icon, &gc, 
			  &ximage, FALSE);
	      if(affCOLOR == MODNB) 
		DOUBLE = TRUE;
	    }
	}
	else
	  if(CONTEXTEXIST){
	    if(ICONIC_DISPLAY){
	      ICONIC_DISPLAY =FALSE;
	      XUnmapWindow(dpy, icon);
	    }else
	      XtUnmapWidget(appShell);
	    while (XtAppPending(appCtxt))
	      XtAppProcessEvent(appCtxt, XtIMAll);	
	  }
      }
      
      if(TUNING){
	InitCOMPY2CCIR(COMPY2CCIR, param->brightness, param->contrast);
	TUNING=FALSE;
      }
    }
/******************
** looking for X events on win
*/
    if (param->LOCAL_DISPLAY) {
      XEvent evt;
      while(XtAppPending (appCtxt)){
	XtAppNextEvent(appCtxt, &evt) ;
	switch(evt.type){
	case UnmapNotify :
	  if (evt.xunmap.window == icon) {
	    ICONIC_DISPLAY = FALSE ;
	  }
	  break ;
	case MapNotify :
	  if ( evt.xmap.window == win ) {
	    ICONIC_DISPLAY = FALSE ;
	    break ;
	  }
	  if (evt.xmap.window == icon) {
	    ICONIC_DISPLAY = TRUE ;
	    build_icon_image(icon, param->video_size, new_Y, new_Cb, new_Cr,
			     COL_MAX, L_col, L_lig, TRUE);
	  }
	  break ;
	} /* hctiws evt.type */
	XtDispatchEvent (&evt) ;
      } /* elihw XtAppPending */
    } /* fi LOCAL_DISPLAY */
  }while(CONTINUE);
  
 stop:
  
  /* Clean up and exit */
#ifdef INPUT_FILE
  /* waiting for 5 seconds to allow video decoding */
  sleep(5);
#endif    
  close(param->f2s_fd[0]);
  close(param->s2f_fd[1]);
  if(param->LOCAL_DISPLAY)
    XCloseDisplay(dpy);
#ifndef SOLARIS
  vfc_destroy(param->vfc_dev);
#endif  
#ifdef OUTPUT_FILE
  fclose(F_original_file);
#endif
  free((char *)new_Y);
  free((char *)new_Cb);
  free((char *)new_Cr);
  exit(0);
  /* NOT REACHED */
}

#endif /* VIDEOPIX */



