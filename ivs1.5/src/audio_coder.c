/**************************************************************************\
*          Copyright (c) 1992 INRIA Sophia Antipolis, FRANCE.              *
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
*  File    : audio_coder.c                                                 *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Audio coder.                                             *
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#ifdef DECSTATION
#include <Alib.h>
#endif 
#include "protocol.h"



void audio_encode(group, port, ttl, encoding, fd)
     char *group, *port;
     u_char ttl;
     int encoding;
     int fd[2];
{
  int sock, len_addr;
  struct sockaddr_in addr;
  int squelch = 40; 	/* Squelch level if > 0 */
  int len;
  unsigned char inbuf[P_MAX], outbuf[P_MAX];
  unsigned char *buf;
  int lbuf=1024;
#ifdef DECSTATION
  AudioServer    *aserver;

  LoudId          rootloudid;
  LoudId          cloud;
  VDeviceId       recorder;
  VDeviceId       mic;
  WireId          wire;
  SoundId         soundid;
  aLoudAttributes cloudAttrs;
  aVDeviceAttributes recorderAttrs;
  aVDeviceAttributes micAttrs;
  aWireAttributes wireAttrs;
  aSoundHandleAttributes soundAttr;
  PortInfoStruct  sink1, source1;
  char           *soundname;
  aLoudAttributes loudAttrs;
  LoudValueMask   loudmask;
  VDeviceValueMask vdevicemask;
  WireValueMask   wiremask;
  SoundValueMask  soundmask;
  int length=125;  /* duration : 125 ms --> 1000 bytes */

#endif /* DECSTATION */ 
  
  sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
  outbuf[0] = 0x80;
  outbuf[1] = (u_char)encoding;

#ifdef SPARCSTATION
  if (!soundinit(O_RDONLY)) {
    fprintf(stderr, "audio_decode: Cannot open audio device.\n");
    exit(1);
  }

  if (soundrecord()) 
#elif DECSTATION
  /* initialize audio connection  */
  aserver = AOpenAudio(NULL);
  if(aserver == NULL){
    fprintf(stderr, "Cannot open audio device.\n");
    exit(-3);
  }
  strcpy(soundname, "dec_audio_file");
  rootloudid = AGetRootLoudId(aserver);

  /* create the cloud */
  loudmask = LoudEventMaskValue;
  cloudAttrs.eventmask = QueueNotifyMask;
  cloud = ACreateLoud(aserver, rootloudid, loudmask, &cloudAttrs, 
		      ACurrentTime);
  /* create the recorder as a child of 'cloud' */
  vdevicemask = VDeviceClassValue | VDeviceSinksValue;
  recorderAttrs.dclass = AA_CLASS_RECORDER;
  recorderAttrs.sinks = 1;
  recorderAttrs.sinkslist = &sink1;
  sink1.encoding = AA_8_BIT_ULAW;
  sink1.samplerate = 8000;
  sink1.samplesize = 8;
  recorder =
    ACreateVDevice(aserver, cloud, vdevicemask, &recorderAttrs, ACurrentTime);

  /* create  a mic(input) device as child of 'cloud' */
  vdevicemask = VDeviceClassValue | VDeviceSourcesValue;
  micAttrs.dclass = AA_CLASS_INPUT;
  micAttrs.sources = 1;
  micAttrs.sourceslist = &source1;
  source1.encoding = AA_8_BIT_ULAW;
  source1.samplerate = 8000;
  source1.samplesize = 8;
  mic = ACreateVDevice(aserver, cloud, vdevicemask,  &micAttrs, ACurrentTime);

  /* create a wire between the recorder sink and mic source */
  wiremask = (WireSinkVdIdValue |  WireSinkIndexValue |
              WireSourceVdIdValue | WireSourceIndexValue);
  wireAttrs.sinkvdid = recorder;
  wireAttrs.sinkindex = 0;
  wireAttrs.sourcevdid = mic;
  wireAttrs.sourceindex = 0;
  wire = ACreateWire(aserver, wiremask, &wireAttrs, ACurrentTime);

  /* map and activate the cloud */
  AMapLoud(aserver, cloud, ACurrentTime);

  /* Create a Sound Handle */
  soundmask = SoundModeValue | SoundLengthValue | SoundTypeValue;
  soundAttr.soundmode = soundHandleLoopBuffer | soundHandleRead |
                        soundHandleWrite;
  soundAttr.length = P_MAX;
  soundAttr.soundtype.sampleRate = 8000;
  soundAttr.soundtype.sampleSize = 8;
  soundid = ACreateSoundHandle(aserver, soundmask, &soundAttr, ACurrentTime);

#endif 
  {
    struct timeval tim;
    int mask, mask0;
    
    mask0 = 1 << fd[0];
    tim.tv_sec = 0;
    tim.tv_usec = 0;

    if(encoding == PCM_64){
      buf = &outbuf[2];
    }else{
      buf = inbuf;
    }

    do{
      int soundel;
      
      /* Get the sound ... */
#ifdef SPARCSTATION
      soundel = soundgrab(buf, lbuf);
#elif DECSTATION
      ARecord(aserver, cloud, recorder, soundid, 0, 0, length, 0, 
	      ACurrentTime);
      AFlush(aserver);
		    
#endif
      mask = mask0;
      if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim) > 0){
#ifdef DECSTATION
	ADeleteSoundHandle(aserver, soundid, ACurrentTime);
	/* Close the connection to the audio server */
	ACloseAudio(aserver);
#endif /* DECSTATION */
	exit(0);
      }
      if (soundel > 0) {
	register unsigned char *start = buf;
	register int i;
	int squelched = (squelch > 0);

	/* If entire buffer is less than squelch, ditch it. */
	if (squelch > 0) {
	  for (i=0; i<soundel; i++) {
	    if (((*start++ & 0x7F) ^ 0x7F) > squelch) {
	      squelched = 0;
	      break;
	    }
	  }
	}
	
	if (!squelched){
	  switch(encoding){
	  case PCM_64:
	    break;
	  case ADPCM_32:
	    adpcm_coder(inbuf, &outbuf[2], soundel);
	    soundel /= 2;
	    break;
	  case VADPCM:
	    soundel = vadpcm_coder(inbuf, &outbuf[2], soundel);
	    break;
	  }
	  if (sendto(sock, outbuf, soundel+2, 0, (struct sockaddr *) &addr, 
		     len_addr) < 0) {
	    perror("audio_coder: when sending datagram packet");
	  }
	}
      } 
    }while(1);
  }
}


