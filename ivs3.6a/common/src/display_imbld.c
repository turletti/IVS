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
*  File              : display_imbld.c                                     *
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
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */


#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h> /* pas vraiment autorise mais pratique pour
                             la structure core */
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <sys/socket.h>
#include <sys/time.h>

#include "general_types.h"
#include "protocol.h"
#include "general_types.h"
#define DATA_MODULE
#include "display.h"

/*
#define CVR_CUB_CUVG(JX) \
	  CVR = cv2r[Cv_data[JX]];                     \
	  CUB = cu2b[Cu_data[JX]];                     \
	  CUVG = cv2g[Cv_data[JX]] + cu2g[Cu_data[JX]]
-###-*/

#define CVR_CUB_CUVG(JX) \
          aux = (u_long)(&cv2r[0]+Cv_data[JX]);     \
          CVR = *(TYPE_CUV2RGB *)aux;              \
          CUVG= *((TYPE_CUV2RGB *)aux + 256);      \
          aux = (u_long)(&cu2b[0]+Cu_data[JX]);     \
	  CUB = *(TYPE_CUV2RGB *)aux;              \
	  CUVG += *((TYPE_CUV2RGB *)aux + 256)

#define COMPUTE24(AUX) \
	   (((((XR_ECRETE_255(AUX + CUB)) << 8) |  \
	      (XR_ECRETE_255(AUX - CUVG)))<< 8) |  \
               XR_ECRETE_255( AUX + CVR))
#define COLOR24(OUT, YDATA, IX) \
          aux = cy2yrgb[YDATA[IX]]; \
	  OUT[IX] = COMPUTE24(aux)

#define COLOR24_4px(OUT, OUT2, YDATA, YDATA2, XX)    \
            CVR_CUB_CUVG((XX)/2);                    \
            COLOR24(OUT, YDATA, XX);                 \
            COLOR24(OUT, YDATA, XX+1);               \
            COLOR24(OUT2, YDATA2, XX);               \
            COLOR24(OUT2, YDATA2, XX+1)

#define COLOR24_ZOOM_IN_4px(OUT, OUT1, YDATA, IX) \
          aux = cy2yrgb[YDATA[IX]];                   \
    OUT1[2*(IX)+1] = OUT1[2*(IX)] =                   \
     OUT[2*(IX)+1] =  OUT[2*(IX)] = COMPUTE24(aux)

#define COLOR24_ZOOM_IN_16px(OUT, OUT1, OUT2, OUT21, YDATA, YDATA2, XX) \
                  CVR_CUB_CUVG((XX)/2);                           \
		  COLOR24_ZOOM_IN_4px( OUT,  OUT1,  YDATA,   (XX));     \
		  COLOR24_ZOOM_IN_4px( OUT,  OUT1,  YDATA, (XX+1));     \
		  COLOR24_ZOOM_IN_4px(OUT2, OUT21, YDATA2,   (XX));     \
		  COLOR24_ZOOM_IN_4px(OUT2, OUT21, YDATA2, (XX+1))

#define COLOR24_ZOOM_OUT(OUT, YDATA, IX) \
          CVR_CUB_CUVG(IX);             \
          aux = cy2yrgb[YDATA[2*(IX)]]; \
	    OUT[IX] = COMPUTE24(aux)

#define GRAY24_ZOOM_IN(OUT, OUT2, YDATA, IX)  \
	    sto_tmp = cy2y24rgb[YDATA[(IX)/2]];           \
            OUT[IX] = OUT[IX+1] = OUT2[IX] = OUT2[IX+1] = \
	    sto_tmp | (sto_tmp<<8) | (sto_tmp<<16)

#define GRAY24_ZOOM_IN_16px(OUT, OUT2, YDATA, XX) \
	    GRAY24_ZOOM_IN(OUT, OUT2, YDATA, XX);   \
	    GRAY24_ZOOM_IN(OUT, OUT2, YDATA, XX+2); \
	    GRAY24_ZOOM_IN(OUT, OUT2, YDATA, XX+4); \
	    GRAY24_ZOOM_IN(OUT, OUT2, YDATA, XX+6)

#define GRAY24_ZOOM_OUT(OUT, YDATA, IX) \
	    sto_tmp = cy2y24rgb[YDATA[2*(IX)]]; \
	    OUT[IX] = sto_tmp | (sto_tmp<<8) | (sto_tmp<<16)


#define GRAY24(OUT, YDATA, IX) \
	  sto_tmp = cy2y24rgb[Y_data[IX]];                   \
	  OUT[IX] = sto_tmp | (sto_tmp<<8) | (sto_tmp<<16)

#define GRAY24_4px(OUT, YDATA, XX) \
               GRAY24(OUT, YDATA, XX);   \
               GRAY24(OUT, YDATA, XX+1); \
               GRAY24(OUT, YDATA, XX+2); \
               GRAY24(OUT, YDATA, XX+3)
  
#define RVB(YDATA, IX) \
	    aux = cy2yrgb[YDATA[(IX)]];    \
	    R = XR_ECRETE_255(aux + CVR);  \
	    V = XR_ECRETE_255(aux - CUVG); \
	    B = XR_ECRETE_255(aux + CUB)

