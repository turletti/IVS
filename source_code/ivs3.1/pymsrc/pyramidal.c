#include <stdio.h>
#include "pyramidal.h"


#define u_char unsigned char
#define BOOLEAN unsigned char
#define TRUE  1
#define FALSE 0
#define abs(a) ((a) < 0 ? -(a) : (a))

#define put_ccbit(octet,bit,i,j) \
    (!j ? (octet[(i)++] |=bit, j=7) : \
     (octet[i] |=(bit<< j--)))

#define put_bit(octet, bit, i, j) \
    (!j ? (octet[(i)++] |=bit, octet[(i)]=0, j=7) : \
     (octet[i] |=(bit<< (j)--)))

#define get_bit(octet,i,j) \
    (j==7 ? (j=0, octet[(i)++]&0x01) : (octet[(i)]>>(7-(j)++))&0x01)


void put_value(chain_ptr, value, n_bits, i, j) 
     unsigned char *chain_ptr;
     int value;
     int n_bits;
     int *i, *j;
{
  while (*j <= (n_bits - 1)){
    *chain_ptr++ |= (value >> ((n_bits - 1 ) - *j));
    n_bits -= (*j + 1);
    *chain_ptr = 0;
    *j = 7;
    *i += 1;
  }
  *chain_ptr |=(value << ( *j - (n_bits - 1)));
  *j -= n_bits;
}



void put_cc(chain_ptr, value, n_bits, i, j) 
     unsigned char *chain_ptr;
     int value;
     int n_bits;
     int *i, *j;
{
  
  while (*j <= (n_bits - 1)){
    *chain_ptr++ |= (value >> ((n_bits - 1 ) - *j));
    n_bits -= (*j + 1);
    *j = 7;
    *i += 1;
  }
  *chain_ptr |=(value << ( *j - (n_bits - 1)));
  *j -= n_bits;
}


void encode_block(block, chain_out, delta, i, j, offset)
     u_char *block, *chain_out;
     int delta;
     int *i, *j;
     int offset;
{

  int *hoffset, *loffset;
  register u_char *pt1, *pt2;
  register int cc, pix11, pix12, pix21, pix22;
  int ii, jj;
  int cc2x2_11, cc2x2_12, cc2x2_21, cc2x2_22;
  int cc4x4[4];
  BOOLEAN OK_MODE_1;
  BOOLEAN OK_MODE_2;
  int firsti, firstj, k, l;
#ifdef DEBUG
  int cpt=0;
#endif

  switch(offset){
  case 352:
    hoffset = hoffset352; 
    loffset = loffset352; 
    break;
  case 176:
    hoffset = hoffset176; 
    loffset = loffset176; 
    break;
  case 8:
    hoffset = hoffset8; 
    loffset = loffset8; break;
  default:
    fprintf(stderr, "Bad offset value %d\n", offset);
  }

  OK_MODE_1 = TRUE;
  firsti = ii = *i;
  firstj = jj = *j;

  /* Encode the cc value (8 bits) + the continue bit (here unset) */
  put_value(chain_out+ii, 0, 9, &ii, &jj);
  for(k=0; k < 4; k++){

    OK_MODE_2 = TRUE;

    pt1 = block + hoffset[k];
    pt2 = pt1 + offset;

    pix11 = *pt1++;
    pix12 = *pt1++;
    pix21 = *pt2++;
    pix22 = *pt2++;
    
    cc = cc2x2_11 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if((same_value(cc, pix11, delta) && 
       same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && 
       same_value(cc, pix22, delta)) == 0)
      {
	/* Mode 4 forced */
	if(OK_MODE_1){
	  for(l=0; l < k; l++){
	    put_mode(chain_out, MODE_2, ii, jj);
	    put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	  }
	  OK_MODE_1 = FALSE;
	}
	OK_MODE_2 = FALSE;
	put_mode(chain_out, MODE_4, ii, jj);
	put_value(chain_out+ii, pix11, 8, &ii, &jj);
	put_value(chain_out+ii, pix12, 8, &ii, &jj);
	put_value(chain_out+ii, pix21, 8, &ii, &jj);
	put_value(chain_out+ii, pix22, 8, &ii, &jj);
#ifdef DEBUG
	cpt += 4;
#endif
      }
    
    pix11 = *pt1++;
    pix12 = *pt1;
    pix21 = *pt2++;
    pix22 = *pt2;

    cc = cc2x2_12 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
#ifdef DEBUG
      cpt += 4;
#endif
    }
    
    pt1 = block + loffset[k];
    pt2 = pt1 + offset;

    pix11 = *pt1++;
    pix12 = *pt1++;
    pix21 = *pt2++;
    pix22 = *pt2++;

    cc = cc2x2_21 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
#ifdef DEBUG
      cpt += 4;
#endif
    }
    
    pix11 = *pt1++;
    pix12 = *pt1;
    pix21 = *pt2++;
    pix22 = *pt2;
    
    cc = cc2x2_22 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_21, 8, &ii, &jj);
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
#ifdef DEBUG
      cpt += 4;
