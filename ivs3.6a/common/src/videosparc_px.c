/**************************************************************************\
 *          Copyright (c) 1995 INRIA Rocquencourt FRANCE.                   *
 *                                                                          *
 * Permission to use, copy, modify, and distribute this material for any    *
 * purpose and without fee is hereby granted, provided that the above       *
 * copyright notice and this permission notice appear in all copies.        *
 * WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
 * MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
 * OR IMPLIED WARRANTIES.                                                   *
 \**************************************************************************/
/***************************************************************************\
 * 	                						    *
 *  File              : videosparc_px.c       	                	    *
 *  Author            : Andrzej Wozniak					    *
 *  Last Modification : 1995/3/5                                            *
 *--------------------------------------------------------------------------*
 *  Description       : interface to Parallax video card for image grabbing *
 *                      using low level hardware library                    *
 *--------------------------------------------------------------------------*
 *        Name	         |    Date   |          Modification                *
 *--------------------------------------------------------------------------*
 * Andrzej Wozniak       | 93/10/30  | max frame rate limit added           *
 * wozniak@inria.fr      |           | patch for use with X11R5 parallax    *
 *                       |           | server.                              *
 *--------------------------------------------------------------------------*
 * Andrzej Wozniak       | 95/03/05  | NTSC/QCIF bug corrected              *
 \**************************************************************************/

#ifdef SUN_PX_VIDEO /* Parallax */

#include <stdio.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <XPlxExt.h>
#include <X11/Intrinsic.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "squeeze_px.h"
#include "general_types.h"
#include "h261.h"
#ifdef SOLARIS
#undef abs
#endif
#include "protocol.h"

#include "grab_px.h"

int wait_debug = 0; /* @@@@ */
/*---------------------------------------------------------*/
#define DST_X_MIN 16
#define DST_Y_MIN 16

/*---------------------------------------------------------*/

/* from video_coder.c: */
extern BOOLEAN STAT_MODE;
/*---------------------------------------------------------*/

TYPE_A  red_2_YCrCb[256];
TYPE_A  green_2_YCrCb[256];
TYPE_A  blue_2_YCrCb[256];

/*-------------------- display & grab ---------------------*/

static int col, lig;
static int w, h;
static int screen;
static Display *dpy;
static GC gc;
static Window win;
static unsigned long mask, cmask;
static XEvent report;
static plx_signal *plxsignal = NULL;


/*---------------------------------------------------------*/
int video_std_map[] = {
PLX_NTSC, PLX_PAL, PLX_SECAM, 0, 0
};
int video_hint_map[] = {
0, 0, 0, PLX_PAL, PLX_SECAM
};
/*---------------------------------------------------------*/
static TYPE_GRAB_WINDOW grab_window[2][3] = {
  /*                          startline,    x_offset, noninterlace  */
  /*       col,     lig, sqzx, sqzy,   stopline,  y_offset,         */
2*COL_MAX, 2*LIG_MAX, 4095, 4095,     0,    620,  16,   34,    0, /* CIF4 PAL/SECAM */
  COL_MAX,   LIG_MAX, 2047, 4095,     0,    620,  16,   17,    1, /* CIF  PAL/SECAM */
/*COL_MAX,   LIG_MAX, 2047, 2047,    10,    620,  16,   36,    0, *//* CIF  PAL/SECAM */
/*COL_MAX,   LIG_MAX, 2047, 2047,    10,    620,  16,   36,    0,*//* QCIF PAL/SECAM */
COL_MAX/2, LIG_MAX/2, 1023, 2047,    0,    620,  16,   17,    1, /* QCIF PAL/SECAM */
                                                                                  
  NTSC_W,   NTSC_HU,  4095, 4095,    10, NTSC_H,   0,   24,    0, /* CIF4 NTSC */ 
  COL_MAX,   LIG_MAX, 2457, 2457,    10, NTSC_H,  24,   24,    0, /* CIF  NTSC */  
  COL_MAX/2,   LIG_MAX/2, 2457/2, 2457/2,    10, NTSC_H,  24,   24,    0, /* QCIF NTSC */  
}
;
static TYPE_GRAB_WINDOW *Xwin;

/*---------------------------------------------------------*/

void sun_px_cif4_full();
void sun_px_cif4_half();
void sun_px_cif();
void sun_px_qcif();

void sun_px_cif4_color_full();
void sun_px_cif4_color_half();
void sun_px_cif_color();
void sun_px_qcif_color();

void sun_px_cif4_full_nld();
void sun_px_cif4_half_nld();
void sun_px_cif_nld();
void sun_px_qcif_nld();

