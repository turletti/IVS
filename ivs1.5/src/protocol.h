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
*  File    : protocol.h                                                    *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Header file for videoconf tool.                          *
*                                                                          *
\**************************************************************************/

#define VIDEOCONF_VERSION    "IVS (1.9)   INRIA Videoconference System"
#define PORT_VIDEO_CONTROL   3333
#define DEFAULT_TTL             4
#define N_MAX_STATIONS         10

#define BOOLEAN     unsigned char
#ifndef TRUE
#define TRUE                    1
#endif TRUE
#ifndef FALSE                   
#define FALSE                   0
#endif FALSE

#define P_DECLARATION           0
#define P_ASK_INFO              1
#define P_ABORT                 2

#define PP_QUIT                 0
#define PP_NEW_TABLE            1

#define VIDEO_TYPE              0
#define AUDIO_TYPE              2
#define CONTROL_TYPE            3

#define ABORT_AUDIO             1
#define UNMAPP_AUDIO            2
#define MAPP_AUDIO              4
#define ABORT_VIDEO             8
#define UNMAPP_VIDEO           16
#define MAPP_VIDEO             32
#define NEW_STATION            64
#define KILL_STATION          128

#define PCM_64                  0
#define PCM_32                  1
#define ADPCM_32                2
#define VADPCM                  3

#define P_MAX               10000


typedef struct
{
  char name[100];                         /* name@hostname                */
  u_long  sin_addr;                       /* Internet address             */
  BOOLEAN video_encoding, audio_encoding; /* encoder's parameters         */
  BOOLEAN video_decoding, audio_decoding; /* choice of decoding           */
  BOOLEAN valid;
  int     lasttime;                       /* last Packet Declaration time */
} STATION;

typedef STATION LOCAL_TABLE[N_MAX_STATIONS];


static char DEF_AUDIO_PORT[] =   "2233";
static char DEF_VIDEO_PORT[] =   "2244";
static char DEF_CONTROL_PORT[] = "2255";
static char DEF_IP_GROUP[] =     "224.8.8.8";

static char AUDIO_REPLAY_PORT[] = "2233";
static char VIDEO_REPLAY_PORT[] = "2244";
