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
*  File              : ivs.c       	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/3/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Main file.                                          *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
* Andrzej Wozniak       |  93/29/6  | parallax video src menu added        *
*                                                                          *
\**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#ifdef SPARCSTATION
#include <sys/utsname.h>
#endif
#ifdef VIDEOPIX
#include <vfc_lib.h>
#endif

#ifdef SUNVIDEO
#undef NULL
#include <xil/xil.h>
XilImage rtvc_image;
#endif

#ifdef VIGRAPIX
#include <vigrapix.h>
#endif


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	

#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Label.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Dialog.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "quantizer.h"
#include "rtp.h"

#ifdef CRYPTO
#include "ivs_secu.h"
#endif

#define NB_MAX_BOARD   10
#define SCAN_MS        40
#define min(a,b) ((a) > (b) ? (b) : (a))

typedef struct {
  Boolean color; /* Color encoding flag */
  Boolean force_grey; /* Force grey decoding */
  Boolean qcif2cif; /* qcif decoding is zoomed */
  Boolean auto_send; /* video is automatically sent at initialization */
  Boolean local_display;
  Boolean privilege_quality;
  Boolean max_frame_rate;
#ifdef PARALLAX
  Boolean px_squeeze;
  int px_channel; /* (0) CH_AUTO, (1) CH_1, (2) CH_2 */
  int px_standard; /* (0) NTSC, (1) PAL, (2) SECAM, (3) AUTO_PAL,
		      (4) AUTO_SECAM */
#endif /* PARALLAX */  
#ifdef VIGRAPIX
  int secam_format; /* (0) DISABLED, (1) ENABLED */
#endif /* VIGRAPIX */
#ifdef GALILEOVIDEO
  int pal_format; /* (0) DISABLED, (1) ENABLED */
#endif
#ifdef SCREENMACHINE
  int video_format;
  int video_field;
  int current_sm;
#endif
  int floor_mode; /* (0) FLOOR_BY_DEFAULT, (1) FLOOR_ON_OFF and
		     (2) FLOOR_RATE_MIN_MAX */
  int conf_size; /* (0) LARGE, (1) MEDIUM, (3) SMALL */
  int size; /* (0) SCIF, (1) CIF, (2) QCIF */
  int rate_control; /* (0) Automatic, (1) Manual, (2) No control */
  int rate_min; /* Between 10 and rate_max */
  int rate_max; /* Between rate_min and 500 */
  int max_rate_max; /* Maximal rate_max value for the cursor */
  int port_video; /* Port Video */
  int default_brightness; /* Between 0 and 100 */
  int default_contrast; /* Between 0 and 100 */
  int default_quantizer; /* between 1 and 31 */
  int frame_rate_max; /* Between 1 and 30 */
  int audio_format; /* (0) PCM, (1) ADPCM-6, (2) ADPCM-5, (3) ADPCM-4,
		       (4) ADPCM-3, (5) ADPCM-2, (6) VADPCM, (7) LPC, 
		       (8) GSM-13 */
  int audio_redundancy; /* (0) PCM, (1) ADPCM-6, (2) ADPCM-5, (3) ADPCM-4,
			   (4) ADPCM-3, (5) ADPCM-2, (6) VADPCM, (7) LPC, 
			   (8) GSM-13, (9) NONE */
  int default_record; /* Between 0 and 100 */
  int default_squelch; /* Between 0 and 100 */
  int default_volume; /* Between 0 and 100 */
  int default_ttl;
  int nb_max_feedback; /* nb_stations > nb_max_feedback then NACK,
			  FIR are avoided */
  int board_selected; /* (1) PX, (2) SunVideo, (3) VFC, (4) Indigo,
			 (5) Galileo, (6) Indy, (7) RasterOps, (8) VTX,
			 (9) J300 */
  String session_name;
  String group; /* Default address group */
  String video_port; /* Default multicast video port */
  String audio_port; /* Default multicast audio port */
  String control_port; /* Default multicast control port */
  Boolean auto_muting;
  Boolean implicit_video_decoding;
  Boolean implicit_audio_decoding;
  Boolean implicit_get_audio;
  Boolean without_any_audio;
  Boolean restrict_saving;
  Boolean recv_only;
  Boolean log_mode;
  Boolean stat_mode;
  Boolean debug_mode;
} AppliData, *PtrAppliData;

#define streq(a, b)        ( strcmp((a), (b)) == 0 )
#define maxi(x,y) ((x)>(y) ? (x) : (y))
#define DiffTime(new, old)  \
  (1000*((new).tv_sec - (old).tv_sec) + ((new).tv_usec - (old).tv_usec)/1000)
#define same_station(p, sin_addr, identifier) \
(p->sin_addr != sin_addr ? 0 : (p->identifier != identifier ? 0 : 1))

#ifdef HPUX
static char audioserver[50]; 
#endif

#define LEN_DATA           16 /* len data from coder : fps, bw and rate_max */

#define LoginFileName          ".ivs.login"

#define LabelCallOn            "Call up"
#define LabelCallOff           "Abort call"

#define LabelPushToTalk        "Push to talk"
#define LabelSpeakNow          "Speak now !!"   

#define LabelTakeAudio         "  Get   \n audio"
#define LabelReleaseAudio      "Release \n audio "
#define LabelVideoCoderPanel   "Video Coder Panel"
#define LabelVideoGrabberPanel "Video Grabber Panel"
#define LabelAudioPanel        "Audio Panel"
#define LabelControlPanel      "Control Panel"
#define LabelInfoPanel         "About IVS ..."


#define Mark_width 15
#define Mark_height 15

#define headphone_width 32
#define headphone_height 32
static u_char headphone_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xe0, 0x03, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x08, 0x08, 0x00,
   0x00, 0x04, 0x10, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00, 0x02, 0x20, 0x00,
   0x00, 0x01, 0x40, 0x00, 0x80, 0x01, 0xc0, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x0c, 0x98, 0x00,
   0x80, 0x1c, 0x9c, 0x00, 0xc0, 0x1f, 0xfc, 0x01, 0xc0, 0x1f, 0xfc, 0x01,
   0xc0, 0x1f, 0xfc, 0x01, 0xc0, 0x1f, 0xfc, 0x01, 0x00, 0x1c, 0x1c, 0x00,
   0x00, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define speaker_ext_width 32
#define speaker_ext_height 32
static u_char speaker_ext_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x27, 0xf2, 0x01,
   0x80, 0x40, 0x41, 0x00, 0x80, 0x87, 0x40, 0x00, 0x80, 0x40, 0x41, 0x00,
   0x80, 0x27, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x00, 0x00, 0x60, 0x40, 0x00,
   0x00, 0x70, 0x80, 0x00, 0x00, 0x78, 0x08, 0x01, 0x00, 0x7c, 0x10, 0x02,
   0x00, 0x7e, 0x20, 0x02, 0xf0, 0x7f, 0x22, 0x04, 0xf0, 0x7f, 0x44, 0x04,
   0xf0, 0x7f, 0x44, 0x08, 0xf0, 0x7f, 0x88, 0x08, 0xf0, 0x7f, 0x48, 0x08,
   0xf0, 0x7f, 0x44, 0x04, 0xf0, 0x7f, 0x24, 0x04, 0xf0, 0x7f, 0x22, 0x02,
   0x00, 0x7e, 0x10, 0x02, 0x00, 0x7c, 0x08, 0x01, 0x00, 0x78, 0x80, 0x00,
   0x00, 0x70, 0x40, 0x00, 0x00, 0x60, 0x20, 0x00, 0x00, 0x40, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


#define speaker_width 32
#define speaker_height 32
static u_char speaker_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x00, 0x00, 0x60, 0x40, 0x00,
   0x00, 0x70, 0x80, 0x00, 0x00, 0x78, 0x08, 0x01, 0x00, 0x7c, 0x10, 0x02,
   0x00, 0x7e, 0x20, 0x02, 0xf0, 0x7f, 0x22, 0x04, 0xf0, 0x7f, 0x44, 0x04,
   0xf0, 0x7f, 0x44, 0x08, 0xf0, 0x7f, 0x88, 0x08, 0xf0, 0x7f, 0x48, 0x08,
   0xf0, 0x7f, 0x44, 0x04, 0xf0, 0x7f, 0x24, 0x04, 0xf0, 0x7f, 0x22, 0x02,
   0x00, 0x7e, 0x10, 0x02, 0x00, 0x7c, 0x08, 0x01, 0x00, 0x78, 0x80, 0x00,
   0x00, 0x70, 0x40, 0x00, 0x00, 0x60, 0x20, 0x00, 0x00, 0x40, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define mike_width 32
#define mike_height 32
static u_char mike_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x6c, 0x00,
   0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xab, 0x01, 0x00, 0x00, 0x55, 0x01,
   0x00, 0x00, 0xab, 0x01, 0x00, 0x80, 0xd7, 0x00, 0x00, 0xc0, 0x6f, 0x00,
   0x00, 0xe0, 0x3f, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xf8, 0x03, 0x00,
   0x00, 0xfc, 0x03, 0x00, 0x00, 0xfe, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00,
   0x80, 0x7f, 0x03, 0x00, 0x80, 0x3f, 0x03, 0x00, 0x80, 0x1f, 0x03, 0x00,
   0x80, 0x0f, 0x03, 0x00, 0xc0, 0x00, 0x03, 0x00, 0x40, 0x00, 0x03, 0x00,
   0x40, 0x00, 0x03, 0x00, 0x60, 0x00, 0x03, 0x00, 0x20, 0x00, 0x03, 0x00,
   0x20, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define mike_ext_width 32
#define mike_ext_height 32
static u_char mike_ext_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x27, 0xf2, 0x01,
   0x80, 0x40, 0x41, 0x00, 0x80, 0x87, 0x40, 0x00, 0x80, 0x40, 0x41, 0x00,
   0x80, 0x27, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x6c, 0x00,
   0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xab, 0x01, 0x00, 0x00, 0x55, 0x01,
   0x00, 0x00, 0xab, 0x01, 0x00, 0x80, 0xd7, 0x00, 0x00, 0xc0, 0x6f, 0x00,
   0x00, 0xe0, 0x3f, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xf8, 0x03, 0x00,
   0x00, 0xfc, 0x03, 0x00, 0x00, 0xfe, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00,
   0x00, 0x7f, 0x03, 0x00, 0x00, 0x3f, 0x03, 0x00, 0x00, 0x1f, 0x03, 0x00,
   0x80, 0x01, 0x03, 0x00, 0xc0, 0x00, 0x03, 0x00, 0x40, 0x00, 0x03, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define listen_width 16
#define listen_height 16
static u_char listen_bits[] = {
   0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x09,
   0x00, 0x19, 0x00, 0x19, 0x00, 0x09, 0x00, 0x05, 0x20, 0x03, 0xf0, 0x01,
   0xf8, 0x00, 0xf8, 0x00, 0x70, 0x00, 0x00, 0x00};

#define muted_width 16
#define muted_height 16
static u_char muted_bits[] = {
   0x01, 0x80, 0x02, 0x41, 0x04, 0x23, 0x08, 0x13, 0x10, 0x0d, 0x20, 0x0d,
   0x40, 0x1b, 0x80, 0x19, 0x80, 0x09, 0x40, 0x07, 0x20, 0x07, 0xf0, 0x09,
   0xf8, 0x10, 0xfc, 0x20, 0x72, 0x40, 0x01, 0x80};

#define shown_width 16
#define shown_height 16
static u_char shown_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0xf0, 0x1f, 0x1c, 0x38, 0x02, 0x40,
   0xe0, 0x07, 0x98, 0x19, 0xc4, 0x33, 0x42, 0x42, 0xc4, 0x23, 0x98, 0x19,
   0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define hidden_width 16
#define hidden_height 16
static u_char hidden_bits[] = {
   0x01, 0x80, 0x02, 0x40, 0xe4, 0x27, 0xf8, 0x1f, 0x1c, 0x38, 0x22, 0x44,
   0xe0, 0x07, 0x98, 0x19, 0xc4, 0x33, 0x42, 0x42, 0xe4, 0x27, 0x98, 0x19,
   0xe8, 0x17, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80};

static u_char Mark_bits[] = {
   0x00, 0x00, 0x80, 0x00, 0xc0, 0x01, 0xe0, 0x03, 0xf0, 0x07, 0xf8, 0x0f,
   0xfc, 0x1f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03,
   0xc0, 0x01, 0x80, 0x00, 0x00, 0x00};

#ifndef NO_AUDIO
static ActionMicro(), ActionMicroToggle();
#endif

#ifdef CRYPTO
void NewKey();
extern Widget form_key, label_key, key_value;
extern int getkey();
#endif

XtActionsRec actions[] = {
#ifndef NO_AUDIO
  {"ActionMicro",          (XtActionProc) ActionMicro},
  {"ActionMicroToggle",    (XtActionProc) ActionMicroToggle},
#endif
#ifdef CRYPTO
  {"NewKey",               (XtActionProc) NewKey},
#endif
};

static void AudioDecode();

#if defined(VIDEOPIX) || defined(PARALLAX) || defined(SUNVIDEO) \
  || defined (VIGRAPIX) \
  || defined(INDIGOVIDEO) || defined(GALILEOVIDEO) || defined(INDYVIDEO) \
  || defined(VIDEOTX) ||  defined(J300) \
  || defined(HP_ROPS_VIDEO) \
  || defined(SCREENMACHINE)
#define ANY_VIDEO_BOARD 1
#endif

static XtResource my_resources[] = {
#if defined(ANY_VIDEO_BOARD) && !defined(HP_ROPS_VIDEO)
  {"color", "Color", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, color), XtRString, "TRUE"},
#else  
  {"color", "Color", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, color), XtRString, "FALSE"},
#endif
/* Color encoding */
  {"force_grey", "Force_grey", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, force_grey), XtRString, "FALSE"},
/* Decoding is not forced in grey */
  {"qcif2cif", "Qcif2cif", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, qcif2cif), XtRString, "FALSE"},
/* QCIF video is not zoomed */
  {"auto_send", "Auto_send", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, auto_send), XtRString, "FALSE"},
/* Video is not sent at initialization */
  {"local_display", "Local_display", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, local_display), XtRString, "FALSE"},
/* Local display off */
  {"privilege_quality", "Privilege_quality", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, privilege_quality), XtRString, "TRUE"},
/* Privilege Quality selected */
  {"max_frame_rate", "Max_frame_rate", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, max_frame_rate), XtRString, "FALSE"},
/* Max frame rate option off */
#ifdef PARALLAX
  {"px_squeeze", "Px_squeeze", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, px_squeeze), XtRString, "TRUE"},
/* SQUEEZE mode set */
  {"px_channel", "Px_channel", XtRInt, sizeof(int), XtOffset(PtrAppliData, px_channel), XtRImmediate, (caddr_t) 0},
/* CH_AUTO selected */
  {"px_standard", "Px_standard", XtRInt, sizeof(int), XtOffset(PtrAppliData, px_standard), XtRImmediate, (caddr_t) 3},
/* AUTO_PAL selected */
#endif /* PARALLAX */  
#ifdef VIGRAPIX
  {"secam_format", "Secam_format", XtRInt, sizeof(int), XtOffset(PtrAppliData, secam_format), XtRImmediate, (caddr_t) 0},
/* SECAM format disabled */
#endif /* VIGRAPIX */
#ifdef GALILEOVIDEO
  {"pal_format", "Pal_format", XtRInt, sizeof(int), XtOffset(PtrAppliData, pal_format), XtRImmediate, (caddr_t) 0},
/* PAL format disabled */
#endif /* VIGRAPIX */
#ifdef SCREENMACHINE
  {"video_format", "Video_format", XtRInt, sizeof(int), XtOffset(PtrAppliData, video_format), XtRImmediate, (caddr_t) 0},
/* NTSC by default */
  {"video_field", "Video_field", XtRInt, sizeof(int), XtOffset(PtrAppliData, video_field), XtRImmediate, (caddr_t) 0},
/* Freeze by default */
  {"current_sm", "Current_sm", XtRInt, sizeof(int), XtOffset(PtrAppliData, current_sm), XtRImmediate, (caddr_t) 0},
/* SM_1 by default */
#endif /* SCREENMACHINE */
  {"floor_mode", "Floor_mode", XtRInt, sizeof(int), XtOffset(PtrAppliData, floor_mode), XtRImmediate, (caddr_t) 0},
/* Floor by default */
  {"conference_size", "Conference_size", XtRInt, sizeof(int), XtOffset(PtrAppliData, conf_size), XtRImmediate, (caddr_t) 3},
/* SMALL conference size by default */
  {"size", "Size", XtRInt, sizeof(int), XtOffset(PtrAppliData, size), XtRImmediate, (caddr_t) 2},
/* QCIF by default */
  {"rate_control", "Rate_control", XtRInt, sizeof(int), XtOffset(PtrAppliData, rate_control), XtRImmediate, (caddr_t) 0},
/* Automatic control selected */
  {"rate_min", "Min_rate_max", XtRInt, sizeof(int), XtOffset(PtrAppliData, rate_min), XtRImmediate, (caddr_t) 15},
/* rate_min = 15 kbps */
  {"rate_max", "Rate_max", XtRInt, sizeof(int), XtOffset(PtrAppliData, rate_max), XtRImmediate, (caddr_t) 150},
/* rate_max = 150 kbps */
  {"port_video", "Port_video", XtRInt, sizeof(int), XtOffset(PtrAppliData, port_video), XtRImmediate, (caddr_t) 0},
  /* Default video port (the first in the menu) */
  {"default_brightness", "Default_brightness", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_brightness), XtRImmediate, (caddr_t) 50},
/* Default Brightness = 70 */
  {"default_contrast", "Default_contrast", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_contrast), XtRImmediate, (caddr_t) 50},
/* Default Contrast = 50 */
  {"default_quantizer", "Default_quantizer", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_quantizer), XtRImmediate, (caddr_t) 3},
/* Default quantizer = 3 */
  {"frame_rate_max", "Frame_rate_max", XtRInt, sizeof(int), XtOffset(PtrAppliData, frame_rate_max), XtRImmediate, (caddr_t) 5},
/* Frame rate max = 25 fps */
  {"audio_format", "Audio_format", XtRInt, sizeof(int), XtOffset(PtrAppliData, audio_format), XtRImmediate, (caddr_t) 0},
/* Default audio encoding = PCM (0) */
  {"audio_redundancy", "Audio_redundancy", XtRInt, sizeof(int), XtOffset(PtrAppliData, audio_redundancy), XtRImmediate, (caddr_t) 9},
/* Default audio redundancy = NONE (9) */
#ifdef SGISTATION 
  {"default_record", "Default_record", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_record), XtRImmediate, (caddr_t) 80},
