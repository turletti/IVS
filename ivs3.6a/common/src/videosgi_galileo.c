/**************************************************************************\
*									   *
*          Copyright (c) 1995 by the computer center			   *
*				 university of Stuttgart		   *
*				 Germany           			   *
*                                                                          *
*  						                           *
*  File              : videosgi_galileo.c 		                   *
*									   *
*				                                           *
*  Author            : Lei Wang, computer center			   *
*				 university of Stuttgart		   *
*				 Germany  				   *
*									   *
*  Description	     : interface for SGI machine with Galileo video board  *
*									   *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Lei Wang                 |  94/04/   |          Ver 1.0                  *
* wang@rus.uni-stuttgart.de|	       |				   *
\**************************************************************************/
#ifdef GALILEOVIDEO

#include <stdio.h>
#include <vl/vl.h>
#include <sys/invent.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "videosgi.h"

extern BOOLEAN	CONTEXTEXIST;  /* from display.c */
extern BOOLEAN  encCOLOR;
extern u_char   affCOLOR;
extern XImage  *ximage;
extern Widget   appShell;

extern VLBuffer trans_buf; /* from grab_galileovideo.c */

extern void galileovideo_init_grab();

/*
**		GLOBAL VARIABLES
*/
XtAppContext appCtxt;
Display     *dpy;
GC           gc;
Window       win;
Window       icon;
XColor       colors[256];
int          depth;

VLDevList    dev_list;
VLServer     sgi_video_hdl;
VLPath       path_hdl;
VLControlValue val;

VLNode        drn;
static VLNode src;
static int sgi_video_type = 100; /* supposing there exists
                                    no such type */


/************************************************
 *		checking video device		*
 ***********************************************/
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
	    fprintf(stderr, "video device is %d\n", invent_rec->inv_type);
    }
    endinvent();

    if(sgi_video_type == 100)
      return 0;
    else
      if((sgi_video_type == INV_VIDEO_VO2) || /* Sirius Video */
         (sgi_video_type == INV_VIDEO_VINO) || /* vino video */
         (sgi_video_type == INV_VIDEO_EXPRESS)) { /* kaleidecope video */
        sgi_video_hdl = vlOpenVideo("");
        if(!sgi_video_hdl) {
        fprintf(stderr, "by sgi_checkvideo: ");
        vlPerror("vlOpenVideo\n");
	return 0; /* Fixed 95/3/10 */
        }
        vlCloseVideo(sgi_video_hdl);
        sgi_video_hdl = NULL;
      } else
        fprintf(stdout, "video device is %d\n", sgi_video_type);
    return 1;
}


/****************************************************************
 *		             set video port                     *
 ****************************************************************/
static void set_video_port(port_video)
     int port_video;
{

/*fprintf(stderr, "port_video = %d\n", port_video);*/

  switch(port_video) {
  case 1 :
    val.intVal = VL_TIMING_625_SQ_PIX;
    if((vlSetControl(sgi_video_hdl, path_hdl, src, VL_TIMING, &val)) < 0) {
      fprintf(stderr, "by set_video_port: ");
      vlPerror("vlSetControl TIMING");
      fprintf(stderr, "\n");
      exit(1);
    }

    val.intVal = VL_FORMAT_COMPOSITE;
    if((vlSetControl(sgi_video_hdl, path_hdl, src, VL_FORMAT, &val)) < 0){
	fprintf(stderr, "by set_video_port: ");
	vlPerror("vlSetControl MUXSWITCH");
	fprintf(stderr, "\n");
	exit(1);
    }
    break;

  case 2 :
    val.intVal = VL_TIMING_625_SQ_PIX;
    if((vlSetControl(sgi_video_hdl, path_hdl, src, VL_TIMING, &val)) < 0) {
      fprintf(stderr, "by set_video_port: ");
      vlPerror("vlSetControl TIMING");
      fprintf(stderr, "\n");
      exit(1);
    }

    val.intVal = VL_FORMAT_SVIDEO;
    if((vlSetControl(sgi_video_hdl, path_hdl, src, VL_FORMAT, &val)) < 0){
	fprintf(stderr, "by set_video_port: ");
	vlPerror("vlSetControl MUXSWITCH");
	fprintf(stderr, "\n");
	exit(1);
    }
    break;

  case 0 :
  default :
    break;
  }
}


/************************************************
 *	Galileo video board initialisation	*
 ***********************************************/
void init_galileo_video(color, port_video)
     BOOLEAN color;
     int port_video;
{
  VLDevList devlist;

  /* Open video device */
  if(!(sgi_video_hdl = vlOpenVideo(""))){
    fprintf(stderr, "by init_galileo_video: ");
    vlPerror("vlOpenVideo\n");
    exit(1);
  }

  if (vlGetDeviceList(sgi_video_hdl, &dev_list) < 0)
    {
      fprintf(stderr, "Error in init_galileo_video: %s\n",
	      vlStrError(vlErrno));
      exit(1);
    }

  if(sgi_video_type == INV_VIDEO_VINO) {
    switch(port_video) {
    case 1 :
    case 2 :
      src = vlGetNode(sgi_video_hdl, VL_SRC, VL_VIDEO, 1);
      break;
    case 0 :
    default :
      src = vlGetNode(sgi_video_hdl, VL_SRC, VL_VIDEO, 0);
      break;
    }
  }

  if(sgi_video_type == INV_VIDEO_EXPRESS)
    src = vlGetNode(sgi_video_hdl, VL_SRC, VL_VIDEO, 0);

  drn = vlGetNode(sgi_video_hdl, VL_DRN, VL_MEM, VL_ANY);

/* using first device */
    if((path_hdl = vlCreatePath(sgi_video_hdl, VL_ANY, src, drn)) < 0){
      fprintf(stderr, "by init_galileo_video: ");
      vlPerror("vlCreatePath\n");
      exit(1);
    }

/* Initialisation of hardware */
    if((vlSetupPaths(sgi_video_hdl, (VLPathList)&path_hdl, 1, VL_LOCK,
		     VL_LOCK)) < 0){
	fprintf(stderr, "by init_galileo_video: ");
	vlPerror("vlSetupPaths\n");
	exit(1);
    }
}


