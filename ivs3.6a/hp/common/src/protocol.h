/**************************************************************************\
*          Copyright (c) 1995 INRIA Sophia Antipolis, FRANCE.              *
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
*  File    : protocol.h       	                			   *
*  Date    : 1995/3/9		           				   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description :  Header file for H.261 coder and decoder.                 *
*                                                                          *
\**************************************************************************/
#ifdef SUNVIDEO
#undef NULL
#include <xil/xil.h>
#undef NULL
#define NULL 0
#endif
#ifdef VIDEOPIX
#include <vfc_lib.h>
#endif
#ifdef VIGRAPIX
#include "vigrapix.h"
#endif 
#include <sys/types.h>
#include <netinet/in.h>

#ifndef EXTERN
#ifdef DATA_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif
#endif



#define IVS_VERSION    "IVS 3.6a INRIA Videoconferencing System "
#define RTP_IVS_VERSION      0x33
#define IVS_CLIP_VERSION     "IVS CLIP 3.3v"

#define N_MAX_STATIONS        200
#define LEN_MAX_NAME          218

#define MC_PERCENTCONGESTEDTHRESH  10

/* between a video_decoder and a video_coder */

#define VIDEO_TYPE              0
#define AUDIO_TYPE              2
#define CONTROL_TYPE            3

#define PCM_64                  0
#define ADPCM_6                 1
#define ADPCM_5                 2
#define ADPCM_4                 3
#define ADPCM_3                 4
#define ADPCM_2                 5
#define VADPCM                  6
#define LPC                     7
#define GSM_13                  8
#define NONE                    9

#define CIF4_SIZE               0
#define CIF_SIZE                1
#define QCIF_SIZE               2

#define P_MAX                2000 /* Max is 1000 for video */

#define PIP_QUIT                0  /* The process must kill himself */
#define PIP_SIZE                1  /* New size (CIF4_SIZE or CIF_SIZE or 
				      QCIF_SIZE) */
#define PIP_FREEZE              2  /* Freeze the image */
#define PIP_UNFREEZE            3  /* Unfreeze the image */
#define PIP_VIDEO_PORT          4  /* New VFC/SUNVIDEO port */
#define PIP_MAX_RATE            5  /* Maximun bandwidth value */
#define PIP_LDISP               6  /* Local display */
#define PIP_NOLDISP             7  /* No local display */
#define PIP_TUNING              8  /* New tuning */
#define PIP_NEW_TABLE           9  /* New table sent to audio decoders */
#define PIP_FORK               10  /* Fork process to accept new talk (ivsd) */
#define PIP_FORMAT             11  /* New audio encoding format */
#define PIP_MIC_ENABLE         12  /* Enable the microphone */
#define PIP_MIC_DISABLE        13  /* Disable the microphone */
#define PIP_DROP               14  /* Drop the audio packets received */
#define PIP_CONT               15  /* Continue the audio decoding */
#define PIP_RATE_CONTROL       16  /* automatic, manual or without control */
#define PIP_PRIVILEGE_QUALITY  17  /* Privilege quality or frame-rate */
#define PIP_PRIVILEGE_FR       18  /* Privilege quality or frame-rate */
#define PIP_NACK_FIR_ALLOWED   19  /* Accept NACK/FIR packets */
#define PIP_CONF_SIZE          20  /* Conference size SMALL/MEDIUM/LARGE */
#define PIP_NB_STATIONS        21  /* Participants number */
#define PIP_MAX_FR_RATE        22  /* Frames/second value */
#define PIP_NO_MAX_FR_RATE     23  /* Frames/second value */
#define PIP_FRAME_RATE         24  /* Frames/second value */
#define PIP_PAUSE              25  /* Pause the ivs replay process */
#define PIP_ENCRYPTION         26  /* New encryption method/key */
#define PIP_VIDEO_FORMAT       27  /* New VIGRAPIX format */

#define FLOOR_BY_DEFAULT   0
#define FLOOR_ON_OFF       1
#define FLOOR_RATE_MIN_MAX 2


typedef  struct {
    int channel;
    int format;
    int standard;
    int squeeze;
    int mirror_x;
    int mirror_y;
} PX_PORT;

typedef struct
{
  char    name[LEN_MAX_NAME];      /* login or text describing the source */
  u_long  sin_addr;        /* packet has been received from this address  */
  u_long  sin_addr_coder;                         /* coder source address */
  u_short identifier;         /* source identifier, set to 0 if no bridge */
  u_short feedback_port;                          /* feedback port number */
  int32   lasttime;                       /* last Packet Declaration time */
  u_int8  video_encoding:1;           /* set if the station encodes video */
  u_int8  video_decoding:1;             /* set if video decoding selected */
  u_int8  valid:1;                       /* set if station is still alive */
  u_int8  reserved:5;
  int8    reserved2;
  int     dv_fd[2];          /* Pipes id between video decoder and father */
} STATION;

typedef STATION LOCAL_TABLE[N_MAX_STATIONS];


typedef struct
{
  int32 tv_sec;     /* last time audio packet received (second) */
  BOOLEAN flashed; /* True if the station is speaking          */
} AUDIO_STATION;

