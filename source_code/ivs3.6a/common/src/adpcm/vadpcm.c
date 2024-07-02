/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
**
** Change log:
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4.
** - There was an off-by-one error causing it to pick
**   an incorrect delta once in a blue moon.
** - The NODIVMUL define has been removed. Computations are now always done
**   using shifts, adds and subtracts. It turned out that, because the standard
**   is defined using shift/add/subtract, you needed bits of fixup code
**   (because the div/mul simulation using shift/add/sub made some rounding
**   errors that real div/mul don't make) and all together the resultant code
**   ran slower than just using the shifts all the time.
** - Changed some of the variable names to be more meaningful.
*/
/**************************************************************************\
* File        : vadpcm.c                                                   *
* Author      : Jack Jansen                                                *
* Modified by : Thierry Turletti                                           *
* Date        : 1994/2/24                                                  *
*--------------------------------------------------------------------------*
* Modifications : - ulaw to linear and linear to ulaw addition, (12/8 bits)*
*                 - Huffman tables are used to encode audio ADPCM samples. *
*                 - AVG: send the header with valpred & index.             *
\**************************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../ulaw2linear.h"
#include "adpcm_n.h"

#define BOOLEAN     unsigned char
#ifndef TRUE
#define TRUE                    1
#endif /* TRUE */
#ifndef FALSE                   
#define FALSE                   0
#endif /* FALSE */


#define get_bit(octet,i,j) \
  (j==7 ? (j=0, ((u_char*)octet)[(i)++]&0x01) : (((u_char*)octet)[(i)]>>(7-(j)++))&0x01)

#define put_bit(octet, bit, i, j) \
  (j==7 ? (((u_char*)octet)[(i)++]+=bit, ((u_char*)octet)[(i)]=0, j=0): \
   (((u_char*)octet)[i]+=(bit<<(7-(j)++))))


#define n_adpcm 16

static dvi_stat stat={0,0};

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

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

int vadpcm_coder(indata, outdata, len)
     char *indata;
     char *outdata;
     int len;
{
  char *inp;		        /* Input buffer pointer */
  char *outp;                   /* Output buffer pointer */
  int val;			/* Current input sample value */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int diff;                     /* Difference between val and valprev */
  int step;			/* Stepsize */
  int valpred;		        /* virtual previous output value */
  int vpdiff;			/* Current change to valpred */
  int index;			/* Current step change index */
  int i=1, j=0;
  extern int chain_forecast();

#ifdef DELTA_STAT
  static FILE *F_delta, *F_histo;
  static int cpt=0;
  static int first=1;
  static int delta_histo[16];


  if(first){
    if((F_delta=fopen("delta.stat", "w")) == NULL){
      fprintf(stderr, "Cannot open delta.stat file\n");
      exit(1);
    }
    if((F_histo=fopen("delta.histo", "w")) == NULL){
      fprintf(stderr, "Cannot open delta.stat file\n");
      exit(1);
    }
    first=0;
  }
#endif /* DELTA_STAT */

  outp = outdata + DVI_STAT_SIZE;
  inp = indata;

  valpred=stat.valpred;
  *((short *)outdata)=(short)htons( stat.valpred );
  
  index=stat.index;
  outdata[2]=stat.index;
 
  step = stepsizeTable[index];
    
  outp[1] = 0;

  for(; len>0 ;len--){
    val = audio_u2c(*inp++); /* ulaw to linear encoding */

    /* Step 1 - compute difference with previous value */
    diff = val - valpred;
    sign = (diff < 0) ? 8 : 0;
    if (sign) diff = (-diff);

    /* Step 2 - Divide and clamp */
    /* Note:
    ** This code *approximately* computes:
    **    delta = diff*4/step;
    **    vpdiff = (delta+0.5)*step/4;
    ** but in shift step bits are dropped. The net result of this is
    ** that even if you have fast mul/div hardware you cannot put it to
    ** good use since the fixup would be too expensive.
    */
    delta = 0;
    vpdiff = (step >> 3);
    
    if ( diff >= step ) {
      delta = 4;
      diff -= step;
      vpdiff += step;
    }
    step >>= 1;
    if ( diff >= step  ) {
      delta |= 2;
      diff -= step;
      vpdiff += step;
    }
    step >>= 1;
    if ( diff >= step ) {
      delta |= 1;
      vpdiff += step;
    }
    
    
    /* Step 3 - Update previous value */
    if(sign)
      valpred -= vpdiff;
    else
      valpred += vpdiff;

    /* Step 4 - Clamp previous value to 8 bits (256 levels) */
    if(valpred > 127)
      valpred = 127;
    else if(valpred < -128)
      valpred = -128;

    /* Step 5 - Assemble value, update index and step values */
    delta |= sign;
    index += indexTable[delta];
    if(index < 0) index = 0;
    if(index > 88) index = 88;
    step = stepsizeTable[index];

#ifdef DELTA_STAT
    delta_histo[delta] ++;
    cpt ++;
#endif

    /* Step 6 - Output value */
    {
      register char *pt=adpcm_chain[delta];
      do{
	put_bit(outp, (*pt == '0' ? 0 : 1), i, j);
	pt ++;
      }while(*pt);
    }
  }
  outp[0] = j;
#ifdef DELTA_STAT
  {
    register int l;

    for(l=0; l<16; l++){
      fprintf(F_histo, "%d\t%f\n", l, (100.0*delta_histo[l]/(double)cpt));
    }
    fprintf(F_histo, "cpt = %d\n", cpt);
    fflush(F_histo);
  }
  fflush(F_delta);
#endif /* DELTA_STAT */

  stat.valpred=valpred;
  stat.index=index;

  return(i+1+DVI_STAT_SIZE);
}

int vadpcm_decoder(indata, outdata, len)
     char *indata;
     char *outdata;
     int len;
{
  char *outp;                   /* output buffer pointer */
  char *inp;                    /* input buffer pointer */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int valpred;		        /* Predicted value */
  int vpdiff;			/* Current change to valpred */
  int index;			/* Current step change index */
  int out_len=0;                /* output buffer len */
  int x, new_x;                 /* previous & new state */
  int new_bit;
  int i=1, j=0;
  int endj;
  static BOOLEAN first_error=FALSE;

  /* The state comes from the data header */
  outp = (u_char *)outdata;
  inp = (u_char *)indata + DVI_STAT_SIZE;

  valpred = (short)ntohs( *((short *)indata) );
  index = indata[2];

  len-=DVI_STAT_SIZE;
    
  endj = (char)inp[0]; 
  step = stepsizeTable[index];

  if (len==0)
    return 0;
  do{
    /* Step 1 - get the delta value */
    new_x = 0;
    do{
      x = new_x;
      new_bit = get_bit(inp, i, j);
      new_x = state_adpcm[x].bit[new_bit];
    }while(new_x != 0);
    delta = state_adpcm[x].result[new_bit];
    if(delta < 0 || i>len) {
	if(!first_error) {
	    fprintf(stderr, 
		    "vadpcm: received bad VADPCM data, old VADPCM coder ?\n");
	    first_error = TRUE;
	}
	return(0);
    }
    out_len ++;
    /* Step 2 - Find new index value (for later) */
    index += indexTable[delta];
    if (index < 0) index = 0;
    if (index > 88) index = 88;

    /* Step 3 - Separate sign and magnitude */
    sign = delta & 8;
    delta = delta & 7;
    
    /* Step 4 - Compute difference and new predicted value */
    /*
    ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
    ** in adpcm_coder.
    */
    vpdiff = step >> 3;
    if ( delta & 4 ) vpdiff += step;
    if ( delta & 2 ) vpdiff += step>>1;
    if ( delta & 1 ) vpdiff += step>>2;
    
    if ( sign )
      valpred -= vpdiff;
    else
      valpred += vpdiff;
    
    /* Step 5 - clamp output value */
    if(valpred > 127)
      valpred = 127;
    else if(valpred < -128)
      valpred = -128;

    /* Step 6 - Update step value */
    step = stepsizeTable[index];

    /* Step 7 - Output value */
    *outp++ = audio_c2u(valpred); /* linear to ulaw encoding */

  }while((i<(len-1)) || (j!=endj));
  return(out_len);
}
