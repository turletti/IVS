/**************************************************************************\
*          Copyright (c) 1992 by Stichting Mathematisch Centrum,           *
*                                Amsterdam, The Netherlands.               *
*                                                                          *
*                           All Rights Reserved                            *
*                                                                          *
* Permission to use, copy, modify, and distribute this software and its    *
* documentation for any purpose and without fee is hereby granted,         *
* provided that the above copyright notice appear in all copies and that   *
* both that copyright notice and this permission notice appear in          *
* supporting documentation, and that the names of Stichting Mathematisch   *
* Centrum or CWI not be used in advertising or publicity pertaining to     *
* distribution of the software without specific, written prior permission. *
*                                                                          *
* STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO   *
* THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND   *
* FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE      *
* FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES        *
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN    *
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT     *
* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.        *
*                                                                          *
\**************************************************************************/
/**************************************************************************\
* 	                						   *
*  File              : videosgi.c                                          *
*  Author            : Guido van Rossum, CWI, Amsterdam.                   *
*  Last Modification : 1993/03/26                                          *
*--------------------------------------------------------------------------*
*  Description       : Video interface for SGI Indigo.                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
* Thierry Turletti      |  93/19/1  | Adaptation to the 2.1 version.       *
\**************************************************************************/
#ifdef INDIGOVIDEO

#include <stdio.h>
#include <svideo.h>

#include "h261.h"
#include "protocol.h"

#define LIG_MAX			288	/* Max number of lines and columns */
#define COL_MAX			352

#define LIG_MAX_DIV2		144	
#define COL_MAX_DIV2		176

#define CIF_WIDTH               352     /* Width and height of CIF */
#define CIF_HEIGHT		288

#define QCIF_WIDTH              176     /* Width and height of Quarter CIF */
#define QCIF_HEIGHT		144

#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

/*
**		IMPORTED VARIABLES
*/
/*--------- from display.c --------------*/
extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;
extern u_char map_lut[];

  /* from video_coder.c: */
extern BOOLEAN STAT_MODE;
extern FILE *F_loss;

/*
**		GLOBAL VARIABLES
*/

static XtAppContext appCtxt;
static Widget appShell;
static Display *dpy;
static XImage *ximage;
static GC gc;
static Window win;
static XColor colors[256];
static int depth;

void	build_image();

/* Table mapping 0..255 pixel values to Y values in the range 16..235 */

static unsigned char sgi_map_mono_to_y[256];




/* sgi_checkvideo() -- return 1 if video capture is possible, 0 if not */

int sgi_checkvideo() {
	SVhandle dev;

	/* Open the video device.  If this fails, assume we don't have video */
	dev = svOpenVideo();
	if (dev == NULL) {
		svPerror("sgi_checkvideo: svOpenVideo");
		return 0;
	}
	/* Close the device again; sgi_video_encode will re-open it */
	svCloseVideo(dev);
	return 1;
}


static void sgi_init_mapping() {
	int pixel;
	for (pixel = 0; pixel < 256; pixel++)
		sgi_map_mono_to_y[pixel] = 16 + (pixel * 219) / 255;
}


/* sgi_get_image() -- grab a CIF image */

static void sgi_get_image(dev, im_data, lig, col, w, h)
	SVhandle dev;
	u_char im_data[][COL_MAX];
	int lig, col;
	int w, h;
{
	char *ptr;
	long field;

	register u_char *p_data;
	register char *p_buf;
	register int j;
	char *buf_even;
	char *buf_odd;
	int i;
	int xoff, yoff;

	while (1) {
		ptr = NULL;
		if (svGetCaptureData(dev, (void **)&ptr, &field) < 0) {
			svPerror("sgi_get_image: svGetCaptureData");
			exit(1);
		}
		if (ptr != NULL)
			break;
		sginap(1);
	}

	/* The capture area is larger than col*lig; use the center */
	xoff = (w-col)/2;
	yoff = (h-lig)/2;
	buf_odd = &ptr[(yoff/2)*w + xoff];
	buf_even = &ptr[(yoff/2 + h/2)*w + xoff];

	for (i = 0; i < lig; i++) {
		if ((i&1) == 0)
			p_buf = &buf_even[(i/2)*w];
		else
			p_buf = &buf_odd[(i/2)*w];
		p_data = &im_data[i][0];
		for (j = col; --j >= 0; ) {
		  *p_data++ = sgi_map_mono_to_y[*p_buf++];
		}
	}
	svUnlockCaptureData(dev, ptr);
}


/* sgi_video_encode() -- the video capture, coding and tranmission process */


void sgi_video_encode(group, port, ttl, size, fd_f2s, fd_s2f, STAT, COLOR,
		      FREEZE, LOCAL_DISPLAY, FEEDBACK, display, rate_control, 
		      rate_max, default_quantizer)
     char *group, *port;     /* Address socket */
     u_char ttl;             /* TTL value */
     int size;               /* image's size */
     int fd_f2s[2];          /* Father to son pipe descriptor */
     int fd_s2f[2];          /* Son to father pipe descriptor */
     BOOLEAN STAT;           /* TRUE if statistical mode selected */
     BOOLEAN COLOR;          /* TRUE if color encoded */
     BOOLEAN FREEZE;         /* TRUE if image frozen */
     BOOLEAN LOCAL_DISPLAY;  /* TRUE if local display selected */
     BOOLEAN FEEDBACK;       /* TRUE if FEEDBACK is allowed from decoders */
     char *display;          /* Display name */
     int rate_control;       /* WITHOUT_CONTROL or REFRESH_CONTROL or
                                QUALITY_CONTROL */
     int rate_max;           /* Maximum bandwidth allowed (kb/s) */
     int default_quantizer;
     
