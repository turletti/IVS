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
*  File              : videosparc_px.c 	                		   *
*  Authors           : edgar@prz.tu-berlin.dbp.de                          *
*                    : frank@prz.tu-berlin.dbp.de                          *
*  Last Modification : 93/15/1                                             *
*--------------------------------------------------------------------------*
*  Description       : Video Interface for PARALLAX video framegrabber.    *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Thierry Turletti    |  93/19/1  |  Adaptation to the 2.1 version.      *
\**************************************************************************/

#ifdef SUN_PX_VIDEO /* Parallax */

#include <stdio.h>
#include <X11/Xlib.h>
#include <XPlxExt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

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


  /* from video_coder.c: */
extern BOOLEAN STAT_MODE;
extern FILE *F_loss;


void sun_px_cif();
void sun_px_qcif();


/* Table mapping 0..255 pixel values to Y values in the range 16..235 */
static unsigned char sun_map_mono_to_y[256];


/* Initialize same */
static void sun_init_mapping() {
  int pixel;
  
  for (pixel = 0; pixel < 256; pixel++)
    sun_map_mono_to_y[pixel] = 16 + (pixel * 219) / 255;
}



int sun_px_checkvideo() 
{
  int screen;
  Display *dpy;
  GC gc;
  Window win;
  struct _plx_IO *io;

  dpy = XOpenDisplay(0);
  if (!dpy) {
    perror("Couldn't open display\n");
    exit(-1);
  }
  screen = XDefaultScreen(dpy);
  MakeWindow(dpy, &gc, &win, 2, 2);

  io = (struct _plx_IO *)XPlxQueryConfig(dpy, win, gc);
  if( io->hardware != PLX_XVIDEO_24 ){
    XDestroyWindow(dpy, win);
    XFlush(dpy);
    return(0);
  }
  else{
    XDestroyWindow(dpy, win);
    XFlush(dpy);
    return(1);
  }
}


MakeWindow(dpy, pgc, pwin, width, height)
     Display *dpy;
     GC *pgc;
     Window *pwin;
     int width, height;
{
  int screen = XDefaultScreen(dpy);
  
  *pwin = XCreateSimpleWindow(dpy, XRootWindow(dpy, screen), 0, 0, width, 
			      height, 1, BlackPixel(dpy, screen), 
			      WhitePixel(dpy, screen));
  *pgc = XDefaultGC(dpy, screen);

  XMapWindow(dpy, *pwin);
}



/* sun_px_video_encode() -- the video capture, coding and tranmission 
* process.
*/
void sun_px_video_encode(group, port, ttl, size, fd_f2s, fd_s2f, STAT,
			 FREEZE, FEEDBACK, display, rate_control, rate_max,
			 default_quantizer)
     char *group, *port;     /* Address socket */
     u_char ttl;             /* TTL value */
     int size;               /* image's size */
     int fd_f2s[2];          /* Father to son pipe descriptor */
     int fd_s2f[2];          /* Son to father pipe descriptor */
     BOOLEAN STAT;           /* TRUE if statistical mode selected */ 
     BOOLEAN FREEZE;         /* TRUE if image frozen */
     BOOLEAN FEEDBACK;       /* TRUE if feedback is allowed from decoders */
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
  BOOLEAN LOCAL_DISPLAY = TRUE;
  int col, lig;
  int w, h;
  int screen;
  Display *dpy;
  GC gc;
  Window win;
  unsigned long mask, cmask;
  XEvent report;
  plx_signal *plxsignal = NULL;

  int fildes;
  caddr_t addr;
  int prot, flags;
  off_t off;
  size_t len;
  void (*get_image)();
  void sun_init_mapping();

  /* Following not used */
  u_char new_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
  u_char new_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
  int new_size, new_vfc_port, brightness, contrast;
  BOOLEAN TUNING;


  sun_init_mapping();
  switch (size){
  case CIF_SIZE:
    NG_MAX = 12;
    lig = CIF_HEIGHT;
    col = CIF_WIDTH;
    get_image = sun_px_cif;
    break;
  case QCIF_SIZE:
    NG_MAX = 5;
    lig = QCIF_HEIGHT;
    col = QCIF_WIDTH;
    get_image = sun_px_qcif;
    break;
  }
	
  /*
  * opens local display
  */
  dpy = XOpenDisplay(0);
  if (!dpy) {
    perror("Couldn't open display\n");
    exit(-1);
  }
  screen = XDefaultScreen(dpy);
  MakeWindow(dpy, &gc, &win, col, lig);
  XSelectInput(dpy, win, ExposureMask);

  /* Ask for input on channel 0, with the following characteristics:
     Standard = PAL
     Format = Composite
     Type = RGB24    */
  XPlxVideoInputSelect(dpy, win, gc, PLX_INPUT_0, PLX_PAL, PLX_COMP,
		       PLX_RGB24);
  
  plxsignal = (plx_signal *) XPlxQueryVideo(dpy, win, gc);
  
  /* Wait until a signal is input into the board */
  while ( !plxsignal->sync_ok ) {
    fprintf(stderr, " Sync Absent - Hook up an input source");
    usleep(500000);
  }
  XPlxVideoSqueezeLive(dpy, win, gc, 0, 0, plxsignal->w, plxsignal->h,
		       0, 0, col, lig);

  sock_recv = declare_listen(PORT_VIDEO_CONTROL);

  /* Open video device */
  len = 4147200;
  off = 0x140000;
  prot = PROT_READ | PROT_WRITE;
  flags = MAP_SHARED;
  if ((fildes = open ("/dev/fb", O_RDWR)) < 0) {
    perror ("open /dev/fb");
    exit (1);
  }
  if ((addr = mmap(0, len, prot, flags, fildes, off)) == (caddr_t) -1) {
    perror ("mmap");
    exit (1);
  }
  
  if(STAT){
    if((F_loss = fopen(".videoconf.loss", "w")) == NULL) {
      fprintf(stderr, "cannot create videoconf.loss file\n");
    }else
      STAT = FALSE;
  }
  STAT_MODE = FALSE;

  do{
    if(!FREEZE){
#ifdef LOW_LEVEL
      (*get_image)(addr, new_Y, win, dpy);
#else
      (*get_image)(new_Y, win, dpy);
#endif
    }
    encode_h261(group, port, ttl, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX,
		sock_recv, FIRST, COLOR, FEEDBACK, rate_control, rate_max,
		default_quantizer, fd_s2f[1]);

    FIRST = FALSE;
    if(CONTINUE = HandleCommands(fd_f2s, &new_size, &rate_control, 
				 &new_vfc_port, &rate_max, &FREEZE, 
				 &LOCAL_DISPLAY, &TUNING, &brightness,
				 &contrast)){
      if(XCheckWindowEvent(dpy, win, ExposureMask, &report) == True )
	XPlxVideoSqueezeLive(dpy, win, gc, 0, 0, plxsignal->w,
			     plxsignal->h, 0, 0, col, lig);
    }
  }while(CONTINUE);

  /* Clean up and exit */

  close(fd_f2s[0]);
  close(fd_s2f[1]);
  XCloseDisplay(dpy);
  if(STAT_MODE)
    fclose(F_loss);
  close(fildes);
  exit(0);
  /* NOTREACHED */
}



