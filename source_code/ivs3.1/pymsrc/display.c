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
*                                                                          *
*  File              : display.c                                           *
*  Author            : Thierry Turletti                                    *
*  Last modification : 93/3/22                                             *
*--------------------------------------------------------------------------*
*  Description : Initialization of the display: colormap, window.          *
*									   *
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*   Pierre Delamotte      |  93/3/20  |  Colormap managing added.          *
* delamot@wagner.inria.fr |           | (all building colormap functions)  *
\**************************************************************************/

#include "h261.h"
#include "protocol.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>


/*
**	CONSTANTES & MACROS
*/
#define CIF_WIDTH       352
#define ORD_DITH_SZ	16
/********************************
#define Y2RGB		8422
#define U2B		7836
#define U2G		1473
#define V2G		2965
#define V2R		5965
********************************/
#define Y2RGB		4788
#define U2B		8295
#define U2G		1611
#define V2G		3343
#define V2R		6563

#define Y_CCIR_MAX	240
#define YNBR		2
#define RAMPGREY	3
#define RAMPCOL		4
#define MODCOLOR	4
#define MODGREY		2
#define MODNB		1

#define XR_ECRETE(x, n)	((n)>(x) ? (x) :((n)<0 ? 0:(n) ))

#ifdef MITSHM
BOOLEAN does_shm;
#endif /* MITSHM */

/*
**	EXPORTED VARIABLES
*/

u_char		map_lut[256] ;
BOOLEAN		CONTEXTEXIST = FALSE;
BOOLEAN		encCOLOR;
u_char		affCOLOR = MODCOLOR; /* prend les valeurs 1 = N&B, 
					2 = niveaux de gris, 4 = couleur */
u_char   	noir, blanc;


int	cur_brght = 0;
float	cur_ctrst = 1.0;
int	cur_int_ctrst = 0;

/*
**	GLOBAL VARIABLES
*/
/* tables de dithering en couleur 8 bits */

static Display		*dpy;
static GC 		gc;
static Window 		win;
static int		depth;
static Widget		appShell, panel;

static u_char		*colmap_lut ;

static u_char 		*r_darrays[ORD_DITH_SZ],
               		*g_darrays[ORD_DITH_SZ],
                	*b_darrays[ORD_DITH_SZ],
                	*y_darrays[ORD_DITH_SZ] ;

int			COL_MP_LGTH = 0;
int			cv2r[256],
              		cv2g[256],
                	cu2g[256],
                	cu2b[256],
			cy2yrgb[256];
u_char			y2rgb_conv[256];

Colormap        	colmap = 0;
u_char			nbSet[RAMPGREY] = {64, 32, 16};
u_char			colCard[RAMPCOL] = {162, 120, 50, 18};
u_char			colSet[RAMPCOL][3] = 	{
                                 		6, 9, 3,
                                 		5, 8, 3,
                                 		5, 5, 2,
                                 		3, 3, 2,
                               			};


static u_char		RR;
static u_char		GR;
static u_char		BR;
static u_char		YR;


static u_char		std_map_lut[256];
static int		stdmplt_size = 0;
static u_long		pixels[256];


static int		offset_NB = 0;
static BOOLEAN		GREYMAPLUT = FALSE;
static BOOLEAN		COLORMAPLUT= FALSE;

		
static u_char		y_map[256] ,
	 		red_map [256],
         		green_map [256],
         		blue_map [256] ;
static u_char		blanc8[8], noir8[8] ;

/*  Useful ordered dither matrices  */

BOOLEAN 		DITH_TABLES_EXIST = FALSE ;


/*
**		PROTOTYPES
*/

BOOLEAN 		build_grey_cmap() ;
BOOLEAN 		build_color_cmap() ;
void 			build_map_lut () ;
void 			rebuild_map_lut() ;
void 			build_dither_tables() ;

/*
**		CORPS DE FONCTION
*/



#ifdef MITSHM
int socket_is_local(s)
     int s;
{
  struct sockaddr name;
  int len = sizeof(name);
 
  if(getsockname(s, &name, &len) == -1){
    perror("getsockname");
    exit(1);
  }
  return(len == 0 ? 1 : 0);
}
#endif /* MITSHM */



Visual *get_visual(dpy)
     Display *dpy;
{
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



XImage *alloc_shared_ximage(dpy, w, h, depth)
     Display *dpy;
     int w, h, depth;
{
  XImage *ximage;
  u_char *data;
  Visual *im_visu ;
  int oct_w_sz, oct_h_sz, format, im_pad ;

  if (depth == 1 || affCOLOR == MODNB) {
/*
    oct_w_sz = (w + 7)/8 ;
    oct_h_sz = (h + 7)/8 ;
*/
    oct_w_sz = w ;
    oct_h_sz = h ;
    format = XYBitmap ;
    depth = 1 ;
  } else {
    oct_w_sz = w ;
    oct_h_sz = h ;
    format = ZPixmap ;
  }/* fi depth */

  if (depth == 24) {
    im_pad = 32 ;
    im_visu = get_visual(dpy) ;
  } else {
    im_pad = 8 ;
    im_visu = NULL ;
  }/* fi depth */
    
#ifdef MITSHM
  if(does_shm){
    XShmSegmentInfo *shminfo;

    shminfo = (XShmSegmentInfo *) sure_malloc(sizeof(XShmSegmentInfo),
				    "Can't malloc XShmSegmentInfo\n");
    ximage = XShmCreateImage(dpy, im_visu, depth, format, NULL, shminfo, w, h);
    shminfo->shmid = shmget(IPC_PRIVATE,oct_w_sz*oct_h_sz,IPC_CREAT|0777);
    if(shminfo->shmid < 0){
      perror("shmget");
      does_shm = FALSE ;
      XDestroyImage(ximage);
    } else {
      shminfo->shmaddr = (char *)shmat(shminfo->shmid, 0, 0);
      if(shminfo->shmaddr == ((char *)-1)){
        perror("shmat");
        does_shm = FALSE ;
        XDestroyImage(ximage);
      } else {
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
          does_shm = FALSE ;
        } else
          return(ximage);
      } /* fi shminfo->shmaddr == */
    } /* fi shminfo->shmid */
  } /* fi does_shm */
#endif /* MITSHM */

  data = (u_char *) sure_malloc(oct_w_sz*oct_h_sz,"Can't malloc visual image\n");
  ximage = XCreateImage(dpy, im_visu, depth, format, 0, data, w, h, im_pad, 0);
  return(ximage);
}


