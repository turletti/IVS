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
*  File              : display.h                                           *
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

/*-----------------------------------------------------*/
#define XR_ECRETE(x, n)	((n)>(x) ? (x) :((n)<0 ? 0:(n) ))

#define XR_ECRETE_255(n)	(((unsigned long int) (n) <= 255)? \
              (n) : (((~(n))>>(sizeof(unsigned long int)*4-1)) & 255))
/*-----------------------------------------------------*/
#undef EXTERN
#ifdef DATA_MODULE
#define EXTERN 
#else
#define EXTERN extern
#endif
/*-----------------------*/
EXTERN Colormap        	colmap
#ifdef DATA_MODULE
 = 0
#endif
;
EXTERN u_char	RR;
EXTERN u_char	GR;
EXTERN u_char	BR;
EXTERN u_char	YR;

EXTERN u_char	grey4Set[RAMPGREY]
#ifdef DATA_MODULE
 = {8, 6, 4}
#endif
;
EXTERN u_char	grey8Set[RAMPGREY]
#ifdef DATA_MODULE
 = {64, 32, 16}
#endif
;
EXTERN u_char	colSet[RAMPCOL][3]
#ifdef DATA_MODULE
 = {6, 9, 3,
    5, 8, 3,
    5, 5, 2,
    3, 3, 2,
}
#endif
;
/*-----------------------------------------------------*/
EXTERN u_char  affCOLOR 
#ifdef DATA_MODULE
= MODCOLOR /* prend les valeurs 1 = N&B, 
	    2 = niveaux de gris, 4 = couleur */
#endif
;
/*-----------------------------------------------------*/
#define TYPE_CUV2RGB int

#define cv2r (&cv2_rg[0])
#define cv2g (&cv2_rg[256])
#define cu2b (&cu2_bg[0])
#define cu2g (&cu2_bg[256])

EXTERN TYPE_CUV2RGB    	cv2_rg[512],
                	cu2_bg[512];

#define TYPE_CY2RGB int
EXTERN TYPE_CY2RGB     	cy2yrgb[256],
                        cy2y24rgb[256];

EXTERN u_char 		y2rgb_conv[256];

/*-----------------------------------------------------*/
EXTERN u_char 		*r_darrays[ORD_DITH_SZ],
               		*g_darrays[ORD_DITH_SZ],
                	*b_darrays[ORD_DITH_SZ],
                	*y_darrays[ORD_DITH_SZ];

/*-----------------------------------------------------*/
EXTERN u_char		*colmap_lut;
/*-----------------------------------------------------*/
EXTERN Display		*dpy_disp; /*?????*/
EXTERN GC 		gc_disp;
EXTERN Window 		win_disp;
EXTERN int		depth_disp;
EXTERN Widget		appShell, panel;
/*-----------------------------------------------------*/
EXTERN BOOLEAN 	locDOUBLE
#ifdef DATA_MODULE
 = FALSE
#endif
 ;
EXTERN BOOLEAN 	locHALF
#ifdef DATA_MODULE
 = FALSE
#endif
 ;
/*-----------------------------------------------------*/
#ifdef MITSHM
EXTERN BOOLEAN does_shm;
#endif /* MITSHM */
/*---------- PROTOTYPE FORWARD REFERENCE --------------*/
char *sure_malloc();
void *malloc();
XImage *alloc_shared_ximage();
void destroy_shared_ximage();
/*-----------------------------------------------------*/







