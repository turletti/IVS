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
*                                                                          *
*  File              : video_decoder.h                                     *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 video decoder.                                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Pierre Delamotte    |  93/3/20  | Color display adding.                *
*   Andrzej Wozniak     |  93/10/1  | get_bit() macro changes.             *
\**************************************************************************/

#include "endian.h"
#undef EXTERN
#ifdef DATA_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif



#define ESCAPE        0
#define EOB          -1
#define L_Y         352
#define L_Y_DIV2    176
#define T_READ_MAX 4096

#define PADDING       0 /* Padding decoding (11 bits) */
#define CDI          -1 /* CDI decoding */
#define MORE0        -2 /* More than 15 zeroes */
#define Y_CCIR_MAX	240
#define MODCOLOR	4
#define MODGREY		2
#define MODNB		1

#define NMemoPacket   4

extern u_int ReadBuffer();


typedef struct{
u_char *ptr_chaine_codee;
u_int byte4_codee;
u_int chaine_count;
u_int rj;
} TYPE_dec_bit_state;


#define PUT_bit_state(str,ptr,b4,byc,bit) \
  str.byte4_codee = b4;       \
  str.ptr_chaine_codee = ptr; \
  str.chaine_count = byc;     \
  str.rj = bit

#define GET_bit_state(str,ptr,b4,byc,bit) \
  b4 = str.byte4_codee;        \
  ptr = str.ptr_chaine_codee;  \
  byc = str.chaine_count;      \
  bit = str.rj

#define expGET_bit_state(str,ptr,b4,byc,bit) \
  b4 = str.byte4_codee,        \
  ptr = str.ptr_chaine_codee,  \
  byc = str.chaine_count,      \
  bit = str.rj

#ifdef BIT_ZERO_ON_LEFT
#define LOAD_CODE(ptr_octet) \
        byte4_codee = *(((u_int*)ptr_octet)++)
#else
#define LOAD_CODE(ptr_octet)\
        byte4_codee =  *(ptr_octet+3)      | (*(ptr_octet+2)<<8)   \
                    | (*(ptr_octet+1)<<16) | (*(ptr_octet+0)<<24), \
        (u_int)(ptr_octet += 4)
#endif

#ifdef BIT_ZERO_ON_LEFT
#define LOAD_CODEn(ptr_octet, nn) \
        (byte4_codee = *(((u_int*)ptr_octet)++) & ~((1<<(32-8*nn))-1))

#else
#define LOAD_CODEn(ptr_octet, nn)\
        (byte4_codee =(  *(ptr_octet+3)      | (*(ptr_octet+2)<<8)   \
                    | (*(ptr_octet+1)<<16) | (*(ptr_octet+0)<<24))   \
                    & ~((1<<(32-8*nn))-1)),                          \
        (u_int)(ptr_octet += 4)
#endif

/*---------------------------------------------------------------------------*/

#define get_bit(ptr_octet,rj) \
  (--rj ?  ((byte4_codee<<=1)>>31) /* get from temporary register */          \
   : (/* get next word from buffer */                                         \
      (                                                                       \
       ((chaine_count-=4)>0) ?                                                \
       ( /* next word */					              \
	LOAD_CODE(ptr_octet),                                                 \
	rj = 32                                                               \
	)							              \
       : ( /* tail proceed */				                      \
	  chaine_count ?                                                      \
	  (/* get last bytes of the buffer */	                              \
	   rj =  -8*chaine_count,         		      	              \
	   chaine_count = 4,						      \
	   LOAD_CODE(ptr_octet)                                               \
	  )                                                                   \
	  : ( /* read next buffer */			                      \
	     ReadBuffer(sock, &bit_state),		                      \
         expGET_bit_state(bit_state, ptr_octet, byte4_codee, chaine_count, rj)\
	     )                                                                \
	  )                                                                   \
       ), byte4_codee>>31                                                     \
      )                                                                       \
   )

#define skip_bit(ptr_octet,rj) \
  (--rj ?  ((byte4_codee<<=1)) /* get from temporary register */              \
   : (/* get next word from buffer */                                         \
      (                                                                       \
       ((chaine_count-=4)>0) ?                                                \
       ( /* next word */				                      \
	   LOAD_CODE(ptr_octet),                                              \
	rj = 32                                                               \
	)						                      \
       : ( /* tail proceed */					              \
	  chaine_count ?                                                      \
	  (/* get last bytes of the buffer */	                              \
	   rj =  -8*chaine_count,         			              \
	   chaine_count = 4,					              \
	   LOAD_CODE(ptr_octet)                                               \
	  )                                                                   \
	  : ( /* read next buffer */			                      \
	     ReadBuffer(sock, &bit_state),		                      \
         expGET_bit_state(bit_state, ptr_octet, byte4_codee, chaine_count, rj)\
	     )                                                                \
	  )                                                                   \
       )                                                                      \
      )                                                                       \
   )

