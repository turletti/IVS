/**************************************************************************
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
*  File              : videosparc_vfc.c                                    *
*  Last modification : November 1992                                       *
*  Author            : Thierry Turletti                                    *
*--------------------------------------------------------------------------*
*  Description :  Video interface for VIDEOPIX framegrabber.               *
*                                                                          *
* Added support for NTSC, and auto format detection,                       *
* David Berry, Sun Microsystems,                                           *
* david.berry@sun.com                                                      *
*                                                                          *
* The VideoPix specific code is originally from work that is               *
* Copyright (c) 1990-1992 by Sun Microsystems, Inc.                        *
* Permission to use, copy, modify, and distribute this software and        * 
* its documentation for any purpose and without fee is hereby granted,     *
* provided that the above copyright notice appear in all copies            * 
* and that both that copyright notice and this permission notice appear    * 
* in supporting documentation.                                             *
*                                                                          *
* This file is provided AS IS with no warranties of any kind.              * 
* The author shall have no liability with respect to the infringement      *
* of copyrights, trade secrets or any patents by this file or any part     * 
* thereof.  In no event will the author be liable for any lost revenue     *
* or profits or other special, indirect and consequential damages.         *
*--------------------------------------------------------------------------*
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
* Winston Dang            |  92/10/1  | alterations were made for speed    *
* wkd@Hawaii.Edu          |           | purposes.                          *
*--------------------------------------------------------------------------*
* Pierre Delamotte        |  93/2/27  | COMPYTOCCIR added to grabbing      *
* delamot@wagner.inria.fr |           | procedures.                        *
*--------------------------------------------------------------------------*
* Thierry Turletti        |  93/3/4   | Now image's format are the same in *
*                         |           | PAL and NTSC formats.              *
*--------------------------------------------------------------------------*
* Pierre Delamotte        |  93/3/20  | Color grabbing procedures added.   *
\**************************************************************************/

#ifdef VIDEOPIX

#include "h261.h"
#include "protocol.h"
#include <vfc_lib.h>

#define bornCCIR(x) ((x) > 235 ? 235 : ((x) < 16 ? 16 : (x)))

#define GET_UBITS        0x00c00000
#define GET_VBITS        0x00300000
#define NTSC_FIELD_HEIGHT       240     /* Height of a field in NTSC */
#define PAL_FIELD_HEIGHT        287     /* Height of a field in PAL */
#define PAL_FIELD_HEIGHT_DIV2	143
#define HALF_YUV_WIDTH          360     /* Half the width of YUV (720/2) */
#define CIF_WIDTH               352     /* Width of CIF */
#define CIF_HEIGHT              288     /* Height of CIF */
#define QCIF_WIDTH              176     /* Width of Quarter CIF */
#define QCIF_WIDTH_DIV2         88      /* Half Width of Quarter CIF */
#define COL_MAX                 352
#define LIG_MAX                 288
#define LIG_MAX_DIV2            144
#define COL_MAX_DIV2            176

#define Y_CCIR_MAX		240
#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

/*
**		IMPORTED VARIABLES
*/

extern FILE *F_loss;
extern BOOLEAN STAT_MODE;
/*--------- from display.c --------------*/
extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;

/*
**		GLOBAL VARIABLES
*/

static int 		uv65[4] = {0, 32, 64, 96};
static int 		uv43[4] = {0, 8, 16, 24};
static int 		uv21[4] = {0, 2, 4, 6};
static int 		uv0[2] = {0, 1};

/* Transform the Cr,Cb videopix output to CCIR signals.
*  Cr, Cb outputs are twos complement values in [-63,63]
*  CCIR are unsigned values in [16, 240] with 128 <-> 0.
*/
static int COMPUV2CCIR[128]={
  128, 129, 131, 133, 134, 136, 138, 139, 141, 143,
  145, 146, 148, 150, 151, 153, 155, 156, 158, 160,
  162, 163, 165, 167, 168, 170, 172, 173, 175, 177,
  179, 180, 182, 184, 185, 187, 189, 190, 192, 194,
  196, 197, 199, 201, 202, 204, 206, 207, 209, 211,
  213, 214, 216, 218, 219, 221, 223, 224, 226, 228,
  230, 231, 233, 235, 19, 20, 22, 24, 26, 27,
  29, 31, 32, 34, 36, 37, 39, 41, 43, 44,
  46, 48, 49, 51, 53, 54, 56, 58, 60, 61,
  63, 65, 66, 68, 70, 71, 73, 75, 77, 78,
  80, 82, 83, 85, 87, 88, 90, 92, 94, 95,
  97, 99, 100, 102, 104, 105, 107, 109, 111, 112,
  114, 116, 117, 119, 121, 122, 124, 126
  };


static int YTOCCIR[128]={
  16, 17, 19, 21, 22, 24, 26, 28,
  29, 31, 33, 34, 36, 38, 40, 41,
  43, 45, 47, 48, 50, 52, 53, 55,
  57, 59, 60, 62, 64, 66, 67, 69,
  71, 72, 74, 76, 78, 79, 81, 83,
  84, 86, 88, 90, 91, 93, 95, 97,
  98, 100, 102, 103, 105, 107, 109, 110,
  112, 114, 116, 117, 119, 121, 122, 124,
  126, 128, 129, 131, 133, 134, 136, 138,
  140, 141, 143, 145, 147, 148, 150, 152,
  153, 155, 157, 159, 160, 162, 164, 166,
  167, 169, 171, 172, 174, 176, 178, 179,
  181, 183, 184, 186, 188, 190, 191, 193,
  195, 197, 198, 200, 202, 203, 205, 207,
  209, 210, 212, 214, 216, 217, 219, 221,
  222, 224, 226, 228, 229, 231, 233, 235
};

