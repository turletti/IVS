/**************************************************************************\
*               Copyright 1995 by edgar@prz.tu-berlin.dbp.de               *
*                                 frank@prz.tu-berlin.dbp.de               *
*                                                                          *
*                        All Rights Reserved                               *
*                                                                          *
* Permission to use, copy, modify, and distribute this software and its    *
* documentation for any purpose and without fee is hereby granted,         *
* provided that the above copyright notice appear in all copies and that   *
* both that copyright notice and this permission notice appear in          *
* supporting documentation, and that the names of Technische Universitaet  * 
* Berlin or TUB not be used in advertising or publicity pertaining to      *
* distribution of the software without specific, written prior permission. *
*                                                                          *
* TECHNISCHE UNIVERSITAET BERLIN DISCLAIMS ALL WARRANTIES WITH REGARD TO   *
* THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND   *
* FITNESS, IN NO EVENT SHALL TECHNISCHE UNIVERSITAET BERLIN BE LIABLE      *
* FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES        *
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN    *
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT     *
* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.        *
*                                                                          *
\**************************************************************************/
/**************************************************************************\
* 	                						   *
*  File              : videohp_rops.c                                      *
*  Authors           : edgar@prz.tu-berlin.dbp.de                          *
*                    : frank@prz.tu-berlin.dbp.de                          *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Video interface for HP rops framegrabber.           *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*  Thierry Turletti     |  93/6/28  |  Adaptation to the 3.2 version.      *
\**************************************************************************/

#ifdef HP_ROPS_VIDEO

#include <stdio.h>
#include <sys/fcntl.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"

#define LIG_MAX			288	/* Max number of lines and columns */
#define COL_MAX			352

#define COL_MAX_DIV2            176     /* Half these values */
#define LIG_MAX_DIV2            144     

#define CIF_WIDTH               352     /* Width and height of CIF */
#define CIF_HEIGHT		288

#define QCIF_WIDTH              176     /* Width and height of Quarter CIF */
#define QCIF_HEIGHT		144

#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

#include <sys/ioctl.h>

/* io mapping ioctl command defines */
#define IOMAPID         _IOR('M',0,int)
#define IOMAPMAP        _IOWR('M',1,int)
#define IOMAPUNMAP      _IOWR('M',2,int)

/* io mapping minor number macros */
#define IOMAP_SC(x)     (((x) >> 8) & 0xffff)
#define IOMAP_SIZE(x)   ((x) & 0xff)

/*
**		IMPORTED VARIABLES
*/
/*--------- from display.c --------------*/

extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;

/*
**		GLOBAL VARIABLES
*/

XtAppContext   appCtxt;
Widget         appShell;
Display        *dpy;
XImage 	       *ximage;
GC             gc;
Window         win;
Window         icon;
XColor         colors[256];
int            depth;



void 	grab_image();
void	build_image();
void    build_icon_image();


/* Table mapping 0..255 pixel values to Y values in the range 16..235 */

static unsigned char hp_map_mono_to_y[256];


/* Initialize same */
static void hp_init_mapping() 
{
  int pixel;
  for (pixel = 0; pixel < 256; pixel++)
    hp_map_mono_to_y[pixel] = 16 + (pixel * 219) / 255;
}



int hp_rops_checkvideo() 
{
  int fildes;
  
  if ((fildes = open ("/dev/hptv0_tc", O_RDWR)) < 0) {
    return(0);
  }
  else{
    close(fildes);
    return(1);
  }
}



