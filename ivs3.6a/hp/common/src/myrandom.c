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
*  File              : my_random.c    	                		   *
*  Author            : Christian Huitema				   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : A random generation function.                       *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifdef CRYPTO
#include <sys/types.h>
#include <sys/time.h>
#include "md5.h"

static int random_set[64] = {
    0x7be9c1bd, 0x88aa1020, 0x3d38509b, 0x746b9fbe,
    0x2d04417f, 0x775d4351, 0x53c48d96, 0xc2b26e0b,
    0x418fedcf, 0x19dbc19e, 0x78512adb, 0x1a1f5e2b,
    0xb07d7761, 0x6584c1f0, 0x24e3c36f, 0x2232310f,
    0x2dac5ceb, 0x106e8b5a, 0x5a05a938, 0x5e6392b6,
    0x66b90348, 0x75264901, 0x4174f402, 0xd18b18a4,
    0x3a6ab4bf, 0x3c4bc289, 0x2657a9a9, 0x4e68589b,
    0x89648aa6, 0x3fc489bb, 0x1c1b715c, 0x54e4c63,
    0x484f2abd, 0x5953c1f8, 0x79b9ec22, 0x75536c3d,
    0x50b10549, 0x4d7e79b8, 0x7805da48, 0x1240f318,
    0x675a3b56, 0x70570523, 0x2c605143, 0x17d7b2b7,
    0x55dbc713, 0x514414b2, 0x3a09e3c7, 0xa38823ff,
    0x61b2a00c, 0x140f8cff, 0x61ebb6b5, 0x486ba354,
    0x0935d600, 0x2360aab8, 0x29f6bbf8, 0x43a08abf,
    0x5fac6d41, 0x504e65a2, 0xb208e35b, 0x6910f7e7,
    0x1012ef5d, 0x2e2454b7, 0x6e5f444b, 0x58621a1b,
};
static int next_number = 0;

char my_random()
{
    register long rdn;
    register i = next_number;
    char byte_rdn;

    rdn = random_set[i];
    next_number = (i-1) & 63;
    i++;
    i &= 63;
    random_set[i] ^= rdn;
    byte_rdn = (rdn >> 16) & 0xff;
    return(byte_rdn);
}

/*
 * Input a new seed in the process.
 *
 * Seed is always expressed as a character string. It can be a 
 * system identification or any other form of text. It will be
 * passed through MD5 in order to concentrate the randomness.
 *
 * A very good idea is to initiate the seed by taking some data
 * from a microphone. Noise is a very good approximation of white
 * noise.
 */
static void my_random_seed(s, l)
    char * s;
    int l;
{
    MD5_CTX mdContext;
    int i, j;
    long r;

    /*
     * Apply MD5 to string so as to concentrate 
     * the randomness.
     */
    MD5Init  (&mdContext);
    MD5Update(&mdContext, (unsigned char *)s, (unsigned int)l);
    MD5Final (&mdContext);
 
    /*
     * Update 4 values.
     * Using XOR garantees that we do not remove randomness.
     */
    for(i=0; i<4; i++){
	random_set[next_number--] ^= mdContext.buf[i];
	next_number &= 63;
    }
    /*
     * Check that we do not end up with an entirely null
     * line, because it would remain null as aeternam.
     * Correct if needed.
     */
    r = 0;
    for (i=0; i<64; i++){
	r |= random_set[i];
    }
    random_set[0] |= ~r;
    /*
     * Spread the randomness on the whole set of 64 numbers.
     */
    j = next_number;
    for (i=0; i<64; i++){
	register k = (j+4)&63;
	random_set[j--] ^= random_set[k];
	j &= 63;
    }
}

/*
 * Do the very initial randomization, using machine
 * specific parameters and the time of day.
 */

struct random_seed {
    struct timeval tp;
    struct timezone tzp; 
    short pid;
    char hname[256];
};

my_random_init()
{
    struct random_seed rs;
    (void) gettimeofday(&rs.tp, &rs.tzp);
    rs.pid = getpid();
    gethostname(rs.hname, 256);
    my_random_seed ((char *)&rs, sizeof(rs));
}

#endif /* CRYPTO */