#define COLOR8_COMPUTE(OUT, IY, IX) \
     OUT[IX] =                                   \
         colmap_lut[ b_darrays[(IY)][B] + g_darrays[(IY)][V] + r_darrays[(IY)][R]];

#define COLOR8_1px(OUT, YDATA, IY, IX) \
   RVB(YDATA, IX);                     \
   COLOR8_COMPUTE(OUT, IY, IX)

#define COLOR8_4px(OUT, OUT2, YDATA, YDATA2, YY, XX) \
     COLOR8_1px( OUT,  YDATA, YY+ 0,   XX);         \
     COLOR8_1px( OUT,  YDATA, YY+ 8, XX+1);         \
     COLOR8_1px(OUT2, YDATA2, YY+12,   XX);         \
     COLOR8_1px(OUT2, YDATA2, YY+ 4, XX+1)

#define COLOR8_ZOOM_IN_4px(OUT1, OUT11, YDATA, YY, XX) \
     RVB(YDATA, (XX)/2);                \
     COLOR8_COMPUTE( OUT1,    YY,   XX);   \
     COLOR8_COMPUTE( OUT1,  YY+8, XX+1);   \
     COLOR8_COMPUTE(OUT11, YY+12,   XX);   \
     COLOR8_COMPUTE(OUT11,  YY+4, XX+1)

#define COLOR8_ZOOM_OUT(OUT, YDATA, YY, XX) \
          CVR_CUB_CUVG(XX);                 \
          RVB(YDATA, 2*(XX));               \
          COLOR8_COMPUTE(OUT, YY, XX)


#define GRAY4_ZOOM_IN(OUT, OUT2, YDATA, IY, IX)   \
   OUT[IX] = (y_darrays [   IY][YDATA[IX]] << 4 ) | y_darrays [IY+8][YDATA[IX]]; \
  OUT2[IX] = (y_darrays [IY+12][YDATA[IX]] << 4 ) | y_darrays [IY+4][YDATA[IX]]


#define GRAY1_ZOOM_IN(YDATA, IY, IX) \
            idx = Y_data[IX];                        \
            val  |= y_darrays [IY+ 0][idx]<<(7-2*(IX)); \
            val  |= y_darrays [IY+ 8][idx]<<(6-2*(IX)); \
            val2 |= y_darrays [IY+12][idx]<<(7-2*(IX)); \
            val2 |= y_darrays [IY+ 4][idx]<<(6-2*(IX))

#define GRAY1_ZOOM_IN_16px(OUT, OUT2, YDATA, IY, IY2) \
            idx  = Y_data[0];                      \
            val   = y_darrays [IY+ 0][idx]<<7;     \
            val  |= y_darrays [IY+ 8][idx]<<6;     \
            val2 = y_darrays [IY+12][idx]<<7;      \
            val2 |= y_darrays [IY+ 4][idx]<<6;     \
            GRAY1_ZOOM_IN(YDATA, IY2, 1);          \
            GRAY1_ZOOM_IN(YDATA,  IY, 2);          \
            GRAY1_ZOOM_IN(YDATA, IY2, 3);          \
            *OUT++  = val;                         \
            *OUT2++ = val2

#define GRAY1_IMAGE_8px(OUT, YDATA, AA, BB, CC, DD)   \
            val  = y_darrays [AA][Y_data[0]] << 7;  \
	    val |= y_darrays [BB][Y_data[1]] << 6;  \
	    val |= y_darrays [CC][Y_data[2]] << 5;  \
	    val |= y_darrays [DD][Y_data[3]] << 4;  \
	    val |= y_darrays [AA][Y_data[4]] << 3;  \
	    val |= y_darrays [BB][Y_data[5]] << 2;  \
	    val |= y_darrays [CC][Y_data[6]] << 1;  \
	    val |= y_darrays [DD][Y_data[7]];       \
            *OUT++ = val;


/*  Useful ordered dither matrices  */

static BOOLEAN DITH_TABLES_EXIST = FALSE;


/*
**		PROTOTYPES
*/

XImage  		*alloc_shared_ximage() ;
void  			destroy_shared_ximage() ;
BOOLEAN 		build_grey4_cmap();
BOOLEAN 		build_grey8_cmap();
BOOLEAN 		build_color_cmap();
void 			build_map_lut ();
void 			rebuild_map_lut();
void 			build_dither8_tables();
void 			build_dither4_tables();
void 			build_ditherBW_tables();
void			build_rgb_tables();



/*
**		CORPS DE FONCTION
*/
/*--------------------------------------------*/

