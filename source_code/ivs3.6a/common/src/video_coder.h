/**************************************************************************
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
*  File              : video_coder.h                                       *
*  Author            : Andrzej Wozniak & Thierry Turletti                  *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  get_bit() macros used by the H.261 coder.                *
*                 Global definitions to video_coder*.c files               *
*                                                                          *
\**************************************************************************/

#include "endian.h"
#undef EXTERN
#ifdef DATA_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif

#ifndef _mc_header_h
#include "mc_header.h"
#endif  /* ifndef */


#define MAX_BSIZE               1000
#define LIG_MAX                 288
#define COL_MAX                 352
#define LIG_MAX_DIV2            144
#define COL_MAX_DIV2            176
#define NBLOCKMAX              1584

#if defined(SUNVIDEO) || defined (PARALLAX)
#define DEFAULT_THRESHOLD        40
#define LOW_THRESHOLD            30
#else
#define DEFAULT_THRESHOLD        30 /* 30 by default */
#define LOW_THRESHOLD            20
#endif
#define LOW_QUANTIZER             3
#define LEN_DATA                 16

#define TOLERABLE_LOSS            5

#ifdef DATA_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif

/********************  MACRODEFINITIONS *********************/

#define PUT_bit_state(s, p, b4, bc) \
{                           \
  s.ptr_chaine_codee = p;   \
  s.byte4_codee = b4;       \
  s.jj = bc;                \
			  }

#define GET_bit_state(s, p, b4, bc) \
{                           \
  p = s.ptr_chaine_codee;   \
  b4 = s.byte4_codee;       \
  bc = s.jj;                \
			  }

/* mod -pointers are faster than array lookup */
#define put_bit_ptr(octet,bit,i,j) \
    (!j ? (*octet++ |=bit, *octet=0, i++, j=7) : \
     (*octet |=(bit<< j--)))
#ifdef BIT_ZERO_ON_LEFT 
#define STORE_CODE(ptr, code) \
     *(((u_int *)ptr)++) = code
#else
#define STORE_CODE(ptr, _val_) \
{                                       \
 register  u_int ___code;               \
     ___code = _val_;                   \
     *(ptr+3) = (u_char)___code;        \
     *(ptr+2) = (u_char)(___code>>=8);  \
     *(ptr+1) = (u_char)(___code>>=8);  \
     *(ptr+0) = (u_char)(___code>>=8);  \
     ptr += 4;                          \
   }
#endif

#define PUT_ij(ptr_octet, jj) \
      ptr_octet = &chaine_codee[(i&~0x3)];\
      jj = (24-(i&0x3)*8+j+1);\
      byte4_codee = *((u_int *)ptr_octet)>>jj; \

#define cPUT_ij(ptr_octet, jj,pi,pj) \
      ptr_octet = &chaine_codee[(pi&~0x3)];\
      jj = (24-(pi&0x3)*8+pj+1);\
      byte4_codee = *((u_int *)ptr_octet)>>jj; \

#define Mput_bit_ptr(ptr_octet, bit, jj) \
    {                                            \
	/* PUT_ij(ptr_octet, jj);*/ \
	byte4_codee = (byte4_codee<<1) | bit;    \
	if(!--jj){                               \
	  STORE_CODE(ptr_octet, byte4_codee);    \
          byte4_codee = 0;                       \
	  jj = 32;                               \
	  }                                      \
	/* GET_ij(ptr_octet, jj); */\
	  }

#define Mput_value_ptr(ptr_octet, val, n_bits, jj)\
    {                                                   \
	/* PUT_ij(ptr_octet, jj);*/ \
	 if((jj -= n_bits)>=0){                         \
	byte4_codee = (byte4_codee << n_bits) | val;    \
	if(!jj){                                        \
	  STORE_CODE(ptr_octet, byte4_codee);           \
          byte4_codee = 0;                              \
	  jj = 32;                                      \
          }                                             \
	  }else{                                        \
	byte4_codee =  (byte4_codee << (n_bits+jj))     \
	               | (val >> -jj);                  \
	STORE_CODE(ptr_octet, byte4_codee);             \
	byte4_codee = val & ~(-1 << -jj);               \
        jj += 32;                                       \
	}                                               \
        /*GET_ij(ptr_octet, jj);*/\
	}


/*---------------------------------------------------------------------------*/

#define Midx_chain_forecast(idx_tab_code, offset, max, result, chain, jj, msg)\
{                                                             \
  register int _n;                                            \
  register int _val;                                          \
  register TYPE_HUFF_TAB *_ptr;                               \
    if((_n = result - offset)<0  || (result >= max)           \
       || !(_n = ((_ptr = (idx_tab_code + _n))->len)) ){      \
	fprintf(stderr, msg, result);                         \
        exit(-1);                                             \
    }else{                                                    \
      _val = _ptr->val;                                       \
	Mput_value_ptr(chain, _val, _n, jj)                   \
	}                                                     \
	}

