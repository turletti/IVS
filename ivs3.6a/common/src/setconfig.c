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
*  File              : setconfig.c   	                		   *
*  Author            : Christian Huitema				   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Some declarations...                                *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifdef CRYPTO

#define SETCONFIG 1
#include <stdio.h>
#include "ivs_secu.h"

extern int dull_crypt();
extern int lessdull_crypt(), lessdull_uncrypt();
#ifdef DES1
extern int des1_crypt_rtp(), des1_uncrypt_rtp(), des1_convertkey();
extern int des1_crypt_rtcp(), des1_uncrypt_rtcp();
#endif /* DES1 */

struct crypto_method method[] = {
    {"dull", dull_crypt, dull_crypt, dull_crypt, dull_crypt, NULL, LEN_KEY},
    {"lessdull", lessdull_crypt, lessdull_uncrypt, lessdull_crypt, 
     lessdull_uncrypt, NULL, LEN_KEY},
#ifdef DES1
    {"des1", des1_crypt_rtp, des1_uncrypt_rtp, des1_crypt_rtcp, 
     des1_uncrypt_rtcp, des1_convertkey, 8}
#endif /* DES1 */
  };

int nb_methods = sizeof(method)/ sizeof(struct crypto_method);
int current_method = RTP_NO_CRYPT; /* No encryption by default */
int len_key;
char s_key[LEN_MAX_STR];

#endif /* CRYPTO */
