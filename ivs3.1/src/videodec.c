/**************************************************************************
*          Copyright 1993 by Jian Zhang & Ian West                         *
*                            CSIRO/Joint Research Centre                   *
* 		             in Information Technology                     *
*                            Flinders University of South Australia        *
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
*  File    : dec_color_grabbing.c                                          *
*  Date    : 14/5/93                                                       *
*  Author  : Jian Zhang  & Ian west E-mail: jian@jrc.flinders.edu.au	   *
*                                   E-mail: westyjrc.flinders.edu.au       *
*  Reference : DEC Xmedia Toolkits and coder.c (with bug-commentaries)     *
*              from SOnny SE Kiu Kone  se@cceio.enet.dec.com               *
*--------------------------------------------------------------------------*
*  Description :       VIDEOTX grabbing procedure                          *
*                                                                          *
* This VIDEOTX grabbing procedure needs to  be compiled with Digital       *
* Equipment Corporation's Xmedia Toolkit library which requires an         *
* appropriate Xmedia Toolkit license.                                      *
*                                                                          *
* This file is provided AS IS with no warranties of any kind.              * 
* The author shall have no liability with respect to the infringement      *
* of copyrights, trade secrets or any patents by this file or any part     * 
* thereof.  In no event will the author be liable for any lost revenue     *
* or profits or other special, indirect and consequential damages.         *
*--------------------------------------------------------------------------*
* Thanks  Thierry Turletti, he gave me the helpful suggestions while I     *
* implemented this procedure.                                              *
\**************************************************************************/

#include "h261.h"
#include "protocol.h"

#ifdef VIDEOTX
#include <X11/extensions/Xvlib.h>


static unsigned int SIG_TO_CCIR[256] ={
16,  16,  17,  18,  19,  20,  21,  22,  
22,  23,  24,  25,  26,  27,  28,  28,
29,  30,  31,  32,  33,  34,  34,  35,  
36,  37,  38,  39,  40,  40,  41,  42,
43,  44,  45,  46,  46,  47,  48,  49,  
50,  51,  52,  52,  53,  54,  55,  56,
57,  58,  58,  59,  60,  61,  62,  63,  
64,  64,  65,  66,  67,  68,  69,  70,
70,  71,  72,  73,  74,  75,  76,  76,  
77,  78,  79,  80,  81,  82,  82,  83,
84,  85,  86,  87,  88,  89,  89,  90,  
91,  92,  93,  94,  95,  95,  96,  97,
98,  99, 100, 101, 101, 102,  103, 104, 
105, 106, 107, 107, 108, 109, 110, 111,
112, 113, 113, 114, 115, 116, 117, 118, 
119, 119, 120, 121, 122, 123, 124, 125,
125, 126, 127, 128, 129, 130, 131, 131, 
132, 133, 134, 135, 136, 137, 137, 138,
139, 140, 141, 142, 143, 143, 144, 145, 
146, 147, 148, 149, 149, 150, 151, 152,
153, 154, 155, 155, 156, 157, 158, 159, 
160, 161, 162, 162, 163, 164, 165, 166,
167, 168, 168, 169, 170, 171, 172, 173, 
174, 174, 175, 176, 177, 178, 179, 180,
180, 181, 182, 183, 184, 185, 186, 186, 
187, 188, 189, 190, 191, 192, 192, 193,
194, 195, 196, 197, 198, 198, 199, 200, 
201, 202, 203, 204, 204, 205, 206, 207,
208, 209, 210, 210, 211, 212, 213, 214, 
215, 216, 216, 217, 218, 219, 220, 221,
222, 222, 223, 224, 225, 226, 227, 228, 
228, 229, 230, 231, 232, 233, 234, 235
};