void build_image (Y_data, Cu_data, Cv_data, DOUBLE, outXImage, srcWidth,
		  dstOffset, w, h )
     u_char *Y_data, *Cu_data, *Cv_data;
     BOOLEAN DOUBLE;
     XImage *outXImage;
     int srcWidth, dstOffset, w, h;
{
  register u_int l, c, offsetY, offsetC, offsetX, sto_tmp;

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : enter\n");
  fflush(stderr);
#endif


  DOUBLE = locDOUBLE ;

  if (outXImage->format == ZPixmap) {
    
    if ((outXImage->depth == 24 || outXImage->depth == 32 )
	&& affCOLOR & MODCOLOR) {

      register u_char	*Y_data2;
      register int 	CUB, CVR, CUVG; /*@@@*/
      register u_int    *out;
      register u_int 	*out2;

      offsetY = (srcWidth << 1) - w;
      offsetC = (srcWidth >> 1) - (w >> 1);

#ifdef BIT24_DEBUG
      fprintf(stderr, "DISPLAY.C-build_image() : entree - traitement 24 bit couleur, DOUBLE = %s HALF = %s\n",
	      DOUBLE ? "True" : "False", locHALF ? "True" : "False");
      fflush(stderr);
#endif
        
      if (DOUBLE){
	register u_int *out11, *out21 ;
	
	offsetX = (((outXImage->width)<<1) - w) << 1;
	Y_data2 = Y_data+srcWidth;

	out = ((u_int *)outXImage->data) + (dstOffset << 1);
	out11 = out + outXImage->width;
	out2 = out11 + outXImage->width;
	out21 = out2 + outXImage->width;
	
	l = h/2;
	do{
	  c = w/16;
	  do{
	    register long int aux;
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 0);
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 2);
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 4);
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 6);
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 8); 
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 10);
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 12); 
	    COLOR24_ZOOM_IN_16px(out, out11, out2, out21, Y_data, Y_data2, 14);
	    
	    out += 32 ; out11 += 32 ;
	    out2 += 32 ; out21 += 32 ;
	    Y_data += 16 ; Y_data2 += 16;
	    Cu_data += 8 ; Cv_data += 8;
	    
	  }while(--c);
	  out += offsetX ; out11 += offsetX ;
	  out2 += offsetX ; out21 += offsetX ;
	  Y_data += offsetY ;
	  Y_data2 += offsetY ;
	  Cu_data += offsetC ;
	  Cv_data += offsetC ;
	}while(--l);
	return ;
      } /* fi DOUBLE */

      if (locHALF) {
	offsetX = outXImage->width - (w >> 1);

	out = ((u_int *)outXImage->data) + (dstOffset >> 1) ;
	l = h/2;
	do{
	    c = w/16;
	  do{
	    register long int aux;

	    COLOR24_ZOOM_OUT(out, Y_data, 0);    
	    COLOR24_ZOOM_OUT(out, Y_data, 1);    
	    COLOR24_ZOOM_OUT(out, Y_data, 2);    
	    COLOR24_ZOOM_OUT(out, Y_data, 3);    
	    COLOR24_ZOOM_OUT(out, Y_data, 4);    
	    COLOR24_ZOOM_OUT(out, Y_data, 5);    
	    COLOR24_ZOOM_OUT(out, Y_data, 6);    
	    COLOR24_ZOOM_OUT(out, Y_data, 7);    

	    out += 8 ; Y_data += 16 ; Cv_data += 8 ; Cu_data += 8 ;
	  }while(--c);
	  Y_data += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	}while(--l);
	return ;
      } /* fi locHALF */

    
      out = ((u_int *)outXImage->data) + dstOffset;
      offsetX = (outXImage->width << 1) - w;
      
      Y_data2 = Y_data + srcWidth;
      out2 = out + outXImage->width;
      
#ifdef BIT24_DEBUG
      fprintf(stderr, 
	      "DISPLAY.C-build_image() : traitement 24 bit couleur - out = \n");
