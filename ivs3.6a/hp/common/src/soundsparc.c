/**************************************************************************\
* 	                						   *
*  File              : soundsparc.c    	                		   *
*  Author            : John Walker                                         *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Sound interface for SPARC station                   *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*    Pierre Delamotte   |  92/12/4  | soundrecmute(), soundrecrestart(),   *
*                       |           | soundrecflush() added.               *
*   Thierry Turletti    |  94/1/27  | soundrecget() added.                 *
*   Hugues CREPIN       |  94/5/30  | soundstoprecord(), soundrecordinit(),*
*                       |           | issoundrecordinit(),soundrecordterm()*
*                       |           | added.                               *
*                       |           | soundgrab() modified.                *
\**************************************************************************/


#if defined(SPARCSTATION) && !defined(NO_AUDIO)

#ifdef	SOLARIS
#include <dirent.h>
#else
#include <sys/types.h>
#include <sys/dir.h>
#endif
#include <sys/file.h>

#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <sys/ioctl.h>
#ifndef __NetBSD__
#include <stropts.h>
#endif
#ifdef	SOLARIS
#include <sys/audioio.h>
#define irint(x)	((int)rint(x))
#else
#ifdef __NetBSD__
#include <machine/bsd_audioio.h>
#define irint(x)	((int)rint(x))
#else
#include <sun/audioio.h>
#endif
#endif
#ifdef  SOLARIS
#include <sys/filio.h>  /* FIONREAD */
#endif

#define SoundFile       "/dev/audio"
#ifndef __NetBSD__
#define AUDIO_CTLDEV    "/dev/audioctl"
#endif

#define MAX_GAIN	100

#define TRUE  1
#define FALSE 0

#define V     (void)

/*  Local variables  */

static int audiof = -1; 	      /* Audio device file descriptor */
#ifdef __NetBSD__
#define Audio_fd audiof		      /* Audio device & control are the same */
#else
static int audiofrec = -1; 	      /* Audio device file record descriptor */
static int Audio_fd = -1;             /* Audio control port */
#endif
static audio_info_t Audio_info;       /* Current configuration info */
static int recording = FALSE;	      /* Recording in progress ? */
#ifndef __NetBSD__
static long play_count;
#endif
void soundplayflush();

/*  Forward functions  */

/* Convert local gain into device parameters */