static unsigned CSIG_TO_CCIR[256] ={  
128, 128, 129, 130, 131, 132, 133, 134, 
135, 135, 136, 137, 138, 139, 140, 141, 
142, 142, 143, 144, 145, 146, 147, 148, 
149, 149, 150, 151, 152, 153, 154, 155, 
156, 156, 157, 158, 159, 160, 161, 162, 
163, 164, 164, 165, 166, 167, 168, 169, 
170, 171, 171, 172, 173, 174, 175, 176, 
177, 178, 178, 179, 180, 181, 182, 183, 
184, 185, 185, 186, 187, 188, 189, 190, 
191, 192, 193, 193, 194, 195, 196, 197, 
198, 199, 200, 200, 201, 202, 203, 204, 
205, 206, 207, 207, 208, 209, 210, 211, 
212, 213, 214, 214, 215, 216, 217, 218, 
219, 220, 221, 221, 222, 223, 224, 225, 
226, 227, 228, 229, 229, 230, 231, 232, 
233, 234, 235, 236, 236, 237, 238, 239,
16,  17,  18,  19,  20,  20,  21,  22,  
23,  24,  25,  26,  27,  27,  28,  29,  
30,  31,  32,  33,  34,  35,  35,  36,  
37,  38,  39,  40,  41,  42,  42,  43,  
44,  45,  46,  47,  48,  49,  49,  50,  
51,  52,  53,  54,  55,  56,  56,  57,  
58,  59,  60,  61,  62,  63,  63,  64,  
65,  66,  67,  68,  69,  70,  71,  71,  
72,  73,  74,  75,  76,  77,  78,  78,  
79,  80,  81,  82,  83,  84,  85,  85,  
86,  87,  88,  89,  90,  91,  92,  92,  
93,  94,  95,  96,  97,  98,  99,  100, 
100, 101, 102, 103, 104, 105, 106, 107, 
107, 108, 109, 110, 111, 112, 113, 114, 
114, 115, 116, 117, 118, 119, 120, 121, 
121, 122, 123, 124, 125, 126, 127, 128
};

typedef long INT32;                   
#define SCALEBITS       14
#define ONE_HALF        ((INT32) 1 << (SCALEBITS-1))
#define FIX(x)          ((INT32) ((x) * (1L<<SCALEBITS) + 0.5))
#define OFFSET       ((INT32) 1<<(SCALEBITS + 7))

#define PAL_FIELD_HEIGHT        287     /* Height of a field in PAL */ 
#define PAL_FIELD_HEIGHT_DIV2   143
#define CIF_WIDTH               352     /* Width of CIF */
#define CIF_HEIGHT              288     /* Height of CIF */
#define QCIF_WIDTH              176     /* Width of Quarter CIF */
#define QCIF_HEIGHT		144     /* Height of Quarter CIF */
#define COL_MAX                 352     /* Width of CIF */
#define LIG_MAX                 288     /* Height of CIF */
#define LIG_MAX_DIV2            144     /* Width of Cr and Cb */
#define COL_MAX_DIV2            176     /* height of Cr and Cb */
#define QCIF_WIDTH_DIV2         88      /* Half Width of Quarter CIF */


extern BOOLEAN STAT_MODE;
extern FILE *F_loss;



static void Xv_Encoding(dpy, port, width, height, rate, size)
    Display        *dpy;
    XvPortID        port;
    int            *width, *height;
    XvRational     *rate;
    int size;	
{
    int             encoding;
    unsigned int    nEncodings;
    XvEncodingInfo *encodingInfo;
    XvEncodingInfo *a;
    int             i;
    Atom            encodingAtom;

    encodingAtom = XInternAtom(dpy, "XV_ENCODING", True);

    XvGetPortAttribute(dpy, port, encodingAtom, &encoding);


    XvQueryEncodings(dpy, port, &nEncodings, &encodingInfo);
    for (i = 0; i < nEncodings; i++) {
        a = &encodingInfo[i];
        if (a->encoding_id == encoding) {
            *rate = a->rate;
            XvFreeEncodingInfo(a);
         }
    }
   
}

static void Setup_v_port(pAdaptors, p_port, p_vis_id)
    XvAdaptorInfo  *pAdaptors;
    unsigned long  *p_port;
    unsigned long  *p_vis_id;

{
    int             port;
    int             visual_id;
    XvAdaptorInfo  *pAdaptor;
    XvFormat       *pFormat;
    XvRational      r;

    pAdaptor = pAdaptors;
    port = pAdaptor->base_id;
    pFormat = pAdaptor->formats;
    visual_id = pFormat->visual_id;

    *p_port = port;

    *p_vis_id = visual_id;
}



static int WindowSetup(dpy, width, height, mainWin, gc, port)
    Display        *dpy;
    int            *width, *height;
    Window         *mainWin;
    GC             *gc;
    XvPortID       *port;