typedef AUDIO_STATION AUDIO_TABLE[N_MAX_STATIONS];

typedef struct
{
  u_int16 sec;         /* 16-bits seconds             */
  u_int16 usec;        /* 16 bits microseconds (<< 4) */
} TIMESTAMP;


typedef struct {
  u_int16 len;
  u_int16 delay;
} ivs_record_hdr;



typedef struct {
  int sock;
  struct sockaddr_in addr;
  int f2s_fd[2];
  int s2f_fd[2];
  char display[50];
  int rate_control;/* AUTOMATIC_CONTROL, MANUAL_CONTROL or WITHOUT_CONTROL */
  int rate_min; /* Minimal rate_max value */
  int rate_max; /* Maximum bandwidth allowed (kb/s) */
  int max_rate_max; /* Maximum rate_max value for the cursor */
  int max_frame_rate; /* Maximum nominal frame rate */
  int video_size; /* Image size: CIF_SIZE, CIF4_SIZE or QCIF_SIZE */
  int default_quantizer;
  int brightness; /* Current brightness encoded value */
  int contrast; /* Current contrast encoded value */
  int conf_size; /* Conference size: SMALL, MEDIUM or LARGE */
  int nb_stations; /* Current number of stations in the group */
  int video_port; /* Current input port video */
  int board_selected;
  int COLOR; /* True if color encoding */
  int FREEZE; /* True if image frozen */
  int LOCAL_DISPLAY;  /* True if the local image is displayed */
  int PRIVILEGE_QUALITY;
  int START_EVIDEO; /* True if video is currently encoded */
  int START_EAUDIO; /* True if audio is currently encoded */
  int MAX_FR_RATE_TUNED;
  int audio_format;
  int audio_redundancy;
  int floor_mode; /* FLOOR_BY_DEFAULT, FLOOR_ON_OFF or FLOOR_RATE_MIN_MAX */
  int memo_rate_max;   /* Maximum bandwidth allowed (kb/s) */
  char ni[256]; /* Network Interface Name */
  
#ifdef VIDEOPIX
  int vfc_format;
  VfcDev *vfc_dev;
#endif
#ifdef PARALLAX
  PX_PORT px_port_video;
  int *px_dev;
#endif
#ifdef SUNVIDEO
  XilSystemState xil_state;
#endif
#ifdef VIGRAPIX
  vigrapix_t vp;
  int secam_format;
#endif
#ifdef GALILEOVIDEO
  int pal_format;
#endif
#ifdef SCREENMACHINE
  int video_format;
  int video_field;
  int current_sm;
#endif
  int garbage;
} S_PARAM;


/* Default multicast address */
EXTERN char MULTI_IP_GROUP[]
#ifdef DATA_MODULE
=     "224.2.224.2"
#endif
;

/* Default port numbers in multicast mode */
EXTERN char MULTI_VIDEO_PORT[]
#ifdef DATA_MODULE
=   "2232"
#endif
;
EXTERN char MULTI_AUDIO_PORT[]
#ifdef DATA_MODULE
=   "2233"
#endif
;
EXTERN char MULTI_CONTROL_PORT[]
#ifdef DATA_MODULE
= "2234"
#endif
;

/* Port numbers in unicast mode */
EXTERN char UNI_VIDEO_PORT[]
#ifdef DATA_MODULE
=   "2235"
#endif
;
EXTERN char UNI_AUDIO_PORT[]
#ifdef DATA_MODULE
=   "2236"
#endif
;
EXTERN char UNI_CONTROL_PORT[]
#ifdef DATA_MODULE
= "2237"
#endif
;

EXTERN char VIDEO_REPLAY_PORT[]
#ifdef DATA_MODULE
= "2238"
#endif
;
EXTERN char AUDIO_REPLAY_PORT[]
#ifdef DATA_MODULE
= "2239"
#endif
;

/* Port number for video decoders to video coder messages */

#define PORT_VIDEO_CONTROL   2240


/* For control rate */

#define AUTOMATIC_CONTROL       0
#define MANUAL_CONTROL          1
#define WITHOUT_CONTROL         2


/* Different conference sizes */

#define LARGE_SIZE              0
#define MEDIUM_SIZE             1
#define SMALL_SIZE              3


/* For IVS DAEMON */

#define IVSD_PORT            2241
#define CALL_ACCEPTED           0
#define CALL_REFUSED            1
#define CALL_REQUESTED          2
#define CALL_ABORTED            3


#define NO_BOARD             0
#define PARALLAX_BOARD       1
#define SUNVIDEO_BOARD       2
#define VIDEOPIX_BOARD       3
#define INDIGO_BOARD         4
#define GALILEO_BOARD        5
#define INDY_BOARD           6
#define RASTEROPS_BOARD      7
#define VIDEOTX_BOARD        8
#define J300_BOARD           9
#define VIGRAPIX_BOARD      10
#define SCREENMACHINE_BOARD 11

#define A_SPEAKER    0x01
#define A_HEADPHONE  0x02
#define A_EXTERNAL   0x04

#define A_MICROPHONE 0x01
#define A_LINE_IN    0x02

