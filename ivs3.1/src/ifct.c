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
#include "ffct.h"

#define NEW_IFCT

#ifdef NO_QUANT
#define REC_OFFSET 2047
#else
#define REC_OFFSET 127
#endif


#define ecrete(x) (v_tmp = (x),v_tmp>240? 240 : ((v_tmp)>16 ? (v_tmp) : 16))
#ifdef SPARCSTATION
#define b_ecrete(x, b) (ecrete(x)<<((3-b)*8))
#else
#define b_ecrete(x, b) (ecrete(x)<<(b*8))
#endif

#define FD IFCT_FACT
#define FA (1<<(IFCTxRES))

#define MSCALE(fa, va, c) (((va)*c)/(fa))
#define TRFSCALE(f, v)  ((v)/(f))

#define OPERz(t10, t11, t20, t21, c) ((t10 || t11) \
	 && (p1 = MSCALE(FA, t11, c), (t21 = t10 - p1) | (t20 = t10 + p1)))

#define OPER(t10, t11, t20, t21, c) \
	 p1 = MSCALE(FA, t11, c); t21 = t10 - p1 ; t20 = t10 + p1

#define OPERb(t10, t11, b0, b1, c)    \
	 p1 = MSCALE(FA, t11, c);     \
         p2 = TRFSCALE(FD, t10 + p1); \
	 w0 |= b_ecrete(p2, b0);\
         p2 = TRFSCALE(FD, t10 - p1); \
         w1 |= b_ecrete(p2, b1)

#define OPERbi(t10, t11, b0, b1, c)    \
	  p1 = MSCALE(FA, t11, c);     \
          p2 = TRFSCALE(FD, t10 + p1); \
          w0 |= b_ecrete((int)x.b[b0] + p2, b0); \
          p2 = TRFSCALE(FD, t10 - p1); \
	  w1 |= b_ecrete((int)y.b[b1] + p2, b1)



typedef union{
unsigned  int word;
unsigned char b[4];
} X_TYPE;

