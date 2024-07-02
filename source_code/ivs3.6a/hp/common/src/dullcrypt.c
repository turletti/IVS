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
*  File              : dullcrypt.c     	                		   *
*  Author            : Christian Huitema				   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       :  A dull encrypt/decrypt procedure.                  *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifdef CRYPTO
#include "md5.h"
#include "ivs_secu.h"


int dull_crypt(buf_in, lbuf, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int lbuf, lkey;
{
    register int i, j;

    int len_key=4; /* Only the first 32 bits of the key are used */

    for(i=0; i<lbuf; ){
	for(j=0; j<len_key && i<lbuf; i++, j++){
	    buf_out[i]= buf_in[i]^key[j];
	}
    }
    return(lbuf);
}
#endif /* CRYPTO */