static int COMPY2CCIR[128]; /* Dynamic Y to CCIR transformation */

XtAppContext	appCtxt;
Widget		appShell;
Display 	*dpy;
XImage 		*ximage;
GC 		gc;
Window 		win;
XColor 		colors[256];
int 		depth;

XImage	*alloc_shared_ximage();
void 	destroy_shared_ximage();
void	build_image();

static int reset_ioctl = MEMPRST;
static int grab_ioctl = CAPTRCMD;


/* Grabbing procedures */


void vfc_get_cif_pal(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    static int limit = VFC_ESKIP_PAL >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r_data;


    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    /* Read in the odd field data */
    r_data = (u_char *)im_data;

    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<LIG_MAX; i++){

      /* 46 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j<348; j+=12){
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      }
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
    }
    bcopy((char *)(r_data-(CIF_WIDTH)), (char *)(r_data), 
	  CIF_WIDTH);
  }


void vfc_get_cif_color_pal(vfc_dev, Y_data, Cu_data, Cv_data)
    VfcDev *vfc_dev;
    u_char Y_data[][COL_MAX];
    u_char Cu_data[][COL_MAX_DIV2];
    u_char Cv_data[][COL_MAX_DIV2];
  {
    register int i, j, chrom_compt, lum_compt;
    register u_char *y_data, *cu_data, *cv_data;
    register u_int tmpdata, *data_port;
    register u_int pixel0, pixel1, pixel2, pixel3;
    register int cu, cv;
    static int grab_ioctl = CAPTRCMD;
    static int limit = VFC_ESKIP_PAL >> 4;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl);

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    /* Read in the odd field data */
    y_data =  (u_char *)Y_data;
    cu_data = (u_char *)Cu_data;
    cv_data = (u_char *)Cv_data;


    for(i=0; i < PAL_FIELD_HEIGHT; i++){
/*
** Drop the first three pixels
*/
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j < QCIF_WIDTH; j++){
/*
** 2 pixels are processed at each 176*2 = 352 pass.
*/

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;
/*
**  Chrominance components are used one line out of two.
*/
	if (!(i%2)){
	  *cu_data++ =	COMPUV2CCIR[
	     		  uv65[(pixel0 & GET_UBITS)>>22] | 
				    uv43[(pixel1 & GET_UBITS)>>22] |
	     		  uv21[(pixel2 & GET_UBITS)>>22] | 
				    uv0[(pixel3 & GET_UBITS)>>23]];
	  *cv_data++ =	COMPUV2CCIR[
             		  uv65[(pixel0 & GET_VBITS)>>20] | 
				    uv43[(pixel1 & GET_VBITS)>>20] |
	     		  uv21[(pixel2 & GET_VBITS)>>20] | 
				    uv0[(pixel3 & GET_VBITS)>>21]];
	}
	*y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];
	*y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel2)];
      }

/*
** 708 to 720 pixels are dropped.
*/
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
    } /* rof lecture ligne */
/*
** PAL_FIELD_HEIGHT = 287, CIF_HEIGHT = 288
*/
    bcopy ((y_data - CIF_WIDTH), y_data, CIF_WIDTH);
}


void vfc_get_cif4_pal(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][LIG_MAX][COL_MAX];
  {
    static int WIDTH = CIF_WIDTH >> 5;
    static int limit = VFC_ESKIP_PAL >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r0e, *r1e, *r2e, *r3e;
    register u_char *r0o, *r1o, *r2o, *r3o;


    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /* Even lines */
    r0e = (u_char *)im_data[0]; /* CIF 0, even line */
    r1e = (u_char *)im_data[1]; /* CIF 1, even line */
    r2e = (u_char *)im_data[2]; /* CIF 2, even line */
    r3e = (u_char *)im_data[3]; /* CIF 3, even line */
    /* Odd lines */
    r0o = r0e + CIF_WIDTH; /* CIF 0, odd line */
    r1o = r1e + CIF_WIDTH; /* CIF 1, odd line */ 
    r2o = r2e + CIF_WIDTH; /* CIF 2, odd line */
    r3o = r3e + CIF_WIDTH; /* CIF 3, odd line */

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }


    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<144; i++){
      
      /* 45 dropped pixels */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* CIF0 Image */

      for(j=0; j<168; j+=12){
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r0e += 2; r0o += 2;

	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r0e += 2; r0o += 2;

      }
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r0e += 2; r0o += 2;
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r0e += CIF_WIDTH;
      r0o += CIF_WIDTH;

      /* CIF1 Image */

      for(j=0; j<168; j+=12){
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r1e += 2; r1o += 2;

	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r1e += 2; r1o += 2;

      }
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r1e += 2; r1o += 2;
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r1e += CIF_WIDTH;
      r1o += CIF_WIDTH;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      
    }
    for(i=0; i<144; i++){
      
      /* 45 dropped pixels */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* CIF2 Image */

      for(j=0; j<168; j+=12){
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r2e += 2; r2o += 2;

	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r2e += 2; r2o += 2;

      }
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r2e += 2; r2o += 2;
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r2e += CIF_WIDTH;
      r2o += CIF_WIDTH;


      /* CIF3 Image */
      for(j=0; j<168; j+=12){
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r3e += 2; r3o += 2;

	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r3e += 2; r3o += 2;

      }
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r3e += 2; r3o += 2;
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r3e += CIF_WIDTH;
      r3o += CIF_WIDTH;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      
    }
  }