#else
#ifdef SPARCSTATION   
  {"default_record", "Default_record", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_record), XtRImmediate, (caddr_t) 20},
#else
#ifdef AF_AUDIO  
  {"default_record", "Default_record", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_record), XtRImmediate, (caddr_t) 90},
#else
  {"default_record", "Default_record", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_record), XtRImmediate, (caddr_t) 90},
#endif /* AF_AUDIO */
#endif /* SPARCSTATION */
#endif /* SGISTATION */
/* Default record value */  
  {"default_squelch", "Default_squelch", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_squelch), XtRImmediate, (caddr_t) 40},
/* Default silence detection threshold value */  
  {"default_volume", "Default_volume", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_volume), XtRImmediate, (caddr_t) 50},
/* Default volume value */
  {"default_ttl", "Default_ttl", XtRInt, sizeof(int), XtOffset(PtrAppliData, default_ttl), XtRImmediate, (caddr_t) 16},
/* Default TTL value */
  {"nb_max_feedback", "Nb_max_feedback", XtRInt, sizeof(int), XtOffset(PtrAppliData, nb_max_feedback), XtRImmediate, (caddr_t) 10},
/* If nb_stations > nb_max_feedback, NACK and FIR are avoided */
#ifdef VIGRAPIX
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) VIGRAPIX_BOARD},
#else
#ifdef SUNVIDEO
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) SUNVIDEO_BOARD},
#else
#ifdef VIDEOPIX  
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) VIDEOPIX_BOARD},
#else
#ifdef PARALLAX  
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) PARALLAX_BOARD},
#endif
#endif /* VIDEOPIX */
#endif /* SUNVIDEO */
#endif /* VIGRAPIX */  
#ifdef SGISTATION
#ifdef INDYVIDEO
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) INDY_BOARD},
#else  
#ifdef GALILEOVIDEO
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) GALILEO_BOARD},
#else  
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) INDIGO_BOARD},
#endif /* GALILEOVIDEO */
#endif /* INDYVIDEO */
#endif /* SGISTATION */  
#ifdef HP_ROPS_VIDEO
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) RASTEROPS_BOARD},
#endif
#ifdef VIDEOTX
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) VIDEOTX_BOARD},
#endif  
#ifdef J300
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) J300_BOARD},
#endif  
#ifdef SCREENMACHINE
  {"board_selected", "Board_selected", XtRInt, sizeof(int), XtOffset(PtrAppliData, board_selected), XtRImmediate, (caddr_t) SCREENMACHINE_BOARD},
#endif
/* Board selected */
  {"session_name", "Session_name", XtRString, sizeof(String), XtOffset(PtrAppliData, session_name), XtRString, ""},
/* Default session name, e-mail used if null */
  {"group", "Multicast_group", XtRString, sizeof(String), XtOffset(PtrAppliData, group), XtRString, MULTI_IP_GROUP},
  {"video_port", "Multicast_video_port", XtRString, sizeof(String), XtOffset(PtrAppliData, video_port), XtRString, MULTI_VIDEO_PORT},
  {"audio_port", "Multicast_audio_port", XtRString, sizeof(String), XtOffset(PtrAppliData, audio_port), XtRString, MULTI_AUDIO_PORT},
  {"control_port", "Multicast_control_port", XtRString, sizeof(String), XtOffset(PtrAppliData, control_port), XtRString, MULTI_CONTROL_PORT},
  {"auto_muting", "Auto_muting", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, auto_muting), XtRString, "TRUE"},
/* The loudspeaker is automatically muted while speaking */
  {"implicit_video_decoding", "Implicit_video_decoding", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, implicit_video_decoding), XtRString, "TRUE"},
/* A new video flow is automatically decoded */
  {"implicit_audio_decoding", "Implicit_audio_decoding", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, implicit_audio_decoding), XtRString, "TRUE"},
/* A new audio flow is automatically decoded */
  {"without_any_audio", "Without_any_audio", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, without_any_audio), XtRString, "FALSE"},
/* Enable the audio codec */
  {"implicit_get_audio", "Implicit_get_audio", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, implicit_get_audio), XtRString, "TRUE"},
/* The audio device is automatically taken */
  {"restrict_saving", "Restrict_saving", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, restrict_saving), XtRString, "FALSE"},
/* All stations (actives and passives are listed */
  {"recv_only", "Recv_only", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, recv_only), XtRString, "FALSE"},
/* Receive only mode off */
  {"log_mode", "Log_mode", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, log_mode), XtRString, "FALSE"},
/* Log mode off */
  {"stat_mode", "Stat_mode", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, stat_mode), XtRString, "FALSE"},
/* Stat mode off */
  {"debug_mode", "Debug_mode", XtRBoolean, sizeof(Boolean), XtOffset(PtrAppliData, debug_mode), XtRString, "FALSE"},
/* Debug mode off */
};
  
  
String fallback_resources[] = {
  "*Command.font: -adobe-courier-bold-r-normal--12-120-75-75-m-70-iso8859-1",
  "*Label.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*Text*font: -adobe-courier-bold-r-normal--12-120-75-75-m-70-iso8859-1",
  "*ButtonPopup1.font: -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1",
  "*ButtonPopup2.font: -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1",
  "*ButtonPopup3.font: -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1",
  "*ButtonPopup4.font: -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1",
  "*ButtonPopup5.font: -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1",
  "*HostName.font: -adobe-courier-bold-r-normal--14-140-75-75-m-90-iso8859-1",
  "*MyHostName.font: -adobe-courier-bold-r-normal--14-140-75-75-m-90-iso8859-1",
  "*NameBrightness.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameContrast.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameControl.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameFrms.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*LabelB*.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*NameScale.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  "*ValueScale.font: -adobe-courier-bold-r-normal--10-100-75-75-m-60-iso8859-1",
  
  "*Form.background:               Grey",
  "*Label.background:              Grey",
  "*Command.background:            MistyRose1",
  "*Command.foreground:            Black",
  "*ButtonQuit.background:         Red",
  "*ButtonQuit.foreground:         White",
  "*ButtonDismiss.background:      Red",
  "*ButtonDismiss.foreground:      White",
  "*FormBandwidth.background:      White",
  "*ScrollScale.background:        LightSteelBlue4",
  "*ScrollScale.foreground:        White",
  "*LabelVideo.background:         Black",
  "*LabelVideo.foreground:         MistyRose1",
  "*LabelAudio.background:         Black",
  "*LabelAudio.foreground:         MistyRose1",
  "*MsgError.background:           Red",
  "*HostName.background:           White",
  "*MyHostName.background:         MistyRose1",
  "*HostName.foreground:           Black",
  "*MyHostName.foreground:         Black",
#ifdef SCREENMACHINE
  "*NameBrightness.label:          Luma Level:",
  "*NameContrast.label:            Chroma Level:",
#else
  "*NameBrightness.label:          Brightness:",
  "*NameContrast.label:            Contrast:",
#endif
  "*ButtonPopup1.label:            Grabber >",
  "*ButtonPopup2.label:            Video >",
  "*ButtonPopup3.label:            Audio >",
  "*ButtonPopup4.label:            Control >",
  "*ButtonPopup5.label:            Info >",
  "*ButtonDefault.label:           Default",
  "*ButtonControl.label:           Bandwidth control",
  "*ButtonEVideo.label:            Encode video",
  "*ButtonQuit.label:              Quit",
  "*ButtonColor.label:             Color encoding",
  "*ButtonGrey.label:              Grey encoding",
  "*ButtonQCIF.label:              Small size",
  "*ButtonCIF.label:               Normal size",
  "*ButtonSCIF.label:              Large size",
  "*ButtonFreeze.label:            Freeze image",
  "*ButtonUnfreeze.label:          Unfreeze image",
  "*ButtonLocal.label:             With local image",
  "*ButtonNoLocal.label:           Without local image",
#ifdef SPARCSTATION
  "*ButtonBoard1.label:            SunVideo",
  "*ButtonBoard2.label:            VigraPix",
  "*ButtonBoard3.label:            Parallax",
  "*ButtonBoard4.label:            VideoPix",
#endif  
#ifdef SGISTATION
  "*ButtonBoard1.label:            Indy board",
  "*ButtonBoard2.label:            Galileo board",
  "*ButtonBoard3.label:            Indigo board",
#endif
#ifdef PC
  "*ButtonBoard1.label:            ScreenMachine II board",
  "*ButtonBoard2.label:            Other",
#endif
#ifdef HP_ROPS_VIDEO
  "*ButtonBoard1.label:            VideoLive board",
  "*ButtonBoard2.label:            Other",
#endif
  "*NameControl.label:             Max bandwidth:",
  "*NameFrms.label:                Max frame rate:",
  "*FormBandwidth.resizable:       True",
  "*LabelB1.label:                 Bandwidth:",
  "*LabelB2.label:                 _____",
  "*LabelB3.label:                 kb/s",
  "*LabelR1.label:                 Frame rate:",
  "*LabelR2.label:                 ____",
  "*LabelR3.label:                 f/s",
  "*NameVolAudio.label:            Volume Gain",
  "*NameRecAudio.label:            Mike Gain",
  "*LabelKey.label:                Key: ",
  "*Key*echo:                      False",
  "*Form.borderWidth:              0",
  "*Label.borderWidth:             0",
  "*FormScale.borderWidth:         0",
  "*NameScale.borderWidth:         0",
  "*ValueScale.borderWidth:        0",
  "*FormAudio.borderWidth:         1",
  "*FormList.borderWidth:          1",
  "*ScrollScale.borderWidth:       4",
  "*ScrollScale.width:             85",
  "*ScrollScale.height:            10",
  "*ScrollScale.minimumThumb:      4",
  "*ScrollScale.thickness:         4",
  "*ButtonLarsen.height:           35",
  "*ButtonRAudio.height:           35",
  "*ButtonRAudio.width:            54",
  "*HostName.width:                254",
  "*MyHostName.width:              254",
  "*ButtonMicro.width:             95",
  "*ButtonDAudio.internalWidth:    2",
  "*ButtonDVideo.internalWidth:    2",
  "*ButtonDAudio.internalHeight:   2",
  "*ButtonDVideo.internalHeight:   2",
  "*ScrollScale.orientation:       horizontal",
  "*ButtonMicro.Translations:      #override   \\n\
    <EnterWindow>:        	   highlight() \\n\
    <LeaveWindow>:        	   reset() \\n\
    <Btn1Down>:	  		   ActionMicroToggle() unset() \\n\
    <Btn1Up>: 	 		   ActionMicroToggle() unset() \\n\
    <Btn3Down>:	  		   ActionMicro() unset() \\n\
    <Btn3Up>: 	 		   ActionMicro() unset()",
  NULL,
};


static char *ConferenceSize[] = {
  "Large Conf.", "Medium Conf.", "Invalid mode", "Small Conf."
};


static char *BoardsItems[] = {
  "",
  "Parallax",
  "SunVideo",
  "VideoPix",
  "Indigo",
  "Galileo",
  "Indy",
  "Raster Ops",
  "VideoTX",
  "J300",
  "VigraPix",
  "ScreenMachine II",
};

/* Global Variables */


BOOLEAN RATE_STAT=FALSE; /* STAT mode for the coder */
BOOLEAN UNICAST=FALSE;
Pixmap mark;
BOOLEAN MICE_STAT=FALSE;

static XtAppContext app_con;
static Widget toplevel;
static Widget form_popup;
static Widget form_onoff;
static Widget button_e_video, button_quit, button_ring;

static Widget form_stations, view_list, form_list, label_hosts;
static Widget host_name[N_MAX_STATIONS];
static Widget button_d_video[N_MAX_STATIONS], button_d_audio[N_MAX_STATIONS];

static LOCAL_TABLE t;
static char log_file[100];
static char IP_GROUP[256], AUDIO_PORT[6], VIDEO_PORT[6], CONTROL_PORT[6];
static char name_user[255];
static struct sockaddr_in addr_s, addr_r, addr_dist;
static int csock_s, csock_r, asock_r, vlen_addr_s;
static int len_addr_s, len_addr_r;
static int fd_answer[2], fd_call[2];
static u_char ttl;
static FILE *F_login;
static int statusp;
static u_long my_sin_addr;
static u_short my_feedback_port;

static XtInputId id_input; /* ID returned from the video coder 
			      XtAppAddInput call. */
static XtInputId id_call_input; /* ID returned from the Call up 
				   XtAppAddInput call. */
static XtPointer client_data; /* Not used but ... */
static XtIntervalId IdTimeOut; /* Id returned from XtAppAddTimeOut */
static XtIntervalId IdTimeOutEncode; /* Id returned from XtAppAddTimeOut */

static int nb_stations=0;  /* Current number of stations in the group */
static AUDIO_TABLE audio_station;
static int first_passive=1; /* Index of the first passive station in the 
			       list */
static int nb_max_feedback; /* if nb_stations > nb_max_feedback, NACK and FIR
			       are avoided */
static u_short class_name=RTP_SDES_CLASS_USER;
static int squelch_level; /* Current squelch level for audio rec */
#ifdef CRYPTO
static char key_string[LEN_MAX_STR+1];
#endif
static BOOLEAN AUDIO_SET=FALSE;
static BOOLEAN IMPLICIT_GET_AUDIO;
static BOOLEAN QCIF2CIF;
static BOOLEAN IMPLICIT_AUDIO_DECODING; /* True if implicite audio decoding. */
static BOOLEAN IMPLICIT_VIDEO_DECODING; /* True if implicite video 
					   decoding. */
static BOOLEAN RESTRICT_SAVING; /* True if only encoders stations 
				   are saved in the local table. */
static BOOLEAN RING_UP=FALSE;
static BOOLEAN NACK_FIR_ALLOWED; /* True if feedback from video decoder is
				    allowed. (QOS is always sent) */
static BOOLEAN LOG_MODE; /* True if LOG mode set */
static BOOLEAN STAT_MODE; /* True if STAT mode set */
static BOOLEAN FORCE_GREY; /* True if only grey display */
static BOOLEAN DEBUG_MODE;
static BOOLEAN AUTO_SEND; /* True if video is sent at initialization */
static BOOLEAN BOARD_OPENED=FALSE;
static BOOLEAN MICRO_TOGGLE=FALSE; /* True if Mouse button1 used for micro */
static BOOLEAN MICRO_PTT=FALSE; /* True if Mouse button3 used for micro */
static BOOLEAN EXPLICIT_RATE_MAX=FALSE; /* True if rate_max is not default */
static Pixmap LabelListen, LabelMuted, LabelShown, LabelHidden;

#ifdef MICE_VERSION
/* global prefix for log files in /usr/tmp */
int logprefix;
#endif

/* from athena.c module */

extern Widget main_form;
extern Widget msg_error;
extern Widget video_coder_panel;
extern Widget video_grabber_panel;
extern Widget audio_panel;
extern Widget control_panel;
extern Widget info_panel;
extern Widget form_control, scroll_control, value_control;
extern Widget form_frms, form_format;
extern Widget label_band2, label_ref2;
extern Widget button_popup1, button_popup2, button_popup3, button_popup4;
extern Widget button_popup5;

extern void DestroyWarning(), ShowWarning(), ChangeLabel(),
  ChangeAndMappLabel(), ReverseColor(), 
  SetSensitiveVideoOptions(), CreateScale(), InitScale(),
  CreateVideoCoderPanel(), CreateVideoGrabberPanel(), CreateControlPanel(),
  PopVideoCoderPanel(), PopVideoGrabberPanel(), PopAudioPanel(),
  PopControlPanel(), PopInfoPanel(), CallbackFreeze();
  
extern BOOLEAN RECV_ONLY; /* True if only reception is allowed */
extern BOOLEAN MICRO_MUTED;
extern BOOLEAN AUDIO_DROPPED;
extern BOOLEAN SPEAKER, MICROPHONE;
extern BOOLEAN AUTO_MUTING; /* True if loudspeaker is muted while speaking */
extern BOOLEAN AUDIO_INIT_FAILED;
extern BOOLEAN WITHOUT_ANY_AUDIO;
extern Widget button_r_audio, button_larsen;
extern Widget input_audio, output_audio;
extern Widget button_ptt;
extern int output_speaker; /* A_SPEAKER, A_HEADPHONE or A_EXTERNAL */
extern int input_microphone; /* A_MICROPHONE or A_LINE_IN */
extern int volume_gain;
extern int record_gain;
extern Pixmap LabelSpeaker, LabelHeadphone, LabelSpeakerExt;
extern Pixmap LabelMicrophone, LabelLineIn;
extern void SetVolumeGain(), SetRecordGain();
extern S_PARAM param;
void CallbackEVideo();

static XtTimerCallbackProc CallbackLoopDeclare();


/***************************************************************************
 * Procedures
 **************************************************************************/


void Usage(name)
     char *name;
{
  fprintf(stderr, IVS_VERSION);
  fprintf(stderr,
	  "\n\nUsage: %s \t [dest[/video_port]] [-t ttl] [-name user_name]\n",
	  name);
  fprintf(stderr,
	  "\t\t [-r] [-a] [-v] [-start_send] [-grey] [-q quantizer]\n");
  fprintf(stderr,
     "\t\t [-S squelch] [-floor{1|2}] [-rate_min value] [-rate_max value]\n");
  fprintf(stderr,
      "\t\t [-K key] [-echo_on] [-{qcif|cif|scif}] [-{pcm|adpcm|vadpcm}]\n");
  fprintf(stderr,
	"\t\t [-{small|medium|large}] [-deaf] [-start_deaf] [-stat] [-log]\n");
  fprintf(stderr, 
	"\n\t dest is a multicast or unicast address or simply a hostname \n");
  fprintf(stderr, 
   "\t -name <session_name>: this name appears on the control window panel\n");
  fprintf(stderr, "\t -t <ttl value>: set the ttl value\n");
  fprintf(stderr, 
	  "\t -ni <network interface>: select a network interface\n");
  fprintf(stderr,
	  "\t -S <squelch_level>: set the silence detection threshold\n");
#ifdef HPUX
  fprintf(stderr, "\t -as <audio>: set audioserver to audio\n");
#endif
  fprintf(stderr,
"\t -r: only the encoders stations will appear on the control panel window\n");
  fprintf(stderr, "\t -a: disable the implicit audio decoding mode\n");
  fprintf(stderr, "\t -deaf: run ivs without any audio facilities\n");
  fprintf(stderr, "\t -start_deaf: do not get the audio device by default\n");
  fprintf(stderr, "\t -v: disable the implicit video decoding mode\n");
  fprintf(stderr, "\t -grey: force grey decoding display\n");
  fprintf(stderr, 
	    "\t -q <default_quantizer>: force the default quantizer value\n");
  fprintf(stderr,
     "\t -floor1: SIG_USR1 and SIG_USR2 used to start/stop video encoding\n");
  fprintf(stderr,
      "\t -floor2: SIG_USR1 and SIG_USR2 used to toggle rate_max/rate_min\n");
  fprintf(stderr, "\t -start_send: send audio and video at initialization\n");
  fprintf(stderr, "\t -rate_min value: set the minimal rate_max to value\n");
  fprintf(stderr, "\t -rate_max value: set the maximal rate_max to value\n");
#ifdef CRYPTO
  fprintf(stderr, 
	  "\t -K <key>: Use key as the encryption key for this ivs session\n");
#endif
  fprintf(stderr, 
	  "\t -echo_on: Disable the loudspeaker muting while speaking\n");
  fprintf(stderr,
	"\t -{qcif|cif|scif}: set the default image size at the coder side\n");
  fprintf(stderr, "\t -{pcm|adpcm|vadpcm}: set the default audio encoding\n");
  fprintf(stderr, "\t -{small|medium|large}: set the conference size style\n");
#ifdef SPARCSTATION
  fprintf(stderr, 
	  "\t -{px|sunvideo|vigrapix|vfc}: set the default video board\n");
#endif
#ifdef SGISTATION
  fprintf(stderr, "\t -{indigo|galileo|indy}: set the default video board\n");
#endif
#ifdef HPUX
  fprintf(stderr, "\t -rops: set the default video board\n");
#endif
#ifdef DECSTATION
  fprintf(stderr, "\t -{videotx|j300}: set the default video board\n");
#endif  
#ifdef MICE_VERSION
  fprintf(stderr, "\t -lp <pid_numer>: only for MICE version\n");
#endif  
  fprintf(stderr, "\t -log: create a list of participants\n");
  fprintf(stderr, "\t -stat: set the statistic mode\n");
  exit(1);
}