void 
destroy_shared_ximage(dpy, ximage)
XImage *ximage;
Display *dpy;
{

#ifdef MITSHM
  if(does_shm){
    XShmDetach(dpy, ximage->obdata);
    shmdt(ximage->data);
    ximage->data = NULL;
    free(ximage->obdata);
    ximage->obdata = NULL;
    XDestroyImage(ximage);
    return ;
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

void


kill_window(dpy, ximage)
     Display *dpy;
     XImage *ximage;
{
  destroy_shared_ximage(dpy, ximage);
  XCloseDisplay(dpy);
}



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



void init_window(L_lig, L_col, name_window, display, pt_appCtxt, pt_appShell,
		 pt_depth, pt_dpy, pt_win, pt_gc, pt_ximage, FORCE_GREY)
     int		L_lig, L_col;
     char 		*name_window;
     char 		*display;
     XtAppContext	*pt_appCtxt;
     Widget		*pt_appShell;
     int 		*pt_depth;
     Display 	        **pt_dpy;
     Window 		*pt_win;
     GC		        *pt_gc;
     XImage 		**pt_ximage;
     BOOLEAN            FORCE_GREY;
{

  Window 		root = 0;
  int 			screen = 0;
  Visual 		*visu;
  Screen 		*SCREEN;
  u_char 		*data;
  register int 		k;
  int			largc = 1;
  char                  control_name[200];

  if(CONTEXTEXIST) 
    return;

  if(!encCOLOR || FORCE_GREY) 
    affCOLOR = MODGREY;

  
  *pt_appCtxt =	XtCreateApplicationContext();
  CONTEXTEXIST = TRUE;

  if ((dpy = XtOpenDisplay(*pt_appCtxt,  display, "tvIVS", "TvIVS", NULL, 0, 
		      &largc, &name_window)) == NULL) {
    fprintf(stderr, "DISPLAY.C-init_window() : XtOpenDisplay failed\n") ;
    exit(1) ;
  }

#ifdef MITSHM
  if(socket_is_local(ConnectionNumber(dpy))){
    int *majorv, *minorv;
    Bool *sharedPixmaps;

    does_shm = XShmQueryVersion(dpy, &majorv, &minorv, &sharedPixmaps);
#ifdef DEBUG
    if(does_shm)
      printf("Local videoconf, using Shared Memory\n");
#endif
  }else
    does_shm = FALSE;
#endif /* MITSHM */

  appShell = *pt_appShell = XtAppCreateShell("tvIVS", "TvIVS", 
					     topLevelShellWidgetClass, dpy,
					     NULL, 0);
  XtSetMappedWhenManaged(*pt_appShell, FALSE);
/*
**  Management of display control panel
*/

  XtAddEventHandler(*pt_appShell, ButtonPressMask, FALSE, display_panel, 
		     NULL);

  strcpy(control_name, "Tuning Panel : ");
  strcat(control_name, name_window);
  CreatePanel(control_name, &panel, *pt_appShell);
  XtSetMappedWhenManaged(panel, FALSE);

  XtResizeWidget(*pt_appShell, L_col, L_lig, 0);

  XtRealizeWidget(*pt_appShell);


  screen = 	DefaultScreen(dpy);
  depth = 	DefaultDepth(dpy,screen); 

  win = 	XtWindow(*pt_appShell);
  XStoreName(dpy, win, name_window);
  XSelectInput(dpy, win, ButtonPressMask);
  root = 	RootWindow(dpy, screen);

  noir = 	(u_char) BlackPixel(dpy, screen);
  blanc = 	(u_char) WhitePixel(dpy, screen);

  gc = 		XtGetGC(*pt_appShell, 0, NULL);

  XSetWindowBackground(dpy, win, blanc);

  for (k = 0 ; k < 8 ; k++) {
    blanc8[k] = blanc << k ;
    noir8[k] = noir << k ;
  } /* rof k */

  switch (depth) {
    case 1 :
      XSetBackground(dpy, gc, noir) ;
      XSetForeground(dpy, gc, blanc) ;
      affCOLOR = MODNB ;
    case 4 :

    case 8 :
/*
**  Construction de la colormap et de la map_lut
*/

      YR = nbSet[0];
      RR = colSet[0][0];
      GR = colSet[0][1];
      BR = colSet[0][2];
      build_map_lut(dpy, screen, depth) ;
      if (affCOLOR == MODNB && L_col <= CIF_WIDTH) {
        L_col <<= 1 ;
        L_lig <<= 1 ;
        XtResizeWidget(*pt_appShell, L_col, L_lig, 0);
      } /* fi affCOLOR */
      break ;

    case 24 :
/*
** In 24 bit - TRUECOLOR, a RGB pattern is stored in an int : RRGGBB
*/
      L_col <<= 2 ;
      XSetWindowBorder(dpy, win, blanc);
      colmap = XCreateColormap(dpy, root, DefaultVisual(dpy, screen), AllocNone) ;
      break ;

    default :
      fprintf(stderr, 
	      "DISPLAY.C-init_window() : screen depth not yet supported\n") ;
      exit(1) ;
  } /* hctiws depth */



  XSetWindowColormap(dpy, win, colmap);



#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : allocation de la zone image\n");
  fflush(stderr);
#endif

  *pt_ximage = alloc_shared_ximage(dpy, L_col, L_lig, depth);
  (*pt_ximage)->bits_per_pixel = depth;

  *pt_gc = gc;
  *pt_win = win;
  *pt_depth = depth;
  *pt_dpy = dpy;

  XtMapWidget(*pt_appShell);

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-init_window() : sortie\n");
  fflush(stderr);
#endif
  XSync(dpy, False);
}


void build_map_lut(display, screen, depth)
     Display	*display;
     int	screen, depth;
{
  Screen		*SCREEN;
  register int 		i = 0, k;
  int			rampdisp = 0;
  float 		tmp_ind;
  XColor		colors [256];
  register int 		ri, bi, gi;

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-build_map_lut() : entree\n");
  fflush(stderr);
#endif

  SCREEN = XScreenOfDisplay(display, screen);
  colmap = DefaultColormapOfScreen(SCREEN);


  if(depth > 4){
    /*
    *  Grey and color colormaps are automatically set up.
    */
  
    /*
    *  Building the grey colormap.
    */

    if(affCOLOR > MODNB){
      k = 0;
      YR = nbSet[k];
      while((!build_grey_cmap(display, colmap, &rampdisp)) && (++k < RAMPGREY)) 
	YR = nbSet[k];
      if(k >= RAMPGREY) affCOLOR = MODNB;
      else{
	/*
	*  std_map_lut's size is saved. Looking for the number of available 
	*  colors.
	*/
        stdmplt_size = offset_NB = rampdisp;
	/*
	*  16 <= Yccir <= 235
	*/

        for(i=0; i<16; i++){
          y2rgb_conv[i] = map_lut[0];
        } /* rof i */
        for(; i<236; i++){
          tmp_ind = (double)(i + 30)*(double)(YR-1)/235.0;
          y2rgb_conv[i] = map_lut[XR_ECRETE(YR-1, (int) (tmp_ind + 0.5) )];
        } /* rof i */
        for(; i<(Y_CCIR_MAX+1); i++){
          y2rgb_conv[i] = map_lut[YR];
        } /* rof i */
      } /* fi k */

      /*
      *  Building the color colormap.
      */

      if(affCOLOR == MODCOLOR){
        k = 0;
        RR = colSet[k][0];
        GR = colSet[k][1];
        BR = colSet[k][2];
        while(!build_color_cmap(display, colmap, &rampdisp) && 
	      (++k < RAMPCOL)){
          RR = colSet[k][0];
          GR = colSet[k][1];
          BR = colSet[k][2];
        } /* elihw build_color_cmap */
        if(k >= RAMPCOL) affCOLOR = MODGREY;
        else{
          colmap_lut = map_lut + offset_NB;
          COL_MP_LGTH = RR*GR*BR;
          stdmplt_size += rampdisp;
        } /* fi k */
      } /* fi MODCOLOR */
    } /* fi affCOLOR */

  }else 
    affCOLOR = MODNB; /* fi depth */

  /*
  *  Building the dithering's tables.
  */

  build_dither_tables();
#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-build_map_lut() : sortie\n");
  fflush(stderr);
#endif
} /* end build_map_lut () */



BOOLEAN build_grey_cmap(display, colmap, i)
     Display		*display;
     Colormap           colmap;
     int		*i;
{
  register int		k;
  XColor		colors[256];

  *i = 0;
#ifdef COL_DEBUG
  fprintf(stderr, "DISPLAY.C-build_grey_cmap(): entree, YR vaut %d\n", YR);
  fflush(stderr);
#endif
  for(k=0; (k<256) && (*i<256); k+=255/(YR-1)){
    y_map[*i] = k;
    colors[*i].red = colors[*i].green = colors[*i].blue = k<<8;
    colors[*i].flags = DoRed | DoGreen | DoBlue;
    if(!XAllocColor(display, colmap, &colors[*i])){
      /*
      *  No much place in the default colormap.
      */

      fprintf(stderr, "DISPLAY.C-build_grey_cmap() : no more space available \
in standard colormap, IVS will not be able to display in a nice color mode\n" );

      XFreeColors(display, colmap, pixels, (*i), 0);

      GREYMAPLUT = FALSE ;
#ifdef COL_DEBUG
      fprintf(stderr, 
   "DISPLAY.C-build_grey_cmap(): error return, %d places remain, GREYMAPLUT = %d\n",
	      (*i), GREYMAPLUT);
#endif
      return(GREYMAPLUT) ;
    }else{
      std_map_lut[*i] = map_lut[*i]  = pixels[*i] = colors[*i].pixel;
      (*i)++;
    } /* fi XAllocColor */
  } /* rof k */

  GREYMAPLUT = TRUE ;
#ifdef COL_DEBUG
  fprintf(stderr, 
	  "DISPLAY.C-build_grey_cmap(): valid return, %d places used,
GREYMAPLUT = %d\n",
	  (*i), GREYMAPLUT);
  fflush(stderr);
#endif
  return (GREYMAPLUT) ;
}



BOOLEAN build_color_cmap(display, colmap, i)
     Display		*display;
     Colormap           colmap;
     int		*i;
{
  XColor		colors[256];
  register int 		ri, bi, gi;

  *i = 0;
  for(ri = 0; ri < 256; ri += 255/(RR-1))
    for(gi = 0; gi < 256; gi += 255/(GR-1))
      for(bi = 0; bi < 256; bi += 255/(BR-1)){
        red_map[*i] = ri;
        colors[*i].red = ri<<8;
        green_map[*i] = gi;
        colors[*i].green = gi<<8;
        blue_map[*i] = bi;
        colors[*i].blue = bi<<8;
        colors[*i].flags = DoRed | DoGreen | DoBlue;

        if( !XAllocColor(display, colmap, &colors[*i])){
	  /* No much place in the default colormap. We have to reduce the 
	  * number of colors used.
	  */
#ifdef COL_DEBUG
          fprintf(stderr, "DISPLAY.C-build_color_cmap() : no more space \
available in standard colormap,\nIVS will use of a reduced set of color\n");
#endif
          XFreeColors(display, colmap,(pixels + YR), (*i), 0);

          COLORMAPLUT = FALSE;
#ifdef COL_DEBUG
  fprintf(stderr, 
  "DISPLAY.C-build_color_cmap(): error returned, %d places remain,
COLORMAPLUT = %d\n",
	  (*i), COLORMAPLUT);
#endif
          return(COLORMAPLUT);
        }else{
          std_map_lut[(*i) + offset_NB] = map_lut[(*i) + offset_NB]  = 
	    pixels[(*i) + offset_NB] = colors[*i].pixel;
          (*i)++;
        } /* fi XAllocColor */
      } /* rof rof rof */

  COLORMAPLUT = TRUE;
#ifdef COL_DEBUG
  fprintf(stderr, 
	 "DISPLAY.C-build_color_cmap(): valid return, %d places used,
COLORMAPLUT = %d\n",
	  (*i), COLORMAPLUT);
  fflush(stderr);
#endif
  return(COLORMAPLUT);
}


void build_dither_tables()
{
  int		i, j, k, err_range, threshval ;
  int		r_values[256], g_values[256], b_values[256], y_values[2] ;
  u_char 	*tmp_darrays ;
  float 	tmp_ind ;

  for (i = 0 ; i < RR ; i++)
    r_values[i] = (i * 256) / RR + 256/(RR*2) ;
  for (i = 0 ; i < GR ; i++)
    g_values[i] = (i * 256) / GR + 256/(GR*2) ;
  for (i = 0 ; i < BR ; i++)
    b_values[i] = (i * 256) / BR + 256/(BR*2) ;

  y_values[0] = 220/4 + 16 ;
  y_values[1] = 220/2 + 220/4 + 16 ;

/*
** Generate a table entry for each possible colour
** value for every dither table location
*/

  for (i = 0 ; i < ORD_DITH_SZ ; i++) {
    tmp_darrays = r_darrays[i] = (u_char *)
sure_malloc(256,"DISPLAY.C-build_dither_tables() : cannot alloc r_darrays\n") ;
    err_range = r_values[0] ;
    threshval = ((i * err_range) / ORD_DITH_SZ) ;
    for (k = 0 ; k < r_values[0] ; k++) {
      *tmp_darrays++ = (k > threshval) ? GR*BR : 0 ;
    } /* rof k */
    for (j = 0 ; j < (RR-1) ; j++) {
      err_range = r_values[j+1] - r_values[j] ;
      threshval = ((i * err_range) / ORD_DITH_SZ) + r_values[j] ;

      for (k = r_values[j] ; k < r_values[j+1] ; k++) {
        *tmp_darrays++ = (k > threshval) ? ((j + 1)*GR*BR) : (j*GR*BR) ;
      } /* rof k */
    } /* rof j */
    for (j = r_values[RR-1] ; j < 256 ; j++) {
      *tmp_darrays++ = (RR-1)*GR*BR ;
    }
  } /* rof i RED */


  for (i = 0 ; i < ORD_DITH_SZ ; i++) {
    tmp_darrays = g_darrays[i] = (u_char *)
sure_malloc(256,"DISPLAY.C-build_dither_tables() : cannot alloc g_darrays\n") ;
    err_range = g_values[0] ;
    threshval = ((i * err_range) / ORD_DITH_SZ) ;
    for (k = 0 ; k < g_values[0] ; k++) {
      *tmp_darrays++ = (k > threshval) ? BR : 0 ;
    } /* rof k */
    for (j = 0 ; j < (GR-1) ; j++) {
      err_range = g_values[j+1] - g_values[j] ;
      threshval = ((i * err_range) / ORD_DITH_SZ) + g_values[j] ;

      for (k = g_values[j] ; k < g_values[j+1] ; k++) {
        *tmp_darrays++ = (k > threshval) ? ((j + 1)*BR) : (j*BR) ;
      } /* rof k */
    } /* rof j */
    for (j = g_values[GR-1] ; j < 256 ; j++) {
      *tmp_darrays++ = (GR-1)*BR ;
    }  
  } /* rof i GREEN */

  for (i = 0 ; i < ORD_DITH_SZ ; i++) {
    tmp_darrays = b_darrays[i] = (u_char *)
sure_malloc(256,"DISPLAY.C-build_dither_tables() : cannot alloc b_darrays\n") ;
    err_range = b_values[0] ;
    threshval = ((i * err_range) / ORD_DITH_SZ) ;
    for (k = 0 ; k < b_values[0] ; k++) {
      *tmp_darrays++ = (k > threshval) ? 1 : 0 ;
    } /* rof k */
    for (j = 0 ; j < (BR-1) ; j++) {
      err_range = b_values[j+1] - b_values[j] ;
      threshval = ((i * err_range) / ORD_DITH_SZ) + b_values[j] ;

      for (k = b_values[j] ; k < b_values[j+1] ; k++) {
        *tmp_darrays++ = (k > threshval) ? (j + 1) : j ;
      } /* rof k */
    } /* rof j */
    for (j = b_values[BR-1] ; j < 256 ; j++) {
      *tmp_darrays++ = BR-1 ;
    }
      
  } /* rof i BLUE */

  for (i = 0 ; i < ORD_DITH_SZ ; i++) {
#ifdef DITH_DEBUG
  fprintf(stderr, "DISPLAY.C-build_dither_tables() : allocation du %d
eme tableau y_arrays\n", i) ;
  fflush(stderr) ;
#endif 
    tmp_darrays = y_darrays[i] = (u_char *)
sure_malloc(256,"DISPLAY.C-build_dither_tables() : cannot alloc y_darrays\n")  ;
/**********************
    tmp_darrays = &y_darrays[i][0] ;
**********************/

    for (j = 0 ; j < y_values[0] ; j++) {
      *tmp_darrays++ = noir ;
    }
    err_range = y_values[1] - y_values[0] ;
    threshval = ((i * err_range) / ORD_DITH_SZ) + y_values[0] ;
    for (k = y_values[0] ; k < y_values[1] ; k++) {
      *tmp_darrays++ = (k > threshval) ? blanc : noir ;
    } /* rof k */
    for (j = y_values[1] ; j < 256 ; j++) {
      *tmp_darrays++ = blanc ;
    }
  } /* rof i B&W */


/*
**  Generates YUVccir ==> RGB translation coefficients tables :
**	cv2r[], cv2g[], cu2g[], cu2b[], cy2yrgb[]
**  
*/

    for(i = 16 ; i < Y_CCIR_MAX + 1 ; i++){
      tmp_ind = (double)(i-128)*(double)(V2R)/4096.0;
      cv2r[i] = (int)(tmp_ind + 0.5);
      tmp_ind = (double)(i-128)*(double)(V2G)/4096.0;
      cv2g[i] = (int)(tmp_ind + 0.5);
      tmp_ind = (double)(i-128)*(double)(U2G)/4096.0;
      cu2g[i] = (int)(tmp_ind + 0.5);
      tmp_ind = (double)(i-128)*(double)(U2B)/4096.0;
      cu2b[i] = (int)(tmp_ind + 0.5);
/*      tmp_ind = (double)(i-16)*(double)(Y2RGB)/4096.0;*/
      tmp_ind = (double)(i+5)*(double)(Y2RGB)/4096.0; 
      cy2yrgb[i] = (int)(tmp_ind + 0.5);
    } /* rof i */
    for(i = 0 ; i < 16; i++){
      cy2yrgb[i] = cy2yrgb[16] ;
      cv2r[i] = cv2r[16] ;
      cv2g[i] = cv2g[16] ;
      cu2g[i] = cu2g[16] ;
      cu2b[i] = cu2b[16] ;
    }/* rof i */
    for(i = Y_CCIR_MAX + 1 ; i < 256 ; i++){
      cv2r[i] = cv2r[Y_CCIR_MAX] ;
      cv2g[i] = cv2g[Y_CCIR_MAX] ;
      cu2g[i] = cu2g[Y_CCIR_MAX] ;
      cu2b[i] = cu2b[Y_CCIR_MAX] ;
    }/* rof i */
    for(i = Y_CCIR_MAX - 5 ; i < 256 ; i++) cy2yrgb[i] = cy2yrgb[Y_CCIR_MAX - 5] ;


} /* end build_dither_tables() */    




void rebuild_map_lut()
{
  register int i, ri, gi, bi, yi;
  float 	tmp_ind;

  if((COLORMAPLUT || GREYMAPLUT) && (cur_brght == 0) && (cur_int_ctrst == 0)){
    for(i=0; i<stdmplt_size; i++) 
      map_lut[i] = std_map_lut[i];
    return;
  }

  if(GREYMAPLUT) {
#ifdef RBM_DEBUG
  fprintf(stderr, "DISPLAY.C-rebuilt_map_lut() : on passe par traitmt
gris, GREYMAPLUT = %d\n", GREYMAPLUT ) ;
  fflush(stderr) ;
#endif
    for (i=0;i<YR; i++){
      yi = XR_ECRETE((YR-1),
          (int)(((double)y_map[i] +
(double)cur_brght)*cur_ctrst*(double)(YR-1)/255.0 + 0.5));
      map_lut[i] = std_map_lut[yi];
    } /* rof i */
    for(i=16; i<236; i++){
      tmp_ind = (double)(i + 30)*(double)(YR-1)/235.0;
      y2rgb_conv[i] = map_lut[XR_ECRETE(YR-1, (int) tmp_ind)];
    } /* rof i */
  } /* fi GREYMAPLUT */

  if(COLORMAPLUT) {

    for(i = 16 ; i < Y_CCIR_MAX + 1 ; i++){
      tmp_ind = ((double) i + (double)cur_brght)*cur_ctrst ;
      tmp_ind = (tmp_ind - 16.0)*(double)(Y2RGB)/4096.0;
      cy2yrgb[i] = (int)(tmp_ind + 0.5);
    } /* rof i */
    for(i = 0 ; i < 16; i++){
      cy2yrgb[i] = cy2yrgb[16] ;
    }/* rof i */
    for(i = Y_CCIR_MAX - 5 ; i < 256 ; i++) cy2yrgb[i] = cy2yrgb[Y_CCIR_MAX - 5] ;
  } /* fi COLORMAPLUT */

  if(affCOLOR < MODGREY){
    register int j, k, err_range, threshval ;
    int 	y_values[2] ;
    u_char 	*tmp_darrays ;

    y_values[0] = 220/4 + 16 ;
    y_values[1] = 220/2 + 220/4 + 16 ;
    y_values[1] = XR_ECRETE(255, (int)(((double)(256/2 + 256/4) +
(double)cur_brght )*cur_ctrst + 0.5)) ;

    for (i = 0 ; i < ORD_DITH_SZ ; i++) {
      tmp_darrays = &y_darrays[i][0] ;
      for (j = 0 ; j < y_values[0] ; j++) *tmp_darrays++ = noir ;

      for (j = 0 ; j < 1 ; j++) {
        err_range = y_values[j+1] - y_values[j] ;
        threshval = ((i * err_range) / ORD_DITH_SZ) + y_values[j] ;
        for (k = y_values[j] ; k < y_values[j+1] ; k++) {
          *tmp_darrays++ = ((int)(((double)k + (double)cur_brght
)*cur_ctrst + 0.5) > threshval) ? blanc : noir ;
        } /* rof k */
      } /* rof j */
      for (j = y_values[1] ; j < 240 ; j++) *tmp_darrays++ = blanc ;
      
    } /* rof i B&W */
  } /* fi affCOLOR */
} /* end rebuild_map_lut() */


void
build_image (Y_data, Cu_data, Cv_data, DOUBLE, outXImage, srcWidth,
dstOffset, w, h )
u_char *Y_data, *Cu_data, *Cv_data ;
BOOLEAN DOUBLE ;
XImage *outXImage ;
int srcWidth, dstOffset, w, h ;
{
  register int l, c, offsetY, offsetC, offsetX ;
  register u_char *out ;

#ifdef BW_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : entree - DOUBLE = %d,
dstOffset = %d, ximage->width = %d\n",
          DOUBLE, dstOffset, outXImage->width) ;
  fflush(stderr) ;
#endif

  if (outXImage->format == ZPixmap) {

    if(outXImage->depth == 24 && affCOLOR & MODCOLOR) {
        register u_char 	*Y_data2 ;
        register int 	R, V, B, CUB, CVR, CUVG ;
        register u_int 	*i_out1, *i_out2 ;

#ifdef BIT24_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : entree - traitement 24 bit
couleur\n") ;
  fflush(stderr) ;
#endif

        i_out1 = ((u_int *)outXImage->data) + dstOffset ;
        offsetY = (srcWidth << 1) - w ;
        offsetC = (srcWidth >> 1) - (w >> 1) ;
        offsetX = (outXImage->width << 1) - w;

        Y_data2 = Y_data + srcWidth ;
        i_out2 = i_out1 + outXImage->width ;

        for(l=0; l < h; l+=2){
          for(c=0; c < w; c+=2){

            CVR = cv2r[*Cv_data] ;
            CUB = cu2b[*Cu_data] ;
            CUVG = cv2g[*Cv_data++] + cu2g[*Cu_data++] ;

            R = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CVR) ;
            V = XR_ECRETE(255, cy2yrgb[Y_data[0]] - CUVG) << 8;
            B = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CUB) << 16 ;

            i_out1[0] = R|V|B ; 

            R = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CVR) ;
            V = XR_ECRETE(255, cy2yrgb[Y_data[1]] - CUVG) << 8 ;
            B = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CUB) << 16 ;

            i_out1[1] = R|V|B ; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CVR) ;
            V = XR_ECRETE(255, cy2yrgb[Y_data2[0]] - CUVG) << 8 ;
            B = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CUB) << 16 ;

            i_out2[0] = R|V|B ; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CVR) ;
            V = XR_ECRETE(255, cy2yrgb[Y_data2[1]] - CUVG) << 8 ;
            B = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CUB) << 16 ;

            i_out2[1] = R|V|B ; 

            i_out1 += 2 ; i_out2 += 2 ;
            Y_data += 2 ; Y_data2 += 2 ;

          } /* rof c */
          Y_data += offsetY ;
          Y_data2 += offsetY ;
          Cu_data += offsetC ;
          Cv_data += offsetC ;
          i_out1 += offsetX;
          i_out2 += offsetX;


        } /* rof l */
    } else 
    if(outXImage->depth == 24 && affCOLOR & MODGREY) {

        register u_int 	*i_out ;

        i_out = ((u_int *)outXImage->data) + dstOffset ;
        offsetY = srcWidth - w ;

#ifdef BIT24_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : entree - traitement 24 bit
gris, DOUBLE = %s\n",
                  DOUBLE ? "True" : "False") ;
  fflush(stderr) ;
