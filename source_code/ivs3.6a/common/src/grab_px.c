/**************************************************************************\
 *          Copyright (c) 1995 INRIA Rocquencourt FRANCE.                   *
 *                                                                          *
 * Permission to use, copy, modify, and distribute this material for any    *
 * purpose and without fee is hereby granted, provided that the above       *
 * copyright notice and this permission notice appear in all copies.        *
 * WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
 * MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
 * OR IMPLIED WARRANTIES.                                                   *
 \**************************************************************************/
/***************************************************************************\
 * 	                						    *
 *  File              : grab_px.c                          	            *
 *  Author            : Andrzej Wozniak				 	    *
 *  Last Modification : 1995/2/15                                           *
 *--------------------------------------------------------------------------*
 *  Description       : parallax card grabbing                              *
 *                                                                          *
 *--------------------------------------------------------------------------*
 *        Name	        |    Date   |          Modification                 *
 *--------------------------------------------------------------------------*
 \**************************************************************************/
#include <sys/types.h>
#include <X11/Xlib.h>
#include <fcntl.h>
#include <stdio.h>
#include "grab_px.h"
#include "px_macro.h"
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern int wait_debug;

extern int   red_2_YCrCb[2*256];
extern int green_2_YCrCb[2*256];
extern int  blue_2_YCrCb[2*256];

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*                        FRAME GRABBING PROCEDURES                          */
/*---------------------------------------------------------------------------*/
#ifndef GRAB_FAST
/*-------------------------------------------------------------------*/

