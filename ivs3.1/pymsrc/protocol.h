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
* 	                						   *
*  File    : protocol.h       	                			   *
*  Date    : May 1992		           				   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description :  Header file for H.261 coder and decoder.                 *
*                                                                          *
\**************************************************************************/

#define IVS_VERSION    "IVS (3.1) INRIA Videoconferencing System"
#define RTP_IVS_VERSION        0x30

#define DEFAULT_TTL            32
#define N_MAX_STATIONS        100
#define LEN_MAX_NAME           60

#define BOOLEAN     unsigned char
#ifndef TRUE
#define TRUE                    1
#endif /* TRUE */
#ifndef FALSE                   
#define FALSE                   0
#endif /* FALSE */

/* between a video_decoder and a video_coder */

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

#define CIF4_SIZE               0
#define CIF_SIZE                1
#define QCIF_SIZE               2

#define P_MAX               10000

#define PIP_QUIT                0  /* The process must kill himself */
#define PIP_SIZE                1  /* New size (CIF4_SIZE or CIF_SIZE or 
				      QCIF_SIZE) */
#define PIP_FREEZE              2  /* Freeze the image */
#define PIP_UNFREEZE            3  /* Unfreeze the image */
#define PIP_WCONTROL            4  /* Without any rate control */
#define PIP_RCONTROL            5  /* Control refresh */
#define PIP_QCONTROL            6  /* Control the quality */
#define PIP_VFC_PORT            7  /* New VFC port */
#define PIP_MAX_RATE            8  /* Maximun bandwidth value */
#define PIP_LDISP               9  /* Local display */
#define PIP_NOLDISP            10  /* No local display */
#define PIP_TUNING             11  /* New tuning */
#define PIP_FEEDBACK           12  /* Feedback is allowed from video decoders*/
#define PIP_NOFEEDBACK         13  /* Feedback is not allowed */
#define PIP_NEW_TABLE          14  /* New table sent to audio decoders */
#define PIP_FORK               15  /* Fork process to accept new talk (ivsd) */
#define PIP_FORMAT             16  /* New audio encoding format */
#define PIP_MIC_ENABLE         17  /* Enable the microphone */
#define PIP_MIC_DISABLE        18  /* Disable the microphone */
#define PIP_DROP               19  /* Drop the audio packets received */
#define PIP_CONT               20  /* Continue the audio decoding */

typedef struct
{
  char    name[LEN_MAX_NAME];             /* name@hostname                */
  u_long  sin_addr;                       /* Internet address             */
  BOOLEAN video_encoding, audio_encoding; /* encoder's parameters         */
  BOOLEAN video_decoding, audio_decoding; /* choice of decoding           */
  BOOLEAN valid;
  int     lasttime;                       /* last Packet Declaration time */
  u_short feedback_port;                  /* Feedback port number         */
} STATION;

typedef STATION LOCAL_TABLE[N_MAX_STATIONS];

typedef struct
{
  u_long  sin_addr;                  /* Internet address                */
  int audio_decoding;                /* TRUE if audio decoding selected */
} AUDIO_STATION;

typedef AUDIO_STATION AUDIO_TABLE[N_MAX_STATIONS];

/* Default port numbers in multicast mode */
static char MULTI_AUDIO_PORT[] =   "2233";
static char MULTI_VIDEO_PORT[] =   "2244";
static char MULTI_CONTROL_PORT[] = "2255";

/* Default multicast address */
static char MULTI_IP_GROUP[] =     "224.8.8.8";

/* Port numbers in unicast mode */
static char UNI_AUDIO_PORT[] =   "2232";
static char UNI_VIDEO_PORT[] =   "2243";
static char UNI_CONTROL_PORT[] = "2254";

/* Port number for video decoders to video coder messages */
#define PORT_VIDEO_CONTROL   2230

static char AUDIO_REPLAY_PORT[] = "2231";
static char VIDEO_REPLAY_PORT[] = "2242";


/* For control rate */

#define WITHOUT_CONTROL         0
#define REFRESH_CONTROL         1
#define QUALITY_CONTROL         2


/* For IVS DAEMON */

#define IVSD_PORT            2241
#define CALL_ACCEPTED           0
#define CALL_REFUSED            1
#define CALL_REQUEST            2
#define CALL_ABORT              3



/*
*   The RTP Header...


  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Ver| flow      |0|S|  content  | sequence number               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp (seconds)           | timestamp (fraction)          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |S|Sbit |E|Ebit |C|  F  |I| MBZ |         H.261 stream...       :
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

With:

Ver:  (2 bits) RTP version number. [01]
flow: (6 bits) flow number. [000000]
S:(1 bit) Synchronized bit: [1] if it is the first packet of a GOB.
                              Note that the GBSC is not encoded and 
			      must be appended before decoding.
			    [0] else.
content: (6 bits) H.261 type [011111]
sequence number: (16 bits)
timestamp: (32 bits)
S: (1 bit) [1] if beginning of GOB; [0] else.
Sbit: (3 bits) Indicates the number of bits that should be ignored in the
               first (start) data octet. (Here always 0...)
E: (1 bit) [1] if end of GOB; [0] else.
Ebit: (3 bits) Indicates the number of bits that should be ignored in the 
               last (end) data octet.
C: (1 bit) [1] if color encoded; [0] else.
F: (3 bits) Image format:
            [000] QCIF; [001] CIF; [100] upper left corner CIF;
            [101] upper right corner CIF; [110] lower left corner CIF;
            [111] lower right corner CIF.
I: (1 bit) [1] if it is the first packet a full intra encoded image;
           [0] else.
MBZ: (3 bits) Must Be Zero  

*
*/

#define RTP_VERSION      1
#define RTP_H261_CONTENT 0x1f
#define RTP_PYRA_CONTENT 0x1e
#define RTP_BYTE1        0x40
#define RTP_BYTE2_SYNC   0x5f
#define RTP_BYTE2_NOSYNC 0x1f

#define RTP_RAD          0x41
#define RTP_RAD_FIR      0x00
#define RTP_RAD_NACK     0x01

#define RTP_CDESC        0x20
#define RTP_SDESC        0x21
#define RTP_BYE          0x23
#define RTP_HELLO        0x24

typedef struct
{
  u_short sec;         /* 16-bits seconds             */
  u_short usec;        /* 16 bits microseconds (<< 4) */
} TIMESTAMP;