void sun_px_cif4_color_full_nld();
void sun_px_cif4_color_half_nld();
void sun_px_cif_color_nld();
void sun_px_qcif_color_nld();
/*---------------------------------------------------------*/
void sun_px_cif_squeeze();
void sun_px_qcif_squeeze();

void sun_px_cif_color_squeeze();
void sun_px_qcif_color_squeeze();

void sun_px_cif_nld_squeeze();
void sun_px_qcif_nld_squeeze();

void sun_px_cif_color_nld_squeeze();
void sun_px_qcif_color_nld_squeeze();
/*---------------------------------------------------------*/
void fast_sun_px_cif4_color_full_nld();
void fast_sun_px_cif_color_nld();       
void fast_sun_px_qcif_color_nld();      

void fast_sun_px_cif4_color_full_nld_ntsc(); 
void fast_sun_px_cif_color_nld_ntsc();       
void fast_sun_px_qcif_color_nld_ntsc();      

void fast_sun_px_cif_color_nld_squeeze();
void fast_sun_px_qcif_color_nld_squeeze();

/*---------------------------------------------------------*/
void sun_px_cif4_full_ntsc();
void sun_px_cif4_half_ntsc();
void sun_px_cif_ntsc();
void sun_px_qcif_ntsc();

void sun_px_cif4_color_full_ntsc();
void sun_px_cif4_color_half_ntsc();
void sun_px_cif_color_ntsc();
void sun_px_qcif_color_ntsc();

void sun_px_cif4_full_nld_ntsc();
void sun_px_cif4_half_nld_ntsc();
void sun_px_cif_nld_ntsc();
void sun_px_qcif_nld_ntsc();

void sun_px_cif4_color_full_nld_ntsc();
void sun_px_cif4_color_half_nld_ntsc();
void sun_px_cif_color_nld_ntsc();
void sun_px_qcif_color_nld_ntsc();
/*---------------------------------------------------------*/

caddr_t OpenHardwareParallax();

/*---------------------------------------------------------*/

void (*fun_tab[8][2][3])() = 
{
    sun_px_cif4_full, sun_px_cif, sun_px_qcif,
    sun_px_cif4_color_full, sun_px_cif_color, sun_px_qcif_color,
    sun_px_cif4_full_nld, sun_px_cif_nld, sun_px_qcif_nld,
    sun_px_cif4_color_full_nld, sun_px_cif_color_nld, sun_px_qcif_color_nld,

    sun_px_cif4_full_ntsc, sun_px_cif_ntsc, sun_px_qcif_ntsc,
    sun_px_cif4_color_full_ntsc, sun_px_cif_color_ntsc, sun_px_qcif_color_ntsc,
    sun_px_cif4_full_nld_ntsc, sun_px_cif_nld_ntsc, sun_px_qcif_nld_ntsc,
    sun_px_cif4_color_full_nld_ntsc, sun_px_cif_color_nld_ntsc, sun_px_qcif_color_nld_ntsc,

    sun_px_cif4_full, sun_px_cif_squeeze, sun_px_qcif_squeeze,
    sun_px_cif4_color_full, sun_px_cif_color_squeeze, sun_px_qcif_color_squeeze,
#ifdef NO_FAST_GRAB
    sun_px_cif4_full_nld, sun_px_cif_nld_squeeze, sun_px_qcif_nld_squeeze,
    sun_px_cif4_color_full_nld, sun_px_cif_color_nld_squeeze, sun_px_qcif_color_nld_squeeze,
#else
    fast_sun_px_cif4_color_full_nld, fast_sun_px_cif_color_nld_squeeze, fast_sun_px_qcif_color_nld_squeeze,
    fast_sun_px_cif4_color_full_nld, fast_sun_px_cif_color_nld_squeeze, fast_sun_px_qcif_color_nld_squeeze,
#endif
    sun_px_cif4_full_ntsc, sun_px_cif_squeeze, sun_px_qcif_squeeze,
    sun_px_cif4_color_full_ntsc, sun_px_cif_color_squeeze, sun_px_qcif_color_squeeze,
    sun_px_cif4_full_nld_ntsc, sun_px_cif_nld_squeeze, sun_px_qcif_nld_squeeze,
    sun_px_cif4_color_full_nld_ntsc, sun_px_cif_color_nld_squeeze, sun_px_qcif_color_nld_squeeze,

    }
;
/*---------------------------------------------------------*/
  plx_IO          *aw_plx_io; /*@@@@*/
  int px_channels = 0;
