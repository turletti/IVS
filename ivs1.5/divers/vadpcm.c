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
** Version 1.0, 7-Jul-92.
*/
/**************************************************************************\
* File        : vadpcm.c                                                   *
* Author      : Jack Jansen                                                *
* Modified by : Thierry Turletti                                           *
* Date        : 24/7/92                                                    *
*--------------------------------------------------------------------------*
* Modifications : - ulaw to linear and linear to ulaw addition, (12/8 bits)*
*                 - Fixed rate ADPCM codec generates sequences of          * 
* identical successives coefficients. Here, each 4-bit sample code is      *
* followed by a 4-bit code which represents the number of successive       *
* occurences of the sample code. A version with huffman table is under     *
* consideration.                                                           *
\**************************************************************************/

#include <stdio.h>
#include "ulaw2linear.h"

#ifndef __STDC__
#define signed
#endif

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
     char outdata[];
     int len;
{
  unsigned char *inp;		/* Input buffer pointer */
  char *outp;    		/* output buffer pointer */
  int val;			/* Current input sample value */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int valprev;		        /* virtual previous output value */
  int vpdiff;			/* Current change to valprev */
  int index;			/* Current step change index */
  int outputbuffer;		/* place to keep previous 4-bit value */
  int out_len=0;                /* output buffer len */
  int prev_delta;               /* Previous adpcm output value */
  int n_delta=-1;               /* number of same adpcm output value */

#ifdef DELTA_STAT
  static FILE *F_delta;
  static int cpt=0;
  static int first=1;
  static int histo[16];

  if(first){
    if((F_delta=fopen("delta.stat", "w")) == NULL){
      fprintf(stderr, "Cannot open delta.stat file\n");
      exit(1);
    }
    first=0;
  }
#endif DELTA_STAT

  outp = (signed char *)outdata;
  inp = indata;

  valprev = 0;
  index = 0; 
  step = stepsizeTable[index];
    
  for ( ; len > 0 ; len-- ) {
    val = audio_u2c(*inp++); /* ulaw to linear encoding */
    
    /* Step 1 - compute difference with previous value */
    delta = val - valprev;
    sign = (delta < 0) ? 8 : 0;
    if (sign) delta = (-delta);

    /* Step 2 - Divide and clamp */
#ifdef NODIVMUL
    {
      int tmp = 0;

      vpdiff = 0;
      if(delta > step){
	tmp = 4;
	delta -= step;
	vpdiff = step;
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

    vpdiff = (delta*step) >> 2;
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


    /* Step 6 - Output value 
    * 4-bits sample level + 4-bits number of occurences of this sample.
    */
    if(n_delta < 0){
      prev_delta = delta;
      n_delta ++;
    }else{
      if(delta == prev_delta){
	n_delta ++;
	if(n_delta == 15){
#ifdef DELTA_STAT
	  fprintf(F_delta, "%d \t %d\n", cpt, n_delta);
	  histo[delta] += n_delta;
	  cpt ++;
#endif DELTA_STAT
	  *outp++ = (prev_delta << 4) | 15;
	  n_delta = -1;
	  out_len ++;
	}
      }else{
#ifdef DELTA_STAT
	fprintf(F_delta, "%d \t %d\n", cpt, n_delta);
	histo[delta] += n_delta;
	cpt ++;
#endif DELTA_STAT
	*outp++ = (prev_delta << 4) | n_delta;
	n_delta = 0;
	prev_delta = delta;
	out_len ++;
      }
    }
  }

  /* Output last samples, if needed */
  if(n_delta >= 0){
    *outp++ = (prev_delta << 4) | n_delta;
    out_len ++;
  }
#ifdef DELTA_STAT
  if(cpt>10000){
    register int i;
    cpt=0;
    for(i=0; i<16; i++)
      fprintf(stderr, "%d\t %d\n", i, histo[i]);
  }
#endif DELTA_STAT
  return(out_len);
}



int vadpcm_decoder(indata, outdata, len)
     char indata[];
     char outdata[];
     int len;
{
  signed char *inp;		/* Input buffer pointer */
  char *outp;		        /* output buffer pointer */
  int sign;			/* Current adpcm sign bit */
  int delta;			/* Current adpcm output value */
  int step;			/* Stepsize */
  int valprev;		        /* virtual previous output value */
  int vpdiff;			/* Current change to valprev */
  int index;			/* Current step change index */
  int inputbuffer;		/* place to keep next 4-bit value */
  int out_len=0;                /* output buffer len */
  int n_delta=-1;               /* Previous adpcm output value */
  int prev_delta;               /* number of same adpcm output value */


  outp = outdata;
  inp = (signed char *)indata;
  valprev = 0; 
  index = 0; 
  step = stepsizeTable[index];

  do{
    /* Step 1 - get the delta value */
    if(n_delta < 0){
      prev_delta = delta = (*inp >> 4) & 0x0f;
      n_delta = (*inp++ & 0x0f) - 1;
      len --;
    }else{
      delta = prev_delta;
      n_delta --;
    }
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
    vpdiff = 0;
    if (delta & 4) vpdiff  = (step << 2);
    if (delta & 2) vpdiff += (step << 1);
    if (delta & 1) vpdiff += step;
    vpdiff >>= 2;
#else
    vpdiff = (delta*step) >> 2;
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
    *outp++ = audio_c2u(valprev); /* linear to ulaw encoding */

  }while(len || (n_delta>=0));
  return(out_len);
}