BOOLEAN SetAudio()
{
  BOOLEAN SET=FALSE;
  static BOOLEAN FIRST=TRUE;

#ifndef NO_AUDIO
 if(! WITHOUT_ANY_AUDIO){
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
  if(!soundinit(O_WRONLY)){
    if(DEBUG_MODE){
      fprintf(stderr,"audio_decoder: cannot open audio driver\n");
    }
  }else{
    SET = TRUE;
  }
#endif /* SPARCSTATION OR SGISTATION OR I386AUDIO */
#ifdef AF_AUDIO
  if(!audioInit()){
    if(DEBUG_MODE){
      fprintf(stderr,"audio_decoder: cannot connect to AF AudioServer\n");
    }
  }else{
    SET = TRUE;
    sounddest(output_speaker);
  }
#endif /* AF_AUDIO */
  if(SET && !FIRST){
#if defined(SPARCSTATION) || defined(AF_AUDIO)
    sounddest(output_speaker);
#endif
    SetVolumeGain(volume_gain);
    SetVolume(volume_gain);
    SetRecordGain(record_gain);
  }
  FIRST = FALSE;
  if(SET && DEBUG_MODE){
    fprintf(stderr, "Open the audio device\n");
  }
 } /* WITHOUT_ANY_AUDIO */
#endif /* NO_AUDIO */  
  return(SET);
}



static UnSetAudio()
{
#ifndef NO_AUDIO
 if(! WITHOUT_ANY_AUDIO){
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
  if(DEBUG_MODE){
    fprintf(stderr, "Close the audio device\n");
  }
  soundplayterm();
#endif /* SPARCSTATION OR SGISTATION OR I386AUDIO */
 } /* ! WITHOUT_ANY_AUDIO */
#endif /* NO_AUDIO */  
}



static InitAudioStation(audio_station)
     AUDIO_TABLE audio_station;  
{
  register int i;

  for(i=0; i<N_MAX_STATIONS; i++){
    audio_station[i].tv_sec = 0;
    audio_station[i].flashed = FALSE;
  }
  return;
}



static CheckAudioStation(audio_station, tv_sec)
     AUDIO_TABLE audio_station;
     long tv_sec;
{
  register int i;
  int max=min(nb_stations, first_passive+2);

  for(i=0; i<max; i++){
    if(audio_station[i].flashed){
      if((tv_sec - audio_station[i].tv_sec) > 1){
	audio_station[i].flashed = FALSE;
	ReverseColor(host_name[i]);
      }
    }
  }
  return;
}




void ShowAudioStation(audio_station, i_station, tv_sec)
     AUDIO_TABLE audio_station;
     int i_station;
     long tv_sec;
{
  audio_station[i_station].tv_sec = tv_sec;
  if(audio_station[i_station].flashed == FALSE){
    ReverseColor(host_name[i_station]);
    audio_station[i_station].flashed = TRUE;
  }
  CheckAudioStation(audio_station, tv_sec);
  return;
}


static BOOLEAN InitDVideo(fd, i)
     int fd[2];
     int i; /* Station's index in the local table */

{
  int idv;

  if(pipe(fd) < 0){
    ShowWarning("InitDVideo: Cannot create pipe");
    return (FALSE);
  }
  if((idv=fork()) == -1){
    ShowWarning("InitDVideo: Cannot fork decoding video process");
    return (FALSE);
  }
  if(!idv){
    XtRemoveTimeOut(IdTimeOut);
    close(fd[1]);
#ifndef NO_AUDIO
   if(! WITHOUT_ANY_AUDIO){
    if(param.START_EAUDIO){
      UnSetAudio();
      close(asock_r);
      AudioQuit();
      AudioEncode(param.audio_format, MICRO_MUTED);
      XtRemoveTimeOut(IdTimeOutEncode);
    }
   } /* ! WITHOUT_ANY_AUDIO */
#endif
    close(csock_r);
    close(csock_s);
#ifndef SOLARIS
    if(nice(10) < 0){
      perror("nice'VideoDecode");
    }
#endif
    VideoDecode(fd, VIDEO_PORT, IP_GROUP, t[i].name, t[i].sin_addr,
		t[i].sin_addr_coder, t[i].identifier, param.ni, 
		t[i].feedback_port, param.display, QCIF2CIF, STAT_MODE, 
		FORCE_GREY, UNICAST);
    exit(0);
  }else{
    close(fd[0]);
  }
  return (TRUE);
}



static void AddLoginName(name, sin_addr, sin_addr_coder)
     char *name;
     u_long sin_addr;
     u_long sin_addr_coder;
{
  struct in_addr in_from, in_coder;
  char ad_coder[20], ad_from[20];
  
  if((F_login = fopen(log_file, "a")) == NULL){
    fprintf(stderr, "cannot open login file\n");
    LOG_MODE = FALSE;
  }else{
    bcopy((char *)&sin_addr_coder, (char *)&in_coder, 4);
    strcpy(ad_coder, inet_ntoa(in_coder));
    if(sin_addr == sin_addr_coder){
      fprintf(F_login, " %40s  [%s]\n", name, ad_coder);
    }else{
      bcopy((char *)&sin_addr, (char *)&in_from, 4);
      strcpy(ad_from, inet_ntoa(in_from));
      fprintf(F_login, " %40s  [%s] from [%s]\n", name, ad_coder, ad_from);
    }
    fclose(F_login);
  }
}


static void Wait(delay)
     int delay; /* in ms */
{
  static int sock;
  static BOOLEAN FIRST=TRUE;
  struct timeval timer;
  
  if(FIRST){
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      return;
    }
    FIRST = FALSE;
  }
  if(delay > 7){
    timer.tv_sec = (delay-7) / 1000;
    timer.tv_usec = ((delay-7) % 1000) * 1000;
#ifdef HPUX
    (void) select(sock, (int *)0, (int *)0, (int *)0, &timer);
#else
    (void) select(sock, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timer);
#endif
  }
  return;
}



static void CheckStations(t, time)
     LOCAL_TABLE t;
     long time;
{
  register int i;
  STATION *p;
  
  for(i=1; i<nb_stations; i++){
    p = &(t[i]);
    if(p->valid){
      if((time - p->lasttime) > 35){
	if(DEBUG_MODE){
	  fprintf(stderr, 
		"Station %s not responding\n", p->name);
	}
	p->valid = FALSE;
	XtSetSensitive(host_name[i], FALSE);
      }
    }
  }
  return;
}



