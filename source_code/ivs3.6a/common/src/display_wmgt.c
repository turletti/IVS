/**************************************************************************\
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
*  File              : display_wmgt.c                                      *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description : Initialization of the display: colormap, window.          *
*									   *
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*   Pierre Delamotte      |  3/20/93  |  Colormap managing added.          *
* delamot@wagner.inria.fr |           | (all building colormap functions)  *
*--------------------------------------------------------------------------*
*   Pierre Delamotte      |  5/4/93   |  Screen depth managment added :	   *
* delamot@wagner.inria.fr |           |    - 1 bit, 4 bits, 8 bits, 24 bits*
*                         |           |  Color dither added :              *
*                         |           |    - RGB dither in 8 bits depth,   *
*                         |           |    - grey dither in 4 bits depth,  *
*                         |           |    - B&W dither in 1 bit depth,    *
*                         |           |    - B&W dither forced in 4 bits   *
*                         |           |      and 8 bits depth when default *
*                         |           |      colormap is full.             *
*                         |           |   The dither algorithm used come   *
*                         |           |   from the MPEG package distributed*
*                         |           |   by the University of California, *
*                         |           |   that is, a blend of the Floyd-   *
*                         |           |   Steinberg and of the ordered     *
*                         |           |   dither algorithms where a 4x4    *
*                         |           |   FS matrix is used to bound the   *
*                         |           |   range of the ordered dither to a *
*                         |           |   neighbourhood of 16 pixels.      *
*--------------------------------------------------------------------------*
*   Pierre Delamotte      |  6/15/93  | Dynamic zoom functions added,      *
* delamot@wagner.inria.fr |           | triggered by buttons in display    *
*                         |           | control panel.                     *
*                         |           |                                    *
*--------------------------------------------------------------------------*
*   Andrzej Wozniak       |  93/12/01 | macrogeneration and sppedup,       *
* wozniak@inria.fr        |           | display.c divided into 3 sources   *
*                         |           | display_wmgt.c, display_gentab.c,  *
*                         |           | and display_imbld.c                *
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#ifdef __NetBSD__
#include <signal.h>
#include <errno.h>
#endif
#include <math.h>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h> /* pas vraiment autorise mais pratique pour
                             la structure core */
#include <X11/Shell.h>

#ifdef MITSHM
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* MITSHM */

#include <sys/socket.h>
#include <netdb.h>
#ifdef TELESIA
#include "WcCreate.h"
#endif

#include "general_types.h"
#include "protocol.h"
#include "general_types.h"
#include "display.h"

#ifdef TELESIA
#define ASSOCIATE( Cident, Xname, AShell ) \
 ((Widget)NULL ==( Cident = WcFullNameToWidget(AShell, Xname) ) \
 ? ( fprintf(stderr,"Missing ressource name: %s \n" , Xname ) , 0 ): 1 )
#endif

int wait_debug;
/*--------------------------------------------*/
/*
**		IMPORTED VARIABLES
*/
#ifdef TELESIA
/*
** from motif_stuff.c
*/
extern void	brightnessCB() ;
extern void	contrastCB() ;
#else
/*----- from athena.c ----------*/
extern void Valid(), Unvalid();
extern Widget colorButton, greyButton ;
extern Widget doubleButton, regularButton, halfButton ;
#endif

/*
**	GLOBAL VARIABLES
*/
/* tables de dithering en couleur 8 bits */
#ifdef TELESIA
static   Widget	TVControle, coulAff, coulButton, greyButton, doubleButton, regularButton, halfButton ;
#endif

/*--------------------------------------------*/

/*
**	EXPORTED VARIABLES
*/

BOOLEAN		CONTEXTEXIST = FALSE;
BOOLEAN		encCOLOR;
XImage		*ximage ;
u_char   	noir, blanc;


/*---------- variables ----------------------------------*/


/*--------------------------------------------*/