/*---------------------------------------------------------------------------*/

#define get_n_bit(ptr_octet, rj, nn, dest) \
  {  register int kk, val = 0;                \
       kk=nn;                                 \
       byte4_codee<<=1;                       \
	 /*PRINT1*/ \
	 do{                                  \
	   if(rj - kk>=1){                    \
	     val |= byte4_codee >> (32 - kk); \
	     byte4_codee <<= kk - 1;          \
	     rj -= kk;                        \
	     break;                           \
	     }                                \
	   if(rj != 1){                       \
	     val = byte4_codee >> (32 - kk);  \
	     kk -= rj - 1;                    \
	     }                                \
       if((chaine_count-=4)>0){               \
      /* next word */                              \
	   LOAD_CODE(ptr_octet);                   \
	  /*PRINT2*/                               \
	rj = 32;                                   \
	}else{                                     \
	/* tail proceed */	         	   \
	  /*PRINT3*/ \
	  if(chaine_count){                        \
	/* get last bytes of the buffer */	   \
	   rj =  -8*chaine_count;                  \
	   chaine_count = 4;                       \
	   LOAD_CODE(ptr_octet);                   \
	   }else{                                   \
	     /* read next buffer */				\
	     ReadBuffer(sock, &bit_state);		        \
             GET_bit_state(bit_state, ptr_octet, byte4_codee, chaine_count, \
			   rj); \
	     }                                                  \
	     } rj++;                                            \
	     }while(1);                                         \
	     dest = val;                                        \
	     /*PRINT4*/ \
	     }


#define skip_n_bit(ptr_octet, rj, nn) \
  {  register int kk;                         \
       kk=nn;                                 \
       byte4_codee<<=1;                       \
	 /*PRINT1*/ \
	 do{                                  \
	   if(rj - kk>=1){                    \
	     byte4_codee <<= kk - 1;          \
	     rj -= kk;                        \
	     break;                           \
	     }                                \
	   if(rj != 1){                       \
	     kk -= rj - 1;                    \
	     }                                \
       if((chaine_count-=4)>0){                \
      /* next word */                              \
	   LOAD_CODE(ptr_octet);                   \
	  /*PRINT2*/         \
	rj = 32;                                   \
	}else{                                     \
	/* tail proceed */	         	   \
	  /*PRINT3*/ \
	  if(chaine_count){                        \
	/* get last bytes of the buffer */	   \
	   rj =  -8*chaine_count;                  \
	   chaine_count = 4;                       \
	   LOAD_CODE(ptr_octet);                   \
	   }else{                                  \
	     /* read next buffer */				\
	     ReadBuffer(sock, &bit_state);		                      \
         GET_bit_state(bit_state, ptr_octet, byte4_codee, chaine_count, rj);  \
	     }                                                  \
	     } rj++;                                            \
	     }while(1);                                         \
	     /*PRINT4*/ \
	     }


/*---------------------------------------------------------------------------*/

#define equal_time(s1,us1,s2,us2) \
    ((s1) == (s2) ? ((us1) == (us2) ? 1 : 0) : 0)


#define nextp(p) ((int)((p) + 1) % 65536)


#define old_packet(recvd, expctd) \
    ((expctd - recvd + 65536) % 65536 < 32768 ? TRUE : FALSE)

/*------------------*\
* Imported variables *
\*------------------*/

/*
**  From display.c
*/

extern BOOLEAN	        CONTEXTEXIST ;
extern BOOLEAN		encCOLOR ;
extern u_char		map_lut[] ;
extern u_char		affCOLOR ;
extern int              tab_rec1[][255], tab_rec2[][255];
extern XImage 		*ximage;

/*----------------*\
* Global variables *
\*----------------*/

EXTERN int sock, fd_pipe;
EXTERN char host_coder[50];
EXTERN u_long host_sin_addr_from;
EXTERN struct sockaddr_in addr_coder;
EXTERN int len_addr_coder;
EXTERN u_short host_identifier;
EXTERN int RT
#ifdef DATA_MODULE
=0
#endif
;                /* Temporal reference */

EXTERN int last_RT_displayed
#ifdef DATA_MODULE
=0
#endif
; /* Last picture displayed */


