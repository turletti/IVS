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
* 	                						   *
*  File              : ivs.c       	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Perform a fast DCT on a 8x8 block. ffct_4x4() only  *
*                      keeps 4x4 coefficients. Returns maximum coefficient *
*                      value and the cmax column such as                   *
*                      block_out[][cmax+1]==0.                             *
*                      ffct_intra_XxX() procedure are necessary to process *
*                      intra MB, because input image is in unsigned char   *
*                      format. The algorithm is refered to "A new          *
*                      Two-Dimensional Fast Cosine Transform Algorithm".   *
*                      IEEE Trans. on signal proc. vol 39 no. 2 (S.C. Chan *
*                      & K.L. Ho).                                         *

*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
* Andrzej Wozniak       |  93/12/1  | Rewrite the code using macros.       *
*                                                                          *
\**************************************************************************/

#include "h261.h"
#include "ffct.h"
#include "general_types.h"

#define Nb_coeff_max 10
#define COL_MAX 352

#define CODE_CC(x) (tmp_v = (x), tmp_v > 254 ? 254 : tmp_v)

#define FA  (FFCTxSCALE) 
#define FA1 (FFCTxRES)
#define FB  (1)
#define FC  (FFCTxRES)
#define FC1 (FFCTxSCALE)

#define FD  (FFCTxRES)
#define FD1 (FFCTxRES)
#define FE  (FFCTxRES+FFCT_FIX)
#define FE1 (FFCTxRES+FFCT_FIX)
#define FF  (FFCTxFIX)
#define FG  (FFCTxRES)

#define FU (FFCTxFIX)

#define TSYM(f,v)    ((v<0)?-(1<<(f-1)):1<<(f-1))
#define TASYM(f,v)    ((1<<(f-1)))
#define TNEG(f,v)    ((v<0)?(1<<(f-1)):0)

#define SSHIFT(f,v)  ((v)>>(f)) 
#define SDIV(f,v)    ((v)/(1<<(f)))

#define USCALE(f, v)       ((v)<<(f))
#define DSCALE(f, v)       SSHIFT(f, v)
#define MSCALE(f, v, c)    ((tmp_v = (v))?SDIV(f, tmp_v*c):0)
#define TRDSCALE(f, v)     ((tmp_v = (v))?SDIV(f, tmp_v):0)


#define EX_TYPE int

#define tmp2_1 tmp2_0
#define tmp2_3 tmp2_2
#define tmp2_5 tmp2_4
#define tmp2_7 tmp2_6

#define tmp1_1 tmp2_0
#define tmp1_3 tmp2_2
#define tmp1_5 tmp2_4
#define tmp1_7 tmp2_6

#define D01 (tmp1_0 - tmp1_1)
#define D23 (tmp1_2 - tmp1_3)
#define D45 (tmp1_4 - tmp1_5)
#define D67 (tmp1_6 - tmp1_7)

#define S01 (tmp1_0 + tmp1_1)
#define S23 (tmp1_2 + tmp1_3)
#define S45 (tmp1_4 + tmp1_5)
#define S67 (tmp1_6 + tmp1_7)


#ifndef FFCT_CHROMA
#define FFCT_8x8 ffct_8x8
#define FFCT_INTRA_8x8 ffct_intra_8x8
#define ARRAY_COL_MAX (COL_MAX)
#else
#define FFCT_8x8 chroma_ffct_8x8
#define FFCT_INTRA_8x8 chroma_ffct_intra_8x8
#define ARRAY_COL_MAX (COL_MAX>>1)
#endif


#ifndef FFCT_INTRA
int FFCT_8x8(vimage_in, block_out, L_col, inter, cmax)
     int *vimage_in;      /* may be negative       */
     BLOCK8x8 block_out; /* output coefficients   */
     int L_col;          /* image_in column max   */
     BOOLEAN inter;      /* True if INTER mode    */
     int *cmax;          /* block_out[][cmax+1]=0 */
#define FFCT_CAST *(int *)
#define COL_STEP (ARRAY_COL_MAX*4)
#else
     int FFCT_INTRA_8x8(vimage_in, block_out, L_col, cmax)
     u_char *vimage_in; /* always positive */
     BLOCK8x8 block_out;
     int L_col; /* image_in column max */
     int *cmax; /* block_out[][cmax+1]=0 */
