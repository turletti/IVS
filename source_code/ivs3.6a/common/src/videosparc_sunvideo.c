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
*  File              : videosparc_sunvideo.c                               *
*  Last modification : 1995/2/15                                           *
*  Author            : Thierry Turletti                                    *
*--------------------------------------------------------------------------*
*  Description :  Video interface for SUNVIDEO framegrabber.               *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#ifdef SUNVIDEO
#ident "@(#) videosparc_sunvideo.c 1.1@(#)"

#include <stdio.h>
#include <stdlib.h>
#undef NULL
#include <xil/xil.h> 
#include <sys/time.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "grab_vfc.h"
#include "grab_sunvideo.h"


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



void sun_sunvideo_video_encode(param)
     S_PARAM *param;

{
  extern int n_mba, n_tcoeff, n_cbc, n_dvm;
  extern int tab_rec1[][255], tab_rec2[][255]; /* for quantization */
  IMAGE_CIF image_coeff[4];
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  static int cif_size=101376;
  static int qcif_size=25244;
  int NG_MAX;
  BOOLEAN CONTINUE=TRUE;
  BOOLEAN LOCAL_DISPLAY, TUNING=FALSE;
  BOOLEAN DOUBLE = FALSE ;
  BOOLEAN ICONIC_DISPLAY = FALSE;
  int i, seq=0;
  int video_size, video_port;
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


  if((new_Y  = (u_char *) malloc(sizeof(char)*LIG_MAX*COL_MAX)) == NULL){
    perror("sunvideo_encode'malloc");
    exit(1);
  }
  if((new_Cb  = (u_char *) malloc(sizeof(char)*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("sunvideo_encode'malloc");
    exit(1);
  }
  if((new_Cr  = (u_char *) malloc(sizeof(char)*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("sunvideo_encode'malloc");
    exit(1);
  }

  SunVideoInitialize(param->xil_state, param->video_port,
		     param->video_size, param->COLOR, &NG_MAX, &L_lig,
		     &L_col, &Lcoldiv2, &L_col2, &L_lig2, &get_image);
  
  if(param->LOCAL_DISPLAY){
    if(param->video_size == CIF4_SIZE){
      init_window(L_lig2, L_col2, "Local display", param->display,
		  &appCtxt, &appShell, &depth, &dpy, &win, &icon, &gc,
		  &ximage, FALSE);
    }else{
      init_window(L_lig, L_col, "Local display", param->display, &appCtxt, 
		  &appShell, &depth, &dpy, &win, &icon, &gc, &ximage, FALSE);
      if(affCOLOR == MODNB) 
	DOUBLE = TRUE ;
    }
  }
  
#ifdef INPUT_FILE
  if((F_input=fopen("input_file", "r")) == NULL){
    fprintf(stderr, "cannot open input_file\n");
    exit(0);
  }
#ifdef YUV_4_1_1
  /* Throw away 182 bytes */
/*  fread((char *) new_Y, 1, 182, F_input); */
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
	  if(fread((new_Y+l*cif_size), 1, L_col, F_input) <= 0)
	    goto stop;
	usleep(100000);
      }
#endif /* YUV_4_1_1 */	
#else
      if(!param->FREEZE){
	if(param->COLOR)
	  (*get_image)(new_Y, new_Cb, new_Cr);
	else
	  (*get_image)(new_Y);
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
	  if(fwrite((char *) (new_Y+l*cif_size), 1, L_col, F_original_file) !=
	     L_col)
	    perror("fwrite output file");
      }
#endif /* GRAB_COLOR */      
#endif /* OUTPUT_FILE */
      
      encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff[0],
		  NG_MAX);
      
      if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
	
	build_image((u_char *) new_Y, 
		    (u_char *) new_Cb, 
		    (u_char *) new_Cr, 
		    DOUBLE, ximage, COL_MAX, 0, L_col, L_lig ) ;
	
	show(dpy, win, ximage);
      }
    } else { 
      fprintf(stderr, "Sorry, Super CIF is not yet supported\n");
      goto stop;
    } /* else{} Super CIF encoding */
    

    video_port = param->video_port;
    video_size = param->video_size;
    LOCAL_DISPLAY = param->LOCAL_DISPLAY;
    if(CONTINUE = HandleCommands(param, &TUNING)){
      if(video_port != param->video_port){
	SetSunVideoPort(param->video_port);
      }
      if(video_size != param->video_size){
	register int w , h ;

	video_size = param->video_size;
	SunVideoInitialize(param->xil_state, video_size, param->video_port,
		   param->COLOR, &NG_MAX, &L_lig, &L_col, &Lcoldiv2,
		   &L_col2, &L_lig2, &get_image);
	if(video_size == CIF4_SIZE){
	  w = L_col2;
	  h = L_lig2;
	}else{
	  w = L_col;
	  h = L_lig;
	}
	if(CONTEXTEXIST){
	  destroy_shared_ximage (dpy, ximage) ;
	  ximage = alloc_shared_ximage (dpy, w, h, depth );
	  XtResizeWidget (appShell, w, h, 0 ) ;
	}
      }
      
      if(LOCAL_DISPLAY != param->LOCAL_DISPLAY){
	if(param->LOCAL_DISPLAY){
	  if(CONTEXTEXIST)
	    XtMapWidget(appShell);
	  else
	    if(video_size == CIF4_SIZE){
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
    }

/******************
** looking for X events on win
*/
    if(param->LOCAL_DISPLAY) {
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
  xil_close(param->xil_state);
#ifdef OUTPUT_FILE
  fclose(F_original_file);
#endif    
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  exit(0);
  /* NOT REACHED */
}

#endif /* SUNVIDEO */



