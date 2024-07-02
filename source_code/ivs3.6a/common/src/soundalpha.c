/**************************************************************************\
*          Copyright (c) Robert Olsson                                     *
*          Swedish University of Agricultural Sciences                     *
*          Uppsala, Sweden                                                 *
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
*  File              : sounddec.c    	                		   *
*  Author            : Robert Olsson                                       *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Sound interface for AF                              *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
* RO                    |  930801   | FindDefaultDevice() by Lance Berc    *
*                       |           | <berc@src.dec.com> added.            *
* RO                    |  931212   | Fixed bugs detected when using AF 3.0*
*   Hugues CREPIN       |  94/5/27  | soundstoprecord() added.             *
* RO                    |  940704   | Added soundplayqueue, soundrecordterm*
\**************************************************************************/

#if defined(AF_AUDIO) && !defined(NO_AUDIO)

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <AF/AFlib.h>
#include <sys/file.h>
#include <sys/limits.h>
#include <signal.h>
#include "general_types.h"
#include "protocol.h"

#define TRUE  1
#define FALSE 0

static int recording = 0;               /* Recording in progress */

/* convert data type to sample size.  Should this be in the AF include
   file somewhere for everyone to use?  See the type encodings in 'audio.h'
*/
int sample_sizes[] = {
	1,		/* MU255 */
	1,		/* ALAW */
	2,		/* Linear PCM, 16 bits, -1.0 <= x < 1.0 */
	2,		/* Linear PCM, 32 bits, -1.0 <= x < 1.0 */
	1,		/* G.721, 64Kbps to/from 32Kbps. */
	1,		/* G.723, 64Kbps to/from 32Kbps. */
	0
};


AC ac;
AFAudioConn *aud;
int srate;				/* sample rate, per second */
int nsamples;			/* number of samples */
int ssize;				/* sample size, in bytes */
AFSetACAttributes attributes;
int out_min_vol, out_max_vol; /* min resp max output volume for audio device */
int in_min_vol, in_max_vol; /* min resp max input volume for audio device */
ATime schedplay;              /* Next scheduled play */

/*-----------------------------------------------------------------------------
 * SOUNDPLAYVOL  --  Set playback volume from 0 (min) to 100 (max).
 */
void soundplayvol(value) int value;
{
  int af_vol;
  if( value > 100 ) value = 100;
  if( value < 0 ) value = 0;

  af_vol = value*(out_max_vol-out_min_vol)/100 + out_min_vol;
#if defined(DEBUG)  
  printf("Volume = %d dB\n", af_vol );
#endif
  AFSetOutputGain(ac, af_vol);
  AFFlush( ac );/*****/
}
/*---------------------------------------------------------------------------  
 * SOUNDPLAYGET  --  Get playback volume from 0 (silence) to 100 (full on). 
 */
int get_soundplayvol()
{
  int value;
  
  value = AFQueryOutputGain(ac, &out_min_vol, &out_max_vol);
  return value;
}


/*-----------------------------------------------------------------------------
 * SOUNDRECGAIN  --  Set recording gain from 0 (min) to 100 (max).  
 */
void soundrecgain(value) int value;
{
  int af_rec;
  if( value > 100 ) value = 100;
  if( value < 0 ) value = 0;

  af_rec = value*(in_max_vol-in_min_vol)/100 + in_min_vol;
#if defined(DEBUG)  
  printf("Inputgain = %d dB\n", af_rec );
#endif
  AFSetInputGain(ac, af_rec);
  AFFlush( ac );/*****/
}

/*-----------------------------------------------------------------------------
 * SOUNDDEST  --  SOUNDDEST  --  Set destination for generated sound.  
 *  "where" is either 0 (built-in speaker) or 1 (output jack).
 */
void sounddest(where) int where;
{
  AMask old, new, mask;

  if( where == 0 ) {
  AFDisableOutput(ac, 1, &old, &new);
  AFEnableOutput(ac, 2, &old, &new);
  AFFlush( aud );
  }
  else if( where == 1 ){
  AFDisableOutput(ac, 2, &old, &new);
  AFEnableOutput(ac, 1, &old, &new);
  AFFlush( aud );
  }
  else {
    printf("Illegal request in sounddest <%d> \n", where);
    return;
  }
}
/*-----------------------------------------------------------------------------
 * SOUNDPLAY  --  Begin playing a sound.  
 */
void soundplay(nbytes, buf) int nbytes; unsigned char *buf;
{

#define DELAY_PLAY 0.12                /* seconds delay to start play */
                           /* Should be dynamic and user configurable */

    ATime pt;
    float toffset = DELAY_PLAY;		

    pt = AFGetTime(ac); 

/*    printf("Diff pt-oldpt %d, schedplay = %u, pt = %u, Diff pt-schedplay %d,
 nybytes = %d\n", pt-oldpt, schedplay, pt, pt-schedplay, nbytes); 
    oldpt = pt;
*/

    if( pt > schedplay ){
#if defined(DEBUG)
      printf("***Resync \n"); 
#endif
      schedplay = pt + (toffset * srate) ; /* Need to sync ? */
    }
    nsamples = nbytes;
    AFPlaySamples(ac, schedplay, nbytes, buf);
    schedplay += nsamples;
}


/* SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */

int soundplayqueue()
{
  int t1;

    t1 = schedplay - AFGetTime(ac);
    if ( t1 < 0 ) t1 = 0;
    return t1;
}

/*-----------------------------------------------------------------------------
 * SOUNDGRAB  --  Get audio samples from the audio server.
 */
int soundgrab_block(buf, len) unsigned char *buf; int len;
{     
  static ATime t;
  ATime pt;
#define A_REC_DIFF 400

  if( recording ) {
    pt = AFGetTime(ac);
    if( pt-t > A_REC_DIFF ) {
#if defined(DEBUG)
      printf("Sync New Time is %u \n", pt); 
#endif
      t = pt; /* Need to sync ? */
    }
    AFRecordSamples( ac, t, len, buf, ABlock);
    t += len;
    return len;
  }
}

/*-----------------------------------------------------------------------------
 * SOUNDGRAB  --  Get audio samples from the audio server.
 */
int soundgrab(buf, len) unsigned char *buf; int len;
{     
  static ATime t;
  ATime pt;
#define A_REC_DIFF_NEW 1400

  if( recording ) {
    pt = AFGetTime(ac);
    if( pt-t > A_REC_DIFF_NEW ) {
#if defined(DEBUG)
      printf("Sync New Time is %u \n", pt); 
#endif
      t = pt; /* Need to sync ? */
    }
    pt = AFRecordSamples( ac, t, len, buf, ANoBlock);
    len = pt - t;
    t = pt;
    return len;
  }
}


/*  SOUNDSTOPRECORD  --  Begin recording of sound.	*/

int soundstoprecord()
{
  recording = 0;
  return FALSE;
}


/*-----------------------------------------------------------------------------
 * SOUNDRECORD  --  Begin recording of sound.
 */
int soundrecord()
{   
  recording = 1;
  return TRUE;
}


/*-----------------------------------------------------------------------------
 * SOUNDRECORDTERM  --  Endrecording of sound.
 */
int soundrecordterm()
{   
  recording = 0;
  return TRUE;
}

/*-----------------------------------------------------------------------------
 * AUDIOINIT -- Initialize AF audio server 
 */
int audioInit()
{
    int device, p1;
    int cflag = 0;
    AEncodeType	type;
    unsigned int channels;

#if defined(DEBUG)
    printf("Entering audioInit (AF - version)\n");
#endif

    attributes.preempt = Mix;
    attributes.start_timeout = 0;
    attributes.end_silence = 0;
    attributes.play_gain = 0;
    attributes.rec_gain =  0;

    if ( (aud = AFOpenAudioConn("")) == NULL) {
	fprintf(stderr, " can't open audio (AF) connection.\n");
	return(0);
    }

   device = FindDefaultDevice(aud);

    if (device == -1) {
	fprintf(stderr, "Couldn't find an 8kHz mono non-phone device\n");
	return(0);
    }


/* set up audio context, find sample size and sample rate */

    ac = AFCreateAC(aud, device, ACPlayGain, &attributes);
    srate = ac->device->playSampleFreq;
    type = ac->device->playBufType;
    channels = ac->device->playNchannels;
    ssize = sample_sizes[type] * channels;

/* Audio device min & max input/output volume just once */

    (void *) AFQueryOutputGain(ac, &out_min_vol, &out_max_vol);
    (void *) AFQueryInputGain(ac, &in_min_vol, &in_max_vol);
    set_soundrecgain(33);
    set_soundplayvol(50);
    return(1);
}

/*-----------------------------------------------------------------------------
 * AUDIOTERM -- Close Audio Connection *** NOT USED ***
 */

void audioTerm()
{
    AFFreeAC (ac);
    AFCloseAudioConn(ac);
    fprintf(stderr, "audioTerm is called\n");
}

/* sleep for fractions of a second.  Clock arg is in microseconds. */


usleep(msec_wait)
     int msec_wait;
{
  struct timeval wait_tv;

  wait_tv.tv_sec = msec_wait / 1000000;
  wait_tv.tv_usec = msec_wait % 1000000;
  select(0,NULL,NULL,NULL,&wait_tv);
}

/*-----------------------------------------------------------------------------
  Find a a suitable audio device for ivs. 
  Written by Lance Berc <berc@src.dec.com> 
*/
int FindDefaultDevice(aud)
    AFAudioConn *aud;
{
  AFDeviceDescriptor *aDev;
  int i;
  char *s;

  s = (char *) getenv("AF_DEVICE");
  if (s != NULL) return(atoi(s));

  /* Find the first non-phone, 8kHz mu-law, mono device */
  for(i=0; i < ANumberOfAudioDevices(aud); i++) {
    aDev = AAudioDeviceDescriptor(aud, i);
    if ((aDev->inputsFromPhone == 0) &&
        (aDev->outputsToPhone == 0) &&
        (aDev->playBufType == MU255) &&
        (aDev->playNchannels == 1))
      return i;
  }
  return -1;
}


#endif /* AF_AUDIO && ! NO_AUDIO */





