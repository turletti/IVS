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
*  File    : rtp.h       	                			   *
*  Date    : 1995/2/15		           				   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description :   RTP/RTCP definitions according to the RTP draft from    *
*                  Henning Schulzrinne, Stephen Casner RTP: A Transport    *
*                  Protocol for Real-Time Applications INTERNET-DRAFT,     *
*                  October 20, 1993.                                       *
*                                                                          *
\**************************************************************************/
#ifndef _rtp_h
#define _rtp_h

#include "endian.h"


#ifndef LEN_MAX_NAME
#define LEN_MAX_NAME          218
#endif
#define RTP_VERSION            1
#define LEN_RTP_HEADER         8

#define RTP_PCM_CONTENT        0
#define RTP_ADPCM_4            5
#define RTP_VADPCM_CONTENT    15

#define RTP_ADPCM_6           33
#define RTP_ADPCM_5           34
#define RTP_ADPCM_3           35
#define RTP_ADPCM_2           36
#define RTP_LPC                7
#define RTP_GSM_13             3

#define RTP_H261_CONTENT      31
#define RTP_CONTROL_CONTENT   16 /* used by ivs_record/replay */


#define RTP_IPv4               1
#define RTP_SDES              34
#define RTP_SSRC               1
#define RTP_BYE               35
#define RTP_QOS               36
#define RTP_FIR               96
#define RTP_NACK              97
#define RTP_APP              127


#define RTP_TYPE_ADDR          1
#define RTP_TYPE_PORT          2
#define RTP_SDES_CLASS_USER    0
#define RTP_SDES_CLASS_TEXT   16

#define RTP_APP_MCCT_VAL 0x4D434354
#define RTP_APP_MCCR_VAL 0x4D434352

typedef struct {
#ifdef BIT_ZERO_ON_RIGHT
  u_int8 channel:6;  /* channel id            */
  u_int8 ver:2;      /* version number        */
  u_int8 format:6;   /* format of payload     */
  u_int8 s:1;        /* sync bit              */
  u_int8 p:1;        /* option bit present    */
#else /* BIT_ZERO_ON_LEFT */
  u_int8 ver:2;      /* version number        */
  u_int8 channel:6;  /* channel id            */
  u_int8 p:1;        /* option bit present    */
  u_int8 s:1;        /* sync bit              */
  u_int8 format:6;   /* format of payload     */
#endif
  u_int16 seq;      /* sequence number       */
  u_int16 ts_sec;   /* timestamp (seconds)   */
  u_int16 ts_usec;  /* timestamp (fraction)  */
} rtp_hdr_t;


typedef struct {
#ifdef BIT_ZERO_ON_RIGHT
  u_int8 type:7;         /* option type                    */
  u_int8 final:1;        /* final option                   */
#else /* BIT_ZERO_ON_LEFT */
  u_int8 final:1;        /* final option                   */
  u_int8 type:7;         /* option type                    */
#endif
  u_int8 length;         /* length including type/length   */
  u_int16 subtype;       /* subtype = 0                    */
  u_int32 name;          /* name "MCCR"                    */
#ifdef BIT_ZERO_ON_RIGHT
  u_int8 R2:2;	         /* reserved                       */
  u_int8 state:2;        /* state according to transmitter */
  u_int8 R1:2;	         /* reserved                       */
  u_int8 rttSolicit:1;   /* Rtt solicited                  */
  u_int8 sizeSolicit:1;  /* number of receivers solicited  */
#else /* BIT_ZERO_ON_LEFT */
  u_int8 sizeSolicit:1;  /* number of receivers solicited  */    
  u_int8 rttSolicit:1;   /* Rtt solicited                  */
  u_int8 R1:2;	         /* reserved                       */
  u_int8 state:2;        /* state according to transmitter */
  u_int8 R2:2;	         /* reserved                       */
#endif
  u_int32 rttDelay:24;   /* key to sample                  */
  u_int16 ts_sec;        /* timestamp (seconds)   */
  u_int16 ts_usec;       /* timestamp (fraction)  */
} app_mccr_t;            