/*---------------------------------------------------------*/
/************************************************************/
int verify_align(px_vport, x, y)
int *x;
int *y;
PXport *px_vport;
/************************************************************/
{
  int move = 0;
#ifdef DEBUG
printf("\ninit_fb_x = 0x%x\n", *x);
#endif
  if(*x & 0x7){
    *x &= ~0x7;
    move = 1;
  }
    if(*x < DST_X_MIN){
      move = 2;
      *x = DST_X_MIN;
    }else
      if(*x > PX_FB_WIDTH - px_vport->col ){
	move = 2;
	*x = PX_FB_WIDTH - px_vport->col;
      }
    if(*y < DST_Y_MIN){
      move = 2;
      *y = DST_Y_MIN;
    }else
      if(*y > PX_FB_HEIGHT - px_vport->lig - 10){
	move = 2;
	*y = PX_FB_HEIGHT - px_vport->lig - 10;
      }
/*@@@@*/
#ifdef DEBUG
printf("\nfb_x = 0x%x\n", *x);
printf("\nfb_y = 0x%x\n", *y);
printf("\nmove = 0x%x\n", move);
#endif
/*@@@@*/
  return move;
}
/*---------------------------------------------------------*/
int check_tag(px_vport)
PXport *px_vport;
  {
    int w, ww;
    int h;
    unsigned int cc;
    u_int *ptr;
    
    ptr = px_vport->fb_window_addr;
#ifdef DEBUG
    printf("\nlig = %d, col = %d\n", px_vport->lig,px_vport->col);
#endif
    XGrabServer(dpy);
    XSync(dpy, False);
    aw_wait_for_done_px();
    cc = *px_vport->fb_window_addr;
    if( !((cc>>28) & 0x2)){
#ifdef DEBUG      
      printf("\n tag correction\n");
#endif      
      aw_set_video_tag(0);
      h = px_vport->lig;
      ww = px_vport->col; 
      do{
	w = ww;
	do{
	  *ptr++ = 0x20050505;
	}while(--w);
	ptr += PX_FB_WIDTH-ww;
      }while(--h);
    }
    cc = *px_vport->fb_window_addr;
#ifdef DEBUG
    printf("\ncc = %08x\n", cc);
#endif
    XUngrabServer(dpy);/*@@@@*/
    XSync(dpy, False);
    return !((cc>>28) & 0x2);
  }
/************************************************************/
int align_window(px_vport, dst_x, dst_y)
PXport *px_vport;
int *dst_x;
int *dst_y;
/************************************************************/
{
  int x, y, w, h, b, d;
  Window root, child;
  XEvent report;
  int dx, dy;
  int cc = 1;

  XTranslateCoordinates(px_vport->dpy, px_vport->win,
			DefaultRootWindow(px_vport->dpy), 0, 0, &x, &y, &child);
  dx = x & ~0x7; dy = y;
    while(cc){
      switch (cc = verify_align(px_vport, &x, &y)){
      case 0:
	continue;
      case 2:
	dx = x; dy = y;
      }
      XMoveWindow(px_vport->dpy, px_vport->win, dx, dy);
      d = 20;
      while(d-- && XCheckWindowEvent(px_vport->dpy, px_vport->win,
				     StructureNotifyMask, &report) != True)
	usleep(20000);
      XTranslateCoordinates(px_vport->dpy,px_vport->win,DefaultRootWindow(px_vport->dpy),
			    0, 0, &x, &y, &child);
      if(y != dy || x != dx){
	if(abs(y-dy)>4 || abs(x-dx)>8){
	  dx = x;
	  dy = y;
	}else{
#ifdef DEBUG
	  printf("\n###\n");
	  printf("\ndx = 0x%x\n", dx);
#endif
	  XMoveWindow(px_vport->dpy, px_vport->win, dx--, dy);
	  d = 20;
	  while(d-- && XCheckWindowEvent(px_vport->dpy, px_vport->win,
					 StructureNotifyMask, &report) != True)
	    usleep(20000);
	  XTranslateCoordinates(px_vport->dpy,px_vport->win,DefaultRootWindow(px_vport->dpy),
				0, 0, &x, &y, &child);
	}
      }
    }

  XRaiseWindow(px_vport->dpy, px_vport->win);
  d = 5;
  while(d-- && XCheckWindowEvent(px_vport->dpy, px_vport->win,
			  VisibilityChangeMask, &report) != True)
    usleep(20000);

  XFillRectangle(px_vport->dpy, px_vport->win, px_vport->gc,
		 0, 0, px_vport->col, px_vport->lig);
  XSync(px_vport->dpy, True);

  px_vport->fb_window_addr  = px_vport->fb_addr  + ( y * PX_FB_WIDTH + x );
  px_vport->fb_window_addrf = px_vport->fb_addrf + ( y * PX_FB_WIDTH + x );

#ifdef DEBUG
  printf("\nfb_window_addr = 0x%x\n", px_vport->fb_window_addr);
  printf("\nfb_window_addrf = 0x%x\n", px_vport->fb_window_addrf);
#endif
  *dst_x = x;
  *dst_y = y;
  return   check_tag(px_vport);

}
/*---------------------------------------------------------*/

