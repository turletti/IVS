/**************************************************************************
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
*  File              : grab_vfc.c                                          *
*  Last modification : 1995/2/15                                           *
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
*--------------------------------------------------------------------------*
* Andrzej Wozniak         |  93/6/18  | procedures rewriting for speed,    *
* wozniak@inria.fr        |           | square pixel color  for  both      *
*                         |           | PAL & NTSC added.                  *
\**************************************************************************/

#ifdef VIDEOPIX
#define GRAB_VFC 1

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
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
/*------------------------------------------------------------------*/
int stop_wait = 0;

void vfc_get_cif4_pal();
void vfc_get_cif_pal();
void vfc_get_qcif_pal();
void vfc_get_cif4_color_pal();
void vfc_get_cif_color_pal();
void vfc_get_qcif_color_pal();

void vfc_get_cif4_ntsc();
void vfc_get_cif_ntsc();
void vfc_get_qcif_ntsc();
void vfc_get_cif4_color_ntsc();
void vfc_get_cif_color_ntsc();
void vfc_get_qcif_color_ntsc();
/*------------------------------------------------------------------*/

void (*fun_table[2][2][3])() = 
{
vfc_get_cif4_ntsc, vfc_get_cif_ntsc, vfc_get_qcif_ntsc,
vfc_get_cif4_color_ntsc, vfc_get_cif_color_ntsc, vfc_get_qcif_color_ntsc,
vfc_get_cif4_pal, vfc_get_cif_pal, vfc_get_qcif_pal,
vfc_get_cif4_color_pal, vfc_get_cif_color_pal, vfc_get_qcif_color_pal

    }