typedef struct {
#ifdef BIT_ZERO_ON_RIGHT
  u_int8 type:7;         /* option type                      */
  u_int8 final:1;        /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
  u_int8 final:1;        /* final option                     */
  u_int8 type:7;         /* option type                      */
#endif
  u_int8 length;         /* length including type/length     */
  u_int16 subtype;       /* subtype = 0                      */
  u_int32 name;          /* name "MCCT"                      */
#ifdef BIT_ZERO_ON_RIGHT
  u_int8 R2:2;           /* reserved                         */
  u_int8 state:2;        /* state according to transmitter   */
  u_int8 R1:2;           /* reserved                         */
  u_int8 rttSolicit:1;   /* Rtt solicited                    */
  u_int8 sizeSolicit:1;  /* number of receivers solicited    */
  u_int8 keyShift:4;     /* number of bits to shift key left */
  u_int8 rttShift:4;   /* number of bits used to generate rtt delay period */
#else /* BIT_ZERO_ON_LEFT */
  u_int8 sizeSolicit:1;  /* number of receivers solicited    */    
  u_int8 rttSolicit:1;	 /* Rtt solicited                    */
  u_int8 R1:2;	         /* reserved                         */
  u_int8 state:2;        /* state according to transmitter   */
  u_int8 R2:2;           /* reserved                         */
  u_int8 rttShift:4;   /* number of bits used to generate rtt delay period */
  u_int8 keyShift:4;     /* number of bits to shift key left */
#endif
  u_int16 key;           /* key to sample                    */
  u_int32 maxRtt;        /* Maximum rtt found                */
} app_mcct_t;


typedef union {

    struct {       /* Generic first 16 bits of options */
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;  /* option type                      */
    u_int8 final:1; /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1; /* final option                     */
    u_int8 type:7;  /* option type                      */
#endif
    u_int8 length;  /* length including type/length     */
  } generic;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;  /* option type                      */
    u_int8 final:1; /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1; /* final option                     */
    u_int8 type:7;  /* option type                      */
#endif
    u_int8 length;  /* length including type/length     */
    u_int16 id;     /* content source identifier        */
  } ssrc;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;  /* option type                      */
    u_int8 final:1; /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1; /* final option                     */
    u_int8 type:7;  /* option type                      */
#endif
    u_int8 length;  /* length including type/length     */
    u_int16 id;     /* content source identifier        */
  } bye;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;      /* option type                  */
    u_int8 final:1;     /* final option                 */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1;     /* final option                 */
    u_int8 type:7;      /* option type                  */
#endif
    u_int8 length;      /* length including type/length */
    u_int16 subtype;    /* subtype = 0                  */
    u_int32 name;       /* name "IVS "                  */
    u_int8 ver;         /* IVS version 0x33             */
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 R1:4;        /* reserved                     */
    u_int8 conf_size:2; /* Conference size              */
    u_int8 video:1;     /* video encoding flag          */
    u_int8 audio:1;     /* audio encoding flag          */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 audio:1;     /* audio encoding flag          */
    u_int8 video:1;     /* video encoding flag          */
    u_int8 conf_size:2; /* Conference size              */
    u_int8 R1:4;        /* reserved                     */
#endif
    u_int16 R2;         /* reserved                     */
  } app_ivs;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;      /* option type                  */
    u_int8 final:1;     /* final option                 */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1;     /* final option                 */
    u_int8 type:7;      /* option type                  */
#endif
    u_int8 length;      /* length including type/length */
    u_int16 subtype;    /* subtype = 0                  */
    u_int32 name;       /* name "IVS "                  */
    u_int16 index;         /* IVS version 0x33             */
    u_int16 valpred;
  } app_ivs_audio;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;      /* option type                  */
    u_int8 final:1;     /* final option                 */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1;     /* final option                 */
    u_int8 type:7;      /* option type                  */
#endif
    u_int8 length;      /* length including type/length */
    u_int16 subtype;    /* subtype = 0                  */
    u_int32 name;       /* name "IVS "                  */
    u_int16 index;         /* IVS version 0x33             */
    int16 encoding_type;
    int16 length_data;
    u_int16 reserved;
    u_int8 data;        /* for pointing to the data */
    u_int8 reserved1;
    u_int16 reserved2;
    
  } app_ivs_audio_redondancy;


  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;  /* option type                      */
    u_int8 final:1; /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1; /* final option                     */
    u_int8 type:7;  /* option type                      */