XtCallbackProc affColorCB (w, closure, call_data)
     Widget w ;
     XtPointer closure, call_data ;
{
  if(affCOLOR != MODCOLOR){
#ifndef TELESIA
    Valid(colorButton);
    Unvalid(greyButton);
#endif
    affCOLOR = MODCOLOR ;
  } 
} /* end affColorCB */
/*--------------------------------------------*/

XtCallbackProc affGreyCB (w, closure, call_data)
     Widget w ;
     XtPointer closure, call_data ;
{
  if(affCOLOR != MODGREY){
#ifndef TELESIA
    Valid(greyButton);
    Unvalid(colorButton);
#endif
    affCOLOR = MODGREY ;
  }
} /* end affColorCB */

/*--------------------------------------------*/

XtCallbackProc doubleCB (w, closure, call_data)
     Widget w ;
     XtPointer closure, call_data ;
{
  int width, height ;
  if (locDOUBLE) return ;
  locDOUBLE = TRUE ;
#ifndef TELESIA
  Valid(doubleButton);
#endif

  if (!locHALF) {
#ifndef TELESIA
    Unvalid(regularButton);
#endif
    width = (ximage->width) << 1 ;
    height = (ximage->height) << 1 ;
  } else {
#ifndef TELESIA
    Unvalid(halfButton);
#endif
    locHALF = FALSE ;
    width = (ximage->width) << 2 ;
    height = (ximage->height) << 2 ;
  }
    
  destroy_shared_ximage (dpy_disp, ximage) ;
  ximage = (XImage *)alloc_shared_ximage (dpy_disp, width, height,
					  depth_disp) ;
  XtResizeWidget(appShell, width, height, 0) ;
} /* end doubleCB */

/*--------------------------------------------*/

XtCallbackProc regularCB (w, closure, call_data)
     Widget w ;
     XtPointer closure, call_data ;
{
  int width, height ;

  if (!locDOUBLE && !locHALF) return ;
#ifndef TELESIA
  Valid(regularButton);
#endif

  if (locDOUBLE) {
#ifndef TELESIA
    Unvalid(doubleButton);
#endif 
    locDOUBLE = FALSE ;
    width = (ximage->width) >> 1 ;
    height = (ximage->height) >> 1 ;
  } else {
#ifndef TELESIA
    Unvalid(halfButton);
#endif /* TELESIA */
    locHALF = FALSE ;
    width = (ximage->width) << 1 ;
    height = (ximage->height) << 1 ;
  }

  destroy_shared_ximage (dpy_disp, ximage) ;
  ximage = (XImage *)alloc_shared_ximage (dpy_disp, width, height,
					  depth_disp) ;
  XtResizeWidget(appShell, width, height, 0) ;
} /* end regularCB */

/*--------------------------------------------*/

XtCallbackProc halfCB (w, closure, call_data)
     Widget w ;
     XtPointer closure, call_data ;
{
  int width, height ;

  if (locHALF) return ;
#ifndef TELESIA
  Valid(halfButton);
#endif /* TELESIA */
  locHALF = TRUE ;

  if (locDOUBLE) {
#ifndef TELESIA
    Unvalid(doubleButton);
#endif /* TELESIA */
    locDOUBLE = FALSE ;
    width = (ximage->width) >> 2 ;
    height = (ximage->height) >> 2 ;
  } else {
#ifndef TELESIA
    Unvalid(regularButton);
#endif /* TELESIA */
    width = (ximage->width) >> 1 ;
    height = (ximage->height) >> 1 ;
  }
  destroy_shared_ximage (dpy_disp, ximage) ;
  ximage = (XImage *)alloc_shared_ximage (dpy_disp, width, height,
					  depth_disp) ;
  XtResizeWidget(appShell, width, height, 0) ;

} /* end halfCB */

/*--------------------------------------------*/