void vfc_get_cif4_color_pal (vfc_dev,  Y_data, Cu_data, Cv_data)
VfcDev *vfc_dev;
u_char Y_data[][LIG_MAX][COL_MAX];
u_char Cu_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
u_char Cv_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
{
  register int i, j;
  register u_char *y_data0,  *y_data1,  *y_data2,  *y_data3;
  register u_char *y_data01, *y_data11, *y_data21, *y_data31;
  register u_char 	*cu_data0, *cv_data0,
			*cu_data1, *cv_data1,
                        *cu_data2, *cv_data2,
			*cu_data3, *cv_data3;
  register u_int tmpdata, *data_port;
  register u_int pixel0, pixel1, pixel2, pixel3;
  register int cu, cv;
  static int grab_ioctl = CAPTRCMD;
  static int limit = VFC_ESKIP_PAL >> 4;

/*
** Grab the image
*/
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl);

    
    data_port = vfc_dev->vfc_port2;

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    y_data0 = (u_char *)Y_data[0]; /* CIF0 image pair */
    y_data1 = (u_char *)Y_data[1]; /* CIF1 image pair */
    y_data2 = (u_char *)Y_data[2]; /* CIF2 image pair */
    y_data3 = (u_char *)Y_data[3]; /* CIF3 image pair */

    y_data01 = y_data0 + COL_MAX; /* CIF0 image impair */
    y_data11 = y_data1 + COL_MAX; /* CIF1 image impair */
    y_data21 = y_data2 + COL_MAX; /* CIF2 image impair */
    y_data31 = y_data3 + COL_MAX; /* CIF3 image impair */

    cu_data0 = (u_char *)Cu_data[0]; /*  */
    cu_data1 = (u_char *)Cu_data[1]; /*  */
    cu_data2 = (u_char *)Cu_data[2]; /*  */
    cu_data3 = (u_char *)Cu_data[3]; /*  */

    cv_data0 = (u_char *)Cv_data[0]; /*  */
    cv_data1 = (u_char *)Cv_data[1]; /*  */
    cv_data2 = (u_char *)Cv_data[2]; /*  */
    cv_data3 = (u_char *)Cv_data[3]; /*  */

    /* looks gross but there is less time spent loop than reading */

    /* Read in the even/odd fields data in CIF0 & CIF1*/

/*
**  Lecture de la moitie superieure de l'image - construction de CIF1 et CIF2
*/

  for(i=0; i<LIG_MAX_DIV2; i++){

/*
** Drop the first three pixels
*/
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
/*
**  Moitie superieure gauche de l'image - construction de CIF1
*/

      for(j=0; j < QCIF_WIDTH_DIV2; j++){

/*
** lecture de quatre pixels a chaque passage 88*4 = 352 
** construction de huit luminances et deux couples de chrominances
*/

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;

	*cu_data0++ =	COMPUV2CCIR[
	     		  uv65[(pixel0 & GET_UBITS)>>22] | uv43[(pixel1 & GET_UBITS)>>22] |
	     		  uv21[(pixel2 & GET_UBITS)>>22] | uv0[(pixel3 & GET_UBITS)>>23]];
        *cu_data0++ = *(cu_data0 - 1);

	*cv_data0++ =	COMPUV2CCIR[
             		  uv65[(pixel0 & GET_VBITS)>>20] | uv43[(pixel1 & GET_VBITS)>>20] |
	     		  uv21[(pixel2 & GET_VBITS)>>20] | uv0[(pixel3 & GET_VBITS)>>21]];
        *cv_data0++ = *(cv_data0 - 1);

	*y_data0++ = *y_data01++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];
	*y_data0++ = *y_data01++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel1)];
	*y_data0++ = *y_data01++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel2)];
	*y_data0++ = *y_data01++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel3)];

      } /* rof j */
/*
**  Moitie superieure droite de l'image - construction de CIF2
*/

      for(j=0; j < QCIF_WIDTH_DIV2; j++){
/*
** lecture de quatre pixels a chaque passage 88*4 = 352 
** construction de huit luminances et deux couples de chrominances
*/

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;

	*cu_data1++ =	COMPUV2CCIR[
	     		  uv65[(pixel0 & GET_UBITS)>>22] | uv43[(pixel1 & GET_UBITS)>>22] |
	     		  uv21[(pixel2 & GET_UBITS)>>22] | uv0[(pixel3 & GET_UBITS)>>23]];
        *cu_data1++ = *(cu_data1 - 1);

	*cv_data1++ =	COMPUV2CCIR[
             		  uv65[(pixel0 & GET_VBITS)>>20] | uv43[(pixel1 & GET_VBITS)>>20] |
	     		  uv21[(pixel2 & GET_VBITS)>>20] | uv0[(pixel3 & GET_VBITS)>>21]];
        *cv_data1++ = *(cv_data1 - 1);

	*y_data1++ = *y_data11++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];
	*y_data1++ = *y_data11++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel1)];
	*y_data1++ = *y_data11++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel2)];
	*y_data1++ = *y_data11++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel3)];

      } /* rof j */

      y_data0  = y_data01;
      y_data01 += COL_MAX;
      y_data1  = y_data11;
      y_data11 += COL_MAX;


/*
** Elimination des pixels 708 a 720
*/
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

  } /* rof i */
