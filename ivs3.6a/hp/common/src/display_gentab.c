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
*  File              : display_gentab.c                                    *
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

/*--------------------------------------------*/

#include <sys/time.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h> /* pas vraiment autorise mais pratique pour
la structure core */
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <sys/socket.h>

#include "general_types.h"
#include "protocol.h"
#include "general_types.h"
#include "display.h"

/*--------------------------------------------*/
static int		stdmplt_size = 0;
static int		offset_NB = 0;
static BOOLEAN		GREYMAPLUT = FALSE;
static BOOLEAN		COLORMAPLUT= FALSE;
int			COL_MP_LGTH = 0;
/*--------------------------------------------*/
static u_char		y_map[256] ,
	 		red_map [256],
         		green_map [256],
         		blue_map [256];

static  u_long		pixels[256];
static  u_char		std_map_lut[256];

/*----------- EXPORTED VARIABLES -------------*/

int	cur_brght = 0;
float	cur_ctrst = 1.0;
int	cur_int_ctrst = 0;
u_char		map_lut[256];

/*-------- FORWARD REFERENCE -----------------*/
void build_dither8_tables();
void build_dither4_tables();
void build_ditherBW_tables();
void build_rgb_tables();
BOOLEAN build_grey4_cmap();
BOOLEAN build_grey8_cmap();
BOOLEAN build_color_cmap();
/*--------------------------------------------*/

void build_map_lut(display, screen, depth)
     Display	*display;
     int	screen, depth;
{
  Screen		*SCREEN;
  register int 		i = 0, k;
  int			rampdisp = 0;
  double 		tmp_ind;
  XColor		colors [256];
  register int 		ri, bi, gi;

#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-build_map_lut() : entree\n");
  fflush(stderr);
#endif

  SCREEN = XScreenOfDisplay(display, screen);
  colmap = DefaultColormapOfScreen(SCREEN);

  if(affCOLOR > MODNB){

    if(depth == 8){
    /*
    *  Grey and color colormaps are automatically set up.
    */
  
    /*
    *  Building the grey colormap.
    */

      k = 0;
      YR = grey8Set[k];
      while((!build_grey8_cmap(display, colmap, &rampdisp)) && (++k<RAMPGREY)) 
	YR = grey8Set[k];
      if(k < RAMPGREY){
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
      } /* fi k < RAMPGREY */

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
        if(k >= RAMPCOL) affCOLOR = GREYMAPLUT ? MODGREY : MODNB;
        else{
          colmap_lut = map_lut + offset_NB;
          COL_MP_LGTH = RR*GR*BR;
          stdmplt_size += rampdisp;
  /*
  *  Building the color dithering's tables.
  */
          build_dither8_tables();
        } /* fi k */
      } /* fi MODCOLOR */
      if (!COLORMAPLUT && !GREYMAPLUT) affCOLOR = MODNB;

    }else 
    if (depth == 4){
      k = 0;
      YR = grey4Set[k];
      while((!build_grey4_cmap(display, colmap, &rampdisp)) && (++k<RAMPGREY)) 
	YR = grey4Set[k];
      if(k >= RAMPGREY) affCOLOR = MODNB;
      else{
	/*
	*  std_map_lut's size is saved. Looking for the number of available 
	*  colors.
	*/
        stdmplt_size = offset_NB = rampdisp;
  /*
  *  Building the grey dithering's tables.
  */
        build_dither4_tables();
      } /* fi k */
    } /* fi depth */

  } /* fi affCOLOR > MODNB */

  if (affCOLOR & MODNB){ 
  /*
  *  Building the B&W dithering's tables.
  */
    build_ditherBW_tables();
  }/* fi MODNB */


#ifdef WIN_DEBUG
  fprintf(stderr, "DISPLAY.C-build_map_lut() : sortie\n");
  fflush(stderr);
#endif
} /* end build_map_lut () */

/*--------------------------------------------*/