#endif
    u_int8 length;           /* length including type/length   */
    u_int16 id;              /* content source identifier      */
    u_int8 ptype;            /* type = PORT (2)                */
    u_int8 plength;          /* length of PORT option (1)      */
    u_int16 port;            /* UDP port number                */
    u_int8 atype;            /* type = ADDR (1)                */
    u_int8 alength;          /* length of ADDR option (2)      */
    u_int8 R;                /* reserved                       */
    u_int8 adtype;           /* address type                   */
    u_int32 addr;             /* network layer address          */
    u_int8 ctype;            /* class type = CNAME or TXT      */
    u_int8 clength;          /* length of CLASS option         */
    char name[LEN_MAX_NAME]; /* name describing the source     */
  } sdes;
  
  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 Ebit:3; /* Number of bits to ignore at the end of the packet   */
    u_int8 E:1;    /* Set if end of GOB                                   */
    u_int8 Sbit:3; /* Number of bits to ignore at the start of the packet */
    u_int8 S:1;    /* Set if beginning of GOB                             */
    u_int8 size:3; /* (0) QCIF, (1) CIF, or CIF number in SCIF            */
    u_int8 Q:1;    /* Set if QoS measurement packets are requested        */
    u_int8 F:1;    /* Set if FIR and NACK packets are enabled             */
    u_int8 V:1;    /* Set if movement vectors are encoded                 */
    u_int8 I:1;    /* Set it is the first packet of a Full Intra image    */
    u_int8 C:1;    /* Set if color is encoded                             */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 S:1;    /* Set if beginning of GOB                             */
    u_int8 Sbit:3; /* Number of bits to ignore at the start of the packet */
    u_int8 E:1;    /* Set if end of GOB                                   */
    u_int8 Ebit:3; /* Number of bits to ignore at the end of the packet   */
    u_int8 C:1;    /* Set if color is encoded                             */
    u_int8 I:1;    /* Set it is the first packet of a Full Intra image    */
    u_int8 V:1;    /* Set if movement vectors are encoded                 */
    u_int8 F:1;    /* Set if FIR and NACK packets are enabled             */
    u_int8 Q:1;    /* Set if QoS measurement packets are requested        */
    u_int8 size:3; /* (0) QCIF, (1) CIF, or CIF number in SCIF            */
#endif
  } h261;
  
  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;  /* option type                      */
    u_int8 final:1; /* final option                     */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1; /* final option                     */
    u_int8 type:7;  /* option type                      */
#endif
    u_int8 length;  /* length including type/length     */
    u_int16 R1;     /* reserved                         */
  } fir;

  struct {
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 type:7;    /* option type                    */
    u_int8 final:1;   /* final option                   */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 final:1;   /* final option                   */
    u_int8 type:7;    /* option type                    */
#endif
    u_int8 length;    /* length including type/length   */
    u_int16 R1;       /* reserved                       */
    u_int8 fgobl;     /* Number of the first GOB lost   */
    u_int8 lgobl;     /* Number of the last GOB lost    */
    u_int8 R2;        /* reserved                       */
#ifdef BIT_ZERO_ON_RIGHT
    u_int8 size:3;     /* Image format                   */
    u_int8 R3:5;       /* reserved                       */
#else /* BIT_ZERO_ON_LEFT */
    u_int8 R3:5;       /* reserved                       */
    u_int8 size:3;     /* Image format                   */
#endif
    u_int16 ts_sec;   /* lost image timestamp (sec)     */
    u_int16 ts_usec;  /* lost image timestamp (usec)    */
  } nack;

  app_mcct_t app_mcct;
  app_mccr_t app_mccr;

} rtp_t;

/* Hacky way to define where data starts - notice the "2" for the h261 hdr */
#define MCDATA_START (sizeof(app_mcct_t) + sizeof(rtp_hdr_t) + 2)

#endif /* _rtp_h */