static void PurgeStations(t, time)
     LOCAL_TABLE t;
     long time;
{
  register int i,j;
  STATION *p;
  
  for(i=1; i<nb_stations; i++){
    p = &(t[i]);
    if(! p->valid){
      if((time - p->lasttime) > 250){
	/* This station must be removed from the local table */
	if(p->video_decoding){
	  SendPIP_QUIT(p->dv_fd);
	  close(p->dv_fd[1]);
	}
#ifndef NO_AUDIO
	if(! WITHOUT_ANY_AUDIO){
	  if(IsAudioDecoding(p->sin_addr,p->identifier)){
	    UnSetAudioDecoding(p->sin_addr,p->identifier);
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
	    if(AUDIO_SET && !param.START_EAUDIO){
	      if(!AnyAudioDecoding()){
		UnSetAudio();
		AUDIO_SET = FALSE;
	      }
	    }
#endif 
	  }
	} /* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */	
	if(i < first_passive)
	  first_passive --;
	nb_stations --;
	for(j=i; j<nb_stations; j++){
	  if(t[j].valid != t[j+1].valid){
	    if(t[j+1].valid)
	      XtSetSensitive(host_name[j], TRUE);
	    else
	      XtSetSensitive(host_name[j], FALSE);
	  }
	  CopyStation(&t[j], &t[j+1]);
	  ChangeAndMappLabel(host_name[j], t[j].name);
	  if(IsAudioEncoding(t[j].sin_addr,t[j].identifier)){
	    if(IsAudioDecoding(t[j].sin_addr,t[j].identifier))
	      ChangeAndMappPixmap(button_d_audio[j], LabelListen);
	    else
	      ChangeAndMappPixmap(button_d_audio[j], LabelMuted);
	    XtSetMappedWhenManaged(button_d_audio[j], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_audio[j], FALSE);
	  if(t[j].video_encoding){
	    if(t[j].video_decoding)
	      ChangeAndMappPixmap(button_d_video[j], LabelShown);
	    else
	      ChangeAndMappPixmap(button_d_video[j], LabelHidden);
	    XtSetMappedWhenManaged(button_d_video[j], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_video[j], FALSE);
	}
	ResetStation(&t[j], (IMPLICIT_AUDIO_DECODING ? FALSE : TRUE));
	XtSetMappedWhenManaged(host_name[j], FALSE);
	XtSetMappedWhenManaged(button_d_audio[j], FALSE);
	XtSetMappedWhenManaged(button_d_video[j], FALSE);
	if(DEBUG_MODE){
	  fprintf(stderr, 
		"Station %s not responding, removed it from local table\n",
		  p->name);
	}
      } /* dt > 250 seconds */
    } /* p->valid == FALSE */
  } /* for (i) */
}

static void ChangeLabelConferenceSize(w, label)
     Widget w;
     char *label;
{
  char title[200];
  Arg args[1];
  
  strcpy(title, LabelControlPanel);
  strcat(title, " - ");
  strcat(title, label);
  strcat(title, " -");
  XtSetArg(args[0], XtNtitle, title);
  XtSetValues (w, args, 1);
}




#define diff_time(new_sec, new_usec,last_sec, last_usec) \
  (int)((((new_sec) - (last_sec)) * 1000) + \
	(((new_usec) - (last_usec)) / 1000))

static void LoopDeclare(null)
     int null;
{
  static int cpt=0;
  static BOOLEAN FIRST=TRUE;
  static struct timeval realtime, lasttime;
  long time;
  Widget w;
  XtPointer closure, call_data;
  int lwrite;
  u_char command[2];
  int dif;
#ifdef DEBUG_AUDIO
  static int ncpt=0;
#endif

  gettimeofday(&realtime, (struct timezone *)NULL);
  
  if((dif=diff_time(realtime.tv_sec, realtime.tv_usec, lasttime.tv_sec, 
	       lasttime.tv_usec)) > 500) {
#ifdef DEBUG_AUDIO
      if(!ncpt){
	  fprintf(stderr, "No passage !!!\n");
      }else{
	  fprintf(stderr, 
		  "%d passages en 500 ms soit ttes les %f ms\n", ncpt, 
		  (float)dif/(float)ncpt);
      }
      ncpt = 0;
#endif
    /* Every 500 millisecond */
    lasttime.tv_sec = realtime.tv_sec;
    lasttime.tv_usec = realtime.tv_usec;
    time = realtime.tv_sec;
#ifndef NO_AUDIO
    if(! WITHOUT_ANY_AUDIO)
      CheckAudioStation(audio_station, time);
#endif
    if(cpt % 60 == 59){
      CheckStations(t, time); /* Every 30s */
    }
    if(cpt % 1200 == 1199){
      PurgeStations(t, time); /* Every 10 mn */
    }
    if(cpt % 20 == 19){
      /* Every 10s */
      if(!RECV_ONLY && (param.conf_size != LARGE_SIZE)){
	if(nb_stations > nb_max_feedback){
	  if(NACK_FIR_ALLOWED){
	    NACK_FIR_ALLOWED = FALSE;
	    my_feedback_port = 0;
	    ChangeLabelConferenceSize(control_panel,
				      ConferenceSize[param.conf_size]);
	    if(param.START_EVIDEO){
	      command[0] = PIP_CONF_SIZE;
	      command[1] = MEDIUM_SIZE;
	      if((lwrite=write(param.f2s_fd[1], (char *)command, 2)) != 2){
		perror("CallbackConfSize'write ");
		return;
	      }
	    }
	  }
	}else{
	  if(!NACK_FIR_ALLOWED){
	    NACK_FIR_ALLOWED = TRUE;
	    my_feedback_port = PORT_VIDEO_CONTROL;
	    ChangeLabelConferenceSize(control_panel,
				      ConferenceSize[param.conf_size]);
	    if(param.START_EVIDEO){
	      command[0] = PIP_CONF_SIZE;
	      command[1] = SMALL_SIZE;
	      if((lwrite=write(param.f2s_fd[1], (char *)command, 2)) != 2){
		perror("CallbackConfSize'write ");
		return;
	      }
	    }
	  }
	}
      }
      
      if(param.conf_size != LARGE_SIZE || param.START_EAUDIO ||
	 param.START_EVIDEO){
	SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		       param.START_EAUDIO, my_feedback_port,
		       param.conf_size, my_sin_addr, name_user, class_name);
      }
    }
    cpt ++;
  }
  /* Every SCAN_MS=40 ms */
#ifndef NO_AUDIO  
 if(! WITHOUT_ANY_AUDIO)
   ManageAudio(AUDIO_SET,AUDIO_DROPPED);
#endif  
  if(FIRST){
    if(AUTO_SEND){
      CallbackEVideo(w, closure, call_data);
#ifndef NO_AUDIO
      if(AUDIO_SET){
	MICRO_MUTED = FALSE;
	ReverseColor(button_ptt);
	ChangeLabel(button_ptt, LabelSpeakNow);
	MICRO_TOGGLE = TRUE;
	UnSetMicroMuted();
	AUDIO_DROPPED = FALSE;
	XtSetSensitive(output_audio, FALSE);
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
	XtSetSensitive(button_larsen, FALSE);
#endif
      }
#endif
    }
    lasttime.tv_sec = realtime.tv_sec;
    lasttime.tv_usec = realtime.tv_usec;
    FIRST = FALSE;
  }
#ifdef DEBUG_AUDIO
  ncpt ++;
#endif
  IdTimeOut = XtAppAddTimeOut(app_con, SCAN_MS,
			      (XtTimerCallbackProc)CallbackLoopDeclare,
			      client_data);
  return;
}




static XtTimerCallbackProc CallbackLoopDeclare(client_data, id)
     XtPointer client_data;
     XtIntervalId *id;
{
  int null=0;
  
  LoopDeclare(null);
}


#ifndef NO_AUDIO
static XtTimerCallbackProc CallbackAudioEncode(client_data, id)
     XtPointer client_data;
     XtIntervalId *id;
{
  if (AudioEncode(param.audio_format, MICRO_MUTED))
    IdTimeOutEncode = XtAppAddTimeOut(app_con, 5,
			      (XtTimerCallbackProc)CallbackAudioEncode,
				      client_data);
}
#endif


static void ManageStation(t, name, sin_addr, sin_addr_coder, identifier,
			  feedback_port, VIDEO_ENCODING, AUDIO_ENCODING,
			  MYSELF, NEW_STATION)
     LOCAL_TABLE t;
     char *name;
     u_long sin_addr;
     u_long sin_addr_coder;
     u_short identifier;
     u_short feedback_port;
     BOOLEAN VIDEO_ENCODING, AUDIO_ENCODING, MYSELF;
     BOOLEAN *NEW_STATION;
{
  static BOOLEAN FIRST_CALL=TRUE;
  STATION *np, np_memo, *np_new,station_tmp;
  struct timeval realtime;
  long time;
  BOOLEAN FOUND=FALSE;
  BOOLEAN DECALAGE=TRUE;
  BOOLEAN ABORT_DAUDIO=FALSE;
  BOOLEAN ABORT_DVIDEO=FALSE;
  BOOLEAN ACTIVE;
  register int i;
  int i_new, audio_ignored;

  *NEW_STATION = FALSE;
  if(FIRST_CALL  && !MYSELF){
    /* The first station should be myself ... */
    return;
  }
  if(MYSELF){
    np = &t[0];
    if(FIRST_CALL){
      nb_stations++;
      InitStation(np, name, sin_addr, sin_addr_coder, identifier,
		  feedback_port, FALSE, FALSE, TRUE, (long)0);
#ifndef NO_AUDIO      
      if(! WITHOUT_ANY_AUDIO)
	NewAudioStation(sin_addr, identifier, FALSE, FALSE, TRUE);
#endif      
      ChangeAndMappLabel(host_name[0], name);
      FIRST_CALL = FALSE;
      if(LOG_MODE)
	AddLoginName(name, sin_addr, sin_addr_coder);
    }else{
#ifndef NO_AUDIO      
      if(! WITHOUT_ANY_AUDIO){
	if(IsAudioEncoding(np->sin_addr,np->identifier)!= AUDIO_ENCODING){
	  if(!AUDIO_ENCODING){
	    if(IsAudioDecoding(np->sin_addr,np->identifier)){
	      UnSetAudioEncoding(np->sin_addr,np->identifier);
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
	      if(AUDIO_SET && !param.START_EAUDIO){
		if(!AnyAudioDecoding()){
		  UnSetAudio();
		  AUDIO_SET = FALSE;
		}
	      }
#endif 
	    }
	    XtSetMappedWhenManaged(button_d_audio[0], FALSE);
	  }else{
	    if(IsAudioIgnored(np->sin_addr, np->identifier))

	      ChangeAndMappPixmap(button_d_audio[0], LabelMuted);
	    else
	      ChangeAndMappPixmap(button_d_audio[0], LabelListen);
	  }/* NOT AUDIO_ENCODING */
	  if(AUDIO_ENCODING)
	    SetAudioEncoding(np->sin_addr,np->identifier);
	  else
	    UnSetAudioEncoding(np->sin_addr,np->identifier);
	}/* AUDIO_ENCODING CHANGE */
      }/* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */      
      if(feedback_port != t[0].feedback_port){
	t[0].feedback_port = feedback_port;
      }
      if(np->video_encoding != VIDEO_ENCODING){
	if(!VIDEO_ENCODING){
	  if(np->video_decoding){
	    np->video_decoding = FALSE;
	    SendPIP_QUIT(np->dv_fd);
	    close(np->dv_fd[1]);
	  }
	  XtSetMappedWhenManaged(button_d_video[0], FALSE);
	}else{
	  ChangeAndMappPixmap(button_d_video[0], LabelHidden);
	}/* NOT VIDEO_ENCODING */
	np->video_encoding = VIDEO_ENCODING;
      }/* VIDEO_ENCODING CHANGE */
    }/* FIRST_CALL */
  }else{
    /* It is not my declaration */
    gettimeofday(&realtime, (struct timezone *)NULL);
    time = realtime.tv_sec;
    ACTIVE = VIDEO_ENCODING | AUDIO_ENCODING;
    if(ACTIVE){
      /*---------------------------*\
      *  Active station declaration *
      \*---------------------------*/
      for(i=1; i<first_passive; i++){
	np = &(t[i]);
	if(same_station(np, sin_addr, identifier)){
	  /* This station is a known active station */
	  FOUND=TRUE;
	  if(!np->valid){
	    np->valid = TRUE;
	    XtSetSensitive(host_name[i], TRUE);
	  }
	  np->lasttime = time;
#ifndef NO_AUDIO	  
	  if(! WITHOUT_ANY_AUDIO){
	    if(IsAudioEncoding(np->sin_addr,np->identifier) != AUDIO_ENCODING){
	      if(!AUDIO_ENCODING){
		/* The station stopped its audio emission */
		if(IsAudioDecoding(np->sin_addr,np->identifier)){
		  UnSetAudioDecoding(np->sin_addr,np->identifier);
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
		  if(AUDIO_SET && !param.START_EAUDIO){
		    if(!AnyAudioDecoding()){
		      UnSetAudio();
		      AUDIO_SET = FALSE;
		    }
		  }
#endif 
		}
		XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	      }else{
		/* The station started an audio emission */
		if(IsAudioIgnored(np->sin_addr,np->identifier)){
		  SetAudioDecoding(np->sin_addr,np->identifier);
		  ChangeAndMappPixmap(button_d_audio[i], LabelMuted);
		}else{
	          SetAudioDecoding(np->sin_addr,np->identifier);
		  if(AUDIO_SET)
		    ChangeAndMappPixmap(button_d_audio[i], LabelListen);
		  else{
		    ChangeAndMappPixmap(button_d_audio[i], LabelMuted);
		    UnSetAudioDecoding(np->sin_addr,np->identifier);
		  }
		}
	      }/* NOT AUDIO ENCODING */
	      if(AUDIO_ENCODING)
		SetAudioEncoding(np->sin_addr,np->identifier);
	      else
		UnSetAudioEncoding(np->sin_addr,np->identifier);
	    }/* AUDIO_ENCODING CHANGE */
	  } /* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */	  
	  if(feedback_port != t[i].feedback_port){
	    t[i].feedback_port = feedback_port;
	  }
	  if(np->video_encoding != VIDEO_ENCODING){
	    if(!VIDEO_ENCODING){
	      /* The station stopped its video emission */
	      if(np->video_decoding){
		np->video_decoding = FALSE;
		SendPIP_QUIT(np->dv_fd);
		close(np->dv_fd[1]);
	      }
	      XtSetMappedWhenManaged(button_d_video[i], FALSE);
	    }else{
	      if(IMPLICIT_VIDEO_DECODING){
		if(InitDVideo(np->dv_fd, i)){
		  np->video_decoding = TRUE;
		  ChangeAndMappPixmap(button_d_video[i], LabelShown);
		}else
		  ChangeAndMappPixmap(button_d_video[i], LabelHidden); 
	      }else{
		ChangeAndMappPixmap(button_d_video[i], LabelHidden);
	      }		
	    }/* NOT VIDEO_DECODING */
	    np->video_encoding = VIDEO_ENCODING;
	  }/* VIDEO_ENCODING CHANGE */
	  return;
	}/* Known active station declaration */
      }/* for() */
      if(!FOUND){
	/* This is an unknown active station declaration, i==first_passive. */
	np = &(t[i]);
	if(i == nb_stations){
	  /* There is no passive station, so decalage is useless. */
	  DECALAGE = FALSE;
	  nb_stations ++;
	  *NEW_STATION = TRUE;
	  if(LOG_MODE)
	    AddLoginName(name, sin_addr, sin_addr_coder);
	}else{
	  if(!same_station(np, sin_addr, identifier)){
	    /* Decalage is necessary */
	    DECALAGE=TRUE;
	    nb_stations ++;
	    CopyStation(&np_memo, np);
#ifndef NO_AUDIO	    
	    if(! WITHOUT_ANY_AUDIO){
	      if(IMPLICIT_AUDIO_DECODING)
		UnSetAudioIgnored(np->sin_addr,np->identifier);
	      else
		SetAudioIgnored(np->sin_addr,np->identifier);
	    }/* ! WITHOUT_ANY_AUDIO */
#endif	    
	  }else{
	    /* Decalage is not necessary */
	    DECALAGE=FALSE;
	  }
	}
	/* Copy the station at this place */
	ChangeAndMappLabel(host_name[i], name);
	if(np->valid == FALSE)
	  XtSetSensitive(host_name[i], TRUE);
	InitStation(np, name, sin_addr, sin_addr_coder, identifier,
		    feedback_port, AUDIO_ENCODING, VIDEO_ENCODING,
		    IsAudioIgnored(np->sin_addr,np->identifier), time);
#ifndef NO_AUDIO	
	if(! WITHOUT_ANY_AUDIO)
	  NewAudioStation(sin_addr,identifier,AUDIO_ENCODING,
			  IMPLICIT_AUDIO_DECODING,
			  IsAudioIgnored(np->sin_addr,np->identifier));
#endif	
	np_new = np;
	i_new = i;

	if(VIDEO_ENCODING){
	  if(IMPLICIT_VIDEO_DECODING){
	    if(InitDVideo(np->dv_fd, i)){
	      np->video_decoding = TRUE;
	      ChangeAndMappPixmap(button_d_video[i], LabelShown);
	    }else
	      ChangeAndMappPixmap(button_d_video[i], LabelHidden); 
	  }else{
	    ChangeAndMappPixmap(button_d_video[i], LabelHidden);
	  }		
	}/* VIDEO_ENCODING */
	
	first_passive ++;
	FOUND=FALSE;
	if(DECALAGE){
	  /* Insert the np_memo station either at the end of the list
          *  or at the position of the passive station if existed.
	  */
	  for(++i; i<nb_stations-1; i++){
	    np = &(t[i]);
	    if(same_station(np, sin_addr, identifier)){
	      FOUND=TRUE;
	      break;
	    }
	  }
	  ChangeAndMappLabel(host_name[i], np_memo.name);
	  if(FOUND)
	    audio_ignored = IsAudioIgnored(np->sin_addr,np->identifier);
	  CopyStation(&t[i], &np_memo);
	  if(np_memo.valid == FALSE){
	    XtSetSensitive(host_name[i], FALSE);
	  }
	  if(FOUND)
	    nb_stations --;
	  else{
	    *NEW_STATION = TRUE;
	    if(LOG_MODE)
	      AddLoginName(name, sin_addr, sin_addr_coder);
	  }
	}/* DECALAGE */
#ifndef NO_AUDIO	
	if(! WITHOUT_ANY_AUDIO){
	  if(FOUND)
	    if(audio_ignored)
	      SetAudioIgnored(np_new->sin_addr,np_new->identifier);
	    else
	      UnSetAudioIgnored(np_new->sin_addr,np_new->identifier);
	  if(AUDIO_ENCODING){
	    if(IsAudioIgnored(np_new->sin_addr,np_new->identifier)){
	      UnSetAudioDecoding(np_new->sin_addr,np_new->identifier);
	      ChangeAndMappPixmap(button_d_audio[i_new], LabelMuted);
	    }else{
	      SetAudioDecoding(np_new->sin_addr,np_new->identifier);
	      if(AUDIO_SET)
	        ChangeAndMappPixmap(button_d_audio[i_new], LabelListen);
	      else{
		ChangeAndMappPixmap(button_d_audio[i_new], LabelMuted);
		UnSetAudioDecoding(np_new->sin_addr,np_new->identifier);
	      }
	    }
	  }/* AUDIO_ENCODING */
	}/* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */      
      }/* NOT FOUND */
    }else{
      /*----------------------------*\
      *  Passive station declaration *
      \*----------------------------*/
      for(i=1; i<first_passive; i++){
	np = &t[i];
	if(same_station(np, sin_addr, identifier)){
	  /* This station becomes now a passive station.
	   *  We have to put it first after the last active station.
	   *  The following active stations have to move upstairs...
	   */
	  FOUND=TRUE;
	  CopyStation(&station_tmp, np);
	  break;
	}
      }/* for() */
      if(FOUND){
	/* Move upstairs the following active stations */
	audio_ignored = IsAudioIgnored(t[i].sin_addr,t[i].identifier);
	ABORT_DAUDIO = IsAudioDecoding(t[i].sin_addr,t[i].identifier);
	ABORT_DVIDEO = t[i].video_decoding;
	first_passive --;
	for(; i<first_passive; i++){
	  if(t[i].valid != t[i+1].valid){
	    if(t[i+1].valid)
	      XtSetSensitive(host_name[i], TRUE);
	    else
	      XtSetSensitive(host_name[i], FALSE);
	  }
	  CopyStation(&t[i], &t[i+1]);
	  ChangeAndMappLabel(host_name[i], t[i].name);
	  if(IsAudioEncoding(t[i].sin_addr,t[i].identifier)){
	    if(IsAudioDecoding(t[i].sin_addr,t[i].identifier))
	      ChangeAndMappPixmap(button_d_audio[i], LabelListen);
	    else
	      ChangeAndMappPixmap(button_d_audio[i], LabelMuted);
	    XtSetMappedWhenManaged(button_d_audio[i], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	  if(t[i].video_encoding){
	    if(t[i].video_decoding)
	      ChangeAndMappPixmap(button_d_video[i], LabelShown);
	    else
	      ChangeAndMappPixmap(button_d_video[i], LabelHidden);
	    XtSetMappedWhenManaged(button_d_video[i], TRUE);
	  }else
	    XtSetMappedWhenManaged(button_d_video[i], FALSE);
	}/* for() */
	/* Insert here the passive station */
	CopyStation(&t[i], &station_tmp);
	t[i].video_encoding = t[i].video_decoding = FALSE;
#ifndef NO_AUDIO	
	if(! WITHOUT_ANY_AUDIO){
	  UnSetAudioEncoding(t[i].sin_addr,t[i].identifier);
	  UnSetAudioDecoding(t[i].sin_addr,t[i].identifier);
	  if(ABORT_DAUDIO){
	    /* The station stopped its audio emission and we have to
	    *  stop its decoding.
	    */
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
	    if(AUDIO_SET && !param.START_EAUDIO){
	      if(!AnyAudioDecoding()){
		UnSetAudio();
		AUDIO_SET = FALSE;
	      }
	    }
#endif
	  }
	}/* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */
	
	if(ABORT_DVIDEO){
	  /* The station stopped its video emission and we have to
	   *  stop its decoding.
	   */
	  SendPIP_QUIT(t[i].dv_fd);
	  close(t[i].dv_fd[1]);
	}	  
	XtSetSensitive(host_name[i], TRUE);
	XtSetMappedWhenManaged(button_d_audio[i], FALSE);
	XtSetMappedWhenManaged(button_d_video[i], FALSE);
	ChangeAndMappLabel(host_name[i], name);
	return;
      }else{
	/* This station was not a known active station. */
	if(RESTRICT_SAVING)
	  return;
	for(; i<nb_stations; i++){
	  np = &(t[i]);
	  if(same_station(np, sin_addr, identifier)){
	    FOUND=TRUE;
	    break;
	  }
	}
	if(FOUND){
	  /* This station is a known passive station. */
	  if(!np->valid){
	    np->valid = TRUE;
	    XtSetSensitive(host_name[i], TRUE);
	  }
	  np->lasttime = time;
	  return;
	}else{
	  /* This station is a new passive station */
	  np = &(t[nb_stations]);
	  InitStation(np, name, sin_addr, sin_addr_coder, identifier,
		      feedback_port, FALSE, FALSE, (IMPLICIT_AUDIO_DECODING ?
						    FALSE : TRUE), time);
#ifndef NO_AUDIO	  
	  if(! WITHOUT_ANY_AUDIO)
	    NewAudioStation(sin_addr,identifier,FALSE,IMPLICIT_AUDIO_DECODING,
			    (IMPLICIT_AUDIO_DECODING ? FALSE : TRUE));
#endif	  
	  XtSetSensitive(host_name[i], TRUE);
	  ChangeAndMappLabel(host_name[nb_stations], name);
	  nb_stations ++;
	  *NEW_STATION = TRUE;
	  if(LOG_MODE)
	    AddLoginName(name, sin_addr, sin_addr_coder);
	  return;
	}/* FOUND */
      }/* FOUND */
    }/* active station */
  }/* MYSELF */
  return;
}




static void RemoveStation(t, sin_addr, identifier)
     LOCAL_TABLE t;
     u_long sin_addr;
     u_short identifier;
{
  register int i;
  STATION *p;
  BOOLEAN FOUND=FALSE;

  for(i=1; i<nb_stations; i++){
    p = &(t[i]);
    if(same_station(p, sin_addr, identifier)){
      FOUND = TRUE;
      if(p->video_decoding){
	SendPIP_QUIT(p->dv_fd);
	close(p->dv_fd[1]);
      }
#ifndef NO_AUDIO      
      if(! WITHOUT_ANY_AUDIO){
	if(IsAudioDecoding(p->sin_addr,p->identifier)){
	  UnSetAudioDecoding(p->sin_addr,p->identifier);
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
	  if(AUDIO_SET && !param.START_EAUDIO){
	    if(!AnyAudioDecoding()){
	      UnSetAudio();
	      AUDIO_SET = FALSE;
	    }
	  }
#endif 
	}
      }/* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */      
      break;
    }
  }
  if(FOUND){
#ifndef NO_AUDIO    
    if(! WITHOUT_ANY_AUDIO)
      RemoveAudioStation(t[i].sin_addr,t[i].identifier);
#endif    
    if(i < first_passive)
      first_passive --;
    nb_stations --;
    for(; i<nb_stations; i++){
      if(t[i].valid != t[i+1].valid){
	if(t[i+1].valid)
	  XtSetSensitive(host_name[i], TRUE);
	else
	  XtSetSensitive(host_name[i], FALSE);
      }
      CopyStation(&t[i], &t[i+1]);
      ChangeAndMappLabel(host_name[i], t[i].name);
#ifndef NO_AUDIO      
      if(! WITHOUT_ANY_AUDIO){
	if(IsAudioEncoding(t[i].sin_addr,t[i].identifier)){
	  if(IsAudioDecoding(t[i].sin_addr,t[i].identifier))
	    ChangeAndMappPixmap(button_d_audio[i], LabelListen);
	  else
	    ChangeAndMappPixmap(button_d_audio[i], LabelMuted);
	  XtSetMappedWhenManaged(button_d_audio[i], TRUE);
	}else
	  XtSetMappedWhenManaged(button_d_audio[i], FALSE);
      }/* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */
      if(t[i].video_encoding){
	if(t[i].video_decoding)
	  ChangeAndMappPixmap(button_d_video[i], LabelShown);
	else
	  ChangeAndMappPixmap(button_d_video[i], LabelHidden);
	XtSetMappedWhenManaged(button_d_video[i], TRUE);
      }else
	XtSetMappedWhenManaged(button_d_video[i], FALSE);
    }
    ResetStation(&t[i], (IMPLICIT_AUDIO_DECODING ? FALSE : TRUE));
    XtSetMappedWhenManaged(host_name[i], FALSE);
    XtSetMappedWhenManaged(button_d_audio[i], FALSE);
    XtSetMappedWhenManaged(button_d_video[i], FALSE);
  }else{
    if(DEBUG_MODE)
      fprintf(stderr, "Received a unknown RemoveHost Packet \n");
  }
  return;
}



static void ToggleDebug(null)
     int null;
{
  DEBUG_MODE = (DEBUG_MODE ? FALSE : TRUE);
  if(DEBUG_MODE)
    fprintf(stderr, "ivs: Debug mode turned on\n");
  else
    fprintf(stderr, "ivs: Debug mode turned off\n");
  signal(SIGUSR1, ToggleDebug);
  return;
}



static void TakeFloor(null)
     int null;
{
  Widget w;
  XtPointer closure, call_data;
  
  if(!param.START_EVIDEO)
    CallbackEVideo(w, closure, call_data);
  signal(SIGUSR1, TakeFloor);
  return;
}



static void ReleaseFloor(null)
     int null;
{
  Widget w;
  XtPointer closure, call_data;
  
  if(param.START_EVIDEO)
    CallbackEVideo(w, closure, call_data);
  XtSetMappedWhenManaged(button_e_video, FALSE);
  signal(SIGUSR2, ReleaseFloor);
  return;
}



static void SetRateMax(null)
     int null;
{
  XtPointer closure, call_data;
  Arg args[1];
  u_char data[6];
  int lwrite;
  float top;
  
  if(param.START_EVIDEO){
    data[0] = PIP_MAX_RATE;
    sprintf((char *)&data[1], "%d", param.memo_rate_max);
    if((lwrite=write(param.f2s_fd[1], (char *)data, 5)) != 5){
      perror("SetRateMax'write ");
      return;
    }
  }
  sprintf((char *)data, "%d", param.memo_rate_max);
  XtSetArg(args[0], XtNlabel, (String)data);
  XtSetValues(value_control, args, 1);
  top = (float)param.memo_rate_max / (float)param.max_rate_max;
  XawScrollbarSetThumb(scroll_control, top, 0.05);
  signal(SIGUSR1, SetRateMax);
  return;
}



static void SetRateMin(null)
     int null;
{
  XtPointer closure, call_data;
  Arg args[1];
  u_char data[6];
  int lwrite;
  float top;

  if(param.START_EVIDEO){
    data[0] = PIP_MAX_RATE;
    sprintf((char *)&data[1], "%d", param.rate_min);
    if((lwrite=write(param.f2s_fd[1], (char *)data, 5)) != 5){
      perror("SetRateMin'write ");
      return;
    }
  }
  sprintf((char *)data, "%d", param.rate_min);
  XtSetArg(args[0], XtNlabel, (String)data);
  XtSetValues(value_control, args, 1);
  top = (float)param.rate_min / (float)param.max_rate_max;
  XawScrollbarSetThumb(scroll_control, top, 0.05);
  signal(SIGUSR2, SetRateMin);
  return;
}




static void Quit(null)
     int null;
{
  register int i;

  if(RING_UP){
    XtRemoveInput(id_call_input);
    SendPIP_QUIT(fd_call);
    close(fd_call[1]);
    close(fd_answer[0]);
  }
  if(LOG_MODE){
    char date[30];
    time_t a_date;

    if((F_login = fopen(log_file, "a")) == NULL){
      fprintf(stderr, "cannot open login file %s\n", log_file);
    }else{
      a_date = time((time_t *)0);
      strcpy(date, ctime(&a_date)); 
      fprintf(F_login, "\nConcluded at: %s\n", date);
      fclose(F_login);
    }
  }
  /* Inform the group that we abort our ivs session and kill all
  *  the son processes.
  */
  if(param.conf_size != LARGE_SIZE)
    SendBYE(csock_s, &addr_s, len_addr_s);
  for(i=0; i<nb_stations; i++)
    if(t[i].video_decoding)
      SendPIP_QUIT(t[i].dv_fd);
#ifndef NO_AUDIO 
  if(! WITHOUT_ANY_AUDIO){
#ifdef HPUX
    audioTerm();
#endif /* HPUX */
    if(param.START_EAUDIO)
      AudioQuit();
  }
#endif
  if(param.START_EVIDEO){
    XtRemoveInput(id_input);
    SendPIP_QUIT(param.f2s_fd);
  }
  XtDestroyApplicationContext(app_con);
  exit(0);
}




/**************************************************
 * Callbacks
 **************************************************/


/*------------------------------------------------*\
*               IVS-TALK  callbacks                *
\*------------------------------------------------*/

void ShowAnswerCall(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  Arg         args[5];
  Widget      popup;
  Position    x, y;
  Dimension   width, height;
  Cardinal    n;
  char        msg[100];
  int         lr;

  RING_UP = FALSE;
  ChangeAndMappLabel(button_ring, LabelCallOn);
  XtRemoveInput(id_call_input);
  bzero(msg, 100);
  if((lr=read(fd_answer[0], msg, 100)) < 0){
    ShowWarning("ShowAnswerCall: read failed");
    return;
  }

  n = 0;
  XtSetArg(args[0], XtNwidth, &width); n++;
  XtSetArg(args[1], XtNheight, &height); n++;
  XtGetValues(main_form, args, (Cardinal) n);
  XtTranslateCoords(main_form, (Position) (width/2), (Position) (height/2),
		    &x, &y);
  
  n = 0;
  XtSetArg(args[n], XtNx, x); n++;
  XtSetArg(args[n], XtNy, y); n++;
  
  popup = XtCreatePopupShell("Answer", transientShellWidgetClass, main_form,
			     args, (Cardinal) n);
  
  n = 0;
  XtSetArg(args[n], XtNlabel, msg); n++;
  msg_error = XtCreateManagedWidget("MsgError", dialogWidgetClass, popup,
				    args, (Cardinal) n);
  
  XawDialogAddButton(msg_error, "OK", DestroyWarning, (XtPointer) msg_error);
  XtPopup(popup, XtGrabNone);
  return;
}



void CallbackRing(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  int id;
  
  if(!RING_UP){

    if(pipe(fd_call) < 0){
      ShowWarning("CallbackRing: Cannot create pipe");
      return;
    }
    if(pipe(fd_answer) < 0){
      ShowWarning("CallbackRing: Cannot create pipe");
      return;
    }
    if((id=fork()) == -1){
      ShowWarning("CallbackRing: Cannot fork Ring up process");
      return;
    }

    if(!id){
      char data[10];
      char msg[100];
      int lr, sock_cx;
      int fmax, mask, mask_sock, mask_pipe;
#ifndef NO_AUDIO
      if(! WITHOUT_ANY_AUDIO){
	if(param.START_EAUDIO){
	  UnSetAudio();
	  close(asock_r);
	  AudioQuit();
	  AudioEncode(param.audio_format, MICRO_MUTED);
	  XtRemoveTimeOut(IdTimeOutEncode);
	}
      }/* ! WITHOUT_ANY_AUDIO */
#endif      
      XtRemoveTimeOut(IdTimeOut);
      close(fd_call[1]);
      close(fd_answer[0]);

      /* Create the TCP socket */
      if((sock_cx = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	strcpy(msg, "CallbackRing: Cannot create socket");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
      /* Try to connect to the distant ivs_daemon */
      if(connect(sock_cx, (struct sockaddr *)&addr_dist, sizeof(addr_dist)) 
	 < 0){
	strcpy(msg, "Cannot connect to the distant ivs daemon");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
      
      mask_pipe = (1 << fd_call[0]);
      mask_sock = (1 << sock_cx);
      fmax = maxi(fd_call[0], sock_cx) + 1;
      if(!SendTalkRequest(sock_cx)){
	strcpy(msg, "CallbackRing: SendTalkRequest failed");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }

      mask = mask_pipe | mask_sock;
#ifdef HPUX
      if(select(fmax, (int *)&mask, (int *)0, (int *)0,
		(struct timeval *)0) > 0){
#else
      if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0,
		(struct timeval *)0) > 0){
#endif
	if(mask & mask_pipe){
	  /* Abort the call */
	  if(!SendTalkAbort(sock_cx)){
	    strcpy(msg, "CallbackRing: SendTalkAbort failed");
	    if(write(fd_answer[1], msg, strlen(msg)) < 0){
	      perror("CallbackRing'write");
	    }
	  }
	  exit(0);
	}else{
	  if(mask & mask_sock){
	    /* A message is arrived */
	    if((lr=read(sock_cx, data, 10)) <= 0){
	      perror("CallbackRing'read");
	      if(write(fd_answer[1], msg, 1+strlen(&msg[1])) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    }
	    switch(data[0]){
	    case CALL_ACCEPTED:
	      strcpy(msg, "IVS talk accepted by your party");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    case CALL_REFUSED:
	      strcpy(msg, "IVS talk refused. Please, retry later ...");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(0);
	    default:
	      strcpy(msg, 
                    "CallbackRing: Illegal answer received from your party");
	      if(write(fd_answer[1], msg, strlen(msg)) < 0){
		perror("CallbackRing'write");
	      }
	      exit(1);
	    }
	  }
	}
      }else{
	/* The distant ivs_daemon aborts ... */
	strcpy(msg, "CallbackRing: select failed");
	if(write(fd_answer[1], msg, strlen(msg)) < 0){
	  perror("CallbackRing'write");
	}
	exit(0);
      }
    }else{
      close(fd_call[0]);
      close(fd_answer[1]);
      id_call_input = XtAppAddInput(app_con, fd_answer[0],
				    (XtPointer)XtInputReadMask, 
				    (XtInputCallbackProc)ShowAnswerCall, NULL);
      ChangeAndMappLabel(button_ring, LabelCallOff);
      RING_UP = TRUE;
    }
  }else{
    XtRemoveInput(id_call_input);
    SendPIP_QUIT(fd_call);
    close(fd_call[1]);
    close(fd_answer[0]);
    ChangeAndMappLabel(button_ring, LabelCallOn);
    RING_UP = FALSE;
    waitpid(0, &statusp, WNOHANG);
  }
  return;
}



/*------------------------------------------------*\
*              Display rate callback               *
\*------------------------------------------------*/

void DisplayRate(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  static BOOLEAN FIRST=TRUE;
  static struct timeval tim_out;
  int fmax, mask0;
  int mask, lr, new_rate;
  float top;
  char data[LEN_DATA];
  Arg args[1];
  
  if(FIRST){
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    FIRST=FALSE;
  }
  
  mask0 = 1 << param.s2f_fd[0];
  fmax = param.s2f_fd[0] + 1;

  /* Should add in the stuff to display receiver estimates and stuff here */
  
  do{
    if((lr=read(param.s2f_fd[0], data, LEN_DATA)) != LEN_DATA){
      ShowWarning("The video encoder process suddenly aborted\n");
      SetSensitiveVideoOptions(TRUE);
      XtRemoveInput(id_input);
      close(param.f2s_fd[1]);
      close(param.s2f_fd[0]);
      param.START_EVIDEO=FALSE;
      SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		     param.START_EAUDIO, my_feedback_port, param.conf_size,
		     my_sin_addr, name_user, class_name);
      ChangeLabel(button_e_video, "Encode video");
      waitpid(0, &statusp, WNOHANG);
      break;
    }
    
    XtSetArg(args[0], XtNlabel, (String)data);
    XtSetValues(label_band2, args, 1);
    XtSetArg(args[0], XtNlabel, (String)&data[6]);
    XtSetValues(label_ref2, args, 1);
    sscanf((char *) &data[12], "%d", &new_rate);
    if(new_rate != param.rate_max){
      param.rate_max = new_rate;
      XtSetArg(args[0], XtNlabel, (String)&data[12]);
      XtSetValues(value_control, args, 1);
      top = (float)param.rate_max / (float)param.max_rate_max;
      XawScrollbarSetThumb(scroll_control, top, 0.05);
    }
    mask = mask0;
#ifdef HPUX
    if(select(fmax, (int *)&mask, (int *)0, (int *)0, &tim_out) <= 0)
#else
    if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) <= 0)
#endif
      break;
  }while(TRUE);
  
}


/*---------------------------------------------------------------------------*/



/*----------------------- Audio tuning callbacks ----------------------------*/

#ifndef NO_AUDIO
static ActionMicroToggle(widget, event, params, num_params)
     Widget   widget;
     XEvent   *event;
     String   *params;
     Cardinal *num_params;
{
  if(param.START_EAUDIO && !MICRO_PTT){  
    /* flip-flop mode */
    if(event -> type == ButtonRelease){
      if(MICRO_MUTED){
	MICRO_MUTED = FALSE;
	ReverseColor(button_ptt);
	ChangeLabel(button_ptt, LabelSpeakNow);
	MICRO_TOGGLE = TRUE;
	UnSetMicroMuted();
	if(SPEAKER && AUTO_MUTING){
	  AUDIO_DROPPED = TRUE;
	}else{
	  AUDIO_DROPPED = FALSE;
	}
	XtSetSensitive(output_audio, FALSE);
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
	XtSetSensitive(button_larsen, FALSE);
#endif
      }else{
	MICRO_MUTED = TRUE;
	ReverseColor(button_ptt);
	ChangeLabel(button_ptt, LabelPushToTalk);
	MICRO_TOGGLE = FALSE;
	SetMicroMuted();
	AUDIO_DROPPED = FALSE;
	XtSetSensitive(output_audio, TRUE);
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
	XtSetSensitive(button_larsen, TRUE);
#endif
      }/* MICRO MUTED */
    } /* Button released */
  }
}



static ActionMicro(widget, event, params, num_params)
     Widget   widget;
     XEvent   *event;
     String   *params;
     Cardinal *num_params;
{
  if(param.START_EAUDIO && !MICRO_TOGGLE){
    /* Push-button mode */
    if(event -> type == ButtonPress){
      UnSetMicroMuted();
      ChangeLabel(button_ptt, LabelSpeakNow);
      MICRO_PTT = TRUE;
      ReverseColor(button_ptt);
      if(SPEAKER && AUTO_MUTING){
	AUDIO_DROPPED = TRUE;
      }else{
	AUDIO_DROPPED = FALSE;
      }
      return;
    }
    if(event -> type == ButtonRelease){
      SetMicroMuted();
      ChangeLabel(button_ptt, LabelPushToTalk);
      ReverseColor(button_ptt);
      MICRO_PTT = FALSE;
      AUDIO_DROPPED = FALSE;
    }
  }
  return;
}
#endif /* ! NO_AUDIO */
/*---------------------------------------------------------------------------*/



/*-------------- Video and audio encoding procedures callbacks --------------*/


BOOLEAN CheckVideoBoard(board, LET_OPENED)
     int board;
     BOOLEAN LET_OPENED;
{
  int err, fd, fd_stderr;

  switch(board){
#ifdef PARALLAX
  case PARALLAX_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    if(LET_OPENED){
      if(!sun_px_local_display()){
	ShowWarning("Parallax board only runs on local display");
	err = TRUE;
      }else{
	err = sun_px_checkvideo() ? FALSE : TRUE;
      }
    }else{
      if((fd = open("/dev/tvtwo0", O_RDWR, 0)) < 0){
	err = TRUE;
      }else{
	err = FALSE;
	close(fd);
      }
    }
    dup2(fd_stderr, 2);
    close(fd_stderr);
    return (!err);
    break;
#endif /* PARALLAX */
#ifdef SUNVIDEO
  case SUNVIDEO_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    param.xil_state = xil_open();
    if(param.xil_state){
      rtvc_image = xil_create_from_device(param.xil_state, "SUNWrtvc", NULL);
      if(rtvc_image == NULL){
	xil_close(param.xil_state);
	param.xil_state = NULL;
      }else{
	xil_close(param.xil_state); /* to close rtvc_image ... */
	if(LET_OPENED){
	  param.xil_state = xil_open();
	}
      }
    }
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(param.xil_state == NULL) {
      return FALSE;
    }else{
      return TRUE;
    }
    break;
#endif /* SUNVIDEO */    
#ifdef VIGRAPIX
  case VIGRAPIX_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    if((param.vp = vigrapix_open("/dev/vigrapix0")) == NULL) {
      err = TRUE;
    }else{
      err = FALSE;
    }
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(err)
      return FALSE;
    else{
      if(LET_OPENED){
        vigrapix_set_port(param.vp, 2-param.video_port);
        if(vigrapix_get_format(param.vp)==VIGRAPIX_NOLOCK){
	  ShowWarning("Hardware VigraPix is ok, but no video signal is found");
	  err = TRUE;
        }
      }
      vigrapix_close(param.vp);
      param.vp = (vigrapix_t) 0;
      return (!err);
    }
    break;
#endif /* VIGRAPIX */      
#ifdef VIDEOPIX
  case VIDEOPIX_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    param.vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV);
    if(param.vfc_dev == NULL){
      dup2(fd_stderr, 2);
      close(fd_stderr);
      return FALSE;
    }
    vfc_set_port(param.vfc_dev, param.video_port+1);
    if(vfc_set_format(param.vfc_dev, VFC_AUTO,
		      &param.vfc_format) < 0){
#ifndef SOLARIS
      vfc_destroy(param.vfc_dev);
#else
      BOARD_OPENED=TRUE;
#endif      
      dup2(fd_stderr, 2);
      close(fd_stderr);
      return FALSE;
    }else{
      dup2(fd_stderr, 2);
      close(fd_stderr);
      param.vfc_format = vfc_video_format(param.vfc_format);
#ifdef SOLARIS
      BOARD_OPENED=TRUE;
#else
      if(!LET_OPENED){
	vfc_destroy(param.vfc_dev);
      }else{
        BOARD_OPENED=TRUE;
      }
#endif      
      return TRUE;
    }
    break;
#endif /* VIDEOPIX */      
#if defined(INDIGOVIDEO) || defined(GALILEOVIDEO) 
  case INDIGO_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    err = sgi_checkvideo();
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(!err){
      return FALSE;
    }else{
      return TRUE;
    }
    break;
  case GALILEO_BOARD: 
  case INDY_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    err = sgi_checkvideo();
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(!err){
      return FALSE;
    }else{
      return TRUE;
    }
    break;
#endif /* INDIGOVIDEO or GALILEOVIDEO */
#ifdef HP_ROPS_VIDEO
  case RASTEROPS_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    err = hp_rops_checkvideo();
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(!err){
      return FALSE;
    }else{
      return TRUE;
    }
    break;
#endif /* HP_ROPS_VIDEO */      
#ifdef VIDEOTX
  case VIDEOTX_BOARD:
    /* Any test function ??? */
    return TRUE;
    break;
#endif /* VIDEOTX */
#ifdef SCREENMACHINE
  case SCREENMACHINE_BOARD:
    fd_stderr = dup(2);
    close(2);
    open("/dev/null", O_RDONLY);
    err = sm_checkvideo();
    dup2(fd_stderr, 2);
    close(fd_stderr);
    if(!err){
      return FALSE;
    }else{
      return TRUE;
    }
    break;
#endif /* SCREENMACHINE */      
  default:
    return FALSE;
  }
}


  
void CallbackEVideo(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  int id, fd_stderr;
  Arg args[1];
  u_char data[6];
  char mess[200];
  float top;
  
  if(!param.START_EVIDEO){
    XtSetMappedWhenManaged(button_e_video, FALSE);
    if(pipe(param.f2s_fd) < 0){
      ShowWarning("CallbackEVideo: Cannot create father to son pipe");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    if(pipe(param.s2f_fd) < 0){
      ShowWarning("CallbackEVideo: Cannot create son to father pipe");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      return;
    }
    if(!(BOARD_OPENED && (param.board_selected == VIDEOPIX_BOARD))){
      switch(param.board_selected){
#ifdef ANY_VIDEO_BOARD
      case VIDEOPIX_BOARD:
      case PARALLAX_BOARD:
      case SUNVIDEO_BOARD:
      case VIGRAPIX_BOARD:
      case INDIGO_BOARD:
      case GALILEO_BOARD:
      case INDY_BOARD:
      case VIDEOTX_BOARD:
      case J300_BOARD:
      case RASTEROPS_BOARD:
      case SCREENMACHINE_BOARD:
	if(!CheckVideoBoard(param.board_selected, TRUE)){	
	  strcpy(mess, "Cannot open hardware ");
	  strcat(mess, BoardsItems[param.board_selected]);
	  ShowWarning(mess);
	  XtSetMappedWhenManaged(button_e_video, TRUE);
	  return;
	}
	break;
#endif	
      default:
	ShowWarning("Sorry, this IVS is configured without video capture");
	XtSetMappedWhenManaged(button_e_video, TRUE);
	return;
      }
    }
    if((id=fork()) == -1){
      ShowWarning(
		  "Too many processes, Cannot fork video encoding process");
      XtSetMappedWhenManaged(button_e_video, TRUE);
      switch(param.board_selected){
#ifdef VIDEOPIX
      case VIDEOPIX_BOARD:
	vfc_destroy(param.vfc_dev);
	break;
#endif
#ifdef VIGRAPIX
      case VIGRAPIX_BOARD:
	vigrapix_close(param.vp);
	param.vp = (vigrapix_t) 0;
	break;
#endif
#ifdef SUNVIDEO
      case SUNVIDEO_BOARD:
	xil_close(param.xil_state);
	break;
#endif
      default:
	break;
      }
    }
    /* fathers's process goes on , son's process starts encoding */
    if(id){
      param.START_EVIDEO = TRUE;
      close(param.f2s_fd[0]);
      close(param.s2f_fd[1]);
      id_input = XtAppAddInput(app_con, param.s2f_fd[0],
			       (XtPointer)XtInputReadMask,
			       (XtInputCallbackProc)DisplayRate, NULL);
      SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		     param.START_EAUDIO, my_feedback_port, param.conf_size,
		     my_sin_addr, name_user, class_name);
      ChangeLabel(button_e_video, "Stop video");
      SetSensitiveVideoOptions(FALSE);
      XtSetMappedWhenManaged(button_e_video, TRUE);
    }else{

      XtRemoveTimeOut(IdTimeOut);
#ifndef NO_AUDIO
      if(! WITHOUT_ANY_AUDIO){
	if(param.START_EAUDIO){
	  UnSetAudio();
	  close(asock_r);
	  AudioQuit();
	  AudioEncode(param.audio_format, MICRO_MUTED);
	  XtRemoveTimeOut(IdTimeOutEncode);
	}
      }/* ! WITHOUT_ANY_AUDIO */
#endif      
      close(csock_r);
      close(csock_s);
      close(param.f2s_fd[1]);
      close(param.s2f_fd[0]);
      param.rate_max = (param.floor_mode == FLOOR_RATE_MIN_MAX ?
			  param.rate_min : param.memo_rate_max);
      param.nb_stations = nb_stations;
      sprintf((char *)data, "%d", param.memo_rate_max);
      XtSetArg(args[0], XtNlabel, (String)data);
      XtSetValues(value_control, args, 1);
      top = (float)param.memo_rate_max / (float)param.max_rate_max;
      XawScrollbarSetThumb(scroll_control, top, 0.05);
#ifndef SOLARIS
      if(nice(10) < 0){
	perror("nice'VideoEncode");
      }
#endif
      switch(param.board_selected){
#ifdef PARALLAX
      case PARALLAX_BOARD:
	sun_px_video_encode(&param);
#endif	
#ifdef SUNVIDEO
      case SUNVIDEO_BOARD:
	sun_sunvideo_video_encode(&param);
#endif 
#ifdef VIGRAPIX
      case VIGRAPIX_BOARD:
	sun_vigrapix_video_encode(&param);
#endif 
#ifdef VIDEOPIX
      case VIDEOPIX_BOARD:
	sun_vfc_video_encode(&param);
#endif
#ifdef INDIGOVIDEO
      case INDIGO_BOARD:
	indigo_video_encode(&param);
#endif
#ifdef GALILEOVIDEO
      case GALILEO_BOARD:
      case INDY_BOARD:
	galileo_video_encode(&param);
#endif
#ifdef HP_ROPS_VIDEO
      case RASTEROPS_BOARD:
	hp_rops_video_encode(&param);
#endif
#ifdef VIDEOTX
      case VIDEOTX_BOARD:
	dec_video_encode(&param);
#endif
#ifdef SCREENMACHINE
      case SCREENMACHINE_BOARD:
	sm_video_encode(&param);
#endif
      default:
	exit(1);
      }
      exit(0);
    }
  }else{
    SetSensitiveVideoOptions(TRUE);
    XtRemoveInput(id_input);
    SendPIP_QUIT(param.f2s_fd);
    close(param.f2s_fd[1]);
    close(param.s2f_fd[0]);
    param.START_EVIDEO = FALSE;
    if(param.FREEZE)
      CallbackUnfreeze(w, closure, call_data);
    SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		   param.START_EAUDIO, my_feedback_port, param.conf_size,
		   my_sin_addr, name_user, class_name);
    ChangeLabel(button_e_video, "Encode video");
    ChangeLabel(label_band2, "_____");
    ChangeLabel(label_ref2, "_____");
    waitpid(0, &statusp, WNOHANG);
  }
}
/*---------------------------------------------------------------------------*/




/*--------------- Video and audio decoding procedures callbacks -------------*/

void CallbackDVideo(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  register int i;
  
  for(i=0; i<nb_stations; i++){
    if(w == button_d_video[i]){
      break;
    }
  }
  
  if(t[i].video_decoding == FALSE){
    if(InitDVideo(t[i].dv_fd, i)){
      t[i].video_decoding = TRUE;
      ChangePixmap(button_d_video[i], LabelShown);
    }
  }else{
    SendPIP_QUIT(t[i].dv_fd);
    close(t[i].dv_fd[1]);
    t[i].video_decoding = FALSE;
    ChangePixmap(button_d_video[i], LabelHidden);
    waitpid(0, &statusp, WNOHANG);
  }
}


#ifndef NO_AUDIO
void CallbackDAudio(w, closure, call_data)
     Widget w;
     XtPointer closure, call_data;
{
  register int i;

  if(AUDIO_SET){
    for(i=0; i<nb_stations; i++){
      if(w==button_d_audio[i]){
	break;
      }
    }
    if(IsAudioDecoding(t[i].sin_addr,t[i].identifier) == FALSE){
      SetAudioDecoding(t[i].sin_addr,t[i].identifier);
      UnSetAudioIgnored(t[i].sin_addr,t[i].identifier);
      ChangePixmap(button_d_audio[i], LabelListen);
    }else{
      UnSetAudioDecoding(t[i].sin_addr,t[i].identifier);
      SetAudioIgnored(t[i].sin_addr,t[i].identifier);
      ChangePixmap(button_d_audio[i], LabelMuted);
#if defined(SPARCSTATION) || defined(SGISTATION) || defined(I386AUDIO)
      if(!param.START_EAUDIO){
	if(!AnyAudioDecoding()){
	  UnSetAudio();
	  AUDIO_SET = FALSE;
	}
      }
#endif 
    }
  }else{
    ShowWarning("Cannot open audio device, get audio before");
  }
}
#endif /* NO_AUDIO */
/*---------------------------------------------------------------------------*/



/*--------------------------- Others callbacks ------------------------------*/

#ifdef CRYPTO
void NewKey(widget, event, params, num_params)
     Widget   widget;
     XEvent   *event;
     String   *params;
     Cardinal *num_params;
{
    Arg args[1];
    static String s_null="";
    String input_string;
    register int i;
    STATION *np;

    /* Get the key value */

    XtSetArg(args[0], XtNstring, &input_string);
    XtGetValues(key_value, args, 1);
    strncpy(key_string, input_string, LEN_MAX_STR);
    key_string[LEN_MAX_STR] = 0;
    /* Parse the input string and get current_method & key value */
    current_method = getkey(key_string, s_key, &len_key);
#ifdef DEBUG_CRYPTO
    test_key("NewKey");
#endif
    if(ENCRYPTION_ENABLED())
      XtSetSensitive(label_key, TRUE);
    else
      XtSetSensitive(label_key, FALSE);

    /* Send updates to the video coder process */
    if(param.START_EVIDEO)
      SendPIP_ENCRYPTION(param.f2s_fd);

    /* Send updates to all the video decoders */
    for(i=0; i<first_passive; i++){
      np = &(t[i]);
      if(np->video_decoding)
	SendPIP_ENCRYPTION(np->dv_fd);
    }

    /* Refresh the X string input */
    XtSetArg(args[0], XtNstring, s_null);
    XtSetValues(key_value, args, 1);
    return;
}
#endif /* CRYPTO */



#ifndef NO_AUDIO
static void AudioReceive(client_data,source,id)
     XtPointer client_data;
     int * source;
     XtInputId * id;
{
  u_long addr_em;
  u_short identifier_em;
  struct timeval realtime;
  int i;
  int i_station;
  reception(&addr_em,&identifier_em);
  if (addr_em){
    gettimeofday(&realtime, (struct timezone *)NULL);
    i_station = -1;
    for(i=0; i<nb_stations; i++){
      if((t[i].sin_addr == addr_em) && (t[i].identifier == identifier_em)){
	i_station = i;
	break;
      }
    }
    if (i_station != -1)
      ShowAudioStation(audio_station, i_station, realtime.tv_sec);
  }
}



void AudioRelease(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  register int i;
  
  if(AUDIO_SET){
    /* Release the audio port (read/write) */
    AudioQuit();
    param.START_EAUDIO = FALSE;
    AUDIO_SET = FALSE;
    UnSetAudio();
    SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		   param.START_EAUDIO, my_feedback_port, param.conf_size,
		   my_sin_addr, name_user, class_name);
    ChangeLabel(button_r_audio, LabelTakeAudio);
    XtSetSensitive(button_ptt, FALSE);
  }else{
    /* Open the audio port (r/w) */
    if(AUDIO_SET = SetAudio()){
      if(AudioEncode(param.audio_format, MICRO_MUTED)){
	IdTimeOutEncode = XtAppAddTimeOut(app_con, 5,
					  (XtTimerCallbackProc)
					  CallbackAudioEncode, client_data);
	param.START_EAUDIO=TRUE;
	restoreinitialstate();
	for(i=0; i<nb_stations; i++){
	  if(IsAudioDecoding(t[i].sin_addr,t[i].identifier)){
	    ChangePixmap(button_d_audio[i], LabelListen);
	  }else{
	    ChangePixmap(button_d_audio[i], LabelMuted);
	  }
	}
	SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		       param.START_EAUDIO, my_feedback_port,
		       param.conf_size, my_sin_addr, name_user, class_name);
      }else{
	UnSetAudio();
	AUDIO_SET = FALSE;
      }
    }
    if(!AUDIO_SET){
      ShowWarning("Cannot open the audio device");
    }else{
      ChangeLabel(button_r_audio, LabelReleaseAudio);
      XtSetSensitive(button_ptt, TRUE);
    }
  }
}
#endif /* ! NO_AUDIO */


