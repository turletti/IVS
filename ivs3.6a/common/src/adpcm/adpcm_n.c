/*
** Intel/DVI ADPCM coder/decoder.
*/

#ifndef __STDC__
#define signed
#endif

#include <stdio.h>
#include "../ulaw2linear.h"
#include "adpcm_n.h"

/*----------------------------------------------------------------------*/

static dvi_stat stat5={0,0};
static dvi_stat stat6={0,0};

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


int adpcm6_coder(indata, outdata, len)
    char *indata;
    char *outdata;
    int len;
{    return(adpcm_coder(indata,outdata,len,6, (char *)&stat6)); }

int adpcm5_coder(indata, outdata, len, stat)
    char *indata;
    char *outdata;
    int len;
{    return(adpcm_coder(indata,outdata,len,5,(char *)&stat5)); }

/*
 * The coding routine takes in a PCM encoded sound piece (inp, len)
 * and returns an ADPCM encoded packet (outp). The procedure returns
 * the length of the encoding.
 */

static int adpcm_coder(inp, outp, len, nbits, stat)
    unsigned char * inp;
    unsigned char * outp;  
    int len, nbits;
    char *stat;
{
    int val;			/* Current input sample value */
    int sign;			/* Current adpcm sign bit */
    int delta;			/* Current adpcm output value */
    int diff;			/* Difference between val and valprev */
    int step;			/* Stepsize */
    int valpred;		/* Predicted output value */
    int vpdiff;			/* Current change to valpred */
    int index;	      		/* Current step change index */
    int outputbuffer=0;		/* place to keep previous nbits value */
    int total,comptr,reste;	/* toggle between outputbuffer/output */
    int lg;
    unsigned char * outp0;

/*----------*/
/*    
    valpred = pac_prec->valpred;
    index =pac_prec->index;

    outp0 = outp;
    */
/*----------*/
    outp0 = outp;

    valpred=((dvi_stat *)stat)->valpred;
    index=((dvi_stat *)stat)->index;

    /* Compute the number of unused bits */
    total = len*nbits;
    reste = total>>3;
    reste <<= 3;
    if (total == reste){
	reste = 0;
    }else{
	reste += 8;
	reste -= total;
    }

    /* Compute the adpcm content header */
    *outp++ = (nbits<<4)|(reste);
    *outp++ = valpred>>8;
    *outp++ = valpred&0xFF;
    *outp++ = index;

    /* Encode the data */
    step = stepsizeTable[index];
    comptr=0;
    lg=len;
    for ( ; len > 0 ; len-- ) {
	val = audio_u2s(*inp++);
	
	/* Step 1 - compute difference with previous value */

	diff = val - valpred;
	sign = (diff < 0) ? 32 : 0;
	if ( sign ) diff = (-diff);

	/* Step 2 - Divide and clamp */
	delta = 0;
	vpdiff = (step >> 5);
	if ( diff >= step ) {
	    delta = 16;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step && nbits > 2) {
	    delta |= 8;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step && nbits > 3) {
	    delta |= 4;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step  && nbits > 4) {
	    delta |= 2;
	    diff -= step;
	    vpdiff += step;
	  }
	step >>= 1;
	if ( diff >= step && nbits > 5) {
	    delta |= 1;
	    vpdiff += step;
	}

	/* Step 3 - Update previous value */ 
	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 4 - Clamp previous value to 16 bits */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;

	/* Step 5 - Assemble value, update index and step values */
	delta |= sign;
	index += indexTable[(delta>>2)];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	step = stepsizeTable[index];

	/* Step 6 - Output value */
	delta >>= (6 - nbits);
	total=comptr + nbits;     
	if(total<8) 
	{
	    outputbuffer |= (delta << (8-total));
	    comptr += nbits;
	}
	else
	{
	    reste = total - 8;    /* what is left for the next round */
	    *outp++ =  outputbuffer|(delta >> reste);
	    outputbuffer = (delta << (8-reste));
	    comptr = reste;
	} 
    }  /* for() */

    if (comptr){
	outputbuffer <<= (8 - comptr);
	*outp++ =  outputbuffer;
    }

    ((dvi_stat *)stat)->valpred=valpred;
    ((dvi_stat *)stat)->index=index;

    return (outp - outp0);
}

/*
 * The decoding routine takes as input an ADPCM encoded (inp)
 * packet of "len" bytes long. The PCM value is placed in
 * the buffer "outp"; its length is returned.
 *
 * The ADPCM packet starts by a content header giving the reference
 * value, the current index, the bits per sample and the number
 * of unused bits at the end of the packet.
 */

int adpcm_decoder(inp, outp, len)
    unsigned char * inp;
    unsigned char * outp;
    int len;
{
    int nbits;			/* Bits per sample */
    int sign;			/* Current adpcm sign bit */
    int delta;			/* Current adpcm output value */
    int step;			/* Stepsize */
    int valpred;		/* Predicted value */
    int vpdiff;			/* Current change to valpred */
    int inputbuffer;		/* place to keep next nbits value */
    int comptr,reste;	        /* toggle between inputbuffer/input */
    int index;
    int nsamples;
    unsigned char * outp0;

    short data[2];
    int i=0;
    register int idx=0;

    /*
     * Decode the content header
     */
    if (len < 5){
	/* What ? */
	return(0);
    }
    nbits = (inp[0]>>4)&0x0F;
    reste = inp[0]&0x0F;
    if (nbits < 2 || nbits > 6 || index < 0 || index > 88 || reste > 7){
	/* this is bogus */
	return(0);
    }
    valpred = (inp[1]<<8)|(inp[2]);
    if (valpred&0x8000)
	valpred -= 0x010000;
    index = inp[3];
    /*
     * Compute the number of samples.
     */
    inp += 4;
    len -= 4;
    nsamples = ((len * 8) - reste)/nbits;

    /*
     * Decode all the samples.
     */
    outp0 = outp;
    step = stepsizeTable[index];
    comptr = 8;
    inputbuffer = *inp++;
    for (; nsamples; nsamples--){
	/* 
	 * Get the next sample 
	 */
	if (comptr > nbits){
	    comptr -= nbits;
	    delta = inputbuffer >> comptr;
	}else{
	    delta = inputbuffer;
	    inputbuffer = *inp++;
	    delta <<= (nbits - comptr);
	    comptr += 8 - nbits;
	    delta |= (inputbuffer >> comptr);
	}
	/* 
	 * Align to a size of 6 bits 
	 */
	delta <<= (6 - nbits);
	delta &= 0x3F;

	/* 
	 * Step 2 - Find new index value (for later) 
	 */
	index += indexTable[delta>>2];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;

	/* 
	 * Step 3 - Separate sign and magnitude 
	 */
	sign = delta & 32;
	delta &= 31;

	/* 
	 * Step 4 - Compute difference and new predicted value.
	 * Note that we take the "middle" value between two code points.
	 */
	vpdiff = step >> (nbits - 1);
        if ( delta & 16 ) vpdiff += step;
	if ( delta & 8 ) vpdiff += step>>1;
        if ( delta & 4 ) vpdiff += step>>2;
        if ( delta & 2 ) vpdiff += step>>3;
        if ( delta & 1 ) vpdiff += step>>4;

	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* 
	 * Step 5 - clamp output value.
	 */

	if ( valpred > 32767 )
	    valpred = 32767;
	else if ( valpred < -32768 )
	    valpred = -32768;
	/* 
	 * Step 6 - Update step value 
	 */
	step = stepsizeTable[index];
	/* 
	 * Step 7 - Output value.
	 */
	*outp++ = audio_s2u(valpred);
    }

    return (outp - outp0);
}


