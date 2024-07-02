/**************************************************************************\
 *									  *
 *          Copyright (c) 1995 by the computer center			  *
 *				 university of Stuttgart		  *
 *				 Germany           			  *
 *                                                                        *
 *  						                          *
 *  File              : grab_galileovideo.c 			          *
 *								  	  *
 *				                                          *
 *  Author            : Lei Wang, computer center			  *
 *				 university of Stuttgart		  *
 *				 Germany  				  *
 *									  *
 *  Description	     : grabbing proceduces for SGI machine with Galileo   *
 *		       video board 					  *
 *									  *
 *									  *
 *------------------------------------------------------------------------*
 *        Name		    |    Date   |          Modification           *
 * Lei Wang                 |  95/2/15  |          Ver 1.0                *
 * wang@rus.uni-stuttgart.de|	        |				  *
\**************************************************************************/
#ifdef GALILEOVIDEO

#include <stdio.h>
#include <vl/vl.h>

#include "general_types.h"
#include "h261.h"
#include "protocol.h"
#include "videosgi.h"

void  galileovideo_init_grab();
void  galileo_pal_bw_qcif();
void  galileo_pal_bw_qcif_zoom2();
void  galileo_pal_bw_cif();
void  galileo_pal_bw_cif_zoom1();
void  galileo_pal_bw_cif_zoom2_new();
void  galileo_pal_bw_cif4();
void  galileo_pal_color_qcif();
void  galileo_pal_color_qcif_zoom2();
void  galileo_pal_color_cif();
void  galileo_pal_color_cif_zoom2_new();
void  galileo_pal_color_cif_zoom1(); /* test */
void  galileo_pal_color_cif4();

void  galileo_ntsc_bw_cif();
void  galileo_ntsc_color_cif();
void  galileo_ntsc_color_qcif();
void  galileo_ntsc_bw_qcif();
void  galileo_ntsc_bw_cif4();
void  galileo_ntsc_color_cif4();

VLBuffer trans_buf;
const    unsigned char *ptr_str;

extern BOOLEAN	encCOLOR;
extern u_char	affCOLOR;
/* from videosgi_galileo.c */
extern VLServer sgi_video_hdl;
extern VLPath   path_hdl;
extern VLNode   drn;
extern VLControlValue val;

static unsigned  char *ptr_buf;
static VLInfoPtr buf_info;
static unsigned char ccir_y[256];



/* for PAL and NTSC */
void (*galileo_tab[2][2][3])() =
{
   galileo_ntsc_bw_cif4, galileo_ntsc_bw_cif,
   galileo_ntsc_bw_qcif, galileo_ntsc_color_cif4,
   galileo_ntsc_color_cif, galileo_ntsc_color_qcif,

   galileo_pal_bw_cif4, galileo_pal_bw_cif_zoom2_new,
   galileo_pal_bw_qcif_zoom2, galileo_pal_color_cif4,
   galileo_pal_color_cif, galileo_pal_color_qcif_zoom2
};


/****************************************
 *	Initialization of capture	*
 ***************************************/
