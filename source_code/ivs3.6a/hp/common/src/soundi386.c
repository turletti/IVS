/**************************************************************************\
*                                                                          *
*  File              : soundi386.c                                         *
*  Author            : Vesa Ruokonen, LUT, Finland                         *
*  Last Modification : 1995/2/1595                                         *
*--------------------------------------------------------------------------*
*  Description       : Sound interface for Linux/x86 (VoxWare driver)      *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name           |    Date   |          Modification                *
*--------------------------------------------------------------------------*
\**************************************************************************/
  
#if !defined(NO_AUDIO) && defined(I386AUDIO)

/*
 * As the audio driver doesn't support multiple open fd's at same time,
 * we close old & open new fd every time play/record changes.
 * (Jan 1995/Linux 1.1.78/VoxWare 3.0alpha)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include "general_types.h"

#ifndef _PATH_AUDIO
#define _PATH_AUDIO "/dev/audio"
#endif
#ifndef _PATH_MIXER
#define _PATH_MIXER "/dev/mixer"
#endif

extern BOOLEAN DEBUG_MODE;

static int afd = -1;          /* audio play fd */
static int recording = FALSE; /* audio recording ON/OFF */

static int selected = TRUE;   /* defaults to play mode (FALSE for rec) */

/*  SOUNDRECORDINIT  --  Open the sound peripheral and initialise for
                         access.  Return TRUE if successful, FALSE
                         otherwise.  It's only for record */
int soundrecordinit()
{
      return (afd == -1 ? FALSE : TRUE);
}

/*  SOUNDINIT  --  Open the sound peripheral and initialise for
                   access.  Return TRUE if successful, FALSE
                   otherwise. */
int soundinit(mode)
int mode;
{
      if ((afd = open(_PATH_AUDIO, O_WRONLY|O_NDELAY)) != -1)
              return TRUE;
#ifdef DEBUG    
      perror(_PATH_AUDIO);
#endif
      return FALSE;
}

/*  ISSOUNDINIT  --  Return TRUE if audio is opened. */
int issoundinit()
{
      return (afd == -1 ? FALSE : TRUE);
}

/*  ISSOUNDRECORDINIT  --  Return TRUE if record audio is opened. */
int issoundrecordinit()
{
      return (afd == -1 ? FALSE : TRUE);
}

/*  SOUNDRECORDTERM  --  Close the record sound device. */
void soundrecordterm()
{
}

/*  SOUNDPLAYTERM  --  Close the play sound device. */
void soundplayterm()
{
      if (afd != -1) {
              if (close(afd)) {
#ifdef DEBUG    
                      perror(_PATH_AUDIO);
#endif
              }
              afd = -1;
      }
}

/*  SOUNDSTOPRECORD  --  Begin recording of sound. */
int soundstoprecord()
{
      recording = FALSE;
      return recording;
}

/* We are limited to single open fd per audio device,
   and to single IO mode too. So let's swap fd when necessary */
void selectplay(play)
int play;     /* TRUE for PLAY, FALSE for RECORD */
{
      if (play == selected)           /* already have right fd */
              return;
      if (close(afd) == -1) {
#ifdef DEBUG    
              perror(_PATH_AUDIO);
#endif
      }
      afd = open(_PATH_AUDIO, (play ? O_WRONLY : O_RDONLY)|O_NDELAY);
      if (afd == -1) {
#ifdef DEBUG    
              perror(_PATH_AUDIO);
#endif
      }
      selected = play;
}
              
/*  SOUNDRECORD  --  Begin recording of sound. */
int soundrecord()
{
      selectplay(FALSE);
      if(afd == -1){
#ifdef DEBUG
              fprintf(stderr, "soundrecord: no fd\n");
#endif
              recording = FALSE;
      } else
              recording = TRUE;
      return recording;
}

/*  SOUNDPLAY  --  Begin playing a sound. */
void soundplay(len, buf)
int len;
char *buf;
{
      selectplay(TRUE);
      if (afd == -1) {
#ifdef DEBUG
              fprintf(stderr, "soundplay: afd = %d\n", afd);
#endif
              return;
      }
      while(len) {
              int rlen;
              if ((rlen = write(afd, buf, len)) == -1) {
#ifdef DEBUG    
                      perror(_PATH_AUDIO);
#endif
                      usleep(10000);
              } else {
                      buf += rlen;
                      len -= rlen;
              }
      }
}

/*  SOUNDGRAB  --  Return audio information in the record queue. */
int soundgrab(buf, len)
char *buf;
int len;
{
      if (recording) {
              len = read(afd, buf, len);
              return len;
      }
      return 0;
}

/* Return FALSE on success, TRUE if something failed. */
int setmixer(req, value)
int req;      /* ioctl request type */
int value;    /* new value for setting */
{
      int mfd, val = value | (value << 8);

      if ((mfd = open(_PATH_MIXER, O_WRONLY|O_NDELAY)) == -1) {
#ifdef DEBUG    
              perror(_PATH_MIXER);
#endif
              return TRUE;
      }
      if (ioctl(mfd, req, &val) == -1) {
#ifdef DEBUG    
              perror(_PATH_MIXER);
#endif
              return TRUE;
      }
      if (close(mfd)) {
#ifdef DEBUG
              perror(_PATH_MIXER);
#endif
              return TRUE;
      }
      return FALSE;
}

/*  SOUNDPLAYVOL  --  Set playback volume from 0 (silence) to 100 (full on). */
void soundplayvol(value)
int value;
{
      if (setmixer(SOUND_MIXER_WRITE_VOLUME, value) && DEBUG_MODE)
              fprintf(stderr, "Audio volume setting failed.\n");
      return;
}

/*  SOUNDRECGAIN  --  Set recording gain from 0 (minimum) to 100 (maximum). */
void soundrecgain(value)
int value;
{
      if (setmixer(SOUND_MIXER_WRITE_RECLEV, value) && DEBUG_MODE)
              fprintf(stderr, "Audio rec setting failed.\n");
      /* Probably safest to set these both */
      if (setmixer(SOUND_MIXER_WRITE_MIC, value) && DEBUG_MODE)
              fprintf(stderr, "Audio mic setting failed.\n");
        return;
}

/*  SOUNDDEST  --  Set destination for generated sound. */
void sounddest(dest)
int dest;
{
      /* It's fun to click the icon, but nothing happens here :) */
      return;
}

/*  SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */
long soundplayqueue()
{
      return FALSE;
}

#endif /* !NO_AUDIO && I386AUDIO */
