/**************************************************************************\
*									   *
*          Copyright (c) 1995 Frank Mueller,nig@nig.rhoen.de               *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* 								           *
* I MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS      *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
***************************************************************************/
/*          		                                                   *
*                                                                          *
*  						                           *
*                                                                          *
* 	                						   *
*  File              : grab_sm.c 					   *
*				                                           *
*  Author            : Frank Mueller,nig@nig.rhoen.de,			   *
*		       Fachhochschule Fulda       			   *
*				  					   *
*									   *
*  Description	     : grabbing proceduces for ScreenMachine II for Linux  *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Frank Mueller            |  95/04/01 |          Ver 1.0                  *
* nig@nig.rhoen.de 	   |	       |				   *
\**************************************************************************/

#ifdef SCREENMACHINE

#include <stdio.h>
#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "grab_general.h"

/* Global, imagedata contains the actual image */

extern u_char *imagedata;
extern int imagesize;
extern int current_sm;

void  sm_init_grab();
void  sm_pal_bw_qcif();
void  sm_pal_bw_cif();
void  sm_pal_bw_scif_div4();
void  sm_pal_color_qcif();
void  sm_pal_color_cif();
void  sm_pal_color_scif_div4();
void  sm_pal_bw_cif4();
void  sm_pal_color_cif4();

void (*sm_tab[2][3])() =
{
   sm_pal_bw_cif4,     sm_pal_bw_cif,     sm_pal_bw_qcif,
   sm_pal_color_cif4,  sm_pal_color_cif,  sm_pal_color_qcif
};



/* 	This is if you have more than one SM II 
	so that you can toggle between the SM and sends commands to the SM II
	you have selected 
*/



void sm_pal_bw_cif4()
{

/* Not supported at the moment */

}

void sm_pal_color_cif4()
{

/* Not supported at the moment */

}


/*======================================================*
               Initialization of capture
 *======================================================*/
void sm_init_grab(color, size, ptr_ng_max, ptr_col, ptr_lig, get_image)
  int       color;
  int       size;
  int      *ptr_ng_max;
  int      *ptr_col, *ptr_lig;
  void   (**get_image)();
{
  
    if(size == QCIF_SIZE) {
	*ptr_ng_max = 5;
	*ptr_lig = QCIF_HEIGHT;
	*ptr_col = QCIF_WIDTH;
    		
	/* Set the capture window to QCIF_WITH,QCIF_HEIGHT */
	smSetWindowFrame(current_sm,0,0,QCIF_WIDTH,QCIF_HEIGHT);
    		
	/* Malloc WIDHT*HEIGHT*2+Header */
	imagedata = (u_char *)malloc(QCIF_WIDTH*QCIF_HEIGHT*2+64);
    }
    if(size == CIF_SIZE) {
	*ptr_ng_max = 12;
	*ptr_lig = CIF_HEIGHT;
	*ptr_col = CIF_WIDTH;

	/* Set the capture window to CIF_WITH,CIF_HEIGHT */
	smSetWindowFrame(current_sm,0,0,CIF_WIDTH,CIF_HEIGHT+20);

	/* Malloc WIDHT*HEIGHT*2+Header */
	imagedata = (u_char *)malloc(CIF_WIDTH*CIF_HEIGHT*2+64);
    }
	
    *get_image = sm_tab[color][size];
}


/*======================================================*
                   get BW QCIF image
                   Pal or NTSC doesn't matter
                   because QCIF is QCIF
 *======================================================*/
void sm_pal_bw_qcif(im_data)
	u_char im_data[][CIF_WIDTH];
{
    register int i, j;
    int zaehler;
	
    smReadYImage(current_sm,imagedata,&imagesize);
    zaehler=0;

    for(i = 0; i < QCIF_HEIGHT; i++ ) {
	for(j = 0; j < QCIF_WIDTH; j++) {
	    im_data[i][j] = imagedata[zaehler+64];
	    zaehler=zaehler+1;
	}
    }
}


/*======================================================*
                   get BW CIF image
                   Pal or NTSC doesn't matter
                   because CIF is CIF
 *======================================================*/
