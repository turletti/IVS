/**************************************************************************\
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
* 	                						   *
*  File    : ffct.c	                        			   *
*  Date    : September 1992          					   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description : Perform a fast DCT on a 8x8 block. ffct_4x4() only keeps  *
*    4x4 coefficients. Returns maximum coefficient value and the cmax      *
*    column such as block_out[][cmax+1]==0.                                *
*    ffct_intra_XxX() procedure are necessary to process intra MB,         *
*    because input image is in unsigned char format. (directly grabbed by  *
*    VideoPix).                                                            *
*    The algorithm is refered to "A new Two-Dimensional Fast Cosine        *
*    Transform Algorithm". IEEE Trans. on signal proc. vol 39 no. 2        *
*    (S.C. Chan & K.L. Ho).                                                *
*                                                                          *
*--------------------------------------------------------------------------*
* 10/1/92 wkd@Hawaii.Edu                                                   *
* Modified by Winston Dang to use pointers as much possible rather than    *
* two dimensional array lookups. (tmpblk_ptr instead of tmpblk[][]).       *
*                                                                          *
\**************************************************************************/
  

#include "h261.h"

#define BOOLEAN unsigned char

#define Nb_coeff_max 10
#define COL_MAX 352
#define CODE_CC(x) ((x) > 254.0 ? 254 : (int)(x))

#define UnSurRac2  0.70710678
#define UnSur8Rac2 0.088388348
#define Rac2Sur8   0.1767767
#define UnSur8     0.125
#define cf1_0      1.9615706
#define cf1_1      1.1111405
#define cf1_2      -0.39018064
#define cf1_3      -1.6629392
#define cf2_0      1.8477591
#define cf2_1      -0.76536686


static float tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
static float tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
static float tmpblk[8][8];
float *tmpblk_ptr;
int *block_ptr;