#endif

      l = h/2;
      do{
	register long int aux;

#define STEP_IM 16
	c = w/STEP_IM;
	do{
/*asm("label_aaaaa:");*/
	  COLOR24_4px(out, out2, Y_data, Y_data2,  0);
	  COLOR24_4px(out, out2, Y_data, Y_data2,  2);
	  COLOR24_4px(out, out2, Y_data, Y_data2,  4);
	  COLOR24_4px(out, out2, Y_data, Y_data2,  6);

	  COLOR24_4px(out, out2, Y_data, Y_data2,  8);
	  COLOR24_4px(out, out2, Y_data, Y_data2, 10);
	  COLOR24_4px(out, out2, Y_data, Y_data2, 12);
	  COLOR24_4px(out, out2, Y_data, Y_data2, 14);
	  
	  out += STEP_IM; out2 += STEP_IM;
	  Y_data += STEP_IM; Y_data2 += STEP_IM;
	  Cu_data += STEP_IM/2; Cv_data += STEP_IM/2;
#ifdef BIT24_DEBUG
	  fprintf(stderr, " %x ", *(out - 1));
#endif
	  
	}while(--c);
	Y_data += offsetY;
	Y_data2 += offsetY;
	Cu_data += offsetC;
	Cv_data += offsetC;
	out += offsetX;
	out2 += offsetX;
	
      }while(--l);
      return ;
    } /* fi 24 bits COLOR */ 

    if ((outXImage->depth == 24 || outXImage->depth == 32) 
	&& affCOLOR & MODGREY){
      register u_int *out;
      
      out = ((u_int *)outXImage->data) + dstOffset;
      
#ifdef BIT24_DEBUG
      fprintf(stderr, 
	      "DISPLAY.C-build_image() : entree - traitement 24 bit gris, DOUBLE = %s HALF = %s\n",
	      DOUBLE ? "True" : "False", locHALF ? "True" : "False");
      fflush(stderr);
#endif
      if (DOUBLE){
	register u_int *out2, aux;

	offsetY = srcWidth - w;
	offsetX = (outXImage->width - w) << 1;
	
	out = ((u_int *)outXImage->data) + (dstOffset << 1);
	out2 = out + outXImage->width;
	
	l = h;
	do{
	  c = w/16;
	  do{
	    GRAY24_ZOOM_IN_16px(out, out2, Y_data,  0);
	    GRAY24_ZOOM_IN_16px(out, out2, Y_data,  8);
	    GRAY24_ZOOM_IN_16px(out, out2, Y_data, 16);
	    GRAY24_ZOOM_IN_16px(out, out2, Y_data, 24);

	    out += 32; out2 += 32;
	    Y_data += 16;
	  }while(--c);
	  Y_data += offsetY;
	  out += offsetX;
	  out2 += offsetX;
	}while(--l);
	return ;
      } /* fi DOUBLE */
      if (locHALF) {

	offsetY = (srcWidth<<1) - w;

	offsetX = outXImage->width - (w >> 1);
	out = ((u_int *)outXImage->data) + (dstOffset >> 1) ;
	
	l = h/2;
	do{
	  c = w/16;
	  do{
	    GRAY24_ZOOM_OUT(out, Y_data, 0);
	    GRAY24_ZOOM_OUT(out, Y_data, 1);
	    GRAY24_ZOOM_OUT(out, Y_data, 2);
	    GRAY24_ZOOM_OUT(out, Y_data, 3);
	    GRAY24_ZOOM_OUT(out, Y_data, 4);
	    GRAY24_ZOOM_OUT(out, Y_data, 5);
	    GRAY24_ZOOM_OUT(out, Y_data, 6);
	    GRAY24_ZOOM_OUT(out, Y_data, 7);
	    out += 8 ; Y_data += 16;
	  }while(--c);
	  Y_data += offsetY;
	  out += offsetX;
	}while(--l);
	return ;
      } /* fi locHALF */
      offsetY = srcWidth - w;

      offsetX = outXImage->width - w;
      out = ((u_int *)outXImage->data) + dstOffset;
      
      l = h;
      do{
	c = w/16;
	do{
	  GRAY24_4px(out, Y_data,  0);
	  GRAY24_4px(out, Y_data,  4);
	  GRAY24_4px(out, Y_data,  8);
	  GRAY24_4px(out, Y_data, 12);
	  out += 16; Y_data += 16;
	}while(--c);
	Y_data += offsetY;
	out += offsetX;
      }while(--l);
      return ;
    } /* fi 24 bits and grey */

    if (outXImage->depth >= 8 && affCOLOR & MODCOLOR) {
      offsetY = (srcWidth << 1) - w;
      offsetC = (srcWidth >> 1) - (w >> 1);

      if (DOUBLE) {
	register u_char *out1, *out11 ;
	register u_char *out2, *out21 ;
	register u_char *Y_data2;
	register int R, V, B, CUB, CVR, CUVG;
	
	out1 = (u_char *)outXImage->data + (dstOffset << 1);
	offsetX = (outXImage->width << 2) - (w << 1) ;
	
	Y_data2 = Y_data + srcWidth;
	out11 = (u_char *)out1 + outXImage->width ;
	out2 = (u_char *)out11 + outXImage->width ;
	out21 = (u_char *)out2 + outXImage->width ;
	
	l = h/2;
	do{
	  c = w/2;
	  do{
	    register long int aux;

	    CVR_CUB_CUVG(0);
	    COLOR8_ZOOM_IN_4px(out1, out11,  Y_data, 0, 0);
	    COLOR8_ZOOM_IN_4px(out1, out11,  Y_data, 2, 2);
	    COLOR8_ZOOM_IN_4px(out2, out21, Y_data2, 3, 0);
	    COLOR8_ZOOM_IN_4px(out2, out21, Y_data2, 1, 2);
	    
	    out1 += 4 ; out11 += 4 ; out2 += 4 ; out21 += 4 ;
	    Y_data += 2 ; Y_data2 += 2 ;
	    Cv_data++; Cu_data++;
	  }while(--c);
	  
	  Y_data += offsetY ;
	  Y_data2 += offsetY ;
	  Cu_data += offsetC ;
	  Cv_data += offsetC ;
	  out1 += offsetX ; out11 += offsetX ;
	  out2 += offsetX ; out21 += offsetX ;
	}while(--l);

	return ;
      } /* fi DOUBLE */
      
      if (locHALF) {
	/*
	 *  Keep Y_tab even lines only, linked with their related
	 *  Cu_tab, Cv_tab lines	
	 */
	register u_char *out;
	register int R, V, B, CUB, CVR, CUVG;
	out = (u_char *)outXImage->data + (dstOffset >> 1) ;

	offsetX = outXImage->width - (w >> 1);
	
	l = h/8;
	do{
	  c = w/8;
	  do{
	    register long int aux;

	    COLOR8_ZOOM_OUT(out, Y_data,  0, 0);
	    COLOR8_ZOOM_OUT(out, Y_data,  8, 1);
	    COLOR8_ZOOM_OUT(out, Y_data,  2, 2);
	    COLOR8_ZOOM_OUT(out, Y_data, 10, 3);
	    	    
	    out += 4; Y_data += 8 ;
	    Cu_data += 4;
	    Cv_data += 4;
	  }while(--c);
	  
	  Y_data += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;

	  c = w/8;
	  do{
	    register long int aux;

	    COLOR8_ZOOM_OUT(out, Y_data,  12, 0);
	    COLOR8_ZOOM_OUT(out, Y_data,   4, 1);
	    COLOR8_ZOOM_OUT(out, Y_data,  14, 2);
	    COLOR8_ZOOM_OUT(out, Y_data,   6, 3);
	    
	    out += 4; Y_data += 8 ;
	    Cu_data += 4;
	    Cv_data += 4;
	  }while(--c);
	  
	  Y_data += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	  
	  c = w/8;
	  do{
	    register long int aux;
	    
	    COLOR8_ZOOM_OUT(out, Y_data,   3, 0);
	    COLOR8_ZOOM_OUT(out, Y_data,  11, 1);
	    COLOR8_ZOOM_OUT(out, Y_data,   1, 2);
	    COLOR8_ZOOM_OUT(out, Y_data,   9, 3);

	    out += 4; Y_data += 8 ;
	    Cu_data += 4;
	    Cv_data += 4;
	  }while(--c);
	  
	  Y_data += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	  
	  c = w/8;
	  do{
	    register long int aux;

	    COLOR8_ZOOM_OUT(out, Y_data,  15, 0);
	    COLOR8_ZOOM_OUT(out, Y_data,   7, 1);
	    COLOR8_ZOOM_OUT(out, Y_data,  13, 2);
	    COLOR8_ZOOM_OUT(out, Y_data,   5, 3);
    
	    out += 4; Y_data += 8 ;
	    Cu_data += 4;
	    Cv_data += 4;
	  }while(--c);
	  
	  Y_data += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	  
	}while(--l);
	return ;
      } /* fi locHALF */
      /*
       *  Regular
       */
      {
	register u_char *out;
	register u_char *out2;
	register u_char *Y_data2;
	register int R, V, B, CUB, CVR, CUVG;
	
	out = (u_char *)outXImage->data + dstOffset;
	offsetX = (outXImage->width << 1) - w;
	
	Y_data2 = Y_data + srcWidth;
	out2 = (u_char *)out + outXImage->width;
	
	l = h/4;
	do{
	  c = w/4;
	  do{
	    register long int aux;
	    
	    CVR_CUB_CUVG(0);
	    COLOR8_4px(out, out2, Y_data, Y_data2, 0, 0);
	    CVR_CUB_CUVG(1);
	    COLOR8_4px(out, out2, Y_data, Y_data2, 2, 2);
	    
	    out += 4; out2 += 4;
	    Y_data += 4; Y_data2 += 4;
	    Cu_data += 2;
	    Cv_data += 2;
	    
	  }while(--c);
	  
	  Y_data += offsetY;
	  Y_data2 += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	  out2 += offsetX;

	  c = w/4;
	  do{
	    register long int aux;

	    CVR_CUB_CUVG(0);
	    COLOR8_4px(out, out2, Y_data, Y_data2, 3, 0);
	    CVR_CUB_CUVG(1);
	    COLOR8_4px(out, out2, Y_data, Y_data2, 1, 2);
	    
	    out += 4; out2 += 4;
	    Y_data += 4; Y_data2 += 4;
	    Cu_data += 2;
	    Cv_data += 2;
	    
	  }while(--c);
	  
	  Y_data += offsetY;
	  Y_data2 += offsetY;
	  Cu_data += offsetC;
	  Cv_data += offsetC;
	  out += offsetX;
	  out2 += offsetX;
	  
	}while(--l);
	return ;
      } /* block regular */
    } /* depth >= 8 and color */
    if(outXImage->depth >= 8 && (affCOLOR & MODGREY)){
      register u_char *out;
      
      /*  GREY LEVEL DISPLAYING
       **  One pixel is encoded with 6 bits (64 shades of gray)
       */
      
      if (DOUBLE){
	register u_char *out2, aux;

	offsetY = srcWidth - w;
	offsetX = (outXImage->width - w) << 1;
	
        out = (u_char *)outXImage->data + (dstOffset << 1);
        out2 = out + outXImage->width;
	
	l = h;
	do{
	  c = w/4;
	  do{
	    out[0] = out[1] = out2[0] = out2[1] = y2rgb_conv[Y_data[0]];
	    out[2] = out[3] = out2[2] = out2[3] = y2rgb_conv[Y_data[1]];
	    out[4] = out[5] = out2[4] = out2[5] = y2rgb_conv[Y_data[2]];
	    out[6] = out[7] = out2[6] = out2[7] = y2rgb_conv[Y_data[3]];
	    out += 8;
	    out2+= 8;
	    Y_data += 4;
          }while(--c);
          Y_data += offsetY;
          out += offsetX;
          out2 += offsetX;
        }while(--l);
	return ;
      } /* fi DOUBLE */
      if (locHALF) {

	offsetY = (srcWidth << 1) - w;
	offsetX = outXImage->width - (w >> 1) ;
	out = ((u_char *)outXImage->data) + (dstOffset >> 1) ;

	
	l = h/2;
	do{
	  c = w/8;
	  do{
	    out[0] = y2rgb_conv[Y_data[0]];
	    out[1] = y2rgb_conv[Y_data[2]];
	    out[2] = y2rgb_conv[Y_data[4]];
	    out[3] = y2rgb_conv[Y_data[6]];
	    out += 4;
	    Y_data += 8 ;
	  }while(--c);
	  Y_data += offsetY;
	  out += offsetX;
	}while(--l);
	return ;
      } /* fi locHALF */
      /*
       *  Regular
       */
      offsetY = srcWidth - w;
      offsetX = outXImage->width - w ;
      out = (u_char *)outXImage->data + dstOffset;

      l = h;
      do{
	c = w/8;
	do{
	  out[0] = y2rgb_conv[Y_data[0]];
	  out[1] = y2rgb_conv[Y_data[1]];
	  out[2] = y2rgb_conv[Y_data[2]];
	  out[3] = y2rgb_conv[Y_data[3]];
	  out[4] = y2rgb_conv[Y_data[4]];
	  out[5] = y2rgb_conv[Y_data[5]];
	  out[6] = y2rgb_conv[Y_data[6]];
	  out[7] = y2rgb_conv[Y_data[7]];
	  Y_data += 8;
	  out += 8;
	}while(--c);
	Y_data += offsetY;
	out += offsetX;
      }while(--l);
      return ;
    } /* fi MODGREY */

    if((outXImage->depth) == 4 && (affCOLOR & MODGREY)){
      register u_char *out;
      offsetY = srcWidth - w;

      if (DOUBLE) {
	register u_char *out2, aux;
	
	offsetX = outXImage->width - w;
	
	out = (u_char *)outXImage->data + dstOffset;
	out2 = out + (outXImage->width >> 1);
	
	l = h/2;
	do{
	  c = w/4;
	  do{
	    GRAY4_ZOOM_IN(out, out2, Y_data, 0, 0);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 2, 1);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 0, 2);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 2, 3);
	    Y_data += 4;
	    out += 4; out2 += 4;
	  }while(--c);
	  Y_data  += offsetY;
	  out += offsetX;
	  out2 += offsetX;

	  c = w/4;
	  do{
	    GRAY4_ZOOM_IN(out, out2, Y_data, 3, 0);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 1, 1);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 3, 2);
	    GRAY4_ZOOM_IN(out, out2, Y_data, 1, 3);
	    Y_data += 4;
	    out += 4; out2 += 4;
	  }while(--c);
	  Y_data  += offsetY;
	  out += offsetX;
	  out2 += offsetX;
	}while(--l);
	return ;
      } /* fi DOUBLE */

      offsetX = (outXImage->width - w) >> 1;
      out = (u_char *)outXImage->data + (dstOffset >> 1);
      
      l = h/4;
      do{
        c = w/8;
	do{
          out[0] = (y_darrays [0][Y_data[0]] << 4 ) | y_darrays [8][Y_data[1]];
          out[1] = (y_darrays [2][Y_data[2]] << 4) | y_darrays [10][Y_data[3]];
          out[2] = (y_darrays [0][Y_data[4]] << 4) | y_darrays [8][Y_data[5]];
          out[3] = (y_darrays [2][Y_data[6]] << 4) | y_darrays [10][Y_data[7]];
          Y_data += 8;
          out += 4;
        }while(--c);
        Y_data  += offsetY;
        out += offsetX;
        c = w/8;
	do{
          out[0] = (y_darrays [12][Y_data[0]] << 4) | y_darrays [4][Y_data[1]];
          out[1] = (y_darrays [14][Y_data[2]] << 4) | y_darrays [6][Y_data[3]];
          out[2] = (y_darrays [12][Y_data[4]] << 4) | y_darrays [4][Y_data[5]];
          out[3] = (y_darrays [14][Y_data[6]] << 4) | y_darrays [6][Y_data[7]];
          Y_data += 8;
          out += 4;
        }while(--c);
        Y_data += offsetY;
        out += offsetX;
        c = w/8;
	do{
          out[0] = (y_darrays [3][Y_data[0]] << 4) | y_darrays [11][Y_data[1]];
          out[1] = (y_darrays [1][Y_data[2]] << 4) | y_darrays [9][Y_data[3]];
          out[2] = (y_darrays [3][Y_data[4]] << 4) | y_darrays [11][Y_data[5]];
          out[3] = (y_darrays [1][Y_data[6]] << 4) | y_darrays [9][Y_data[7]];
          Y_data += 8;
          out += 4;
        }while(--c);
        Y_data  += offsetY;
        out += offsetX;
        c = w/8;
	do{
          out[0] = (y_darrays [15][Y_data[0]] << 4) | y_darrays [7][Y_data[1]];
          out[1] = (y_darrays [13][Y_data[2]] << 4) | y_darrays [5][Y_data[3]];
          out[2] = (y_darrays [15][Y_data[4]] << 4) | y_darrays [7][Y_data[5]];
          out[3] = (y_darrays [13][Y_data[6]] << 4) | y_darrays [5][Y_data[7]];
          Y_data += 8;
          out += 4;
        }while(--c);
        Y_data += offsetY;
        out += offsetX;
      }while(--l);
      return ;
    } /* fi 4 bits */
  } /* fi format == XPixmap */
  
  
  /*
   **  BLACK & WHITE DISPLAYING
   **  using ordered dither algorithme
   */
  offsetY = srcWidth - w;
  