{

    int             ii;
    int             status;

    Window          root;
    int             screen;
    Visual         *def_vis;
    unsigned long   vis_id;
    XVisualInfo    *p_vis_info;
    XVisualInfo     vis_info_tmpl;
    XGCValues       gc_attr;
    XSetWindowAttributes win_attr;
    Colormap        cmap;
    XEvent          event;
    XColor          scolor;
    XColor          ecolor;
    Visual         *vis;
    unsigned int    major_opcode;
    unsigned int    version;
    unsigned int    revision;
    unsigned int    event_base;
    unsigned int    error_base;
    unsigned int    nAdaptors;
    XvAdaptorInfo  *pAdaptors;
    int             win_h, win_w;
    XvRational      r;
    
    win_w = *width;
    win_h = *height;

    root = XDefaultRootWindow(dpy);
    screen = XDefaultScreen(dpy);
    status = XvQueryExtension(dpy, &version, &revision,
                              &major_opcode, &event_base, &error_base);

    if (status != Success) {
        fprintf(stderr, "\n  Xv video extension not available\n");
        exit(-1);
    }
    XvQueryAdaptors(dpy, root, &nAdaptors, &pAdaptors);

    if (!nAdaptors) {
        fprintf(stderr, "\n  Your display has no video adaptors\n");
        exit(-1);
    }
    
    Setup_v_port(pAdaptors,port, &vis_id);

    Xv_Encoding(dpy, *port, &win_w, &win_h, &r);
    
    vis_info_tmpl.visualid = vis_id;

    p_vis_info = XGetVisualInfo(dpy, VisualIDMask,
                                &vis_info_tmpl, &ii);
    if (!p_vis_info) {
        fprintf(stderr, "      Error: Couldn't find visual ");
        fprintf(stderr, "#%x listed for adaptor.\n", vis_id);
        exit(-1);
    }
    vis = p_vis_info->visual;

    def_vis = XDefaultVisual(dpy, screen);

    if (vis->visualid == def_vis->visualid)
        cmap = XDefaultColormap(dpy, screen);

    else
        cmap = XCreateColormap(dpy, root, vis, AllocNone);

    XAllocNamedColor(dpy, cmap, "midnight blue", &scolor, &ecolor);

    win_attr.colormap = cmap;
    win_attr.background_pixel = scolor.pixel;
    win_attr.event_mask = ExposureMask | VisibilityChangeMask;
    win_attr.backing_store = Always;
    win_attr.border_pixel = scolor.pixel;
    win_attr.override_redirect = True;

    *mainWin = XCreateWindow(dpy, root, 0, 0, win_w, win_h, 0,
                    24, InputOutput, vis,
                    CWColormap | CWBackPixel | CWEventMask |
                    CWBackingStore | CWBorderPixel, 
                    &win_attr);

    XMapWindow(dpy, *mainWin); 

    while (1) {
        XNextEvent(dpy, &event);
        if (event.type == VisibilityNotify)
            break;
    }

    XvSelectVideoNotify(dpy, *mainWin, True);

    /*******************************************
     *
     * setup gc which is used later in Xvputvideo
     */

    gc_attr.foreground = scolor.pixel;

    *gc = XCreateGC(dpy, *mainWin, GCForeground, &gc_attr);

    return (TRUE);

}

static int BitShift(mask)
    unsigned long   mask;

{
    int             i = 0;

    while (!(mask & 1)) {
        mask >>= 1;
        i++;
    }
    return (i);
}



void dec_get_qcif_pal(dpy, win, port, gc, winWidth, winHeight, im_data)
                      
    Display        *dpy;
    Window          win;
    XvPortID        port;
    GC              gc;
    int             winWidth;
    int             winHeight;
    u_char         im_data[LIG_MAX][COL_MAX]; 
  
