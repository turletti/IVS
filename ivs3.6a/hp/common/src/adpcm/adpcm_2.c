/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version  23-fev-94.
**
*/

#ifndef __STDC__
#define signed
#endif

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

    
/*----------------------------------------------------------------------*/
/* Intel ADPCM step variation table */
static int indexTable[4] = {
    -1, 8,
    -1, 8
};

static dvi_stat stat={0,0};

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

int adpcm2_coder(indata, outdata, len)
    char *indata;
    char *outdata;
    int len;
{
    char *inp;			/* Input buffer pointer */
    char *outp;		        /* output buffer pointer */
    int val;			/* Current input sample value */
    int sign;			/* Current adpcm sign bit */
    
    int delta;			/* Current adpcm output value */
    int diff;			/* Difference between val and valprev */
    int step;			/* Stepsize */
    int valpred;    	        /* Predicted output value */
    int vpdiff;			/* Current change to valpred */
    int index;      	        /* Current step change index */
    int outputbuffer;		/* place to keep previous 4-bit value */
    int cptr;	                /* toggle between outputbuffer/output */
    int lg;                     /* longueur du fichier donnee par len */
#ifdef DELTA_STAT
    static FILE *F_delta , *F_histo;
    static int cpt=0;
    static int first=1;
    static int delta_histo[8];
 
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

    outputbuffer = 0;
    cptr=0;   /* remplissage courant*/
    lg=len;

    for ( ; len > 0 ; len-- ) {
	val = audio_u2s(*inp++);
	/*printf("val %i",val);*/

	/* Step 1 - compute difference with previous value */
	diff = val - valpred;
	sign = (diff < 0) ? 2 : 0;
	if ( sign ) diff = (-diff);

	/* Step 2 - Divide and clamp */
	
	delta = 0;
	vpdiff = (step >> 1);
	
	if ( diff >= step ) {
	    delta = 1;
	    diff -= step;
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
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	step = stepsizeTable[index];

#ifdef DELTA_STAT
	delta_histo[delta] ++;
/*	fprintf(F_delta, "%d \t %d\n", cpt, delta);*/
	cpt ++;
#endif

	if (cptr==0 ) outputbuffer = (delta << 6) & 0xc0;
	if (cptr==1 ) outputbuffer |= (delta << 4);
	if (cptr==2 ) outputbuffer |= (delta << 2);
	if (cptr==3) 
	  {
	    *outp++ = (delta) | outputbuffer;
	    cptr=0;
	  } 
	else  cptr++;
    }
  if(cptr!=0) *outp++ =  outputbuffer; 

#ifdef DELTA_STAT
    {
      register int l;
      
      for(l=0; l<8; l++){
	fprintf(F_histo, "%d\t%f\n", l, (100.0*delta_histo[l]/(double)cpt));
      }
      fprintf(F_histo, "cpt = %d\n", cpt);
      fflush(F_histo);
    }
    fflush(F_delta);
#endif 

    stat.valpred=valpred;
    stat.index=index;

    return(outp-outdata);
}

int adpcm2_decoder(indata, outdata, len)
    char *indata;
    char *outdata;
    int len;
{
    u_char *inp;		/* Input buffer pointer */
    char *outp;		        /* output buffer pointer */
    int sign;			/* Current adpcm sign bit */
    int delta;			/* Current adpcm output value */
    int step;			/* Stepsize */
    int valpred;		/* Predicted value */
    int vpdiff;			/* Current change to valpred */
    int index;			/* Current step change index */
    int inputbuffer;           	/* place to keep next 4-bit value */
    int comptr;                 /* toggle between inputbuffer/input*/
  
    outp = outdata;
    inp = (u_char *)indata + DVI_STAT_SIZE;

    valpred = (short)ntohs( *((short *)indata) );
    index = indata[2];

    len-=DVI_STAT_SIZE;
    
    step = stepsizeTable[index];
    comptr = 0;

    len=len*8/2;
    for (inputbuffer=*inp++ ; len > 0 ; len-- ) {
	/* Step 1 - get the delta value */
         if (comptr==0 )  delta=(inputbuffer>>6)&0x03;
	 if (comptr==1 )  delta= (inputbuffer>> 4)&0x03;
	 if (comptr==2 )  delta= (inputbuffer>> 2)&0x03;
	 if (comptr==3) 
	   {
	     delta=(inputbuffer)&0x03;
	     inputbuffer=*inp++;
	     comptr=0;
	   }
	 else comptr++;
 
	/* Step 2 - Find new index value (for later) */
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	/* Step 3 - Separate sign and magnitude */
	sign = delta & 2;        
	delta = delta & 1;    
	/* Step 4 - Compute difference and new predicted value */
	vpdiff = step >> 1;
	if ( delta & 1 ) vpdiff += step;
	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;
	/* Step 5 - clamp output value */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;
	/* Step 6 - Update step value */
	step = stepsizeTable[index];

	/* Step 7 - Output value */
	 *outp++ = audio_s2u(valpred);
    }/* loop */

    return(outp-outdata);
}