/****************************************************************
 *                      close video device                      *
 ****************************************************************/
void close_video()
{
  vlEndTransfer(sgi_video_hdl, path_hdl);
  vlDeregisterBuffer(sgi_video_hdl, path_hdl, drn, trans_buf);
  vlDestroyBuffer(sgi_video_hdl, trans_buf);
  vlDestroyPath(sgi_video_hdl, path_hdl);
  vlCloseVideo(sgi_video_hdl);
}


/****************************************************************
 *	the video capture, coding and tranmission process	*
 ****************************************************************/
void galileo_video_encode(param)
    S_PARAM *param;
{
  IMAGE_CIF image_coeff[4];
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  static int cif_size=101376;
  static int qcif_size=25344; /* 176*144 */
  int NG_MAX, lig, col, i, port_video, nb_cif;
  BOOLEAN   TUNING = FALSE;
  BOOLEAN   FORCE_GREY = TRUE;
  BOOLEAN   CONTINUE = TRUE;
  BOOLEAN   NEW_LOCAL_DISPLAY;
  BOOLEAN   ICONIC_DISPLAY = FALSE;
  BOOLEAN   DOUBLE = FALSE;
  void      (*get_image)();


  port_video = param->video_port;
  nb_cif = (param->video_size == CIF4_SIZE ? 4 : 1);

  if((new_Y = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX*COL_MAX))
     == NULL){
    perror("galileo_encode'malloc");
    exit(1);
  }
  if((new_Cb = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("galileo_encode'malloc");
    exit(1);
  }
  if((new_Cr = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("galileo_encode'malloc");
    exit(1);
  }

  init_galileo_video(param->COLOR, param->video_port);
  set_video_port(port_video);

  if(sgi_video_type == INV_VIDEO_EXPRESS) /* no NTSC for kaleidecope video */
    galileovideo_init_grab(1, param->COLOR, param->video_size,
			   &NG_MAX, &FORCE_GREY, &col, &lig, &get_image);
  if(sgi_video_type == INV_VIDEO_VO2) /* Sirius Video */
    galileovideo_init_grab(param->pal_format, param->COLOR, param->video_size,
			   &NG_MAX, &FORCE_GREY, &col, &lig, &get_image);
  if(sgi_video_type == INV_VIDEO_VINO)   /* vino video */
    if(!param->video_port) /* Indy Cam */
      galileovideo_init_grab(0, param->COLOR, param->video_size,
			     &NG_MAX, &FORCE_GREY, &col, &lig, &get_image);
    else /* analog cam */
      galileovideo_init_grab(0, param->COLOR, param->video_size,
			     &NG_MAX, &FORCE_GREY, &col, &lig, &get_image);

  if(param->LOCAL_DISPLAY){
    init_window(lig, col, "Local display", param->display, &appCtxt,
		&appShell, &depth, &dpy, &win, &icon, &gc, &ximage,
		FORCE_GREY);
    if(affCOLOR == MODNB)
      DOUBLE = TRUE;
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
        close_video();
        init_galileo_video(param->COLOR, port_video);
        set_video_port(port_video);

        if(sgi_video_type == INV_VIDEO_EXPRESS) 
          galileovideo_init_grab(1, param->COLOR, param->video_size,
	      		         &NG_MAX, &FORCE_GREY, &col, &lig, 
                                 &get_image);
        if(sgi_video_type == INV_VIDEO_VO2)
        galileovideo_init_grab(param->pal_format, param->COLOR, 
                               param->video_size, &NG_MAX, &FORCE_GREY,
                               &col, &lig, &get_image);
        if(sgi_video_type == INV_VIDEO_VINO)
          if(!param->video_port)
            galileovideo_init_grab(0, param->COLOR, param->video_size,
	  		           &NG_MAX, &FORCE_GREY, &col, &lig, 
                                   &get_image);
          else 
            galileovideo_init_grab(0, param->COLOR, param->video_size,
			           &NG_MAX, &FORCE_GREY, &col, &lig, 
                                   &get_image);
        }

      if(NEW_LOCAL_DISPLAY != param->LOCAL_DISPLAY){
	if(param->LOCAL_DISPLAY){
	    if(CONTEXTEXIST)
		XtMapWidget(appShell);
	    else{
		init_window(lig, col, "Local display", param->display,
                            &appCtxt, &appShell, &depth, &dpy, &win,
                            &icon, &gc, &ximage, FORCE_GREY);
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
	    while(XtAppPending(appCtxt))
	      XtAppProcessEvent(appCtxt, XtIMAll);
	  }
      }
    }

/* looking for X events on win */
    if(param->LOCAL_DISPLAY){
      XEvent evt;
      while(XtAppPending(appCtxt)){
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
  close_video();

  if(param->LOCAL_DISPLAY){
      XCloseDisplay(dpy);
  }
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  exit(0);
  /* NOT REACHED */
}
#endif /* GALILEOVIDEO */
