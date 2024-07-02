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
*  File              : grab_vigrapix.c                                     *
*  Last modification : 1995/3/31                                           *
*  Author            : Thierry Turletti                                    *
*--------------------------------------------------------------------------*
*  Description :  Initialization and grabbing functions for the            *
*                 VIGRAPIX framegrabber.                                   *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	          |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#ifdef VIGRAPIX
#ident "@(#) grab_vigrapix.c 1.1@(#)"

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#define GENERAL_GRABBER 1
#include "grab_general.h"
#include <vigrapix.h>

/*
#define TIMING 1
*/

/* Global variables not exported */
static int width, height;
static int video_format;

/*--------- from display.c --------------*/

extern BOOLEAN	CONTEXTEXIST;
extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;



void rescale_y8_to_ccir601(buf_in, buf_out)
    u_char *buf_in, *buf_out;
{
    register u_char *pt_out;
    register u_int32 word32, *pt_in;
    register u_char *y2ccir = &Y8_TO_CCIR[0];
    register int l, c;
    int OFFSET_COL= height == CIF_HEIGHT ? 0 : COL_MAX_DIV2;
    
    pt_in = (u_int32 *) buf_in;
    pt_out = buf_out;

    for(l=0; l<height; l++){
	for(c=0; c<width; c+=4){
	    word32 = *pt_in++;
	    *pt_out++ = *(y2ccir + ((word32 >> 24) & 0xff));
	    *pt_out++ = *(y2ccir + ((word32 >> 16) & 0xff));
	    *pt_out++ = *(y2ccir + ((word32 >> 8) & 0xff));
	    *pt_out++ = *(y2ccir + (word32 & 0xff));
	}
	pt_out += OFFSET_COL;
    }
}


void vigrapix_get_grey(vp, y_data)
    vigrapix_t vp;
    u_char *y_data;
{
    static u_char *y422_data;
    static BOOLEAN FIRST=TRUE;
#ifdef TIMING  
    struct timeval stv, etv;

    gettimeofday(&stv);
#endif  
    if(FIRST){
	y422_data = (void *) malloc((unsigned)width*height);
	FIRST = FALSE;
    }
    vigrapix_read_grey(vp, y422_data);
    rescale_y8_to_ccir601(y422_data, y_data);
#ifdef TIMING  
  gettimeofday(&etv);
  etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
  fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif
}


void rescale_uv_to_ccir601(uv_data, u_data, v_data)
    u_char *uv_data, *u_data, *v_data;
{
    register int l, c;
    register u_int32 *pt_uv_data, word32;
    register u_char *pt_u_data, *pt_v_data;
    register u_char *uv2ccir = &UV8_TO_CCIR[0];
    int OFFSET_COL= height == CIF_HEIGHT ? 0 : COL_MAX_DIV2/2;

    pt_uv_data = (u_int32 *)uv_data;
    pt_u_data = u_data;
    pt_v_data = v_data;

    for(l=0; l<height; l+=2){
	for(c=0; c<width; c+=4){
	    word32 = *pt_uv_data++;

	    *(pt_v_data + 1) = *(uv2ccir + (word32 & 0xff));
	    word32 >>= 8;
	    *(pt_u_data + 1) = *(uv2ccir + (word32 & 0xff));
	    word32 >>= 8;
	    *pt_v_data = *(uv2ccir + (word32 & 0xff));
	    word32 >>= 8;
	    *pt_u_data  = *(uv2ccir + (word32 & 0xff));
	    word32 >>= 8;

	    pt_u_data += 2;
	    pt_v_data += 2;
	}
	pt_uv_data += width/4;
	pt_u_data += OFFSET_COL;
	pt_v_data += OFFSET_COL;
    }
}


void vigrapix_get_color(vp, y_data, u_data, v_data)
    vigrapix_t vp;
    u_char *y_data, *u_data, *v_data;
{
    static u_char *y422_data, *uv_data;
    static BOOLEAN FIRST=TRUE;
#ifdef TIMING  
    struct timeval stv, etv;

    gettimeofday(&stv);
#endif  
    if(FIRST){
	y422_data = (void *) malloc((unsigned)width*height);
	uv_data = (void *) malloc((unsigned)width*height);
	FIRST = FALSE;
    }
    vigrapix_read_y_uv(vp, (u_char *)y422_data, (u_char *)uv_data);
    rescale_y8_to_ccir601((u_char *)y422_data, y_data);
    rescale_uv_to_ccir601((u_char *)uv_data, u_data, v_data);
#ifdef TIMING  
    gettimeofday(&etv);
    etv.tv_usec += (etv.tv_sec - stv.tv_sec) * 1000000;
    fprintf(stderr, "dt grabbing = %d ms\n", etv.tv_usec - stv.tv_usec);
#endif
}