#endif
    }
    cc = cc4x4[k] = (cc2x2_11 + cc2x2_12 + cc2x2_21 + cc2x2_22) >> 2;

    if(OK_MODE_2){
      if(same_value(cc, cc2x2_11, delta) && 
	 same_value(cc, cc2x2_12, delta) &&
	 same_value(cc, cc2x2_21, delta) && 
	 same_value(cc, cc2x2_22, delta) == 0)
	{
	  /* Mode 3 forced */
	  OK_MODE_1 = FALSE;
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_21, 8, &ii, &jj);
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_22, 8, &ii, &jj);
	}else{
	  if(OK_MODE_1 == FALSE){
	    put_mode(chain_out, MODE_2, ii, jj);
	    put_value(chain_out+ii, cc, 8, &ii, &jj);
	  }
	}    
    }else
      OK_MODE_2 = TRUE;
  } /* for(k) */

  cc = (cc4x4[0] + cc4x4[1] + cc4x4[2] + cc4x4[3]) >> 2;
  put_cc(chain_out, cc, 8, &firsti, &firstj);
  if(OK_MODE_1 == FALSE){
    put_bit(chain_out, 1, firsti, firstj);
  }
#ifdef DEBUG
  fprintf(stderr, "nb coeff encoded: %d, cc=%d, i=%d\n", cpt, cc, ii);
#endif
  *i = ii;
  *j = jj;
}
      




void display_block(block, offset)
     u_char *block;
     int offset;
{
  register int l, c;
  register u_char *pt_block;
  int toffset=0;

  for(l=0; l<8; l++){
    pt_block = block + toffset;
    for(c=0; c<8; c++){
      printf("%3d  ", (int)*pt_block++);
    }
    toffset += offset;
    printf("\n");
  }
  printf("\n\n\n");
}

