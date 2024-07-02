/**************************************************************************\
*									   *
*          Copyright (c) 1994 by the computer center			   *
*				 university of Stuttgart		   *
*				 Germany           			   *
*                                                                          *
*  						                           *
*  File              : videosgi_indigo.c 			           *
*									   *
*				                                           *
*  Author            : Lei Wang, computer center			   *
*				 university of Stuttgart		   *
*				 Germany  				   *
*									   *
*  Description	     : interface to SGI Indigo Video			   *
*									   *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Lei Wang                 |  94/05/   |          Ver 1.0                  *
* wang@rus.uni-stuttgart.de|	       |				   *
\**************************************************************************/

#ifdef INDIGOVIDEO

#include <stdio.h>
#include <sys/invent.h>
#include <svideo.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "videosgi.h"


extern BOOLEAN	CONTEXTEXIST;  /* from display.c */
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;
extern XImage  *ximage;
extern Widget   appShell;

XtAppContext appCtxt;
Display     *dpy;
GC           gc;
Window       win;
Window       icon;
XColor       colors[256];
int          depth;
SVhandle     sgi_video_hdl = NULL;
static int sgi_video_type = 100; /* supposing there exists
                              no such type */


/*==============================================*
              checking video device
*==============================================*/
int sgi_checkvideo(){
  struct inventory_s *invent_rec;
  struct inventory_s *getinvent();
  
  setinvent();
  while(1) {
    invent_rec = getinvent();
    if(!invent_rec)
      break;
    if(invent_rec->inv_class == 12)
      sgi_video_type = invent_rec->inv_type;
  }
  endinvent();
  
  if(sgi_video_type == 100)
    return 0;
  if(sgi_video_type == INV_VIDEO_LIGHT) { /* light video */
    sgi_video_hdl = svOpenVideo();
    if(!sgi_video_hdl) {
      svPerror("indigo_checkvideo: svOpenVideo");
      exit(1);
    }
    svCloseVideo(sgi_video_hdl);
    sgi_video_hdl = NULL;
    return 1;
  }else{
    fprintf(stderr, "it's not an IndigoVideo\n");
    return 0;
  }
}



/*==============================================*
              set indigo video port
 *==============================================*/
static void set_video_port(port_video)
    int port_video;
{
  long params[4];

  params[0] = SV_SOURCE;
  params[2] = SV_VIDEO_MODE;

  switch(port_video) {
  case 1:
    params[1] = SV_SOURCE2;
    params[3] = SV_COMP;
    break;
  case 2:
    params[1] = SV_SOURCE3;
    params[3] = SV_COMP;
    break;
  case 3:
    params[1] = SV_SOURCE1;
    params[3] = SV_SVIDEO;
    break;
  case 4:
    params[1] = SV_SOURCE2;
    params[3] = SV_SVIDEO;
    break;
  case 5:
    params[1] = SV_SOURCE3;
    params[3] = SV_SVIDEO;
    break;
  case 0:
  default:
    params[1] = SV_SOURCE1;
    params[3] = SV_COMP;
    break;
  }

  if (svSetParam(sgi_video_hdl, params, 4) < 0) {
    svPerror("set_video_port: svSetParam()");
  }
}


/*==============================================*
            IndigoVideo initialisation
 *==============================================*/
void init_indigo_video(color, vd_port)
     BOOLEAN   color;
     int vd_port;
{
  svCaptureInfo info;
  long params[8];
  int n;

  sgi_video_hdl = svOpenVideo();
  if(!sgi_video_hdl) {
    svPerror("init_indigo_video: svOpenVideo");
    exit(1);
  }

 /* Set the state of video input */

  params[0] = SV_COLOR;
  params[2] = SV_INPUT_BYPASS;

  if(!color){
      params[1] = SV_MONO;
      params[3] = TRUE;
  }else{
      params[1] = SV_DEFAULT_COLOR;
      params[3] = FALSE;
  }

  if (svSetParam(sgi_video_hdl, params, 4) < 0) {
    svPerror("init_indigo_video: svSetParam()");
  }

  n = 0;
  params[n++] = SV_BROADCAST;
  params[n++] = SV_PAL;
  params[n++] = SV_CHROMA_TRAP;
  params[n++] = FALSE;
  params[n++] = SV_PREFILTER;
  params[n++] = FALSE;
  params[n++] = SV_FIELDDROP;
  params[n++] = TRUE;

  if (svSetParam(sgi_video_hdl, params, n) < 0) {
    svPerror("init_indigo_video: svSetParam()");
  }

  set_video_port(vd_port);

    info.format = SV_YUV411_FRAMES_AND_BLANKING_BUFFER;
    info.size = 1;
    info.samplingrate = DEFAULT_SAMPLING_RATE;

    if(svInitContinuousCapture(sgi_video_hdl, &info) < 0){
	svPerror("init_indigo_video: svInitContinuousCapture");
	exit(1);
    }
}


/*===============================================*
       video capture, coding and tranmission
 *===============================================*/