void SetVigraPixPort(vp, port_video)
    vigrapix_t vp;
    int port_video;
{
    int new_video_format, x_offset, input_width;
    int width, height;

    vigrapix_set_port(vp, 2-port_video);
    new_video_format = vigrapix_get_format(vp);
    if(new_video_format != video_format) {
	switch(new_video_format) {
	case VIGRAPIX_NOLOCK:
	    fprintf(stderr, "No video signal found\n");
	case VIGRAPIX_NTSC:
	    width = NTSC_WIDTH;
	    height = NTSC_HEIGHT;
	    break;
	case VIGRAPIX_PAL:
	    width = PAL_WIDTH;
	    height = PAL_HEIGHT;
	    break;
	case VIGRAPIX_SECAM:
	    width = SECAM_WIDTH;
	    height = SECAM_HEIGHT;
	    break;
	}
	video_format = new_video_format;
	if(vigrapix_set_scaler(vp, -1, -1, width, height, -1, -1) == -1) {
	    fprintf(stderr, "ivs: Couldn't scale the image\n");
	}
	if(video_format == VIGRAPIX_NTSC){
	    input_width = (width * NTSC_HEIGHT) / height;
	}else{
	    input_width = (width * PAL_HEIGHT) / height;
	}
	x_offset = (vp->in_w - input_width) / 2;
	
	if(vigrapix_set_scaler(vp, x_offset, -1, input_width, -1, -1, -1) 
	   == -1) {
	    fprintf(stderr, "ivs: Couldn't rescale the image\n");
	}
    }
}



void VigraPixInitialize(vp, port_video, size, SECAM, COLOR, NG_MAX, L_lig, 
			L_col, Lcoldiv2, L_col2, L_lig2, get_image)
    vigrapix_t *vp;
    int port_video;
    int size;
    BOOLEAN SECAM, COLOR;
    int *NG_MAX, *L_lig, *L_col, *Lcoldiv2, *L_col2, *L_lig2;
    void (**get_image)();
{
    int input_width, input_height, y_offset, x_offset;

    if(! *vp) {
	/* Open the VigraPix device */
	if((*vp = vigrapix_open("/dev/vigrapix0")) == NULL){
	    fprintf(stderr, "ivs: failed to open /dev/vigrapix0 device\n");
	    exit(1);
	}
    }

    vigrapix_set_port(*vp, 2-port_video);

    if(vigrapix_secam_mode(*vp, SECAM) == -1) {
	if(SECAM)
	    fprintf(stderr, "ivs: Cannot enable SECAM mode\n");
	else
	    fprintf(stderr, "ivs: Cannot disable SECAM mode\n");	
    }

    switch(video_format=vigrapix_get_format(*vp)) {
    default:
    case VIGRAPIX_NOLOCK:
	fprintf(stderr, "ivs: No video signal found\n");
    case VIGRAPIX_NTSC:
	width = NTSC_WIDTH;
	height = NTSC_HEIGHT;
	break;
    case VIGRAPIX_PAL:
	width = PAL_WIDTH;
	height = PAL_HEIGHT;
	break;
    case VIGRAPIX_SECAM:
	width = SECAM_WIDTH;
	height = SECAM_HEIGHT;
	break;
    }

    if(size == QCIF_SIZE){
	*NG_MAX = 5;
	*L_lig = QCIF_HEIGHT;
	*L_col = QCIF_WIDTH;
    }else{
	*NG_MAX = 12;
	*L_lig = CIF_HEIGHT;
	*L_col = CIF_WIDTH;
    }
    width = *L_col;
    height = *L_lig;

    *Lcoldiv2 = *L_col >> 1;
    *L_col2 = *L_col << 1;
    *L_lig2 = *L_lig << 1;

    
    if(vigrapix_set_scaler(*vp, -1, -1, -1, -1, width, height) == -1) {
	fprintf(stderr, "ivs: Couldn't scale the image\n");
    }

    if(video_format == VIGRAPIX_NTSC){
	input_width = (width * NTSC_HEIGHT) / height;
    }else{
	input_width = (width * PAL_HEIGHT) / height;
    }
    x_offset = ((*vp)->in_w - input_width) / 2;

    if(vigrapix_set_scaler(*vp, x_offset, -1, input_width, -1, -1, -1) == -1) {
	fprintf(stderr, "ivs: Couldn't rescale the image\n");
    }
    
    if(!COLOR){
	encCOLOR = FALSE;
	affCOLOR = MODGREY;
    }else{
	encCOLOR = TRUE;
	affCOLOR = MODCOLOR;
    }

    *get_image = COLOR ? vigrapix_get_color : vigrapix_get_grey;
}

#endif	/* VIGRAPIX */

