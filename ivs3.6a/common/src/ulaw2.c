/*
** These routines convert between 16-bit linear and ulaw.
**
** Craig Reese: IDA/Supercomputing Research Center
** Joe Campbell: Department of Defense
** 29 September 1989
**
** References:
** 1) CCITT Recommendation G.711  (very difficult to follow)
** 2) "A New Digital Technique for Implementation of Any
**     Continuous PCM Companding Law," Villeret, Michel,
**     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
**     1973, pg. 11.12-11.17
** 3) MIL-STD-188-113,"Interoperability and Performance Standards
**     for Analog-to_Digital Conversion Techniques,"
**     17 February 1987
*/
#include "ulaw2linear.h"

#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635

/******************************************
***                                     ***
***   l i n e a r 1 6 _ t o _ u l a w   ***
***                                     ***
*******************************************
Convert 16 bit linear sample to 8-bit ulaw sample.
*/
unsigned char linear16_to_ulaw(sample)
int sample;
{
  static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                             4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
  int sign, exponent, mantissa;
  unsigned char ulawbyte;

  /* Get the sample into sign-magnitude. */
  sign = (sample >> 8) & 0x80;		/* set aside the sign */
  if (sign) sample = -sample;		/* get magnitude */
  if (sample > CLIP) sample = CLIP;	/* clip the magnitude */

  /* Convert from 16 bit linear to ulaw. */
  sample = sample + BIAS;
  exponent = exp_lut[( sample >> 7 ) & 0xFF];
  mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
  ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
#ifdef ZEROTRAP
  if (ulawbyte == 0) ulawbyte = 0x02;	/* optional CCITT trap */
#endif

  return ulawbyte;
}


/******************************************
***                                     ***
***   u l a w _ t o _ l i n e a r 1 6   ***
***                                     ***
*******************************************
Convert input of 8-bit ulaw to signed 16-bit linear sample.
*/
short ulaw_to_linear16( ulawbyte )
unsigned char ulawbyte;
{
  static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
  int sign, exponent, mantissa, sample;

  ulawbyte = ~ ulawbyte;
  sign = ( ulawbyte & 0x80 );
  exponent = ( ulawbyte >> 4 ) & 0x07;
  mantissa = ulawbyte & 0x0F;
  sample = exp_lut[exponent] + ( mantissa << ( exponent + 3 ) );
  if (sign) sample = -sample;

  return sample;
}


/*
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/


/*
** This macro converts from ulaw to 16 bit linear, faster.
**
** Jef Poskanzer
** 23 October 1989
**
** Input: 8 bit ulaw sample
** Output: signed 16 bit linear sample
*/

short u2l16_table[256] = {
/*   0 */    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
/*   8 */    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
/*  16 */    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
/*  24 */    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
/*  32 */     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
/*  40 */     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
/*  48 */     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
/*  56 */     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
/*  64 */     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
/*  72 */     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
/*  80 */      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
/*  88 */      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
/*  96 */      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
/* 104 */      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
/* 112 */      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
/* 120 */       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
/* 128 */     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
/* 136 */     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
/* 144 */     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
/* 152 */     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
/* 160 */      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
/* 168 */      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
/* 176 */      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
/* 184 */      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
/* 192 */      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
/* 200 */      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
/* 208 */       876,    844,    812,    780,    748,    716,    684,    652,
/* 216 */       620,    588,    556,    524,    492,    460,    428,    396,
/* 224 */       372,    356,    340,    324,    308,    292,    276,    260,
/* 232 */       244,    228,    212,    196,    180,    164,    148,    132,
/* 240 */       120,    112,    104,     96,     88,     80,     72,     64,
/* 248 */	 56,     48,     40,     32,     24,     16,      8,      0 };

