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
*                                                                          *
*  File    : adpcm.h                                                       *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Huffman state array used by variable rate adpcm codec.   *
*                                                                          *
\**************************************************************************/

#define get_bit(octet,i,j) \
  (j==7 ? (j=0, octet[(i)++]&0x01) : (octet[(i)]>>(7-(j)++))&0x01)

#define put_bit(octet, bit, i, j) \
  (j==7 ? (octet[(i)++]+=bit, octet[(i)]=0, j=0): \
   (octet[i]+=(bit<<(7-(j)++))))


#define n_adpcm 16

typedef struct {
   int bit[2];
   int result[2];
 } etat;

typedef struct {
   int result;
   char chain[n_adpcm];
 } mot_code;


static etat state_adpcm[] = {
	{{1,3},{-99,-99}}, {{0,2},{0,-99}}, {{0,0},{1,9}}, 
	{{0,4},{8,-99}}, {{5,6},{-99,-99}}, {{0,0},{2,10}}, 
	{{7,8},{-99,-99}}, {{0,0},{3,11}}, {{9,10},{-99,-99}}, 
	{{0,0},{4,12}}, {{11,12},{-99,-99}}, {{0,0},{5,13}}, 
	{{13,14},{-99,-99}}, {{0,0},{6,14}}, {{0,0},{7,15}}
};


static char *adpcm_chain[16] = {
  "00", "010", "1100", "11100", "111100", "1111100", 
  "11111100", "11111110", "10", "011", "1101", "11101",
  "111101", "1111101", "11111101", "11111111"
  };