int ffct_8x8(image_in, block_out, L_col, inter, cmax)
     int *image_in;      /* may be negative       */
     BLOCK8x8 block_out; /* output coefficients   */
     int L_col;          /* image_in column max   */
     BOOLEAN inter;      /* True if INTER mode    */
     int *cmax;          /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    int col2, col3, col4, col5, col6, col7;

    col2 = L_col << 1;
    col3 = col2 + L_col;
    col4 = col3 + L_col;
    col5 = col4 + L_col;
    col6 = col5 + L_col;
    col7 = col6 + L_col;

    for(v=0; v<8; v++){
      register int p1, p2;

      p1 = *(image_in+v); p2 = *(image_in+col7+v);
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1 = *(image_in+col2+v); p2 = *(image_in+col5+v);
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1 = *(image_in+col4+v); p2 = *(image_in+col3+v);
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1 = *(image_in+col6+v); p2 = *(image_in+L_col+v);
      tmp2_3 = p1 + p2;
      tmp2_7 = (p1 - p2) * cf1_3;

      tmp1_0 =  tmp2_0 + tmp2_2;
      tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
      tmp1_1 =  tmp2_1 + tmp2_3;
      tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
      tmp1_4 =  tmp2_4 + tmp2_6;
      tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
      tmp1_5 =  tmp2_5 + tmp2_7;
      tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

      tmpblk_ptr = &tmpblk[0][v];
      *tmpblk_ptr = tmp1_0 + tmp1_1;
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk_ptr += 8;
      *tmpblk_ptr = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
	             - 0.5 * (tmp1_4 + tmp1_5);
    }

      for(u=0; u<8; u++){
	{
	  register float p1, p2;
	  register float *pt_tmpblk = &(tmpblk[u][0]);

	  p1 = *pt_tmpblk;
	  p2 = *(pt_tmpblk+7);
	  tmp2_0 = p1 + p2;
	  tmp2_4 = cf1_0*(p1 - p2);

	  p1 = *(pt_tmpblk+2);
	  p2 = *(pt_tmpblk+5);
	  tmp2_1 = p1 + p2;
	  tmp2_5 = cf1_1*(p1 - p2);

	  p1 = *(pt_tmpblk+4);
	  p2 = *(pt_tmpblk+3);
	  tmp2_2 = p1 + p2;
	  tmp2_6 = cf1_2*(p1 - p2);

	  p1 = *(pt_tmpblk+6);
	  p2 = *(pt_tmpblk+1);
	  tmp2_3 = p1 + p2;	
	  tmp2_7 = cf1_3*(p1 - p2);
	}
	tmp1_0 =  tmp2_0 + tmp2_2;
	tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
	tmp1_1 =  tmp2_1 + tmp2_3;
	tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
	tmp1_4 = tmp2_4 + tmp2_6;
	tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
	tmp1_5 = tmp2_5 + tmp2_7;
	tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

	if(u==0){
	  block_ptr = &block_out[0][0];
	  if(inter)
	    *block_ptr++ = (int)(UnSur8*(tmp1_0 + tmp1_1));
	  else
	    *block_ptr++ = CODE_CC((tmp1_0 + tmp1_1)/64);
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  *block_ptr++ = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	  *block_ptr++ = (int)(UnSur8*(tmp1_0 - tmp1_1));
	  *block_ptr++ = (int)(UnSur8*(tmp1_4 - tmp1_5)) - block_out[0][3];
	  *block_ptr++ = (int)((UnSur8*(tmp1_2 - tmp1_3) 
				- UnSur8Rac2*(tmp1_2  + tmp1_3)));
	  *block_ptr++ = (int)((UnSur8*(tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) 
				- UnSur8Rac2*(tmp1_4 + tmp1_5)));
	}else{
	  block_ptr = &block_out[u][0];
	  *block_ptr++ = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  *block_ptr++ = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
		         - (tmp1_4  + tmp1_5)));
	  *block_ptr++ = (int)(((tmp1_0 - tmp1_1)*Rac2Sur8));
	  *block_ptr++ = (int)(((tmp1_4 - tmp1_5) * Rac2Sur8)) 
	                 - block_out[u][3];
	  *block_ptr++ = (int)((((tmp1_2 - tmp1_3) * Rac2Sur8) 
				- (UnSur8 * (tmp1_2  + tmp1_3))));
	  *block_ptr++ = (int)(((tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5)*Rac2Sur8
				- UnSur8 * (tmp1_4 + tmp1_5)));
	}
      }
    /* Now, we have to find coeff max and cmax value (block_out[][cmax+1]=0) */
    v=7;
    do{
      for(u=0; u<8; u++)
	  if(block_out[u][v]){
	    nonul=1;
	    break;
	  }
      v --;
    }while((!nonul) && (v>=0));
    v ++;
    {
      register int w;
      register int aux;

      for(u=0; u<8; u++){
	block_ptr = &block_out[u][0];
	for(w=0; w<=v; w++){
	  if(aux = *block_ptr++){
	    if(aux>0){
	      if(aux>max)
		max=aux;
	    }else
	      if(-aux>max)
		max = -aux;
	  }
	}
      }
    }
    *cmax=v;
    return(max);
  }