{

    unsigned int    *ptr;
    int              i, j;
    int              width, height;
    XImage          *image;
    register u_char *r_data;
    unsigned int tmp;
    


    /**************************************************************
     *
     * Hold the server to make sure no part of the window is obscured
     */

    
    XGrabServer(dpy);
    XRaiseWindow(dpy, win);
    XSync(dpy,0);
    XvPutVideo(dpy, port, win, gc, 0, 0,
               winWidth, winHeight, 0, 0, winWidth, winHeight);
    image = XGetImage(dpy, win, 0, 0, winWidth, winHeight, -1, ZPixmap);
    XUngrabServer(dpy);
    XSync(dpy,0);

 
    /**************************************************************
     *
     *  Read in the data from Image strcture and store the image data in   
     *  buffer.
     */


    ptr = (unsigned int  *) image->data; 
                                         
    /* Read in data */
 
    width = image->width;
    height = image->height;
    

    
     for (j = 0; j < height; j++) {
       r_data = &(im_data[j][0]);
      for (i = 0; i < width; i++) { 
           *r_data ++ = SIG_TO_CCIR[*ptr ++ &0xff];
        }

    }

    XDestroyImage(image);
   
}

void dec_get_cif_pal(dpy, win, port, gc, winWidth, winHeight, im_data)
                      
    Display        *dpy;
    Window          win;
    XvPortID        port;
    GC              gc;
    int             winWidth;
    int             winHeight;
    u_char         im_data[][COL_MAX]; 
  
{

    unsigned int    *ptr;
    int              i, j;
    int              width, height;
    XImage          *image;
    register u_char *r_data;
    unsigned int tmp;
    


    /**************************************************************
     *
     * Hold the server to make sure no part of the window is obscured
     */

    
    XGrabServer(dpy);
    XRaiseWindow(dpy, win);
    XSync(dpy,0);
    XvPutVideo(dpy, port, win, gc, 0, 0,
               winWidth, winHeight, 0, 0, winWidth, winHeight);
    image = XGetImage(dpy, win, 0, 0, winWidth, winHeight, -1, ZPixmap);
    XUngrabServer(dpy);
    XSync(dpy,0);

 
    /**************************************************************
     *
     *  Read in the data from Image strcture and store the image data in   
     *  buffer.
     */


    ptr = (unsigned int  *) image->data; 
                                         
    /* Read in data */
 
    width = image->width;
    height = image->height;
    r_data = (u_char *)im_data;    

     for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) { 
           *r_data ++ = SIG_TO_CCIR[*ptr ++ &0xff];
        }

    }

    XDestroyImage(image);
   
}


void dec_get_color_qcif_pal(dpy, win, port, gc, winWidth, winHeight, 
			                            Y_data, Cb_data, Cr_data)
                      
    Display        *dpy;
    Window          win;
    XvPortID        port;
    GC              gc;
    int             winWidth;
    int             winHeight;
    u_char         Y_data[][COL_MAX]; 
    u_char         Cr_data[][COL_MAX_DIV2]; 
    u_char         Cb_data[][COL_MAX_DIV2];   
{

    unsigned int    *ptr;
    int              i, j;
    int             width, height;
    XImage          *image;
    register u_char *y_data, *cr_data, *cb_data;
    register int r, g, b, y;
    unsigned int    redMask, greenMask, blueMask;
    int             redVal, greenVal, blueVal;


    /**************************************************************
     *
     * Hold the server to make sure no part of the window is obscured
     */

    
    XGrabServer(dpy);
    XRaiseWindow(dpy, win);
    XSync(dpy,0);
    XvPutVideo(dpy, port, win, gc, 0, 0,
               winWidth, winHeight, 0, 0, winWidth, winHeight);
    image = XGetImage(dpy, win, 0, 0, winWidth, winHeight, -1, ZPixmap);
    XUngrabServer(dpy);
    XSync(dpy,0);

 
    /**************************************************************
     *
     *  Read in the data from Image strcture and store the image data in   
     *  buffer.
     */


    ptr = (unsigned int  *) image->data; 
                                      
    /* Read in data */
 

    width = image->width;
    height = image->height;

    redMask = image->red_mask;
    greenMask = image->green_mask;
    blueMask = image->blue_mask;

    redVal = BitShift(redMask);
    greenVal = BitShift(greenMask);
    blueVal = BitShift(blueMask);

     for (i = 0; i <height ; i++) {
     y_data = (u_char *)&Y_data[i][0];   
     cr_data = (u_char *)&Cr_data[i/2][0];
     cb_data = (u_char *)&Cb_data[i/2][0];
       for (j = 0; j <width; j++) {

    /**************************************************************
     *
     * get the R,G,B and translate them into Y, Cr, Cb. 
     */

	r = (*ptr & redMask) >> redVal;
	g = (*ptr & greenMask) >> greenVal;
	b = (*ptr & blueMask) >> blueVal;	

y = ((  FIX(0.29900)*r  + FIX(0.58700)*g + FIX(0.11400)*b + ONE_HALF) >> SCALEBITS);

	*y_data++ = SIG_TO_CCIR[y&0xff];
	
	   ptr ++;

	   if ( (!(i%2)) && (!(j%2)) )
	     {

			   
 *cr_data++ = CSIG_TO_CCIR[((r - y)*FIX(0.56500)>>SCALEBITS)&0xff];

 *cb_data++ = CSIG_TO_CCIR[((b - y)*FIX(0.71320)>>SCALEBITS)&0xff];
	
 
	 }	


       
        }

    }

    XDestroyImage(image);
   
}