{
  IMAGE_CIF image_coeff;
  u_char new_Y[LIG_MAX][COL_MAX];
  int NG_MAX;
  int sock_recv = 0;
  BOOLEAN FIRST = TRUE;
  BOOLEAN CONTINUE = TRUE;
  BOOLEAN NEW_LOCAL_DISPLAY;
  int lig, col, coldiv2;
  int w, h;
  BOOLEAN FORCE_GREY = TRUE;

  SVhandle dev;
  svCaptureInfo info;
  long params[4];

  /* Following not used */
  u_char new_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
  u_char new_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
  BOOLEAN TUNING;
  int new_size, new_vfc_port, brightness, contrast;
  
  /* Open video device */
  dev = svOpenVideo();
  if (dev == NULL) {
    fprintf(stderr, "sgi_video_encode: svOpenVideo failed\n");
    svPerror("sgi_video_encode: svOpenVideo");
    exit(1);
  }
  COLOR = FALSE; /* XXX Add color! */

  /* Select B/W non-dithered mode */
  params[0] = SV_COLOR;
  params[1] = SV_MONO;
  params[2] = SV_DITHER;
  params[3] = FALSE;
  if (svSetParam(dev, params, 4) < 0) {
    svPerror("sgi_video_encode: svSetParam(b/w, non-dithered)");
  }

  /* Initialize sgi_map_mono_to_y */
  sgi_init_mapping();

/*************22april93********/ 
  encCOLOR = FALSE;
  affCOLOR = MODGREY;

  if(size == QCIF_SIZE){
    NG_MAX = 5;
    lig = QCIF_HEIGHT;
    col = QCIF_WIDTH;
  }else{
    NG_MAX = 12;
    lig = CIF_HEIGHT;
    col = CIF_WIDTH;
  }
  coldiv2 = col / 2;

  sock_recv = declare_listen(PORT_VIDEO_CONTROL);
  
  w = col;
  h = lig;
  if (w*3/4 < h)
    w = h*4/3;
  if (h*4/3 < w)
    h = w*3/4;
  svQuerySize(dev, w, h, &w, &h);
  svSetSize(dev, w, h);

  /* Initialize continuous capture mode */
  info.format = SV_RGB8_FRAMES;
  info.width = w;
  info.height = h;
  info.size = 1;
  info.samplingrate = 2;
  svInitContinuousCapture(dev, &info);
  w = info.width;
  h = info.height;
  if (w < col || h < lig) {
    fprintf(stderr, "sgi_encode_video: capture size too small\n");
    exit(1);
  }

  if(STAT){
    if((F_loss = fopen(".videoconf.loss", "w")) == NULL) {
      fprintf(stderr, "cannot create videoconf.loss file\n");
    }else
      STAT = FALSE;
  }
  STAT_MODE = STAT;

  if(LOCAL_DISPLAY){
    init_window(lig, col, "Local display", display, &appCtxt, &appShell, 
		&depth, &dpy, &win, &gc, &ximage, FORCE_GREY);
  }

  do{
    if(!FREEZE)
      sgi_get_image(dev, new_Y, lig, col, w, h);
    encode_h261(group, port, ttl, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX, 
		sock_recv, FIRST, COLOR, FEEDBACK, rate_control, rate_max, 
		default_quantizer, fd_s2f[1]);
    if(LOCAL_DISPLAY){
      register int l, c;
      register u_char *im_pix = (u_char *) ximage->data;

      build_image((u_char *) &new_Y[0][0], 
                      (u_char *) &new_Cb[0][0], 
                      (u_char *) &new_Cr[0][0], 
                      FALSE, ximage, 
                      COL_MAX, 0, col, lig ) ;
      show(dpy, ximage, col, lig);
    }
    FIRST = FALSE;
    NEW_LOCAL_DISPLAY = LOCAL_DISPLAY;
    if(CONTINUE = HandleCommands(fd_f2s, &new_size, &rate_control, 
				&new_vfc_port, &rate_max, &FREEZE, 
				&NEW_LOCAL_DISPLAY, &TUNING, &FEEDBACK, 
				&brightness, &contrast)){
      if(NEW_LOCAL_DISPLAY != LOCAL_DISPLAY){
	LOCAL_DISPLAY = NEW_LOCAL_DISPLAY;
	if(LOCAL_DISPLAY){
          if (CONTEXTEXIST) XtMapWidget(appShell);
          else
	    init_window(lig, col, "Local display", display, &appCtxt, 
			&appShell, &depth, &dpy, &win, &gc, &ximage, 
			FORCE_GREY);
	}else
          if(CONTEXTEXIST){
            XtUnmapWidget(appShell);
            while ( XtAppPending (appCtxt))
              XtAppProcessEvent (appCtxt, XtIMAll);	
            }
      }
    }
  }while(CONTINUE);

  /* Clean up and exit */
  close(fd_f2s[0]);
  close(fd_s2f[1]);
  if(LOCAL_DISPLAY)
    XCloseDisplay(dpy);
  svEndContinuousCapture(dev);
  svCloseVideo(dev);
  if(STAT_MODE)
    fclose(F_loss);
  exit(0);
  /* NOT REACHED */
}
#endif /* INDIGOVIDEO */
