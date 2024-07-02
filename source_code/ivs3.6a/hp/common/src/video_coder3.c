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
*  File              : video_coder3.c                                      *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  Some H.261 encoding functions:                           *
*                   - Quantize()                                           *
*                   - ImageChangeDetect()                                  *
*                   - CodeInterMode()                                      *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
\**************************************************************************/

#include <memory.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "idx_huffman_coder.h"
#include "video_coder.h"



int Quantize(mb, max_coeff, p_bnul, p_mbnul, quantizer, FULL_INTRA, BperMB,
	     GobEncoded, NG, default_quantizer)
     MACRO_BLOCK *mb;
     int max_coeff;
     int *p_bnul;
     int *p_mbnul;
     int quantizer;
     int FULL_INTRA;
     int BperMB;
     BOOLEAN GobEncoded[];
     int NG;
     int default_quantizer;
{
  register int n, l, c;
  register int bnul  = *p_bnul;
  register int mbnul = *p_mbnul;
  register int quant_mb;
  int cc;
  
#ifdef QUANT_FIX
  quant_mb = default_quantizer;
#else
  quant_mb = (max_coeff >> 8) + 1;
  if(quantizer > quant_mb){
    quant_mb = (FULL_INTRA ? 5 : quantizer); 
  }
#endif
  mb->QUANT = quant_mb;
  /*
   *  We have to quantize mb->P[n].
   */
  bnul=0;
  if(quant_mb & 0x01){
    /* odd quantizer */
    for(n=0; n<BperMB; n++){
      register int *pt_P = &(mb->P[n][0][0]);
      register int nb_coeff=0;
      
      if((mb->CBC) & MASK_CBC[n]){
	cc = *pt_P;
	for(l=0; l<8; l++)
	  for(c=0; c<8; c++){
	    if(*pt_P > 0){
	      if((*pt_P=(*pt_P / quant_mb - 1) / 2)){
		nb_coeff ++;
		if(*pt_P > 127)
		  *pt_P = 127;
	      }
	    }else{
	      if(*pt_P < 0)
		if((*pt_P=(*pt_P /quant_mb + 1) / 2)){
		  nb_coeff ++;
		  if(*pt_P < (-127))
		    *pt_P = -127;
		}
	    }
	    pt_P ++;
	  }
	if(mb->MODE == INTRA){
	  if((mb->P[n][0][0]==0) && cc)
	    nb_coeff ++;
	  mb->P[n][0][0] = cc;
	}
	if(!nb_coeff){
	  bnul ++;
	  (mb->CBC) &= (~MASK_CBC[n]);
	}
      }else
	bnul ++;
      mb->nb_coeff[n] = nb_coeff;
    }
  }else{
    /* even quantizer */
    for(n=0; n<BperMB; n++){
      register int *pt_P = &(mb->P[n][0][0]);
      register int nb_coeff=0;
      
      if((mb->CBC) & MASK_CBC[n]){
	cc = *pt_P;
	for(l=0; l<8; l++)
	  for(c=0; c<8; c++){
	    if(*pt_P > 0){
	      if((*pt_P=((*pt_P + 1)/quant_mb - 1)/2 )){
		nb_coeff ++;
		if(*pt_P > 127)
		  *pt_P = 127;
	      }
	    }else{
	      if(*pt_P < 0)
		if((*pt_P=((*pt_P - 1)/quant_mb + 1 /* bug fixed */)/2 )){ 
		  nb_coeff ++;
		  if(*pt_P < (-127))
		    *pt_P = -127;
		}
	    }
	    pt_P ++;
	  }
	if (mb->MODE == INTRA) {
	  if((!mb->P[n][0][0]) && cc)
	    nb_coeff ++;
	  mb->P[n][0][0] = cc;
	}
	if(! nb_coeff){
	  bnul ++;
	  (mb->CBC) &= (~MASK_CBC[n]);
	}
      }else
	bnul ++;
      mb->nb_coeff[n] = nb_coeff;
    }		      
  }
  if(bnul == BperMB){
    mbnul ++;
    if(mbnul == 33){
      GobEncoded[NG-1] = FALSE;
    }
  }
  *p_bnul =  bnul;
  *p_mbnul = mbnul;
  return(quant_mb);
}