/*
**  Lecture de la moitie inferieure de l'image - construction de CIF3 et CIF4
*/

  for(i=0; i<PAL_FIELD_HEIGHT_DIV2; i++){

/*
**  Drop the first three pixels
*/
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
/*
**  Moitie inferieure gauche de l'image - construction de CIF3
*/

      for(j=0; j < QCIF_WIDTH_DIV2; j++){
/*
** lecture de quatre pixels a chaque passage 88*4 = 352 
** construction de huit luminances et deux couples de chrominances
*/

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;

	*cu_data2++ =	COMPUV2CCIR[
	     		  uv65[(pixel0 & GET_UBITS)>>22] | uv43[(pixel1 & GET_UBITS)>>22] |
	     		  uv21[(pixel2 & GET_UBITS)>>22] | uv0[(pixel3 & GET_UBITS)>>23]];
        *cu_data2++ = *(cu_data2 - 1);

	*cv_data2++ =	COMPUV2CCIR[
             		  uv65[(pixel0 & GET_VBITS)>>20] | uv43[(pixel1 & GET_VBITS)>>20] |
	     		  uv21[(pixel2 & GET_VBITS)>>20] | uv0[(pixel3 & GET_VBITS)>>21]];
        *cv_data2++ = *(cv_data2 - 1);

	*y_data2++ = *y_data21++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];
	*y_data2++ = *y_data21++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel1)];
	*y_data2++ = *y_data21++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel2)];
	*y_data2++ = *y_data21++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel3)];

      } /* rof j */
/*
**  Moitie inferieure droite de l'image - construction de CIF4
*/

      for(j=0; j < QCIF_WIDTH_DIV2; j++){
/*
** lecture de quatre pixels a chaque passage 88*4 = 352 
** construction de huit luminances et deux couples de chrominances
*/

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;

	*cu_data3++ =	COMPUV2CCIR[
	     		  uv65[(pixel0 & GET_UBITS)>>22] | uv43[(pixel1 & GET_UBITS)>>22] |
	     		  uv21[(pixel2 & GET_UBITS)>>22] | uv0[(pixel3 & GET_UBITS)>>23]];
        *cu_data3++ = *(cu_data3 - 1);

	*cv_data3++ =	COMPUV2CCIR[
             		  uv65[(pixel0 & GET_VBITS)>>20] | uv43[(pixel1 & GET_VBITS)>>20] |
	     		  uv21[(pixel2 & GET_VBITS)>>20] | uv0[(pixel3 & GET_VBITS)>>21]];
        *cv_data3++ = *(cv_data3 - 1);

	*y_data3++ = *y_data31++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];
	*y_data3++ = *y_data31++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel1)];
	*y_data3++ = *y_data31++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel2)];
	*y_data3++ = *y_data31++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel3)];

      } /* rof j */

      y_data2  = y_data21;
      y_data21 += COL_MAX;
      y_data3  = y_data31;
      y_data31 += COL_MAX;


/*
** Elimination des pixels 708 a 720
*/
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

  } /* rof i */
/*
** PAL_FIELD_HEIGHT = 287, CIF_HEIGHT = 288
*/
    bcopy ((y_data2 - CIF_WIDTH), y_data2, CIF_WIDTH);
    bcopy ((y_data2 - CIF_WIDTH), y_data21, CIF_WIDTH);
    bcopy ((cu_data2 - QCIF_WIDTH), cu_data2, QCIF_WIDTH);
    bcopy ((cv_data2 - QCIF_WIDTH), cv_data2, QCIF_WIDTH);


    bcopy ((y_data3 - CIF_WIDTH), y_data3, CIF_WIDTH);
    bcopy ((y_data3 - CIF_WIDTH), y_data31, CIF_WIDTH);
    bcopy ((cu_data3 - QCIF_WIDTH), cu_data3, QCIF_WIDTH);
    bcopy ((cv_data3 - QCIF_WIDTH), cv_data3, QCIF_WIDTH);

}



void vfc_get_qcif_pal(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    static int WIDTH = CIF_WIDTH >> 5;
    static int limit = VFC_ESKIP_PAL >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r_data;


    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    for(i=0; i<LIG_MAX; i+=2){

      /* Useful QCIF line */

      r_data = (u_char *)&im_data[i>>1][0];

      /* 46 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j<348; j+=12){
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;

	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* Useless QCIF line */

      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j<704; j+=64){
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
    }
  }


void vfc_get_qcif_color_pal(vfc_dev, Y_data, Cu_data, Cv_data)
    VfcDev *vfc_dev;
    u_char Y_data[][COL_MAX];
    u_char Cu_data[][COL_MAX_DIV2];
    u_char Cv_data[][COL_MAX_DIV2];
{
  register int i, j;
  register u_char *y_data, *cu_data, *cv_data;
  register u_int tmpdata, *data_port;
  register u_int pixel0, pixel1, pixel2, pixel3;
  static int grab_ioctl = CAPTRCMD;
  static int VFC_YUV_WIDTH_DIV64 = VFC_YUV_WIDTH >> 6;
  static int WIDTH = QCIF_WIDTH >> 4;
  static int limit = VFC_ESKIP_PAL >> 4;
    
/*
* Local register copy of the data port
* improves performance
*/
  data_port = vfc_dev->vfc_port2;

/*
* Grab the image
*/
  ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
  ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl);


/*
**  Skip over the non-active samples
**  looks gross but there is less time spent loop than reading
*/
  for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
  }

/*
**  Read in the odd field data
*/
  y_data =  (u_char *)Y_data;
  cu_data = (u_char *)Cu_data;
  cv_data = (u_char *)Cv_data;