/************************************************************/
void init_RGB_2_YCrCb()
/************************************************************/
{

  int * ptr;
  int i;

#define CSCALE (64>>SH)

  ptr = (int *)&red_2_YCrCb[0];
  for(i=0; i<256; i++){
      *ptr++ = (77*55*i)/(CSCALE*256)+(5<<SH);
      *ptr++ = (((-44*55*i+43*64*256)/(CSCALE*256))<<16)
	+ (((131*55*i+43*64*256)/(CSCALE*256)));
	 }
	
  ptr = (int *)&green_2_YCrCb[0];
  for(i=0; i<256; i++){
      *ptr++ = (150*55*i)/(CSCALE*256)+(6<<SH);
      *ptr++ = (((-87*55*i+43*64*256)/(CSCALE*256))<<16)
	+ (((-110*55*i+43*64*256)/(CSCALE*256)));
	 }
	
  ptr = (int *)&blue_2_YCrCb[0];
  for(i=0; i<256; i++){
      *ptr++ = (29*55*i)/(CSCALE*256)+(5<<SH);
      *ptr++ = (((131*55*i+42*64*256)/(CSCALE*256))<<16)
	+ (((-21*55*i+42*64*256)/(CSCALE*256)));
	 }
}
/*---------------------------------------------------------------------------*/
/************************************************************/
int sun_px_local_display()
/************************************************************/
{
  struct sockaddr name;
  int len = sizeof(name);
  Display *dpy;
  int *majorv, *minorv;
  int s, does_shm;
  Bool *sharedPixmaps;

  return 1;
  
  dpy = XOpenDisplay(0);
  s = ConnectionNumber(dpy);
  if(getsockname(s, &name, &len) == -1){
    perror("getsockname");
    return(0);
  }
  if(len == 0){
#ifdef MITSHM
    does_shm = XShmQueryVersion(dpy, &majorv, &minorv, &sharedPixmaps);
    if(does_shm){
      return(1);
    }
#endif /* MITSHM */
  }
  return(0);
}

#define MAXHOSTNAMELEN_1 128
char px_dpy_name[MAXHOSTNAMELEN_1+20];

/************************************************************/
int sun_px_checkvideo()
/************************************************************/
{

  int screen;
  /*Display *dpy;*/
  GC gc;
  Window win;
  int fd;

  char** listext;
  char dpy_name_ext[] = ":0.0";
  int i;
  int loop;
  int nextensions;
  char* px_dpy_env_var = "PARALLAX_DISPLAY";
  char* dpy_name;

#ifdef IMAGE_FROM_FILE
  return 1; /* dummy return */
#endif
  
  if(px_channels)
    return px_channels;
  
  if((fd = open("/dev/tvtwo0", O_RDWR, 0)) < 0){
    fprintf(stderr, "\ncan't open /dev/tvtwo0, O_RDWR\n");
    return(0);
  }
  close(fd);

  if((dpy_name = 
      (char *)getenv(px_dpy_env_var)) == NULL){

    gethostname(px_dpy_name, MAXHOSTNAMELEN_1);
    strcat(px_dpy_name, dpy_name_ext);
    loop = 1;
  }else{
    strcpy(px_dpy_name, dpy_name);
    loop = 4;
  }
  
  for( ;loop--;  px_dpy_name[strlen(px_dpy_name)-1]++){
    if(!(dpy = XOpenDisplay(px_dpy_name)))
      continue;

    listext = XListExtensions(dpy, &nextensions);
    
#ifdef VIDEO_DEBUG
    for(i=0; i< nextensions ; i++){
      fprintf(stderr, "Extension %d: %s\n", i+1, listext[i]);
    }
#endif

    for(i=0; i< nextensions ; i++){
      
      if(strstr(listext[i], "Parallax")!= NULL)
	goto video_ext;
      if(strstr(listext[i], "parallax")!= NULL)
	goto video_ext;
      if(strstr(listext[i], "PARALLAX")!= NULL)
	goto video_ext;
    }
    XFreeExtensionList(listext);
    XCloseDisplay(dpy);

  }
    fprintf(stderr, "Can't open display = %s\n", px_dpy_name);
    fprintf(stderr, "Check your environment variable %s\n",
	    px_dpy_env_var);
    close(fd);
    return 0;

    video_ext:

  screen = XDefaultScreen(dpy);
  
  MakeWindow(dpy, &gc, &win, 2, 2,0,0);
  aw_plx_io = (plx_IO *)XPlxQueryConfig(dpy, win, gc);
  if( aw_plx_io->hardware != PLX_XVIDEO_24 ){
    XDestroyWindow(dpy, win);
    XFlush(dpy);
    return(0);
  }else{
    {
       plx_IO_list     *plx_in;
       plx_in = aw_plx_io->inputs;
       while(plx_in != NULL)
	{
	  px_channels++;
	  plx_in = plx_in->next_IO;
	}
      
      XDestroyWindow(dpy, win);
      XFlush(dpy);
      return(px_channels);
    }
  }
    XCloseDisplay(dpy);
}


