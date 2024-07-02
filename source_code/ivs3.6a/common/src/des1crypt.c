/**************************************************************************\
* 	                						   *
*  File              : des1crypt.c	                		   *
*  Last Modification : 1995/2/22                                           *
*--------------------------------------------------------------------------*
*  Description       : Interface for the des1 encryption/decryption method.*
*                      Here, for the Saleem N. Bhatti DES code from UCL.   *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
* Ian Wakeman (UCL)     |  95/7/12  |  des1_convertkey() added.            *
\**************************************************************************/
#if defined(CRYPTO) && defined(DES1)
#include <stdio.h>
#include "endian.h"
#include "ivs_secu.h"
#include "qfDES.h"

/*#define DEBUG_DES1 1*/

extern char my_random();
extern void my_random_init();


int des1_convertkey(from, to)
    char *from, *to;
/* Take the 7 bytes in "from" and expand them to 8 bytes in "to"
 * Add parity bits later.
 * Assume that least significant bit is the parity bit
 */
{ 
    to[0] = from[0]; 
    to[1] = ((from[0] & 0x1)<<7) | (from[1]>>1);
    to[2] = ((from[1] & 0x3)<<6) | (from[2]>>2);
    to[3] = ((from[2] & 0x7)<<5) | (from[3]>>3);
    to[4] = ((from[3] & 0xf)<<4) | (from[4]>>4);
    to[5] = ((from[4] & 0x1f)<<3) | (from[5]>>5);
    to[6] = ((from[5] & 0x3f)<<2) | (from[6]>>6);
    to[7] = ((from[6] & 0x7f)<<1); 
}
 
 

int des1_crypt_rtp(buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{

    int lbuf, mod, pad;
    unsigned char initVector[8];
    unsigned char *s;
    register int i;

    for(i=0; i<8; i++)
	initVector[i] = 0;

    memcpy((char *)buf_out, (char *)buf_in, l);

    /* 
     * Pad the buffer input if necessary (8 byte chunks) 
     */
    s = (unsigned char *)(buf_out + l);
    mod = l & 7;
    pad = 8 - mod;
    *s++ = 128;
    for (i=1; i<pad; i++){
	*s++ = 0;
    }
    lbuf = l + pad;
    qfDES_CBC_e(key, buf_out, lbuf, initVector);
    return(lbuf);
}



int des1_uncrypt_rtp(buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{
    int lbuf=l;
    unsigned char initVector[8];
    register int i;

    for(i=0; i<8; i++)
	initVector[i] = 0;
    /* 
     * Assume buf_in is the same that buf_out to avoid another memcpy...
     */
    qfDES_CBC_d(key, buf_in, lbuf, initVector);
    for(i=0; i<7; i++){
	if(buf_in[lbuf-1] != 0)
	    break;
	lbuf--;
    }
    if(buf_in[lbuf-1] == 128){
	lbuf--;
    }
    return(lbuf);
}



int des1_crypt_rtcp(buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{

    static int first=1;
    int lbuf, mod, pad;
    unsigned char initVector[8];
    unsigned char *s;
    register int i;

    for(i=0; i<8; i++)
	initVector[i] = 0;

#ifdef DEBUG_DES1
    fprintf(stderr, "lbuf old: %d, new %d\n", l, lbuf);
#endif
    if(first){
	/* 
	 * Initialize a random seed for my_random().
	 */
	my_random_init();
	first = 0;
    }

    /*
     * Find LEN_HDR_RTCP_PROTECT random bytes to protect the header.
     */
    for(i=0; i<LEN_HDR_RTCP_PROTECT; i++){
	buf_out[i] = my_random();
    }
    memcpy((char *)buf_out+LEN_HDR_RTCP_PROTECT, (char *)buf_in, l);
    lbuf = l + LEN_HDR_RTCP_PROTECT;
#ifdef DEBUG_DES1
    fprintf(stderr, "before encryption ");
    dump_bytes(buf_out, 16);
#endif

    /* 
     * Pad the buffer input if necessary (8 byte chunks) 
     */
    s = (unsigned char *)(buf_out + lbuf);
    mod = lbuf & 7;
    pad = 8 - mod;
    *s++ = 128;
    for (i=1; i<pad; i++){
	*s++ = 0;
    }
    lbuf += pad;

    qfDES_CBC_e(key, buf_out, lbuf, initVector);
#ifdef DEBUG_DES1
    fprintf(stderr, "after encryption  ");
    dump_bytes(buf_out, 16);
#endif
    return(lbuf);
}



int des1_uncrypt_rtcp(buf_in, l, key, lkey, buf_out)
    unsigned char *buf_in, *key, *buf_out;
    int l, lkey;
{
    int lbuf=l;
    unsigned char initVector[8];
    register int i;

    for(i=0; i<8; i++)
	initVector[i] = 0;

#ifdef DEBUG_DES1
    fprintf(stderr, "before uncryption ");
    dump_bytes(buf_in, 16);
#endif
    qfDES_CBC_d(key, buf_in, lbuf, initVector);
#ifdef DEBUG_DES1
    fprintf(stderr, "after uncryption  ");
    dump_bytes(buf_in, 16);
    fprintf(stderr, "\n");
#endif
    for(i=0; i<7; i++){
	if(buf_in[lbuf-1] != 0)
	    break;
	lbuf--;
    }
    if(buf_in[lbuf-1] == 128){
	lbuf--;
    }
    lbuf -= LEN_HDR_RTCP_PROTECT;
    memcpy((char *)buf_out, (char *)(buf_in+LEN_HDR_RTCP_PROTECT), lbuf);
    return(lbuf);
}

#endif /* CRYPTO  && DES1 */