/*
**  144 lines to be read
**  Read first 143 lines : 0 --> 142
*/	

  for( i = 0; i < PAL_FIELD_HEIGHT_DIV2; i++){
/*
** Drop the three first pixels
*/
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;

    for(j=0; j < QCIF_WIDTH ; j++){

	pixel0 = *data_port;
	pixel1 = *data_port;
	pixel2 = *data_port;
	pixel3 = *data_port;

	if (!(i%2) && !(j%2)){ 
        *cu_data++ = COMPUV2CCIR[
				 uv65[(pixel0 & GET_UBITS)>>22] | 
				 uv43[(pixel1 & GET_UBITS)>>22] |
				 uv21[(pixel2 & GET_UBITS)>>22] | 
				 uv0[(pixel3 & GET_UBITS)>>23]];
	*cv_data++ = COMPUV2CCIR[
				 uv65[(pixel0 & GET_VBITS)>>20] | 
				 uv43[(pixel1 & GET_VBITS)>>20] |
				 uv21[(pixel2 & GET_VBITS)>>20] | 
				 uv0[(pixel3 & GET_VBITS)>>21]];
      }
      *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(pixel0)];

    } /* rof lecture colonne */

/*
**  Skip over the end of the line : pixels 708 to 720.
*/
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;

    y_data += QCIF_WIDTH;
    if (!(i%2)) {
      cu_data += QCIF_WIDTH_DIV2;
      cv_data += QCIF_WIDTH_DIV2;
    }
/*
**  Skip over the next line - 720 pixels = VFC_YUV_WIDTH_DIV64 * 64 + 16
*/

    for ( j = 0; j < VFC_YUV_WIDTH_DIV64; j++) {

     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
/*16*/
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
/*32*/
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
/*48*/
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
     tmpdata = *data_port;
    }
/*
**  Plus le reste de la division 720/64 soit 16
*/

    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;

  } /* rof lecture ligne */
/*
** Read the 144th line : no 143 ==> pas d'analyse de chrominance
**
** Drop the three first pixels
*/
  tmpdata = *data_port;
  tmpdata = *data_port;
  tmpdata = *data_port;

  for(j=0; j<WIDTH; j++){
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
    tmpdata = *data_port;
    tmpdata = *data_port;
    tmpdata = *data_port;
    *y_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

  } /* rof ligne 143 */

}



void vfc_get_cif_ntsc(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    static int WIDTH = CIF_WIDTH >> 5;
    static int limit = VFC_OSKIP_NTSC >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r_data;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    /* Read in the odd field data */
    r_data = (u_char *)im_data;

    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<240; i++){

      /* 46 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j<348; j+=12){
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      }
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      if((i%5==0)){
	/* The previous line is duplicated */
	bcopy((char *)(r_data-CIF_WIDTH), (char *)r_data, CIF_WIDTH);
	r_data += CIF_WIDTH;
      }
    }
    /* The last line is not entirely valid */
    bcopy((char *)(r_data-(2*CIF_WIDTH)), (char *)(r_data-CIF_WIDTH), 
	  CIF_WIDTH);
  }