void eencode_block(block_in, block_out, chain_out, delta, i, j, offset)
     u_char *block_in, *block_out, *chain_out;
     int delta;
     int *i, *j;
     int offset;
{

  int *hoffset, *loffset, offset1, toffset, *tab_offset;
  register u_char *pt1, *pt2;
  register int l, ll, cc, pix11, pix12, pix21, pix22;
  int ii, jj;
  int cc2x2_11, cc2x2_12, cc2x2_21, cc2x2_22;
  int cc4x4[4];
  BOOLEAN OK_MODE_1;
  BOOLEAN OK_MODE_2;
  int firsti, firstj, k;
  u_char *pt_out;
  int cpt=0;

  switch(offset){
  case 352:
    hoffset = hoffset352; 
    loffset = loffset352; 
    tab_offset = tab_offset352;
    break;
  case 176:
    hoffset = hoffset176; 
    loffset = loffset176; 
    tab_offset = tab_offset176; 
    break;
  case 8:
    hoffset = hoffset8; 
    loffset = loffset8; 
    tab_offset = tab_offset8;
    break;
  default:
    fprintf(stderr, "Bad offset value %d\n", offset);
  }
  offset1 = offset + 1;

  OK_MODE_1 = TRUE;
  firsti = ii = *i;
  firstj = jj = *j;

  /* Encode the cc value (8 bits) + the continue bit (here unset) */
  put_value(chain_out+ii, 0, 9, &ii, &jj);

  pt_out = block_out;

  for(k=0; k < 4; k++){
    
    OK_MODE_2 = TRUE;
    
    pt1 = block_in + hoffset[k];
    pt2 = pt1 + offset;
    
    pix11 = *pt1++;
    pix12 = *pt1++;
    pix21 = *pt2++;
    pix22 = *pt2++;
    
    cc = cc2x2_11 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if((same_value(cc, pix11, delta) && 
	same_value(cc, pix12, delta) &&
	same_value(cc, pix21, delta) && 
	same_value(cc, pix22, delta)) == 0)
      {
	/* Mode 4 forced */
	if(OK_MODE_1){
	  for(l=0; l < k; l++){
	    put_mode(chain_out, MODE_2, ii, jj);
	    put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	    toffset = 0;
	    for(ll=0; ll<4; ll++){
	      memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
	      toffset += offset;
	    }
	    cpt += 16;
	    pt_out = block_out + tab_offset[cpt];
	  }
	  OK_MODE_1 = FALSE;
	}
	OK_MODE_2 = FALSE;
	put_mode(chain_out, MODE_4, ii, jj);
	put_value(chain_out+ii, pix11, 8, &ii, &jj);
	put_value(chain_out+ii, pix12, 8, &ii, &jj);
	put_value(chain_out+ii, pix21, 8, &ii, &jj);
	put_value(chain_out+ii, pix22, 8, &ii, &jj);
	*pt_out = pix11;
	*(pt_out+1) = pix12;
	*(pt_out+offset) = pix21;
	*(pt_out+offset1) = pix22;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
      }
    
    pix11 = *pt1++;
    pix12 = *pt1;
    pix21 = *pt2++;
    pix22 = *pt2;

    cc = cc2x2_12 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc;
	*(pt_out+offset) = *(pt_out+offset1) = cc;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	  memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
	  toffset = 0;
	  for(ll=0; ll<4; ll++){
	    memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
	    toffset += offset;
	  }
	  cpt += 16;
	  pt_out = block_out + tab_offset[cpt];
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_11;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_11;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
      *pt_out = pix11;
      *(pt_out+1) = pix12;
      *(pt_out+offset) = pix21;
      *(pt_out+offset1) = pix22;
      cpt += 4;
      pt_out = block_out + tab_offset[cpt];
    }
    
    pt1 = block_in + loffset[k];
    pt2 = pt1 + offset;
    
    pix11 = *pt1++;
    pix12 = *pt1++;
    pix21 = *pt2++;
    pix22 = *pt2++;
    
    cc = cc2x2_21 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc;
	*(pt_out+offset) = *(pt_out+offset1) = cc;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	  toffset = 0;
	  for(ll=0; ll<4; ll++){
	    memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
	    toffset += offset;
	  }
	  cpt += 16;
	  pt_out = block_out + tab_offset[cpt];
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_11;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_11;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_12;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_12;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
      *pt_out = pix11;
      *(pt_out+1) = pix12;
      *(pt_out+offset) = pix21;
      *(pt_out+offset1) = pix22;
      cpt += 4;
      pt_out = block_out + tab_offset[cpt];
    }
    
    pix11 = *pt1++;
    pix12 = *pt1;
    pix21 = *pt2++;
    pix22 = *pt2;
    
    cc = cc2x2_22 = (pix11 + pix12 + pix21 + pix22) >> 2;
    if(same_value(cc, pix11, delta) && same_value(cc, pix12, delta) &&
       same_value(cc, pix21, delta) && same_value(cc, pix22, delta)){
      if(!OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc;
	*(pt_out+offset) = *(pt_out+offset1) = cc;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
      }
    }else{
      /* Mode 4 forced */
      if(OK_MODE_1){
	for(l=0; l < k; l++){
	  put_mode(chain_out, MODE_2, ii, jj);
	  put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	  toffset = 0;
	  for(ll=0; ll<4; ll++){
	    memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
	    toffset += offset;
	  }
	  cpt += 16;
	  pt_out = block_out + tab_offset[cpt];
	}
	OK_MODE_1 = FALSE;
      }
      if(OK_MODE_2){
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_11;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_11;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_12;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_12;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	put_mode(chain_out, MODE_3, ii, jj);
	put_value(chain_out+ii, cc2x2_21, 8, &ii, &jj);
	*pt_out = *(pt_out+1) = cc2x2_21;
	*(pt_out+offset) = *(pt_out+offset1) = cc2x2_21;
	cpt += 4;
	pt_out = block_out + tab_offset[cpt];
	OK_MODE_2 = FALSE;
      }
      put_mode(chain_out, MODE_4, ii, jj);
      put_value(chain_out+ii, pix11, 8, &ii, &jj);
      put_value(chain_out+ii, pix12, 8, &ii, &jj);
      put_value(chain_out+ii, pix21, 8, &ii, &jj);
      put_value(chain_out+ii, pix22, 8, &ii, &jj);
      *pt_out = pix11;
      *(pt_out+1) = pix12;
      *(pt_out+offset) = pix21;
      *(pt_out+offset1) = pix22;
      cpt += 4;
      pt_out = block_out + tab_offset[cpt];
    }
    cc = cc4x4[k] = (cc2x2_11 + cc2x2_12 + cc2x2_21 + cc2x2_22) >> 2;

    if(OK_MODE_2){
      if(same_value(cc, cc2x2_11, delta) && 
	 same_value(cc, cc2x2_12, delta) &&
	 same_value(cc, cc2x2_21, delta) && 
	 same_value(cc, cc2x2_22, delta) == 0)
	{
	  /* Mode 3 forced */
	  if(OK_MODE_1){
	    for(l=0; l < k; l++){
	      put_mode(chain_out, MODE_2, ii, jj);
	      put_value(chain_out+ii, cc4x4[l], 8, &ii, &jj);
	      toffset = 0;
	      for(ll=0; ll<4; ll++){
		memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
		toffset += offset;
	      }
	      cpt += 16;
	      pt_out = block_out + tab_offset[cpt];
	    }
	  }
	  OK_MODE_1 = FALSE; 
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_11, 8, &ii, &jj);
	  *pt_out = *(pt_out+1) = cc2x2_11;
	  *(pt_out+offset) = *(pt_out+offset1) = cc2x2_11;
	  cpt += 4;
	  pt_out = block_out + tab_offset[cpt];
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_12, 8, &ii, &jj);
	  *pt_out = *(pt_out+1) = cc2x2_12;
	  *(pt_out+offset) = *(pt_out+offset1) = cc2x2_12;
	  cpt += 4;
	  pt_out = block_out + tab_offset[cpt];
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_21, 8, &ii, &jj);
	  *pt_out = *(pt_out+1) = cc2x2_21;
	  *(pt_out+offset) = *(pt_out+offset1) = cc2x2_21;
	  cpt += 4;
	  pt_out = block_out + tab_offset[cpt];
	  put_mode(chain_out, MODE_3, ii, jj);
	  put_value(chain_out+ii, cc2x2_22, 8, &ii, &jj);
	  *pt_out = *(pt_out+1) = cc2x2_22;
	  *(pt_out+offset) = *(pt_out+offset1) = cc2x2_22;
	  cpt += 4;
	  pt_out = block_out + tab_offset[cpt];
	}else{
	  if(OK_MODE_1 == FALSE){
	    for(l=0; l < k; l++){
	      put_mode(chain_out, MODE_2, ii, jj);
	      put_value(chain_out+ii, cc, 8, &ii, &jj);
	      toffset = 0;
	      for(ll=0; ll<4; ll++){
		memset((char *) (pt_out+toffset), (char) cc4x4[l], 4);
		toffset += offset;
	      }
	      cpt += 16;
	      pt_out = block_out + tab_offset[cpt];
	    }
	  }
	}    
    }else
      OK_MODE_2 = TRUE;
  } /* for(k) */
  
  cc = (cc4x4[0] + cc4x4[1] + cc4x4[2] + cc4x4[3]) >> 2;
  put_cc(chain_out+firsti, cc, 8, &firsti, &firstj);
  if(OK_MODE_1 == FALSE){
    put_ccbit(chain_out, 1, firsti, firstj);
  }else{
    toffset = 0;
    for(l=0; l<8; l++){
      memset((char *) (block_out + toffset), (char) cc, 8);
      toffset += offset;
    }
  }
  *i = ii;
  *j = jj;
         
}
 


