/**************************************************************************\
*									   *
*          Copyright (c) 1995 Frank Mueller,nig@nig.rhoen.de               *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* 								           *
* I MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS      *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
***************************************************************************/
/*          		                                                   *
*                                                                          *
*  						                           *
*                                                                          *
* 	                						   *
*  File              : video_sm.c 					   *
*				                                           *
*  Author            : Frank Mueller,nig@nig.rhoen.de,			   *
*		       Fachhochschule Fulda       			   *
*				  					   *
*									   *
*  Description	     : interface proceduces for ScreenMachine II for Linux *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Frank Mueller            |  95/04/01 |          Ver 1.0                  *
* nig@nig.rhoen.de 	   |	       |				   *
\**************************************************************************/

#ifdef SCREENMACHINE

#include <stdio.h>
#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "video_sm.h"


/* 
This is global, because i don't want to do 
every time a grab a frame, a malloc and after that a free 
imagedata contents the actual image, and the data starts at 64 
( before its the header) 

*/

int imagesize;
unsigned char* imagedata;

/* from display.c */
extern BOOLEAN	CONTEXTEXIST;  
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;
extern XImage  *ximage;
extern Widget   appShell;

/* from athena.c */
extern int current_sm; /* Current ScreenMachine II board selected */
extern int nb_sm; /* Number of ScreenMachine II in the platform */

XtAppContext appCtxt;
Display     *dpy;
GC           gc;
Window       win;
Window       icon;
XColor       colors[256];
int          depth;


/*==============================================*
  checking how many SM II we have 
*==============================================*/

int sm_checkvideo()
{
  nb_sm = smInit(0x380, 1024, 768);
  if(nb_sm <= 0) 
    return 0;
  if(nb_sm > 0) 
    return 1;
}



/*==============================================*
              set SM II video port
 *==============================================*/
static void set_video_port(port_video)
    int port_video;
{
    /* Never used here, port management is done in athena.c */
  smSetInput(current_sm, port_video);
}


/*==============================================*
            SM II initialisation
 *==============================================*/
void init_sm_video()
{
 
/* We just open here die SM II , so that we can sent further command to it */
  smOpen(current_sm);
}


/*===============================================*
 SM II  video capture, coding and transmission
 *===============================================*/
void sm_video_encode(param)
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
  BOOLEAN CONTINUE = TRUE;
  BOOLEAN NEW_LOCAL_DISPLAY;
  BOOLEAN ICONIC_DISPLAY = FALSE;
  BOOLEAN DOUBLE = FALSE;
  void (*get_image)();

  nb_cif = (param->video_size == CIF4_SIZE ? 4 : 1);

  if((new_Y  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX*COL_MAX))
     == NULL){
    perror("sm_encode'malloc");
    exit(1);
  }
  if((new_Cb  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("sm_encode'malloc");
    exit(1);
  }
  if((new_Cr  = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("sm_encode'malloc");
    exit(1);
  }

  current_sm = param->current_sm;
  init_sm_video();

  sm_init_grab(param->COLOR, param->video_size, &NG_MAX, &col, &lig, 
	       &get_image);

  {
      register i, j, cpt=0;

      for(i=0; i<288; i++)
	  for(j=0; j<352; j++){
	      new_Y[cpt++] = (j % 150) + 40;
	  }	
      cpt=0;
      for(i=0; i<144; i++)
	  for(j=0; j<176; j++){
	      new_Cb[cpt++] = (j % 150) + 40;
	  }
      cpt=0;
      for(i=0; i<144; i++)
	  for(j=0; j<176; j++){
	      new_Cr[cpt++] = ((176-j) % 150) + 40;
	  }	
  }

  if(param->LOCAL_DISPLAY) {


    init_window(lig, col, "Local display", param->display, &appCtxt,
		&appShell, &depth, &dpy, &win, &icon, &gc, &ximage,
		FALSE);
    if(affCOLOR == MODNB) 
	DOUBLE = TRUE ;
  }

  do{
    if(!param->FREEZE){
      if(param->COLOR) 
	  (*get_image)(new_Y, new_Cb, new_Cr);
      else 
	  (*get_image)(new_Y);
    }

    encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX); 
      
    if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
      build_image(new_Y, new_Cb, new_Cr, DOUBLE, ximage,
		  COL_MAX, 0, col, lig);	
      show(dpy, win, ximage);
    }
    
    NEW_LOCAL_DISPLAY = param->LOCAL_DISPLAY;
    
    if(CONTINUE = HandleCommands(param, &TUNING)) {

      if(port_video != param->video_port){
	port_video = param->video_port;
	set_video_port(port_video);
      }

      if(NEW_LOCAL_DISPLAY != param->LOCAL_DISPLAY) {
	if(param->LOCAL_DISPLAY){
	  if(CONTEXTEXIST)
	      XtMapWidget(appShell);
	  else {
	    init_window(lig, col, "Local display", param->display,
			&appCtxt, &appShell, &depth, &dpy, &win, &icon,
			&gc, &ximage, FALSE);
	    if(affCOLOR == MODNB) DOUBLE = TRUE;
	  }
	}else{
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
    }

/* looking for X events on win */
    if(param->LOCAL_DISPLAY) {
      XEvent evt;
      while(XtAppPending (appCtxt)) {
	XtAppNextEvent(appCtxt, &evt);
	switch(evt.type) {
	case UnmapNotify:
	  if(evt.xunmap.window == icon)
	    ICONIC_DISPLAY = FALSE;
	  break ;
	case MapNotify:
	  if(evt.xmap.window == win){
	    ICONIC_DISPLAY = FALSE;
	    break;
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
/* We are stop the capture, so we can free the imagedata */
  free(imagedata);
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  if(param->LOCAL_DISPLAY)
    XCloseDisplay(dpy);
  smClose(current_sm);
  exit(0);

  /* NOT REACHED */
}

#endif