;
/*------------------------------------------------------------------*/

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
  register u_char  *ptr;
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
  register u_char  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  
  data_port = (TYPE_A *) vfc_dev->vfc_port2;
 
  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_ESKIP_PAL >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX; --i;){
    
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
  j=11; ptr = y_data-(CIF_WIDTH);
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
    register u_char *ptr;
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
  
  for(i=LIG_MAX; --i;){

    /* 35 pixels dropped */

    SKIP_TMP16(data_port);
    SKIP_TMP16(data_port);
    SKIP_TMP2(data_port);
    SKIP_TMP1(data_port);

    j=11;

      if(i&1){
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
  j=11; ptr = y_data-(CIF_WIDTH);
  do{
    R_COPY16(ptr, y_data, 0);
    R_COPY16(ptr, y_data, 16);
    y_data += 32; ptr+= 32; 
  }while(--j);
  
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
void vfc_get_cif4_pal_interleave(vfc_dev, im_data)
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
void vfc_get_cif4_color_pal_interleave (vfc_dev,  Y_data, Cu_data, Cv_data)
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
void vfc_get_qcif_pal(vfc_dev, y_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    u_char *y_data;
  {
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register u_char  *ptr;
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
    register u_char *ptr;
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

FIRST_LINE: /* no line drop at the bigining */

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
void vfc_get_cif_ntsc(vfc_dev, y_data)
/*------------------------------------------------------------------*/
     VfcDev *vfc_dev;
     register u_char *y_data;
{
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register u_char *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  register duplicate = 5+1;
  
  data_port = (TYPE_A *) vfc_dev->vfc_port2;
 
  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=((VFC_OSKIP_NTSC)>> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){

    if(!(--duplicate)){
      /* duplicate input line */
      j=11; ptr = y_data-(CIF_WIDTH);
      do{
	R_COPY16(ptr, y_data, 0);
	R_COPY16(ptr, y_data, 16);
	y_data += 32; ptr+= 32; 
      }while(--j);
      duplicate = 5+1;
      continue;
    }
    
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

  START_NEXT_GRAB
  }
/*------------------------------------------------------------------*/
void vfc_get_cif4_ntsc(vfc_dev, im_data)
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
    register duplicate = 5+1;

    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)im_data[0]; /* CIF 0 */
    y1 = (u_char *)im_data[1]; /* CIF 1 */

    WAIT_GRAB_END

    /* Skip over the non-active samples */
    for(i=(VFC_OSKIP_NTSC >> 4)+1; --i;){
      SKIP_TMP16(data_port2);
    }

    image = 2;

    do{

    for(i=LIG_MAX/2+1; --i; ){

    if(!(--duplicate)){
      register u_char *ptr0;
      register u_char *ptr1;

      /* duplicate input line */

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

      duplicate = 5+1;
    }else{

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
 }
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
      
  }while(1);
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_qcif_ntsc(vfc_dev, y_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    u_char *y_data;
  {
  register int i, j;
  register TYPE_A tmpdata;
  register TYPE_A *data_port;
  register u_char  *ptr;
  register TYPE_A  y;
  register u_char *y2ccir = &COMPY2CCIR[0];
  register duplicate = 2+1;

    data_port = (TYPE_A *) vfc_dev->vfc_port2;

    /*
     * Grab the image
     */

    WAIT_GRAB_END

    /* Skip over the non-active samples */

    for(i=(VFC_OSKIP_NTSC >> 4)+1; --i;){
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
 
     if(!(--duplicate)){
       duplicate = 2+1;
       continue;
     }

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
void vfc_get_cif_color_ntsc(vfc_dev, y_data, u_data, v_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    register u_char *y_data, *u_data, *v_data;
  {
    register int i, j;
    register TYPE_A tmpdata;
    register u_int u, v;
    register TYPE_A  y, cu, cv;
    register u_char *ptr;
    TYPE_A *data_port;
    register u_char *y2ccir = &COMPY2CCIR[0];
    register u_char *cuv2ccir = &COMPUV2CCIR[0];
    register duplicate = 5+1;


    data_port = (TYPE_A *)vfc_dev->vfc_port2;

  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_OSKIP_NTSC >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */
  
  for(i=LIG_MAX+1; --i;){

    if(!(--duplicate)){
      /* duplicate input line */
      j=11; ptr = y_data-(CIF_WIDTH);
      do{
	R_COPY16(ptr, y_data, 0);
	R_COPY16(ptr, y_data, 16);
	y_data += 32; ptr+= 32; 
      }while(--j);
      if(!(i&1)){
	register u_char *ptr1;
	j=11;
	ptr  = (u_data-(CIF_WIDTH)/2);
	ptr1 = (v_data-(CIF_WIDTH)/2);
      do{
      /* duplicate color  */
	R_COPY16(ptr,  u_data, 0);
	R_COPY16(ptr1,  v_data, 0);
	u_data += 16; ptr += 16; 
	v_data += 16; ptr1+= 16; 
      }while(--j);
    }
      duplicate = 5+1;
      continue;
    }
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
    START_NEXT_GRAB
    }
  
/*------------------------------------------------------------------*/
void vfc_get_cif4_color_ntsc (vfc_dev,  Y_data, Cu_data, Cv_data)
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
  register duplicate = 5+1;


    data_port2 = (TYPE_A *)vfc_dev->vfc_port2;

    y0 = (u_char *)Y_data[0]; /* CIF 0 */
    y1 = (u_char *)Y_data[1]; /* CIF 1 */

    u0 = (u_char *)Cu_data[0]; /* CIF 0 */
    u1 = (u_char *)Cu_data[1]; /* CIF 1 */

    v0 = (u_char *)Cv_data[0]; /* CIF 0 */
    v1 = (u_char *)Cv_data[1]; /* CIF 1 */



    WAIT_GRAB_END
    /* Skip over the non-active samples */
    for(i=(VFC_OSKIP_NTSC >> 4)+1; --i;){
      SKIP_TMP16(data_port2);
    }

    image = 2;

    do{

    for(i=LIG_MAX/2+1; --i;){

    if(!(--duplicate)){
      register u_char *ptr0;
      register u_char *ptr1;

      /* duplicate input line */

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

      ptr0 = (u0-(CIF_WIDTH));
      ptr1 = (u1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, u0, 0);
	R_COPY16(ptr1, u1, 0);
	u0 +=16; ptr0 +=16;
	u1 +=16; ptr1 +=16;
      }while(--j);

      ptr0 = (v0-(CIF_WIDTH));
      ptr1 = (v1-(CIF_WIDTH));
      j=11;
      do{
	R_COPY16(ptr0, v0, 0);
	R_COPY16(ptr1, v1, 0);
	v0 +=16; ptr0 +=16;
	v1 +=16; ptr1 +=16;
      }while(--j);
      duplicate = 5+1;

    }else{

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
  }
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
      
  }while(1);
    START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
void vfc_get_qcif_color_ntsc(vfc_dev, y_data, u_data, v_data)
/*------------------------------------------------------------------*/
    VfcDev *vfc_dev;
    register u_char *y_data, *u_data, *v_data;

  {
    register int i, j;
    register TYPE_A tmpdata;
    register u_int u, v;
    register TYPE_A  y, cu, cv;
    register u_char *ptr;
    TYPE_A *data_port;
   register u_char *y2ccir = &COMPY2CCIR[0];
   register u_char *cuv2ccir = &COMPUV2CCIR[0];
    register duplicate = 2+1;


    data_port = (TYPE_A *)vfc_dev->vfc_port2;

  WAIT_GRAB_END
    
    /* Skip over the non-active samples */

    for(i=(VFC_OSKIP_NTSC >> 4)+1; --i;){
      SKIP_TMP16(data_port);
    }
  
  /* Read in the odd field data */

  for(i=(LIG_MAX/2)+1; --i; y_data += COL_MAX/2){

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
    
     if(!(--duplicate)){
       duplicate = 2+1;
       continue;
     }

    /* skip line = 720 pixels */
    j=22;
    do{
      SKIP_TMP16(data_port);
      SKIP_TMP16(data_port);
    }while(--j);
    SKIP_TMP16(data_port);

  }
  
  START_NEXT_GRAB
  }

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void VfcInitialize(vfc_format, size, COLOR, NG_MAX, L_lig, L_col, Lcoldiv2,
/*------------------------------------------------------------------*/
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
  }else{
    encCOLOR = TRUE;
    affCOLOR = MODCOLOR;
  }

  *get_image = fun_table[vfc_format][COLOR][size];

}
/*------------------------------------------------------------------*/

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

#endif