int ImageChangeDetect(new_Y, new_Cb, new_Cr, image_coeff, Acif_number,
			NG_MAX, GobEncoded, COLOR, FULL_INTRA,
			max_inter, max_value, threshold, BperMB,
			image_type, NACK_FIR_ALLOWED)
     u_char *new_Y;
     u_char *new_Cb;
     u_char *new_Cr;
     IMAGE_CIF image_coeff;
     int Acif_number;      /* select the CIF number in the CIF4 (0,1,2 or 3) */
     int NG_MAX;
     BOOLEAN GobEncoded[];
     BOOLEAN COLOR;    /* True if color encoded */
     BOOLEAN FULL_INTRA;
     int max_inter; 
     int max_value; /* A block is forced every 30 images */
     int threshold; /* For image change estimation */
     int BperMB;
     int image_type;
     BOOLEAN NACK_FIR_ALLOWED;    /* True if feedback is allowed */
{

    int k;
    int NG, MBA, INC_NG, CIF;
    GOB *gob;
    int bnul, mbnul;
    int force_value;
    MACRO_BLOCK *mb;
    int Nblock = 0;
    int lig, col;  
    int off_cif_number = 13*Acif_number;
    static int n_im=0;
    static int lig11, lig12, lig21, lig22, col11, col12, col21, col22; 


    INC_NG = 1 + (image_type == QCIF_IMAGE_TYPE);

    if(image_type == CIF4_IMAGE_TYPE)
      memset((char *)GobEncoded, TRUE, NG_MAX);
    else
      for(k=0; k<NG_MAX; k+=INC_NG)
	GobEncoded[k]=TRUE;


    if(FULL_INTRA == FALSE){
      if(Acif_number == 0){
	lig11=LIG[n_im];
	col11=COL[n_im];
	n_im=(n_im+1)%16;
	lig12=lig11+4;
	col12=col11+4;
	lig21=lig11+8;
	lig22=lig12+8;
	col21=col11+8;
	col22=col12+8;
      }
      k=1;

      for(NG=1; NG<=NG_MAX; NG+=INC_NG){

	/* GOB LOOP */

	mbnul=0;
	switch(image_type){
	  
	case QCIF_IMAGE_TYPE:
	  gob = (GOB *)(image_coeff)[NG-k++];
	  force_value = ForceGobEncoding[NG];
	  break;

	case CIF_IMAGE_TYPE:
	  gob = (GOB *)(image_coeff)[NG-1];
	  force_value = ForceGobEncoding[NG];
	  break;

	case CIF4_IMAGE_TYPE:
	  gob = (GOB *) (image_coeff)[NG-1];
	  force_value = (NACK_FIR_ALLOWED ?
			 ForceGobEncoding[off_cif_number+NG] : 0);
	  break;
	}

	for(MBA=1; MBA<34; MBA++){
	  register int sum;
	  int lig_11, lig_12, lig_21, lig_22, col_11, col_12, col_21, col_22;

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];
	  lig_11=lig+lig11;
	  lig_12=lig+lig12;
	  lig_21=lig+lig21;
	  lig_22=lig+lig22;
	  col_11=col+col11;
	  col_12=col+col12;
	  col_21=col+col21;
	  col_22=col+col22;

	  bnul=0;
	  mb = &((*gob)[MBA-1]);

	  /* We choose MB INTRA encoding when a MB has been lost
	  * or more than max_inter INTER encoding have been done or
	  * every max_value images (in order to prevent loss packets).
	  */

	  if((mb->last_encoding > force_value) &&
	     (mb->last_encoding < max_value) && (mb->cpt_inter < max_inter)){
	    mb->CBC = 0;
	    mb->MODE = INTER;
	    /* B movement detection */

	    sum =  abs((int)*(new_Y+COL_MAX*lig_11+col_11) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_11+
			      col_11));
	    sum += abs((int)*(new_Y+COL_MAX*lig_11+col_12) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_11+
			      col_12));
	    sum += abs((int)*(new_Y+COL_MAX*lig_12+col_11) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_12+
			      col_11));
	    sum += abs((int)*(new_Y+COL_MAX*lig_12+col_12) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_12+
			      col_12));
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 32;
	    }else
	      bnul ++;
	  
	    sum =  abs((int)*(new_Y+COL_MAX*lig_11+col_21) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_11+
			      col_21));
	    sum += abs((int)*(new_Y+COL_MAX*lig_11+col_22) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_11+
			      col_22));
	    sum += abs((int)*(new_Y+COL_MAX*lig_12+col_21) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_12+
			      col_21));
	    sum += abs((int)*(new_Y+COL_MAX*lig_12+col_22) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_12+
			      col_22));
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 16;
	    }else
	      bnul ++;

	    sum =  abs((int)*(new_Y+COL_MAX*lig_21+col_11) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_21+
			      col_11));
	    sum += abs((int)*(new_Y+COL_MAX*lig_21+col_12) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_21+
			      col_12));
	    sum += abs((int)*(new_Y+COL_MAX*lig_22+col_11) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_22+
			      col_11));
	    sum += abs((int)*(new_Y+COL_MAX*lig_22+col_12) - 
		       (int)*(old_Y+Acif_number*cif_size+COL_MAX*lig_22+
			      col_12));
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 8;
	    }else
	      bnul ++;

	    sum =  abs((int)*(new_Y+COL_MAX*lig_21+col_21) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_21+
			      col_21));
	    sum += abs((int)*(new_Y+COL_MAX*lig_21+col_22) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_21+
			      col_22));
	    sum += abs((int)*(new_Y+COL_MAX*lig_22+col_21) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_22+
			      col_21));
	    sum += abs((int)*(new_Y+COL_MAX*lig_22+col_22) - 
		       (int)*(old_Y+cif_size*Acif_number+COL_MAX*lig_22+
			      col_22));
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 4;
	    }else
	      bnul ++;

	    if(COLOR){
	      if(mb->CBC){
		Nblock += 2;
		mb->CBC |= 3;
	      }else
		bnul += 2;
	    }

	    if(bnul == BperMB){
	      mbnul ++;
	      (mb->last_encoding)++;
	      if(mbnul == 33){
		GobEncoded[NG-1] = FALSE;
	      }
	    }else{
	      mb->last_encoding = 1;
	      (mb->cpt_inter) ++;
	    }
	  }else{
	    /* INTRA MB encoding */
	    mb->MODE = INTRA;
	    Nblock += BperMB;
	  }
	} /* MACROBLOCK LOOP */
      } /* GOB LOOP */
    } /* if(PARTIAL) */
    return Nblock;
}