int ffct_4x4(image_in, block_out, L_col, inter, cmax)
     int *image_in;      /* may be negative       */
     BLOCK8x8 block_out; /* output coefficients   */
     int L_col;          /* image_in column max   */
     BOOLEAN inter;      /* True if INTER mode    */
     int *cmax;          /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    int col2, col3, col4, col5, col6, col7;

    col2 = L_col << 1;
    col3 = col2 + L_col;
    col4 = col3 + L_col;
    col5 = col4 + L_col;
    col6 = col5 + L_col;
    col7 = col6 + L_col;

    for(v=0; v<8; v++){
      register int p1, p2;

      p1 = *(image_in+v); p2 = *(image_in+col7+v);
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1 = *(image_in+col2+v); p2 = *(image_in+col5+v);
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1 = *(image_in+col4+v); p2 = *(image_in+col3+v);
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1 = *(image_in+col6+v); p2 = *(image_in+L_col+v);
      tmp2_3 = p1 + p2;
      tmp2_7 = (p1 - p2) * cf1_3;

      tmp1_0 =  tmp2_0 + tmp2_2;
      tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
      tmp1_1 =  tmp2_1 + tmp2_3;
      tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
      tmp1_4 =  tmp2_4 + tmp2_6;
      tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
      tmp1_5 =  tmp2_5 + tmp2_7;
      tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

      tmpblk_ptr = &tmpblk[0][v];
      *tmpblk_ptr = tmp1_0 + tmp1_1;
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk_ptr += 8;
      *tmpblk_ptr = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
	             - 0.5 * (tmp1_4 + tmp1_5);
    }

      for(u=0; u<4; u++){
	{
	  register float p1, p2;
	  register float *pt_tmpblk = &(tmpblk[u][0]);

	  p1 = *pt_tmpblk;
	  p2 = *(pt_tmpblk+7);
	  tmp2_0 = p1 + p2;
	  tmp2_4 = cf1_0*(p1 - p2);

	  p1 = *(pt_tmpblk+2);
	  p2 = *(pt_tmpblk+5);
	  tmp2_1 = p1 + p2;
	  tmp2_5 = cf1_1*(p1 - p2);

	  p1 = *(pt_tmpblk+4);
	  p2 = *(pt_tmpblk+3);
	  tmp2_2 = p1 + p2;
	  tmp2_6 = cf1_2*(p1 - p2);

	  p1 = *(pt_tmpblk+6);
	  p2 = *(pt_tmpblk+1);
	  tmp2_3 = p1 + p2;	
	  tmp2_7 = cf1_3*(p1 - p2);
	}
	tmp1_0 =  tmp2_0 + tmp2_2;
	tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
	tmp1_1 =  tmp2_1 + tmp2_3;
	tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
	tmp1_4 = tmp2_4 + tmp2_6;
	tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
	tmp1_5 = tmp2_5 + tmp2_7;
	tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

	if(u==0){
	  block_ptr = &block_out[0][0];
	  if(inter)
	    *block_ptr++ = (int)(UnSur8*(tmp1_0 + tmp1_1));
	  else
	    *block_ptr++ = CODE_CC((tmp1_0 + tmp1_1)/64);
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  *block_ptr++ = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	}else{
	  block_ptr = &block_out[u][0];
	  *block_ptr++ = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  *block_ptr++ = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
					 - (tmp1_4  + tmp1_5)));
	}
      }
    v=3;
    do{
      for(u=0; u<4; u++)
	  if(block_out[u][v]){
	    nonul=1;
	    break;
	  }
      v --;
    }while((!nonul) && (v>=0));
    v ++;
    {
      register int w;
      register int aux;

      for(u=0; u<4; u++){
	block_ptr = &block_out[u][0];
	for(w=0; w<=v; w++){
	  if(aux = *block_ptr++){
	    if(aux>0){
	      if(aux>max)
		max=aux;
	    }else
	      if(-aux>max)
		max = -aux;
	  }
	}
      }
    }
    *cmax=v;
    return(max);
  }