void dec_get_color_cif_pal(dpy, win, port, gc, winWidth, winHeight, 
			                            Y_data, Cb_data, Cr_data)
                      
    Display        *dpy;
    Window          win;
    XvPortID        port;
    GC              gc;
    int             winWidth;
    int             winHeight;
    u_char         Y_data[][COL_MAX]; 
    u_char         Cr_data[][COL_MAX_DIV2]; 
    u_char         Cb_data[][COL_MAX_DIV2];   
{

    unsigned int    *ptr;
    int              i, j;
    int             width, height;
    XImage          *image;
    register u_char *y_data, *cr_data, *cb_data;
    register int    r, g, b, y;
    unsigned int    redMask, greenMask, blueMask;
    int             redVal, greenVal, blueVal;


    /**************************************************************
     *
     * Hold the server to make sure no part of the window is obscured
     */

    
    XGrabServer(dpy);
    XRaiseWindow(dpy, win);
    XSync(dpy,0);
    XvPutVideo(dpy, port, win, gc, 0, 0,
               winWidth, winHeight, 0, 0, winWidth, winHeight);
    image = XGetImage(dpy, win, 0, 0, winWidth, winHeight, -1, ZPixmap);
    XUngrabServer(dpy);
    XSync(dpy,0);

 
    /**************************************************************
     *
     *  Read in the data from Image strcture and store the image data in   
     *  buffer.
     */


    ptr = (unsigned int  *) image->data; 
                                      
    /* Read in data */
 

    width = image->width;
    height = image->height;

    redMask = image->red_mask;
    greenMask = image->green_mask;
    blueMask = image->blue_mask;

    redVal = BitShift(redMask);
    greenVal = BitShift(greenMask);
    blueVal = BitShift(blueMask);

     y_data = (u_char *)Y_data;   
     cr_data = (u_char *)Cr_data;
     cb_data = (u_char *)Cb_data;

     for (i = 0; i <height ; i++) {
       for (j = 0; j <width; j++) {

    /**************************************************************
     *
     * get the R,G,B and translate them into Y, Cr, Cb. 
     */

	r = (*ptr & redMask) >> redVal;
	g = (*ptr & greenMask) >> greenVal;
	b = (*ptr & blueMask) >> blueVal;	

y = ((  FIX(0.29900)*r  + FIX(0.58700)*g + FIX(0.11400)*b + ONE_HALF) >> SCALEBITS);

	*y_data++ = SIG_TO_CCIR[y&0xff];
	
	   ptr ++;

	   if ( (!(i%2)) && (!(j%2)) )
	     {

			   
 *cr_data++ = CSIG_TO_CCIR[((r - y)*FIX(0.56500)>>SCALEBITS)&0xff];

 *cb_data++ = CSIG_TO_CCIR[((b - y)*FIX(0.71320)>>SCALEBITS)&0xff];
	
 
	 }	


       
        }

    }

    XDestroyImage(image);
   
}


void dec_video_encode(group, port, ttl, size,fd_f2s, fd_s2f, COLOR, STAT, 
		      FREEZE, FEEDBACK,display, rate_control, rate_max, 
                      brightness, contrast, default_quantizer)
     char *group, *port;     /* Address socket */
     u_char ttl;             /* ttl value */
     int size;               /* image's size */
     int fd_f2s[2];          /* pipe descriptor father to son */
     int fd_s2f[2];          /* pipe descriptor son to father*/
     BOOLEAN COLOR;          /* Ture if color encoding */
     BOOLEAN STAT;           /* True if statistical mode */
     BOOLEAN FREEZE;         /* True if image frozen */
     BOOLEAN FEEDBACK;        /* True if feedback allowed */
     char *display;          /* Display name */
     int rate_control;       /* WITHOUT_CONTROL or REFRESH_CONTROL or
                                QUALITY_CONTROL */
     int rate_max;           /* Maximum bandwidth allowed (kb/s) */
     int brightness, contrast;
     int default_quantizer;