void galileovideo_init_grab(pal_format, color, size, ptr_ng_max,
                            ptr_force_grey, ptr_col, ptr_lig, get_image)
  BOOLEAN  *ptr_force_grey;
  BOOLEAN   color;
  int       pal_format;
  int      *ptr_ng_max;
  int       size;
  int      *ptr_col, *ptr_lig;
  void   (**get_image)();
{
  VLTransferDescriptor xferDesc;
  int frame_size;
  int sgi_y;

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
	ccir_y[sgi_y] = (16 + 219*sgi_y/255);
   }

    val.intVal = VL_PACKING_YVYU_422_8;

    if((vlSetControl(sgi_video_hdl, path_hdl, drn, VL_PACKING, &val)) < 0){
        fprintf(stderr, "by galileovideo_init_grab: ");
        vlPerror("vlSetControl PACKING");
        fprintf(stderr, "\n");
        exit(1);
    }

    val.fractVal.numerator = 1;
    if(size == QCIF_SIZE)
	val.fractVal.denominator = 2;
    else
	if(size == CIF_SIZE)
	    val.fractVal.denominator = 2;
	else
	    val.fractVal.denominator = 1;

    if((vlSetControl(sgi_video_hdl, path_hdl, drn, VL_ZOOM, &val)) < 0){
	fprintf(stderr, "by galileovideo_init_grab: ");
	vlPerror("vlSetControl ZOOM");
	fprintf(stderr, "\n");
	exit(1);
    }

    val.intVal = TRANSFER_RATE_GALILEO;
    if(vlSetControl(sgi_video_hdl, path_hdl, drn, VL_RATE, &val) < 0){
        fprintf(stderr, "by galileovideo_init_grab: ");
        vlPerror("vlSetControl RATE");
        exit(1);
    }
    vlGetControl(sgi_video_hdl, path_hdl, drn, VL_RATE, &val);

    vlGetControl(sgi_video_hdl, path_hdl, drn, VL_SIZE, &val);

	/* Buffer Initialization */

    trans_buf = vlCreateBuffer(sgi_video_hdl, path_hdl, drn, 2);
    if(trans_buf == NULL){
	fprintf(stderr, "by galileovideo_init_grab: ");
	vlPerror("vlCreateBuf\n");
	exit(1);
    }
    vlRegisterBuffer(sgi_video_hdl, path_hdl, drn, trans_buf);

    xferDesc.mode = VL_TRANSFER_MODE_CONTINUOUS;
    xferDesc.count = 1;
    xferDesc.delay = 0;
    xferDesc.trigger = VLTriggerImmediate;

    if((vlBeginTransfer(sgi_video_hdl, path_hdl, 1, &xferDesc)) == -1){
        fprintf(stderr, "by galileovideo_init_grab: ");
        vlPerror("vlBeginTransfer\n");
        exit(1);
    }

    *get_image = galileo_tab[pal_format][color][size];

}


/*--------------------------------------------------------------*
 |		get pal color CIF image: zoom setting 1		|
 *--------------------------------------------------------------*/
void galileo_pal_color_cif_zoom1(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][CIF_WIDTH_DIV2];
    u_char im_data_v[][CIF_WIDTH_DIV2];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 0;
    ptr_buf_lig = ptr_buf;

    for(i = 0; i < 144; i++ ){ /* CIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &im_data_u[i][0];
        ptr_data_v = &im_data_v[i][0];
        ptr_data_y = &im_data_y[data_lig++][0];

        for(j = 0; j < 176; j++){ /* CIF_WIDTH_DIV2 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	    *ptr_data_v++ = *ptr_buf_col;
	     ptr_buf_col+= 3;
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col+= 3;
	}
	ptr_buf_lig += 768*4; /* 768*4: CIF_LIG_STEP */
	ptr_buf_col = ptr_buf_lig + 1;
	ptr_data_y = &im_data_y[data_lig++][0];

	for(j = 0; j < 352; j++){
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col+= 4;
	}
	ptr_buf_lig+= 768*4; /* CIF_LIG_STEP */
    }


    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }

}


