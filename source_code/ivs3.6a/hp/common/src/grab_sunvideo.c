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
*  File              : grab_sunvideo.c                                     *
*  Last modification : 1995/2/15                                           *
*  Author            : Atanu Ghosh, Thierry Turletti                       *
*--------------------------------------------------------------------------*
*  Description :  Initialization and grabbing functions for the            *
*                 SUNVIDEO framegrabber using xil library.                 *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#ifdef SUNVIDEO
#ident "@(#) grab_sunvideo.c 1.1@(#)"

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <xil/xil.h> 

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "grab_sunvideo.h"

#define CIF_WIDTH               352     /* Width of CIF */
#define CIF_HEIGHT              288     /* Height of CIF */

#define QCIF_HEIGHT (CIF_HEIGHT / 2)    /* Height of QCIF */
#define QCIF_WIDTH  (CIF_WIDTH / 2)     /* Width of QCIF */

#define COL_MAX                 352

/*
#define TIMING 
*/

/* Global variables not exported */

static XilImage rtvc_image, scaled_image;
static float xscale, yscale;
static int height, width;
static output_height, output_width; /* CIF or QCIF height/width values */
static XilImage dithered_image;
static XilLookup colorcube, yuv_to_rgb;
static XilDitherMask mask;
static XilColorspace cspace_ycc601, cspace_rgb709;
static float rescale_values[3];
static float offset_values[3];
static int h_r, w_r;
static int h_src_line[CIF_HEIGHT];
static int w_src_line[CIF_WIDTH];


/*--------- from display.c --------------*/

extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;


void sunvideo_get_color_pal(y_data, u_data, v_data)
     u_char y_data[][COL_MAX];

     u_char u_data[][COL_MAX_DIV2];
     u_char v_data[][COL_MAX_DIV2];
{
  XilMemoryStorage dst;
  u_char *ptr0, *pt_y0, *pt_u0, *pt_v0;
  register int l, c;
  register u_char *ptr, *pt_y, *pt_u, *pt_v;
#ifdef TIMING  
  struct timeval stv, etv;

  gettimeofday(&stv);
#endif  
  xil_scale(rtvc_image, scaled_image, "nearest", xscale, yscale);
  if(XIL_FAILURE == xil_export(scaled_image)){        
    printf("failed to export image\n");
    exit(1);
  }
  xil_get_memory_storage(scaled_image, &dst);

  ptr = ptr0 = dst.byte.data;
  pt_y = pt_y0 = &y_data[0][0];
  pt_u = pt_u0 = &u_data[0][0];
  pt_v = pt_v0 = &v_data[0][0]; 
	
  for(l=0; l<height; l++){
    if(l % 2 == 0){
      for(c=0; c<width; c++){
	*pt_y++ = *ptr;
	if(c % 2 == 0){
	  *pt_u++ = *(ptr+1);
	  *pt_v++ = *(ptr+2);
	}
	ptr += dst.byte.pixel_stride;
      }
      pt_u0 += COL_MAX_DIV2;
      pt_v0 += COL_MAX_DIV2;
      pt_u = pt_u0;
      pt_v = pt_v0;
    }else{
      for(c=0; c<width; c++){
	*pt_y++ = *ptr;
	ptr += dst.byte.pixel_stride;
      }
    }
    ptr0 += dst.byte.scanline_stride;
    ptr = ptr0;
    pt_y0 += COL_MAX;
    pt_y = pt_y0;
  }
  xil_import(scaled_image, 1);
#ifdef COLOR_DITHERING
  xil_ordered_dither(scaled_image, dithered_image, colorcube, mask);
#else
  xil_sync(scaled_image);
#endif
  xil_toss(scaled_image);
#ifdef TIMING  
  gettimeofday(&etv);
  etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
  fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif
}



