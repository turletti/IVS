/**************************************************************************\
*         Copyright 1993 by       markus@prz.tu-berlin.dbp.de              *
*                                                                          *
*                        All Rights Reserved                               *
*                                                                          *
* Permission to use, copy, modify, and distribute this software and its    *
* documentation for any purpose and without fee is hereby granted,         *
* provided that the above copyright notice appear in all copies and that   *
* both that copyright notice and this permission notice appear in          *
* supporting documentation, and that the names of Technische Universitaet  * 
* Berlin or TUB not be used in advertising or publicity pertaining to      *
* distribution of the software without specific, written prior permission. *
*                                                                          *
* TECHNISCHE UNIVERSITAET BERLIN DISCLAIMS ALL WARRANTIES WITH REGARD TO   *
* THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND   *
* FITNESS, IN NO EVENT SHALL TECHNISCHE UNIVERSITAET BERLIN BE LIABLE      *
* FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES        *
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN    *
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT     *
* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.        *
*                                                                          *
\**************************************************************************/
/**************************************************************************\
* 	                						   *
*  File              : soundhpux.c     	                		   *
*  Author            : markus@prz.tu-berlin.dbp.de                         *
*  Last Modification : January 1993                                        *
*--------------------------------------------------------------------------*
*  Description       : Sound interface for HP station                      *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#ifdef HPUX 

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <audio/Alib.h>


#define TRUE  1
#define FALSE 0


/*  Local variables  */
static    long           status;
static    Audio          *playAudio;
static    int            playStreamSocket;
static    int            recordStreamSocket;
static    AErrorHandler  prevHandler;      /* pointer to previous handler */
static    int            recording = 0;    /* Nonzero if we are recording */

/*  Forward functions  */


/*
 * PRINTAUDIOERROR -- print audio error text given error code
 */
void printAudioError(au, message, errorCode)
    Audio    *au;
    char     *message;
    int      errorCode;
{
    char    errorbuff[132];

    AGetErrorText(au, errorCode, errorbuff, 131);
    fprintf(stderr, "%s: %s\n", message, errorbuff);

}


/*
 * MYHANDLER -- error handler for recorder
 */
long myHandler( audio, err_event)
    Audio  *audio;
    AErrorEvent  *err_event;
{
    printAudioError( audio, "Aarrgghh", err_event->error_code ); 

    exit(1);
}



/*
 * SOUNDPLAYVOL  --  Set playback volume from 0 (silence) to 100 (full on).
 */
void soundplayvol(value)
  int value;
{
#ifndef NOHPAUDIO
    AGainDB           gain;

    gain =((value*(AMaxOutputGain(playAudio)-AMinOutputGain(playAudio))
           /100)+AMinOutputGain(playAudio));

    ASetSystemPlayGain(playAudio, gain, (long *)NULL);
#endif
}



/*
 * SOUNDRECGAIN  --  Set recording gain from 0 (minimum) to 100 (maximum).  
 */
void soundrecgain(value)
  int value;
{
#ifndef NOHPAUDIO
    AGainDB           gain;

    gain =((value*(AMaxOutputGain(playAudio)-AMinOutputGain(playAudio))
           /100)+AMinOutputGain(playAudio));

    ASetSystemRecordGain(playAudio, gain, (long *)NULL);
#endif
}



/*
 * SOUNDDEST  --  Set destination for generated sound.  If "where"
 *                is 0, sound goes to the built-in speaker; if
 *                1, to the audio output jack.
 */
void sounddest(where)
  int where;
{
  /* more to implement here */
  if (where == 0){
    fprintf(stderr, "Tried to switch to internal Speaker\n");
  }
  else{
    fprintf(stderr, "Tried to switch to external Speaker\n");
  }
}


/*
 * SOUNDPLAY  --  Begin playing a sound.  
 */
void soundplay(len, buf)
  int len;
  unsigned char *buf;
{
#ifndef NOHPAUDIO
    int len_written;
    
    while( len > 0 ) {
        if(( len_written = write( playStreamSocket, buf, len )) < 0 ) {
            perror( "write failed" );
            exit(1);
        }
        buf += len_written;
        len -= len_written;
    }
#endif
}


/*
 * SOUNDGRAB  --  Return audio information in the record queue.  
 */
int soundgrab(buf, len)
    char *buf;
    int len;
{
#ifndef NOHPAUDIO
    fd_set          selectedFDs;        /* file descriptor for select */
    fd_set          readyFDs;           /* FDs ready for IO from select */
    int numRead;
    int numReady;

    if (recording) {
        len = read( recordStreamSocket, buf, len );
        return(len);
    }else{
        return(0);
    }
#else
    return(0);
#endif
}

/*
 * SOUNDRECORD  --  Begin recording of sound.
 */
int soundrecord()
{
#ifndef NOHPAUDIO
    recording = 1;
    return TRUE;
#endif
}


/* 
 * AUDIOINIT -- Initialize HP audio server 
 */
