/**************************************************************************\
*          Copyright (c) 1995 by the computer center			   *
*				 university of Stuttgart		   *
*				 Germany           			   *
*                                                                          *
*  						                           *
*                                                                          *
* 	                						   *
*  File              : videosgi_grab.c 					   *
*				                                           *
*  Author            : Lei Wang, computer center			   *
*				 university of Stuttgart		   *
*				 Germany  				   *
*									   *
*  Description	     : grabbing proceduces for SGI INDIGOVIDEO		   *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Lei Wang                 |  95/2/15  |          Ver 1.0                  *
* wang@rus.uni-stuttgart.de|	       |				   *
\**************************************************************************/

#ifdef INDIGOVIDEO

#include <stdio.h>
#include <svideo.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "videosgi.h"

void  indigovideo_init_grab();
void  sgi_pal_bw_qcif();
void  sgi_pal_bw_cif();
void  sgi_pal_bw_cif4();
void  sgi_pal_bw_scif_div4();
void  sgi_pal_color_qcif();
void  sgi_pal_color_cif();
void  sgi_pal_color_cif4();
void  sgi_pal_color_scif_div4();

void (*indigo_tab[2][3])() =
{
   sgi_pal_bw_cif4,     sgi_pal_bw_cif,     sgi_pal_bw_qcif,
sgi_pal_color_cif4,  sgi_pal_color_cif,  sgi_pal_color_qcif
};

extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;
extern SVhandle sgi_video_hdl;

static unsigned char SGI_Y_TO_CCIR_Y[256];


/*======================================================*
               Initialization of capture
 *======================================================*/
void indigovideo_init_grab(color, size, ptr_ng_max, ptr_force_grey, ptr_col,
			    ptr_lig, get_image)
  BOOLEAN   color;
  int       size;
  BOOLEAN  *ptr_force_grey;
  int      *ptr_ng_max;
  int      *ptr_col, *ptr_lig;
  void   (**get_image)();
{
  int    sgi_y;


    if(!color){
	encCOLOR = FALSE;
	affCOLOR = MODGREY;
    }else{
       *ptr_force_grey = FALSE;
	encCOLOR = TRUE;
	affCOLOR = MODCOLOR;
    }

    if(size == QCIF_SIZE){
	*ptr_ng_max = 5;
	*ptr_lig = QCIF_HEIGHT;
	*ptr_col = QCIF_WIDTH;
    }else{
	if(size == CIF_SIZE){
	    *ptr_ng_max = 12;
	    *ptr_lig = CIF_HEIGHT;
	    *ptr_col = CIF_WIDTH;
	}else{
	    *ptr_ng_max = 12;
	    *ptr_lig = SCIF_HEIGHT;
	    *ptr_col = SCIF_WIDTH;
	}
    }

    for(sgi_y = 0; sgi_y < 256; sgi_y++){
	SGI_Y_TO_CCIR_Y[sgi_y] = 16 + (sgi_y*219)/255;
    }

    *get_image = indigo_tab[color][size];
}


/*======================================================*
                   get PAL BW QCIF image
 *======================================================*/