int decode_block(chain, block_out, offset, i, j)
     u_char *chain;
     u_char *block_out;
     int offset;
     int *i, *j;
{
  register u_char *pt_out, temp;
  register int l, ii, jj;
  u_char mode;
  int cpt, toffset;
  int offset1=offset+1;
  static int *tab_offset;
  static int imax=100;

  ii = *i;
  jj = *j;

  switch(offset){
  case 8:
    tab_offset = tab_offset8; break;
  case 176:
    tab_offset = tab_offset176; break;
  case 352:
    tab_offset = tab_offset352; break;
  default:
    fprintf(stderr, "Bad offset value: %d\n", offset);
    return(-1);
  }

  temp = get_bit(chain, ii, jj);
  for(l=0; l<7; l++)
    temp = (temp << 1) + get_bit(chain, ii, jj);
  if(get_bit(chain, ii, jj) == 0){
    /* Only the cc component is encoded */
    toffset = 0;
    for(l=0; l<8; l++){
      memset((char *) (block_out + toffset), (char) temp, 8);
      toffset += offset;
    }
    *i = ii;
    *j = jj;
    return(0);
  }

  cpt = 0;
  pt_out = &block_out[0];

  do{
    mode = get_bit(chain, ii, jj);
    mode = (mode << 1) | get_bit(chain, ii, jj);
    switch(mode){

    case MODE_4:
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      *pt_out = temp;
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      *(pt_out+1) = temp;
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      *(pt_out+offset) = temp;
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      *(pt_out+offset1) = temp;
      cpt += 4;
      pt_out = block_out + tab_offset[cpt];
      break;

    case MODE_3:
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      *pt_out = *(pt_out+1) = temp;
      *(pt_out+offset) = *(pt_out+offset1) = temp;
      cpt += 4;
      pt_out = block_out + tab_offset[cpt];
      break;

    case MODE_2:
      temp = get_bit(chain, ii, jj);
      for(l=0; l<7 ; l++)
	temp = (temp << 1) | get_bit(chain, ii, jj);
      toffset = 0;
      for(l=0; l<4; l++){
	memset((char *) (pt_out+toffset), (char) temp, 4);
	toffset += offset;
      }
      cpt += 16;
      pt_out = block_out + tab_offset[cpt];
      break;

    default:
      fprintf(stderr, "Bad mode encoded: %d, cpt =%d, i=%d\n", mode, cpt, ii);
      return(-1);
    }
  }while((cpt < 64) && (ii<imax));

  *i = ii;
  *j = jj;
  return(0);
}