int audioInit(audioServer)
char *audioServer;
{
    AudioAttributes    recStreamAttribs;
    AudioAttributes    playStreamAttribs;
    SSRecordParams     recordStreamParams;
    SSPlayParams       playStreamParams;
    static SStream     recordAudioStream;
    static SStream     playAudioStream;
    static ATransID    rxid;
    static ATransID    pxid;
    AGainEntry         gainEntry;
    unsigned long      sampleRate;
    unsigned long      bitsPerSample;
    char               *pSpeaker;

    bitsPerSample      = 8;
    sampleRate         = 8000;

#ifdef NOHPAUDIO
  return(FALSE);
#endif
    /* open audio connection */
    playAudio = AOpenAudio( audioServer, &status );
    if( status ) {
        printAudioError( playAudio, "Open audio failed", status );
        return(FALSE);
    }

#ifdef DEBUG

    fprintf(stderr,"\nAudio Server Information:\n");
    fprintf(stderr,"\tvendor name           : %s\n",AServerVendor(playAudio));
    fprintf(stderr,"\tvendor release number : %d\n",AVendorRelease(playAudio));
    fprintf(stderr,"\tprotocol revision     : %ld\n",AProtocolRevision(playAudio));
    fprintf(stderr,"\tprotocol version      : %ld\n\n",AProtocolVersion(playAudio));
#endif
    
    /* replace default error handler */
    prevHandler = ASetErrorHandler(myHandler);

    /* set gain */
    gainEntry.u.i.in_ch    = AICTMono;
    gainEntry.u.i.in_src   = AISTMonoMicrophone;
    gainEntry.gain         = AUnityGain;

    /* setup the playback parameters */
    pSpeaker = getenv( "SPEAKER" );         /* get user speaker preference */

    if ( pSpeaker ) {
        if ( (*pSpeaker == 'e') || (*pSpeaker == 'E') ) {
	    gainEntry.u.o.out_dst = AODTMonoJack;
        } else {
            gainEntry.u.o.out_dst = AODTMonoIntSpeaker;
        }
    } else {
        /* SPEAKER environment variable not found - use internal speaker */  
        gainEntry.u.o.out_dst = AODTMonoIntSpeaker;
    }

    gainEntry.u.o.out_dst = AODTMonoJack;

    recStreamAttribs.type                                 = ATSampled;
    recStreamAttribs.attr.sampled_attr.sampling_rate      = sampleRate;
    recStreamAttribs.attr.sampled_attr.data_format        = ADFMuLaw;
    recStreamAttribs.attr.sampled_attr.bits_per_sample    = bitsPerSample;

    playStreamAttribs.type                                = ATSampled;
    playStreamAttribs.attr.sampled_attr.data_format       = ADFMuLaw;

    recordStreamParams.gain_matrix.type                   = AGMTInput ;        /* gain matrix */
    recordStreamParams.gain_matrix.num_entries            = 1;
    recordStreamParams.gain_matrix.gain_entries           = &gainEntry;
    recordStreamParams.record_gain                        = AUnityGain;        /* play volume */
    recordStreamParams.event_mask                         = 0;                 /* don't solicit events */
    recordStreamParams.pause_first                        = FALSE;

    playStreamParams.gain_matrix.type                     = AGMTOutput;        /* gain matrix */
    playStreamParams.gain_matrix.num_entries              = 1;
    playStreamParams.gain_matrix.gain_entries             = &gainEntry;
    playStreamParams.play_volume                          = AUnityGain;        /* play volume */
    playStreamParams.priority                             = APriorityNormal;   /* normal priority */
    playStreamParams.event_mask                           = 0;                 /* don't solicit events*/



    /*
     * create an audio play stream
     */
    pxid = APlaySStream( playAudio, ASDataFormatMask, &playStreamAttribs,
                        &playStreamParams, &playAudioStream, NULL );


    playAudioStream.max_block_size=256;

    /*
     * create an audio play stream socket
     */
    playStreamSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( playStreamSocket < 0 ) {
        perror( "Socket creation failed" );
        exit(1);
    }



    /*
     * connect the play stream socket to the audio stream port
     */
    status = connect( playStreamSocket,
                (struct sockaddr *) &playAudioStream.tcp_sockaddr,
                sizeof(struct sockaddr_in) );
    if( status < 0 ) {
        perror( "Connect failed" );
        exit(1);
    }

    /*
     * create an audio record stream
     */
    rxid = ARecordSStream( playAudio, ASDataFormatMask, &recStreamAttribs,
                        &recordStreamParams, &recordAudioStream, NULL );

    recordAudioStream.max_block_size=256;

    /*
     * create an audio record stream socket
     */
    recordStreamSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( recordStreamSocket < 0 ) {
        perror( "Socket creation failed" );
        exit(1);
    }


    /*
     * connect the record stream socket to the audio stream port
     */
    status = connect( recordStreamSocket, 
                (struct sockaddr *) &recordAudioStream.tcp_sockaddr,
                 sizeof(struct sockaddr_in) );
    if( status < 0 ) {
    perror( "Connect failed" );
    exit(1);
    }

    

    return (TRUE);
}

/* 
 * AUDIOTERM -- Close Audio Connection
 */

void audioTerm()
{
#ifndef NOHPAUDIO
    ACloseAudio( playAudio, &status );
#endif
}


/* SOUNDPLAYQUEUE  --  Return the number of samples in the play queue. */

long soundplayqueue()
{
  return 0;
}


#endif /* HPAUDIO */
