/**************************************************************************\
*          Copyright (c) 1992 INRIA Sophia Antipolis, FRANCE.              *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
\**************************************************************************/
/**************************************************************************\
*  File    : display.c                                                     *
*  Date    : August 1992      						   *
*  Author  : Thierry Turletti					           *
*--------------------------------------------------------------------------*
*  Description : Initialization of the display window (CIF or QCIF format).*
*									   *
\**************************************************************************/

#include "h261.h"
#include <sys/socket.h>
#include <sys/time.h>

#define ecreteL(x) ((x)>255 ? 255 : (x))
#define ecreteL4(x) ((x)>15 ? 15 : (int)(x))


int socket_is_local(s)
     int s;
{
  struct sockaddr name;
  int len = sizeof(name);
 
  if(getsockname(s, &name, &len) == -1){
    perror("getsockname");
    exit(1);
  }
  return (len == 0 ? 1 : 0);
}


Visual *get_visual()
{
  extern Display *dpy;
  int screen=0;

  screen=DefaultScreen(dpy);
  return(DefaultVisual(dpy, screen));
}


char *sure_malloc(size, msg)
     unsigned size;
     char *msg;
{
  char *pt;

  if((pt = (char *)malloc(size)) == NULL){
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }else
    return(pt);
}



#ifdef MITSHM

XImage *alloc_shared_ximage(w, h, depth)
     int w, h, depth;
{
  extern Bool does_shm;
  extern Display *dpy;
  XImage *ximage;
  XShmSegmentInfo *shminfo;
  u_char *data;

  if(does_shm){
    shminfo =
      (XShmSegmentInfo *) sure_malloc(sizeof(XShmSegmentInfo),
				      "Can't malloc XShmSegmentInfo\n");
    ximage = XShmCreateImage(dpy, 0, depth, ZPixmap, 0, shminfo, w, h);
    shminfo->shmid = shmget(IPC_PRIVATE,w*h,IPC_CREAT|0777);
    if(shminfo->shmid < 0){
      perror("shmget");
      exit(1);
    } 
    shminfo->shmaddr = (char *)shmat(shminfo->shmid, 0, 0);
    if(shminfo->shmaddr == ((char *)-1)){
      perror("shmat");
      exit(1);
    }
    ximage->obdata = (char *)shminfo;
    ximage->data = shminfo->shmaddr;
    shminfo->readOnly = False;
    XShmAttach(dpy, shminfo);
    XSync(dpy,False);
    if(shmctl(shminfo->shmid, IPC_RMID, NULL) == -1){
      perror("shmctl(shminfo->shmid, IPC_RMID, NULL)");
      exit(1);
    }
  }
  else {
    data = (u_char *) sure_malloc(w*h, "Can't malloc visual image\n");
    ximage = XCreateImage(dpy, get_visual(), depth, ZPixmap, 0, data, w, h, 
			  8, (depth==4 ? w/2 : w));
  }

  return(ximage);
}


void destroy_shared_ximage(ximage)
     XImage *ximage;
{
  extern Bool does_shm;
  extern Display *dpy;

  if(does_shm){
    XShmDetach(dpy, ximage->obdata);
    shmdt(ximage->data);
    ximage->data = NULL;
    free(ximage->obdata);
    ximage->obdata = NULL;
  }else{
    free(ximage->data);
    ximage->data = NULL;
  }

  /* Don't forget to nullify
   * ximage->data
   * ximage->obdata
   * before XDestroyImage
   */
  XDestroyImage(ximage);
}

#endif /* MITSHM */