ifct_8x8(block_fct, tab_quant1, tab_quant2, inter, block_out, colmax, L_y)
     BLOCK8x8 block_fct; /* Pointer to current 8x8 block of DCT coeff. */
     int *tab_quant1, *tab_quant2; /* Quantization */
     int inter; /* Boolean : True if INTER mode */
     u_char *block_out; /* output 8x8 block of pixels */
     int colmax; /* block_fct[colmax+1][] = 0 */
     int L_y; /* 352 if Y, 176 if Cr or Cb */
{
  
  int  tmpblk[8][8];
  {
    register int v;
    register int *ptr_rec_tab1 = tab_quant1 + REC_OFFSET;
    register int *ptr_rec_tab2 = tab_quant2 + REC_OFFSET;
    register int *ptr_fct = &block_fct[0][0];
    register int *ptr_tmpblk = &tmpblk[0][0];
    
    for(v = 0; v <= colmax; v++, ptr_fct++, ptr_tmpblk++){
      register  int tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
      register int v_tmp;
      
#define tmp2_0 tmp1_0
#define tmp2_1 tmp1_1
#define tmp2_2 tmp1_2
#define tmp2_3 tmp1_3
#define tmp2_4 tmp1_4
#define tmp2_5 tmp1_5
#define tmp2_6 tmp1_6
#define tmp2_7 tmp1_7
      
      if(v==0){
	register int p1=ptr_rec_tab1[*(ptr_fct+8*1)];
	register int p2=ptr_rec_tab1[*(ptr_fct+8*2)];
	register int p3=ptr_rec_tab1[*(ptr_fct+8*3)];

	
	if(inter)
	  tmp1_0 = 2*ptr_rec_tab2[*(ptr_fct+8*0)];
	else
	  tmp1_0 = *(ptr_fct+8*0) * FD;
	tmp1_1 = ptr_rec_tab1[*(ptr_fct+8*4)];
	tmp1_2 = 2*p2;
	tmp1_3 = p2 + ptr_rec_tab1[*(ptr_fct+8*6)];
	tmp1_4 = 2*p1;
	tmp1_5 = ptr_rec_tab1[*(ptr_fct+8*5)] + p3;
	tmp1_6 = 2*(p3 + p1);
	tmp1_7 = p1 + tmp1_5 + ptr_rec_tab1[*(ptr_fct+8*7)];
      }else{
	register int p1=ptr_rec_tab2[*(ptr_fct+8*1)];
	register int p2=ptr_rec_tab2[*(ptr_fct+8*2)];
	register int p3=ptr_rec_tab2[*(ptr_fct+8*3)];
	

	tmp1_0 = ptr_rec_tab1[*(ptr_fct+8*0)];
	tmp1_1 = ptr_rec_tab2[*(ptr_fct+8*4)];
	tmp1_2 = 2*p2;
	tmp1_3 = p2 + ptr_rec_tab2[*(ptr_fct+8*6)];
	tmp1_4 = 2*p1;
	tmp1_5 = ptr_rec_tab2[*(ptr_fct+8*5)] + p3;
	tmp1_6 = 2*(p3 + p1);
	tmp1_7 = p1 + tmp1_5 + ptr_rec_tab2[*(ptr_fct+8*7)];
      }
      {
	register int p1;
      if (
	  OPERz(tmp1_0, tmp1_1, tmp2_0, tmp2_1, Rac2) |
	  OPERz(tmp1_2, tmp1_3, tmp2_2, tmp2_3, Rac2) |
	  OPERz(tmp1_4, tmp1_5, tmp2_4, tmp2_5, Rac2) |
	  OPERz(tmp1_6, tmp1_7, tmp2_6, tmp2_7, Rac2) ){

/*
      if (tmp1_0 || tmp1_1 || tmp1_2 || tmp1_3 || 
	  tmp1_4 || tmp1_5 || tmp1_6 || tmp1_7){
	{
	  register int p1;	  
	  
	  OPERz(tmp1_0, tmp1_1, tmp2_0, tmp2_1, Rac2);
	  OPERz(tmp1_2, tmp1_3, tmp2_2, tmp2_3, Rac2);
	  OPERz(tmp1_4, tmp1_5, tmp2_4, tmp2_5, Rac2);
	  OPERz(tmp1_6, tmp1_7, tmp2_6, tmp2_7, Rac2);


	}
*/
	{
	  register int p1;
	  
	  OPER(tmp2_0, tmp2_2, tmp1_0, tmp1_2, CF2_0);
	  OPER(tmp2_1, tmp2_3, tmp1_1, tmp1_3, CF2_1);
	  OPER(tmp2_4, tmp2_6, tmp1_4, tmp1_6, CF2_0);
	  OPER(tmp2_5, tmp2_7, tmp1_5, tmp1_7, CF2_1);


	}
	{
	  register int p1;
	  
	  OPER(tmp1_0, tmp1_4, *(ptr_tmpblk+8*0), *(ptr_tmpblk+8*7),  CF1_0);
	  OPER(tmp1_3, tmp1_7, *(ptr_tmpblk+8*1), *(ptr_tmpblk+8*6), -CF1_3);
	  OPER(tmp1_1, tmp1_5, *(ptr_tmpblk+8*2), *(ptr_tmpblk+8*5),  CF1_1);
	  OPER(tmp1_2, tmp1_6, *(ptr_tmpblk+8*3), *(ptr_tmpblk+8*4), -CF1_2);

	}
      }else{
	*(ptr_tmpblk+8*0) = 0;
	*(ptr_tmpblk+8*1) = 0;
	*(ptr_tmpblk+8*2) = 0;
	*(ptr_tmpblk+8*3) = 0;
	
	*(ptr_tmpblk+8*4) = 0;
	*(ptr_tmpblk+8*5) = 0;
	*(ptr_tmpblk+8*6) = 0;
	*(ptr_tmpblk+8*7) = 0;
      }}
    } /*for(v=...*/
  }
  {
    register int v;
    register int *ptr_tmpblk;
    
    for(v=colmax+1, ptr_tmpblk=&tmpblk[0][colmax+1] ; v<8; v++, ptr_tmpblk++){

      *(ptr_tmpblk+8*0) = 0;
      *(ptr_tmpblk+8*1) = 0;
      *(ptr_tmpblk+8*2) = 0;
      *(ptr_tmpblk+8*3) = 0;
      
      *(ptr_tmpblk+8*4) = 0;
      *(ptr_tmpblk+8*5) = 0;
      *(ptr_tmpblk+8*6) = 0;
      *(ptr_tmpblk+8*7) = 0;
    }
  }
  
  /*-------------------------------------------------*/
  
  {
    register int u;
    register u_char *block_out_ptr = block_out;
    register int *ptr_tmpblk; 
    
    for(u=0, ptr_tmpblk = &tmpblk[0][0]; u<8;
	u++, ptr_tmpblk+=8, block_out_ptr+=L_y ){
      register int v_tmp;
      register  int tmp1_0, tmp1_1, tmp1_2, tmp1_3, tmp1_4, tmp1_5, tmp1_6, tmp1_7;
      
      register int p1, p2;

      tmp1_0 = *(ptr_tmpblk+0);
      tmp1_1 = *(ptr_tmpblk+4);

      p1 = *(ptr_tmpblk+2);
      tmp1_2 = 2*p1;
      tmp1_3 = p1 + *(ptr_tmpblk+6);

      p1 = *(ptr_tmpblk+1);
      tmp1_7 = *(ptr_tmpblk+3);
      tmp1_4 = 2*p1;
      tmp1_6 = 2*(tmp1_7 + p1);
      tmp1_5 = *(ptr_tmpblk+5) + tmp1_7;
      tmp1_7 = p1 + tmp1_5 + *(ptr_tmpblk+7);
      
      OPER(tmp1_0, tmp1_1, tmp2_0, tmp2_1, Rac2);
      OPER(tmp1_2, tmp1_3, tmp2_2, tmp2_3, Rac2);
      OPER(tmp1_4, tmp1_5, tmp2_4, tmp2_5, Rac2);
      OPER(tmp1_6, tmp1_7, tmp2_6, tmp2_7, Rac2);
      
      OPER(tmp2_0, tmp2_2, tmp1_0, tmp1_2, CF2_0);
      OPER(tmp2_1, tmp2_3, tmp1_1, tmp1_3, CF2_1);
      OPER(tmp2_4, tmp2_6, tmp1_4, tmp1_6, CF2_0);
      OPER(tmp2_5, tmp2_7, tmp1_5, tmp1_7, CF2_1);

      if(inter){
	
	/************\
	 * INTER MODE *
	 \************/
	register X_TYPE x,y;
	register int w0 = 0;
	register int w1 = 0;

	x.word = *(unsigned int *)(block_out_ptr+0);
	y.word = *(unsigned int *)(block_out_ptr+4);

	OPERbi(tmp1_0, tmp1_4, 0, 3,  CF1_0);
	OPERbi(tmp1_3, tmp1_7, 1, 2, -CF1_3);
	OPERbi(tmp1_1, tmp1_5, 2, 1,  CF1_1);
	OPERbi(tmp1_2, tmp1_6, 3, 0, -CF1_2);

	*(unsigned int *)(block_out_ptr+0) = w0;
	*(unsigned int *)(block_out_ptr+4) = w1;

      }else{
	
	/************\
	 * INTRA MODE *
	 \************/
	register int w0 = 0;
	register int w1 = 0;


	OPERb(tmp1_0, tmp1_4, 0, 3,  CF1_0);
	OPERb(tmp1_3, tmp1_7, 1, 2, -CF1_3);
	OPERb(tmp1_1, tmp1_5, 2, 1,  CF1_1);
	OPERb(tmp1_2, tmp1_6, 3, 0, -CF1_2);

	*(unsigned int *)(block_out_ptr+0) = w0;
	*(unsigned int *)(block_out_ptr+4) = w1;

      }
    } 
  }
}