/* hp_rops_video_encode() - the video capture, coding and tranmission 
* process.
*/
void hp_rops_video_encode(param)
     S_PARAM *param;
{
  IMAGE_CIF image_coeff;
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  BOOLEAN CONTINUE=TRUE;
  BOOLEAN LOCAL_DISPLAY, TUNING=FALSE;
  BOOLEAN ICONIC_DISPLAY=FALSE;
  BOOLEAN DOUBLE=FALSE;
  int NG_MAX, lig, col, fildes;
  unsigned long *video_fb_addr;

  /* Following not used */
  int new_size, new_vfc_port, brightness, contrast;

  if((new_Y  = (u_char *) malloc(sizeof(char)*LIG_MAX*COL_MAX)) == NULL){
    perror("rasterops_encode'malloc");
    exit(1);
  }
  if((new_Cb  = (u_char *) malloc(sizeof(char)*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("rasterops_encode'malloc");
    exit(1);
  }
  if((new_Cr  = (u_char *) malloc(sizeof(char)*LIG_MAX_DIV2*COL_MAX_DIV2))
     == NULL){
    perror("rasterops_encode'malloc");
    exit(1);
  }

  hp_init_mapping();
  encCOLOR = FALSE;
  affCOLOR = MODGREY;

  /* Open video device */
  if ((fildes = open ("/dev/hptv0_tc", O_RDWR)) < 0) {
    perror ("open /dev/hptv0_tc");
    exit (1);
  }
  if(ioctl(fildes, IOMAPMAP, &video_fb_addr) < 0){
    perror("ioctl");
  }

  if(param->video_size == QCIF_SIZE){
    NG_MAX = 5;
    lig = QCIF_HEIGHT;
    col = QCIF_WIDTH;
  }else{
    NG_MAX = 12;
    lig = CIF_HEIGHT;
    col = CIF_WIDTH;
  }

  if(param->LOCAL_DISPLAY){
    init_window(lig, col, "Local display", param->display, &appCtxt,
		&appShell, &depth, &dpy, &win, &icon, &gc, &ximage, FALSE);
    if(affCOLOR == MODNB) 
      DOUBLE = TRUE;
  }

  do{
    if(!param->FREEZE)
      grab_image(video_fb_addr, new_Y, param->video_size) ;

    encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff,
		NG_MAX);

    if(param->LOCAL_DISPLAY && !ICONIC_DISPLAY){
      build_image(new_Y, new_Cb, new_Cr, DOUBLE, ximage, COL_MAX, 0,
		  col, lig );
      show(dpy, win, ximage);
    }
    LOCAL_DISPLAY = param->LOCAL_DISPLAY;
    if(CONTINUE = HandleCommands(param, &TUNING)){
      if(LOCAL_DISPLAY != param->LOCAL_DISPLAY){
	if(param->LOCAL_DISPLAY){
          if(CONTEXTEXIST) 
	    XtMapWidget(appShell);
          else{
	    init_window(lig, col, "Local display", param->display, &appCtxt, 
			&appShell, &depth, &dpy, &win, &icon, &gc, &ximage,
			FALSE);
	    if(affCOLOR == MODNB) 
	      DOUBLE = TRUE;
	  }
	}else
          if(CONTEXTEXIST){
	    if(ICONIC_DISPLAY){
	      ICONIC_DISPLAY =FALSE;
	      XUnmapWindow(dpy, icon);
	    }else
	      XtUnmapWidget(appShell);
	    while(XtAppPending (appCtxt))
	      XtAppProcessEvent (appCtxt, XtIMAll);	
          }
      }
    }
/******************
** looking for X events on win
*/
    if(param->LOCAL_DISPLAY){
      XEvent evt;
      while(XtAppPending (appCtxt)){
	XtAppNextEvent(appCtxt, &evt);
	switch(evt.type){
	case UnmapNotify :
	  if (evt.xunmap.window == icon){
	    ICONIC_DISPLAY = FALSE ;
	  }
	  break ;
	case MapNotify :
	  if(evt.xmap.window == win){
	    ICONIC_DISPLAY = FALSE ;
	    break ;
	  }
	  if(evt.xmap.window == icon){
	    ICONIC_DISPLAY = TRUE ;
	    build_icon_image(icon, param->video_size, new_Y, new_Cb,
			     new_Cr, COL_MAX, col, lig, TRUE);
	  }
	  break ;
	} /* hctiws evt.type */
	XtDispatchEvent (&evt) ;
      } /* elihw XtAppPending */
    } /* fi LOCAL_DISPLAY */
  }while(CONTINUE);
  
  /* Clean up and exit */
  close(param->f2s_fd[0]);
  close(param->s2f_fd[1]);
  if(param->LOCAL_DISPLAY)
    XCloseDisplay(dpy);
  close(fildes);
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  exit(0);
  /* NOT REACHED */
}



void grab_image(video_fb_addr, im_data, size)
     unsigned long *video_fb_addr;
     u_char im_data[][COL_MAX];
     int size;
{
  int i, j;
  int width = CIF_WIDTH;
  int height = CIF_HEIGHT;
  unsigned long *src_pixel_ptr;

  /*
  * Grab the image
  */
  src_pixel_ptr = (unsigned long *) video_fb_addr + 4;

  if(size == CIF_SIZE){
    for (i=0; i < height; i++){
      for (j=0; j < width; j++){
	im_data[i][j] = hp_map_mono_to_y[(u_char)
					 (*src_pixel_ptr++ & 0x000000ff)] ;
      }
      src_pixel_ptr += 896 - width;	
    }
  }else{
    for (i=0; i < height; i++){
      for (j=0; j < width; j++){
	im_data[i][j] = hp_map_mono_to_y[(u_char)
					 (*src_pixel_ptr++ & 0x000000ff)] ;
	src_pixel_ptr++;
      }
      src_pixel_ptr += 1792 - width - width;	
    }
  }
}

#endif /* HP_ROPS_VIDEO */
