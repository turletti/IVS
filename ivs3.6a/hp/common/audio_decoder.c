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
*                                                                          *
*  File              : audio_decoder.c                                     *
*  Author            : Thierry Turletti                                    *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  Audio decoder.                                           *
*                                                                          *
\**************************************************************************/

#ifndef NO_AUDIO

#include <stdio.h>
#include <sys/types.h>

#include "general_types.h"
#include "protocol.h"
#include "rtp.h"


void PlayAudio(inbuf, soundel, format)
     unsigned char *inbuf;
     int soundel;
     u_char format;
{
    u_char *val;
    u_char outbuf[P_MAX];
    int len;

    switch(format){
    case RTP_PCM_CONTENT:
      val=inbuf;
      len=soundel;
      break;
    case RTP_ADPCM_CONTENT:
      len = soundel*2;
      adpcm_decoder(inbuf, outbuf, len);
      val = outbuf;
      break;
    case RTP_VADPCM_CONTENT:
      len=vadpcm_decoder(inbuf, outbuf, soundel);
      val = outbuf;
      break;
    default:
      return;
    }
    soundplay(len, val);
    return;
  }
#endif /* NO_AUDIO */

