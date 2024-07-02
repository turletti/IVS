/**************************************************************************\
*          Copyright (c) 1995 by Stichting Mathematisch Centrum,           *
*                                Amsterdam, The Netherlands.               *
*                                                                          *
*                           All Rights Reserved                            *
*                                                                          *
* Permission to use, copy, modify, and distribute this software and its    *
* documentation for any purpose and without fee is hereby granted,         *
* provided that the above copyright notice appear in all copies and that   *
* both that copyright notice and this permission notice appear in          *
* supporting documentation, and that the names of Stichting Mathematisch   *
* Centrum or CWI not be used in advertising or publicity pertaining to     *
* distribution of the software without specific, written prior permission. *
*                                                                          *
* STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO   *
* THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND   *
* FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE      *
* FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES        *
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN    *
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT     *
* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.        *
*                                                                          *
\**************************************************************************/
/**************************************************************************\
* 	                						   *
*  File              : soundsgi.c                                          *
*  Author            : Guido van Rossum, CWI, Amsterdam.                   *
*  Last Modification : September 1992                                      *
*--------------------------------------------------------------------------*
*  Description       : Sound interface for SGI Indigo.                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Hugues CREPIN       |  94/5/30  | soundstoprecord(), soundrecordinit(),*
*                       |           | issoundrecordinit(),soundrecordterm()*
*                       |           | added.                               *
\**************************************************************************/
#if defined(SGISTATION) && !defined(NO_AUDIO)

#include <stdio.h>
#include <audio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <math.h>
#include "ulaw2linear.h"
#include "general_types.h"

#define QSIZE 4000		/* 1 second of sound */

static int recording=0;		/* Nonzero if we are recording */

static ALport rport, wport;	/* ports for reading, writing */


/*  SOUNDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  */

int soundinit(iomode)
	int iomode;
{
	ALconfig config;
	long pvbuf[4];
	if (iomode == O_RDONLY && rport != NULL)
	  return 1;
	if (iomode == O_WRONLY && wport != NULL)
	  return 1;
	config = ALnewconfig();
	if (config == NULL) {
		perror("soundinit: ALnewconfig");
		return 0;
	}
	ALsetchannels(config, AL_MONO);
	ALsetwidth(config, AL_SAMPLE_16);
	ALsetqueuesize(config, QSIZE);
	if (iomode == O_RDONLY) {
		rport = ALopenport("ivs/read", "r", config);
		if (rport == NULL) {
			perror("soundinit: ALopenport(r)");
			return 0;
		}
		/* Set the input sampling rate to 8000 Hz */
		pvbuf[0] = AL_INPUT_RATE;
		pvbuf[1] = AL_RATE_8000;
		ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 2L);
		return 1;
	}
	else if (iomode == O_WRONLY) {
		wport = ALopenport("ivs/write", "w", config);
		if (wport == NULL) {
			perror("soundinit: ALopenport(w)");
			return 0;
		}
		/* Set the output sampling rate to 8000 Hz */
		pvbuf[0] = AL_OUTPUT_RATE;
		pvbuf[1] = AL_RATE_8000;
		pvbuf[2] = AL_INPUT_SOURCE;
		pvbuf[3] = AL_INPUT_MIC;
		ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
		return 1;
	}
	else {
		fprintf(stderr, "soundinit: bad mode 0x%x\n", iomode);
		return 0;
	}
}

/*  SOUNDRECORDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  It's only for record*/


int soundrecordinit()
{
#ifndef NO_AUDIO
  return soundinit(O_RDONLY);
#endif
}

/*  ISSOUNDRECORDINIT  --  Return TRUE if record audio is opened. */


int issoundrecordinit()
{
  if(rport > (ALport)0)
    return(TRUE);
  else
    return(FALSE);
}

int issoundinit()
{
  return wport != NULL;
}


/* SOUNDDEVINIT -- Open just the control part. */