init_window(L_lig, L_col, name_window, map_lut, display)
int L_lig, L_col;
char *name_window;
u_char map_lut[];
char *display;

  {
    extern Display *dpy;
    extern GC gc;
    extern Window win;
    extern XImage *ximage;
    extern XColor colors[256];
    extern Bool does_shm;
    extern int depth;

    Window root = 0;
    int screen = 0;
    unsigned long noir, blanc;
    Colormap colmap = 0;
    Visual *visu;
    XSetWindowAttributes xswa;
    Screen *SCREEN;
    u_char *data;
    register int k;

    if (!(dpy=XOpenDisplay(display))){
      fprintf(stderr, "init_window : cannot open display %s\n", display);
      exit(1);
    }
    screen = DefaultScreen(dpy);
    root = RootWindow (dpy, screen);
    noir = BlackPixel(dpy, screen);
    blanc = WhitePixel(dpy, screen);
    gc = XCreateGC (dpy, root, 0, NULL);
    XSetBackground(dpy, gc, blanc);
    XSetForeground(dpy, gc, noir);
    depth = DefaultDepth(dpy,screen); 

    if(depth < 4){
      fprintf(stderr,
    "Sorry, %s is a %d-bit depth screen, only 4/8/24-bit depth is required\n", 
	      display, depth);
      exit(1);
    }

    if(socket_is_local(ConnectionNumber(dpy))){
#ifdef MITSHM
      int *majorv, *minorv;
      Bool *sharedPixmaps;

      does_shm = XShmQueryVersion(dpy, &majorv, &minorv, &sharedPixmaps);
#else
      does_shm = FALSE;
#endif /* MITSHM */
    }else{
      does_shm = FALSE;
    }
    if(does_shm)
      printf("Local videoconf, using Shared Memory\n");

    if(depth == 4){

      colmap = XCreateColormap(dpy, root, DefaultVisual(dpy, screen), 
			       AllocNone);
      for(k=0; k<16; k++){
	colors[k].red = colors[k].green = colors[k].blue =(u_short)0;
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
      for(k=16; k<232; k++){
	colors[k].red = colors[k].green = colors[k].blue =
	  (u_short)(4680*(k/16));
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
      for(k=232; k<256; k++){
	colors[k].red = colors[k].green = colors[k].blue =(u_short)65535;
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
    }else{

      SCREEN = XScreenOfDisplay(dpy, screen);
      colmap = DefaultColormapOfScreen(SCREEN);
      for(k=0; k<16; k++){
	colors[k].red = colors[k].green=colors[k].blue=(u_short)0;
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
      for(k=16; k<232; k++){
	colors[k].red = colors[k].green = colors[k].blue =
	    (u_short)(1213*((k-16)/4));
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
      for(k=232; k<256; k++){
	colors[k].red = colors[k].green=colors[k].blue = (u_short)65535;
	colors[k].flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, colmap, &colors[k]);
      }
    }
    for(k=0; k<256; k++)
      map_lut[k]=(u_char) colors[ecreteL((int)((255*k)/150))].pixel;

    /* Get window attributes */
    xswa.colormap = colmap;

    win = XCreateWindow (dpy, DefaultRootWindow(dpy),
                       0, 0, L_col, L_lig, 0, depth, CopyFromParent,
                       CopyFromParent, CWColormap|CWEventMask , &xswa );
    XStoreName(dpy, win, name_window);
    XSetWindowBackground(dpy, win, blanc);
    XSetWindowColormap(dpy, win, colmap);
    XMapWindow(dpy, win); 
#ifdef MITSHM
    ximage = alloc_shared_ximage(L_col, L_lig, depth);
#else
    XSync(dpy, True);
    data = (u_char *) sure_malloc(L_col*L_lig, "Can't malloc visual image\n");
    ximage = XCreateImage(dpy, get_visual(), depth, ZPixmap, 0, data,
			  L_col, L_lig, 8, (depth==4 ? L_col/2 : L_col));
#endif /* MITSHM */
    ximage->bits_per_pixel = depth;

  }



void show (L_col, L_lig)
     int L_col, L_lig;

{
  extern Display *dpy;
  extern GC gc;
  extern Window win;
  extern XImage *ximage;
  extern Bool does_shm;

#ifdef MITSHM
  if(does_shm){
    XShmPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, L_col, L_lig, FALSE);
    XSync(dpy, FALSE);
  }else{
    XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, L_col, L_lig);
    XFlush(dpy);
  }
#else
  XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, L_col, L_lig);
  XFlush(dpy);
#endif /* MITSHM */
  return;
}




    