static unsigned scale_gain(g)
  int g;
{
  return (AUDIO_MIN_GAIN + (unsigned)
	  irint(((double) (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) *
		((double)g / (double)MAX_GAIN)));
}


static int unscale_gain(g)
     double g;
{
  return(irint((double) MAX_GAIN * g));
}

/*  SOUNDRECORDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  It's only for record*/


int soundrecordinit()
{
#ifdef __NetBSD__
    return soundinit(0);
#else
    if ((audiofrec = open(SoundFile, O_RDONLY|O_NDELAY)) >= 0) {
      return TRUE;
    }
#ifdef DEBUG    
    perror(SoundFile);
#endif
    return FALSE;
#endif /* __NetBSD__ */
}

/*  SOUNDINIT  --  Open the sound peripheral and initialise for
		   access.  Return TRUE if successful, FALSE
		   otherwise.  */

int soundinit(iomode)
  int iomode;
{
#ifdef __NetBSD__
    /* open only once */
    if (audiof >= 0)
      return TRUE;
    /* audio file descriptor will be reused for both record and control */
    if ((audiof = open(SoundFile, O_RDWR|O_NDELAY, 0)) >= 0)
      return TRUE;
#else
    iomode |= O_NDELAY;
    if ((audiof = open(SoundFile, iomode)) >= 0) {
      play_count = 0;
      return TRUE;
    }
#endif
#ifdef DEBUG    
    perror(SoundFile);
#endif
    return FALSE;
}


/*  ISSOUNDINIT  --  Return TRUE if audio is opened. */

int issoundinit()
{
#ifdef __NetBSD__
  if(audiof >= 0)
#else
  if(audiof > 0)
#endif
    return(TRUE);
  else
    return(FALSE);
}

/*  ISSOUNDRECORDINIT  --  Return TRUE if record audio is opened. */


int issoundrecordinit()
{
#ifdef __NetBSD__
  return issoundinit();
#else
  if(audiofrec > 0)
    return(TRUE);
  else
    return(FALSE);
#endif
}

int sounddevinit()
{
#ifdef __NetBSD__
    return soundinit(0);
#else
    if ((Audio_fd = open(AUDIO_CTLDEV, O_RDWR)) < 0) {
      perror(AUDIO_CTLDEV);
      return FALSE;
    }
    return TRUE;
#endif
}


/*  SOUNDTERM  --  Close the sound device.  */

void soundterm()
{
  if (audiof >= 0) {
    soundplayflush();
#ifndef __NetBSD__
    if(close(audiof) < 0)
      perror("close");
    audiof = -1;
#endif
  }
}

/*  SOUNDRECORDTERM  --  Close the sound record device.  */

void soundrecordterm()
{
#ifndef __NetBSD__
  if (audiofrec >= 0) {
    soundplayflush();
    if(close(audiofrec) < 0)
      perror("close");
    audiofrec = -1;
  }
#endif
}

/*  SOUNDPLAYTERM  --  Close the play sound device.  */

void soundplayterm()
{
  if (audiof >= 0) {
    soundplayflush();
#ifndef __NetBSD__
    if(close(audiof) < 0)
      perror("close");
    audiof = -1;
#endif
  }
}

/*  SOUNDSTOPRECORD  --  Begin recording of sound.	*/

int soundstoprecord()
{
  recording = FALSE;
  return recording;
}

/*  SOUNDRECORD  --  Begin recording of sound.	*/

int soundrecord()
{
  if(recording){
#ifndef DEBUG
    fprintf(stderr, "soundrecord: audiof = -1\n");
#endif
  }
  recording = TRUE;
  return recording;
}

/*  SOUNDPLAY  --  Begin playing a sound.  */

void soundplay(len, buf)
  int len;
  unsigned char *buf;
{
    int ios;

    if(audiof == -1){
#ifndef DEBUG
	fprintf(stderr, "soundplay: audiof = -1\n");
#endif
	return;
    }
    
    while (TRUE) {
#ifdef __NetBSD__
	ios = write(audiof, (char *)buf, len > 1000 ? 1000 : len);
	if (ios < 0) {
	    perror("write audio");
	    break;
#else
	ios = write(audiof, (char *)buf, len);
	if (ios == -1) {
	    usleep(100000);
#endif
	} else {
	    if (ios < len) {
		buf += ios;
		len -= ios;
	    } else {
		break;
	    }
	}
    }
#ifndef __NetBSD__
    play_count += len;
#endif
    return;
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

/*  SOUNDPLAYGET  --  Get playback volume from 0 (silence) to 100 (full on). */
 
int soundplayget()
{
  int value = 0;
  double aux;

  AUDIO_INITINFO(&Audio_info);

  if(ioctl(Audio_fd, AUDIO_GETINFO, &Audio_info) < 0){
    perror("get audio information");
  }else{
    aux = (double) (Audio_info.play.gain - AUDIO_MIN_GAIN) 
	/ (double) AUDIO_MAX_GAIN;
    value = unscale_gain(aux);
  }
  return value;
}



/*  SOUNDRECGET  --  Get record volume from 0 (silence) to 100 (full on). */
 
int soundrecget()
{
  int value = 0;

  AUDIO_INITINFO(&Audio_info);

  if(ioctl(Audio_fd, AUDIO_GETINFO, &Audio_info) < 0){
    perror("get audio information");
  }else
    value = unscale_gain ((double) (Audio_info.record.gain - AUDIO_MIN_GAIN)
			  / (double) AUDIO_MAX_GAIN);
  return value;
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



/*  SOUNDDEST  --  Set destination for generated sound.  
    "where" is either A_SPEAKER or A_HEADPHONE or A_EXTERNAL.
*/

void sounddest(where)
  int where;
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.play.port = where;
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Set output port");
    }
}


/*  SOUNDGETDEST  --  Get destination for generated sound.
*/
 
int soundgetdest()
{
  int value = 0;

  AUDIO_INITINFO(&Audio_info);
  if(ioctl(Audio_fd, AUDIO_GETINFO, &Audio_info) < 0){
    perror("get audio information");
  }else
    value = Audio_info.play.port;
  return value;
}


/*  SOUNDFROM  --  Set input for generated sound.  
    "where" is either A_MICROPHONE or A_LINE_IN.
*/

void soundfrom(where)
  int where;
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.record.port = where;
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Set input port");
    }
}