static void GetControlInfo(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  
  int fromlen, type, lr;
  struct hostent *host;
  struct in_addr coder_from;
#define RBUFLEN 1000
  u_int buf[RBUFLEN / 4];
  char app_name[5];
  struct sockaddr_in from;
  u_long sin_addr_from, sin_addr_coder;
  u_short identifier=0;
  u_short class, feedback_port=0;
  char name[LEN_MAX_NAME];
  BOOLEAN VIDEO_ENCODING, AUDIO_ENCODING;
  BOOLEAN ERROR=FALSE, MYSELF=FALSE, BYE_RECEIVED=FALSE;
  BOOLEAN NEW_STATION;
  int lwrite, r_conf_size;
  int old_nb_stations=nb_stations;
  char command[3];
  u_int32 *pt;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *r; /* RTP options */
    
  waitpid(0, &statusp, WNOHANG);
  fromlen=sizeof(from);
  bzero((char *)&from, fromlen);
  bzero(app_name, 5);
  
  if ((lr = recvfrom(csock_r, (char *)buf, RBUFLEN, 0, 
		     (struct sockaddr *)&from, &fromlen)) < 0){
    perror("receive from");
    return;
  }
  sin_addr_from = (u_long)from.sin_addr.s_addr;

#ifdef CRYPTO
  if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
    test_key("ivs");
#endif
    lr = method[current_method].uncrypt_rtcp(
	(char *)buf, lr, s_key, len_key, (char *)buf);
  }