void sm_pal_bw_cif(im_data)
    u_char im_data[][CIF_WIDTH];
{
    register int i, j;
    int zaehler;
	
    smReadYImage(current_sm, imagedata, &imagesize);
    zaehler=0;
	
    for(i = 0; i < CIF_HEIGHT; i++) {
	for(j = 0; j < CIF_WIDTH; j++) {
	    im_data[i][j] = imagedata[zaehler+64];
	    zaehler=zaehler+1;
	}
    }
}


/************************************************************************
 *			get color qcif image				*
 *			Pal or NTSC doesn't matter			*
 *                      because QCIF is QCIF				*
 ************************************************************************/
void sm_pal_color_qcif(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[][CIF_WIDTH]; /* CIF_WIDTH needed */
	u_char im_data_u[][CIF_WIDTH_DIV2];
	u_char im_data_v[][CIF_WIDTH_DIV2];
{
    int zaehler;
    int line_zaehler;
    int height_zaehler;
    register int i, j;
	

    /* Read a YUV 4:2:2 Frame */
    smReadImage(current_sm, imagedata, &imagesize);
	
    zaehler = 0;
    line_zaehler = 0;
    height_zaehler =0;
	
    /* Now we fill the im_data_y,im_data_v,im_data_u 
       We drop just every second v und u from imagedata */

    for(i = 0; i < QCIF_HEIGHT; i++ ) {
	line_zaehler=0;

	if ((i & 1) == 0) 
	    height_zaehler=height_zaehler+1;

	for (j = 0; j < QCIF_WIDTH;) {
	    im_data_y[i][j]   = imagedata[zaehler+64];
	    im_data_y[i][j+1] = imagedata[zaehler+2+64];
	    im_data_y[i][j+2] = imagedata[zaehler+4+64];
	    im_data_y[i][j+3] = imagedata[zaehler+6+64];
	    if (( i & 1 ) == 0) {
		im_data_u[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+1+64] - 127;
		im_data_v[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+3+64] -127 ; 
		im_data_u[height_zaehler][line_zaehler+1] = 
		    imagedata[zaehler+5+64] -127;
		im_data_v[height_zaehler][line_zaehler+1] = 
		    imagedata[zaehler+7+64] -127 ; 			
		line_zaehler=line_zaehler+2;
	    }
	    zaehler=zaehler+8;
	    j=j+4;
	}
    }	
}


/****************************************************************
 *			get color CIF image			*
 * 			Pal or NTSC doesn't matter		*
 *                      because QCIF is QCIF			*
 ****************************************************************/
void sm_pal_color_cif(im_data_y,im_data_u,im_data_v)
	u_char im_data_y[][CIF_WIDTH];
	u_char im_data_u[][CIF_WIDTH_DIV2];
	u_char im_data_v[][CIF_WIDTH_DIV2];
{
    register i, j;
    int zaehler;
    int line_zaehler;
    int height_zaehler;
	

    smReadImage(current_sm, imagedata, &imagesize);
	
    zaehler=0;
    height_zaehler=0;
	
    for(i = 0; i < CIF_HEIGHT; i++ ) {
	line_zaehler=0;
	if ((i & 1) == 0 ) 
	    height_zaehler=height_zaehler+1;
	
	for ( j = 0; j < CIF_WIDTH;) {
	    im_data_y[i][j]   = imagedata[zaehler+64];
	    im_data_y[i][j+1] = imagedata[zaehler+2+64];
	    im_data_y[i][j+2] = imagedata[zaehler+4+64];
	    im_data_y[i][j+3] = imagedata[zaehler+6+64];
	    if (( i & 1 ) == 0) {
		im_data_u[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+1+64] -127;
		im_data_v[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+3+64] -127;
		line_zaehler=line_zaehler+1;
		im_data_u[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+1+64] -127;
		im_data_v[height_zaehler][line_zaehler] = 
		    imagedata[zaehler+3+64] -127;
		line_zaehler=line_zaehler+1;
	    }			
	    zaehler=zaehler+8;
	    j=j+4;
	}
    }			
}
#endif /* SM II */