#define offsetY (COL_MAX - 8)
#define offsetC (COL_MAX_DIV2 - 8)    



int CodeInterMode(new_Y, new_Cb, new_Cr, mb, cmax, lig, col, Acif_number,
		    COLOR)
     u_char *new_Y;
     u_char *new_Cb;
     u_char *new_Cr;
     MACRO_BLOCK *mb;
     int cmax[];
     int lig;
     int col;
     int Acif_number;  /* select the CIF number in the CIF4 (0,1,2 or 3) */
     BOOLEAN COLOR;    /* True if color encoded */
{

  int max;
  int max_coeff;
  int ligdiv2;
  int coldiv2;
  int b0, b1, b2, b3;

#define DIFF8(pdst, psrc1, psrc2, offset) \
  *(pdst+0) = (int)*(psrc1+0) - (int)*(psrc2+0); \
  *(pdst+1) = (int)*(psrc1+1) - (int)*(psrc2+1); \
  *(pdst+2) = (int)*(psrc1+2) - (int)*(psrc2+2); \
  *(pdst+3) = (int)*(psrc1+3) - (int)*(psrc2+3); \
  *(pdst+4) = (int)*(psrc1+4) - (int)*(psrc2+4); \
  *(pdst+5) = (int)*(psrc1+5) - (int)*(psrc2+5); \
  *(pdst+6) = (int)*(psrc1+6) - (int)*(psrc2+6); \
  *(pdst+7) = (int)*(psrc1+7) - (int)*(psrc2+7); \
    pdst  += offset; \
    psrc1 += offset; \
    psrc2 += offset;


  coldiv2 = col >> 1;
  ligdiv2 = lig >> 1;
  b0 = lig*COL_MAX+col;
  b1 = b0 + 8;
  b2 = b0 + 8*COL_MAX;
  b3 = b2 + 8;

  max_coeff=0;

  if((mb->CBC) & 0x20){
    register int l;
    register int *pt_image = (diff_Y+COL_MAX*lig+col);
    register u_char *pt_new = (new_Y+COL_MAX*lig+col);
    register u_char *pt_old = (old_Y+cif_size*Acif_number+COL_MAX*lig+col);

    l=8;
    do{
      DIFF8(pt_image, pt_new, pt_old, COL_MAX);
    }while(--l); 

    max=ffct_8x8((diff_Y+COL_MAX*lig+col), mb->P[0], COL_MAX, TRUE, &cmax[0]);
    if(max>max_coeff)
      max_coeff=max; 
  }

  if((mb->CBC) & 0x10){
    register int l;
    register int *pt_image = (diff_Y+COL_MAX*lig+col+8);
    register u_char *pt_new = (new_Y+COL_MAX*lig+col+8);
    register u_char *pt_old = (old_Y+cif_size*Acif_number+COL_MAX*lig+col+8);
    
    l=8;
    do{
      DIFF8(pt_image, pt_new, pt_old, COL_MAX);
    }while(--l);
 
    max=ffct_8x8((diff_Y+COL_MAX*lig+col+8), mb->P[1], COL_MAX, TRUE, 
		 &cmax[1]);
    if(max>max_coeff)
      max_coeff=max;
  }

  if((mb->CBC) & 0x08){
    register int l;
    register int *pt_image = (diff_Y+COL_MAX*(lig+8)+col);
    register u_char *pt_new = (new_Y+COL_MAX*(lig+8)+col);
    register u_char *pt_old = (old_Y+cif_size*Acif_number+COL_MAX*(lig+8)+col);

    l=8;
    do{
      DIFF8(pt_image, pt_new, pt_old, COL_MAX);
    }while(--l);

    max=ffct_8x8((diff_Y+COL_MAX*(lig+8)+col), mb->P[2], COL_MAX, TRUE, 
		 &cmax[2]);
    if(max>max_coeff)
      max_coeff=max;
  }

  if((mb->CBC) & 0x04){
    register int l;
    register int *pt_image = (diff_Y+COL_MAX*(lig+8)+col+8);
    register u_char *pt_new = (new_Y+COL_MAX*(lig+8)+col+8);
    register u_char *pt_old = (old_Y+cif_size*Acif_number+COL_MAX*(lig+8)+
			       col+8);

    l=8;
    do{
      DIFF8(pt_image, pt_new, pt_old, COL_MAX);
    }while(--l);

    max=ffct_8x8((diff_Y+COL_MAX*(lig+8)+col+8), mb->P[3], COL_MAX, TRUE, 
		&cmax[3]);
    if(max>max_coeff)
      max_coeff=max;
  }

  if(COLOR){
    if((mb->CBC) & 0x02){
      register int l;
      register int *pt_image = (diff_Cb+COL_MAX_DIV2*ligdiv2+coldiv2);
      register u_char *pt_new = (new_Cb+COL_MAX_DIV2*ligdiv2+coldiv2);
      register u_char *pt_old = (old_Cb+qcif_size*Acif_number+
				 COL_MAX_DIV2*ligdiv2+coldiv2);

    l=8;
    do{
      DIFF8(pt_image, pt_new, pt_old, COL_MAX_DIV2);
    }while(--l);

      max=chroma_ffct_8x8((diff_Cb+COL_MAX_DIV2*ligdiv2+coldiv2), mb->P[4], 
			  COL_MAX_DIV2, TRUE, &cmax[4]);
      if(max>max_coeff)
	max_coeff=max;
    } 
    if((mb->CBC) & 0x01){
      register int l;
      register int *pt_image = (diff_Cr+COL_MAX_DIV2*ligdiv2+coldiv2);
      register u_char *pt_new = (new_Cr+COL_MAX_DIV2*ligdiv2+coldiv2);
      register u_char *pt_old = (old_Cr+qcif_size*Acif_number+
				 COL_MAX_DIV2*ligdiv2+coldiv2);

      l=8;
      do{
	DIFF8(pt_image, pt_new, pt_old, COL_MAX_DIV2);
      }while(--l);

      max=chroma_ffct_8x8((diff_Cr+COL_MAX_DIV2*ligdiv2+coldiv2), mb->P[5], 
			  COL_MAX_DIV2, TRUE, &cmax[5]);
      if(max>max_coeff)
	max_coeff=max;
    }
  }
  return(max_coeff);
}