#endif /* CRYPTO */
  h = (rtp_hdr_t *) buf;
  if(h->ver != RTP_VERSION){
    if(DEBUG_MODE){
      fprintf(stderr, "ParseRTCP: Bad RTP version %d : ", h->ver);
      fprintf(stderr, "ver:%d, ch:%d, p:%d, s:%d, format:%d, seq:%d\n",
	      h->ver, h->channel, h->p, h->s, h->format, ntohs(h->seq));
    }
    ERROR = TRUE;
    goto after_parsing;
  }
  if(!h->p){
    if(DEBUG_MODE){
      fprintf(stderr, "ParseRTCP: RTCP without option");
    }
    ERROR = TRUE;
    goto after_parsing;
  }
  pt = (u_int32 *) (h+1);
  do{
    r = (rtp_t *) pt;
    pt += r->generic.length; /* point to end of option */
    if((((char *)pt - (char *)h) > lr) || (r->generic.length == 0)){
      if(DEBUG_MODE){
	fprintf(stderr, "ParseRTCP: Invalid length field");
      }
      ERROR = TRUE;
      goto after_parsing;
    }
    switch(type=r->generic.type) {
    case RTP_BYE:
      BYE_RECEIVED=TRUE;
      break;
    case RTP_APP:
      strncpy(app_name, (char *)&r->app_ivs.name, 4);
      if((r->app_ivs.subtype != 0) || (!streq(app_name, "IVS "))){
	if(DEBUG_MODE){
	  fprintf(stderr, 
		  "ParseRTCP: Bad subtype field %d , name: %s",
		  r->app_ivs.subtype, app_name);
	}
	ERROR = TRUE;
	break;
      }
      AUDIO_ENCODING = r->app_ivs.audio;
      VIDEO_ENCODING = r->app_ivs.video;
      r_conf_size =  r->app_ivs.conf_size;
      break;
    case RTP_SDES:
      if(r->sdes.adtype != RTP_IPv4){
	if(DEBUG_MODE){
	  fprintf(stderr, 
		  "ParseRTCP: Bad adtype field %d", r->sdes.adtype);
	}
	ERROR = TRUE;
	break;
      }
      feedback_port = r->sdes.port;
      sin_addr_coder = r->sdes.addr;
      bcopy((char *)&sin_addr_coder, (char *)&coder_from, 4);
      
      class = r->sdes.ctype;
      strcpy(name, r->sdes.name);
      if(my_sin_addr == sin_addr_coder)
	MYSELF = TRUE;
      if(class == RTP_SDES_CLASS_USER){
	strcat(name, "@");
	if((host=gethostbyaddr((char *)(&(coder_from)), 
			       sizeof(struct in_addr), from.sin_family))
	   == NULL){
	  strcat(name, inet_ntoa(coder_from));
	}else{
	  strcat(name, host->h_name);
	}
      }else{
	if(class != RTP_SDES_CLASS_TEXT){
	  if(DEBUG_MODE){
	    fprintf(stderr, "GetControlInfo: Bad class name in SDES %d ",
		    class);
	  }
	  ERROR = TRUE;
	  break;
	}
      }
      break;
    case RTP_SSRC:
      if(r->ssrc.length != 1){
	if(DEBUG_MODE){
	  fprintf(stderr, "GetControlInfo: Bad length value in SSRC %d ",
		    r->ssrc.length);
	}
	ERROR = TRUE;
	break;
      }
      identifier = ntohs(r->ssrc.id);
      break;
    default:
      if(DEBUG_MODE){
	fprintf(stderr, "ParseRTCP: Bad RTP option: %d\n", r->generic.type);
      }
      ERROR = TRUE;
      break;
    }
  }while(!r->generic.final && (!ERROR));

 after_parsing:  
  
  if(ERROR){
    if((host=gethostbyaddr((char *)(&(from.sin_addr)), 
			   sizeof(struct in_addr), from.sin_family)) ==NULL){
      strcpy(name, inet_ntoa(from.sin_addr));
    }else{
      strcpy(name, host->h_name);
    }
    if(DEBUG_MODE)
      fprintf(stderr, " from %s\n", name);
  }else{
    if(BYE_RECEIVED){
      RemoveStation(t, sin_addr_from, identifier);
    }else{
      ManageStation(t, name, sin_addr_from, sin_addr_coder, identifier,
		    feedback_port, VIDEO_ENCODING, AUDIO_ENCODING, MYSELF,
		    &NEW_STATION);
      if(r_conf_size != param.conf_size){
	if(!RECV_ONLY && (VIDEO_ENCODING || AUDIO_ENCODING) &&
	   (param.conf_size != LARGE_SIZE)){
	  /* An active station switched in LARGE CONF MODE */
	  param.conf_size = LARGE_SIZE;
	  if(NACK_FIR_ALLOWED){
	    NACK_FIR_ALLOWED = FALSE;
	    my_feedback_port = 0;
	    ChangeLabelConferenceSize(control_panel,
				      ConferenceSize[param.conf_size]);
	    if(param.START_EVIDEO){
	      command[0] = PIP_CONF_SIZE;
	      command[1] = LARGE_SIZE;
	      if((lwrite=write(param.f2s_fd[1], command, 2)) != 2){
		perror("CallbackConfSize'write ");
		return;
	      }
	    }
	  }
	}
      }   
      if(NEW_STATION && NACK_FIR_ALLOWED){
	/* Send DESCRIPTOR */
	SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		       param.START_EAUDIO, my_feedback_port,
		       param.conf_size, my_sin_addr, name_user, class_name);
      }
    }
  }
  if(old_nb_stations != nb_stations){
    if(param.START_EVIDEO){
      command[0] = PIP_NB_STATIONS;
      command[1] = (nb_stations >> 8) & 0xff;
      command[2] = nb_stations & 0xff;
      if((lwrite=write(param.f2s_fd[1], command, 3)) != 3){
	perror("CallbackNbStations'write ");
	return;
      }
    }
  }
  waitpid(0, &statusp, WNOHANG);
}



void CallbackQuit(w, call_data, client_data)
     Widget w;
     XtPointer call_data, client_data;
{
  int null=0;
  
  Quit(null);
}




/***************************************************************************
 * Main Program
 **************************************************************************/