/*---------------------------------------------------------------------------*/

#define equal_time(s1,us1,s2,us2) \
    ((s1) == (s2) ? ((us1) == (us2) ? 1 : 0) : 0)

#define min(x, y) ((x) < (y) ? (x) : (y))

#define maxi(x, y) ((x) > (y) ? (x) : (y))

/*---------------------------------------------------------------------------*/


typedef struct {
  u_char quant;
  u_char threshold;
} STEP;

typedef struct {
u_char *ptr_chaine_codee;
u_int byte4_codee;
int jj;
} TYPE_bit_state;

/*---------------------------------------------------------------------------*/

extern int tab_rec1[][255], tab_rec2[][255];

EXTERN int step_max
#ifdef DATA_MODULE
 = 6
#endif
;

EXTERN STEP Step[7]
#ifdef DATA_MODULE
#if defined(SUNVIDEO) || defined (PARALLAX)
 = {
  {3, 25}, {3, 30}, {5, 35}, {7, 40}, {9, 45}, {11, 50}, {13, 60}
}
#else
 = {
  {3, 15}, {3, 25}, {5, 25}, {7, 30}, {9, 40}, {11, 50}, {13, 60}
}
#endif
#endif
;

EXTERN int LIG[16]
#ifdef DATA_MODULE
={0, 2, 0, 1, 3, 1, 3, 2, 0, 3, 2, 1, 3, 0, 2, 1}
#endif
;
EXTERN int COL[16]
#ifdef DATA_MODULE
={0, 2, 3, 1, 0, 2, 3, 1, 2, 1, 3, 0, 2, 1, 0, 3}
#endif
;
EXTERN int MASK_CBC[6]
#ifdef DATA_MODULE
={0x20, 0x10, 0x08, 0x04, 0x02, 0x01}
#endif
;

/*---------------------------------------------------------------------------*/

/* Raw coefficient block in ZIGZAG scan order */


EXTERN unsigned char ZIG_ZAG[64]
#ifdef DATA_MODULE
= { /* zigzag offset */
          0,  4, 32, 64, 36,  8, 12, 40, 68, 96,128,100, 72, 44, 16, 20,
         48, 76,104,132,160,192,164,136,108, 80, 52, 24, 28, 56, 84,112,
        140,168,196,224,228,200,172,144,116, 88, 60, 92,120,148,176,204,
        232,236,208,180,152,124,156,184,212,240,244,216,188,220,248,252
        }
#endif
;

EXTERN BOOLEAN DEBUG_MODE
#ifdef DATA_MODULE
= FALSE
#endif
;
EXTERN BOOLEAN MODE_STAT
#ifdef DATA_MODULE
= FALSE
#endif
;
EXTERN BOOLEAN LOG_FILES_OPENED
#ifdef DATA_MODULE
= FALSE
#endif
;

EXTERN FILE *file_rate;
EXTERN double dump_rate
#ifdef DATA_MODULE
 = 0.0
#endif
;
EXTERN int n_rate
#ifdef DATA_MODULE
= 0
#endif
;
EXTERN int delta_rate;

EXTERN u_char ForceGobEncoding[52];

/*-------------------------------------------------------------------*/
extern void ToggleCoderDebug();
extern void ToggleCoderStat();
extern int ffct_8x8();
extern int ffct_intra_8x8();
extern int chroma_ffct_intra_8x8();

/*-------------------------------------------------------------------*/
EXTERN u_char *old_Y;
EXTERN u_char *old_Cb;
EXTERN u_char *old_Cr;
static int cif_size=LIG_MAX * COL_MAX;
static int qcif_size=LIG_MAX_DIV2 * COL_MAX_DIV2;

/*-------------------------------------------------------------------*/
EXTERN  u_int Buffer_chaine_codee[T_MAX/4];
#define chaine_codee ((u_char *)&Buffer_chaine_codee[0])

/*-------------------------------------------------------------------*/
#define CIF4_IMAGE_TYPE 4
#define CIF_IMAGE_TYPE  1
#define QCIF_IMAGE_TYPE 0

/*-------------------------------------------------------------------*/
EXTERN     TIMESTAMP timestamps[256];
EXTERN     int *diff_Y;
EXTERN     int *diff_Cb;
EXTERN     int *diff_Cr;

EXTERN     int min_rate_max, max_rate_max;

/*--------------------------- MCCP header -----------------------------*/
EXTERN     Mc_TxState *mcTxState;  /* Pointer to the state for tx mccp */

extern BOOLEAN MICE_STAT;
static int network_state=UNLOADED;