int sounddevinit()
{
	return 1;		/* Nothing we need to do right now */
}


/*  SOUNDTERM  --  Close the sound device.  */

void soundterm()
{
	if (rport) {
		ALcloseport(rport);
		rport = NULL;
	}
	if (wport) {
		ALcloseport(wport);
		wport = NULL;
	}
}


/*  SOUNDRECORDTERM  --  Close the sound record device.  */

void soundrecordterm()
{
  if (rport) {
    ALcloseport(rport);
    rport = NULL;
  }
}

/*  SOUNDPLAYTERM  --  Close the sound device.  */

void soundplayterm()
{
	if (wport) {
		ALcloseport(wport);
		wport = NULL;
	}
}

/*  SOUNDSTOPRECORD  --  Begin recording of sound.	*/

int soundstoprecord()
{
  recording = 0;
  return 0;
}

/*  SOUNDRECORD  --  Begin recording of sound.	*/

int soundrecord()
{
	recording = 1;
	return 1;
}


/*  SOUNDPLAY  --  Begin playing a sound.  */

void soundplay(len, buf)
	int len;
	unsigned char *buf;
{
	short sbuf[QSIZE], *p;
	int i, n;
	if (wport == NULL) {
		fprintf(stderr, 
			"soundplay: soundinit(O_WRONLY) not called\n");
		return;
	}
	while (len > 0) {
		n = len > QSIZE ? QSIZE : len;
		for (i = 0; i < n; i++)
			sbuf[i] = ulaw_to_linear16(buf[i]);
		ALwritesamps(wport, sbuf, n);
		len -= n;
		buf += n;
	}
}


/*  SOUNDPLAYVOL  --  Set playback volume from 0 (silence) to 100 (full on). */

void soundplayvol(value)
	int value;
{
	long pvbuf[4];

	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[1] = (int)(exp((double)value/18.03)) - 1;
/*	pvbuf[1] = value * 255 / 100;*/
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
	pvbuf[3] = pvbuf[1];
/*	pvbuf[3] = value * 255 / 100;*/
	ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
}


/*  SOUNDPLAYGET  --  Get playback volume */

int soundplayget()
{
	long pvbuf[4];
	/* XXX Should do a logarithmic translation */
	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
	ALgetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
	return((int)pvbuf[1]);
}


/*  SOUNDRECGAIN  --  Set recording gain from 0 (minimum) to 100 (maximum).  */

void soundrecgain(value)
	int value;
{
	long pvbuf[4];
	pvbuf[0] = AL_LEFT_INPUT_ATTEN;
	pvbuf[1] = (100 - value) * 255 / 100;
	pvbuf[2] = AL_RIGHT_INPUT_ATTEN;
	pvbuf[3] = (100 - value) * 255 / 100;
	ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
}


/*  SOUNDGRAB  --  Return audio information in the record queue.  */

int soundgrab(buf, len)
	char *buf;
	int len;
{
	short sbuf[QSIZE];
	int i, n;
	if (rport == NULL) {
		fprintf(stderr, "soundgrab: soundinit(O_RDONLY) not called\n");
		return 0;
	}
	if (!recording) {
		fprintf(stderr, "soundgrab: not recording\n");
		return 0;
	}
	n = len;
	if (n > QSIZE)
		n = QSIZE;
	ALreadsamps(rport, sbuf, n);
	for (i = 0; i < n; i++)
		buf[i] = linear16_to_ulaw(sbuf[i]);
	return n;
}


/*  SOUNDDEST  --  Set destination for generated sound.  
    "where" is either A_SPEAKER or A_HEADPHONE or A_EXTERNAL.
*/
void sounddest(where)
  int where;
{
  /* Possible ? */
  ;
}


/* SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */

long soundplayqueue()
{
  return ALgetfilled(wport);
}


usleep(int x)
{
    sginap(x/10000);
}

#endif /* SGISTATION && ! NO_AUDIO*/
