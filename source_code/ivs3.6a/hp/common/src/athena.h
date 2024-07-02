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
*  File              : athena.h    	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/3/9                                            *
*--------------------------------------------------------------------------*
*  Description       : Some initializations for athena.c module.           *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
\**************************************************************************/



typedef char BoardName3[3][20];
typedef char BoardName4[4][20];
typedef char BoardName6[6][20];

#ifdef SUNVIDEO
#define VIDEO_S      0
#define COMPOSITE_1  1
#define COMPOSITE_2  2

static BoardName3 SunVideoBoardItems = {
  "Composite 1", "Composite 2", "  S-Video  ", 
};
#define SUNVIDEO_BOARD_LEN 3
#endif /* SUNVIDEO */

#if defined(PARALLAX) || defined(VIGRAPIX)
#define CH_AUTO      0
#define CH_1         1
#define CH_2         2
#define NTSC         3
#define PAL          4
#define SECAM        5
#define AUTO_PAL     6
#define AUTO_SECAM   7
#define SQUEEZE      8
#define MIRROR_X     9
#define MIRROR_Y     10

static BoardName3 ParallaxBoardItems = {
  "Auto Select", " Channel 1  ", " Channel 2  ",
};
#define PX_BOARD_LEN 3

static char *ParallaxFormatItems[] = {
  "", "", "", "NTSC", "PAL", "SECAM", "NTSC/PAL", "NTSC/SECAM",
  "Squeeze", "Mirror X", "Mirror Y"
  };
#define PX_FORMAT_LEN 11

#endif /* PARALLAX || VIGRAPIX */

#ifdef VIDEOPIX
static BoardName3 VideoPixBoardItems = {
  "   Port 1   ", "   Port 2   ", "  S-Video  " 
};
#define VIDEOPIX_BOARD_LEN 3
#endif /* VIDEOPIX */

#ifdef VIGRAPIX
#define S_VIDEO_1  0
#define S_VIDEO_2  1
#define COMPOSITE  2
static BoardName3 VigraPixBoardItems = {
  " Composite  ", " S_Video 1  ", " S_Video 2  ",  
};
#define VIGRAPIX_BOARD_LEN 3
#endif /* VIGRAPIX */

#ifdef VIDEOTX
#define COMPOSITE_1  0
#define COMPOSITE_2  1
#define COMPOSITE_3  2
static BoardName3 VideoTXBoardItems = {
  "   Port 1   ", "   Port 2   ", "  Port 3  " 
};
#define VIDEOTX_BOARD_LEN 3
#endif /* VIDEOTX */

#ifdef J300
#define COMPOSITE_1  0
#define COMPOSITE_2  1
#define COMPOSITE_3  2
static BoardName3 J300BoardItems = {
  "   Port 1   ", "   Port 2   ", "  Port 3  " 
};
#define J300_BOARD_LEN 3
#endif /* J300 */

#ifdef HP_ROPS_VIDEO
#define COMPOSITE_1  0
#define COMPOSITE_2  1
#define COMPOSITE_3  2
static BoardName3 HPRopsBoardItems = {
  "   Port 1   ", "   Port 2   ", "  Port 3  " 
};
#define HP_ROPS_VIDEO_BOARD_LEN 3
#endif /* HP_ROPS_VIDEO */

#ifdef SCREENMACHINE
#define BLACK   0
#define RED     1
#define YELLOW  2
#define SVHS    3
static BoardName4 ScreenMachineBoardItems = {
  "BLACK", " RED ", "YELLOW", "S-VHS"
};
#define SCREENMACHINE_BOARD_LEN 4
static char *ScreenMachineFormatItems[] = {
    "NTSC", "PAL", "SECAM"
};
#define SCREENMACHINE_FORMAT_LEN 3
static char *ScreenMachineFieldItems[] = {
    "Freeze", "Even", "Odd", "Both"
};
#define SCREENMACHINE_FIELD_LEN 4
static char *ScreenMachineScreenItems[] = {
    "SM 1", "SM 2", "SM 3", "SM 4"
};
#define SCREENMACHINE_SCREEN_LEN 4
#endif /* SCREENMACHINE */

#if defined(INDIGOVIDEO) || defined(GALILEOVIDEO)
#define COMPOSITE_1  0
#define COMPOSITE_2  1
#define COMPOSITE_3  2
#define S_VIDEO_1    3
#define S_VIDEO_2    4
#define S_VIDEO_3    5

static BoardName6 SGIBoardItems = {
  "Composite 1", "Composite 2", "Composite 3", " S-Video 1 ",
  " S-Video 2 ", " S-Video 3 "
  };
#define SGI_BOARD_LEN 6
#endif /* INDIGO_VIDEO || GALILEOVIDEO */

#ifdef INDYVIDEO
#define INDYCAM      0
#define COMPOSITE    1
#define VIDEO_S      2

static BoardName6 IndyBoardItems = {
  "  IndyCam  ", " Composite ", "  S-Video  ", " S-Video 1 ",
  " S-Video 2 ", " S-Video 3 "
};
#define INDY_BOARD_LEN 3
#endif /* INDYVIDEO */

static char *AudioFormatItems[] = {
  " PCM ", "ADPCM-6", "ADPCM-5", "ADPCM-4", 
  "ADPCM-3", "ADPCM-2","VADPCM", "LPC", "GSM-13"
};
#define AUDIO_FORMAT_LEN (sizeof(AudioFormatItems)/sizeof(AudioFormatItems[0]))
                                /* number of different formats */

static char *AudioRedondanceItems[] = {
  " PCM ", "ADPCM-6", "ADPCM-5", "ADPCM-4", 
  "ADPCM-3", "ADPCM-2", "VADPCM", 
  "LPC", "GSM-13", "NONE"
};
#define AUDIO_REDONDANCE_LEN \
(sizeof(AudioRedondanceItems)/sizeof(AudioRedondanceItems[0]))


static char *ControlFormatItems[] = {
  "Automatic", "Manual", "Off"
};
#define RATE_CONTROL_FORMAT_LEN 3

static char *ModeFormatItems[] = {
  "Quality", "Frame Rate"
};
#define MODE_FORMAT_LEN 2

static char *FrameControlFormatItems[] = {
  "  Off   ", "   On   "
};
#define FR_CONTROL_FORMAT_LEN 2

#define LabelAudio        "Coding:"
#define LabelRedondance   "Redundancy:"
#define LabelRate         "Bandwidth control:"
#define LabelFrame        "Frame rate limitation:"
#define LabelMode         "Video Privilege mode:"

#define LabelZoomx2       "  Zoom x 2  "
#define LabelZoomx1       "Default size"
#define LabelZooms2       " Zoom x 1/2 "
#define LabelColor        "  Color "
#define LabelGrey         "  Grey  "
#define LabelDismiss      " Dismiss"

#define MaxVolume         100
#define MaxRecord         100

#define TitleLocalDisplay "Video from Local display"