BOOLEAN build_grey4_cmap(display, colmap, i)
     Display		*display;
     Colormap        colmap;
     int		*i;
{
  register int		k;
  XColor		colors[256];

  *i = 0;
#ifdef COL_DEBUG
  fprintf(stderr, "DISPLAY.C-build_grey4_cmap(): entree, YR vaut %d\n", YR);
  fflush(stderr);
#endif
  for(k=0; (k<256) && (*i<256); k+=255/(int)(YR-1)){
    y_map[*i] = k;
    colors[*i].red = colors[*i].green = colors[*i].blue = k<<8;
    colors[*i].flags = DoRed | DoGreen | DoBlue;
    if(!XAllocColor(display, colmap, &colors[*i])){
      /*
      *  No much place in the default colormap.
      */
#ifdef WIN_DEBUG
      fprintf(stderr, "DISPLAY.C-build_grey4_cmap() : no more space available \
in standard colormap, IVS will not be able to display in a nice color mode\n");
#endif
      XFreeColors(display, colmap, pixels, (*i), 0);

      GREYMAPLUT = FALSE;
#ifdef COL_DEBUG
      fprintf(stderr, 
   "DISPLAY.C-build_grey4_cmap(): error return, %d places remain,
GREYMAPLUT = %d\n",
	      (*i), GREYMAPLUT);
#endif
      return(GREYMAPLUT);
    }else{
      std_map_lut[*i] = map_lut[*i]  = pixels[*i] = colors[*i].pixel;
      (*i)++;
    } /* fi XAllocColor */
  } /* rof k */

  GREYMAPLUT = TRUE;
#ifdef COL_DEBUG
  fprintf(stderr, 
	  "DISPLAY.C-build_grey4_cmap(): valid return, %d places used,
GREYMAPLUT = %d\n",
	  (*i), GREYMAPLUT);
  fflush(stderr);
#endif
  return (GREYMAPLUT);
}

BOOLEAN build_grey8_cmap(display, colmap, i)
     Display		*display;
     Colormap           colmap;
     int		*i;
{
  register int		k;
  XColor		colors[256];

  *i = 0;
#ifdef COL_DEBUG
  fprintf(stderr, "DISPLAY.C-build_grey8_cmap(): entree, YR vaut %d\n", YR);
  fflush(stderr);
#endif
  for(k=0; (k<256) && (*i<256); k+=255/(int)(YR-1)){
    y_map[*i] = k;
    colors[*i].red = colors[*i].green = colors[*i].blue = k<<8;
    colors[*i].flags = DoRed | DoGreen | DoBlue;
    if(!XAllocColor(display, colmap, &colors[*i])){
      /*
      *  No much place in the default colormap.
      */
#ifdef WIN_DEBUG
      fprintf(stderr, "DISPLAY.C-build_grey8_cmap() : no more space available \
in standard colormap, IVS will not be able to display in a nice color mode\n");
#endif
      XFreeColors(display, colmap, pixels, (*i), 0);

      GREYMAPLUT = FALSE;
#ifdef COL_DEBUG
      fprintf(stderr, 
   "DISPLAY.C-build_grey8_cmap(): error return, %d places remain,
GREYMAPLUT = %d\n",
	      (*i), GREYMAPLUT);
#endif
      return(GREYMAPLUT);
    }else{
      std_map_lut[*i] = map_lut[*i]  = pixels[*i] = colors[*i].pixel;
      (*i)++;
    } /* fi XAllocColor */
  } /* rof k */

  GREYMAPLUT = TRUE;
#ifdef COL_DEBUG
  fprintf(stderr, 
	  "DISPLAY.C-build_grey8_cmap(): valid return, %d places used,
GREYMAPLUT = %d\n",
	  (*i), GREYMAPLUT);
  fflush(stderr);
#endif
  return (GREYMAPLUT);
}

/*--------------------------------------------*/

