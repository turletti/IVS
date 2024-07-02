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
*  File              : grab_vfc.c                                          *
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
#define GRAB_VFC
#include "h261.h"
#include "protocol.h"
#include <vfc_lib.h>
#include <vfc_ioctls.h>
#include <sys/time.h>

#include "grab_vfc.h"
#include "vfc_macro.h"

/*------------------------------------------------------------------*/

typedef union{
  double vrd;
  struct{
  u_int w0;
  u_int w1;
} word;
} TYPE_A;

static int 		uv65[4] = {0, 32, 64, 96};
static int 		uv43[4] = {0, 8, 16, 24};
static int 		uv21[4] = {0, 2, 4, 6};
static int 		uv0[2] = {0, 1};

int stop_wait = 0;
/*--------- from display.c --------------*/
extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;

/*------------------------------------------------------------------*/
/*                Grabbing procedures                               */
/*------------------------------------------------------------------*/
 


/*------------------------------------------------------------------*/
void grab_null_fb(zptr, y_data)
/*------------------------------------------------------------------*/
     u_char *zptr;
      u_char *y_data;
{
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register TYPE_A  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  

  data_port = (TYPE_A *) zptr;
 
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){
    
    /* 35 pixels dropped */
    SKIP_TMP1(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    
    j=11;
    do{
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP4(data_port);
    SKIP_TMP2(data_port);
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);

  }

  }

