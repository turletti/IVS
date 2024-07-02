#ifdef SPARCSTATION

/*

	NetFone: Sound interface functions

	Designed and implemented in July of 1991 by John Walker

*/

#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>

#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <sun/audioio.h>

#define SoundFile       "/dev/audio"
#define AUDIO_CTLDEV    "/dev/audioctl"

#define MAX_GAIN	100

#define TRUE  1
#define FALSE 0

#define V     (void)

/*  Local variables  */

static int audiof = -1; 	      /* Audio device file descriptor */
static int Audio_fd = -1;             /* Audio control port */
static audio_info_t Audio_info;       /* Current configuration info */
static int recording = FALSE;	      /* Recording in progress ? */

/*  Forward functions  */

/* Convert local gain into device parameters */

static unsigned scale_gain(g)
  unsigned g;
{
    return (AUDIO_MIN_GAIN + (unsigned)
	irint(((double) (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) *
	((double)g / (double)MAX_GAIN)));
}


/*  SOUNDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  */

int soundinit(iomode)
  int iomode;
{
    iomode |= O_NDELAY;
    if ((audiof = open(SoundFile, iomode)) >= 0) {
      if ((Audio_fd = open(AUDIO_CTLDEV, O_RDWR)) < 0) {
	perror(AUDIO_CTLDEV);
	return FALSE;
      }
      return TRUE;
    }
    perror("/dev/audio");
    return FALSE;
}



int sounddevinit()
{
    if ((Audio_fd = open(AUDIO_CTLDEV, O_RDWR)) < 0) {
      perror(AUDIO_CTLDEV);
      return FALSE;
    }
    return TRUE;
}


/*  SOUNDTERM  --  Close the sound device.  */

void soundterm()
{
	if (audiof >= 0) {
	   V close(audiof);
	   audiof = -1;
	}
}

/*  SOUNDRECORD  --  Begin recording of sound.	*/

int soundrecord()
{
    assert(!recording);
    recording = TRUE;
    return recording;
}

/*  SOUNDPLAY  --  Begin playing a sound.  */

void soundplay(len, buf)
  int len;
  unsigned char *buf;
{
    int ios;

    assert(audiof != -1);
    while (TRUE) {
	ios = write(audiof, buf, len);
	if (ios == -1) {
	    usleep(100000);
	} else {
	    if (ios < len) {
		buf += ios;
		len -= ios;
	    } else {
		break;
	    }
	}
    }
}

/*  SOUNDPLAYVOL  --  Set playback volume from 0 (silence) to 100 (full on). */

void soundplayvol(value)
  int value;
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.play.gain = scale_gain(value);
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Set play volume");
    }
}

/*  SOUNDRECGAIN  --  Set recording gain from 0 (minimum) to 100 (maximum).  */

void soundrecgain(value)
  int value;
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.record.gain = scale_gain(value);
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Set record gain");
    }
}

/*  SOUNDDEST  --  Set destination for generated sound.  If "where"
		   is 0, sound goes to the built-in speaker; if
		   1, to the audio output jack. */

void sounddest(where)
  int where;
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.play.port = (where == 0 ? AUDIO_SPEAKER : AUDIO_HEADPHONE);
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Set output port");
    }
}

/*  SOUNDGRAB  --  Return audio information in the record queue.  */

int soundgrab(buf, len)
    char *buf;
    int len;
{
    if (recording) {
	long read_size;

	if (ioctl(audiof, FIONREAD, &read_size) < 0) {
            perror("FIONREAD ioctl failed");
	} else {
	    if (read_size > len)
		read_size = len;
	    read(audiof, buf, read_size);
	    return read_size;
	}
    }
    return 0;
}
#endif /* SPARCSTATION */