/*---------------------------------------------------------*/
/************************************************************/
plx_signal  *select_input_channel_px(channel, standard, format, std_hint)
int *channel;
int *standard;
int *format;
int std_hint;
/************************************************************/
  {
    /* channel = 1, 2 ...                */
    /* channel = 0 automatic selection   */
    /* std_hint PLX_PAL,  PLX_SECAM      */

    plx_IO_list     *plx_in;
    plx_signal_list *signal_in;
    plx_format_list *format_in;
    plx_signal      *video_signal;    
    int ch = 0;
    int i;
    int screen;
    /*Display *dpy;*/
    GC gc;
    Window win;
    XGCValues values;
    unsigned long valuemask;
    XFontStruct *xfs;
    int chan;
    int std;
    int first = 0;
    int loop = 20;
#ifdef DEBUG
    printf("\n*channel=%d, *standard=%d,*format=%d, std_hint=%d\n",
	   *channel, *standard, *format, std_hint);
#endif

    dpy = XOpenDisplay(px_dpy_name);
    if (!dpy) {
      perror("Couldn't open display\n");
      exit(-1);
    }
    screen = XDefaultScreen(dpy);
    MakeWindow(dpy, &gc, &win, 240, 16, 16, 16);
    XStoreName(dpy, win, "QUERY VIDEO SIGNAL");
    XSync(dpy, False);
    plx_in = aw_plx_io->inputs;
    /*while(!wait_debug);*//*@@@@*/
    while(plx_in != NULL && *channel != 0)
      {
	if(++ch == *channel)
	  break;
	plx_in = plx_in->next_IO;
      }

    /* */do{ /* */
      u_int  status;
      format_in =  plx_in->format_list;
      do{
	signal_in = plx_in->signal_list;
	for(;signal_in != NULL; signal_in = signal_in->next_signal){
	  XPlxVideoInputSelect(dpy, win, gc,
			       plx_in->channel, signal_in->signal->standard,
			       format_in->format,  PLX_RGB24);
	  video_signal = (plx_signal *) XPlxQueryVideo(dpy, win, gc);
	  XSync(dpy, True);/*@@@@*/
	  switch(format_in->format)
	    {
	    case PLX_COMP:
	    case PLX_YC:

	  query_dmsd_sync(plx_in->channel);
	  usleep(100000);
	  status = query_dmsd_sync(plx_in->channel);
	     if(status & 0x40 || !first++ ){
	       query_dmsd_sync(plx_in->channel);
	       sleep(1);
	       query_dmsd_sync(plx_in->channel);
	       continue;
	    }
	      switch(signal_in->signal->standard)
		{
		case PLX_NTSC:
		  if((status & 0x20) && (!*standard || *standard == PLX_NTSC))
		    break;
		    continue;

		case PLX_PAL:
		  if(!(status & 0x20) &&
		     ((!*standard && std_hint == PLX_PAL) || *standard ==
		      PLX_PAL))
		    break;
		  continue;
		case PLX_SECAM:
		  if(!(status & 0x20) &&
		     ((!*standard && std_hint == PLX_SECAM) || *standard ==
		      PLX_SECAM))
		    break;
		  continue;
		}
	      break;

	    case PLX_YUV:
	    case PLX_RGB:
	      if(!video_signal->sync_ok){
		sleep(2);
		continue;
	      }
	      break;
	    }
	      l_ret:
	      XDestroyWindow(dpy, win);
	      XSync(dpy, False);
	      *channel = plx_in->channel;
	      *format = format_in->format;
	      *standard = video_signal->standard;
	      return video_signal;
	}
	format_in = format_in->next_format;
      }while(format_in != NULL);
      if(*channel==0)
	if((plx_in = plx_in->next_IO) == NULL)
	  plx_in = aw_plx_io->inputs;
    }while(plx_in != NULL);
    perror("unknown format/input combination\n");
    exit(-1);
  }