void sunvideo_get_color_ntsc(y_data, u_data, v_data)
     u_char y_data[][COL_MAX];

     u_char u_data[][COL_MAX_DIV2];
     u_char v_data[][COL_MAX_DIV2];
{
  static BOOLEAN FIRST=TRUE;
  XilMemoryStorage dst;
  u_char *ptr0, *pt_y0, *pt_u0, *pt_v0;
  register int l, c;
  register u_char *ptr, *pt_y, *pt_u, *pt_v;
#ifdef TIMING  
  struct timeval stv, etv;

  gettimeofday(&stv);
#endif  

  xil_scale(rtvc_image, scaled_image, "nearest", xscale, yscale);
  if(XIL_FAILURE == xil_export(scaled_image)){        
    printf("failed to export image\n");
    exit(1);
  }
  xil_get_memory_storage(scaled_image, &dst);

  if(FIRST){
    register int l, c;
    
    for(l=0; l<output_height; l++) {
      h_src_line[l] = dst.byte.scanline_stride * ((l * h_r) / output_height);
    }
    for(c=0; c<output_width; c++) {
      w_src_line[c] = dst.byte.pixel_stride * ((c * w_r) / output_width);

    }
    FIRST = FALSE;
  }
  
  ptr = ptr0 = dst.byte.data;
  pt_y = pt_y0 = &y_data[0][0];
  pt_u = pt_u0 = &u_data[0][0];
  pt_v = pt_v0 = &v_data[0][0]; 
	
  for(l=0; l<output_height; l++){
    if(l % 2 == 0){
      for(c=0; c<output_width; c++){
	*pt_y++ = *ptr;
	if(c % 2 == 0){
	  *pt_u++ = *(ptr+1);
	  *pt_v++ = *(ptr+2);
	}
	ptr = ptr0 + w_src_line[c];
      }
      pt_u0 += COL_MAX_DIV2;
      pt_v0 += COL_MAX_DIV2;
      pt_u = pt_u0;
      pt_v = pt_v0;
    }else{
      for(c=0; c<output_width; c++){
	*pt_y++ = *ptr;
	ptr = ptr0 + w_src_line[c];
      }
    }
    ptr0 = dst.byte.data + h_src_line[l];
    ptr = ptr0;
    pt_y0 += COL_MAX;
    pt_y = pt_y0;
  }
  xil_import(scaled_image, 1);
#ifdef COLOR_DITHERING
  xil_ordered_dither(scaled_image, dithered_image, colorcube, mask);
#else
  xil_sync(scaled_image);
#endif
  xil_toss(scaled_image);
#ifdef TIMING  
  gettimeofday(&etv);
  etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
  fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif
}



void sunvideo_get_grey_pal(y_data)
     u_char y_data[][COL_MAX];
{
  XilMemoryStorage dst;
  u_char *ptr0, *pt_y0;
  register int l, c;
  register u_char *ptr, *pt_y;
#ifdef TIMING  
  struct timeval stv, etv;

  gettimeofday(&stv);
#endif  
  xil_scale(rtvc_image, scaled_image, "nearest", xscale, yscale);
  xil_ordered_dither(scaled_image, dithered_image, colorcube, mask);
    
  if(XIL_FAILURE == xil_export(dithered_image)){        
    printf("failed to export image\n");
    exit(1);
  } 
  xil_get_memory_storage(dithered_image, &dst);
  
  ptr = ptr0 = dst.byte.data;
  pt_y = pt_y0 = &y_data[0][0];
	
  for(l=0; l<output_height; l++){
    for(c=0; c<output_width; c++){
      *pt_y++ = *ptr;
      ptr += dst.byte.pixel_stride;
    }
    ptr0 += dst.byte.scanline_stride;
    ptr = ptr0;
    pt_y0 += COL_MAX;
    pt_y = pt_y0;
  }
  xil_import(dithered_image, 1);
  xil_toss(scaled_image);
#ifdef TIMING  
  gettimeofday(&etv);
  etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
  fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif  
}	



void sunvideo_get_grey_ntsc(y_data)
     u_char y_data[][COL_MAX];
{
  static BOOLEAN FIRST=TRUE;
  XilMemoryStorage dst;
  u_char *ptr0, *pt_y0;
  register int l, c;
  register u_char *ptr, *pt_y;
		    
#ifdef TIMING  
  struct timeval stv, etv;

  gettimeofday(&stv);
#endif  
  xil_scale(rtvc_image, scaled_image, "nearest", xscale, yscale);
  xil_ordered_dither(scaled_image, dithered_image, colorcube, mask);
    
  if(XIL_FAILURE == xil_export(dithered_image)){        
    printf("failed to export image\n");
    exit(1);
  } 
  xil_get_memory_storage(dithered_image, &dst);
  
  if(FIRST){
    register int l, c;
    
    for(l=0; l<output_height; l++) {
      h_src_line[l] = dst.byte.scanline_stride * ((l * h_r) / output_height);
    }
    for(c=0; c<output_width; c++) {
      w_src_line[c] = dst.byte.pixel_stride * ((c * w_r) / output_width);
    }
    FIRST = FALSE;
  }
  
  ptr = ptr0 = dst.byte.data;
  pt_y = pt_y0 = &y_data[0][0];
	
  for(l=0; l<output_height; l++){
    for(c=0; c<output_width; c++){
      *pt_y++ = *ptr;
      ptr = ptr0 + w_src_line[c];
    }
    ptr0 = dst.byte.data + h_src_line[l];
    ptr = ptr0;
    pt_y0 += COL_MAX;
    pt_y = pt_y0;
  }
  xil_import(dithered_image, 1);
  xil_toss(scaled_image);
#ifdef TIMING  
  gettimeofday(&etv);
  etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
  fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif  
}	





/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

void SetSunVideoPort(port_video)
     int port_video;
{
  int status;
  
  if((status = xil_set_device_attribute(rtvc_image, "PORT_V",
					(void *) ((port_video+1)%3))) ==

     XIL_FAILURE){
    fprintf(stderr, "Setting PORT_V attribute failed\n");
  }
}