#ifdef LOW_LEVEL
void sun_px_cif(addr, im_data, win, dpy)
     caddr_t addr;
#else
     void sun_px_cif( im_data, win, dpy)
#endif
     u_char im_data[][COL_MAX];
     Window win;
     Display *dpy;
{
  int i, j;
  int width = CIF_WIDTH;
  int height = CIF_HEIGHT;
  unsigned long *src_pixel_ptr;
#ifdef LOW_LEVEL
  int dst_x, dst_y;
  Window child;

  /*
  * Grab the image
  */
  XTranslateCoordinates(dpy,win,DefaultRootWindow(dpy),
			0, 0, &dst_x, &dst_y, &child);
  src_pixel_ptr = (unsigned long *)(caddr_t)addr + ( dst_y * 1152 + dst_x );
#else
  struct _XImage *uncomp_video;
  unsigned long plane_mask;
  
  uncomp_video = (struct _XImage *) XPlxGetImage(dpy, win, 0, 0, width, 
						 height, plane_mask, ZPixmap);
  src_pixel_ptr = (unsigned long *) uncomp_video->data;
#endif

  for (i=0; i < height; i++){
    for (j=0; j < width; j++){
      im_data[i][j] = (u_char)sun_map_mono_to_y[(*src_pixel_ptr++ 
						 & 0x000000ff)];
    }
#ifdef LOW_LEVEL
    src_pixel_ptr += (1152 - width);
#endif
  }
}



#ifdef LOW_LEVEL
void sun_px_qcif(addr, im_data, win, dpy)
     caddr_t addr;
#else
void sun_px_qcif( im_data, win, dpy)
#endif
     u_char im_data[][COL_MAX];
     Window win;
     Display *dpy;
{
  int i, j;
  int width = QCIF_WIDTH;
  int height = QCIF_HEIGHT;
  unsigned long *src_pixel_ptr;
#ifdef LOW_LEVEL
  int dst_x, dst_y;
  Window child;

  /*
  * Grab the image
  */
  XTranslateCoordinates(dpy,win,DefaultRootWindow(dpy), 0, 0, &dst_x, &dst_y, 
			&child);
  src_pixel_ptr = (unsigned long *) addr + ( dst_y * 1152 + dst_x );
#else
  struct _XImage *uncomp_video;
  unsigned long plane_mask;

  uncomp_video = (struct _XImage *) XPlxGetImage(dpy, win, 0, 0, width, 
						 height, plane_mask, ZPixmap);
  src_pixel_ptr = (unsigned long *) uncomp_video->data;
#endif

  for (i=0; i < height; i++){
    for (j=0; j < width; j++){
      im_data[i][j] = (u_char)sun_map_mono_to_y[(*src_pixel_ptr++ 
						 & 0x000000ff)];
    }
#ifdef LOW_LEVEL
    src_pixel_ptr += 1152 - width ;
#endif
  }
}

#endif /* SUN_PX_VIDEO */