BOOLEAN build_color_cmap(display, colmap, i)
     Display		*display;
     Colormap           colmap;
     int		*i;
{
  XColor		colors[256];
  register int 		ri, bi, gi;

#ifdef COL_DEBUG
  fprintf(stderr, 
"DISPLAY.C-build_color_cmap(): entree, RR vaut %d, GR vaut %d, BR vaut %d\n", 
	  RR, GR, BR);
  fflush(stderr);
#endif
  *i = 0;
  for(ri = 0; ri < 256; ri += 255/(int)(RR-1))
    for(gi = 0; gi < 256; gi += 255/(int)(GR-1))
      for(bi = 0; bi < 256; bi += 255/(int)(BR-1)){
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
          XFreeColors(display, colmap,(pixels + offset_NB ), (*i), 0);

          COLORMAPLUT = FALSE;
#ifdef COL_DEBUG
  fprintf(stderr, 
  "DISPLAY.C-build_color_cmap(): error returned, %d places remain,
COLORMAPLUT = %d\n", (*i), COLORMAPLUT);
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
  fprintf(stderr, "DISPLAY.C-build_color_cmap(): valid return, %d
places used, COLORMAPLUT = %d\n",
	  (*i), COLORMAPLUT);
  fflush(stderr);
#endif
  return(COLORMAPLUT);
}

/*--------------------------------------------*/

void build_dither8_tables()
{
  int		i, j, k, err_range, threshval;
  int		r_values[256], g_values[256], b_values[256];
  u_char 	*tmp_darrays;

  for (i = 0; i < RR; i++)
    r_values[i] = (i * 256) / RR + 256/(int)(RR*2);
  for (i = 0; i < GR; i++)
    g_values[i] = (i * 256) / GR + 256/(int)(GR*2);
  for (i = 0; i < BR; i++)
    b_values[i] = (i * 256) / BR + 256/(int)(BR*2);


/*
** Generate a table entry for each possible colour
** value for every dither table location
*/

  for (i = 0; i < ORD_DITH_SZ; i++){
    tmp_darrays = r_darrays[i] = 
      (u_char *) sure_malloc(256,
	         "DISPLAY.C-build_dither8_tables() :cannot alloc r_darrays\n");
    err_range = r_values[0];
    threshval = ((i * err_range) / ORD_DITH_SZ);
    for (k = 0; k < r_values[0]; k++){
      *tmp_darrays++ = (k > threshval) ? GR*BR : 0;
    } /* rof k */
    for (j = 0; j < (RR-1); j++){
      err_range = r_values[j+1] - r_values[j];
      threshval = ((i * err_range) / ORD_DITH_SZ) + r_values[j];

      for (k = r_values[j]; k < r_values[j+1]; k++){
        *tmp_darrays++ = (k > threshval) ? ((j + 1)*GR*BR) : (j*GR*BR);
      } /* rof k */
    } /* rof j */
    for (j = r_values[RR-1]; j < 256; j++){
      *tmp_darrays++ = (RR-1)*GR*BR;
    } /* rof j */
  } /* rof i RED */


  for (i = 0; i < ORD_DITH_SZ; i++){
    tmp_darrays = g_darrays[i] = 
      (u_char *) sure_malloc(256,
	"DISPLAY.C-build_dither8_tables() :cannot alloc g_darrays\n");
    err_range = g_values[0];
    threshval = ((i * err_range) / ORD_DITH_SZ);
    for (k = 0; k < g_values[0]; k++){
      *tmp_darrays++ = (k > threshval) ? BR : 0;
    } /* rof k */
    for (j = 0; j < (GR-1); j++){
      err_range = g_values[j+1] - g_values[j];
      threshval = ((i * err_range) / ORD_DITH_SZ) + g_values[j];

      for (k = g_values[j]; k < g_values[j+1]; k++){
        *tmp_darrays++ = (k > threshval) ? ((j + 1)*BR) : (j*BR);
      } /* rof k */
    } /* rof j */
    for (j = g_values[GR-1]; j < 256; j++){
      *tmp_darrays++ = (GR-1)*BR;
    }  /* rof j */ 
  } /* rof i GREEN */

  for (i = 0; i < ORD_DITH_SZ; i++){
    tmp_darrays = b_darrays[i] = 
      (u_char *) sure_malloc(256,
	"DISPLAY.C-build_dither8_tables() : cannot alloc b_darrays\n");
    for (k = 0; k < b_values[0]; k++){
      *tmp_darrays++ =  0;
    } /* rof k */
    for (j = 0; j < (BR-1); j++){
      err_range = b_values[j+1] - b_values[j];
      threshval = ((i * err_range) / ORD_DITH_SZ) + b_values[j];

      for (k = b_values[j]; k < b_values[j+1]; k++){
        *tmp_darrays++ = (k > threshval) ? (j + 1) : j;
      } /* rof k */
    } /* rof j */
    for (j = b_values[BR-1]; j < 256; j++){
      *tmp_darrays++ = BR-1;
    } /* rof j */
      
  } /* rof i BLUE */

  build_rgb_tables();

} /*end build_dither8_tables */

/*--------------------------------------------*/

void build_dither4_tables()
{
  int		i, j, k, err_range, threshval;
  int		y_values[256];
  u_char 	*tmp_darrays;

  for (i = 0; i < (int)YR; i++)
    y_values[i] = (i * 220)/(int)YR + 220/(int)(YR*2) + 16;

/*
** Generate a table entry for each possible colour
** value for every dither table location
*/

  for (i = 0; i < ORD_DITH_SZ; i++){
    tmp_darrays = y_darrays[i] = 
      (u_char *) sure_malloc(256,
    	      "DISPLAY.C-build_dither4_tables() : cannot alloc y_darrays\n");
    for (k = 0; k < y_values[0]; k++){
      *tmp_darrays++ =  0;
    } /* rof k */
    for (j = 0; j < (YR-1); j++){
      err_range = y_values[j+1] - y_values[j];
      threshval = ((i * err_range) / (int)ORD_DITH_SZ) + y_values[j];

      for (k = y_values[j]; k < y_values[j+1]; k++){
        *tmp_darrays++ = ((int)(((double)k + (double)cur_brght)*cur_ctrst + 
				0.5) > threshval) ? j + 1 : j;
      } /* rof k */
    } /* rof j */
    for (j = y_values[YR-1]; j < 256; j++){
      *tmp_darrays++ = YR-1;
    }
  } /* rof i GREY */

  build_rgb_tables();

} /*end build_dither4_tables */

/*--------------------------------------------*/

#define BLANC 1
#define NOIR  0

void build_ditherBW_tables()
{
  int		i, j, k, err_range, threshval;
  int		y_values[2];
  u_char 	*tmp_darrays;

  y_values[0] = 220/4 + 16;
  y_values[1] = 220/2 + 220/4 + 16;

/*
** Generate a table entry for each possible colour
** value for every dither table location
*/

  for (i = 0; i < ORD_DITH_SZ; i++){
    tmp_darrays = y_darrays[i] = 
      (u_char *) sure_malloc(256,
	    "DISPLAY.C-build_ditherBW_tables() : cannot alloc y_darrays\n");

    for (j = 0; j < y_values[0]; j++){
      *tmp_darrays++ = NOIR;
    }
    err_range = y_values[1] - y_values[0];
    threshval = ((i * err_range) / ORD_DITH_SZ) + y_values[0];
    for (k = y_values[0]; k < y_values[1]; k++){
      *tmp_darrays++ = ((int)(((double)k + (double)cur_brght)*cur_ctrst + 0.5)
			> threshval) ? BLANC : NOIR;
    } /* rof k */
    for (j = y_values[1]; j < 256; j++){
      *tmp_darrays++ = BLANC;
    }
  } /* rof i B&W */
} /*end build_ditherBW_tables */

/*--------------------------------------------*/

void build_rgb_tables()
{
  int	i;
  double tmp_ind;

/*
**  Generates YUVccir ==> RGB translation coefficients tables :
**	cv2r[], cv2g[], cu2g[], cu2b[], cy2yrgb[]
**  
*/

    for(i = 16; i < Y_CCIR_MAX + 1; i++){
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
      cy2y24rgb[i] = XR_ECRETE_255(cy2yrgb[i]);
    } /* rof i */
    for(i = 0; i < 16; i++){
      cy2yrgb[i] = cy2yrgb[16];
      cy2y24rgb[i] = XR_ECRETE_255(cy2yrgb[i]);
      cv2r[i] = cv2r[16];
      cv2g[i] = cv2g[16];
      cu2g[i] = cu2g[16];
      cu2b[i] = cu2b[16];
    }/* rof i */
    for(i = Y_CCIR_MAX + 1; i < 256; i++){
      cv2r[i] = cv2r[Y_CCIR_MAX];
      cv2g[i] = cv2g[Y_CCIR_MAX];
      cu2g[i] = cu2g[Y_CCIR_MAX];
      cu2b[i] = cu2b[Y_CCIR_MAX];
    }/* rof i */
    for(i = Y_CCIR_MAX - 5; i < 256; i++){
      cy2yrgb[i] = cy2yrgb[Y_CCIR_MAX - 5];
      cy2y24rgb[i] = 255;
    }


#ifdef YRGB_DEBUG
  fprintf (stderr, 
  "DISPLAY.C-build_rgb_tables : la table cy2y24rgb contient les valeurs :\n");
  for (i = 0; i < 256; i++)
    fprintf (stderr," %d% ", cy2y24rgb[i]);
#endif


} /* end build_rgb_tables() */    

/*--------------------------------------------*/

void rebuild_map_lut()
{
  register int i, ri, gi, bi, yi;
  double tmp_ind;

  if((COLORMAPLUT || GREYMAPLUT) && (cur_brght == 0) && (cur_int_ctrst == 0)){
    for(i=0; i<stdmplt_size; i++) 
      map_lut[i] = std_map_lut[i];
    return;
  }

  if(GREYMAPLUT && depth_disp == 8){
    for (i=0; i < (int)YR; i++){
      yi = XR_ECRETE((YR-1),
          (int)(((double)y_map[i] + 
		 (double)cur_brght)*cur_ctrst*(double)(YR-1)/255.0 + 0.5));
      map_lut[i] = std_map_lut[yi];
    } /* rof i */
    for(i=16; i<236; i++){
      tmp_ind = (double)(i + 30)*(double)(YR-1)/235.0;
      y2rgb_conv[i] = map_lut[XR_ECRETE((int)YR-1, (int) tmp_ind)];
    } /* rof i */
  } /* fi GREYMAPLUT */

  if(COLORMAPLUT || (depth_disp == 24 || depth_disp==32)){

    for(i = 16; i < Y_CCIR_MAX + 1; i++){
      tmp_ind = ((double) i + (double)cur_brght)*cur_ctrst;
      tmp_ind = (tmp_ind + 5.0)*(double)(Y2RGB)/4096.0;
      cy2yrgb[i] = (int)(tmp_ind + 0.5);
      cy2y24rgb[i] = XR_ECRETE ( 255, cy2yrgb[i]);
    } /* rof i */
    for(i = 0; i < 16; i++){
      cy2yrgb[i] = cy2yrgb[16];
      cy2y24rgb[i] = XR_ECRETE ( 255, cy2yrgb[i]);
    }/* rof i */
    for(i = Y_CCIR_MAX - 5; i < 256; i++){
      cy2yrgb[i] = cy2yrgb[Y_CCIR_MAX - 5];
      cy2y24rgb[i] = 255;
    }/* rof i */
  } /* fi COLOR or depth == 24 */

  if(depth_disp == 4 && affCOLOR & MODGREY) build_dither4_tables();

  if(affCOLOR < MODGREY) build_ditherBW_tables();

} /* end rebuild_map_lut() */

/*--------------------------------------------------------------------------*/
