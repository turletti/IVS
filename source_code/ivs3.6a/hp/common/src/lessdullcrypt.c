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
*  File              : lessdullcrypt.c	                		   *
*  Author            : Christian Huitema				   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : A less dull encryption/decryption method.           *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifdef CRYPTO
#include "endian.h"
#include "md5.h"
#include "ivs_secu.h"

extern char my_random();
extern void my_random_init();

int lessdull_crypt(buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{

    MD5_CTX mdContext;
    unsigned char buf[32];
    register int lbuf, i, j;
    static int first=1;

    if(first){
	/* 
	 * Initialize a random seed for my_random().
	 */
	my_random_init();
	first = 0;
    }
    /*
     * Find 8 random bytes.
     */
    for(i=0; i<8; i++){
	buf_out[i] = buf[i] = my_random();
    }

    for(i=0,lbuf=8; i<lkey && lbuf<32; i++, lbuf++){
	buf[lbuf] = key[i];
    }

    MD5Init  (&mdContext);
    MD5Update(&mdContext, (unsigned char *)buf, (unsigned int)lbuf);
    MD5Final (&mdContext);
#ifdef BIT_ZERO_ON_RIGHT
    MDreverse(mdContext.buf);
#endif

    for(i=0, lbuf=8; i<l; ){
	for(j=0; j<16 && i<l; j++,i++,lbuf++){
	    buf_out[lbuf] = buf_in[i] ^ ((char *)mdContext.buf)[j];
	}
    }
    return(lbuf);
}



int lessdull_uncrypt (buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{
    MD5_CTX mdContext;
    unsigned char buf_temp[32];
    register int lbuf, i, j;

    for(i=0; i<8; i++){
	buf_temp[i] = buf_in[i];
    }
    for(i=0,lbuf=8; i<lkey && lbuf<32; i++, lbuf++){
	buf_temp[lbuf] = key[i];
    }

    MD5Init  (&mdContext);
    MD5Update(&mdContext, (unsigned char *)buf_temp, (unsigned int)lbuf);
    MD5Final (&mdContext);
#ifdef BIT_ZERO_ON_RIGHT
    MDreverse(mdContext.buf);
#endif

    for(i=8, lbuf=0; i<l; ){
	for(j=0; j<16 && i<l; j++,i++,lbuf++){
	    buf_out[lbuf] = buf_in[i] ^ ((char *)mdContext.buf)[j];
	}
    }

    return(lbuf);
}
#endif /* CRYPTO */