EXTERN unsigned char ZIG[] 
#ifdef DATA_MODULE
=
  { /* Raw coefficient block in ZIGZAG scan order */
   0,0,1,2,1,0,0,1,2,3,4,3,2,1,0,0,1,2,3,4,5,6,5,4,3,2,1,0,0,1,2,3,
   4,5,6,7,7,6,5,4,3,2,1,2,3,4,5,6,7,7,6,5,4,3,4,5,6,7,7,6,5,6,7,7
  }
#endif
;

EXTERN unsigned char ZAG[] 
#ifdef DATA_MODULE
=
  { /* Column coefficient block in ZIGZAG scan order */
   0,1,0,0,1,2,3,2,1,0,0,1,2,3,4,5,4,3,2,1,0,0,1,2,3,4,5,6,7,6,5,4,
   3,2,1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,3,4,5,6,7,7,6,5,4,5,6,7,7,6,7
  }
#endif
;


EXTERN XtAppContext	dec_appCtxt;
EXTERN Widget		dec_appShell;
EXTERN Display 		*dec_dpy;
EXTERN GC 		dec_gc;
EXTERN Window 		dec_win;
EXTERN Window  		dec_icon;
EXTERN XColor 		dec_colors[256];
EXTERN int 		dec_depth
#ifdef DATA_MODULE
=0
#endif
;

EXTERN int 		dec_N_LUTH
#ifdef DATA_MODULE
=256
#endif
; 


EXTERN BOOLEAN COLOR_ENCODED;
EXTERN BOOLEAN SUPER_CIF;
EXTERN BOOLEAN FULL_INTRA_REQUEST;
EXTERN BOOLEAN FULL_INTRA; /* True if Full INTRA encoded packet */
EXTERN BOOLEAN PACKET_LOST
#ifdef DATA_MODULE
=FALSE
#endif
; /* loss detected */

EXTERN BOOLEAN AFTER_GBSC
#ifdef DATA_MODULE
=FALSE
#endif
; /* True while resynchronization is ok */

EXTERN BOOLEAN TRYING_RESYNC
#ifdef DATA_MODULE
=FALSE
#endif
;

EXTERN int cif_number;
EXTERN int format, old_format; /* Image format:
	    [000] QCIF; [001] CIF; [100] upper left corner CIF;
            [101] upper right corner CIF; [110] lower left corner CIF;
            [111] lower right corner CIF. */
EXTERN u_short sequence_number;
EXTERN int NG; /* Current GOB number */
EXTERN int NG_MAX; /* Maximum NG value */
EXTERN int L_lig, L_col;
EXTERN int L_lig2, L_col2;
EXTERN u_short old_ustime, old_stime;
EXTERN u_short ts_sec, ts_usec;
EXTERN u_short old_ts_sec, old_ts_usec;
EXTERN BOOLEAN NACK_FIR_ALLOWED;
EXTERN BOOLEAN MV_ENCODED;
EXTERN BOOLEAN STAT_MODE;

extern BOOLEAN DEBUG_MODE;

EXTERN BOOLEAN LOSS_MODE
#ifdef DATA_MODULE
=FALSE
#endif
;

EXTERN FILE *F_dloss;

EXTERN BOOLEAN ICONIC_DISPLAY 
#ifdef DATA_MODULE
= FALSE 
#endif
;

EXTERN int size ;
EXTERN u_char *Y_pt, *Cb_pt, *Cr_pt ;
EXTERN u_short memo_sequence_number; 

EXTERN int nb_lost
#ifdef DATA_MODULE
=0
#endif
;

EXTERN int nb_frame
#ifdef DATA_MODULE
=0
#endif
;

EXTERN MEMBUF memo_packet[MEMOPACKETMAX];

EXTERN u_char buffer_resynchro[] 
#ifdef DATA_MODULE
= {0,0,0,0,0,0,0,0,0,1}
#endif
;

EXTERN u_char *dec_chaine_codee;

#ifdef INPUT_FILE
EXTERN char filename[100]; /* Input H261 file name */
#endif
#ifdef SAVE_FILE
EXTERN FILE *F_decoded_file, *F_time, *F_GOB;
EXTERN struct  timeval realtime;
EXTERN double  oldtime, newtime;
EXTERN int     dim_image, cpt_image;
EXTERN BOOLEAN CIF_MODE;
EXTERN int     GOB_lost;
#endif

/*
**  From display.c
*/
extern void  build_image();
extern void  build_icon_image();

/*
**  From linked_list.c
*/
extern void InitMemBuf(), PrintList(), FreePacket();
extern BOOLEAN InsertPacket();
extern MEMBUF *TakeOutPacket(), *CreatePacket();

/*
**  From video_decoder0.c
*/
extern BOOLEAN decode_bloc_8x8();