/*----------------------------------------------------------------------*
 |		get pal color CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_pal_color_cif(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][CIF_WIDTH_DIV2];
    u_char im_data_v[][CIF_WIDTH_DIV2];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 0;
    ptr_buf_lig = ptr_buf;

    for(i = 0; i < 144; i++ ){ /* CIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &im_data_u[i][0];
        ptr_data_v = &im_data_v[i][0];
        ptr_data_y = &im_data_y[data_lig++][0];

        for(j = 0; j < 176; j++){ /* CIF_WIDTH_DIV2 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	    *ptr_data_v++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	}
	ptr_buf_lig += 384*2; /* CIF_LIG_STEP */
	ptr_buf_col = ptr_buf_lig + 1;
	ptr_data_y = &im_data_y[data_lig++][0];

	for(j = 0; j < 352; j++){
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col+= 2; /* CIF_COL_STEP */
	}
	ptr_buf_lig+= 384*2; /* CIF_LIG_STEP */
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get ntsc color CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_ntsc_color_cif(data_y, data_u, data_v)
    u_char data_y[][CIF_WIDTH];
    u_char data_u[][CIF_WIDTH_DIV2];
    u_char data_v[][CIF_WIDTH_DIV2];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    for(i = 0; i < CIF_HEIGHT; i++)
	for(j = 0; j < CIF_WIDTH; j++)
	    data_y[i][j] = 219;

    for(i = 0; i < CIF_HEIGHT_DIV2; i++)
	for(j = 0; j < CIF_WIDTH_DIV2; j++){
	    data_u[i][j] = 100;
	    data_v[i][j] = 100;
	}

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 24;
    ptr_buf_lig = ptr_buf;

    for(i = 12; i < 132; i++ ){ /* 132 - 12 = NTSC_CIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &data_u[i][8];
        ptr_data_v = &data_v[i][8];
        ptr_data_y = &data_y[data_lig++][16];

        for(j = 0; j < 160; j++){ /* NTSC_CIF_WIDTH_DIV2 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = ccir_y[*ptr_buf_col++];
	    *ptr_data_v++ = *ptr_buf_col++;
	    *ptr_data_y++ = ccir_y[*ptr_buf_col++];
	}
	ptr_buf_lig += 640; /* NTSC_CIF_LIG_STEP */
	ptr_buf_col = ptr_buf_lig + 1;
	ptr_data_y = &data_y[data_lig++][16];

	for(j = 0; j < 320; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col+= 2; /* CIF_COL_STEP */
	}
	ptr_buf_lig+= 640; /* NTSC_CIF_LIG_STEP */
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |	test: new get pal color CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_pal_color_cif_zoom2_new(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][CIF_WIDTH_DIV2];
    u_char im_data_v[][CIF_WIDTH_DIV2];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 0;
    ptr_buf_lig = ptr_buf;

    for(i = 0; i < 288; i++ ){ /* CIF_HEIGHT */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &im_data_u[i][0];
        ptr_data_v = &im_data_v[i][0];
        ptr_data_y = &im_data_y[i][0];

        for(j = 0; j < 88; j++){ /* CIF_WIDTH_DIV4 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	    *ptr_data_v++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col += 2;
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col += 2;
	    *ptr_data_y++ = *ptr_buf_col++;
	}
	ptr_buf_lig += 384*2;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }
}




/*--------------------------------------------------------------*
 |	get pal color QCIF image: with zoom setting 2		|
 *--------------------------------------------------------------*/
void galileo_pal_color_qcif_zoom2(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][QCIF_WIDTH];
    u_char im_data_v[][QCIF_WIDTH];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 0;
    ptr_buf_lig = ptr_buf;

    for(i = 0; i < 72; i++ ){ /* QCIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = (u_char *)(im_data_u + i*LIG_MAX_DIV2);
        ptr_data_v = (u_char *)(im_data_v + i*LIG_MAX_DIV2);
        ptr_data_y = (u_char *)(im_data_y + data_lig*LIG_MAX);
	data_lig ++;

        for(j = 0; j < 88; j++){ /* QCIF_WIDTH_DIV2 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	    *ptr_data_v++ = *ptr_buf_col++;
	    *ptr_data_y++ = *ptr_buf_col++;
	     ptr_buf_col+= 4; /*QCIF_COL_STEP */
	}
	ptr_buf_lig += 384*4; /* QCIF_LIG_STEP */
	ptr_buf_col = ptr_buf_lig + 1;
	ptr_data_y = (u_char *)(im_data_y +data_lig*LIG_MAX);
	data_lig ++;

	for(j = 0; j < 176; j++){
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col+= 4; /* CIF_COL_STEP */
	}
	ptr_buf_lig+= 384*4; /* QCIF_LIG_STEP */
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }

}

/*--------------------------------------------------------------*
 |	get ntsc color QCIF image: with zoom setting 2		|
 *--------------------------------------------------------------*/
void galileo_ntsc_color_qcif(data_y, data_u, data_v)
    u_char data_y[][CIF_WIDTH];
    u_char data_u[][QCIF_WIDTH];
    u_char data_v[][QCIF_WIDTH];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    for(i = 0; i < QCIF_HEIGHT; i++)
	for(j = 0; j < CIF_WIDTH; j++)
	    data_y[i][j] = 219;

    for(i = 0; i < QCIF_HEIGHT_DIV2; i++)
	for(j = 0; j < CIF_WIDTH_DIV2; j++){
	    data_u[i][j] = 100;
	    data_v[i][j] = 100;
	}

    do{
	buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 12;
    ptr_buf_lig = ptr_buf;

    for(i = 6; i < 66; i++ ){ /* 66 - 6 = NTSC_QCIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &data_u[i][4];
        ptr_data_v = &data_v[i][4];
        ptr_data_y = &data_y[data_lig++][8];

        for(j = 0; j < 80; j++){ /* QCIF_WIDTH_DIV2 */
	    *ptr_data_u++ = *ptr_buf_col++;
	    *ptr_data_y++ = ccir_y[*ptr_buf_col++];
	    *ptr_data_v++ = *ptr_buf_col++;
	    *ptr_data_y++ = ccir_y[*ptr_buf_col++];
	     ptr_buf_col+= 4; /*QCIF_COL_STEP */
	}
	ptr_buf_lig += 1280; /* NTSC_QCIF_LIG_STEP */
	ptr_buf_col = ptr_buf_lig + 1;
	ptr_data_y = &data_y[data_lig++][8];

	for(j = 0; j < 160; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col+= 4; /* CIF_COL_STEP */
	}
	ptr_buf_lig+= 1280; /* QCIF_LIG_STEP */
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }

}

