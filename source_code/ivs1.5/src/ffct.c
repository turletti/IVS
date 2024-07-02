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
\**************************************************************************/
  

#include "h261.h"

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



int ffct_8x8(image_in, block_out, lig, col, inter, cmax)
     int image_in[][COL_MAX]; /* may be negative */
     BLOCK8x8 block_out;
     int lig, col;
     int inter; /* Boolean: true if INTER mode */
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    float  tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
    float  tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
    float  tmpblk[8][8];

    for(v=0; v<8; v++){
      register int p1, p2, colv=col+v;

      p1=image_in[lig][colv];p2=image_in[lig+7][colv];
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1=image_in[lig+2][colv];p2=image_in[lig+5][colv];
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1=image_in[lig+4][colv];p2=image_in[lig+3][colv];
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1=image_in[lig+6][colv];p2=image_in[lig+1][colv];
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

      tmpblk[0][v] = tmp1_0 + tmp1_1;
      tmpblk[1][v] = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk[2][v] = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk[3][v] = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk[4][v] = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk[5][v] = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk[6][v] = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk[7][v] = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
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
	  if(inter)
	      block_out[0][0] = (int)(UnSur8*(tmp1_0 + tmp1_1));
	  else
	      block_out[0][0] = CODE_CC((tmp1_0 + tmp1_1)/64);
	  block_out[0][1] = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  block_out[0][2] = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  block_out[0][3] = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					     - (tmp1_4  + tmp1_5)));
	  block_out[0][4] = (int)(UnSur8*(tmp1_0 - tmp1_1));

	  block_out[0][5] = (int)(UnSur8*(tmp1_4 - tmp1_5)) - block_out[0][3];
	  block_out[0][6] = (int)((UnSur8*(tmp1_2 - tmp1_3) 
                                  - UnSur8Rac2*(tmp1_2  + tmp1_3)));
	  block_out[0][7] = (int)((UnSur8*(tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) 
                                  - UnSur8Rac2*(tmp1_4 + tmp1_5)));
	}else{
	  block_out[u][0] = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  block_out[u][1] = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  block_out[u][2] = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  block_out[u][3] = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	  block_out[u][4] = (int)(((tmp1_0 - tmp1_1)*Rac2Sur8));
	  block_out[u][5] = (int)(((tmp1_4 - tmp1_5) * Rac2Sur8)) 
	                          - block_out[u][3];
	  block_out[u][6] = (int)((((tmp1_2 - tmp1_3) * Rac2Sur8) 
				  - (UnSur8 * (tmp1_2  + tmp1_3))));
	  block_out[u][7] = (int)(((tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5)*Rac2Sur8
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

      for(u=0; u<8; u++)
	  for(w=0; w<=v; w++){
	    if((aux=block_out[u][w]) != 0){
	      if(aux>0){
		if(aux>max)
		    max=aux;
	      }else
		  if(-aux>max)
		      max = -aux;
	    }
	  }
    }
    *cmax=v;
    return(max);
  }



int ffct_4x4(image_in, block_out, lig, col, inter, cmax)
     int image_in[][COL_MAX]; /* may be negative */
     BLOCK8x8 block_out;
     int lig, col;
     int inter; /* Boolean: true if INTER mode */
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    float  tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
    float  tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
    float  tmpblk[8][8];

    for(v=0; v<8; v++){
      register int p1, p2, colv=col+v;

      p1=image_in[lig][colv];p2=image_in[lig+7][colv];
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1=image_in[lig+2][colv];p2=image_in[lig+5][colv];
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1=image_in[lig+4][colv];p2=image_in[lig+3][colv];
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1=image_in[lig+6][colv];p2=image_in[lig+1][colv];
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

      tmpblk[0][v] = tmp1_0 + tmp1_1;
      tmpblk[1][v] = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk[2][v] = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk[3][v] = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk[4][v] = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk[5][v] = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk[6][v] = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk[7][v] = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
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
	  if(inter)
	      block_out[0][0] = (int)(UnSur8*(tmp1_0 + tmp1_1));
	  else
	      block_out[0][0] = CODE_CC((tmp1_0 + tmp1_1)/64);
	  block_out[0][1] = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  block_out[0][2] = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  block_out[0][3] = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					     - (tmp1_4  + tmp1_5)));

	}else{
	  block_out[u][0] = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  block_out[u][1] = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  block_out[u][2] = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  block_out[u][3] = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
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

      for(u=0; u<4; u++)
	  for(w=0; w<=v; w++){
	    if((aux=block_out[u][w]) != 0){
	      if(aux>0){
		if(aux>max)
		    max=aux;
	      }else
		  if(-aux>max)
		      max = -aux;
	    }
	  }
    }
    *cmax=v;
    return(max);
  }


