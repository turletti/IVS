/**************************************************************************\
*          Copyright (c) 1992 by Stichting Mathematisch Centrum,           *
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
*                                                                          *
\**************************************************************************/
#ifdef INDIGO

#include <stdio.h>
#include <audio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include "ulaw2linear.h"

#define QSIZE 4000		/* 1 second of sound */

static int recording;		/* Nonzero if we are recording */

static ALport rport, wport;	/* ports for reading, writing */


/*  SOUNDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  */

int soundinit(iomode)
	int iomode;
{
	ALconfig config;
	long pvbuf[4];
	if (rport != NULL || wport != NULL) {
		fprintf(stderr, "soundinit: called again\n");
		return 0;
	}
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
		fprintf(stderr, "soundplay: soundinit(O_WRONLY) not called\n");
		return;
	}
	while (len > 0) {
		n = len > QSIZE ? QSIZE : len;
		for (i = 0; i < n; i++)
			sbuf[i] = audio_u2s(buf[i]);
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
	/* XXX Should do a logarithmic translation */
	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[1] = value * 255 / 100;
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
	pvbuf[3] = value * 255 / 100;
	ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
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
		buf[i] = audio_s2u(sbuf[i]);
	return n;
}


void sounddest(where)
  int where;
{
  /* Possible ? */
  ;
}


/* SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */

long soundplayqueue()
{
  return ALgetfilled(wport.port) / wport.channels;
}


#endif /* INDIGO */