void vfc_get_cif4_ntsc(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][LIG_MAX][COL_MAX];
  {
    static int WIDTH = CIF_WIDTH >> 5;
    static int limit = VFC_OSKIP_NTSC >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r0e, *r1e, *r2e, *r3e;
    register u_char *r0o, *r1o, *r2o, *r3o;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /* Even lines */
    r0e = (u_char *)im_data[0]; /* CIF 0, even line */
    r1e = (u_char *)im_data[1]; /* CIF 1, even line */
    r2e = (u_char *)im_data[2]; /* CIF 2, even line */
    r3e = (u_char *)im_data[3]; /* CIF 3, even line */
    /* Odd lines */
    r0o = r0e + CIF_WIDTH; /* CIF 0, odd line */
    r1o = r1e + CIF_WIDTH; /* CIF 1, odd line */ 
    r2o = r2e + CIF_WIDTH; /* CIF 2, odd line */
    r3o = r3e + CIF_WIDTH; /* CIF 3, odd line */

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }


    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<120; i++){
      
      /* 45 dropped pixels */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* CIF0 Image */

      for(j=0; j<168; j+=12){
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r0e += 2; r0o += 2;

	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r0e += 2; r0o += 2;

      }
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e = *(r0e+1) = *r0o = *(r0o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r0e += 2; r0o += 2;
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r0e++ = *r0o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r0e += CIF_WIDTH;
      r0o += CIF_WIDTH;

      /* CIF1 Image */

      for(j=0; j<168; j+=12){
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r1e += 2; r1o += 2;

	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r1e += 2; r1o += 2;

      }
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e = *(r1e+1) = *r1o = *(r1o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r1e += 2; r1o += 2;
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r1e++ = *r1o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r1e += CIF_WIDTH;
      r1o += CIF_WIDTH;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      if((i%5) & 0x01){
	/* The previous line is duplicated */
	bcopy((char *)(r0o-(2*CIF_WIDTH)), (char *)r0e, CIF_WIDTH);
	bcopy((char *)(r1o-(2*CIF_WIDTH)), (char *)r1e, CIF_WIDTH);
	r0o += CIF_WIDTH;
	r1o += CIF_WIDTH;
	r0e += CIF_WIDTH;
	r1e += CIF_WIDTH;
      }
    }
    for(i=0; i<120; i++){
      
      /* 45 dropped pixels */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* CIF2 Image */

      for(j=0; j<168; j+=12){
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r2e += 2; r2o += 2;

	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r2e += 2; r2o += 2;

      }
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e = *(r2e+1) = *r2o = *(r2o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r2e += 2; r2o += 2;
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r2e++ = *r2o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r2e += CIF_WIDTH;
      r2o += CIF_WIDTH;


      /* CIF3 Image */
      for(j=0; j<168; j+=12){
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r3e += 2; r3o += 2;

	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	*r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	r3e += 2; r3o += 2;

      }
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e = *(r3e+1) = *r3o = *(r3o+1) =
	                  COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      r3e += 2; r3o += 2;
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      *r3e++ = *r3o++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];

      r3e += CIF_WIDTH;
      r3o += CIF_WIDTH;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      
      if((i%5) & 0x01){
	/* The previous line is duplicated */
	bcopy((char *)(r2o-(2*CIF_WIDTH)), (char *)r2e, CIF_WIDTH);
	bcopy((char *)(r3o-(2*CIF_WIDTH)), (char *)r3e, CIF_WIDTH);
	r2o += CIF_WIDTH;
	r3o += CIF_WIDTH;
	r2e += CIF_WIDTH;
	r3e += CIF_WIDTH;
      }
    }
  }



void vfc_get_qcif_ntsc(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    static int WIDTH = CIF_WIDTH >> 5;
    static int limit = VFC_OSKIP_NTSC >> 4;
    register int i, j;
    register u_int tmpdata, *data_port;
    register u_char *r_data = &im_data[0][0];


    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); 

    /* Skip over the non-active samples */
    /* looks gross but there is less time spent loop than reading */
    for(i=0; i<limit; i++){
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;	
    }

    for(i=0; i<240; i+=2){
      
      /* Useful QCIF line */
      
      /* 46 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      for(j=0; j<348; j+=12){
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;

	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	*r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      *r_data++ = COMPY2CCIR[VFC_PREVIEW_Y0(*data_port)];
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* 29 pixels dropped */
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;

      /* Useless QCIF line */

      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      for(j=0; j<704; j+=64){
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
      r_data += QCIF_WIDTH;
      if(i%10==0){
	/* The previous line is duplicated */
	bcopy((char *)(r_data-CIF_WIDTH), (char *)r_data, QCIF_WIDTH);
	r_data += CIF_WIDTH;
      }
    }
  }


int VfcScanPorts(vfc_dev, format)
     VfcDev  *vfc_dev;
     int     *format;
{
  int in_port;

  for(in_port=VFC_PORT1; in_port<VFC_SVIDEO; in_port++){
    vfc_set_port(vfc_dev, in_port);
    if(vfc_set_format(vfc_dev, VFC_AUTO, format) >= 0)
      return(in_port);
  }
  /* in_port == VFC_SVIDEO */
  vfc_set_port(vfc_dev, in_port);
  if(vfc_set_format(vfc_dev, VFC_AUTO, format) >= 0){
    *format = VFC_NTSC;
    return(in_port);
  }else{
    /* No video available */
    return(-1);
  }
}



void Initialize(vfc_format, size, COLOR, NG_MAX, L_lig, L_col, Lcoldiv2,
		L_col2, L_lig2, get_image)
     int vfc_format, size;
     BOOLEAN COLOR;
     int *NG_MAX, *L_lig, *L_col, *Lcoldiv2, *L_col2, *L_lig2;
     void (**get_image)();
{
  if(size == QCIF_SIZE){
    *NG_MAX = 5;
    *L_lig = 144;
    *L_col = 176;
  }else{
    *NG_MAX = 12;
    *L_lig = CIF_HEIGHT;
    *L_col = CIF_WIDTH;
  }

  *Lcoldiv2 = *L_col >> 1;
  *L_col2 = *L_col << 1;
  *L_lig2 = *L_lig << 1;

  if(!COLOR){
    encCOLOR = FALSE;
    affCOLOR = MODGREY;
    switch(size){
    case CIF4_SIZE: *get_image = (vfc_format == VFC_NTSC) ? 
      vfc_get_cif4_ntsc : vfc_get_cif4_pal;
      break;
    case CIF_SIZE:
      *get_image = (vfc_format == VFC_NTSC) ? vfc_get_cif_ntsc :
	vfc_get_cif_pal;
      break;
    case QCIF_SIZE:
      *get_image = (vfc_format == VFC_NTSC) ? vfc_get_qcif_ntsc :
	vfc_get_qcif_pal;
    }
  }else{
    encCOLOR = TRUE;
    affCOLOR = MODCOLOR;
    switch(size){
    case CIF4_SIZE: 
      *get_image = (vfc_format == VFC_NTSC) ? vfc_get_cif4_ntsc : 
	vfc_get_cif4_color_pal;
      break;
    case CIF_SIZE:
      *get_image = (vfc_format == VFC_NTSC) ? vfc_get_cif_ntsc :
	vfc_get_cif_color_pal;
      break;
    case QCIF_SIZE:
      *get_image = (vfc_format == VFC_NTSC) ? vfc_get_qcif_ntsc :
	vfc_get_qcif_color_pal;
    }
  }
}


void InitCOMPY2CCIR(tab, brightness, contrast)
     int *tab;
     int brightness, contrast;
{
  register int i;
  int aux;
  double dbrightness, dcontrast;

  /*
  *  Initialize COMPY2CCIR table according currents brightness
  *  and contrast parameters.
  */
  dbrightness = 2.0*(double)brightness - 100.0;
  if(contrast < 50){
    dcontrast = 0.012*(double)contrast + 0.4;
  }else{
    dcontrast = 0.06*(double)contrast - 2.0;
  }
  for(i=0; i<128; i++){
    aux = ((int)(((double)YTOCCIR[i]+ dbrightness)*dcontrast));
    tab[i] = bornCCIR(aux);
  }
}  



void sun_vfc_video_encode(group, port, ttl, size, fd_f2s, fd_s2f, COLOR, STAT, 
		  FREEZE, LOCAL_DISPLAY, FEEDBACK, display, 
		  rate_control, rate_max, vfc_port, vfc_format, vfc_dev, 
		  brightness, contrast, default_quantizer)
     char *group, *port;      /* Address socket */
     u_char ttl;              /* ttl value */
     int size;                /* image's size */
     int fd_f2s[2];           /* pipe descriptor father to son */
     int fd_s2f[2];           /* pipe descriptor son to father*/
     BOOLEAN COLOR;           /* True if color encoding */
     BOOLEAN STAT;            /* True if statistical mode */
     BOOLEAN FREEZE;          /* True if image frozen */
     BOOLEAN LOCAL_DISPLAY;   /* True if local display selected */
     BOOLEAN FEEDBACK;        /* True if feedback allowed */
     char *display;           /* Display name */
     int rate_control;        /* WITHOUT_CONTROL or REFRESH_CONTROL 
				 or QUALITY_CONTROL */
     int rate_max;            /* Maximum bandwidth allowed (kb/s) */
     int vfc_port;            /* VFC input port selected */
     int vfc_format;          /* VFC_PAL or VFC_NTSC */
     VfcDev *vfc_dev;         /* VFC device (already opened) */
     int brightness, contrast;
     int default_quantizer;

{
    extern int n_mba, n_tcoeff, n_cbc, n_dvm;
    extern int tab_rec1[][255], tab_rec2[][255]; /* for quantization */
    IMAGE_CIF image_coeff[4];
    u_char new_Y[4][LIG_MAX][COL_MAX];
    u_char new_Cb[4][LIG_MAX_DIV2][COL_MAX_DIV2];
    u_char new_Cr[4][LIG_MAX_DIV2][COL_MAX_DIV2];
    int NG_MAX;
    int sock_recv=0;
    BOOLEAN FIRST=TRUE; /* True if it is the first image */
    BOOLEAN CONTINUE=TRUE;
    BOOLEAN NEW_LOCAL_DISPLAY, NEW_SIZE, TUNING;
    BOOLEAN DOUBLE = FALSE ;
    int i;
    int new_size, new_vfc_port;
    void (*get_image)();
    int L_lig, L_col, L_lig2, L_col2, Lcoldiv2 ;

    XEvent	evt_mouse;
#ifdef INPUT_FILE
    FILE *F_input;
    extern int n_rate;
    extern double dump_rate;
#endif

    /* Video control socket declaration */
    sock_recv=declare_listen(PORT_VIDEO_CONTROL);
    if(STAT){
      if((F_loss=fopen(".videoconf.loss", "w")) == NULL){
	fprintf(stderr, "cannot create videoconf.loss file\n");
	STAT_MODE=FALSE;
      }else
	STAT_MODE=TRUE;
    }
#ifdef INPUT_FILE
    if((F_input=fopen("input_file", "r")) == NULL){
      fprintf(stderr, "cannot open input_file\n");
      exit(0);
    }
#endif
    if(vfc_format == VFC_NTSC) 
      COLOR = FALSE ;
    Initialize(vfc_format, size, COLOR, &NG_MAX, &L_lig, &L_col, &Lcoldiv2,
	       &L_col2, &L_lig2, &get_image);

    InitCOMPY2CCIR(COMPY2CCIR, brightness, contrast);

    if(LOCAL_DISPLAY){
      if(size == CIF4_SIZE){
	init_window(L_lig2, L_col2, "Local display", display, &appCtxt, 
		    &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
      }else{
	init_window(L_lig, L_col, "Local display", display, &appCtxt, 
		    &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
        if (affCOLOR == MODNB) DOUBLE = TRUE ;
#ifdef BW_DEBUG
  fprintf(stderr, "VIDEOSPARC_VFC.C-video_encode() : retour init_window() - depth = %d, DOUBLE = %d\n", depth, DOUBLE) ;
  fflush(stderr) ;
#endif
      }
    }

    do{
      if(size != CIF4_SIZE){
#ifdef INPUT_FILE
	if(size == CIF_SIZE){
	  if(fread(new_Y[0], 1, L_lig*L_col, F_input) <= 0)
	    break;
	}else{
	  register int l;
	  for(l=0; l<L_lig; l++)
	    if(fread(&new_Y[0][l][0], 1, L_col, F_input) <= 0)
	      break;
	}
#else
	if(!FREEZE)
          if (COLOR)
	    (*get_image)(vfc_dev, new_Y[0], new_Cb[0], new_Cr[0] );
          else
	    (*get_image)(vfc_dev, new_Y[0]);
#endif
        encode_h261(group, port, ttl, new_Y[0], new_Cb[0], new_Cr[0],
		    image_coeff[0], NG_MAX, sock_recv, FIRST, COLOR, 
		    FEEDBACK, rate_control, rate_max, default_quantizer,
		    fd_s2f[1]);

	if(LOCAL_DISPLAY){

          build_image((u_char *) &new_Y[0][0][0], 
                      (u_char *) &new_Cb[0][0][0], 
                      (u_char *) &new_Cr[0][0][0], 
                      DOUBLE, ximage, 
                      COL_MAX, 0, L_col, L_lig ) ;


	  show(dpy, ximage, ximage->width, ximage->height) ;
	}

      } else { 
/*
** Super CIF image encoding 
*/
        if(!FREEZE)
          if (COLOR)
	    (*get_image)(vfc_dev, new_Y, new_Cb, new_Cr);
          else
	    (*get_image)(vfc_dev, new_Y);

	for(i=0; i<4; i++)
	  encode_h261_cif4(group, port, ttl, new_Y[i], new_Cb[i], 
			     new_Cr[i], (GOB *)image_coeff[i], sock_recv, 
			     FIRST, COLOR, FEEDBACK, i, rate_control, 
			     rate_max, default_quantizer, fd_s2f[1]);

        if(LOCAL_DISPLAY){

          build_image((u_char *) &new_Y[0][0][0], 
                      (u_char *) &new_Cb[0][0][0], 
                      (u_char *) &new_Cr[0][0][0], 
                      FALSE, ximage, 
                      COL_MAX, 0, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[1][0][0], 
                      (u_char *) &new_Cb[1][0][0], 
                      (u_char *) &new_Cr[1][0][0], 
                      FALSE, ximage, 
                      COL_MAX, L_col, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[2][0][0], 
                      (u_char *) &new_Cb[2][0][0], 
                      (u_char *) &new_Cr[2][0][0], 
                      FALSE, ximage, 
                      COL_MAX, L_lig*L_col2, L_col, L_lig ) ;

          build_image((u_char *) &new_Y[3][0][0], 
                      (u_char *) &new_Cb[3][0][0], 
                      (u_char *) &new_Cr[3][0][0], 
                      FALSE, ximage, 
                      COL_MAX, (L_lig*L_col2 + L_col), L_col, L_lig ) ;

	  show(dpy, ximage, L_col2, L_lig2);
        } /* if(local display) */
      } /* else{} Super CIF encoding */

      FIRST = FALSE;
      new_size = size;
      new_vfc_port = vfc_port;
      NEW_LOCAL_DISPLAY = LOCAL_DISPLAY;
      NEW_SIZE = FALSE;
      if(CONTINUE = HandleCommands(fd_f2s, &new_size, &rate_control, 
				   &new_vfc_port, &rate_max, &FREEZE, 
				   &NEW_LOCAL_DISPLAY, &TUNING, &FEEDBACK,
				   &brightness, &contrast)){
	if(new_vfc_port != vfc_port){
	  vfc_port = new_vfc_port;
	  vfc_set_port(vfc_dev, vfc_port);
	}

	if(new_size != size){
          register int w , h ;

	  NEW_SIZE = TRUE;
	  FIRST = TRUE;
	  size = new_size;
	  Initialize(vfc_format, size, COLOR, &NG_MAX, &L_lig, &L_col, 
		     &Lcoldiv2, &L_col2, &L_lig2, &get_image);
          w = (size == CIF4_SIZE) ? L_col2 : L_col ;
          h = (size == CIF4_SIZE) ? L_lig2 : L_lig ;
          if (CONTEXTEXIST) {
            destroy_shared_ximage (dpy, ximage) ;
            ximage = alloc_shared_ximage (dpy, w, h, depth );
            XtResizeWidget (appShell, w, h, 0 ) ;
          }
        }

	if( NEW_LOCAL_DISPLAY != LOCAL_DISPLAY ){
	  LOCAL_DISPLAY = NEW_LOCAL_DISPLAY;
	  if(LOCAL_DISPLAY){
            if (CONTEXTEXIST) XtMapWidget(appShell);
            else
	      if(size == CIF4_SIZE){
	        init_window(L_lig2, L_col2, "Local display", display, 
			    &appCtxt, &appShell, &depth, &dpy, &win, &gc, 
			    &ximage, FALSE);
	      }else{
	        init_window(L_lig, L_col, "Local display", display, &appCtxt, 
			  &appShell, &depth, &dpy, &win, &gc, &ximage, FALSE);
                if (affCOLOR == MODNB) DOUBLE = TRUE ;
#ifdef BW_DEBUG
  fprintf(stderr, "VIDEOSPARC_VFC.C-video_encode() : retour init_window() - depth = %d, DOUBLE = %d\n", depth, DOUBLE) ;
  fflush(stderr) ;
#endif
	      }
	  }
	  else
            if(CONTEXTEXIST){
              XtUnmapWidget(appShell);
              while ( XtAppPending (appCtxt))
                XtAppProcessEvent (appCtxt, XtIMAll);	
            }
	}

	if(TUNING){
	  InitCOMPY2CCIR(COMPY2CCIR, brightness, contrast);
	  TUNING=FALSE;
	}
      }
/******************
** looking for X events on win
*/
      if (LOCAL_DISPLAY) {
        while ( XtAppPending (appCtxt) )
          XtAppProcessEvent (appCtxt, XtIMAll ) ;	
      } /* fi LOCAL_DISPLAY */

    }while(CONTINUE);

    /* Clean up and exit */
    close(fd_f2s[0]);
    close(fd_s2f[1]);
    if(LOCAL_DISPLAY)
      XCloseDisplay(dpy);
    if(STAT_MODE)
      fclose(F_loss);
    vfc_destroy(vfc_dev);
#ifdef INPUT_FILE
    fprintf(stderr, "average rate: %fkbps\n", dump_rate/(double)n_rate);
#endif
    exit(0);
    /* NOT REACHED */
}

#endif /* VIDEOPIX */


