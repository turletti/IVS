#ifndef lint	/* BuildSystemHeader added automatically */
static char *BuildSystemHeader= "$Header: [playsoundname.c,v 1.4 91/12/26 12:57:30 grodnik Exp ]$";	/* BuildSystemHeader */
#endif		/* BuildSystemHeader */

/***************************************************************************
 *
 * Copyright (c) Digital Equipment Corporation, 1991, 1992 All Rights Reserved.
 * Unpublished rights reserved under the copyright laws of the United States.
 * The software contained on this media is proprietary to and embodies the
 * confidential technology of Digital Equipment Corporation.  Possession, use,
 * duplication or dissemination of the software and media is authorized only
 * pursuant to a valid written license from Digital Equipment Corporation.
 * RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by the U.S.
 * Government is subject to restrictions as set forth in 
 * Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as 
 * applicable.
 *
 ***************************************************************************/

#include <Alib.h>


/* 
 * CreatePlayer
 *
 * Creates a player and speaker virtual devices and connects them with a wire.
 * Return the device-id of the player.
 */
VDeviceId
CreatePlayer(aserver, cloud)
     AudioServer *aserver;
     LoudId       cloud;
{
  VDeviceId          player;
  VDeviceId          speaker;
  WireId             wire;
  aVDeviceAttributes playerAttrs;
  aVDeviceAttributes speakerAttrs;
  aWireAttributes    wireAttrs;
  PortInfoStruct     sink1, source1;
  VDeviceValueMask   vdevicemask;
  WireValueMask      wiremask;

  /* create the player as a child of 'cloud' */
  vdevicemask = VDeviceClassValue | VDeviceSourcesValue;
  /*| VDeviceEventMaskValue;*/
  /*  playerAttrs.eventmask = DeviceNotifyMask;*/
  playerAttrs.dclass = AA_CLASS_PLAYER;
  playerAttrs.sources = 1;
  playerAttrs.sourceslist = &source1;
  source1.encoding = AA_8_BIT_ULAW;
  source1.samplerate = 8000;
  source1.samplesize = 8;
  player = 
    ACreateVDevice(aserver, cloud, vdevicemask, &playerAttrs, ACurrentTime);
  /* create  a speaker(output) device as child of 'cloud' */

  vdevicemask = VDeviceClassValue | VDeviceSinksValue;
  speakerAttrs.dclass = AA_CLASS_OUTPUT;
  speakerAttrs.sinks = 1;
  speakerAttrs.sinkslist = &sink1;
  sink1.encoding = AA_8_BIT_ULAW;
  sink1.samplerate = 8000;
  sink1.samplesize = 8;
  speaker = 
    ACreateVDevice(aserver, cloud, vdevicemask,  &speakerAttrs, ACurrentTime);

  /* create a wire between the player sink and speaker source */

  wiremask = (WireSinkVdIdValue |  WireSinkIndexValue | 
	  WireSourceVdIdValue | WireSourceIndexValue);
  wireAttrs.sinkvdid = speaker;
  wireAttrs.sinkindex = 0;
  wireAttrs.sourcevdid = player;
  wireAttrs.sourceindex = 0;
  wire = ACreateWire(aserver, wiremask, &wireAttrs, ACurrentTime);
  return(player);
}