/*  SOUNDGETFROM  --  Get origine of the input sound.
*/
 
int soundgetfrom()
{
  int value = 0;

  AUDIO_INITINFO(&Audio_info);
  if(ioctl(Audio_fd, AUDIO_GETINFO, &Audio_info) < 0){
    perror("get audio information");
  }else
    value = Audio_info.record.port;
  return value;
}


/*  SOUNDGRAB  --  Return audio information in the record queue.  */

int soundgrab(buf, len)
    char *buf;
    int len;
{
    if (recording) {
#ifdef __NetBSD__
	int cc;
  
	if ((cc = read(audiof, buf, len)) >= 0)
	    return cc;
	if (errno != EWOULDBLOCK)
	    perror("AUDIO read failed");
	return 0;
#else
	long read_size;

	if (ioctl(audiofrec, FIONREAD, &read_size) < 0) {
            perror("FIONREAD ioctl failed");
	} else {
	    if (read_size > len)
		read_size = len;
	    read(audiofrec, buf, (int)read_size);
	    return read_size;
	}
#endif
    }
    return 0;
}


/*  SOUNDRECMUTE  --  Mute audio record.  */

void soundrecmute()
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.record.pause = TRUE ;
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Mute audio record");
    }
}



/*  SOUNDRECRESTART  --  Restart audio record.  */

void soundrecrestart()
{
    AUDIO_INITINFO(&Audio_info);
    Audio_info.record.pause = FALSE ;
    if (ioctl(Audio_fd, AUDIO_SETINFO, &Audio_info) < 0) {
        perror("Restart audio record");
    }
}



/*  SOUNDRECFLUSH  --  Flush audio record buffers.  */

void soundrecflush()
{
#ifdef __NetBSD__
    if (ioctl(audiof, AUDIO_FLUSH) < 0) {
#else
    if (ioctl(Audio_fd, I_FLUSH, FLUSHR) < 0) {
#endif
        perror("Flush record buffers");
    }
}


/*  SOUNDPLAYFLUSH  --  Flush audio play buffers.  */

void soundplayflush()
{
#ifdef __NetBSD__
    if (ioctl(audiof, AUDIO_FLUSH) < 0) {
#else
    if (ioctl(Audio_fd, I_FLUSH, FLUSHRW) < 0) {
#endif
        perror("Flush play buffers");
    }
}


/* SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */

long soundplayqueue()
{
#ifdef __NetBSD__
  u_long samples = 0;
  
  ioctl(audiof, AUDIO_WSEEK, (char *)&samples);
  return(samples);
#else
  audio_info_t audio_info;
  long samples;

  ioctl(Audio_fd, AUDIO_GETINFO, (char *)&audio_info);
  samples = audio_info.play.samples;
  if(audio_info.play.error){
    /* reset the error indicator */
    AUDIO_INITINFO(&audio_info);
    audio_info.play.error = 0;
    ioctl(Audio_fd, AUDIO_SETINFO, (char *)&audio_info);
    return (-1);
  }
  return(play_count - samples);
#endif
}


#endif /* SPARCSTATION && !defined(NO_AUDIO) */