/*----------------------------------------------------------------------*
 |		get pal BW QCIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_pal_bw_qcif_zoom2(data_y)
    unsigned  char data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf /* + 768*4 */ + 1;

    for(data_lig = 0; data_lig < QCIF_HEIGHT; data_lig++){
        ptr_buf_col = ptr_buf_lig;
        ptr_data_y = &data_y[data_lig][0];
        for(j = 0; j < QCIF_WIDTH; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
             ptr_buf_col += 4;
        }
        ptr_buf_lig += 768*2;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get ntsc BW QCIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_ntsc_bw_qcif(data_y)
    unsigned  char data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  i, j, data_lig;

    for(i = 0; i < QCIF_HEIGHT; i++)
	for(j = 0; j < CIF_WIDTH; j++)
	    data_y[i][j] = 219;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf + 1;

    for(data_lig = 12; data_lig < 132; data_lig++){
        ptr_buf_col = ptr_buf_lig;
        ptr_data_y = &data_y[data_lig][8];
        for(j = 0; j < 160; j++){
            *ptr_data_y++ = *ptr_buf_col;
             ptr_buf_col += 4;
        }
        ptr_buf_lig += 1280;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get pal BW QCIF image:  without zoom setting		|
 *----------------------------------------------------------------------*/
void gal_pal_bw_qcif(im_data_y)
    unsigned  char im_data_y[][QCIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;

    ptr_buf_lig = ptr_buf /* + 768*4 */ + 1;

    for(data_lig = 0; data_lig < CIF_HEIGHT; data_lig++){
        ptr_buf_col = ptr_buf_lig;
        ptr_data_y = &im_data_y[data_lig][0];
        for(j = 0; j < QCIF_WIDTH; j++){
            *ptr_data_y++ = ccir_y[*ptr_buf_col];
             ptr_buf_col += 8;
        }
        ptr_buf_lig += 768*4;
    }

}


/*------------------------------------------------------------------*
 |            	get pal BW CIF image: zoom setting 1	            |
 *------------------------------------------------------------------*/
void galileo_pal_bw_cif_zoom1(im_data_y)
    unsigned  char im_data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf + 1;


    for(data_lig = 0; data_lig < 288; data_lig++){
 	ptr_buf_col = ptr_buf_lig;
	ptr_data_y = &im_data_y[data_lig][0];
	for(j = 0; j < 352; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col += 4;
	}
        ptr_buf_lig += 768*4;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get pal BW CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_pal_bw_cif(im_data_y)
    unsigned  char im_data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf /*+ 768*4 */ + 1;


    for(data_lig = 0; data_lig < 288; data_lig++){
 	ptr_buf_col = ptr_buf_lig;
	ptr_data_y = &im_data_y[data_lig][0];
	for(j = 0; j < 352; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col += 2;
	}
        ptr_buf_lig += 384*2;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}



/*----------------------------------------------------------------------*
 |		new get pal BW CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_pal_bw_cif_zoom2_new(im_data_y)
    unsigned  char im_data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf /*+ 768*4 */ + 1;


    for(data_lig = 0; data_lig < 288; data_lig++){
 	ptr_buf_col = ptr_buf_lig;
	ptr_data_y = &im_data_y[data_lig][0];
	for(j = 0; j < 352; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col += 2;
	}
        ptr_buf_lig += 384*2;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}

/*----------------------------------------------------------------------*
 |		new get ntsc BW CIF image:  with zoom setting 2		|
 *----------------------------------------------------------------------*/
void galileo_ntsc_bw_cif(data_y)
    unsigned  char data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  i, j, data_lig;

    for(i = 0; i < CIF_HEIGHT; i++)
	for(j = 0; j < CIF_WIDTH; j++)
	    data_y[i][j] = 219;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf + 1;

    for(data_lig = 24; data_lig < 264; data_lig++){
 	ptr_buf_col = ptr_buf_lig;
	ptr_data_y = &data_y[data_lig][16];
	for(j = 0; j < 320; j++){
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col += 2;
	}
        ptr_buf_lig += 640;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get pal BW QCIF image:  with zoom setting 4		|
 *----------------------------------------------------------------------*/
void galileo_pal_bw_qcif(im_data_y)
    unsigned  char im_data_y[][CIF_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_y;
    register  j, data_lig;


    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    ptr_buf_lig = ptr_buf + 1;


    for(data_lig = 0; data_lig < 144; data_lig++){
 	ptr_buf_col = ptr_buf_lig;
	ptr_data_y = &im_data_y[data_lig][0];
	for(j = 0; j < 176; j++){
	    *ptr_data_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col += 2;
	}
        ptr_buf_lig += 384;
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "Error in galileo_pal_bw_cif: vlPutFree\n");
        vlPerror(ptr_str);
        exit(1);
    }
}



/*----------------------------------------------------------------------*
 |		get pal BW CIF4 image:  without zoom setting		|
 *----------------------------------------------------------------------*/
void galileo_pal_bw_cif4(im_data_y)
    unsigned  char im_data_y[][CIF4_WIDTH];
{
    register  unsigned char *ptr_buf_col, *ptr_buf_lig;
    register  unsigned char *ptr_data_even_y, *ptr_data_odd_y;
    register  i, j, data_lig;

    data_lig = 0;
    ptr_buf_lig = ptr_buf + 1;

    for(i = 0; i < CIF4_HEIGHT; i++){
        ptr_buf_col = ptr_buf_lig;
        ptr_data_even_y = &im_data_y[data_lig++][0];

        for(j = 0; j < CIF_WIDTH; j++){
            *ptr_data_even_y++ = ccir_y[*ptr_buf_col];
	     ptr_buf_col += 2;
        }
        ptr_buf_lig += 768*2;
    }
}


/*----------------------------------------------------------------------*
 |              get pal color QCIF image:  with zoom setting 4		|
 *----------------------------------------------------------------------*/
void galileo_pal_color_qcif(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][QCIF_WIDTH];
    u_char im_data_v[][QCIF_WIDTH];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;

    do{
        buf_info = vlGetNextValid(sgi_video_hdl, trans_buf);
    }while(!buf_info);

    ptr_buf = vlGetActiveRegion(sgi_video_hdl, trans_buf, buf_info);

    data_lig = 0;
    ptr_buf_lig = ptr_buf;

    for(i = 0; i < 72; i++ ){ /* QCIF_HEIGHT_DIV2 */
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &im_data_u[i][0];
        ptr_data_v = &im_data_v[i][0];
        ptr_data_y = &im_data_y[data_lig++][0];

        for(j = 0; j < 88; j++){ /* QCIF_WIDTH_DIV2 */
            *ptr_data_u++ = *ptr_buf_col++;
            *ptr_data_y++ = *ptr_buf_col++;
            *ptr_data_v++ = *ptr_buf_col++;
            *ptr_data_y++ = *ptr_buf_col++;
        }
        ptr_buf_lig += 384; /* GAL_QCIF_LIG_STEP */
        ptr_buf_col = ptr_buf_lig + 1;
        ptr_data_y = &im_data_y[data_lig++][0];

        for(j = 0; j < 176; j++){ /* QCIF_WIDTH */
            *ptr_data_y++ = *ptr_buf_col;
             ptr_buf_col += 2;
        }
        ptr_buf_lig += 384; /* GAL_QCIF_LIG_STEP */
    }

    if((vlPutFree(sgi_video_hdl, trans_buf)) < 0){
        fprintf(stderr, "by galileo_pal_col_cif: ");
        vlPerror("vlPutFree\n");
        exit(1);
    }
}


/*----------------------------------------------------------------------*
 |		get pal color QCIF image:  without zoom setting         |
 *----------------------------------------------------------------------*/
void gal_pal_color_qcif(im_data_y, im_data_u, im_data_v)
    u_char im_data_y[][CIF_WIDTH];
    u_char im_data_u[][QCIF_WIDTH_DIV2];
    u_char im_data_v[][QCIF_WIDTH_DIV2];
{
    register  u_char *ptr_buf_col, *ptr_buf_lig;
    register  u_char  *ptr_data_y, *ptr_data_u, *ptr_data_v;
    register  i, j, data_lig;


    data_lig = 0;
    ptr_buf_lig = ptr_buf /*+ 768*4 + 64*/;

    for(i = 0; i < QCIF_HEIGHT; i++ ){
        ptr_buf_col = ptr_buf_lig; /* X_OFFSET */
        ptr_data_u = &im_data_u[i][0];
        ptr_data_v = &im_data_v[i][0];
        ptr_data_y = &im_data_y[data_lig++][0];

        for(j = 0; j < QCIF_WIDTH_DIV2; j++){
            *ptr_data_u++ = *ptr_buf_col++;
            *ptr_data_y++ = *ptr_buf_col++;
            *ptr_data_v++ = *ptr_buf_col;
             ptr_buf_col+= 7; /* GAL_QCIF_COL_STEP */
	    *ptr_data_y++ = *ptr_buf_col;
	     ptr_buf_col+= 7; /* GAL_QCIF_COL_STEP */
        }
        ptr_buf_lig += 768*8; /* GAL_QCIF_LIG_STEP */
    }
}


/*********************************
 *	get pal color CIF4       *
 *********************************/
void galileo_pal_color_cif4(im_data_y, im_data_u, im_data_v)
	u_char *im_data_y;
	u_char *im_data_u;
	u_char *im_data_v;
{
}


/*********************************
 *	get ntsc color CIF4       *
 *********************************/
void galileo_ntsc_color_cif4(im_data_y, im_data_u, im_data_v)
	u_char *im_data_y;
	u_char *im_data_u;
	u_char *im_data_v;
{
}


/*********************************
 *	get ntsc color CIF4       *
 *********************************/
void galileo_ntsc_bw_cif4(im_data_y)
	u_char *im_data_y;
{
}


#endif /* GALILEOVIDEO */