main(argc, argv)
     int argc;
     char **argv;
{
  Widget entry;
  int i, n;
  Arg args[10];
  BOOLEAN NEW_IP_GROUP=FALSE, NEW_PORTS = FALSE;
  char hostname[256], ivs_title[256], data_ttl[5], *ptr;
  float top;
  AppliData my_data;
  int narg=1;
  u_char ttl0=0;
  u_short port_control_source;

#ifdef WAIT_DEBUG_MAIN
  {extern int wait_debug; while(!wait_debug);}
#endif

  /*
  * Initialize the top level widget
  */
  toplevel = XtAppInitialize(&app_con, "IVS", NULL, 0, &narg, argv, 
			     fallback_resources, NULL, 0);

  /*
  *  Get the default application ressources
  */
  XtGetApplicationResources(toplevel, &my_data, my_resources,
			    XtNumber(my_resources), NULL, 0);
  param.COLOR = (BOOLEAN) my_data.color;
  FORCE_GREY = (BOOLEAN) my_data.force_grey;
  QCIF2CIF = (BOOLEAN) my_data.qcif2cif;
  AUTO_SEND = (BOOLEAN) my_data.auto_send;
  param.LOCAL_DISPLAY = (BOOLEAN) my_data.local_display;
  param.PRIVILEGE_QUALITY = (BOOLEAN) my_data.privilege_quality;
  param.MAX_FR_RATE_TUNED = (BOOLEAN) my_data.max_frame_rate;
#ifdef PARALLAX
  param.px_port_video.squeeze = (int)my_data.px_squeeze;
  param.px_port_video.channel = (int)my_data.px_channel;
  param.px_port_video.standard = (int)my_data.px_standard;
#endif /* PARALLAX */ 
#ifdef VIGRAPIX
  param.secam_format = (int) my_data.secam_format;
#endif
#ifdef GALILEOVIDEO
  param.pal_format = (int) my_data.pal_format;
#endif
#ifdef SCREENMACHINE
  param.video_format = (int) my_data.video_format;
  param.video_field = (int) my_data.video_field;
  param.current_sm = (int) my_data.current_sm;
#endif 
  param.floor_mode = (int) my_data.floor_mode;
  param.conf_size = (int) my_data.conf_size;
  param.video_size = (int) my_data.size;
  param.rate_control = (int) my_data.rate_control;
  param.rate_min = (int) my_data.rate_min;
  param.rate_max = (int) my_data.rate_max;
  param.video_port = (int) my_data.port_video;
  param.brightness = (int) my_data.default_brightness;
  param.contrast = (int) my_data.default_contrast;
  param.default_quantizer = (int) my_data.default_quantizer;
  param.max_frame_rate = (int) my_data.frame_rate_max;
  param.audio_format = (int) my_data.audio_format;
  param.audio_redundancy = (int) my_data.audio_redundancy;
  record_gain = (int) my_data.default_record;
  squelch_level = (int) my_data.default_squelch;
  volume_gain = (int) my_data.default_volume;
  ttl = (u_char)  my_data.default_ttl;
  nb_max_feedback = (int) my_data.nb_max_feedback;
  param.board_selected = (int) my_data.board_selected;
  AUTO_MUTING = (BOOLEAN) my_data.auto_muting;
  IMPLICIT_VIDEO_DECODING = (BOOLEAN) my_data.implicit_video_decoding;
  IMPLICIT_AUDIO_DECODING = (BOOLEAN) my_data.implicit_audio_decoding;
  IMPLICIT_GET_AUDIO = (BOOLEAN) my_data.implicit_get_audio;
  WITHOUT_ANY_AUDIO = (BOOLEAN) my_data.without_any_audio;
  RESTRICT_SAVING = (BOOLEAN) my_data.restrict_saving;
  RECV_ONLY = (BOOLEAN) my_data.recv_only;
  LOG_MODE = (BOOLEAN) my_data.log_mode;
  STAT_MODE = (BOOLEAN) my_data.stat_mode;
  DEBUG_MODE = (BOOLEAN) my_data.debug_mode;
  if(!streq(my_data.session_name, "")){
    strcpy(name_user, my_data.session_name);
    class_name=RTP_SDES_CLASS_TEXT;
  }else{
#ifdef __NetBSD__
    strcpy(name_user, getlogin());
#else
    cuserid(name_user);
#endif
  }
  strcpy(IP_GROUP, my_data.group);
  strcpy(VIDEO_PORT, my_data.video_port);
  strcpy(AUDIO_PORT, my_data.audio_port);
  strcpy(CONTROL_PORT, my_data.control_port);
  strcpy(param.display, "");
  strcpy(param.ni, "");
#ifdef MICE_VERSION
  logprefix = getpid();
#endif  
  narg = 1;
  while(narg != argc){
    if(streq(argv[narg], "-name") || streq(argv[narg], "-N")){
      if(++narg == argc)
	Usage(argv[0]);
      if(!streq(argv[narg], "")){
	class_name=RTP_SDES_CLASS_TEXT;
	strcpy(name_user, argv[narg]);
      }else{
	class_name=RTP_SDES_CLASS_USER;
#ifdef __NetBSD__
	strcpy(name_user, getlogin());
#else
	cuserid(name_user);
#endif
      }
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-t") || streq(argv[narg], "-T")){
      if(++narg == argc)
	Usage(argv[0]);
      if(strlen(argv[narg]) > 3)
	Usage(argv[0]);
      ttl = atoi(argv[narg++]);
      if(atoi(argv[narg-1]) > 255)
	Usage(argv[0]);
      continue;
    }
#ifdef MICE_VERSION
    if(streq(argv[narg], "-lp")){
      if(++narg == argc)
	Usage(argv[0]);
      logprefix = atoi(argv[narg++]);
      MICE_STAT = TRUE;
      continue;
    }
#endif    
    if(streq(argv[narg], "-r")){
      RESTRICT_SAVING = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-a")){
      IMPLICIT_AUDIO_DECODING = FALSE;
      narg ++;
      continue;
    }    
    if(streq(argv[narg], "-start_deaf")){
      IMPLICIT_GET_AUDIO = FALSE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-deaf")){
      WITHOUT_ANY_AUDIO = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-v")){
      IMPLICIT_VIDEO_DECODING = FALSE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-echo_on")){
      AUTO_MUTING = FALSE;
      narg ++;
      continue;
    }
#ifdef CRYPTO
    if(streq(argv[narg], "-K")){
      if(++narg == argc)
	Usage(argv[0]);
      strncpy(key_string, argv[narg], LEN_MAX_STR);
      key_string[LEN_MAX_STR] = 0;
      /* Parse the input string and get current_method & key value */
      current_method = getkey(key_string, s_key, &len_key);
#ifdef DEBUG_CRYPTO
    test_key("NewKey");
#endif
      narg ++;
      continue;
    }
#endif /* CRYPTO */
    if(streq(argv[narg], "-log")){
      LOG_MODE = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-stat")){
      STAT_MODE = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-grey")){
      FORCE_GREY = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-q")){
      if(++narg == argc)
	Usage(argv[0]);
      param.default_quantizer = atoi(argv[narg++]);
      continue;
    }
    if(streq(argv[narg], "-S")){
      if(++narg == argc)
	Usage(argv[0]);
      squelch_level = atoi(argv[narg++]);
      continue;
    }
    if(streq(argv[narg], "-recvonly")){
      RECV_ONLY = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-floor1")){
      param.floor_mode = FLOOR_ON_OFF; 
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-floor2")){
      param.floor_mode = FLOOR_RATE_MIN_MAX;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-start_send")){
      AUTO_SEND = TRUE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-rate_min")){
      if(++narg == argc)
	Usage(argv[0]);
      param.rate_min = atoi(argv[narg++]);
      continue;
    }
    if(streq(argv[narg], "-rate_max")){
      if(++narg == argc)
	Usage(argv[0]);
      EXPLICIT_RATE_MAX = TRUE;
      param.rate_max = atoi(argv[narg++]);
      continue;
    }
    if(streq(argv[narg], "-qcif")){
      param.video_size = QCIF_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-cif")){
      param.video_size = CIF_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-scif")){
      param.video_size = CIF4_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-pcm")){
      param.audio_format = 0;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-adpcm")){
      param.audio_format = 1;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-vadpcm")){
      param.audio_format = 2;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-small")){
      param.conf_size = SMALL_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-medium")){
      param.conf_size = MEDIUM_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-large")){
      param.conf_size = LARGE_SIZE;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-ni")){
      if(++narg == argc)
	Usage(argv[0]);
      strcpy(param.ni, argv[narg]);
      narg++;
      continue;
    }
#ifdef SPARCSTATION    
    if(streq(argv[narg], "-vfc")){
      param.board_selected = VIDEOPIX_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-sunvideo")){
      param.board_selected = SUNVIDEO_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-vigrapix")){
      param.board_selected = VIGRAPIX_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-px")){
      param.board_selected = PARALLAX_BOARD;
      narg ++;
      continue;
    }
#endif /* SPARCSTATION */
#ifdef SGISTATION
    if(streq(argv[narg], "-indy")){
      param.board_selected = INDY_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-indigo")){
      param.board_selected = INDIGO_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-galileo")){
      param.board_selected = GALILEO_BOARD;
      narg ++;
      continue;
    }
#endif /* SGISTATION */
#ifdef HP_ROPS_VIDEO
    if(streq(argv[narg], "-rops")){
      param.board_selected = RASTEROPS_BOARD;
      narg ++;
      continue;
    }
#endif /* HP_ROPS_VIDEO */
#ifdef DECSTATION    
    if(streq(argv[narg], "-videotx")){
      param.board_selected = VIDEOTX_BOARD;
      narg ++;
      continue;
    }
    if(streq(argv[narg], "-j300")){
      param.board_selected = J300_BOARD;
      narg ++;
      continue;
    }
#endif /* DECSTATION */    
    if(argv[narg][0] == '-'){
      Usage(argv[0]);
    }
    if((ptr=strstr(argv[narg], "/")) == NULL){
      strcpy(IP_GROUP, argv[narg]);
    }else{
      *ptr++ = (char) 0;
      strcpy(IP_GROUP, argv[narg]);
      strcpy(VIDEO_PORT, ptr);
      i = atoi(VIDEO_PORT);
      sprintf(AUDIO_PORT, "%d", ++i);
      sprintf(CONTROL_PORT, "%d", ++i);
      NEW_PORTS = TRUE;
    }
    NEW_IP_GROUP = TRUE;
    narg ++;
    continue;
  }

  RATE_STAT = STAT_MODE;
  if(!EXPLICIT_RATE_MAX){
    if(ttl > 192){
      param.rate_max = 32;
    }else{
      if(ttl > 128)
	param.rate_max = 64;
    }
  }
  if(param.rate_min < 5)
    param.rate_min = 5;
  if(param.rate_max < param.rate_min)
    param.rate_max = param.rate_min;
  param.max_rate_max = (param.rate_max > 300 ? param.rate_max : 300);

  if(param.floor_mode == FLOOR_RATE_MIN_MAX){
    param.rate_control = MANUAL_CONTROL;
  }
  if(RECV_ONLY)
    AUTO_SEND = FALSE;
  NACK_FIR_ALLOWED = (param.conf_size == SMALL_SIZE ? TRUE : FALSE);
    
#ifndef ANY_VIDEO_BOARD
  RECV_ONLY = TRUE;
#endif

#ifdef SPARCSTATION
#ifndef SOLARIS
  /* Display a warning if SunOS binary is run on a Solaris platform */
{
  struct utsname name;
  
  if(uname(&name) < 0){
    perror("ivs: uname");
  }
  if(name.release[0] == '5')
    fprintf(stderr, 
    "IVS: warning, you should use the solaris binary on a Solaris platform\n");
}
#endif /* ! SOLARIS */
#endif /* SPARCSTATION */  
  if(NEW_IP_GROUP){
    unsigned int i1, i2, i3, i4;

    if(sscanf(IP_GROUP, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) == 4){
      if((i4 < 224) || (i4 > 239)){
	UNICAST = TRUE;
      }
    }else
      UNICAST = TRUE;
  }
  if(UNICAST){
    strcpy(hostname, IP_GROUP);
    if(! NEW_PORTS){
      strcpy(VIDEO_PORT, UNI_VIDEO_PORT);
      strcpy(AUDIO_PORT, UNI_AUDIO_PORT);
      strcpy(CONTROL_PORT, UNI_CONTROL_PORT);
    }
  }

  {
    /* getting my sin_addr */

    struct hostent *hp;
    struct sockaddr_in my_sock_addr;
    int namelen=100;
    char name[100];
    register int i;

    if(gethostname(name, namelen) != 0){
      perror("gethostname");
      exit(1);
    }
    if((hp=gethostbyname(name)) == 0){
	fprintf(stderr, "ivs: %d-Unknown host %s\n", getpid(), name);
	exit(1);
      }
    bcopy(hp->h_addr, (char *)&my_sock_addr.sin_addr, 4);
    my_sin_addr = my_sock_addr.sin_addr.s_addr;
  }

  if(UNICAST){
    unsigned int i1, i2, i3, i4;
    struct hostent *hp;

    strcpy(ivs_title, "ivs- correspondant:  ");
    strcat(ivs_title, hostname);
    strcat(ivs_title, "/");
    strcat(ivs_title, VIDEO_PORT);
    if(RESTRICT_SAVING){
      fprintf(stderr, "ivs: Unicast mode, -r option turned off\n");
      RESTRICT_SAVING = FALSE;
    }
    if(sscanf(hostname, "%u.%u.%u.%u", &i4, &i3, &i2, &i1) != 4){
      if((hp=gethostbyname(hostname)) == 0){
	fprintf(stderr, "ivs: %d-Unknown host %s\n", getpid(), hostname);
	exit(1);
      }
      bcopy(hp->h_addr, (char *)&addr_dist.sin_addr, 4);
      addr_dist.sin_family = AF_INET;
      addr_dist.sin_port = htons((u_short)IVSD_PORT);
      strcpy(IP_GROUP, inet_ntoa(addr_dist.sin_addr));
    }else{
      strcpy(IP_GROUP, hostname);
      addr_dist.sin_family = AF_INET;
      addr_dist.sin_addr.s_addr = (u_long)inet_addr(hostname);
      addr_dist.sin_port = htons((u_short)IVSD_PORT);
    }
  }else{
    strcpy(ivs_title, "ivs -  ");
    strcat(ivs_title, IP_GROUP);
    strcat(ivs_title, "/");
    strcat(ivs_title, VIDEO_PORT);
    strcat(ivs_title, "   ttl:");
    sprintf(data_ttl, "%d", ttl);
    strcat(ivs_title, data_ttl);
  }

/* test */
/*
#ifdef BIT_ZERO_ON_RIGHT
fprintf(stderr, "BIT_ZERO_ON_RIGHT\n");
#endif
#ifdef BIT_ZERO_ON_LEFT
fprintf(stderr, "BIT_ZERO_ON_LEFT\n");
#endif
*/
  /* Initializations */
  ResetTable(t, (IMPLICIT_AUDIO_DECODING ? FALSE : TRUE));
#ifndef NO_AUDIO
  if(! WITHOUT_ANY_AUDIO)
    RemoveAudioAll();
#endif
  param.FREEZE = FALSE;
  param.START_EVIDEO = FALSE;
  if(UNICAST)
    param.LOCAL_DISPLAY = TRUE;
  csock_r=CreateReceiveSocket(&addr_r, &len_addr_r, CONTROL_PORT, IP_GROUP,
			      UNICAST, param.ni);
  csock_s=CreateSendSocket(&addr_s, &len_addr_s, CONTROL_PORT, IP_GROUP, &ttl,
			   &port_control_source, UNICAST, param.ni);
  param.sock = CreateSendSocket(&param.addr, &vlen_addr_s, VIDEO_PORT,
				IP_GROUP, &ttl, &my_feedback_port,
				UNICAST, param.ni);
  param.memo_rate_max = param.rate_max;

  /* Selecting the default video board */

  if(!CheckVideoBoard(param.board_selected, FALSE)){
    for(i=1; i<NB_MAX_BOARD; i++){
      if(CheckVideoBoard(i, FALSE)){
	param.board_selected = i;
	break;
      }
    }
  }

  /* Ensure LOCAL_DISPLAY=TRUE if PARALLAX or VIDEOTX board */
  if((param.board_selected == PARALLAX_BOARD) || 
     (param.board_selected == VIDEOTX_BOARD)){
    param.LOCAL_DISPLAY = TRUE;
  }
#ifndef NO_AUDIO
  /* Audio initialization */
 if(! WITHOUT_ANY_AUDIO){
#ifdef SPARCSTATION
  if(!sounddevinit()){
    fprintf(stderr, "ivs: Cannot open audioctl device\n");
    AUDIO_INIT_FAILED = TRUE;
  }else{
    MICRO_MUTED = TRUE;
    switch(input_microphone = soundgetfrom()){
    case A_MICROPHONE:
      MICROPHONE = TRUE;
      break;
    default:
      MICROPHONE = FALSE;
      break;
    }
    switch(output_speaker = soundgetdest()){
    case A_HEADPHONE:
      /* Headphone mode */
      SPEAKER = FALSE;
      SetSpeaker(FALSE);
      break;
    default:
      SPEAKER = TRUE;
      SetSpeaker(AUTO_MUTING ? TRUE : FALSE);
    }
  }
  volume_gain = soundplayget();
  SetVolume(volume_gain);
  record_gain = soundrecget();
#else
  SetSpeaker(SPEAKER = TRUE);
  MICRO_MUTED = TRUE;
#endif
#ifdef HPUX
  if(!audioInit(audioserver)){
    fprintf(stderr,"ivs: Cannot connect to AudioServer\n");
    AUDIO_INIT_FAILED=TRUE;
  }
#endif
#ifdef AF_AUDIO
  SetSpeaker(SPEAKER = TRUE);
  MICRO_MUTED = TRUE;
  if(!audioInit()){
    fprintf(stderr,"ivs: Cannot connect to AudioServer\n");
    AUDIO_INIT_FAILED=TRUE;
  }else{
    sounddest(A_SPEAKER);
  }
#endif /* AF_AUDIO */
 }/* ! WITHOUT_ANY_AUDIO */
#endif /* NO_AUDIO */

  if(LOG_MODE){
#ifndef MICE_VERSION    
    int pid;
#endif    
    char temp[30];
    time_t a_date;

#ifdef MICE_VERSION
    /* Now uses either a predefined prefix on the command line, or the pid */
    sprintf(temp, "/tmp/%d%s", logprefix, LoginFileName);
#else    
    pid = getpid();
    sprintf(temp, "/tmp/%d%s", pid, LoginFileName);
#endif    
    strcat(log_file, temp);
    if((F_login=fopen(log_file, "w")) == NULL){
      LOG_MODE = FALSE;
      fprintf(stderr, "ivs: Cannot create %s file\n", log_file);
    }else{
      a_date = time((time_t *)0);
      strcpy(temp, ctime(&a_date));
      fprintf(F_login, "\t%s\n\n", ivs_title);
      fprintf(F_login, "Started at: %s\n", temp);
      fclose(F_login);
    }
  }

  /*
  *  Create all the bitmaps ...
  */
  

  mark = XCreateBitmapFromData(XtDisplay(toplevel),
			       RootWindowOfScreen(XtScreen(toplevel)),
			       Mark_bits, Mark_width, Mark_height);
  
  LabelSpeaker = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				speaker_bits, speaker_width, speaker_height);

  LabelHeadphone = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
			        headphone_bits, headphone_width,
				headphone_height);

  LabelSpeakerExt = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				speaker_ext_bits, speaker_ext_width, 
				speaker_ext_height);

  LabelMicrophone = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
			        mike_bits, mike_width,
				mike_height);

  LabelLineIn = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				mike_ext_bits, mike_ext_width, 
				mike_ext_height);

  LabelListen = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				listen_bits, listen_width, listen_height);

  LabelMuted = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				muted_bits, muted_width, muted_height);

  LabelShown = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				shown_bits, shown_width, shown_height);

  LabelHidden = XCreateBitmapFromData(XtDisplay(toplevel),
				XDefaultRootWindow(XtDisplay(toplevel)),
				hidden_bits, hidden_width, hidden_height);
  
  /* 
  * Create the main form widget.
  */

  main_form = XtCreateManagedWidget("MainForm", formWidgetClass, toplevel,
				    (ArgList)NULL, 0);
  /*
  *  Form for the panel pop-up selection.
  */
  n = 0;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  XtSetArg(args[n], XtNborderWidth, 2); n++;
  form_popup = XtCreateManagedWidget("FormPopup", formWidgetClass, main_form,
				     args, (Cardinal) n);
  /*
  * Popup panel selection.
  */
  n = 0;
  if(!RECV_ONLY){
    XtSetArg(args[n], XtNhorizDistance, 0); n++;
    button_popup1 = XtCreateManagedWidget("ButtonPopup1", commandWidgetClass,
					  form_popup, args, (Cardinal) n);
    XtAddCallback(button_popup1, XtNcallback, PopVideoGrabberPanel,
		  (XtPointer) NULL);
    n = 0;
    XtSetArg(args[n], XtNhorizDistance, 0); n++;
    XtSetArg(args[n], XtNfromHoriz, button_popup1); n++;
    button_popup2 = XtCreateManagedWidget("ButtonPopup2", commandWidgetClass,
					  form_popup, args, (Cardinal) n);
    XtAddCallback(button_popup2, XtNcallback, PopVideoCoderPanel,
		  (XtPointer) NULL);
  }
