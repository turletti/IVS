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
** Version 1.1, 16-Dec-92.
**
** Change log:
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4. The old behavior can
**   still be gotten by defining STUPID_V1_BUG.
*/
/**************************************************************************\
* File        : hadpcm.c                                                   *
* Author      : Jack Jansen                                                *
* Modified by : Thierry Turletti                                           *
* Date        : 28/7/92                                                    *
*--------------------------------------------------------------------------*
* Modifications : - ulaw to linear and linear to ulaw addition, (12/8 bits)*
*                 - Huffman tables are used to encode audio ADPCM samples. *
\**************************************************************************/


#include <stdio.h>
#include "adpcm.h"
#include "ulaw2linear.h"

#define BOOLEAN     unsigned char
#ifndef TRUE
#define TRUE                    1
#endif /* TRUE */
#ifndef FALSE                   
#define FALSE                   0
#endif /* FALSE */


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
     unsigned char indata[];
     unsigned char outdata[];
     int len;
{
  unsigned char *inp;		/* Input buffer pointer */
  int val;			/* Current input sample value */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int valprev;		        /* virtual previous output value */
  int vpdiff;			/* Current change to valprev */
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

  outdata[1] = 0;
  inp = indata;

  valprev = 0;
  index = 0; 
  step = stepsizeTable[index];
    
  for(; len>0 ;len--){
    val = audio_u2c(*inp++); /* ulaw to linear encoding */

    /* Step 1 - compute difference with previous value */
    delta = val - valprev;
    sign = (delta < 0) ? 8 : 0;
    if (sign) delta = (-delta);

    /* Step 2 - Divide and clamp */
#ifdef NODIVMUL
    {
      int tmp = 0;

      vpdiff = (step >> 3);
      if(delta > step){
	tmp = 4;
	delta -= step;
	vpdiff += step;
      }
      step >>= 1;
      if(delta > step){
	tmp |= 2;
	delta -= step;
	vpdiff += step;
      }
      step >>= 1;
      if(delta > step){
	tmp |= 1;
	vpdiff += step;
      }
      delta = tmp;
    }
#else
    delta = (delta << 2) / step;
    if(delta > 7) delta = 7;
    vpdiff = ((delta*step) >> 2) + (step >> 3);
#endif
	  
    /* Step 3 - Update previous value */
    if(sign)
      valprev -= vpdiff;
    else
      valprev += vpdiff;

    /* Step 4 - Clamp previous value to 8 bits (256 levels) */
    if(valprev > 127)
      valprev = 127;
    else if(valprev < -128)
      valprev = -128;

    /* Step 5 - Assemble value, update index and step values */
    delta |= sign;
    index += indexTable[delta];
    if(index < 0) index = 0;
    if(index > 88) index = 88;
    step = stepsizeTable[index];

#ifdef DELTA_STAT
    delta_histo[delta] ++;
/*    fprintf(F_delta, "%d \t %d\n", cpt, delta);*/
    cpt ++;
#endif

    /* Step 6 - Output value */
    {
      register char *pt=adpcm_chain[delta];
      do{
	put_bit(outdata, (*pt == '0' ? 0 : 1), i, j);
	pt ++;
      }while(*pt);
    }
  }
  outdata[0] = j;
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
  return(i+1);
}



int vadpcm_decoder(indata, outdata, len, ADD)
     unsigned char indata[];
     unsigned char outdata[];
     int len;
     BOOLEAN ADD; /* True if we have to sum data */
{
  unsigned char *outp;          /* output buffer pointer */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int valprev;		        /* virtual previous output value */
  int vpdiff;			/* Current change to valprev */
  int index;			/* Current step change index */
  int out_len=0;                /* output buffer len */
  int x, new_x;                 /* previous & new state */
  int new_bit;
  int i=1, j=0;
  int endj;

  outp = outdata;
  valprev = 0; 
  index = 0; 
  endj = indata[0]; 
  step = stepsizeTable[index];

  do{
    /* Step 1 - get the delta value */
    new_x = 0;
    do{
      x = new_x;
      new_bit = get_bit(indata, i, j);
      new_x = state_adpcm[x].bit[new_bit];
    }while(new_x != 0);
    delta = state_adpcm[x].result[new_bit];
    out_len ++;
    /* Step 2 - Find new index value (for later) */
    index += indexTable[delta];
    if (index < 0) index = 0;
    if (index > 88) index = 88;

    /* Step 3 - Separate sign and magnitude */
    sign = delta & 8;
    delta = delta & 7;
    
    /* Step 4 - update output value */
#ifdef NODIVMUL
    vpdiff = step >> 1;
    if (delta & 4) vpdiff  = (step << 2);
    if (delta & 2) vpdiff += (step << 1);
    if (delta & 1) vpdiff += step;
    vpdiff >>= 2;
#else
    vpdiff = ((delta*step) >> 2) + (step >> 3);
#endif
    if (sign)
      valprev -= vpdiff;
    else
      valprev += vpdiff;

    /* Step 5 - clamp output value */
    if(valprev > 127)
      valprev = 127;
    else if(valprev < -128)
      valprev = -128;

    /* Step 6 - Update step value */
    step = stepsizeTable[index];

    /* Step 7 - Output value */
    if(ADD){
      *outp = audio_c2u(audio_u2c(*outp)+valprev);
      outp++;
    }else
      *outp++ = audio_c2u(valprev); /* linear to ulaw encoding */

  }while((i<(len-1)) || (j!=endj));
  return(out_len);
}
