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
*  File              : getkey.c       	                		   *
*  Author            : Christian Huitema, Thierry Turletti                 *
*  Last Modification : 1995/2/17                                           *
*--------------------------------------------------------------------------*
*  Description       : getkey() procedure. See also the following draft:   *
*  ``The use of plain text keys for encryption of multimedia conferences'' *
*  Draft V1.3, Feb 23th, 1995 from Mark Handley, UCL.                      *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*  Ian Wakeman (UCL)    |  95/7/12  | Added the convertkey() call.         *
\**************************************************************************/

#ifdef CRYPTO
#include <stdio.h>
#include <malloc.h>
#include "endian.h"
#include "md5.h"
#include "ivs_secu.h"

extern int MDreverse();


int getkey(s, key, lkey)
    char *s;
    char *key;
    int  *lkey;
{
    MD5_CTX mdContext;
    char *buf, *x;
    char tmpkey[16];
    int ml = -1, keyl, bl;
    int m = RTP_DEFAULT_CRYPT;
    register int i, j;

    /* Get the method number from s */

    for (x = s; *x; x++){
        if (*x == '/'){
            ml = x - s;
            break;
        }
    }
    if (ml == -1){
        x = s;
    }else{
	s[ml] = 0;
	x = s + ml + 1;
	m =  RTP_NO_CRYPT;
	for(i=0; i<nb_methods; i++){
	    if(strcmp(method[i].name, s) == 0){
		m = i;
		break;
	    }
	}
	if(m == RTP_NO_CRYPT){
	    m = RTP_DEFAULT_CRYPT;
	    s[ml] = '/';
	    x = s;
	}
    }

    /* Get the key value and encrypt it */
    keyl = strlen(x);
    if(keyl == 0){
	m = RTP_NO_CRYPT;
    }else{
	bl = keyl & 0xFFF0;
	if (keyl != bl){
	    bl += 16;
	}
	buf = malloc((unsigned) bl);
	for(i=0; i<bl; ){
	    for(j=0; j<keyl && i<bl; i++, j++){
		buf[i] = x[j];
	    }
	}
	MD5Init  (&mdContext);
	MD5Update(&mdContext, (unsigned char *)buf, (unsigned int)bl);
	MD5Final (&mdContext);
#ifdef BIT_ZERO_ON_RIGHT
	MDreverse(mdContext.buf);
#endif
	if(method[m].convertkey) {
	    for (i=0; i<16; i++){
		tmpkey[i] = ((char *)mdContext.buf)[i];
	    }
	    method[m].convertkey(tmpkey, key);
	} else {
	    for (i=0; i<16; i++){
		key[i] = ((char *)mdContext.buf)[i];
	    }
	}
	*lkey = 16;
    }
    return(m);	
}

void dump_bytes(buf, len_buf)
    char *buf;
    int len_buf;
{
    register int i;

    for(i=0; i<len_buf; i++)
	fprintf(stderr, "%d ", (unsigned char) *buf++);
    fprintf(stderr, "\n");
}


void test_key(where)
    char *where;
{
   fprintf(stderr, "%s :: method : %d, len_key is %d\n", where, 
	   current_method, len_key);
   fprintf(stderr, "KEY --> ");
   dump_bytes(s_key, len_key); 
   return;
}
#endif /* CRYPTO */