void indigo_video_encode(param)
    S_PARAM *param;
{
  IMAGE_CIF image_coeff[4];
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  static int cif_size=101376;
  static int qcif_size=25344; /* 176*144 */
  int NG_MAX, lig, col, i, port_video, nb_cif;
  BOOLEAN TUNING = FALSE;
  BOOLEAN FORCE_GREY = TRUE;
  BOOLEAN CONTINUE = TRUE;
  BOOLEAN NEW_LOCAL_DISPLAY;
  BOOLEAN ICONIC_DISPLAY = FALSE;
  BOOLEAN DOUBLE = FALSE;
  void (*get_image)();

  port_video = param->video_port;

  nb_cif = (param->video_size == CIF4_SIZE ? 4 : 1);
  
  if((new_Y  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX*COL_MAX))
     == NULL){
    perror("indigo_encode'malloc");
    exit(1);
  }
  if((new_Cb  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("indigo_encode'malloc");
    exit(1);
  }
  if((new_Cr  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("indigo_encode'malloc");
    exit(1);
  }
 
  init_indigo_video(param->COLOR, port_video);
  
  indigovideo_init_grab(param->COLOR, param->video_size, &NG_MAX,
			&FORCE_GREY, &col, &lig, &get_image);
  
  if(param->LOCAL_DISPLAY) {
    init_window(lig, col, "Local display", param->display, &appCtxt, 
		&appShell, &depth, &dpy, &win, &icon, &gc, &ximage,
		FORCE_GREY);
    if(affCOLOR == MODNB) 
      DOUBLE = TRUE ;
  }
  
  do{
    if(!param->FREEZE)
      if(param->COLOR)
	(*get_image)(new_Y, new_Cb, new_Cr);
      else
	(*get_image)(new_Y);
    
    if(param->video_size == CIF4_SIZE){
      
      for(i=0; i<4; i++)
	encode_h261_cif4(param, (new_Y+i*cif_size), (new_Cb+i*qcif_size),
			 (new_Cr+i*qcif_size), (GOB *)image_coeff[i], i);
      
      if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
	build_image((u_char *) new_Y, 
		    (u_char *) new_Cb, 
		    (u_char *) new_Cr, 
		    FALSE, ximage, 
		    COL_MAX, 0, CIF_WIDTH, CIF_HEIGHT);
	
	build_image((u_char *) (new_Y+cif_size),
		    (u_char *) (new_Cb+qcif_size),
		    (u_char *) (new_Cr+qcif_size),
		    FALSE, ximage, 
		    COL_MAX, CIF_WIDTH, CIF_WIDTH, CIF_HEIGHT);
	
	build_image((u_char *) (new_Y+2*qcif_size),
		    (u_char *) (new_Cb+2*qcif_size),
		    (u_char *) (new_Cr+2*qcif_size),
		    FALSE, ximage, 
		    COL_MAX, (ximage->height>>1)*SCIF_WIDTH,
		    CIF_WIDTH, CIF_HEIGHT);
	
	build_image((u_char *) (new_Y+3*cif_size),
		    (u_char *) (new_Cb+3*qcif_size),
		    (u_char *) (new_Cr+3*qcif_size),
		    FALSE, ximage, 
		    COL_MAX, ((ximage->height>>1)*SCIF_WIDTH + CIF_WIDTH),
		    CIF_WIDTH, CIF_HEIGHT);
	
	show(dpy, win, ximage);
      }
    }else{
      encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX);
      
      if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
	build_image(new_Y, new_Cb, new_Cr, DOUBLE, ximage, 
		    COL_MAX, 0, col, lig);
	
	show(dpy, win, ximage);
	
      }
    }
    NEW_LOCAL_DISPLAY = param->LOCAL_DISPLAY;
    if(CONTINUE = HandleCommands(param, &TUNING)){
      if(port_video != param->video_port){
	port_video = param->video_port;
	set_video_port(port_video);
      }
      if(NEW_LOCAL_DISPLAY != param->LOCAL_DISPLAY){
	if(param->LOCAL_DISPLAY){
	    if(CONTEXTEXIST)
                XtMapWidget(appShell);
	    else{
		init_window(lig, col, "Local display", param->display,
			    &appCtxt, &appShell, &depth, &dpy, &win, &icon,
			    &gc, &ximage, FORCE_GREY);
	         if(affCOLOR == MODNB) 
		 DOUBLE = TRUE;
	    }
	}else
          if(CONTEXTEXIST){
	    if(ICONIC_DISPLAY){
	      ICONIC_DISPLAY = FALSE;
	      XUnmapWindow(dpy, icon);
	    }else
	      XtUnmapWidget(appShell);
	    while(XtAppPending (appCtxt))
	      XtAppProcessEvent (appCtxt, XtIMAll);	
	  }
      }
    }

/* looking for X events on win */
    if(param->LOCAL_DISPLAY){
      XEvent evt;
      while(XtAppPending (appCtxt)){
	XtAppNextEvent(appCtxt, &evt);
	switch(evt.type){
	case UnmapNotify :
	  if(evt.xunmap.window == icon){
	    ICONIC_DISPLAY = FALSE ;
	  }
	  break ;
	case MapNotify :
	  if(evt.xmap.window == win){
	    ICONIC_DISPLAY = FALSE;
	    break ;
	  }
	  if(evt.xmap.window == icon){
	    ICONIC_DISPLAY = TRUE;
	    build_icon_image(icon, param->video_size, new_Y, new_Cb, new_Cr,
			     COL_MAX, col, lig, TRUE);
	  }
	  break ;
	} /* hctiws evt.type */
	XtDispatchEvent (&evt) ;
      } /* elihw XtAppPending */
    } /* fi param->LOCAL_DISPLAY */
  }while(CONTINUE);
  
  /* Clean up and exit */
  close(param->f2s_fd[0]);
  close(param->s2f_fd[1]);
  svEndContinuousCapture(sgi_video_hdl);
  svCloseVideo(sgi_video_hdl);
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  if(param->LOCAL_DISPLAY)
    XCloseDisplay(dpy);
  exit(0);
  /* NOT REACHED */
}
#endif /* INDIGOVIDEO */

