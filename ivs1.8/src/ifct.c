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
*  File    : ifct.c      	                			   *
*  Date    : May 1992		        				   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description : Perform a fast inverse DCT on a 8x8 block. The algorithm  *
*    is refered to "A new Two-Dimensional Fast Cosine Transform Algorithm" *
*    IEEE Trans. on signal proc. vol 39 no. 2 (S.C. Chan & K.L. Ho).       *
*                                                                          *
\**************************************************************************/

#include "h261.h"

#define ecrete(x) ((x)>255? 255 : ((x)>0 ? (x) : 0))

#define CF1_0 33409
#define CF1_1 58981
#define CF1_2 -167963
#define CF1_3 -39410
#define CF2_0 35468
#define CF2_1 -85627
#define Rac2  92682


ifct_8x8(block_fct, tab_quant1, tab_quant2, inter, block_out, colmax, L_y)
    BLOCK8x8 block_fct; /* Pointer to current 8x8 block of DCT coeff. */
    int *tab_quant1, *tab_quant2; /* Quantization */
    int inter; /* Boolean : True if INTER mode */
    u_char *block_out; /* output 8x8 block of pixels */
    int colmax; /* block_fct[colmax+1][] = 0 */
    int L_y; /* 352 if Y, 176 if Cr or Cb */
  {

    register int  u,v;
    int tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
    int tmp2_0, tmp2_1, tmp2_2, tmp2_3, tmp2_4, tmp2_5, tmp2_6, tmp2_7;
    int  tmpblk[8][8];


    for(v = 0; v <= colmax; v++){
      if(v==0){
	register int p1=tab_quant1[(block_fct[1][0]+127)];
	register int p2=tab_quant1[(block_fct[2][0]+127)];
	register int p3=tab_quant1[(block_fct[3][0]+127)];

	if(inter)
	    tmp1_0 = 2*tab_quant2[(block_fct[0][0]+127)];
	else
	    tmp1_0 = block_fct[0][0];
	tmp1_1 = tab_quant1[(block_fct[4][0]+127)];
	tmp1_2 = 2*p2;
	tmp1_3 = p2 + tab_quant1[(block_fct[6][0]+127)];
	tmp1_4 = 2*p1;
	tmp1_5 = tab_quant1[(block_fct[5][0]+127)] + p3;
	tmp1_6 = 2*(p3 + p1);
	tmp1_7 = p1 + tmp1_5 + tab_quant1[(block_fct[7][0]+127)];
      }else{
	register int p1=tab_quant2[(block_fct[1][v]+127)];
	register int p2=tab_quant2[(block_fct[2][v]+127)];
	register int p3=tab_quant2[(block_fct[3][v]+127)];

	tmp1_0 = tab_quant1[(block_fct[0][v]+127)];
	tmp1_1 = tab_quant2[(block_fct[4][v]+127)];
	tmp1_2 = 2*p2;
	tmp1_3 = p2 + tab_quant2[(block_fct[6][v]+127)];
	tmp1_4 = 2*p1;
	tmp1_5 = tab_quant2[(block_fct[5][v]+127)] + p3;
	tmp1_6 = 2*(p3 + p1);
	tmp1_7 = p1 + tmp1_5 + tab_quant2[(block_fct[7][v]+127)];
      }
      if (tmp1_0 || tmp1_1 || tmp1_2 || tmp1_3 || 
	  tmp1_4 || tmp1_5 || tmp1_6 || tmp1_7){
      {
	register int p1, p2, p3, p4;

	p1 = (tmp1_1*Rac2)/65536;
	p2 = (tmp1_3*Rac2)/65536;
	p3 = (tmp1_5*Rac2)/65536;
	p4 = (tmp1_7*Rac2)/65536;
	
	tmp2_0 = tmp1_0 + p1;
	tmp2_1 = tmp1_0 - p1;
	tmp2_2 = tmp1_2 + p2;
	tmp2_3 = tmp1_2 - p2;
	tmp2_4 = tmp1_4 + p3;
	tmp2_5 = tmp1_4 - p3;
	tmp2_6 = tmp1_6 + p4;
	tmp2_7 = tmp1_6 - p4;
      }
      {
	register int p1, p2, p3, p4;

	p1 = (tmp2_2*CF2_0)/65536;
	p2 = (tmp2_3*CF2_1)/65536;
	p3 = (tmp2_6*CF2_0)/65536;
	p4 = (tmp2_7*CF2_1)/65536;
	
	tmp1_0 = tmp2_0 + p1;
	tmp1_1 = tmp2_1 + p2;
	tmp1_2 = tmp2_0 - p1;
	tmp1_3 = tmp2_1 - p2;
	tmp1_4 = tmp2_4 + p3;
	tmp1_5 = tmp2_5 + p4;
	tmp1_6 = tmp2_4 - p3;
	tmp1_7 = tmp2_5 - p4;
      }
      {
	register int p1, p2, p3, p4;

	p1 = (tmp1_4*CF1_0)/65536;
	p2 = (tmp1_7*CF1_3)/65536;
	p3 = (tmp1_5*CF1_1)/65536;
	p4 = (tmp1_6*CF1_2)/65536;

	tmpblk[0][v] = tmp1_0 + p1;
	tmpblk[7][v] = tmp1_0 - p1;
	tmpblk[1][v] = tmp1_3 - p2;
	tmpblk[6][v] = tmp1_3 + p2;
	tmpblk[2][v] = tmp1_1 + p3;
	tmpblk[5][v] = tmp1_1 - p3;
	tmpblk[3][v] = tmp1_2 - p4;
	tmpblk[4][v] = tmp1_2 + p4;
      }
     }else{
       register i;

       for(i=0; i<8; i++)
	   tmpblk[i][v] = 0;
     }
    }
    {
      register i;

      for(v=colmax+1; v<8; v++)
	  for(i=0; i<8; i++)
	      tmpblk[i][v] = 0;
    }

    for(u=0; u<8; u++){

      register int offset=L_y*u;
      register int p1, p2, p3, p4;

      p1 = tmpblk[u][1];
      p2 = tmpblk[u][2];
      p3 = tmpblk[u][3];

      tmp1_0 = tmpblk[u][0];
      tmp1_1 = tmpblk[u][4];
      tmp1_2 = 2*p2;
      tmp1_3 = p2 + tmpblk[u][6];
      tmp1_4 = 2*p1;
      tmp1_5 = tmpblk[u][5] + p3;
      tmp1_6 = 2*(p3 + p1);
      tmp1_7 = p1 + tmp1_5 + tmpblk[u][7];

      p1 = (tmp1_1*Rac2)/65536;
      p2 = (tmp1_3*Rac2)/65536;
      p3 = (tmp1_5*Rac2)/65536;
      p4 = (tmp1_7*Rac2)/65536;
	
      tmp2_0 = tmp1_0 + p1;
      tmp2_1 = tmp1_0 - p1;
      tmp2_2 = tmp1_2 + p2;
      tmp2_3 = tmp1_2 - p2;
      tmp2_4 = tmp1_4 + p3;
      tmp2_5 = tmp1_4 - p3;
      tmp2_6 = tmp1_6 + p4;
      tmp2_7 = tmp1_6 - p4;

      p1 = (tmp2_2*CF2_0)/65536;
      p2 = (tmp2_3*CF2_1)/65536;
      p3 = (tmp2_6*CF2_0)/65536;
      p4 = (tmp2_7*CF2_1)/65536;
      
      tmp1_0 = tmp2_0 + p1;
      tmp1_1 = tmp2_1 + p2;
      tmp1_2 = tmp2_0 - p1;
      tmp1_3 = tmp2_1 - p2;
      tmp1_4 = tmp2_4 + p3;
      tmp1_5 = tmp2_5 + p4;
      tmp1_6 = tmp2_4 - p3;
      tmp1_7 = tmp2_5 - p4;

      p1 = (tmp1_4*CF1_0)/65536;
      p2 = (tmp1_7*CF1_3)/65536;
      p3 = (tmp1_5*CF1_1)/65536;
      p4 = (tmp1_6*CF1_2)/65536;
	
      if(inter){

	/************\
        * INTER MODE *
	\************/
	  
	register int aux;
	register u_char *block_out_ptr = &block_out[offset];

	aux = (int)*block_out_ptr + tmp1_0 + p1;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_3 - p2;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_1 + p3;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_2 - p4;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_2 + p4;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_1 - p3;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_3 + p2;
	*block_out_ptr++ = ecrete(aux);
	aux = (int)*block_out_ptr + tmp1_0 - p1;
	*block_out_ptr = ecrete(aux);

      }else{
	  
	/************\
        * INTRA MODE *
        \************/
	
	register int aux;
	
	aux = tmp1_0 + p1;
	block_out[offset]   = ecrete(aux);
	aux = tmp1_0 - p1;
	block_out[offset+7] = ecrete(aux);
	aux = tmp1_1 + p3;
	block_out[offset+2] = ecrete(aux);
	aux = tmp1_1 - p3;
	block_out[offset+5] = ecrete(aux);
	aux = tmp1_2 - p4;
	block_out[offset+3] = ecrete(aux);
	aux = tmp1_2 + p4;
	block_out[offset+4] = ecrete(aux);
	aux = tmp1_3 - p2;
	block_out[offset+1] = ecrete(aux);
	aux = tmp1_3 + p2;
	block_out[offset+6] = ecrete(aux);
      }
    }  
  }
  