void sgi_pal_bw_qcif(im_data)
	u_char im_data[][CIF_WIDTH];
{
	u_char   *ptr_buf_beginn, *ptr_buf_visible;
	long	  field;
	register  u_char *ptr_data;
	register  u_char *ptr_buf_col, *ptr_buf_lig;
	int       i, j;

	do {
	    if (svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				 &field) < 0) {
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if (ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < QCIF_HEIGHT; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data = &im_data[i][0];

	    for(j = 0; j < QCIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_data++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col];
		 ptr_buf_col+= QCIF_COL_STEP;
	    }
	    ptr_buf_lig += QCIF_LIG_STEP;

	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/****************************************************************
 *			get PAL BW CIF image			*
 ****************************************************************/
void sgi_pal_bw_cif(im_data)
	u_char im_data[][CIF_WIDTH];
{
	u_char  *ptr_buf_beginn, *ptr_buf_visible, *ptr_buf_lig;
	long	 field;
	register u_char *ptr_data;
	register u_char *ptr_buf_col;
	int      i, j, buf_lig, lig_div2, col_div2;

	do {
	    if (svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
&field) < 0) {
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if (ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
			      (void **)&ptr_buf_visible, SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data = &im_data[i][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_data++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col];
		 ptr_buf_col+= CIF_COL_STEP;
	    }
	    ptr_buf_lig += CIF_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/****************************************************************
 *		get PAL BW CIF4/4 image (not used)		*
 ****************************************************************/
void sgi_pal_bw_scif_div4(im_data_y)
	u_char im_data_y[CIF_HEIGHT][CIF_WIDTH];
{
	long	field;
	u_char *ptr_buf_beginn, *ptr_buf_visible, *ptr_buf_col, *ptr_buf_lig;
	u_char *ptr_odd_data_y, *ptr_even_data_y;
	int     i, j, data_lig;

	do {
	    if (svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				 &field) < 0) {
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if (ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[data_lig++][0];
	    ptr_even_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						     ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/****************************************************************
 *	get PAL BW CIF4 image continuously (not used)		*
 ****************************************************************/
void sgi_pal_bw_scif(im_data_y)
	u_char im_data_y[SCIF_HEIGHT][SCIF_WIDTH];
{
	long	field;
	u_char *ptr_buf_beginn, *ptr_buf_visible, *ptr_buf_col, *ptr_buf_lig;
	u_char *ptr_odd_data_y, *ptr_odd_data_u, *ptr_odd_data_v;
	u_char *ptr_even_data_y, *ptr_even_data_u, *ptr_even_data_v;
	int     i, j, data_lig;

	do {
	    if (svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				 &field) < 0) {
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if (ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < SCIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[data_lig++][0];
	    ptr_even_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < SCIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						     ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += SCIF_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/****************************************************************
 *			get PAL BW CIF4 image			*
 ****************************************************************/
void sgi_pal_bw_cif4(im_data_y)
	u_char im_data_y[4][LIG_MAX][COL_MAX];
{
              long     field;
              u_char  *ptr_buf_beginn, *ptr_buf_visible;
    register  u_char  *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_odd_data_y, *ptr_even_data_y;
    register           i, j, data_lig;

	do {
	    if (svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				 &field) < 0) {
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if (ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
			(void **)&ptr_buf_visible, SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[0][data_lig++][0];
	    ptr_even_data_y = &im_data_y[0][data_lig++][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						    ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET_CIF4;
	    ptr_odd_data_y = &im_data_y[1][data_lig++][0];
	    ptr_even_data_y = &im_data_y[1][data_lig++][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						    ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible + SV_PAL_YMAX*SV_PAL_XMAX;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[2][data_lig++][0];
	    ptr_even_data_y = &im_data_y[2][data_lig++][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						    ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible + SV_PAL_YMAX*SV_PAL_XMAX;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET_CIF4;
	    ptr_odd_data_y = &im_data_y[3][data_lig++][0];
	    ptr_even_data_y = &im_data_y[3][data_lig++][0];

	    for(j = 0; j < CIF_WIDTH; j++){
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = SGI_Y_TO_CCIR_Y[*ptr_buf_col++];
		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_y++ = SGI_Y_TO_CCIR_Y[(*ptr_even_data_y)|
						    ((*ptr_buf_col++)>>4)];
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/************************************************************************
 *			get pal color qcif image			*
 ************************************************************************/
void sgi_pal_color_qcif(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[][CIF_WIDTH]; /* CIF_WIDTH needed */
	u_char im_data_u[][CIF_WIDTH_DIV2];
	u_char im_data_v[][CIF_WIDTH_DIV2];
{
	long	field;
	u_char *ptr_buf_beginn, *ptr_buf_visible, *ptr_buf_col, *ptr_buf_lig;
	u_char *ptr_data_y, *ptr_data_u, *ptr_data_v;

	int i, j, data_lig, buf_lig;

	do{
	    if(svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				&field) < 0){
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if(ptr_buf_beginn == NULL)
		sginap(1);
	}while(ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < QCIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data_u = &im_data_u[i][0];
	    ptr_data_v = &im_data_v[i][0];
	    ptr_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < QCIF_WIDTH_DIV2; j++){

		/* grabbing from the first 4 byte */
	         ptr_buf_col++;
		*ptr_data_y++ = *ptr_buf_col++;
		*ptr_data_u = (*ptr_buf_col)&0xC0;
		*ptr_data_v = ((*ptr_buf_col)<<2)&0xC0;
	 	 ptr_buf_col+= 4;

		/* grabbing from the second 4 byte */
		*ptr_data_u = (((*ptr_buf_col)>>2)&0x30)|(*ptr_data_u);
		*ptr_data_v = ((*ptr_buf_col)&0x30)|(*ptr_data_v);

		/* grabbing from the third 4 byte */
	 	 ptr_buf_col+= 4;
		*ptr_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_data_v |= ((*ptr_buf_col)>>2)&0x0C;
	 	 ptr_buf_col+= 4;

		/* grabbing from the fourth 4 byte */
		*ptr_data_u++ |= (*ptr_buf_col)>>6;
		*ptr_data_v++ |= ((*ptr_buf_col)>>4)&0x03;

		/* grabbing from the 5th - 8th 4 byte */
	 	 ptr_buf_col+= 3;
		*ptr_data_y++ = *ptr_buf_col;
		 ptr_buf_col+= QCIF_COL_STEP;
	    }
	    ptr_buf_lig += QCIF_LIG_STEP;
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < QCIF_WIDTH_DIV2; j++){

		/* grabbing from the 9th - 16th 4 byte */
	         ptr_buf_col++;
		*ptr_data_y++ = *ptr_buf_col++;
		 ptr_buf_col+= QCIF_COL_STEP;
		*ptr_data_y++ = *ptr_buf_col;
		 ptr_buf_col+= QCIF_COL_STEP;
	    }
	    ptr_buf_lig += QCIF_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/****************************************************************
 *			get pal color CIF image			*
 ****************************************************************/
void sgi_pal_color_cif(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[CIF_HEIGHT][CIF_WIDTH];
	u_char im_data_u[CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
	u_char im_data_v[CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
{
	long	  field;
	u_char   *ptr_buf_beginn, *ptr_buf_visible;
	register  u_char *ptr_buf_col, *ptr_buf_lig;
	register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
	register  i, j, data_lig;

	do{
	    if(svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				&field) < 0){
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if(ptr_buf_beginn == NULL)
		sginap(1);
	}while(ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data_u = &im_data_u[i][0];
	    ptr_data_v = &im_data_v[i][0];
	    ptr_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < CIF_WIDTH_DIV2; j++){

		/* grabbing from the first 4 byte */
	         ptr_buf_col++;
		*ptr_data_y++ = *ptr_buf_col++;
		*ptr_data_u = (*ptr_buf_col)&0xC0;
		*ptr_data_v = ((*ptr_buf_col)<<2)&0xC0;
	 	 ptr_buf_col+= 4;

		/* grabbing from the second 4 byte */
		*ptr_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_data_v |= (*ptr_buf_col)&0x30;

 		/* grabbing from the third 4 byte */
	 	 ptr_buf_col+= 3;
		*ptr_data_y++ = *ptr_buf_col++;
		*ptr_data_u |= ((*ptr_buf_col)>>4)&0xC;
		*ptr_data_v |= ((*ptr_buf_col)>>2)&0xC;
	 	 ptr_buf_col+= 4;

		/* grabbing from the fourth 4 byte */
		*ptr_data_u++ |= (*ptr_buf_col)>>6;
		*ptr_data_v++ |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_buf_col+= 2;
	    }
	    ptr_buf_lig += CIF_LIG_STEP;

	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_data_y = &im_data_y[data_lig++][0];

	    for(j = 0; j < CIF_WIDTH_DIV2; j++){
	         ptr_buf_col++;
		*ptr_data_y++ = *ptr_buf_col++;
		 ptr_buf_col+= CIF_COL_STEP;
		*ptr_data_y++ = *ptr_buf_col;
		 ptr_buf_col+= CIF_COL_STEP;
	    }
	    ptr_buf_lig+= CIF_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/************************************************************************
 *		get PAL color CIF4/4 image (not used)			*
 ************************************************************************/
void sgi_pal_color_scif_div4(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[CIF_HEIGHT][CIF_WIDTH];
	u_char im_data_u[CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
	u_char im_data_v[CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
{
	long	  field;
	u_char   *ptr_buf_beginn, *ptr_buf_visible;
	register  u_char  *ptr_buf_col, *ptr_buf_lig;
	register  u_char  *ptr_odd_data_y, *ptr_odd_data_u, *ptr_odd_data_v;
	register  u_char  *ptr_even_data_y, *ptr_even_data_u, *ptr_even_data_v;
	register  i, j, data_lig, col_div4;

	do{
	    if(svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				&field) < 0){
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if(ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);

	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
			      (void **)&ptr_buf_visible, SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	col_div4 = CIF_WIDTH>>2;
	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[data_lig++][0];
	    ptr_odd_data_u = &im_data_u[i][0];
	    ptr_odd_data_v = &im_data_v[i][0];

	    ptr_even_data_y = &im_data_y[data_lig++][0];
	    ptr_even_data_u = &im_data_u[i][1];
	    ptr_even_data_v = &im_data_v[i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/************************************************************************
 *		get PAL color CIF4 image continuously (not used)	*
 ************************************************************************/
void sgi_pal_color_scif(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[SCIF_HEIGHT][SCIF_WIDTH];
	u_char im_data_u[SCIF_HEIGHT_DIV2][SCIF_WIDTH_DIV2];
	u_char im_data_v[SCIF_HEIGHT_DIV2][SCIF_WIDTH_DIV2];
{
	long	field;
	u_char *ptr_buf_beginn, *ptr_buf_visible, *ptr_buf_col, *ptr_buf_lig;
	u_char *ptr_odd_data_y, *ptr_odd_data_u, *ptr_odd_data_v;
	u_char *ptr_even_data_y, *ptr_even_data_u, *ptr_even_data_v;
	int     i, j, data_lig, buf_lig, lig_div2, col_div4;

	do{
	    if(svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				&field) < 0){
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if(ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);


	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	col_div4 = SCIF_WIDTH>>2;
	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < SCIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[data_lig++][0];
	    ptr_odd_data_u = &im_data_u[i][0];
	    ptr_odd_data_v = &im_data_v[i][0];

	    ptr_even_data_y = &im_data_y[data_lig++][0];
	    ptr_even_data_u = &im_data_u[i][1];
	    ptr_even_data_v = &im_data_v[i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += SCIF_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}


/************************************************************************
 *			get PAL color CIF4 image			*
 ************************************************************************/
void sgi_pal_color_cif4(im_data_y, im_data_u, im_data_v)
	u_char im_data_y[4][CIF_HEIGHT][CIF_WIDTH];
	u_char im_data_u[4][CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
	u_char im_data_v[4][CIF_HEIGHT_DIV2][CIF_WIDTH_DIV2];
{
	              long	   field;
	      u_char  *ptr_buf_beginn, *ptr_buf_visible;
    register  u_char  *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_odd_data_y, *ptr_odd_data_u, *ptr_odd_data_v;
    register  u_char  *ptr_even_data_y, *ptr_even_data_u, *ptr_even_data_v;
    register           i, j, data_lig, col_div4;

	do{
	    if(svGetCaptureData(sgi_video_hdl, (void **)&ptr_buf_beginn,
				&field) < 0){
		svPerror("sgi_get_image: svGetCaptureData");
		svEndContinuousCapture(sgi_video_hdl);
		exit(1);
	    }
	    if(ptr_buf_beginn == NULL)
		sginap(1);
	} while (ptr_buf_beginn == NULL);


	if(svFindVisibleRegion(sgi_video_hdl, (void *)ptr_buf_beginn,
				    (void **)&ptr_buf_visible,
			       SV_PAL_XMAX) < 0){
	    svPerror("get_color_image: svFindVisibleRegion");
	    svEndContinuousCapture(sgi_video_hdl);
	    exit(1);
	}

	col_div4 = CIF_WIDTH>>2;
	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[0][data_lig++][0];
	    ptr_odd_data_u = &im_data_u[0][i][0];
	    ptr_odd_data_v = &im_data_v[0][i][0];

	    ptr_even_data_y = &im_data_y[0][data_lig++][0];
	    ptr_even_data_u = &im_data_u[0][i][1];
	    ptr_even_data_v = &im_data_v[0][i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET_CIF4;
	    ptr_odd_data_y = &im_data_y[1][data_lig++][0];
	    ptr_odd_data_u = &im_data_u[1][i][0];
	    ptr_odd_data_v = &im_data_v[1][i][0];

	    ptr_even_data_y = &im_data_y[1][data_lig++][0];
	    ptr_even_data_u = &im_data_u[1][i][1];
	    ptr_even_data_v = &im_data_v[1][i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible  + SV_PAL_YMAX*SV_PAL_XMAX;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET;
	    ptr_odd_data_y = &im_data_y[2][data_lig++][0];
	    ptr_odd_data_u = &im_data_u[2][i][0];
	    ptr_odd_data_v = &im_data_v[2][i][0];

	    ptr_even_data_y = &im_data_y[2][data_lig++][0];
	    ptr_even_data_u = &im_data_u[2][i][1];
	    ptr_even_data_v = &im_data_v[2][i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}

	data_lig = 0;

	ptr_buf_lig = ptr_buf_visible + SV_PAL_YMAX*SV_PAL_XMAX;

	for(i = 0; i < CIF_HEIGHT_DIV2; i++ ){
	    ptr_buf_col = ptr_buf_lig + X_OFFSET_CIF4;
	    ptr_odd_data_y = &im_data_y[3][data_lig++][0];
	    ptr_odd_data_u = &im_data_u[3][i][0];
	    ptr_odd_data_v = &im_data_v[3][i][0];

	    ptr_even_data_y = &im_data_y[3][data_lig++][0];
	    ptr_even_data_u = &im_data_u[3][i][1];
	    ptr_even_data_v = &im_data_v[3][i][1];

	    for(j = 0; j < col_div4; j++){

		/* the first 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u = (*ptr_buf_col)&0xC0;
		*ptr_odd_data_v = ((*ptr_buf_col)<<2)&0xC0;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u = ((*ptr_buf_col)<<4)&0xC0;
		*ptr_even_data_v = (*ptr_buf_col)<<6;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the second 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>2)&0x30;
		*ptr_odd_data_v |= (*ptr_buf_col)&0x30;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)<<2)&0x30;
		*ptr_even_data_v |= ((*ptr_buf_col)<<4)&0x30;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the third 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= ((*ptr_buf_col)>>4)&0x0C;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>2)&0x0C;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= (*ptr_buf_col)&0x0C;
		*ptr_even_data_v |= ((*ptr_buf_col)<<2)&0x0C;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);

		/* the fourth 4 byte */
	 	 ptr_buf_col++;
		*ptr_odd_data_y++ = *ptr_buf_col++;
		*ptr_odd_data_u |= (*ptr_buf_col)>>6;
		*ptr_odd_data_v |= ((*ptr_buf_col)>>4)&0x03;
		 ptr_odd_data_u += 2;
		 ptr_odd_data_v += 2;

		*ptr_even_data_y = (*ptr_buf_col++)<<4;
		*ptr_even_data_u |= ((*ptr_buf_col)>>2)&0x03;
		*ptr_even_data_v |= (*ptr_buf_col)&0x03;
		 ptr_even_data_u += 2;
		 ptr_even_data_v += 2;
		*ptr_even_data_y++ = (*ptr_even_data_y)|((*ptr_buf_col++)>>4);
	    }
	    ptr_buf_lig += CIF4_LIG_STEP;
	}
	svUnlockCaptureData(sgi_video_hdl, ptr_buf_beginn);
}

#endif /* INDIGOVIDEO */