#define FFCT_CAST (int)*
#define COL_STEP (ARRAY_COL_MAX)
#endif
{
  EX_TYPE tmpblk[8][8];
  {
    register int v=8;
    register EX_TYPE *tmpblk_ptr;
    register u_char *image_in;
    image_in = (u_char *)vimage_in;
    
    tmpblk_ptr = &tmpblk[0][0];
    while(v--){
      register int p1, p2;
      register EX_TYPE tmp1_0, tmp1_2, tmp1_4, tmp1_6;
      register EX_TYPE tmp2_0, tmp2_2, tmp2_4, tmp2_6;
      register int tmp_v;
      
      p1 = FFCT_CAST(image_in+(COL_STEP*0));
      p2 = FFCT_CAST(image_in+(COL_STEP*7));
      
      tmp2_0 = (p1 + p2);
      tmp2_4 = MSCALE(FA,(p1 - p2) , cf1_0);
      
      p1 = FFCT_CAST(image_in+(COL_STEP*4));
      p2 = FFCT_CAST(image_in+(COL_STEP*3));
      tmp2_2 = (p1 + p2);
      tmp2_6 = MSCALE(FA,(p1 - p2) , cf1_2);
      
      
      tmp1_0 = (tmp2_0 + tmp2_2);
      tmp1_2 = MSCALE(FA,(tmp2_0 - tmp2_2) , cf2_0);
      tmp1_4 = (tmp2_4 + tmp2_6);
      tmp1_6 = MSCALE(FA1,(tmp2_4 - tmp2_6) , cf2_0);
      
      p1 = FFCT_CAST(image_in+(COL_STEP*2));
      p2 = FFCT_CAST(image_in+(COL_STEP*5));
      tmp2_1 = (p1 + p2);
      tmp2_5 = MSCALE(FA,(p1 - p2) , cf1_1);
      
      p1 = FFCT_CAST(image_in+COL_STEP*6);
      p2 = FFCT_CAST(image_in+COL_STEP*1);
      tmp2_3 = (p1 + p2);
      tmp2_7 = MSCALE(FA,(p1 - p2) , cf1_3);
      
      tmp1_1 = ((p1=tmp2_1) + tmp2_3);
      tmp1_3 = MSCALE(FA,(p1 - tmp2_3) , cf2_1);
      tmp1_5 = ((p1=tmp2_5) + tmp2_7);
      tmp1_7 = MSCALE(FA1,(p1 - tmp2_7) , cf2_1);
      
      
      *tmpblk_ptr      = USCALE(FU, S01);
      *(tmpblk_ptr+8)  = p1 = DSCALE(FB, p2=S45);
      *(tmpblk_ptr+56) = MSCALE(FC,(D67 - D45) , UnSurRac2) - p1;
      *(tmpblk_ptr+24) = p1 = DSCALE(FB,(S67 - p2));
      *(tmpblk_ptr+40) = MSCALE(FC, D45 , UnSurRac2) - p1;
      *(tmpblk_ptr+16) = p1 = DSCALE(FB, S23);
      *(tmpblk_ptr+48) = MSCALE(FC, D23, UnSurRac2) - p1;
      *(tmpblk_ptr+32) = MSCALE(FC1, D01, UnSurRac2);
      
#ifndef FFCT_INTRA
      image_in+=4;
#else
      image_in++;
#endif
      tmpblk_ptr++;
      
    }
  }
  {
    register int u=8;
    register EX_TYPE *pt_tmpblk;
    register int *block_ptr;   
    block_ptr = &block_out[0][0];
    pt_tmpblk = &(tmpblk[0][0]);
    
    for(u=0; u<8; u++){
      register EX_TYPE tmp1_0, tmp1_2, tmp1_4, tmp1_6;
      register EX_TYPE tmp2_0, tmp2_2, tmp2_4, tmp2_6;
      register EX_TYPE p1, p2;
      register int tmp_v;
      
      p1 = *pt_tmpblk;
      p2 = *(pt_tmpblk+7);
      tmp2_0 = p1 + p2;
      tmp2_4 = MSCALE(FD, (p1 - p2), cf1_0);
      
      p1 = *(pt_tmpblk+4);
      p2 = *(pt_tmpblk+3);
      tmp2_2 = p1 + p2;
      tmp2_6 = MSCALE(FD, (p1 - p2), cf1_2);
      
      tmp1_0 = tmp2_0 + tmp2_2;
      tmp1_2 = MSCALE(FD, (tmp2_0 - tmp2_2) , cf2_0);
      
      tmp1_4 = tmp2_4 + tmp2_6;
      tmp1_6 = MSCALE(FD1,(tmp2_4 - tmp2_6) , cf2_0);
      
      p1 = *(pt_tmpblk+2);
      p2 = *(pt_tmpblk+5);
      tmp2_1 = p1 + p2;
      tmp2_5 = MSCALE(FD, (p1 - p2), cf1_1);
      
      p1 = *(pt_tmpblk+6);
      p2 = *(pt_tmpblk+1);
      tmp2_3 = p1 + p2;	
      tmp2_7 = MSCALE(FD, (p1 - p2), cf1_3);
      
      tmp1_1 = (p1=tmp2_1) + tmp2_3;
      tmp1_3 = MSCALE(FD, (p1 - tmp2_3 ), cf2_1);
      tmp1_5 = (p1=tmp2_5) + tmp2_7;
      tmp1_7 = MSCALE(FD1, (p1 - tmp2_7), cf2_1);
      
      if(u==0){
#ifndef FFCT_INTRA
	if(inter)
	  *block_ptr   = (int)(TRDSCALE(FU+3, S01));
	else
#endif
	  {
	    p1 = TRDSCALE(FU+6, S01);
	    *block_ptr     = (int)(CODE_CC(p1));
	  }
	p1 = MSCALE(FG, (p2=S45), UnSurRac2);
	*(block_ptr+1) = (int)(TRDSCALE(FF+3, p1));
	*(block_ptr+7) = (int)(TRDSCALE(FF+3, (D67 - D45) - p1));
	*(block_ptr+4) = (int)(TRDSCALE(FU+3, D01));
	p1 = MSCALE(FG, (S67 - p2), UnSurRac2);
	*(block_ptr+3) = (int)(TRDSCALE(FF+3, p1));
	*(block_ptr+5) = (int)(TRDSCALE(FF+3, D45 - p1));
	p1 = MSCALE(FG, S23, UnSurRac2);
	*(block_ptr+2) = (int)(TRDSCALE(FF+3, p1));
	*(block_ptr+6) = (int)(TRDSCALE(FF+3, D23 - p1));
      }else{
	*block_ptr     = (int)(TRDSCALE(FE1+2, UnSurRac2*S01));
	*(block_ptr+4) = (int)(TRDSCALE(FE1+2, D01*UnSurRac2));
	*(block_ptr+3) = (int)(TRDSCALE(FF+3, p1 = (S67 - S45)));
	p2 = MSCALE(FG-1, D45, UnSurRac2);
	*(block_ptr+5) = (int)(TRDSCALE(FF+3, p2 - p1));
	
	*(block_ptr+1) = (int)(TRDSCALE(FF+3, p1 = S45));
	p2 = MSCALE(FG-1, (D67 - D45), UnSurRac2);
	*(block_ptr+7) = (int)(TRDSCALE(FF+3, p2 - p1));
	
	*(block_ptr+2) = (int)(TRDSCALE(FF+3, p1 = S23));
	p2 = MSCALE(FG-1, D23, UnSurRac2);
	*(block_ptr+6) = (int)(TRDSCALE(FF+3, p2 - p1));
      }
      block_ptr += 8;
      pt_tmpblk += 8;
    }
  }
  
 {  /* For test only */

      register int i,j;
      static int FIRST=1;
      static u_char MASK[64] ={
	0,1,5,6,14,15,27,28,
	2,4,7,13,16,26,29,42,
	3,8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
      };
      static u_char NMASK[64];
      int cpt=0;
      char last=block_out[0][0];

      if(FIRST) {
	memset(NMASK,0,64);
	for(i=0; i<64; i++)
	  if(MASK[i] % 3 == 1)
	    NMASK[i] = 1;
	FIRST=0;
      }
      NMASK[0] = 1; /* DC COEFF ALWAYS TRANSMITTED */
      for(i=0; i<8; i++)
	for(j=0; j<8; j++, cpt++) {
	  if(NMASK[cpt] == 0)
	    block_out[i][j] = 0/*last*/;
	  else
	    last = block_out[i][j];
	}
 }


  /* Now, we have to find coeff max and cmax value (block_out[][cmax+1]=0) */
  {
    register int v;
    register int *block_ptr;
    
    block_ptr = &block_out[0][7];
    for(v=7; v>=0; v--, block_ptr--){
      if(*(block_ptr+8*0) ||
	 *(block_ptr+8*1) ||
	 *(block_ptr+8*2) ||
	 *(block_ptr+8*3) ||
	 *(block_ptr+8*4) ||
	 *(block_ptr+8*5) ||
	 *(block_ptr+8*6) ||
	 *(block_ptr+8*7)
	 )
	break;
    }

#define MAX_ELEM(x) aux = x, (aux<0)?aux = -aux:aux, (aux>max)?max = aux:max
    {
      register int w;
      register int aux;
      register int max;
      
      block_ptr = &block_out[0][0];
      aux = max = 0;
      for(w=0; w<=v; w++, block_ptr++){
	MAX_ELEM(*(block_ptr+8*0)),
	MAX_ELEM(*(block_ptr+8*1)),
	MAX_ELEM(*(block_ptr+8*2)),
	MAX_ELEM(*(block_ptr+8*3)),
	MAX_ELEM(*(block_ptr+8*4)),
	MAX_ELEM(*(block_ptr+8*5)),
	MAX_ELEM(*(block_ptr+8*6)),
	MAX_ELEM(*(block_ptr+8*7));
      }
      *cmax=v;
      return(max);
    }
  }
}