main(argc, argv)
     int argc;
     char **argv;
{
  static u_char block_in[8][8] = {
    {23, 24, 26, 25, 100, 32, 45, 40},
    {24, 23, 26, 25, 90, 40, 45, 23},
    {23, 24, 26, 25, 100, 32, 45, 40},
    {24, 23, 26, 25, 90, 40, 45, 23},
    {12, 12, 34, 35, 200, 10, 200, 10},   
    {12, 12, 34, 35, 200, 10, 200, 10},
    {45, 11, 34, 100, 10, 23,  56, 44},
    {12, 45, 240, 250, 34, 43, 22, 100}
  };
  int delta;
  u_char chain_out[8000];
  u_char block_out[64];
  int i=0, j=7;

  if(argc != 2){
    fprintf(stderr, "Usage %s delta\n");
    exit(1);
  }

  delta = atoi(argv[1]);
  eencode_block(&block_in[0][0], block_out, chain_out, delta, &i, &j, 8);
  printf("Before encoding :\n");
  display_block(&block_in[0][0], 8);
  printf("After encoding :\n");
  display_block(block_out, 8);
  printf("After encoding (&decoding):\n");

  i=0; j=0;
  if(decode_block(chain_out, block_out, 8, &i, &j) < 0)
    fprintf(stderr, "error detected\n");
  display_block(block_out, 8);

  printf("compression %d/%d: %3.3g\n", 8*i, 512, (float)512/(8*i));
}