int ffct_intra_8x8(image_in, block_out, L_col, cmax)
     u_char *image_in; /* always positive */
     BLOCK8x8 block_out;
     int L_col; /* image_in column max */
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    int col2, col3, col4, col5, col6, col7;

    col2 = L_col << 1;
    col3 = col2 + L_col;
    col4 = col3 + L_col;
    col5 = col4 + L_col;
    col6 = col5 + L_col;
    col7 = col6 + L_col;

    for(v=0; v<8; v++){
      register int p1, p2;

      p1 = (int) *(image_in+v); p2 = (int) *(image_in+col7+v);
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1 = (int) *(image_in+col2+v); p2 = (int) *(image_in+col5+v);
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1 = (int) *(image_in+col4+v); p2 = (int) *(image_in+col3+v);
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1 = (int) *(image_in+col6+v); p2 = (int) *(image_in+L_col+v);
      tmp2_3 = p1 + p2;
      tmp2_7 = (p1 - p2) * cf1_3;

      tmp1_0 =  tmp2_0 + tmp2_2;
      tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
      tmp1_1 =  tmp2_1 + tmp2_3;
      tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
      tmp1_4 =  tmp2_4 + tmp2_6;
      tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
      tmp1_5 =  tmp2_5 + tmp2_7;
      tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

      tmpblk_ptr = &tmpblk[0][v];
      *tmpblk_ptr = tmp1_0 + tmp1_1;
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk_ptr += 8;
      *tmpblk_ptr = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
	             - 0.5 * (tmp1_4 + tmp1_5);
    }

      for(u=0; u<8; u++){
	{
	  register float p1, p2;
	  register float *pt_tmpblk = &(tmpblk[u][0]);

	  p1 = *pt_tmpblk;
	  p2 = *(pt_tmpblk+7);
	  tmp2_0 = p1 + p2;
	  tmp2_4 = cf1_0*(p1 - p2);

	  p1 = *(pt_tmpblk+2);
	  p2 = *(pt_tmpblk+5);
	  tmp2_1 = p1 + p2;
	  tmp2_5 = cf1_1*(p1 - p2);

	  p1 = *(pt_tmpblk+4);
	  p2 = *(pt_tmpblk+3);
	  tmp2_2 = p1 + p2;
	  tmp2_6 = cf1_2*(p1 - p2);

	  p1 = *(pt_tmpblk+6);
	  p2 = *(pt_tmpblk+1);
	  tmp2_3 = p1 + p2;	
	  tmp2_7 = cf1_3*(p1 - p2);
	}
	tmp1_0 =  tmp2_0 + tmp2_2;
	tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
	tmp1_1 =  tmp2_1 + tmp2_3;
	tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
	tmp1_4 = tmp2_4 + tmp2_6;
	tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
	tmp1_5 = tmp2_5 + tmp2_7;
	tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

	if(u==0){
	  block_ptr = &block_out[0][0];
	  *block_ptr++ = CODE_CC((tmp1_0 + tmp1_1)/64);
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  *block_ptr++ = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	  *block_ptr++ = (int)(UnSur8*(tmp1_0 - tmp1_1));
	  *block_ptr++ = (int)(UnSur8*(tmp1_4 - tmp1_5)) - block_out[0][3];
	  *block_ptr++ = (int)((UnSur8*(tmp1_2 - tmp1_3) 
				- UnSur8Rac2*(tmp1_2  + tmp1_3)));
	  *block_ptr++ = (int)((UnSur8*(tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) 
				- UnSur8Rac2*(tmp1_4 + tmp1_5)));
	}else{
	  block_ptr = &block_out[u][0];
	  *block_ptr++ = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  *block_ptr++ = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
		         - (tmp1_4  + tmp1_5)));
	  *block_ptr++ = (int)(((tmp1_0 - tmp1_1)*Rac2Sur8));
	  *block_ptr++ = (int)(((tmp1_4 - tmp1_5) * Rac2Sur8)) 
	                 - block_out[u][3];
	  *block_ptr++ = (int)((((tmp1_2 - tmp1_3) * Rac2Sur8) 
				- (UnSur8 * (tmp1_2  + tmp1_3))));
	  *block_ptr++ = (int)(((tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5)*Rac2Sur8
				- UnSur8 * (tmp1_4 + tmp1_5)));
	}
      }
    /* Now, we have to find coeff max and cmax value (block_out[][cmax+1]=0) */
    v=7;
    do{
      for(u=0; u<8; u++)
	  if(block_out[u][v]){
	    nonul=1;
	    break;
	  }
      v --;
    }while((!nonul) && (v>=0));
    v ++;
    {
      register int w;
      register int aux;

      for(u=0; u<8; u++){
	block_ptr = &block_out[u][0];
	for(w=0; w<=v; w++){
	  if(aux = *block_ptr++){
	    if(aux>0){
	      if(aux>max)
		max=aux;
	    }else
	      if(-aux>max)
		max = -aux;
	  }
	}
      }
    }
    *cmax=v;
    return(max);
  }