/*------------------------------------------------------------------*/
void grab_null(vfc_dev, y_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
      u_char *y_data;
{
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register TYPE_A  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  

  data_port = (TYPE_A *) vfc_dev->vfc_port2;
 
  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){
    
    /* 35 pixels dropped */
    SKIP_TMP1(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    
    j=11;
    do{
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP4(data_port);
    SKIP_TMP2(data_port);
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);

  }
  START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_cif_pal(vfc_dev, y_data)
/*------------------------------------------------------------------*/
     VfcDev *vfc_dev;
     register u_char *y_data;
{
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register TYPE_A  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  
  data_port = (TYPE_A *) vfc_dev->vfc_port2;
 
  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){
    
    /* 35 pixels dropped */
    SKIP_TMP1(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    
    j=11;
    do{
      
      RD8_Y_CIF_PAL_0(data_port, 0);
      RD8_Y_CIF_PAL_1(data_port, 8);
      RD8_Y_CIF_PAL_2(data_port, 16);
      RD8_Y_CIF_PAL_3(data_port, 24);
      y_data += 32;
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);

  }
  j=11; ptr = (TYPE_A *)(y_data-(CIF_WIDTH));
  do{
    R_COPY16(ptr, y_data, 0);
    R_COPY16(ptr, y_data, 16);
    y_data += 32; ptr+= 32; 
  }while(--j);
  
  START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_cif_color_pal(vfc_dev, y_data, u_data, v_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    register u_char *y_data, *u_data, *v_data;
  {
    register int i, j;
    register TYPE_A tmpdata;
    register u_int u, v;
    register TYPE_A  y, cu, cv;
    register TYPE_A *ptr;
    TYPE_A *data_port;
   register u_char *y2ccir = &COMPY2CCIR[0];
   register u_char *cuv2ccir = &COMPUV2CCIR[0];


    data_port = (TYPE_A *)vfc_dev->vfc_port2;

  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){

    /* 35 pixels dropped */

    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP1(data_port);

    j=11;

      if(!(i&1)){
	do{
	  CRD8_Y_CIF_PAL_0(data_port, 0);
	  CRD8_Y_CIF_PAL_1(data_port, 8);
	  CRD8_Y_CIF_PAL_2(data_port, 16);
	  CRD8_Y_CIF_PAL_3(data_port, 24);
	  u_data+=16; v_data+=16;
	  y_data += 32;
	}while(--j);
      }else
	do{
	  RD8_Y_CIF_PAL_0(data_port, 0);
	  RD8_Y_CIF_PAL_1(data_port, 8);
	  RD8_Y_CIF_PAL_2(data_port, 16);
	  RD8_Y_CIF_PAL_3(data_port, 24);
	  y_data += 32;
	}while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);

  }
  j=11; ptr = (TYPE_A *)(y_data-(CIF_WIDTH));
  do{
    R_COPY16(ptr, y_data, 0);
    R_COPY16(ptr, y_data, 16);
    y_data += 32; ptr+= 32; 
  }while(--j);
  
  START_NEXT_GRAB
  }
  
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
void Avfc_get_cif_color_pal(vfc_dev, Y_data, Cu_data, Cv_data)
/*------------------------------------------------------------------*/
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

    static int limit = VFC_ESKIP_PAL >> 4;

    data_port = vfc_dev->vfc_port2;

    WAIT_GRAB_END

    /* Skip over the non-active samples */

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

    START_NEXT_GRAB

}

/*------------------------------------------------------------------*/
void vfc_get_cif4_pal(vfc_dev, im_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    u_char im_data[][LIG_MAX][COL_MAX];
  {
    register int i, j;
    register TYPE_A tmpdata, *data_port2;
    register u_char *y0, *y1;
    register TYPE_A y;
    register u_char *y2ccir = &COMPY2CCIR[0];
    register int image;

    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)im_data[0]; /* CIF 0 */
    y1 = (u_char *)im_data[1]; /* CIF 1 */

    WAIT_GRAB_END

    /* Skip over the non-active samples */
    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port2);
    }

    image = 2;
    i=LIG_MAX/2+1;
    do{

    for(; --i; ){

    /* 35 pixels dropped */
    SKIP_TMP1(data_port2);
    SKIP_TMP2(data_port2);
    SKIP_TMP16(data_port2);
    SKIP_TMP16(data_port2);
    
    j=11;
    do{
      GET30Y32(data_port2, y0, 0);
      y0 += 32;
    }while(--j);

    j=11;
    do{
      GET30Y32(data_port2, y1, 0);
      y1+=32;
    }while(--j);
    /* 25 pixels dropped */
    SKIP_TMP16(data_port2);
    SKIP_TMP8(data_port2);
    SKIP_TMP1(data_port2);

      j=11;
      do{
	R_COPY16((y0-(CIF_WIDTH)), y0, 0);
	R_COPY16((y0-(CIF_WIDTH)), y0, 16);
	R_COPY16((y1-(CIF_WIDTH)), y1, 0);
	R_COPY16((y1-(CIF_WIDTH)), y1, 16);
	y0 += 32;
	y1 += 32;
      }while(--j);
  }
    if(!(--image))
      break;
      y0 = (u_char *)im_data[2]; /* CIF 2 */
      y1 = (u_char *)im_data[3]; /* CIF 3 */
      
      i= LIG_MAX/2;
  }while(1);
    {
      register u_char *ptr0;
      register u_char *ptr1;
      ptr0 = (y0-(CIF_WIDTH));
      ptr1 = (y1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, y0, 0);
	R_COPY16(ptr0, y0, 16);
	R_COPY16(ptr0, y0+COL_MAX, 0);
	R_COPY16(ptr0, y0+COL_MAX, 16);

	R_COPY16(ptr1, y1, 0);
	R_COPY16(ptr1, y1, 16);
	R_COPY16(ptr1, y1+COL_MAX, 0);
	R_COPY16(ptr1, y1+COL_MAX, 16);
	y0 += 32; ptr0+= 32; 
	y1 += 32; ptr1+= 32; 
      }while(--j);
    }
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void Bvfc_get_cif4_pal(vfc_dev, im_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    u_char im_data[][LIG_MAX][COL_MAX];
  {
    register int i, j;
    register TYPE_A tmpdata, *data_port1, *data_port2;
    register u_char *y0, *y1;
    register TYPE_A y;
    register u_char *y2ccir = &COMPY2CCIR[0];
    register int image;

    data_port1 = (TYPE_A *)vfc_dev->vfc_port1;
    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)im_data[0]; /* CIF 0 */
    y1 = (u_char *)im_data[1]; /* CIF 1 */

    WAIT_GRAB_END

    /* Skip over the non-active samples */
    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port1);
      SKIP_TMP16(data_port2);
    }

    image = 2;
    i=LIG_MAX/2+1;
    do{

    for(; --i; y0 += COL_MAX, y1 += COL_MAX){

    /* 35 pixels dropped */
    SKIP_TMP1(data_port1);
    SKIP_TMP2(data_port1);
    SKIP_TMP16(data_port1);
    SKIP_TMP16(data_port1);
    /* 35 pixels dropped */
    SKIP_TMP1(data_port2);
    SKIP_TMP2(data_port2);
    SKIP_TMP16(data_port2);
    SKIP_TMP16(data_port2);
    
    j=11;
    do{
      GET30Y32(data_port1, y0, 0);
      GET30Y32(data_port2, y0+COL_MAX, 0);
      y0 += 32;
    }while(--j);

    j=11;
    do{
      GET30Y32(data_port1, y1, 0);
      GET30Y32(data_port2, y1+COL_MAX, 0);
      y1+=32;
    }while(--j);
    /* 25 pixels dropped */
    SKIP_TMP16(data_port1);
    SKIP_TMP8(data_port1);
    SKIP_TMP1(data_port1);
    /* 25 pixels dropped */
    SKIP_TMP16(data_port2);
    SKIP_TMP8(data_port2);
    SKIP_TMP1(data_port2);
  }
    if(!(--image))
      break;
      y0 = (u_char *)im_data[2]; /* CIF 2 */
      y1 = (u_char *)im_data[3]; /* CIF 3 */
      
      i= LIG_MAX/2;
  }while(1);
    {
      register u_char *ptr0;
      register u_char *ptr1;
      ptr0 = (y0-(CIF_WIDTH));
      ptr1 = (y1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, y0, 0);
	R_COPY16(ptr0, y0, 16);
	R_COPY16(ptr0, y0+COL_MAX, 0);
	R_COPY16(ptr0, y0+COL_MAX, 16);

	R_COPY16(ptr1, y1, 0);
	R_COPY16(ptr1, y1, 16);
	R_COPY16(ptr1, y1+COL_MAX, 0);
	R_COPY16(ptr1, y1+COL_MAX, 16);
	y0 += 32; ptr0+= 32; 
	y1 += 32; ptr1+= 32; 
      }while(--j);
    }
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void Avfc_get_cif4_pal(vfc_dev, im_data)
/*------------------------------------------------------------------*/
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
    WAIT_GRAB_END

    /* Skip over the non-active samples */
    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
       tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
      tmpdata = *data_port;
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
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_cif4_color_pal (vfc_dev,  Y_data, Cu_data, Cv_data)
/*------------------------------------------------------------------*/
VfcDev *vfc_dev;
u_char Y_data[][LIG_MAX][COL_MAX];
u_char Cu_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
u_char Cv_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
{
  register int i, j;
  register u_char *y0,  *y1;
  register u_char *u0, *v0;
  register u_char *u1, *v1;
  register u_int u, v;
  register TYPE_A  tmpdata, y, cu, cv;
  TYPE_A  *data_port2;
  register u_char *y2ccir = &COMPY2CCIR[0];
  register u_char *cuv2ccir = &COMPUV2CCIR[0];
  register int image;

#define D COL_MAX
#define DD COL_MAX_DIV2

    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)Y_data[0]; /* CIF 0 */
    y1 = (u_char *)Y_data[1]; /* CIF 1 */

    u0 = (u_char *)Cu_data[0]; /* CIF 0 */
    u1 = (u_char *)Cu_data[1]; /* CIF 1 */

    v0 = (u_char *)Cv_data[0]; /* CIF 0 */
    v1 = (u_char *)Cv_data[1]; /* CIF 1 */



    WAIT_GRAB_END
    /* Skip over the non-active samples */
    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port2);
    }

    image = 2;
    i=LIG_MAX/2+1;
    do{

    for(; --i;){

    /* 35 pixels dropped */
    SKIP_TMP1(data_port2);
    SKIP_TMP2(data_port2);
    SKIP_TMP16(data_port2);
    SKIP_TMP16(data_port2);


    colorGET30Y32(data_port2, y0, u0, v0, 0);
      y0 += 32; u0+=16; v0+=16;

    j=5;
    do{
      colorGET30Y32S(data_port2, y0, u0, v0, 0);
      colorGET30Y32(data_port2, y0, u0, v0, 32);

      y0 += 64;  u0+=32; v0+=32;
    }while(--j);

    colorGET30Y32S(data_port2, y1, u1, v1,0);

      y1 += 32; u1+=16; v1+=16;

    j=5;
    do{

    colorGET30Y32(data_port2, y1, u1, v1, 0);

    colorGET30Y32S(data_port2, y1, u1, v1, 32);


      y1 += 64;  u1+=32; v1+=32;
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port2);
    SKIP_TMP8(data_port2);
    SKIP_TMP1(data_port2);

      j=11;
      do{
	R_COPY16((y0-(CIF_WIDTH)), y0, 0);
	R_COPY16((y0-(CIF_WIDTH)), y0, 16);
	R_COPY16((y1-(CIF_WIDTH)), y1, 0);
	R_COPY16((y1-(CIF_WIDTH)), y1, 16);
	y0 += 32;
	y1 += 32;
      }while(--j);

  }
      if(!(--image))
      break;
     y0 = (u_char *)Y_data[2]; /* CIF 2 */
     y1 = (u_char *)Y_data[3]; /* CIF 3 */

     u0 = (u_char *)Cu_data[2]; /* CIF 2 */
     u1 = (u_char *)Cu_data[3]; /* CIF 3 */

     v0 = (u_char *)Cv_data[2]; /* CIF 2 */
     v1 = (u_char *)Cv_data[3]; /* CIF 3 */
      
      i= LIG_MAX/2;
  }while(1);
    {
      register u_char *ptr0;
      register u_char *ptr1;
      ptr0 = (y0-(CIF_WIDTH));
      ptr1 = (y1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, y0, 0);
	R_COPY16(ptr0, y0, 16);
	R_COPY16(ptr0, y0+COL_MAX, 0);
	R_COPY16(ptr0, y0+COL_MAX, 16);

	R_COPY16(ptr1, y1, 0);
	R_COPY16(ptr1, y1, 16);
	R_COPY16(ptr1, y1+COL_MAX, 0);
	R_COPY16(ptr1, y1+COL_MAX, 16);
	y0 += 32; ptr0+= 32; 
	y1 += 32; ptr1+= 32; 
      }while(--j);
       ptr0 = (u0-(COL_MAX_DIV2));
       ptr1 = (u1-(COL_MAX_DIV2));
      j=11;
      do{
	R_COPY16(ptr1, u0, 0);
	R_COPY16(ptr1, u1, 0);
	u0+=16; u1+=16;
      }while(--j);
       ptr0 = (v0-(COL_MAX_DIV2));
       ptr1 = (v1-(COL_MAX_DIV2));
      j=11;
      do{
	R_COPY16(ptr1, v0, 0);
	R_COPY16(ptr1, v1, 0);
	v0+=16; v1+=16;
      }while(--j);
   }
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void Bvfc_get_cif4_color_pal (vfc_dev,  Y_data, Cu_data, Cv_data)
/*------------------------------------------------------------------*/
VfcDev *vfc_dev;
u_char Y_data[][LIG_MAX][COL_MAX];
u_char Cu_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
u_char Cv_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
{
  register int i, j;
  register u_char *y0,  *y1;
  register u_char *u0, *v0;
  register u_char *u1, *v1;
  register u_int u, v;
  register TYPE_A  tmpdata, y, cu, cv;
  TYPE_A *data_port1, *data_port2;
  register u_char *y2ccir = &COMPY2CCIR[0];
  register u_char *cuv2ccir = &COMPUV2CCIR[0];
  register int image;

#define D COL_MAX
#define DD COL_MAX_DIV2

    data_port1 = (TYPE_A *)vfc_dev->vfc_port1;
    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)Y_data[0]; /* CIF 0 */
    y1 = (u_char *)Y_data[1]; /* CIF 1 */

    u0 = (u_char *)Cu_data[0]; /* CIF 0 */
    u1 = (u_char *)Cu_data[1]; /* CIF 1 */

    v0 = (u_char *)Cv_data[0]; /* CIF 0 */
    v1 = (u_char *)Cv_data[1]; /* CIF 1 */



    WAIT_GRAB_END
    /* Skip over the non-active samples */
    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port1);
      SKIP_TMP16(data_port2);
    }

    image = 2;
    i=LIG_MAX/2+1;
    do{

    for(; --i; y0 += COL_MAX, y1 += COL_MAX){

    /* 35 pixels dropped */
    SKIP_TMP1(data_port1);
    SKIP_TMP2(data_port1);
    SKIP_TMP16(data_port1);
    SKIP_TMP16(data_port1);
    /* 35 pixels dropped */
    SKIP_TMP1(data_port2);
    SKIP_TMP2(data_port2);
    SKIP_TMP16(data_port2);
    SKIP_TMP16(data_port2);


         GET30Y32(data_port2, y0+COL_MAX, 0);
    colorGET30Y32(data_port1, y0, u0, v0, 0);
      y0 += 32; u0+=16; v0+=16;

    j=5;
    do{
      colorGET30Y32S(data_port1, y0, u0, v0, 0);
           GET30Y32(data_port2, y0+COL_MAX, 0);

           GET30Y32(data_port2, y0+COL_MAX, 32);
      colorGET30Y32(data_port1, y0, u0, v0, 32);

      y0 += 64;  u0+=32; v0+=32;
    }while(--j);

    colorGET30Y32S(data_port1, y1, u1, v1,0);
         GET30Y32(data_port2, y1+COL_MAX, 0);
      y1 += 32; u1+=16; v1+=16;

    j=5;
    do{
         GET30Y32(data_port2, y1+COL_MAX, 0);
    colorGET30Y32(data_port1, y1, u1, v1, 0);

    colorGET30Y32S(data_port1, y1, u1, v1, 32);
         GET30Y32(data_port2, y1+COL_MAX, 32);

      y1 += 64;  u1+=32; v1+=32;
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port1);
    SKIP_TMP8(data_port1);
    SKIP_TMP1(data_port1);
    /* 25 pixels dropped */
    SKIP_TMP16(data_port2);
    SKIP_TMP8(data_port2);
    SKIP_TMP1(data_port2);
  }
      if(!(--image))
      break;
     y0 = (u_char *)Y_data[2]; /* CIF 2 */
     y1 = (u_char *)Y_data[3]; /* CIF 3 */

     u0 = (u_char *)Cu_data[2]; /* CIF 2 */
     u1 = (u_char *)Cu_data[3]; /* CIF 3 */

     v0 = (u_char *)Cv_data[2]; /* CIF 2 */
     v1 = (u_char *)Cv_data[3]; /* CIF 3 */
      
      i= LIG_MAX/2;
  }while(1);
    {
      register u_char *ptr0;
      register u_char *ptr1;
      ptr0 = (y0-(CIF_WIDTH));
      ptr1 = (y1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, y0, 0);
	R_COPY16(ptr0, y0, 16);
	R_COPY16(ptr0, y0+COL_MAX, 0);
	R_COPY16(ptr0, y0+COL_MAX, 16);

	R_COPY16(ptr1, y1, 0);
	R_COPY16(ptr1, y1, 16);
	R_COPY16(ptr1, y1+COL_MAX, 0);
	R_COPY16(ptr1, y1+COL_MAX, 16);
	y0 += 32; ptr0+= 32; 
	y1 += 32; ptr1+= 32; 
      }while(--j);
       ptr0 = (u0-(COL_MAX_DIV2));
       ptr1 = (u1-(COL_MAX_DIV2));
      j=11;
      do{
	R_COPY16(ptr1, u0, 0);
	R_COPY16(ptr1, u1, 0);
	u0+=16; u1+=16;
      }while(--j);
       ptr0 = (v0-(COL_MAX_DIV2));
       ptr1 = (v1-(COL_MAX_DIV2));
      j=11;
      do{
	R_COPY16(ptr1, v0, 0);
	R_COPY16(ptr1, v1, 0);
	v0+=16; v1+=16;
      }while(--j);
   }
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void Avfc_get_cif4_color_pal (vfc_dev,  Y_data, Cu_data, Cv_data)
/*------------------------------------------------------------------*/
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

  static int limit = VFC_ESKIP_PAL >> 4;