/*---------------------------------------------------------*/

/************************************************************/
MakeWindow(dpy, pgc, pwin, width, height, x, y)
     Display *dpy;
     GC *pgc;
     Window *pwin;
     int width, height;
     int x, y;
/************************************************************/
{
  XSetWindowAttributes  attributes;
  XSizeHints hints;
  unsigned long valuemask = CWOverrideRedirect;
  Visual  *visual = CopyFromParent;
  int screen = XDefaultScreen(dpy);
  
  hints.flags = USPosition | PPosition | PSize;
  hints.x = x;
  hints.y = y;
  hints.width = width;
  hints.height = height;
  
  *pwin = XCreateSimpleWindow(dpy, XRootWindow(dpy, screen),
			      x, y, width, height, 1,
			      BlackPixel(dpy, screen), 
			      WhitePixel(dpy, screen));
  XSetWMSizeHints(dpy, *pwin, &hints, XA_WM_NORMAL_HINTS);
  
  attributes.override_redirect = True;
  *pgc = XDefaultGC(dpy, screen);
  XMapWindow(dpy, *pwin);
}

/*---------------------------------------------------------*/

/************************************************************/
MakeSWindow(dpy, pgc, pwin, width, height, x, y)
     Display *dpy;
     GC *pgc;
     Window *pwin;
     int width, height;
     int x, y;
/************************************************************/
{
  XSetWindowAttributes  attributes;
  XSizeHints hints;
  XGCValues xgc_values;
  unsigned long xgc_valuemask = 0;
  unsigned long valuemask = CWOverrideRedirect;
  Visual  *visual = CopyFromParent;
  int screen = XDefaultScreen(dpy);
  
  hints.flags = USPosition | PPosition | PSize;
  hints.x = x;
  hints.y = y;
  hints.width = width;
  hints.height = height;
  
  *pwin = XCreateSimpleWindow(dpy, XRootWindow(dpy, screen),
			      x, y, width, height, 1,
			      BlackPixel(dpy, screen), 
			      WhitePixel(dpy, screen));
  XSetWMSizeHints(dpy, *pwin, &hints, XA_WM_NORMAL_HINTS);
  
  attributes.override_redirect = True;
  *pgc = XCreateGC(dpy, *pwin, xgc_valuemask, &xgc_values);
  XMapWindow(dpy, *pwin);
}

/*---------------------------------------------------------*/
/************************************************************/
fill_array(ptr, len, pattern)
     register int *ptr;
     register len;
     register pattern;
/************************************************************/
{
  do{
    *ptr++ = pattern;
  }while(len--);
}
/*---------------------------------------------------------*/

/* sun_px_video_encode() -- the video capture, coding and tranmission 
 * process.
 */
/************************************************************/
void sun_px_video_encode(param)
     S_PARAM *param;       /* Video characteristics */