#ifdef MITSHM
int socket_is_local(s)
     int s;
{
/*************************************/
/* C Andrzej.Wozniak@inria.fr        */
/* thanks to Robert Ehrlich for help */
/*************************************/
#define HN_LEN 128

  struct sockaddr name;
  int len = sizeof(name);

  if(getsockname(s, &name, &len) == -1){
    perror("getsockname");
    exit(1);
  }

  if(!len || (name.sa_family == AF_UNIX)){
    return 1;
  }
  if(name.sa_family == AF_INET){

    struct sockaddr_in * p_sock_in = (struct sockaddr_in *)&name;
    struct hostent *hp;
    char **ap;
    char hostname[HN_LEN];

    len = sizeof(name);
    if( getpeername(s, &name, &len) == -1){
      perror("getpeername");
      exit(1);
    }
#ifndef DECSTATION
    if(p_sock_in->sin_addr.s_addr == INADDR_LOOPBACK){
      return 1;
    }
#endif
    if(gethostname(hostname, HN_LEN)==-1){
      return 0;
    }
    hp = gethostbyname(hostname);
    for(ap=hp->h_addr_list; *ap; ++ap){
      if(p_sock_in->sin_addr.s_addr == *(u_int *)(*ap)){
	return 1;
      }
    }
  }
  return 0;
}
#endif /* MITSHM */

/*--------------------------------------------*/

Visual *get_visual(dpy)
     Display *dpy;
{
  int screen=0;

  screen=DefaultScreen(dpy);
  return(DefaultVisual(dpy, screen));
}

/*--------------------------------------------*/

char *sure_malloc(size, msg)
     unsigned size;
     char *msg;
{
  char *pt;

  if((pt = (char *)malloc((size_t)size)) == NULL){
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }else
    return(pt);
}

#ifdef __NetBSD__
/* Need signal handler for SIGSYS under NetBSD 1.0 */
void catchSIGSYS()
{
  does_shm = FALSE;
  signal(SIGSYS, catchSIGSYS);
}
#endif

/*--------------------------------------------*/