#ifndef NO_AUDIO
  if(! WITHOUT_ANY_AUDIO){
    n = 0;
    XtSetArg(args[n], XtNhorizDistance, 0); n++;
    button_popup3 = XtCreateManagedWidget("ButtonPopup3", commandWidgetClass,
					  form_popup, args, (Cardinal) n);
    XtAddCallback(button_popup3, XtNcallback, PopAudioPanel, (XtPointer) NULL);
  }
#endif /* NO_AUDIO */

  n = 0;
  XtSetArg(args[n], XtNhorizDistance, 0); n++;
  button_popup4 = XtCreateManagedWidget("ButtonPopup4", commandWidgetClass,
					form_popup, args, (Cardinal) n);
  XtAddCallback(button_popup4, XtNcallback, PopControlPanel,
		(XtPointer) NULL);
  n = 0;
  XtSetArg(args[n], XtNhorizDistance, 0); n++;
  XtSetArg(args[n], XtNfromHoriz, button_popup4); n++;
  button_popup5 = XtCreateManagedWidget("ButtonPopup5", commandWidgetClass,
				        form_popup, args, (Cardinal) n);
  XtAddCallback(button_popup5, XtNcallback, PopInfoPanel, (XtPointer) NULL);

  /*
   *  Form for start/stop audio/video encoding and quit buttons
   */
  if(!RECV_ONLY){
    n = 0;
    XtSetArg(args[n], XtNfromVert, form_popup); n++;
    XtSetArg(args[n], XtNtop, XtChainTop); n++;
    XtSetArg(args[n], XtNbottom, XtChainTop); n++;
    form_onoff = XtCreateManagedWidget("FormOnOff", formWidgetClass, main_form,
				       args, (Cardinal) n);
  }
  /*
  *  Create the start encoding buttons.
  */
  if(!RECV_ONLY){
    n = 0;
    button_e_video = XtCreateManagedWidget("ButtonEVideo", commandWidgetClass,
					   form_onoff, args, (Cardinal) n);
    XtAddCallback(button_e_video, XtNcallback, CallbackEVideo,
		  (XtPointer)NULL);
  }
#ifndef NO_AUDIO    
  if(! WITHOUT_ANY_AUDIO){
    n = 0;
    if(!RECV_ONLY){
      XtSetArg(args[n], XtNfromHoriz, button_e_video); n++;
    }
    XtSetArg(args[n], XtNlabel, LabelPushToTalk); n++;
    if(RECV_ONLY){
      button_ptt = XtCreateManagedWidget("ButtonMicro", commandWidgetClass, 
					 form_popup, args, (Cardinal) n);
    }else{
      button_ptt = XtCreateManagedWidget("ButtonMicro", commandWidgetClass, 
					 form_onoff, args, (Cardinal) n);
    }
    if(AUDIO_INIT_FAILED){
      XtSetSensitive(button_ptt, False); 
    }
  } /* ! WITHOUT_ANY_AUDIO */
#endif /* NO_AUDIO */

#if defined(CRYPTO) || !defined(NO_AUDIO)
  XtAppAddActions(app_con, actions, XtNumber(actions));
#endif

  n = 0;
  if(UNICAST){
    if(RECV_ONLY){
#ifndef NO_AUDIO
     if(! WITHOUT_ANY_AUDIO){
       XtSetArg(args[n], XtNfromHoriz, button_ptt); n++;
     } /* ! WITHOUT_ANY_AUDIO */
#endif    
    }else{
#ifndef NO_AUDIO
     if(! WITHOUT_ANY_AUDIO){
	XtSetArg(args[n], XtNfromHoriz, button_ptt); n++;
     }else{
       /* WITHOUT_ANY_AUDIO */
       XtSetArg(args[n], XtNfromHoriz, button_e_video); n++;
     }
#else
      XtSetArg(args[n], XtNfromHoriz, button_e_video); n++;
#endif
    }
    XtSetArg(args[n], XtNlabel, LabelCallOn); n++;
    XtSetArg(args[n], XtNwidth, 80); n++;
    if(RECV_ONLY){
      button_ring = XtCreateManagedWidget("ButtonRing", commandWidgetClass,
					  form_popup, args, (Cardinal) n);
    }else{
      button_ring = XtCreateManagedWidget("ButtonRing", commandWidgetClass,
					  form_onoff, args, (Cardinal) n);
    }
    XtAddCallback(button_ring, XtNcallback, CallbackRing,
		  (XtPointer)NULL);
  }

#ifndef NO_AUDIO
  if(! WITHOUT_ANY_AUDIO){
    n = 0;
    if(!RECV_ONLY){
      XtSetArg(args[n], XtNfromHoriz, button_popup2); n++;
    }else{
      if(UNICAST){
	XtSetArg(args[n], XtNfromHoriz, button_ring); n++;
      }else{
	XtSetArg(args[n], XtNfromHoriz, button_ptt); n++;
      }
    }
    XtSetValues(button_popup3, args, n);
  }
#endif    

  n = 0;
  if(RECV_ONLY){
    XtSetArg(args[n], XtNfromHoriz, button_popup5); n++;
  }else{    
    if(UNICAST){
      XtSetArg(args[n], XtNfromHoriz, button_ring); n++;
    }else{
#ifndef NO_AUDIO
     if(! WITHOUT_ANY_AUDIO){
       XtSetArg(args[n], XtNfromHoriz, button_ptt); n++;
     }else{
       /* WITHOUT_ANY_AUDIO */
       XtSetArg(args[n], XtNfromHoriz, button_e_video); n++;
     }
#else
      XtSetArg(args[n], XtNfromHoriz, button_e_video); n++;
#endif
    }
  }
  if(RECV_ONLY){
    XtSetArg(args[n], XtNhorizDistance, 1); n++;
    button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
					form_popup, (ArgList)args, n);
  }else{
    button_quit = XtCreateManagedWidget("ButtonQuit", commandWidgetClass,
					form_onoff, (ArgList)args, n);
  }
  XtAddCallback(button_quit, XtNcallback, CallbackQuit,
		(XtPointer)NULL);

  n = 0;
  if(RECV_ONLY){
    if(UNICAST){
#ifndef NO_AUDIO
      if(!WITHOUT_ANY_AUDIO){
	XtSetArg(args[n], XtNfromHoriz, button_popup3); n++;
      }else{
	XtSetArg(args[n], XtNfromHoriz, button_ring); n++;
      }
#else
      XtSetArg(args[n], XtNfromHoriz, button_ring); n++;
#endif
    }else{
	/* multicast and recv_only */
#ifndef NO_AUDIO
      if(!WITHOUT_ANY_AUDIO){
	XtSetArg(args[n], XtNfromHoriz, button_popup3); n++;      
      }
#endif
    }
  }else{
      /* ! recv_only */
#ifndef NO_AUDIO
    if(!WITHOUT_ANY_AUDIO){
      XtSetArg(args[n], XtNfromHoriz, button_popup3); n++;
    }else{
      XtSetArg(args[n], XtNfromHoriz, button_popup2); n++;
    }
#else
    XtSetArg(args[n], XtNfromHoriz, button_popup2); n++;
#endif
  }
  XtSetValues(button_popup4, args, n);
	


  /*
  *  Form for the stations.
  */
  n = 0;
  if(RECV_ONLY){
    XtSetArg(args[n], XtNfromVert, form_popup); n++;
  }else{
    XtSetArg(args[n], XtNfromVert, form_onoff); n++;
  }
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNleft, XtChainLeft); n++;
  XtSetArg(args[n], XtNdefaultDistance, 0); n++;
  XtSetArg(args[n], XtNborderWidth, 2); n++;
  form_stations = XtCreateManagedWidget("FormStations", formWidgetClass,
					main_form, args, (Cardinal) n);
  /*
  *  Create the Scroll Bar Window.
  */
  n = 0;
  XtSetArg(args[n], XtNright, main_form); n++;
  XtSetArg(args[n], XtNallowVert, TRUE); n++;
  if(UNICAST || RESTRICT_SAVING){
    XtSetArg(args[n], XtNheight, 50); n++;
  }else{
    XtSetArg(args[n], XtNheight, 150); n++;
  }
  XtSetArg(args[n], XtNfromVert, label_hosts); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNtop, XtChainTop); n++;
  XtSetArg(args[n], XtNbottom, XtChainBottom); n++;
  view_list = XtCreateManagedWidget("ViewList", viewportWidgetClass,
				    form_stations, args, (Cardinal) n);
  n = 0;
  form_list = XtCreateManagedWidget("FormList", formWidgetClass,
				    view_list, args, (Cardinal) n);
  
  n = 0;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  host_name[0] = XtCreateManagedWidget("MyHostName", labelWidgetClass, 
				       form_list, args, (Cardinal) n);
  n = 0;
  XtSetArg(args[n], XtNfromHoriz, host_name[0]); n++;
  XtSetArg(args[n], XtNbitmap, LabelHidden); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  button_d_video[0] = XtCreateManagedWidget("ButtonDVideo", 
					    commandWidgetClass, form_list,
					    args, (Cardinal) n);
  XtAddCallback(button_d_video[0], XtNcallback, CallbackDVideo,
		(XtPointer)NULL);

  n = 0;
  XtSetArg(args[n], XtNfromHoriz, button_d_video[0]); n++;
  XtSetArg(args[n], XtNbitmap, LabelMuted); n++;
  XtSetArg(args[n], XtNright, XtChainRight); n++;
  XtSetArg(args[n], XtNleft, XtChainRight); n++;
  XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
  button_d_audio[0] = XtCreateManagedWidget("ButtonDAudio", 
					    commandWidgetClass, form_list,
					    args, (Cardinal) n);
#ifndef NO_AUDIO    
  if(! WITHOUT_ANY_AUDIO)
    XtAddCallback(button_d_audio[0], XtNcallback, CallbackDAudio,
		  (XtPointer)NULL);
#endif
  if(UNICAST){
    char head[100];
    char my_hostname[80];
    BOOLEAN NEW_STATION;

    strcpy(head, name_user);
    if(class_name == RTP_SDES_CLASS_USER){
      strcat(head, "@");
      if(gethostname(my_hostname, 80) < 0)
	perror("gethostname");
      else
	strcat(head, my_hostname);
    }
    ManageStation(t, head, my_sin_addr, my_sin_addr, 0,
		  (u_short)(NACK_FIR_ALLOWED ? PORT_VIDEO_CONTROL : 0), FALSE,
		  FALSE, TRUE, &NEW_STATION);
    XtSetArg(args[0], XtNwidth, 254); 
    XtSetValues(host_name[0], args, 1);
  }

  for(i=1; i<N_MAX_STATIONS; i++){
    n = 0;
    XtSetArg(args[n], XtNfromVert, host_name[i-1]); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    host_name[i] = XtCreateManagedWidget("HostName", labelWidgetClass, 
					 form_list, args, (Cardinal) n);
    n = 0;
    XtSetArg(args[n], XtNfromHoriz, host_name[i]); n++;
    XtSetArg(args[n], XtNfromVert, host_name[i-1]); n++;
    XtSetArg(args[n], XtNbitmap, LabelHidden); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNleft, XtChainRight); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    button_d_video[i] = XtCreateManagedWidget("ButtonDVideo", 
					      commandWidgetClass, form_list,
					      args, (Cardinal) n);
    XtAddCallback(button_d_video[i], XtNcallback, CallbackDVideo,
		  (XtPointer)NULL);

    n = 0;
    XtSetArg(args[n], XtNfromHoriz, button_d_video[i]); n++;
    XtSetArg(args[n], XtNfromVert, host_name[i-1]); n++;
    XtSetArg(args[n], XtNbitmap, LabelMuted); n++;
    XtSetArg(args[n], XtNright, XtChainRight); n++;
    XtSetArg(args[n], XtNleft, XtChainRight); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    button_d_audio[i] = XtCreateManagedWidget("ButtonDAudio",
					      commandWidgetClass, form_list,
					      args, (Cardinal) n);
#ifndef NO_AUDIO
    if(!WITHOUT_ANY_AUDIO)
      XtAddCallback(button_d_audio[i], XtNcallback, CallbackDAudio,
		    (XtPointer)NULL);
#endif    
  }
    

  /*
  * Create the select input for control packets and send the first DESCRIPTOR.
  */
      
  XtAppAddInput(app_con, csock_r, (XtPointer)XtInputReadMask,
		(XtInputCallbackProc)GetControlInfo,
		NULL);

  if(param.conf_size == LARGE_SIZE){
    if(SetSockTTL(csock_s, ttl0) != 0){
      perror("setsockopt ttl0");
    }
  }
  SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		 param.START_EAUDIO, 
		 my_feedback_port, param.conf_size, my_sin_addr, name_user,
		 class_name);
  if(param.conf_size == LARGE_SIZE){
    if(SetSockTTL(csock_s, ttl) != 0){
      perror("setsockopt ttl");
    }
  }
#ifndef NO_AUDIO
  if(!WITHOUT_ANY_AUDIO){    
    /*
    * Create the audio decoding socket and its select input.
    */
  
    InitAudioStation(audio_station);
    InitAudio((IMPLICIT_AUDIO_DECODING ? FALSE : TRUE));
    InitAudioSocket(AUDIO_PORT, IP_GROUP, ttl, squelch_level, UNICAST);
    AudioSocketReceive(&asock_r);
    XtAppAddInput(app_con, asock_r, (XtPointer) XtInputReadMask, AudioReceive,
		  NULL);
  } /* ! WITHOUT_ANY_AUDIO */
#endif /* NO_AUDIO */

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, Quit);
  signal(SIGINT, Quit);
  signal(SIGQUIT, Quit);
  switch(param.floor_mode){
  case FLOOR_BY_DEFAULT:
    signal(SIGUSR1, ToggleDebug);
    break;
  case FLOOR_ON_OFF:
    XtSetMappedWhenManaged(button_e_video, FALSE);
    signal(SIGUSR1, TakeFloor);
    signal(SIGUSR2, ReleaseFloor);
    break;
  case FLOOR_RATE_MIN_MAX:
    signal(SIGUSR1, SetRateMax);
    signal(SIGUSR2, SetRateMin);
    break;    
  }

  IdTimeOut = XtAppAddTimeOut(app_con, SCAN_MS,
			      (XtTimerCallbackProc)CallbackLoopDeclare,
			      client_data);
  if(!RECV_ONLY){
    CreateVideoCoderPanel(LabelVideoCoderPanel, &video_coder_panel, toplevel);
    CreateVideoGrabberPanel(LabelVideoGrabberPanel, &video_grabber_panel,
			    toplevel);
    SetSensitiveVideoOptions(TRUE);
  }
#ifndef NO_AUDIO
  if(! WITHOUT_ANY_AUDIO){
    CreateAudioPanel(LabelAudioPanel, &audio_panel,toplevel);
#if defined(SPARCSTATION) || defined(AF_AUDIO) || defined(SGISTATION) || defined(I386AUDIO)
    XtAddCallback(button_r_audio, XtNcallback, AudioRelease,
		  (XtPointer)NULL);
#endif /* SPARCSTATION || AF_AUDIO || SGISTATION */
  }
#endif /* NO_AUDIO */
  CreateControlPanel(LabelControlPanel, &control_panel,
		     toplevel);
  SetDefaultButtons();
  ChangeLabelConferenceSize(control_panel, ConferenceSize[param.conf_size]);
  CreateInfoPanel(LabelInfoPanel, &info_panel, toplevel);

#ifndef NO_AUDIO  
  if(! WITHOUT_ANY_AUDIO){
    if(IMPLICIT_GET_AUDIO)
      AUDIO_SET = SetAudio(); 
    if(AUDIO_SET){
      if(AudioEncode(param.audio_format, MICRO_MUTED)){
	IdTimeOutEncode = XtAppAddTimeOut(app_con, 5,
					  (XtTimerCallbackProc)
					  CallbackAudioEncode, client_data);
	param.START_EAUDIO=TRUE;
	SendDESCRIPTOR(csock_s, &addr_s, len_addr_s, param.START_EVIDEO,
		       param.START_EAUDIO, my_feedback_port, param.conf_size,
		       my_sin_addr, name_user, class_name);
      }else{
	UnSetAudio();
	AUDIO_SET = FALSE;
      }
    }    
    if(!AUDIO_SET){
      ChangeLabel(button_r_audio, LabelTakeAudio);
      XtSetSensitive(button_ptt, FALSE);
    }
  } /* ! WITHOUT_ANY_AUDIO */
#endif /* ! NO_AUDIO */

  XtRealizeWidget(toplevel);
  XStoreName(XtDisplay(toplevel), XtWindow(toplevel), ivs_title);
  XtAppMainLoop(app_con);
}