/*
** Grab the image
*/
  WAIT_GRAB_END
    
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

  START_NEXT_GRAB
}



/*------------------------------------------------------------------*/
void vfc_get_qcif_pal(vfc_dev, y_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    u_char *y_data;
  {
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register TYPE_A  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];

    data_port = (TYPE_A *) vfc_dev->vfc_port2;

    /*
     * Grab the image
     */

    WAIT_GRAB_END

    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }

   for(i = LIG_MAX/2+1;--i; y_data += COL_MAX/2){ 

      /* Useful QCIF line */

    /* 35 pixels dropped */
    SKIP_TMP1(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    
     j=11;
    do{
      
      RD8_Y_QCIF_PAL_0(data_port, 0);
      RD8_Y_QCIF_PAL_1(data_port, 8);
      y_data += 16;
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);
 
      /* Useless QCIF line */

      SKIP_TMP16(data_port);
  j=11; 
  do{
      SKIP_TMP16(data_port);
      SKIP_TMP16(data_port);
      SKIP_TMP16(data_port);
      SKIP_TMP16(data_port);
    }while(--j);
  }
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_qcif_color_pal(vfc_dev, y_data, u_data, v_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    register u_char *y_data, *u_data, *v_data;

  {
    register int i, j;
    register TYPE_A tmpdata;
    register u_int u, v;
    register TYPE_A  y, cu, cv;
    register TYPE_A *ptr;
    TYPE_A *data_port;
   register u_char *y2ccir = &COMPY2CCIR[0];
   register u_char *cuv2ccir = &COMPUV2CCIR[0];


    data_port = (TYPE_A *)vfc_dev->vfc_port2;

  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */

  i=(LIG_MAX/2);
    goto FIRST_LINE;

  for(; --i; y_data += COL_MAX/2){

/* skip line = 720 pixels */
    j=22;
    do{
      SKIP_TMP16(data_port);
      SKIP_TMP16(data_port);
    }while(--j);
    SKIP_TMP16(data_port);

FIRST_LINE: /* no line drop at the begining */

    /* 35 pixels dropped */

    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP1(data_port);

    j=11;
    if(!(i&1)){
    do{
      CRD8_Y_QCIF_PAL_0(data_port, 0);
      CRD8_Y_QCIF_PAL_1(data_port, 8);
      u_data+=8; v_data+=8;
      y_data += 16;
    }while(--j);
    u_data+=COL_MAX/4; v_data+=COL_MAX/4;
    }else
    do{
      RD8_Y_QCIF_PAL_0(data_port, 0);
      RD8_Y_QCIF_PAL_1(data_port, 8);
      y_data += 16;
    }while(--j);

    /* 25 pixels dropped */
    SKIP_TMP16(data_port);
    SKIP_TMP8(data_port);
    SKIP_TMP1(data_port);

  }
  
  START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_cif_ntsc(vfc_dev, im_data)
/*------------------------------------------------------------------*/
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

    WAIT_GRAB_END

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

    START_NEXT_GRAB
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

    WAIT_GRAB_END

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
    START_NEXT_GRAB
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

    WAIT_GRAB_END

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
    START_NEXT_GRAB
  }

#endif


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
     char *tab;
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