{
    IMAGE_CIF image_coeff;
    u_char new_Y[LIG_MAX][COL_MAX];
    u_char new_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
    u_char new_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
    int NG_MAX;
    int sock_recv=0;
    BOOLEAN FIRST=TRUE; /* True if the first image */
    BOOLEAN CONTINUE = TRUE;
    BOOLEAN NEW_LOCAL_DISPLAY=FALSE;
/*    BOOLEAN COLOR = TRUE;          Ture if color encoding */
    BOOLEAN TUNING; 
    int vfc_format = 1; /* VFC_PAL */
    int new_size, new_vfc_port, brightness, contrast; /* Not used here */
    void (*get_image)(); 

    /* Video control socket declaration */

    sock_recv=declare_listen(PORT_VIDEO_CONTROL);

    if(STAT){
      if((F_loss = fopen(".videoconf.loss", "w")) == NULL) {
	fprintf(stderr, "cannot create videoconf.loss file\n");
      }else
	STAT = FALSE;
    }
    STAT_MODE = STAT;


 

/*********************************************************************
 *
 *  Video coming from "DEC video board", VIDEOTX start.
 *
 */


{
    int             windowOpened = FALSE;
    Window          mainWin;
    Display        *dpy;
    GC              gc;
    XvPortID        xvport;
    int             width, height;

    
    switch(size){
	
    case CIF_SIZE:
      NG_MAX =12;
      width = CIF_WIDTH;
      height = PAL_FIELD_HEIGHT;
      break;
    
    case QCIF_SIZE:
      NG_MAX = 5;
      width = QCIF_WIDTH;
      height = PAL_FIELD_HEIGHT_DIV2;
      break;
    }    



    if (!COLOR){
	COLOR = FALSE;

   switch(size){
      case CIF_SIZE:
	get_image = dec_get_cif_pal;
	break;
      case QCIF_SIZE:
	get_image = dec_get_qcif_pal;
	break;
    }
    }else{
	switch(size){
	  case CIF_SIZE:
	    get_image = dec_get_color_cif_pal;
	    break;
	  case QCIF_SIZE:
	    get_image = dec_get_color_qcif_pal;
	    break;
	}	
    }

   dpy = XOpenDisplay(display);
   if (!dpy) {
        fprintf(stderr,"Cannot open display %s\n", display);
         exit(-11); /* ERROR */
    }
 
    windowOpened = WindowSetup(dpy, &width, &height,
                               &mainWin, &gc, &xvport);
 
 

    if (!windowOpened) {
        fprintf(stderr, "Failed to open the proper window.\n");
        exit(-1);
    }


    /**************************
     *
     *  start grab frames
     */

    
    do{
      if(FREEZE == FALSE) 
	if (COLOR)
 (*get_image)(dpy, mainWin, xvport, gc, width, height, new_Y, new_Cb, new_Cr); 
      else
  (*get_image)(dpy, mainWin, xvport, gc, width, height, new_Y); 
      encode_h261(group, port, ttl, new_Y, new_Cb, new_Cr,
		  image_coeff, NG_MAX, sock_recv, FIRST, COLOR, FEEDBACK, 
		  rate_control, rate_max,fd_s2f[1]);
      CONTINUE = HandleCommands(fd_f2s, &new_size, &rate_control, 
				&new_vfc_port, &rate_max, &FREEZE, 
				&NEW_LOCAL_DISPLAY, &TUNING, &FEEDBACK, 
				&brightness, &contrast);    
      FIRST = FALSE;	
  
    }while(CONTINUE);

    XCloseDisplay(dpy);
    close(fd_f2s[0]);
    close(fd_s2f[1]);
    if(STAT_MODE)
    fclose(F_loss);
    exit(0);

  }
}

#endif /* VIDEOTX */