#ifdef NO_SQUEEZE_PX
void SUN_PX_CIF4_FULL(px_port, y_data)
     PXport *px_port;
     u_char y_data[][LIG_MAX][COL_MAX];
{
  register  u_int *src_pixel_ptr;
  register TYPE_A y;
  register u_int px_tmp;
  register int height;
  register int width;
  register u_char *y0, *y1;
  int select;
#ifdef COLOR_MAP
  register TYPE_A  m;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
#endif
  
#ifdef GRAB_FAST
  aw_select_Y();
#endif
  
  src_pixel_ptr = px_port->fb_window_addr;
#ifdef GRAB_NTSC
  y0 = &y_data[0][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER;
  y1 = &y_data[1][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH;
#else
  y0 = &y_data[0][0][0];
  y1 = &y_data[1][0][0];
#endif
  
  select = 2;
  
  do{
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC;
#else
    height = CIF_HEIGHT;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(4*STEP); 
#else
      width = CIF_WIDTH/(4*STEP);
#endif
      
      do{
	monoCIF4_RD_16_PIX_XXX(y0, 0*STEP);
	monoCIF4_RD_16_PIX_XXX(y0, 2*STEP);
	y0+=4*STEP;
	src_pixel_ptr+=4*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(4*STEP); 
#else
      width = CIF_WIDTH/(4*STEP);
#endif
      do{
	monoCIF4_RD_16_PIX_XXX(y1, 0*STEP);
	monoCIF4_RD_16_PIX_XXX(y1, 2*STEP);
	y1+=4*STEP;
	src_pixel_ptr+=4*STEP;    
      }while(--width);
#ifdef GRAB_NTSC
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      y0+= CIF4_NTSC_BORDER;
      y1+= CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
    if(!(--select))
      break;
#ifdef GRAB_NTSC
    y0 = &y_data[2][0][0] + CIF4_NTSC_BORDER;
    y1 = &y_data[3][0][0];
#else
    y0 = &y_data[2][0][0];
    y1 = &y_data[3][0][0];
#endif
  } while(1);
  
}
#endif
/*---------------------------------------------------------------------------*/
#if defined(HALF_RESOLUTION_CIF4) && defined(NO_SQUEEZE_PX)

void SUN_PX_CIF4_HALF(px_port, y_data)
     PXport *px_port;
     u_char y_data[][LIG_MAX][COL_MAX];
{
  register  u_int *src_pixel_ptr;
  register TYPE_A y;
  register u_int px_tmp;
  register int height;
  register int width;
  register u_char *y0, *y1;
  int select;
#ifdef COLOR_MAP
  register TYPE_A  m;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
#endif
  
#ifdef GRAB_FAST
  aw_select_Y();
#endif
  
  src_pixel_ptr = px_port->fb_window_addr;
#ifdef GRAB_NTSC
  y0 = &y_data[0][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER;
  y1 = &y_data[1][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH;
#else
  y0 = &y_data[0][0][0];
  y1 = &y_data[1][0][0];
#endif
  
  select = 2;
  
  do{
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC/2;
#else
    height = CIF_HEIGHT/2;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(4*STEP);
#else
      width = CIF_WIDTH/(4*STEP);
#endif
      
      do{
	monoCIF4_RD_16_PIX_XXX(y0, 0*STEP);
	monoCIF4_RD_16_PIX_XXX(y0, 2*STEP);
	y0+=4*STEP;
	src_pixel_ptr+=4*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(4*STEP);
#else
      width = CIF_WIDTH/(4*STEP);
#endif
      do{
	monoCIF4_RD_16_PIX_XXX(y1, 0*STEP);
	monoCIF4_RD_16_PIX_XXX(y1, 2*STEP);
	y1+=4*STEP;
	src_pixel_ptr+=4*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width=CIF_WIDTH_NTSC/32;
#else
      width=CIF_WIDTH/32;
#endif
      do{
	R_COPY16((y0-(CIF_WIDTH)), y0, 0);
	R_COPY16((y0-(CIF_WIDTH)), y0, 16);
	R_COPY16((y1-(CIF_WIDTH)), y1, 0);
	R_COPY16((y1-(CIF_WIDTH)), y1, 16);
	y0 += 32;
	y1 += 32;
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      y0+= CIF4_NTSC_BORDER;
      y1+= CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
    if(!(--select))
      break;
#ifdef GRAB_NTSC
    y0 = &y_data[2][0][0] + CIF4_NTSC_BORDER;
    y1 = &y_data[3][0][0];
#else
    y0 = &y_data[2][0][0];
    y1 = &y_data[3][0][0];
#endif
  } while(1);
  
}
#endif
/*---------------------------------------------------------------------------*/

void SUN_PX_CIF(px_port, y_data)
     PXport *px_port;
     register     u_char *y_data;
{
  register  u_int *src_pixel_ptr;
  register TYPE_A y;
  register u_int px_tmp;
  register int height;
  register int width;
#ifdef COLOR_MAP
  register TYPE_A  m;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
#endif
  static  int loop;
  
#ifdef GRAB_FAST
  aw_select_Y();
#endif
  
  src_pixel_ptr = px_port->fb_window_addr;
  
#ifdef GRAB_NTSC
  y_data += CIF_NTSC_UBORDER*CIF_WIDTH + CIF_NTSC_BORDER;
  height = CIF_HEIGHT_NTSC;
#else
  height = CIF_HEIGHT;
#endif
  
  do{
#ifdef GRAB_NTSC
    width = CIF_WIDTH_NTSC/(4*STEP);
#else
    width = CIF_WIDTH/(4*STEP);
#endif
    do{
      monoCIF_RD_16_PIX_XXX(y_data, 0);
      monoCIF_RD_16_PIX_XXX(y_data, 2*STEP);
      y_data+=4*STEP;
      src_pixel_ptr+=4*STEP*CIF_STEP;    
    }while(--width);
#ifdef GRAB_NTSC
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH_NTSC*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
    y_data+= CIF_WIDTH - CIF_WIDTH_NTSC;
#else
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
#endif
  }while (--height);
}


/*-------------------------------------------------------------------*/

void SUN_PX_QCIF(px_port, y_data)
     PXport *px_port;
     register  u_char *y_data;
{
  register  u_int *src_pixel_ptr;
  register TYPE_A y;
  register u_int px_tmp;
  register int height;
  register int width;
#ifdef COLOR_MAP
  register TYPE_A  m;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
#endif
  
  
#ifdef GRAB_FAST
  aw_select_Y();
#endif
  
  src_pixel_ptr = px_port->fb_window_addr;
  
#ifdef GRAB_NTSC
  y_data += QCIF_NTSC_UBORDER*CIF_WIDTH + QCIF_NTSC_BORDER;
  height = QCIF_HEIGHT_NTSC;
#else
  height = QCIF_HEIGHT;
#endif
  NNOP;  
  do{
#ifdef GRAB_NTSC
    width = QCIF_WIDTH/(2*STEP);
#else
    width = QCIF_WIDTH/(2*STEP);
#endif
    do{
      monoQCIF_RD_16_PIX_XXX(y_data, 0);
      y_data+=2*STEP;
      src_pixel_ptr+=2*STEP*QCIF_STEP;    
    }while(--width);
#ifdef GRAB_NTSC
    src_pixel_ptr += 
      PX_FB_WIDTH - QCIF_WIDTH_NTSC*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_data+= CIF_WIDTH - QCIF_WIDTH_NTSC ;
#else
    src_pixel_ptr += 
      PX_FB_WIDTH - QCIF_WIDTH*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_data+= CIF_WIDTH - QCIF_WIDTH;
#endif
  }while (--height);
  
}

/*---------------------------------------------------------------------------*/

#ifdef NO_SQUEEZE_PX
void SUN_PX_CIF4_COLOR_FULL(px_port, y_data, u_data, v_data)
     PXport *px_port;
     u_char y_data[][LIG_MAX][COL_MAX];
     u_char u_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     u_char v_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     
{
  register  u_int *src_pixel_ptr;
  register TYPE_A m, x, y, cr, cb;
  register u_int px_tmp;
  /*  register */ int height;
  /*  register */ int width;
  register u_char *y0, *y1;
  register u_char *u0, *v0;
  register u_char *u1, *v1;
  /*  register */ int select;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
  
  
  
  src_pixel_ptr = px_port->fb_window_addr;
#ifdef GRAB_NTSC
  y0 = &y_data[0][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER;
  y1 = &y_data[1][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH;
  
  u0 = &u_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2 + CIF4_NTSC_BORDER)/2;
  u1 = &u_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
  
  v0 = &v_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2 + CIF4_NTSC_BORDER)/2; 
  v1 = &v_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
#else
  y0 = &y_data[0][0][0];
  y1 = &y_data[1][0][0];
  
  u0 = &u_data[0][0][0]; 
  u1 = &u_data[1][0][0]; 
  
  v0 = &v_data[0][0][0]; 
  v1 = &v_data[1][0][0]; 
#endif
  
  select = 2;
  
  do{
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC/2;
#else
    height = CIF_HEIGHT/2;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	colorCIF4_RD_16_PIX_CCC(y0, u0, v0, 0);
	u0+=1*STEP;
	v0+=1*STEP;
	y0+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
      u0 += CIF4_NTSC_BORDER/2;
      v0 += CIF4_NTSC_BORDER/2;
      y0 += CIF4_NTSC_BORDER;
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	colorCIF4_RD_16_PIX_CCC(y1, u1, v1, 0);
	u1+=1*STEP;
	v1+=1*STEP;
	y1+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
#ifdef GRAB_NTSC
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      u1 += CIF4_NTSC_BORDER/2;
      v1 += CIF4_NTSC_BORDER/2;
      y1 += CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	monoCIF4_RD_16_PIX_CCC(y0, 0);
	y0+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
      y0 += CIF4_NTSC_BORDER;
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	monoCIF4_RD_16_PIX_CCC(y1, 0);
	y1+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
#ifdef GRAB_NTSC
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      y1 += CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
    if(!(--select))
      break;
#ifdef GRAB_NTSC
    y0 = &y_data[2][0][0] + CIF4_NTSC_BORDER;
    y1 = &y_data[3][0][0];
    
    u0 = &u_data[2][0][0] + CIF4_NTSC_BORDER/2; 
    u1 = &u_data[3][0][0]; 
    
    v0 = &v_data[2][0][0] + CIF4_NTSC_BORDER/2; 
    v1 = &v_data[3][0][0]; 
#else
    y0 = &y_data[2][0][0];
    y1 = &y_data[3][0][0];
    
    u0 = &u_data[2][0][0]; 
    u1 = &u_data[3][0][0]; 
    
    v0 = &v_data[2][0][0]; 
    v1 = &v_data[3][0][0]; 
#endif
  } while(1);
  
}
#endif
/*---------------------------------------------------------------------------*/
#if defined(HALF_RESOLUTION_CIF4) && defined(NO_SQUEEZE_PX)

void SUN_PX_CIF4_COLOR_HALF(px_port, y_data, u_data, v_data)
     PXport *px_port;
     u_char y_data[][LIG_MAX][COL_MAX];
     u_char u_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     u_char v_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     
{
  register  u_int *src_pixel_ptr;
  register TYPE_A m, x, y, cr, cb;
  register u_int px_tmp;
  /*  register */ int height;
  /*  register */ int width;
  register u_char *y0, *y1;
  register u_char *u0, *v0;
  register u_char *u1, *v1;
  /*  register */ int select;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
  
  
  
  src_pixel_ptr = px_port->fb_window_addr;
#ifdef GRAB_NTSC
  y0 = &y_data[0][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER;
  y1 = &y_data[1][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH;
  
  u0 = &u_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2 + CIF4_NTSC_BORDER)/2;
  u1 = &u_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
  
  v0 = &v_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER)/2; 
  v1 = &v_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
#else
  y0 = &y_data[0][0][0];
  y1 = &y_data[1][0][0];
  
  u0 = &u_data[0][0][0]; 
  u1 = &u_data[1][0][0]; 
  
  v0 = &v_data[0][0][0]; 
  v1 = &v_data[1][0][0]; 
#endif
  
  select = 2;
  
  do{
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC/2;
#else
    height = CIF_HEIGHT/2;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	colorCIF4_RD_16_PIX_CCC(y0, u0, v0, 0);
	u0+=1*STEP;
	v0+=1*STEP;
	y0+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
      u0 += CIF4_NTSC_BORDER/2;
      v0 += CIF4_NTSC_BORDER/2;
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      
      do{
	colorCIF4_RD_16_PIX_CCC(y1, u1, v1, 0);
	u1+=1*STEP;
	v1+=1*STEP;
	y1+=2*STEP;
	src_pixel_ptr+=2*STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      u1 += CIF4_NTSC_BORDER/2;
      v1 += CIF4_NTSC_BORDER/2;
      y1 += CIF4_NTSC_BORDER;
#endif
      
      width=CIF_WIDTH/32;
      width=CIF_WIDTH/32;
      do{
	R_COPY16((y0-(CIF_WIDTH)), y0, 0);
	R_COPY16((y0-(CIF_WIDTH)), y0, 16);
	R_COPY16((y1-(CIF_WIDTH)), y1, 0);
	R_COPY16((y1-(CIF_WIDTH)), y1, 16);
	y0 += 32;
	y1 += 32;
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      y0 += CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
    if(!(--select))
      break;
#ifdef GRAB_NTSC
    y0 = &y_data[2][0][0] + CIF4_NTSC_BORDER;
    y1 = &y_data[3][0][0];
    
    u0 = &u_data[2][0][0] + CIF4_NTSC_BORDER/2; 
    u1 = &u_data[3][0][0]; 
    
    v0 = &v_data[2][0][0] + CIF4_NTSC_BORDER/2; 
    v1 = &v_data[3][0][0]; 
#else
    y0 = &y_data[2][0][0];
    y1 = &y_data[3][0][0];
    
    u0 = &u_data[2][0][0]; 
    u1 = &u_data[3][0][0]; 
    
    v0 = &v_data[2][0][0]; 
    v1 = &v_data[3][0][0]; 
#endif
  } while(1);
  
}
#endif
/*-------------------------------------------------------------------*/

void SUN_PX_CIF_COLOR(px_port, y_data, u_data, v_data)
     PXport *px_port;
     register     u_char *y_data, *u_data, *v_data;
{
  register  u_int *src_pixel_ptr;
  register TYPE_A m, x, y, cr, cb;
  register u_int px_tmp;
  register int height;
  register int width;
  
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
  
  
  src_pixel_ptr = px_port->fb_window_addr;
  
#ifdef GRAB_NTSC
  y_data += CIF_NTSC_UBORDER*CIF_WIDTH + CIF_NTSC_BORDER;
  u_data += (CIF_NTSC_UBORDER*CIF_WIDTH/2 + CIF_NTSC_BORDER)/2;
  v_data += (CIF_NTSC_UBORDER*CIF_WIDTH/2 + CIF_NTSC_BORDER)/2;
  height = CIF_HEIGHT_NTSC/2;
#else
  height = CIF_HEIGHT/2;
#endif
  
  do{
#ifdef GRAB_NTSC
    width = CIF_WIDTH_NTSC/(2*STEP);
#else
    width = CIF_WIDTH/(2*STEP);
#endif
    do{
      colorCIF_RD_16_PIX_CCC(y_data, u_data, v_data, 0);
      u_data+=1*STEP;
      v_data+=1*STEP;
      y_data+=2*STEP;
      src_pixel_ptr+=2*STEP*CIF_STEP;    
    }while(--width);
    
#ifdef GRAB_NTSC
    width = CIF_WIDTH_NTSC/(2*STEP);
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH_NTSC*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
    y_data +=  CIF_WIDTH - CIF_WIDTH_NTSC;
    u_data +=  (CIF_WIDTH - CIF_WIDTH_NTSC)/2;
    v_data +=  (CIF_WIDTH - CIF_WIDTH_NTSC)/2;
#else
    width = CIF_WIDTH/(2*STEP);
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
#endif
    do{
      monoCIF_RD_16_PIX_CCC(y_data, 0);
      y_data+=2*STEP;
      src_pixel_ptr+=2*STEP*CIF_STEP;    
    }while(--width);
    
#ifdef GRAB_NTSC
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH_NTSC*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
    y_data +=  CIF_WIDTH - CIF_WIDTH_NTSC;
#else
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
#endif
  }while (--height);
  
}

/*-------------------------------------------------------------------*/

void SUN_PX_QCIF_COLOR(px_port, y_data, u_data, v_data)
     PXport *px_port;
     register     u_char *y_data, *u_data, *v_data;
{
  register  u_int *src_pixel_ptr;
  register TYPE_A m, x, y, cr, cb;
  register u_int px_tmp;
  register int height;
  register int width;
  register u_char *map_r = (u_char *)  &red_2_YCrCb[0];
  register u_char *map_g = (u_char *)&green_2_YCrCb[0];
  register u_char *map_b = (u_char *) &blue_2_YCrCb[0];
  
  
  
  src_pixel_ptr = px_port->fb_window_addr;
  
#ifdef GRAB_NTSC
  y_data += QCIF_NTSC_UBORDER*CIF_WIDTH + QCIF_NTSC_BORDER;
  u_data += (QCIF_NTSC_UBORDER*CIF_WIDTH/2 + QCIF_NTSC_BORDER)/2;
  v_data += (QCIF_NTSC_UBORDER*CIF_WIDTH/2 + QCIF_NTSC_BORDER)/2;
  height = QCIF_HEIGHT_NTSC;
#else
  height = QCIF_HEIGHT;
#endif
  
  do{
#ifdef GRAB_NTSC
    width = QCIF_WIDTH_NTSC/(2*STEP);
#else
    width = QCIF_WIDTH/(2*STEP);
#endif
    if(height & 1){
      do{
	monoQCIF_RD_16_PIX_CCC(y_data, 0);
	y_data+=2*STEP;
	src_pixel_ptr+=2*STEP*QCIF_STEP;
      }while(--width);
    }else{
      do{
	colorQCIF_RD_16_PIX_CCC(y_data, u_data, v_data, 0);
	u_data+=1*STEP;
	v_data+=1*STEP;
	y_data+=2*STEP;
	src_pixel_ptr+=2*STEP*QCIF_STEP;    
      }while(--width);
#ifdef GRAB_NTSC
      u_data += CIF_WIDTH/2 - QCIF_WIDTH_NTSC/2;
      v_data += CIF_WIDTH/2 - QCIF_WIDTH_NTSC/2;
#else
      u_data += CIF_WIDTH/2 - QCIF_WIDTH/2;
      v_data += CIF_WIDTH/2 - QCIF_WIDTH/2;
#endif
    }
#ifdef GRAB_NTSC
    src_pixel_ptr += 
      PX_FB_WIDTH - QCIF_WIDTH_NTSC*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_data += CIF_WIDTH - QCIF_WIDTH_NTSC;
#else
    src_pixel_ptr += 
      PX_FB_WIDTH - QCIF_WIDTH*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_data += CIF_WIDTH - QCIF_WIDTH;
#endif
  }while (--height);
  
}
/*-------------------------------------------------------------------*/
#else /* GRAB_FAST */
/*-------------------------------------------------------------------*/
#ifdef NO_SQUEEZE_PX
FAST_SUN_PX_CIF4_COLOR_FULL_NLD(px_port, y_data, u_data, v_data)
     PXport *px_port;
     u_char y_data[][LIG_MAX][COL_MAX];
     u_char u_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     u_char v_data[][LIG_MAX_DIV2][COL_MAX_DIV2];
     
{
  register  u_char *src_pixel_ptr;
  register u_int tmp, y, m;
  register  int height;
  register  int width;
  register u_char *y0, *y1;
  register int select;
  
#define u0 y0
#define v0 y0
#define u1 y1
#define v1 y1
  
  aw_select_Y();
  
  src_pixel_ptr = px_port->fb_window_addrf;

#ifdef GRAB_NTSC
  y0 = &y_data[0][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH + CIF4_NTSC_BORDER;
  y1 = &y_data[1][0][0] + CIF4_NTSC_UBORDER*CIF_WIDTH;
#else
  y0 = &y_data[0][0][0];
  y1 = &y_data[1][0][0];
#endif
  
  select = 2;
  do{
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC;
#else
    height = CIF_HEIGHT;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP);
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	FY_CIF4_RD_16_PIX_XXX(y0, src_pixel_ptr, 0);
	y0+=2*STEP;
	src_pixel_ptr+=2*STEP;
      }while(--width);
      
#ifdef GRAB_NTSC
      y0 += CIF4_NTSC_BORDER;
      width = CIF_WIDTH_NTSC/(2*STEP);
#else
      width = CIF_WIDTH/(2*STEP);
#endif
      do{
	FY_CIF4_RD_16_PIX_XXX(y1, src_pixel_ptr, 0);
	y1+=2*STEP;
	src_pixel_ptr+=2*STEP;
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      y1 += CIF4_NTSC_BORDER;
#else
      src_pixel_ptr += PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
    
    if(!(--select))
      break;
#ifdef GRAB_NTSC
    y0 = &y_data[2][0][0] + CIF4_NTSC_BORDER;
    y1 = &y_data[3][0][0];
#else
    y0 = &y_data[2][0][0];
    y1 = &y_data[3][0][0];
#endif
  }while(1);
  
  if(!px_port->color)
    return;
  /*-----------------*/
  
  select = 4; do{
    switch(select){
    case 4:
      src_pixel_ptr = px_port->fb_window_addrf;
#ifdef GRAB_NTSC
      u0 = &u_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2 + CIF4_NTSC_BORDER)/2;
      u1 = &u_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
#else
      u0 = &u_data[0][0][0]; 
      u1 = &u_data[1][0][0]; 
#endif
      aw_select_Cb();
      break;
    case 3:
#ifdef GRAB_NTSC
      u0 = &u_data[2][0][0] + CIF4_NTSC_BORDER/2; 
      u1 = &u_data[3][0][0]; 
#else
      u0 = &u_data[2][0][0]; 
      u1 = &u_data[3][0][0]; 
#endif
      break;
    case 2:
      src_pixel_ptr = px_port->fb_window_addrf;
#ifdef GRAB_NTSC
      v0 = &v_data[0][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2 + CIF4_NTSC_BORDER)/2;
      v1 = &v_data[1][0][0] + (CIF4_NTSC_UBORDER*CIF_WIDTH/2)/2; 
#else
      v0 = &v_data[0][0][0]; 
      v1 = &v_data[1][0][0]; 
#endif
      aw_select_Cr();
      break;
    case 1:
#ifdef GRAB_NTSC
      v0 = &v_data[2][0][0] + CIF4_NTSC_BORDER/2; 
      v1 = &v_data[3][0][0]; 
#else
      v0 = &v_data[2][0][0]; 
      v1 = &v_data[3][0][0]; 
#endif
    }
    
#ifdef GRAB_NTSC
    height = CIF_HEIGHT_NTSC/2;
#else
    height = CIF_HEIGHT/2;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP*2);
#else
      width = CIF_WIDTH/(2*STEP*2);
#endif
      do{
	FCrb_CIF4_RD_16_PIX_XXX(u0, src_pixel_ptr,  0);
	u0+=STEP*2;
	src_pixel_ptr+=2*STEP*2;    
      }while(--width);
      
#ifdef GRAB_NTSC
      u0 += CIF4_NTSC_BORDER/2;
      width = CIF_WIDTH_NTSC/(2*STEP*2);
#else
      width = CIF_WIDTH/(2*STEP*2);
#endif
      do{
	FCrb_CIF4_RD_16_PIX_XXX(u1, src_pixel_ptr, 0);
	u1+=STEP*2; /* *2 */
	src_pixel_ptr+=2*STEP*2;    
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH_NTSC;
      u1 += CIF4_NTSC_BORDER/2;
#else
      src_pixel_ptr += 2*PX_FB_WIDTH - 2*CIF_WIDTH;
#endif
    }while (--height);
  }while(--select);
}
#endif
/*---------------------------------------------------------------------------*/

FAST_SUN_PX_CIF_COLOR_NLD(px_port, y_data, u_data, v_data)
     PXport *px_port;
     u_char *y_data, *u_data, *v_data;
{
  register  u_char *src_pixel_ptr;
  register u_int tmp, y, m;
  register int height;
  register int width;
  register u_char *y_ptr;
  int select; 
#define uv_ptr y_ptr


  aw_select_Y();
  src_pixel_ptr = px_port->fb_window_addrf;
  
#ifdef GRAB_NTSC
  y_ptr = y_data + CIF_NTSC_UBORDER*CIF_WIDTH + CIF_NTSC_BORDER;
  height = CIF_HEIGHT_NTSC;
#else
  y_ptr = y_data;
  height = CIF_HEIGHT;
#endif
  
  do{
#ifdef GRAB_NTSC
    width = CIF_WIDTH_NTSC/(2*STEP);
#else
    width = CIF_WIDTH/(2*STEP);
#endif
    do{
      FY_CIF_RD_16_PIX_XXX(y_ptr, src_pixel_ptr, 0);
      y_ptr+=2*STEP;
      src_pixel_ptr+=2*STEP*CIF_STEP;    
    }while(--width);
    
#ifdef GRAB_NTSC
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH_NTSC*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
    y_ptr +=  CIF_WIDTH - CIF_WIDTH_NTSC;
#else
    src_pixel_ptr +=
      PX_FB_WIDTH - CIF_WIDTH*CIF_STEP + (CIF_STEP-1)*(PX_FB_WIDTH);
#endif
  }while (--height);
  
  if(!px_port->color)
    return;
  select = 2;
  
  select = 2;do{
    src_pixel_ptr = px_port->fb_window_addrf;
    
    if(select==2){
      aw_select_Cb(); 
#ifdef GRAB_NTSC
      uv_ptr = u_data + (CIF_NTSC_UBORDER*CIF_WIDTH/2 + CIF_NTSC_BORDER)/2;
#else
      uv_ptr = u_data;
#endif
    }else{
      aw_select_Cr ();
#ifdef GRAB_NTSC
      uv_ptr = v_data + (CIF_NTSC_UBORDER*CIF_WIDTH/2 + CIF_NTSC_BORDER)/2;
#else
      uv_ptr = v_data;
#endif
    }
#ifdef GRAB_NTSC
    height =  CIF_HEIGHT_NTSC/2;
#else
    height =  CIF_HEIGHT/2;
#endif
    do{
#ifdef GRAB_NTSC
      width = CIF_WIDTH_NTSC/(2*STEP*2);
#else
      width = CIF_WIDTH/(2*STEP*2);
#endif
      do{
	FCrb_CIF_RD_16_PIX_XXX(uv_ptr, src_pixel_ptr, 0);
	uv_ptr+=2*STEP;
	src_pixel_ptr+=2*STEP*CIF_STEP*2;    
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr +=
        - CIF_WIDTH_NTSC*CIF_STEP + (CIF_STEP*2)*(PX_FB_WIDTH);
      uv_ptr +=  CIF_WIDTH - CIF_WIDTH_NTSC;
#else
      src_pixel_ptr +=
         - CIF_WIDTH*CIF_STEP + (CIF_STEP*2)*(PX_FB_WIDTH);
#endif
    }while (--height);
  }while(--select);
}

/*---------------------------------------------------------------------------*/

FAST_SUN_PX_QCIF_COLOR_NLD(px_port, y_data, u_data, v_data)
     PXport *px_port;
     u_char *y_data, *u_data, *v_data;
{
  register  u_char *src_pixel_ptr;
  register u_int tmp, y, m;
  register int height;
  register int width;
  register u_char *y_ptr;
  int select;
#define uv_ptr y_ptr
  
  aw_select_Y();
  src_pixel_ptr = px_port->fb_window_addrf;
  
#ifdef GRAB_NTSC
  y_ptr = y_data + QCIF_NTSC_UBORDER*QCIF_WIDTH + QCIF_NTSC_BORDER;
  height = QCIF_HEIGHT_NTSC;
#else
  y_ptr = y_data;
  height = QCIF_HEIGHT;
#endif
  
  do{
#ifdef GRAB_NTSC
    width = QCIF_WIDTH_NTSC/(2*STEP);
#else
    width = QCIF_WIDTH/(2*STEP);
#endif
    do{
      FY_QCIF_RD_16_PIX_XXX(y_ptr, src_pixel_ptr, 0);
      y_ptr+=2*STEP;
      src_pixel_ptr+=2*STEP*QCIF_STEP;    
    }while(--width);
    
#ifdef GRAB_NTSC
    src_pixel_ptr +=
      PX_FB_WIDTH - QCIF_WIDTH_NTSC*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_ptr +=  QCIF_WIDTH - QCIF_WIDTH_NTSC;
#else
    src_pixel_ptr +=
      PX_FB_WIDTH - QCIF_WIDTH*QCIF_STEP + (QCIF_STEP-1)*(PX_FB_WIDTH);
    y_ptr += CIF_WIDTH - QCIF_WIDTH;
#endif
  }while (--height);
  
  if(!px_port->color)
    return;
  select = 2;
  
  select = 2;do{
    src_pixel_ptr = px_port->fb_window_addrf;
    
    if(select==2){
      aw_select_Cb();
#ifdef GRAB_NTSC
      uv_ptr = u_data + (QCIF_NTSC_UBORDER*QCIF_WIDTH/2 + QCIF_NTSC_BORDER)/2;
#else
      uv_ptr = u_data;
#endif
    }else{
      aw_select_Cr();
#ifdef GRAB_NTSC
      uv_ptr = v_data + (QCIF_NTSC_UBORDER*QCIF_WIDTH/2 + QCIF_NTSC_BORDER)/2;
#else
      uv_ptr = v_data;
#endif
    }

#ifdef GRAB_NTSC
    height =  QCIF_HEIGHT_NTSC/2;
#else
    height =  QCIF_HEIGHT/2;
#endif
    
    do{
#ifdef GRAB_NTSC
      width = QCIF_WIDTH_NTSC/(STEP*2);
#else
      width = QCIF_WIDTH/(STEP*2);
#endif
      do{
	FCrb_QCIF_RD_8_PIX_XXX(uv_ptr, src_pixel_ptr, 0);
	uv_ptr+=STEP;
	src_pixel_ptr+=2*STEP*QCIF_STEP;    
      }while(--width);
      
#ifdef GRAB_NTSC
      src_pixel_ptr +=
       PX_FB_WIDTH - QCIF_WIDTH_NTSC*QCIF_STEP + (2*QCIF_STEP-1)*(PX_FB_WIDTH);
      uv_ptr +=  CIF_WIDTH/2 - QCIF_WIDTH_NTSC/2;
#else
      src_pixel_ptr +=
      PX_FB_WIDTH - QCIF_WIDTH*QCIF_STEP + (2*QCIF_STEP-1)*(PX_FB_WIDTH);
      uv_ptr +=  CIF_WIDTH/2 - QCIF_WIDTH/2;
#endif
    }while (--height);
  }while(--select);
}

/*-------------------------------------------------------------------*/
#endif /* GRAB_FAST */
/*-------------------------------------------------------------------*/