#ifdef BW_DEBUG
  fprintf(stderr, "DISPLAY.C-build_image() : traitement N&B\n");
  fflush(stderr);
#endif
  
  if (DOUBLE){
    register u_char *out;
    register u_char *out2;
    register u_char val;
    register u_char val2;
    register int idx;
    
    offsetX = (outXImage->width - w ) >> 2;
    
    out = (u_char *)outXImage->data + (dstOffset >> 2);
    out2 = out + (outXImage->width >> 3);
    
    l = h/2;
    do{
      c = w/4;
      do{
	GRAY1_ZOOM_IN_16px(out, out2, Y_data, 0, 2);
	Y_data += 4;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;
      out2 += offsetX;

      c = w/4;
      do{
	GRAY1_ZOOM_IN_16px(out, out2, Y_data, 3, 1);
	Y_data += 4;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;
      out2 += offsetX;
    }while(--l);
    return ;
  } /* fi DOUBLE */
  {
    register u_char *out;
    register u_char val;
    offsetX = (outXImage->width - w) >> 3;
    out = (u_char *)outXImage->data + (dstOffset >> 3);
    
    l = h/4;
    do{
      c = w/8;
      do{
	 GRAY1_IMAGE_8px(out, Y_data, 0, 8, 2, 10);
	Y_data += 8;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;

      c = w/8;
      do{
	GRAY1_IMAGE_8px(out, Y_data, 12, 4, 14, 6);
	Y_data += 8;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;

      c = w/8;
      do{
	GRAY1_IMAGE_8px(out, Y_data, 3, 11, 1, 9);
	Y_data += 8;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;

      c = w/8;
      do{
	GRAY1_IMAGE_8px(out, Y_data, 15, 7, 13, 5);
	Y_data += 8;
      }while(--c);
      Y_data  += offsetY;
      out += offsetX;
    } while(--l);
    return ;
  }
  
} /* end build_image */

/*-----------------------------------------------------------------------*/

void show(dpy, dst_win, ximage)
     Display *dpy;
     Window dst_win ;
     XImage *ximage;

{
  int L_col = ximage->width;
  int L_lig = ximage->height;
/*  {
    extern int wait_debug;
    while(!wait_debug);
  }*/
#ifdef BW_DEBUG
  fprintf(stderr, "DISPLAY.C-show() : entree\n");
  fflush(stderr);            
#endif
  
#ifdef MITSHM
  if(does_shm){
    XShmPutImage(dpy, dst_win, gc_disp, ximage, 0, 0, 0, 0, L_col, L_lig, FALSE);
    return;
  }else
#endif /* MITSHM */
    {
      XPutImage(dpy, dst_win, gc_disp, ximage, 0, 0, 0, 0, L_col, L_lig);
    }
}

/*--------------------------------------------*/

void build_icon_image(dst_win, size, lum_tab, cb_tab, cr_tab, col_max,
col_util, lig_util, SHOW)
     Window dst_win ;
     int size ;
     u_char *lum_tab ;
     u_char *cb_tab ;
     u_char *cr_tab ;
     int col_max, col_util, lig_util ;
     BOOLEAN SHOW ;
{

  char Y[64][64], Cb[32][32], Cr[32][32] ;
  int order, l, c, lum_offset, chrom_offset, lig_offset, col_offset ;
  static XImage *icon_image = NULL ;
  BOOLEAN saveDOUBLE, saveHALF ;
 
#ifdef WIN_DEBUG
  fprintf (stderr, "DISPLAY.C-build_icon_image : input - col_max = %d,
col_util = %d, lig_util = %d\n",
	   col_max, col_util, lig_util) ;
#endif

  if (!icon_image) {
    icon_image = alloc_shared_ximage(dpy_disp, 64, 64, depth_disp);
    /*icon_image->bits_per_pixel = depth_disp;*/
  }


  col_offset = col_util - (col_util>>6)<<6 ;
  lig_offset = lig_util - (lig_util>>6)<<6 ;
  /*
   *  Rebuild Y, Cb, Cr
   */
  switch (size) 
    {
    case QCIF_SIZE :
      col_offset = 48 ;
      lig_offset = 16 ;
      lum_offset = col_offset + (col_max<<1) - col_util ;
#ifdef WIN_DEBUG
  fprintf (stderr, "DISPLAY.C-build_icon_image : QCIF - lum_offset =
%d\n", lum_offset) ;
#endif
      lum_tab += (col_offset>>1) + col_max*(lig_offset>>1) ;
      for (l = 0 ; l < 64 ; l++) {
	for (c = 0 ; c < 64 ; c++) {
	  Y[l][c] = *lum_tab ;
	  lum_tab += 2 ;
	} /* rof c */
	lum_tab += lum_offset ;
      } /* rof l */
      if (affCOLOR) {
	chrom_offset = lum_offset >> 1 ;
	cb_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	cr_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	for (l = 0 ; l < 32 ; l++) {
	  for (c = 0 ; c < 32 ; c++) {
	    Cb[l][c] = *cb_tab ;
	    Cr[l][c] = *cr_tab ;
	    cb_tab += 2 ;
	    cr_tab += 2 ;
	  } /* rof c */
	  cb_tab += chrom_offset ;
	  cr_tab += chrom_offset ;
	} /* rof l */
      } /* fi affCOLOR */      
      break ;
    case CIF_SIZE :
      col_offset = 96 ;
      lig_offset = 32 ;
      lum_offset = col_offset + (col_max<<2) - col_util ;
#ifdef WIN_DEBUG
  fprintf (stderr, "DISPLAY.C-build_icon_image : CIF - lum_offset =
%d\n", lum_offset) ;
#endif
      lum_tab += (col_offset>>1) + col_max*(lig_offset>>1) ;
      for (l = 0 ; l < 64 ; l++) {
	for (c = 0 ; c < 64 ; c++) {
	  Y[l][c] = *lum_tab ;
	  lum_tab += 4 ;
	} /* rof c */
	lum_tab += lum_offset ;
      } /* rof l */
      if (affCOLOR) {
	chrom_offset = lum_offset >> 1 ;
	cb_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	cr_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	for (l = 0 ; l < 32 ; l++) {
	  for (c = 0 ; c < 32 ; c++) {
	    Cb[l][c] = *cb_tab ;
	    Cr[l][c] = *cr_tab ;
	    cb_tab += 4 ;
	    cr_tab += 4 ;
	  } /* rof c */
	  cb_tab += chrom_offset ;
	  cr_tab += chrom_offset ;
	} /* rof l */
      } /* fi affCOLOR */
      break ;
    case CIF4_SIZE :
      {
	int l_deb, c_deb, l_fin, c_fin ;
	col_offset = 192 ;
	lig_offset = 64 ;
	lum_offset = (col_offset>>1) + (col_max<<3) - col_util ;
	chrom_offset = lum_offset >> 1 ;
	for (order = 0 ; order < 4 ; order++) {
	  switch (order)
	    {
	    case 0 :
	      /*
	       *  1er quadrant
	       */
	      lum_tab += (col_offset>>1) + col_max*(lig_offset>>1) ;
	      cb_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	      cr_tab += (col_offset>>2) + (col_max>>1)*(lig_offset>>2) ;
	      c_deb = l_deb = 0 ;
	      break ;
	    case 1 :
	      /*
	       *  2eme quadrant
	       */
	      lum_tab += col_max*(lig_offset>>1) - (col_offset>>1) ;
	      cb_tab += (col_max>>1)*(lig_offset>>2) - (col_offset>>2) ;
	      cr_tab += (col_max>>1)*(lig_offset>>2) - (col_offset>>2) ;
	      c_deb = 32 ; l_deb = 0 ;
	      break ;
	    case 2 :
	      /*
	       *  3eme quadrant
	       */
	      lum_tab += (col_offset>>1) ;
	      cb_tab += (col_offset>>2) ;
	      cr_tab += (col_offset>>2) ;
	      c_deb = 0 ; l_deb = 32 ;
	      break ;
	    case 3 :
	      /*
	       *  4eme quadrant
	       */
	      lum_tab += col_max*(lig_offset>>1) - (col_offset>>1) ;
	      cb_tab += (col_max>>1)*(lig_offset>>2) - (col_offset>>2) ;
	      cr_tab += (col_max>>1)*(lig_offset>>2) - (col_offset>>2) ;
	      c_deb = l_deb = 32 ;
	      break ;
	    } /* hctiws order */
	  c_fin = c_deb + 32 ;
	  l_fin = l_deb + 32 ;
	  for (l = l_deb ; l < l_fin ; l++) {
	    for (c = c_deb ; c < c_fin ; c++) {
	      Y[l][c] = *lum_tab ;
	      lum_tab += 8 ;
	    } /* rof c */
	    lum_tab += lum_offset ;
	  } /* rof l */
	  if (affCOLOR) {
	    for (l = (l_deb>>1) ; l < (l_fin>>1) ; l++) {
	      for (c = (c_deb>>1) ; c < (c_fin>>1) ; c++) {
		Cb[l][c] = *cb_tab ;
		Cr[l][c] = *cr_tab ;
		cb_tab += 8 ;
		cr_tab += 8 ;
	      } /* rof c */
	      cb_tab += chrom_offset ;
	      cr_tab += chrom_offset ;
	    } /* rof l */
	  } /* fi affCOLOR */
	  
	} /* rof order */
      }
      break ;
    } /* hctiws size */
  saveDOUBLE = locDOUBLE ;
  saveHALF = locHALF ;
  locDOUBLE = locHALF = FALSE ;

  build_image(Y, Cb, Cr, FALSE, icon_image, 64, 0, 64, 64) ;
  locDOUBLE = saveDOUBLE ;
  locHALF = saveHALF ;

  if (SHOW)
    show(dpy_disp, dst_win, icon_image) ;


} /* end build_icon_image */

/*--------------------------------------------*/



