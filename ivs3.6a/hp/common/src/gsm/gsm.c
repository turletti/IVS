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
*  File              : gsm.c          	                		   *
*  Author            : Andres Vega Garcia				   *
*  Last Modification : 1995/04/6                                           *
*--------------------------------------------------------------------------*
*  Description       : The IVS interface to the gsm-1.0.4 package.         *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#include <memory.h>

#include "private.h"
#include "gsm.h"
#include "toast.h"

#define GSMSIZE 160
#define GSMFRAME 33

/***********************************************
 * indata must have available space to align
 * to the upper 160 bytes frame.
 * IN 160 bytes -> OUT 33 bytes
 ***********************************************/
int gsm_coder(indata, outdata, len)
    char indata[];
    char outdata[];
    int len;
{
    gsm_signal s[GSMSIZE];
    int in,fr;

    static struct gsm_state gsms;
    static FIRST=1;
    
    if (FIRST) {
	FIRST=0;
	memset((char *)&gsms,0,sizeof(gsms));
	gsms.nrp=40;
    }
    for(in=fr=0;len >= GSMSIZE;len-=GSMSIZE,in+=GSMSIZE,fr+=GSMFRAME) {
	ulaw_input(&indata[in],s);
	gsm_encode(&gsms, s, (gsm_byte *)&outdata[fr]);
    }
    if (len>0) {
	memset(&indata[len],0,GSMSIZE-len);
	ulaw_input(&indata[in],s);
	gsm_encode(&gsms, s, (gsm_byte *)&outdata[fr]);
	fr+=GSMFRAME;
    }
    return(fr);
}

int gsm_decoder(indata, outdata, len)
    char indata[];
    char outdata[];
    int len;
{
    gsm_signal d[GSMSIZE];
    int out,fr;

    static struct gsm_state gsms;
    static FIRST=1;
    
    if (FIRST) {
	FIRST=0;
	memset((char *)&gsms,0,sizeof(gsms));
	gsms.nrp=40;
    }
    for(out=fr=0;len >= GSMFRAME;out+=GSMSIZE,len-=GSMFRAME,fr+=GSMFRAME) {
	gsm_decode(&gsms,(gsm_byte *)&indata[fr],d);
	ulaw_output(d,&outdata[out]);
    }
    if (len > 0)
	fprintf(stderr,"gsm_decoder: discarded frame with size %d < %d bytes\n",len,GSMFRAME);
    return(out);
}

void coder_lpc2(indata, outdata, len)
    char indata[];
    char outdata[];
    int len;
{
}

void decoder_lpc2(indata, outdata, len)
    char indata[];
    char outdata[];
    int len;
{
}