XImage *alloc_shared_ximage(dpy, w, h, depth)
     Display *dpy;
     int w, h, depth;
{
  XImage *ximage;
  u_char *data;
  Visual *im_visu;
  int oct_w_sz, oct_h_sz, format, im_pad;

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-alloc_shared_ximage() : entree\n");
  fflush(stderr);
#endif

  if (affCOLOR & MODNB) depth = 1;
  switch (depth){
    case 1 : 
/*
      oct_w_sz = (w + 7)/8;
      oct_h_sz = (h + 7)/8;
*/
      locDOUBLE = TRUE ;
      XtSetSensitive(doubleButton, FALSE) ;
      XtSetSensitive(regularButton, FALSE) ;
      XtSetSensitive(halfButton, FALSE) ;
      oct_w_sz = w;
      oct_h_sz = h;
      im_pad = 8;
      im_visu = NULL;
      format = XYBitmap;
      break;
    case 4 :
    case 8 :
      oct_w_sz = w;
      oct_h_sz = h;
      format = ZPixmap;
      im_pad = 8;
      im_visu = NULL;
      break;

    case 24 :
    case 32:
/*
**  24 bits depth : one pixel per int
*/
      oct_w_sz = w * 4;
      oct_h_sz = h;
      format = ZPixmap;
      im_pad = 32;
      im_visu = get_visual(dpy);
      break;

    default :
      fprintf (stderr, 
       "DISPLAY.C-alloc_shared_ximage() : screen depth not yet supported\n");
      exit(1);
  } /* hctiws depth */

    
#ifdef MITSHM
#ifdef __NetBSD__
  /* NetBSD 1.0 delivers a SIGSYS if SHM not enabled in the kernel */
  signal(SIGSYS, SIG_IGN);
#endif
  if(does_shm){
    XShmSegmentInfo *shminfo;

    shminfo = (XShmSegmentInfo *) sure_malloc(sizeof(XShmSegmentInfo),
				    "Can't malloc XShmSegmentInfo\n");
    ximage = XShmCreateImage(dpy, im_visu, depth, format, NULL, shminfo, w, h);
    shminfo->shmid = shmget(IPC_PRIVATE,oct_w_sz*oct_h_sz,IPC_CREAT | 0777);
    if(shminfo->shmid < 0){
#ifdef __NetBSD__
      /* If no shared memory in the kernel, will catch SIGSYS and get EINVAL */
      if (errno != EINVAL)
#endif
      perror("shmget");
      does_shm = FALSE;
      XDestroyImage(ximage);
    }else{
      shminfo->shmaddr = (char *)shmat(shminfo->shmid, 0, 0);
      if(shminfo->shmaddr == ((char *)-1)){
        perror("shmat");
        does_shm = FALSE;
        XDestroyImage(ximage);
      }else{
        ximage->obdata = (char *)shminfo;
        ximage->data = shminfo->shmaddr;
        shminfo->readOnly = False;
        XShmAttach(dpy, shminfo);
        XSync(dpy,False);
        if(shmctl(shminfo->shmid, IPC_RMID, NULL) == -1){
          perror("shmctl(shminfo->shmid, IPC_RMID, NULL)");
          XShmDetach(dpy, shminfo);
          ximage->data = NULL;
          ximage->obdata = NULL;
          XDestroyImage(ximage);
          does_shm = FALSE;
        } else {
#ifdef WIN_DEBUG
	  fprintf(stderr, "DISPLAY.C-alloc_shared_ximage() : sortie -
shared memory\n");
	  fflush(stderr);
#endif
          return(ximage);
	}
      } /* fi shminfo->shmaddr == */
    } /* fi shminfo->shmid */
  } /* fi does_shm */
#endif /* MITSHM */

  data = (u_char *) sure_malloc(oct_w_sz*oct_h_sz,
				"Can't malloc visual image\n");
  ximage = XCreateImage(dpy, im_visu, 
			depth, format, 0, data, w, h, im_pad, 0);

  ximage->byte_order = ImageByteOrder(dpy);

  ximage->bitmap_bit_order = BitmapBitOrder(dpy) ;
  
#ifdef WIN_DEBUG
	  fprintf(stderr, "DISPLAY.C-alloc_shared_ximage() : sortie -
no shared memory\n");
	  fflush(stderr);
#endif
  return(ximage);
}

/*--------------------------------------------*/

void destroy_shared_ximage(dpy, ximage)
     XImage *ximage;
     Display *dpy;
{

#ifdef MITSHM
  if(does_shm){
    XShmDetach(dpy, ( XShmSegmentInfo *) ximage->obdata);
    shmdt(ximage->data);
    ximage->data = NULL;
    free(ximage->obdata);
    ximage->obdata = NULL;
    XDestroyImage(ximage);
    return;
  } /* fi does_shm */
#endif /* MITSHM */
  free(ximage->data);
  ximage->data = NULL;

  /* Don't forget to nullify
   * ximage->data
   * ximage->obdata
   * before XDestroyImage
   */
  XDestroyImage(ximage);
}

/*--------------------------------------------*/

void kill_window(dpy, ximage)
     Display *dpy;
     XImage *ximage;
{
  destroy_shared_ximage(dpy, ximage);
  XCloseDisplay(dpy);
}

/*--------------------------------------------*/

void display_panel (w, unused, evt)
     Widget		w;
     XtPointer	unused;
     XEvent		*evt;
{
  static BOOLEAN NOSCALEPANEL = TRUE;
  if(NOSCALEPANEL){
    XtPopup(panel, XtGrabNone);
    NOSCALEPANEL = FALSE;
  }
  else{
    XtPopdown(panel);
    NOSCALEPANEL = TRUE;
  }
}
/*--------------------------------------------*/

void init_window(L_lig, L_col, name_window, display, pt_appCtxt, pt_appShell,
		 pt_depth, pt_dpy, pt_win, pt_icon, pt_gc, pt_ximage,
		 FORCE_GREY)
     int		L_lig, L_col;
     char 		*name_window;
     char 		*display;
     XtAppContext	*pt_appCtxt;
     Widget		*pt_appShell;
     int 		*pt_depth;
     Display 	        **pt_dpy;
     Window 		*pt_win;
     Window		*pt_icon;
     GC		        *pt_gc;
     XImage 		**pt_ximage;
     BOOLEAN            FORCE_GREY;

{

  Window 		root = 0;
  Window 		icon_win ;
  XSetWindowAttributes  xswa ;
  XWMHints 		xwmh ;
  Arg 			args[10] ;

  int 			screen = 0;
  Visual 		*visu;
  Screen 		*SCREEN;
  u_char 		*data;
  register int 		k;
  int			largc = 1;
  char                  control_name[200];
#ifdef TELESIA
  Widget		TVScreen ;
#endif /* TELESIA */

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : enter\n");
  fflush(stderr);
#endif

  if(CONTEXTEXIST) 
    return;

  if(!encCOLOR || FORCE_GREY) 
    affCOLOR = MODGREY;

#ifdef TELESIA
  WcAllowDuplicateRegistration(TRUE) ;
#endif /* TELESIA */
  
  *pt_appCtxt =	XtCreateApplicationContext();
  CONTEXTEXIST = TRUE;

#ifdef TELESIA

  if ((dpy_disp = XtOpenDisplay(*pt_appCtxt,  display, "tvihm", "TvIHM",
				NULL, 0, &largc, &name_window)) == NULL)
#else
  if ((dpy_disp = XtOpenDisplay(*pt_appCtxt,  display, "IVS", "IVS",
				NULL, 0, &largc, &name_window)) == NULL)
#endif /* TELESIA */
    {
      fprintf(stderr, "DISPLAY.C-init_window() : XtOpenDisplay failed\n");
      exit(1);
    }

#ifdef MITSHM
  if(socket_is_local(ConnectionNumber(dpy_disp))){
    int majorv, minorv;
    Bool sharedPixmaps;

    does_shm = XShmQueryVersion(dpy_disp, &majorv, &minorv, &sharedPixmaps);
#ifdef DEBUG
    if(!does_shm)
      fprintf(stderr, "Shared image memory unavailable.\n");
#endif
  }else
    does_shm = FALSE;
#endif /* MITSHM */

  /*
   *  Creation de la fenetre de l'appli : renvoie le WINDOWID de l'appli
   */

  largc = 0;
  XtSetArg( args[largc], XtNwidth, L_col ); largc++;
  XtSetArg( args[largc], XtNheight, L_lig ); largc++;
  XtSetArg( args[largc], XtNtitle, name_window); largc++;

#ifdef TELESIA
  appShell = *pt_appShell = XtAppCreateShell(/*name_window*/"tvihm", "TvIHM", 
					     applicationShellWidgetClass,
					     dpy_disp, args, largc);
#else
  appShell = *pt_appShell = XtAppCreateShell(name_window, "IVS", 
					     topLevelShellWidgetClass,
					     dpy_disp, args, largc);
/*
**  Management of display control panel
*/

  XtAddEventHandler(*pt_appShell, ButtonPressMask, FALSE,
		    (XtEventHandler)display_panel, NULL);
#endif /* TELESIA */

  appShell->core.widget_class->core_class.compress_exposure =
    XtExposeCompressMaximal;

  /*
   *  Create associated iconic window
   */
  xswa.background_pixel  = WhitePixel(dpy_disp, DefaultScreen(dpy_disp)) ;
  xswa.border_pixel  	 = BlackPixel(dpy_disp, DefaultScreen(dpy_disp)) ;
  xswa.bit_gravity 	 = NorthWestGravity ;
  xswa.win_gravity	 = NorthWestGravity ;
  xswa.backing_store 	 = Always ;
  xswa.event_mask 	 = EnterWindowMask | LeaveWindowMask | ExposureMask |
    VisibilityChangeMask | SubstructureNotifyMask | StructureNotifyMask |
      SubstructureRedirectMask | PropertyChangeMask ;

  icon_win = XCreateWindow (dpy_disp, DefaultRootWindow(dpy_disp),
			    1, 1, 64, 64, 2,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWBorderPixel | CWBitGravity | CWWinGravity |
			    CWBackPixel | CWEventMask | CWBackingStore,
			    &xswa) ;

#ifndef TELESIA
  XtSetMappedWhenManaged(*pt_appShell, FALSE);

  strcpy(control_name, "Video from ");
  strcat(control_name, name_window);
#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : call to CreatePanel\n");
  fflush(stderr);
#endif
  CreatePanel(control_name, &panel, *pt_appShell);
  XtSetMappedWhenManaged(panel, FALSE);
#else

 /* register call back procedures associated to the TV set */
  WcRegisterCallback( *pt_appCtxt, "affColorCB", affColorCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "affGreyCB", affGreyCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "doubleCB", doubleCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "regularCB", regularCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "halfCB", halfCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "CallbackContrast", contrastCB, NULL ) ;
  WcRegisterCallback( *pt_appCtxt, "CallbackBrightness", brightnessCB, NULL ) ;

  MriRegisterMotif( *pt_appCtxt );	  /*   register all Motif and public widget */

  WcWidgetCreation( appShell );  /* Create Widget tree below toplevel shell */

  if ( !ASSOCIATE( TVScreen  , "*ecran", appShell ) ) exit(1) ;
  if ( !ASSOCIATE( TVControle  , "*claviersh", appShell ) ) exit(1) ;
  if ( !ASSOCIATE( coulAff  , "*clavier.couleurRC", appShell ) ) exit(1) ;
  if ( !ASSOCIATE( coulButton  , "*clavier.couleurRC*color", appShell ) )
    exit(1) ;
  if ( !ASSOCIATE( greyButton  , "*clavier.couleurRC*greyLevel", appShell ) )
    exit(1) ;
  if ( !ASSOCIATE( doubleButton  , "*clavier.couleurRC*double", appShell ) )
    exit(1) ;
  if ( !ASSOCIATE( regularButton  , "*clavier.couleurRC*regular", appShell ) )
    exit(1) ;
  if ( !ASSOCIATE( halfButton  , "*clavier.couleurRC*half", appShell ) )
    exit(1) ;
/*
  if ( !ASSOCIATE( nbButton  , "*clavier.couleurRC*BandW", appShell ) ) exit(1) ;
*/

#endif /* TELESIA */

  XtResizeWidget(*pt_appShell, L_col, L_lig, 0);

  XtRealizeWidget(*pt_appShell);
#ifdef TELESIA

  if (!encCOLOR) {
    affCOLOR = MODGREY ;
    XtSetSensitive(coulButton, FALSE);
  }
#else
  Valid(regularButton);
  if(!encCOLOR ||affCOLOR != MODCOLOR) {
    XtSetSensitive(colorButton, FALSE);
    XtSetSensitive(greyButton, FALSE);
  }else{
    Valid(colorButton);
  }
#endif /* TELESIA */

  if (L_col > CIF_WIDTH) 
    XtSetSensitive(doubleButton, FALSE) ;


  screen = 	DefaultScreen(dpy_disp);
  depth_disp = 	DefaultDepth(dpy_disp, screen); 

#ifdef TELESIA
  win_disp = 	XtWindow(TVScreen);
#else
  win_disp =  	XtWindow( appShell );
#endif /* TELESIA */
  XSelectInput(dpy_disp, win_disp, ButtonPressMask);
  XStoreName(dpy_disp, win_disp, name_window);
  xwmh.flags = IconWindowHint ;
  xwmh.icon_window = icon_win ;
#ifdef TELESIA
  XSetWMHints (dpy_disp, XtWindow(appShell), &xwmh) ;
#else
  XSetWMHints (dpy_disp, win_disp, &xwmh) ;
#endif /* TELESIA */

  root = 	RootWindow(dpy_disp, screen);

  noir = 	(u_char) BlackPixel(dpy_disp, screen);
  blanc = 	(u_char) WhitePixel(dpy_disp, screen);

#ifdef TELESIA
  gc_disp = 		XtGetGC (TVScreen, 0, NULL) ;
#else
  gc_disp =    	XtGetGC(*pt_appShell, 0, NULL);
#endif /* TELESIA */

  XSetWindowBackground(dpy_disp, win_disp, blanc);

  switch (depth_disp){
    case 1 :
      XtSetSensitive(doubleButton, FALSE) ;
      XtSetSensitive(regularButton, FALSE) ;
      XtSetSensitive(halfButton, FALSE) ;
      locDOUBLE = TRUE ;
      XSetBackground(dpy_disp, gc_disp, noir);
      XSetForeground(dpy_disp, gc_disp, blanc);
      affCOLOR = MODNB;
      if (L_col <= CIF_WIDTH){
        L_col <<= 1;
        L_lig <<= 1;
        XtResizeWidget(*pt_appShell, L_col, L_lig, 0);
      } /* fi L_col */
      build_ditherBW_tables();
      break;
    case 4 :
      YR = grey4Set[0];
      build_map_lut(dpy_disp, screen, depth_disp);
      if (affCOLOR == MODNB && L_col <= CIF_WIDTH){
#ifdef TELESIA
        XtSetSensitive(coulAff, FALSE ) ;
	XtSetSensitive(doubleButton, FALSE ) ;
#endif /* TELESIA */
        L_col <<= 1;
        L_lig <<= 1;
        XtResizeWidget(*pt_appShell, L_col, L_lig, 0);
      } /* fi affCOLOR */
#ifdef TELESIA
      XtSetSensitive(coulButton, FALSE ) ;
#endif /* TELESIA */
      break;

    case 8 :
/*
**  Construction de la colormap et de la map_lut
*/

      YR = grey8Set[0];
      RR = colSet[0][0];
      GR = colSet[0][1];
      BR = colSet[0][2];
      build_map_lut(dpy_disp, screen, depth_disp);
      if (affCOLOR == MODNB && L_col <= CIF_WIDTH){
#ifdef TELESIA
        XtSetSensitive(coulAff, FALSE ) ;
	XtSetSensitive(doubleButton, FALSE ) ;
#endif /* TELESIA */
        L_col <<= 1;
        L_lig <<= 1;
        XtResizeWidget(*pt_appShell, L_col, L_lig, 0);
      } /* fi affCOLOR */
      break;

    case 24 :
    case 32 :
/*
** In 24 bit - TRUECOLOR, a RGB pattern is stored in an int : RRGGBB
*/
      XSetWindowBorder(dpy_disp, win_disp, blanc);
      build_rgb_tables();
      colmap = XCreateColormap(dpy_disp, root, DefaultVisual(dpy_disp, screen), 
			       AllocNone);
      break;

    default :
      fprintf (stderr, 
	       "DISPLAY.C-init_window() : screen depth not yet supported\n");
      exit(1);
  } /* hctiws depth */

  if(affCOLOR != MODCOLOR){
    XtSetSensitive(greyButton, FALSE) ;
    XtSetSensitive(colorButton, FALSE) ;
  }

  XSetWindowColormap(dpy_disp, win_disp, colmap);
  XSetWindowColormap(dpy_disp, icon_win, colmap);


#ifdef WIN_DEBUG
  fprintf(stderr,
	  "DISPLAY.C-init_window() : image buffer allocation - L_col =
%d, L_lig = %d, depth = %d\n",
	  L_col, L_lig, depth_disp);
  fflush(stderr);
#endif

  *pt_ximage = alloc_shared_ximage(dpy_disp, L_col, L_lig, depth_disp);

  *pt_gc = gc_disp;
  *pt_win = win_disp;
  *pt_icon = icon_win ;
  *pt_depth = depth_disp;
  *pt_dpy = dpy_disp;

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : display the window\n");
  fflush(stderr);
#endif
#ifndef HIDE_WINDOW
  XtMapWidget(*pt_appShell);
#endif
#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : normal exit\n");
  fflush(stderr);
#endif
  XSync(dpy_disp, False);

return ;
}

/*---------------------------------------------------------------*/