#endif
        if (DOUBLE) {
          register u_int *i_out2, aux ;

          offsetX = (outXImage->width - w) << 1 ;

          i_out = ((u_int *)outXImage->data) + (dstOffset << 1) ;
          i_out2 = i_out + outXImage->width ;

          for(l=0; l < h; l++){
            for(c=0; c < w; c++) {
              aux = y2rgb_conv[*Y_data] | y2rgb_conv[*Y_data]<<8 |
y2rgb_conv[*Y_data++]<<16 ;
              *i_out++ = aux ;
              *i_out++ = aux ;
              *i_out2++ = aux ;
              *i_out2++ = aux ;
            } /* rof c */
            Y_data += offsetY ;
            i_out += offsetX ;
            i_out2 += offsetX ;
          } /* rof l */
        } else {
          offsetX = outXImage->width - w ;
          i_out = ((u_int *)outXImage->data) + dstOffset ;

          for(l=0; l < h; l++){
            for(c=0; c < w; c++)
              *i_out++ = y2rgb_conv[*Y_data] | y2rgb_conv[*Y_data]<<8 |
y2rgb_conv[*Y_data++]<<16 ;
            Y_data += offsetY ;
            i_out += offsetX ;
          } /* rof l */
        } /* fi double */
    } else
    if(outXImage->depth >= 8 && affCOLOR & MODCOLOR) {

        register u_char *Y_data2, *out2;
        register int R, V, B, CUB, CVR, CUVG ;

        out = (u_char *)outXImage->data + dstOffset ;
        offsetY = (srcWidth << 1) - w ;
        offsetC = (srcWidth >> 1) - (w >> 1) ;
        offsetX = (outXImage->width << 1) - w;

        Y_data2 = Y_data + srcWidth ;
        out2 = out + outXImage->width ;

        for(l=0; l < h; l+=4){
          for(c=0; c < w; c+=4){

            CVR = cv2r[*Cv_data] ;
            CUB = cu2b[*Cu_data] ;
            CUVG = cv2g[*Cv_data++] + cu2g[*Cu_data++] ;

            R = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[0]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CUB);

            out[0] = colmap_lut[ b_darrays[0][B] + g_darrays[0][V] +
r_darrays[0][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[1]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CUB);

            out[1] = colmap_lut[ b_darrays[8][B] + g_darrays[8][V] +
r_darrays[8][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[0]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CUB);

            out2[0] = colmap_lut[ b_darrays[12][B] + g_darrays[12][V] +
r_darrays[12][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[1]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CUB);

            out2[1] = colmap_lut[ b_darrays[4][B] + g_darrays[4][V] +
r_darrays[4][R] ]; 

            CVR = cv2r[*Cv_data] ;
            CUB = cu2b[*Cu_data] ;
            CUVG = cv2g[*Cv_data++] + cu2g[*Cu_data++] ;

            R = XR_ECRETE(255, cy2yrgb[Y_data[2]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[2]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[2]] + CUB);

            out[2] = colmap_lut[ b_darrays[2][B] + g_darrays[0][V] +
r_darrays[0][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data[3]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[3]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[3]] + CUB);

            out[3] = colmap_lut[ b_darrays[10][B] + g_darrays[8][V] +
r_darrays[8][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[2]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[2]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[2]] + CUB);

            out2[2] = colmap_lut[ b_darrays[14][B] + g_darrays[12][V] +
r_darrays[12][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[3]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[3]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[3]] + CUB);

            out2[3] = colmap_lut[ b_darrays[6][B] + g_darrays[4][V] +
r_darrays[4][R] ]; 
    
            out += 4 ; out2 += 4 ;
            Y_data += 4 ; Y_data2 += 4 ;

          } /* rof c */

          Y_data += offsetY ;
          Y_data2 += offsetY ;
          Cu_data += offsetC ;
          Cv_data += offsetC ;
          out += offsetX;
          out2 += offsetX;

          for(c=0; c < w; c+=4){

            CVR = cv2r[*Cv_data] ;
            CUB = cu2b[*Cu_data] ;
            CUVG = cv2g[*Cv_data++] + cu2g[*Cu_data++] ;

            R = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[0]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[0]] + CUB);

            out[0] = colmap_lut[ b_darrays[3][B] + g_darrays[3][V] +
r_darrays[3][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[1]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[1]] + CUB);

            out[1] = colmap_lut[ b_darrays[11][B] + g_darrays[11][V] +
r_darrays[11][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[0]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[0]] + CUB);

            out2[0] = colmap_lut[ b_darrays[15][B] + g_darrays[15][V] +
r_darrays[15][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[1]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[1]] + CUB);

            out2[1] = colmap_lut[ b_darrays[7][B] + g_darrays[7][V] +
r_darrays[7][R] ]; 

            CVR = cv2r[*Cv_data] ;
            CUB = cu2b[*Cu_data] ;
            CUVG = cv2g[*Cv_data++] + cu2g[*Cu_data++] ;

            R = XR_ECRETE(255, cy2yrgb[Y_data[2]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[2]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[2]] + CUB);

            out[2] = colmap_lut[ b_darrays[1][B] + g_darrays[1][V] +
r_darrays[1][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data[3]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data[3]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data[3]] + CUB);

            out[3] = colmap_lut[ b_darrays[9][B] + g_darrays[9][V] +
r_darrays[9][R] ]; 

            R = XR_ECRETE(255, cy2yrgb[Y_data2[2]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[2]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[2]] + CUB);

            out2[2] = colmap_lut[ b_darrays[13][B] + g_darrays[13][V] +
r_darrays[13][R] ]; 
  
            R = XR_ECRETE(255, cy2yrgb[Y_data2[3]] + CVR);
            V = XR_ECRETE(255, cy2yrgb[Y_data2[3]] - CUVG);
            B = XR_ECRETE(255, cy2yrgb[Y_data2[3]] + CUB);

            out2[3] = colmap_lut[ b_darrays[5][B] + g_darrays[5][V] +
r_darrays[5][R] ]; 

            out += 4 ; out2 += 4 ;
            Y_data += 4 ; Y_data2 += 4 ;

          } /* rof c */

          Y_data += offsetY ;
          Y_data2 += offsetY ;
          Cu_data += offsetC ;
          Cv_data += offsetC ;
          out += offsetX;
          out2 += offsetX;

       } /* rof l */
    }else
    if(outXImage->depth >= 8 && (affCOLOR & MODGREY)) {

/*  GREY LEVEL DISPLAYING
**  One pixel is encoded with 6 bits (64 shades of gray)
*/
      offsetY = srcWidth - w ;

      if (DOUBLE) {
        register u_char *out2, aux ;

#ifdef BW_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : traitement gris QCIF->CIF\n") ;
  fflush(stderr) ;
#endif
        offsetX = (outXImage->width - w) << 1 ;

        out = (u_char *)outXImage->data + (dstOffset << 1) ;
        out2 = out + outXImage->width ;

        for(l=0; l < h; l++){
          for(c=0; c < w; c++) {
            aux = y2rgb_conv[*Y_data++];
            *out++ = aux ;
            *out++ = aux ;
            *out2++ = aux ;
            *out2++ = aux ;
          } /* rof c */
          Y_data += offsetY ;
          out += offsetX ;
          out2 += offsetX ;
        } /* rof l */
      } else {
        offsetX = outXImage->width - w ;
        out = (u_char *)outXImage->data + dstOffset ;

        for(l=0; l < h; l++){
          for(c=0; c < w; c++) *out++ = y2rgb_conv[*Y_data++] ;
          Y_data += offsetY ;
          out += offsetX ;
        } /* rof l */
      } /* fi double */
    } /* fi MODGREY */
  } else{
/*
**  BLACK & WHITE DISPLAYING
**  using ordered dither algorithme
*/
    offsetY = srcWidth - w;

    if (DOUBLE) {
      register u_char *out2 ;

      offsetX = (outXImage->width - w ) >> 2 ;

      out = (u_char *)outXImage->data + (dstOffset >> 2) ;
      out2 = out + (outXImage->width >> 3) ;

      for(l = 0; l < h; l += 2){
        for(c = 0; c < w; c += 4) {
          *out = 0 ;
          *out2 = 0 ;
          *out |= y_darrays [0][*Y_data] ? blanc8[7] : noir8[7] ;
          *out2 |= y_darrays [12][*Y_data] ? blanc8[7] : noir8[7] ;
          *out |= y_darrays [8][*Y_data] ? blanc8[6] : noir8[6]  ;
          *out2 |= y_darrays [4][*Y_data++] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [2][*Y_data] ? blanc8[5] : noir8[5]  ;
          *out2 |= y_darrays [14][*Y_data] ? blanc8[5] : noir8[5]  ;
          *out |= y_darrays [10][*Y_data] ? blanc8[4] : noir8[4]  ;
          *out2 |= y_darrays [6][*Y_data++] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [0][*Y_data] ? blanc8[3] : noir8[3] ;
          *out2 |= y_darrays [12][*Y_data] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [8][*Y_data] ? blanc8[2] : noir8[2]  ;
          *out2 |= y_darrays [4][*Y_data++] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [2][*Y_data] ? blanc8[1] : noir8[1]  ;
          *out2 |= y_darrays [14][*Y_data] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [10][*Y_data] ? blanc8[0] : noir8[0]  ;
          *out2++ |= y_darrays [6][*Y_data++] ? blanc8[0] : noir8[0]  ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
        out2 += offsetX ;
        for(c = 0; c < w; c += 4) {
          *out = 0 ;
          *out2 = 0 ;
          *out |= y_darrays [3][*Y_data] ? blanc8[7] : noir8[7] ;
          *out2 |= y_darrays [15][*Y_data] ? blanc8[7] : noir8[7] ;
          *out |= y_darrays [11][*Y_data] ? blanc8[6] : noir8[6]  ;
          *out2 |= y_darrays [7][*Y_data++] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [1][*Y_data] ? blanc8[5] : noir8[5] ;
          *out2 |= y_darrays [13][*Y_data] ? blanc8[5] : noir8[5]  ;
          *out |= y_darrays [9][*Y_data] ? blanc8[4] : noir8[4]  ;
          *out2 |= y_darrays [5][*Y_data++] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [3][*Y_data] ? blanc8[3] : noir8[3] ;
          *out2 |= y_darrays [15][*Y_data] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [11][*Y_data] ? blanc8[2] : noir8[2]  ;
          *out2 |= y_darrays [7][*Y_data++] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [1][*Y_data] ? blanc8[1] : noir8[1]  ;
          *out2 |= y_darrays [13][*Y_data] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [9][*Y_data] ? blanc8[0] : noir8[0]  ;
          *out2++ |= y_darrays [5][*Y_data++] ? blanc8[0] : noir8[0]  ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
        out2 += offsetX ;
      } /* rof l */
    } else {
      offsetX = (outXImage->width - w) >> 3 ;
      out = (u_char *)outXImage->data + (dstOffset >> 3) ;

      for(l=0; l<h; l+=4){
        for(c=0; c<w; c+=8) {
          *out = 0 ;
          *out |= y_darrays [0][Y_data[0]] ? blanc8[7] : noir8[7] ;
          *out |= y_darrays [8][Y_data[1]] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [2][Y_data[2]] ? blanc8[5] : noir8[5] ;
          *out |= y_darrays [10][Y_data[3]] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [0][Y_data[4]] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [8][Y_data[5]] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [2][Y_data[6]] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [10][Y_data[7]] ? blanc8[0] : noir8[0]  ;
          Y_data += 8 ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
        for(c=0; c<w; c+=8) {
          *out = 0 ;
          *out |= y_darrays [12][Y_data[0]] ? blanc8[7] : noir8[7] ;
          *out |= y_darrays [4][Y_data[1]] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [14][Y_data[2]] ? blanc8[5] : noir8[5] ;
          *out |= y_darrays [6][Y_data[3]] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [12][Y_data[4]] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [4][Y_data[5]] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [14][Y_data[6]] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [6][Y_data[7]] ? blanc8[0] : noir8[0]  ;
          Y_data += 8 ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
        for(c=0; c<w; c+=8) {
          *out = 0 ;
          *out |= y_darrays [3][Y_data[0]] ? blanc8[7] : noir8[7] ;
          *out |= y_darrays [11][Y_data[1]] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [1][Y_data[2]] ? blanc8[5] : noir8[5] ;
          *out |= y_darrays [9][Y_data[3]] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [3][Y_data[4]] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [11][Y_data[5]] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [1][Y_data[6]] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [9][Y_data[7]] ? blanc8[0] : noir8[0]  ;
          Y_data += 8 ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
        for(c=0; c<w; c+=8) {
          *out = 0 ;
          *out |= y_darrays [15][Y_data[0]] ? blanc8[7] : noir8[7] ; 
          *out |= y_darrays [7][Y_data[1]] ? blanc8[6] : noir8[6]  ;
          *out |= y_darrays [13][Y_data[2]] ? blanc8[5] : noir8[5] ;
          *out |= y_darrays [5][Y_data[3]] ? blanc8[4] : noir8[4]  ;
          *out |= y_darrays [15][Y_data[4]] ? blanc8[3] : noir8[3] ;
          *out |= y_darrays [7][Y_data[5]] ? blanc8[2] : noir8[2]  ;
          *out |= y_darrays [13][Y_data[6]] ? blanc8[1] : noir8[1]  ;
          *out++ |= y_darrays [5][Y_data[7]] ? blanc8[0] : noir8[0]  ;
          Y_data += 8 ;
        } /* rof c */
        Y_data  += offsetY ;
        out += offsetX ;
      } /* rof l */
    } /* fi double */

  } /* fi format == XPixmap */
} /* end build_image */

void
show(dpy, ximage, L_col, L_lig)
Display *dpy;
XImage *ximage;
int L_col, L_lig;

{
#ifdef MITSHM
  if(does_shm){
    XShmPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, L_col, L_lig, FALSE);
    XSync(dpy, FALSE);
    return;
  }
#endif /* MITSHM */
  XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, L_col, L_lig);
  XFlush(dpy);
}