void SunVideoInitialize(state, port_video, size, COLOR, NG_MAX, L_lig, L_col,
			Lcoldiv2, L_col2, L_lig2, get_image)
     
     XilSystemState state;
     int port_video;
     int size;
     BOOLEAN COLOR;
     int *NG_MAX, *L_lig, *L_col, *Lcoldiv2, *L_col2, *L_lig2;
     void (**get_image)();
{
  XilImage rtvc_luma, luma_image, single_band1, single_band2;
  XilDataType datatype;
  int nbands;
  int h, w, status;
  int max_buffers = 1;
  BOOLEAN PAL_FORMAT=FALSE;
  
  /*
  ** xil calls
  */
  if((rtvc_image = xil_create_from_device(state, "SUNWrtvc", NULL))
     == NULL) {
    fprintf(stderr, "failed to open SUNWrtvc device\n");
    xil_close(state);
    exit(1);
  }
  
  if((status = xil_set_device_attribute(rtvc_image, "PORT_V",
					(void *) ((port_video+1)%3))) ==
     XIL_FAILURE){
    fprintf(stderr, "Setting PORT_V attribute failed\n");
  }

  if((status = xil_set_device_attribute(rtvc_image,"MAX_BUFFERS", 
				       (void *)max_buffers)) ==
     XIL_FAILURE){
    fprintf(stderr, "Setting MAX_BUFFERS attribute failed\n");
  }

  xil_get_info(rtvc_image, (u_int *)&w, (u_int *)&h,
	       (u_int *) &nbands, &datatype);

  if(size == QCIF_SIZE){
    *NG_MAX = 5;
    *L_lig = output_height = QCIF_HEIGHT;
    *L_col = output_width = QCIF_WIDTH;
    h_r = h / 4;
    w_r = w / 4;
    xscale = 0.25;
    yscale = 0.25;
  }else{
    *NG_MAX = 12;
    *L_lig = output_height = CIF_HEIGHT;
    *L_col = output_width = CIF_WIDTH;
    h_r = h / 2;
    w_r = w / 2;
    xscale = 0.5;
    yscale = 0.5;
  }
  
  *Lcoldiv2 = *L_col >> 1;
  *L_col2 = *L_col << 1;
  *L_lig2 = *L_lig << 1;
  
  width = (int)(w * xscale);
  height = (int)(h * yscale);
  
  if(!COLOR){
    encCOLOR = FALSE;
    affCOLOR = MODGREY;
  }else{
    encCOLOR = TRUE;
    affCOLOR = MODCOLOR;
  }

  if(h == PAL_HEIGHT && w == PAL_WIDTH){
    PAL_FORMAT = TRUE;
  }

  if((scaled_image = xil_create(state, width, height, nbands, XIL_BYTE))
     == NULL) {
    fprintf(stderr, "xil_create scaled_image failed\n");
    exit(1);
    return;
  }

  if(COLOR)
    *get_image = (PAL_FORMAT ? sunvideo_get_color_pal :
		  sunvideo_get_color_ntsc);
  else{
    XilLookup greyramp;
    Xil_unsigned8* greydata = malloc(3 * 256);
    int num_entries = 256;
    int offset = -2;
    float cmap_range = 1.0;
    unsigned int dim[3] = {256,1,1};     

    int mult[3] = {1,256,256};
    float dmask[] =
      {
	1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
	
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      };
    
    *get_image = (PAL_FORMAT ? sunvideo_get_grey_pal : sunvideo_get_grey_ntsc);

    get_dither_info(state, rescale_values, offset_values, &mask,
		    &yuv_to_rgb, &colorcube);
    
    colorcube = xil_colorcube_create(state, XIL_BYTE, XIL_BYTE,
				     3, 0, mult, dim);
    
    mask = xil_dithermask_create(state, 4, 4, 3, dmask);
    
    rescale_values[0] = cmap_range;
    rescale_values[1] = cmap_range;
    rescale_values[2] = cmap_range;
    offset_values[0] = (float)offset;
    offset_values[1] = (float)offset;
    offset_values[2] = (float)offset;

    dithered_image = xil_create(state, width, height, 1, XIL_BYTE);

  }
#ifdef COLOR_DITHERING
  get_dither_info(state, rescale_values, offset_values, &mask,
		  &yuv_to_rgb, &colorcube);
  
  colorcube  = xil_lookup_get_by_name(state, "cc496");
  
  cspace_ycc601 = xil_colorspace_get_by_name(state, "ycc601");
  
  xil_set_colorspace(scaled_image, cspace_ycc601);
  
  dithered_image = xil_create(state, width, height, 1, XIL_BYTE);
#endif /* COLOR_DITHERING */
  
}


#endif	/* SUNVIDEO */