int ffct_intra_8x8(image_in, block_out, lig, col, cmax)
     u_char image_in[][COL_MAX]; /* always positive */
     BLOCK8x8 block_out;
     int lig, col;
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    float  tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
    float  tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
    float  tmpblk[8][8];

    for(v=0; v<8; v++){
      register int p1, p2, colv=col+v;

      p1=(int)image_in[lig][colv];p2=(int)image_in[lig+7][colv];
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1=(int)image_in[lig+2][colv];p2=(int)image_in[lig+5][colv];
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1=(int)image_in[lig+4][colv];p2=(int)image_in[lig+3][colv];
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1=(int)image_in[lig+6][colv];p2=(int)image_in[lig+1][colv];
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

      tmpblk[0][v] = tmp1_0 + tmp1_1;
      tmpblk[1][v] = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk[2][v] = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk[3][v] = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk[4][v] = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk[5][v] = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk[6][v] = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk[7][v] = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
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
	  block_out[0][0] = CODE_CC((tmp1_0 + tmp1_1)/64);
	  block_out[0][1] = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  block_out[0][2] = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  block_out[0][3] = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					     - (tmp1_4  + tmp1_5)));
	  block_out[0][4] = (int)(UnSur8*(tmp1_0 - tmp1_1));

	  block_out[0][5] = (int)(UnSur8*(tmp1_4 - tmp1_5)) - block_out[0][3];
	  block_out[0][6] = (int)((UnSur8*(tmp1_2 - tmp1_3) 
                                  - UnSur8Rac2*(tmp1_2  + tmp1_3)));
	  block_out[0][7] = (int)((UnSur8*(tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) 
                                  - UnSur8Rac2*(tmp1_4 + tmp1_5)));
	}else{
	  block_out[u][0] = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  block_out[u][1] = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  block_out[u][2] = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  block_out[u][3] = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
					   - (tmp1_4  + tmp1_5)));
	  block_out[u][4] = (int)(((tmp1_0 - tmp1_1)*Rac2Sur8));
	  block_out[u][5] = (int)(((tmp1_4 - tmp1_5) * Rac2Sur8)) 
	                          - block_out[u][3];
	  block_out[u][6] = (int)((((tmp1_2 - tmp1_3) * Rac2Sur8) 
				  - (UnSur8 * (tmp1_2  + tmp1_3))));
	  block_out[u][7] = (int)(((tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5)*Rac2Sur8
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

      for(u=0; u<8; u++)
	  for(w=0; w<=v; w++){
	    if((aux=block_out[u][w]) != 0){
	      if(aux>0){
		if(aux>max)
		    max=aux;
	      }else
		  if(-aux>max)
		      max = -aux;
	    }
	  }
    }
    *cmax=v;
    return(max);
  }



int ffct_intra_4x4(image_in, block_out, lig, col, cmax)
     u_char image_in[][COL_MAX]; /* always positive */
     BLOCK8x8 block_out;
     int lig, col;
     int *cmax; /* block_out[][cmax+1]=0 */
  {

    register int u,v;
    int nonul=0, max=0;
    float  tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
    float  tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
    float  tmpblk[8][8];

    for(v=0; v<8; v++){
      register int p1, p2, colv=col+v;

      p1=(int)image_in[lig][colv];p2=(int)image_in[lig+7][colv];
      tmp2_0 = p1 + p2;
      tmp2_4 = (p1 - p2) * cf1_0;
      p1=(int)image_in[lig+2][colv];p2=(int)image_in[lig+5][colv];
      tmp2_1 = p1 + p2;
      tmp2_5 = (p1 - p2) * cf1_1;
      p1=(int)image_in[lig+4][colv];p2=(int)image_in[lig+3][colv];
      tmp2_2 = p1 + p2;
      tmp2_6 = (p1 - p2) * cf1_2;
      p1=(int)image_in[lig+6][colv];p2=(int)image_in[lig+1][colv];
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

      tmpblk[0][v] = tmp1_0 + tmp1_1;
      tmpblk[1][v] = 0.5 * (tmp1_4 + tmp1_5);
      tmpblk[2][v] = 0.5 * (tmp1_2 + tmp1_3);
      tmpblk[3][v] = 0.5 * (tmp1_6 + tmp1_7 - tmp1_4 - tmp1_5);
      tmpblk[4][v] = (tmp1_0 - tmp1_1) * UnSurRac2;
      tmpblk[5][v] = (tmp1_4 - tmp1_5) * UnSurRac2 - tmpblk[3][v];
      tmpblk[6][v] = UnSurRac2*(tmp1_2 - tmp1_3) -  0.5 * (tmp1_2 + tmp1_3);
      tmpblk[7][v] = (tmp1_6 - tmp1_7 - tmp1_4 + tmp1_5) * UnSurRac2
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
	  block_out[0][0] = CODE_CC((tmp1_0 + tmp1_1)/64);
	  block_out[0][1] = (int)(UnSur8Rac2*(tmp1_4 + tmp1_5));
	  block_out[0][2] = (int)(UnSur8Rac2*(tmp1_2 + tmp1_3));
	  block_out[0][3] = (int)(UnSur8Rac2*((tmp1_6  + tmp1_7) 
					     - (tmp1_4  + tmp1_5)));

	}else{
	  block_out[u][0] = (int)(Rac2Sur8*(tmp1_0  + tmp1_1));
	  block_out[u][1] = (int)(UnSur8 * (tmp1_4  + tmp1_5));
	  block_out[u][2] = (int)(UnSur8 * (tmp1_2  + tmp1_3));
	  block_out[u][3] = (int)(UnSur8 * ((tmp1_6  + tmp1_7) 
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

      for(u=0; u<4; u++)
	  for(w=0; w<=v; w++){
	    if((aux=block_out[u][w]) != 0){
	      if(aux>0){
		if(aux>max)
		    max=aux;
	      }else
		  if(-aux>max)
		      max = -aux;
	    }
	  }
    }
    *cmax=v;
    return(max);
  }