/************************************************************/
{
  static IMAGE_CIF image_coeff[4];
  static u_char *new_Y;
  static u_char *new_Cb;
  static u_char *new_Cr;
  
  int NG_MAX;
  BOOLEAN FIRST = TRUE;
  BOOLEAN CONTINUE = TRUE;
  char win_name[32];
  
  Window child;
  Window parent;
  static  int dst_x = 16;
  static  int dst_y = 16;
  
  int channel;
  int standard;
  int format;
  int ntsc_signal;
  int io_format;
  
#define  X_EVENT_MASK  \
  VisibilityChangeMask | ExposureMask | StructureNotifyMask | SubstructureNotifyMask
  
  u_int *px_ptr;
  int prot, flags;
  off_t off;
  size_t len;
  /*-----------------------------*/
  PXport px_vport;
  /*-----------------------------*/
  static int cif_size=101376;
  static int qcif_size=25344; /* 176*144 */
  void (*get_image)();
  
  int new_size, new_px_port, nb_cif;
  BOOLEAN LOCAL_DISPLAY = param->LOCAL_DISPLAY;
  BOOLEAN TUNING=FALSE;

  nb_cif = (param->video_size == CIF4_SIZE ? 4 : 1);
  
  if((new_Y = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX*COL_MAX))
     == NULL){
    perror("px_encode'malloc");
    exit(1);
  }
  if((new_Cb = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("px_encode'malloc");
    exit(1);
  }
  if((new_Cr = (u_char *) malloc(sizeof(char)*nb_cif*LIG_MAX_DIV2*
				  COL_MAX_DIV2)) == NULL){
    perror("px_encode'malloc");
    exit(1);
  }

#ifndef IMAGE_FROM_FILE
  
  /* Open video device */
#ifdef WAIT_DEBUG
  while(!wait_debug);
#endif

  if(OpenHardwareParallax(&px_vport)<0){
    perror("can't open Parallax hardware");
    exit(-1);
  }
  
  channel = param->px_port_video.channel;
  standard = video_std_map[param->px_port_video.standard];
  
  select_input_channel_px(&channel, &standard, &format,
			  video_hint_map[param->px_port_video.standard]);
#endif
  
#define CIF_NG_MAX  12
#define QCIF_NG_MAX  5
  
  if(param->video_size == QCIF_SIZE){
    NG_MAX = QCIF_NG_MAX;
    lig = QCIF_HEIGHT;
    col = QCIF_WIDTH;
  }else{
    NG_MAX = CIF_NG_MAX;
    lig = CIF_HEIGHT;
    col = CIF_WIDTH;
  }
  ntsc_signal = standard == PLX_NTSC;
  Xwin = &grab_window[ntsc_signal][param->px_port_video.squeeze?
				   param->video_size:0];
  
  get_image = fun_tab[(!param->LOCAL_DISPLAY)+2*ntsc_signal+
		4*param->px_port_video.squeeze][param->COLOR][param->video_size];
  
  /*
   * opens local display
   */
  
#ifndef IMAGE_FROM_FILE
  
  /*dpy = XOpenDisplay(0);*/
  if (!dpy) {
    perror("Couldn't open display\n");
    exit(-1);
  }
  screen = XDefaultScreen(dpy);
/*  MakeSWindow(dpy, &gc, &win, Xwin->col, Xwin->lig,
	      PX_FB_WIDTH - Xwin->col - 8, DST_Y_MIN);*/
  
  {
    char *ptr;
    
    io_format = 0;
    strcpy(win_name, channel==1?"Input 1  ":"Input 2  ");
    if(format == PLX_COMP || format == PLX_YC){
      switch(standard){
      case PLX_NTSC:
	ptr = " NTSC ";
	break;
      case PLX_PAL:
	ptr = " PAL ";
	io_format = 1;
	break;
      case PLX_SECAM:
	ptr = " SECAM ";
	io_format = 2;
	break;
      default:
	ptr = " ???? ";
      }
      strcat(win_name, ptr);
      strcat(win_name, format == PLX_COMP ? "COMPOSITE":"YC");
    }else{
      switch(format){
      case PLX_RGB:
	ptr = "RGB";
	break;
      case PLX_YUV:
	ptr = "YUV";
	break;
      default:
	ptr = "???";
      }
      strcat(win_name, ptr);
    }
    if(param->px_port_video.mirror_x ||
       param->px_port_video.mirror_x){
      strcat(win_name, "  MIRROR ");
      if(param->px_port_video.mirror_x)
	strcat(win_name, "X");
      if(param->px_port_video.mirror_y)
	strcat(win_name, "Y");
    }
    /*XStoreName(dpy, win, win_name);*/
  }
  do{
    MakeSWindow(dpy, &gc, &win, Xwin->col, Xwin->lig,
		PX_FB_WIDTH - Xwin->col - 8, DST_Y_MIN);
    XStoreName(dpy, win, win_name);
    XSync(dpy, False);
  
  /*    DO NOT REMOVE THIS BLOCK !!! */  

  {
    Window root;
    Window *children;
    int nchildren;
    
    do{
      XQueryTree(dpy, win, &root, &parent, &children, &nchildren);
      XFree((void *)children);
    }while(root==parent);
  }
  
  
  XPlxVideoTag(dpy, win, gc, PLX_VIDEO);
  XSelectInput(dpy, win, X_EVENT_MASK);
  
  XPlxVideoInputSelect(dpy, win, gc,
		       channel, standard, format,  PLX_RGB24);
  
  px_vport.dpy = dpy;
  px_vport.win = win;
  px_vport.gc  = gc;
  px_vport.color = param->COLOR;
  px_vport.col = Xwin->col;
  px_vport.lig = Xwin->lig;

  XSync(dpy, False);
  if(align_window(&px_vport, &dst_x, &dst_y)){
    XDestroyWindow(dpy, win);
    XSync(dpy, False);
    fprintf(stderr, ".\n");
    sleep(2);
  }else
    break;
  }while(1);


  
#ifdef CAPTURE_VIDEO
#ifdef LASER_DISK
  init_laser_disk();
  
#endif
  open_grab_file(param->size, param->COLOR);
#endif
  
  
  
  /*align_window(&px_vport, &dst_x, &dst_y);*/
  
  if(aw_select_v(channel, format)){
    fprintf(stderr, "\nunknown channel/format/combination\n");
    exit(-1);
  }
  
  init_RGB_2_YCrCb();
  fill_array(new_Y, sizeof(new_Y)/sizeof(int),   0x80808080);
  fill_array(new_Cr, sizeof(new_Cr)/sizeof(int), 0x80808080); 
  fill_array(new_Cb, sizeof(new_Cb)/sizeof(int), 0x80808080); 
  
  if (format == PLX_COMP || format == PLX_YC){
    if(!LOCAL_DISPLAY)
      set_parallax_YUV411(channel);
    else
      set_parallax_RGB24(channel);
    usleep(100000);
  }
  aw_v_squeeze(dst_x, dst_y, Xwin,
	       param->px_port_video.mirror_x,
	       param->px_port_video.mirror_y,
	       Xwin->noninterlace, SQZE_SINGLE); 
  aw_next_filed();
  
#else
  open_image_file(param->size, param->COLOR);
#endif
  
#define NTSC_FIELD_TIME        33333
#define PAL_SECAM_FIELD_TIME   (40000)
#define NTSC_TURN_TIME (NTSC_FIELD_TIME)
#define PAL_SECAM_TURN_TIME (PAL_SECAM_FIELD_TIME)
#define FREEZE_FRAME_RATE 10
#ifndef JITTER
#define JITTER 20000
#endif
#ifdef EXT_FRAME_RATE
#define MAX_FRAME_RATE (EXT_FRAME_RATE)
#else
#define MAX_FRAME_RATE 1
#endif
  
  do{
    int i;

#ifndef IMAGE_FROM_FILE
    if(!param->FREEZE){
      XGrabServer(dpy);
      XSync(dpy, False);
      aw_wait_for_done_px();
      (*get_image)(&px_vport, new_Y, new_Cr, new_Cb);
      
      
#ifdef LASER_DISK
      next_player_frame();
#endif
      aw_next_filed();
      XUngrabServer(dpy);
      XSync(dpy, False);
    }
#else
    image_from_file(param->size, param->COLOR, new_Y, new_Cb, new_Cr);
#endif
    
#ifdef CAPTURE_VIDEO
    grab_into_file(param->size, param->COLOR, new_Y, new_Cb, new_Cr);
#else
    
    if(param->video_size == CIF4_SIZE){
      for(i=0; i<4; i++)
	encode_h261_cif4(param, (new_Y+i*cif_size), (new_Cb+i*qcif_size),
			 (new_Cr+i*qcif_size),
			 (GOB *)image_coeff[i], i);
    }else{
      encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff[0],
		  NG_MAX);
    }
    
#endif
    FIRST = FALSE;
    if(CONTINUE = HandleCommands(param, &TUNING)){
      
#ifdef IMAGE_FROM_FILE
      continue;
#endif
      if(param->LOCAL_DISPLAY != LOCAL_DISPLAY
	 || XCheckWindowEvent(dpy, win, X_EVENT_MASK, &report) == True){
	int x, y, w, h, b, d;
	Window root;
	
	LOCAL_DISPLAY = param->LOCAL_DISPLAY;
	
	align_window(&px_vport, &dst_x, &dst_y);
	
	if (format == PLX_COMP || format == PLX_YC){
	  if(!LOCAL_DISPLAY)
	    set_parallax_YUV411(channel);
	  else
	    set_parallax_RGB24(channel);
	  XSync(dpy, True);
	  aw_next_filed();
	  usleep(100000);
	  XSync(dpy, True);
	}
	get_image = fun_tab[(!LOCAL_DISPLAY)+2*ntsc_signal+
		4*param->px_port_video.squeeze][param->COLOR][param->video_size];
	
	aw_v_squeeze(dst_x, dst_y, Xwin,
		     param->px_port_video.mirror_x,
		     param->px_port_video.mirror_y,
		     Xwin->noninterlace, SQZE_SINGLE); 
      }
    }
  }while(CONTINUE);
  
  /* Clean up and exit */
  
  close(param->f2s_fd[0]);
  close(param->s2f_fd[1]);
  /*XCloseDisplay(dpy);*/
  free(new_Y);
  free(new_Cb);
  free(new_Cr);
  exit(0);
  /* NOTREACHED */
}
/*---------------------------------------------------------------------------*/


#endif /* SUN_PX_VIDEO */




