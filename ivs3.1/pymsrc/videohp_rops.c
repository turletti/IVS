/**************************************************************************\
*               Copyright 1993 by edgar@prz.tu-berlin.dbp.de               *
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
*  Last Modification : 18/1/93                                             *
*--------------------------------------------------------------------------*
*  Description       : Video interface for HP rops framegrabber.           *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*  Thierry Turletti     |   4/6/93  |  Adaptation to the 2.1 version.      *
\**************************************************************************/

#ifdef HP_ROPS_VIDEO

#include <stdio.h>
#include <sys/fcntl.h>

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
extern u_char map_lut[];


void 	grab_image();
void	build_image();

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
void hp_rops_video_encode(group, port, ttl, size, fd_f2s, fd_s2f, STAT,
			  FREEZE, FEEDBACK, LOCAL_DISPLAY, display, 
			  rate_control, rate_max, default_quantizer)
     char *group, *port;     /* Address socket */
     u_char ttl;             /* TTL value */
     int size;               /* image's size */
     int fd_f2s[2];          /* Father to son pipe descriptor */
     int fd_s2f[2];          /* Son to father pipe descriptor */
     BOOLEAN STAT;           /* TRUE if statistical mode selected */ 
     BOOLEAN FREEZE;         /* TRUE if image frozen */
     BOOLEAN FEEDBACK;       /* TRUE id feedback is allowed */
     BOOLEAN LOCAL_DISPLAY;  /* TRUE if local display selected */
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
  BOOLEAN COLOR = FALSE;
  BOOLEAN NEW_LOCAL_DISPLAY;
  int lig, col, coldiv2;
  int fildes;
  unsigned long  *video_fb_addr;

  /* Following not used */
  u_char new_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
  u_char new_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
  int new_size, new_vfc_port, brightness, contrast;
  BOOLEAN TUNING;


  hp_init_mapping();
/*************22april93********/ 
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

  if(STAT){
    if((F_loss = fopen(".videoconf.loss", "w")) == NULL) {
      fprintf(stderr, "cannot create videoconf.loss file\n");
    }else
      STAT = FALSE;
  }
  STAT_MODE = STAT;

  if(LOCAL_DISPLAY){
    init_window(lig, col, "Local display", display, &appCtxt, &appShell, 
		&depth, &dpy, &win, &gc, &ximage);

  }

  do{
    if(!FREEZE)
      grab_image(video_fb_addr, new_Y, size) ;

    encode_h261(group, port, ttl, new_Y, new_Cb, new_Cr, image_coeff, 
		NG_MAX, sock_recv, FIRST, COLOR, FEEDBACK, rate_control, 
		rate_max, default_quantizer, fd_s2f[1]);

    if(LOCAL_DISPLAY){
      register int l, c;

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
				&NEW_LOCAL_DISPLAY, &TUNING, 
				&brightness, &contrast)){
      if(NEW_LOCAL_DISPLAY != LOCAL_DISPLAY){
	LOCAL_DISPLAY = NEW_LOCAL_DISPLAY;
	if(LOCAL_DISPLAY){
          if (CONTEXTEXIST) XtMapWidget(appShell);
          else
	    init_window(lig, col, "Local display", display, &appCtxt, &appShell, 
		      &depth, &dpy, &win, &gc, &ximage);
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
  if(STAT_MODE)
    fclose(F_loss);
  close(fildes);
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