int ffct_intra_4x4(image_in, block_out, L_col, cmax)
     u_char *image_in; /* always positive */
     BLOCK8x8 block_out;
     int L_col; /* image_in column max */
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    int col2, col3, col4, col5, col6, col7;

    col2 = L_col << 1;
    col3 = col2 + L_col;
    col4 = col3 + L_col;
    col5 = col4 + L_col;
    col6 = col5 + L_col;
    col7 = col6 + L_col;

    for(v=0; v<8; v++){
      register int p1, p2;

      p1 = (int) *(image_in+v); p2 = (int) *(image_in+col7+v);
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1 = (int) *(image_in+col2+v); p2 = (int) *(image_in+col5+v);
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1 = (int) *(image_in+col4+v); p2 = (int) *(image_in+col3+v);
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1 = (int) *(image_in+col6+v); p2 = (int) *(image_in+L_col+v);
      tmp2_3 = p1 + p2;
      tmp2_7 = (p1 - p2) * cf1_3;

      tmp1_0 =  tmp2_0 + tmp2_2;
      tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
      tmp1_1 =  tmp2_1 + tmp2_3;
      tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
      tmp1_4 =  tmp2_4 + tmp2_6;
      tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
      tmp1_5 =  tmp2_5 + tmp2_7;
      tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

      tmpblk_ptr = &tmpblk[0][v];
      *tmpblk_ptr = tmp1_0 + tmp1_1;
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk_ptr += 8;
      *tmpblk_ptr = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk_ptr += 8;
      *tmpblk_ptr = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
	             - 0.5 * (tmp1_4 + tmp1_5);
    }

      for(u=0; u<4; u++){
	{
	  register float p1, p2;
	  register float *pt_tmpblk = &(tmpblk[u][0]);

	  p1 = *pt_tmpblk;
	  p2 = *(pt_tmpblk+7);
	  tmp2_0 = p1 + p2;
	  tmp2_4 = cf1_0*(p1 - p2);

	  p1 = *(pt_tmpblk+2);
	  p2 = *(pt_tmpblk+5);
	  tmp2_1 = p1 + p2;
	  tmp2_5 = cf1_1*(p1 - p2);

	  p1 = *(pt_tmpblk+4);
	  p2 = *(pt_tmpblk+3);
	  tmp2_2 = p1 + p2;
	  tmp2_6 = cf1_2*(p1 - p2);

	  p1 = *(pt_tmpblk+6);
	  p2 = *(pt_tmpblk+1);
	  tmp2_3 = p1 + p2;	
	  tmp2_7 = cf1_3*(p1 - p2);
	}
	tmp1_0 =  tmp2_0 + tmp2_2;
	tmp1_2 = (tmp2_0 - tmp2_2) * cf2_0;
	tmp1_1 =  tmp2_1 + tmp2_3;
	tmp1_3 = (tmp2_1 - tmp2_3) * cf2_1;
	tmp1_4 = tmp2_4 + tmp2_6;
	tmp1_6 = (tmp2_4 - tmp2_6) * cf2_0;
	tmp1_5 = tmp2_5 + tmp2_7;
	tmp1_7 = (tmp2_5 - tmp2_7) * cf2_1;

	if(u==0){
	  block_ptr = &block_out[0][0];
	  *block_ptr++ = CODE_CC((tmp1_0 + tmp1_1)/64);
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  *block_ptr++ = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  *block_ptr++ = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	}else{
	  block_ptr = &block_out[u][0];
	  *block_ptr++ = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  *block_ptr++ = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  *block_ptr++ = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
					 - (tmp1_4  + tmp1_5)));
	}
      }
    v=3;
    do{
      for(u=0; u<4; u++)
	  if(block_out[u][v]){
	    nonul=1;
	    break;
	  }
      v --;
    }while((!nonul) && (v>=0));
    v ++;
    {
      register int w;
      register int aux;

      for(u=0; u<4; u++){
	block_ptr = &block_out[u][0];
	for(w=0; w<=v; w++){
	    if(aux = *block_ptr++){
	      if(aux>0){
		if(aux>max)
		  max=aux;
	      }else
		if(-aux>max)
		  max = -aux;
	    }
	  }
      }
    }
    *cmax=v;
    return(max);
  }



