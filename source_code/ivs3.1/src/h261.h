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
*  File    : h261.h         	                			   *
*  Date    : May 1992		           				   *
*  Author  : Thierry Turletti						   *
*--------------------------------------------------------------------------*
*  Description :  Header file for H.261 coder and decoder.                 *
*                                                                          *
\**************************************************************************/

#include<stdio.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifdef MITSHM
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* MITSHM */

#define abs(x) ((x)>0 ? (x) : -(x))

#ifndef u_char
#define u_char unsigned char
#endif


#define L_MAX 14    /* Maximum length of mot_code */
#define L_STATE 80  /* Maximum length of state */
#define T_MAX 22000 /* Maximum length of H.261 buffer */

/* For H.261 Decoder Control Panel */
#define QUIT 0x01
#define PLAY 0x02
#define LOOP 0x03
#define STOP 0x04

/* Macroblock mode */
#define INTRA 0
#define INTER 1
#define CM    2

/* 
*  LAYERS DEFINITIONS
*/

/*************\
* BLOCK LAYER *
\*************/
typedef int BLOCK8x8[8][8];

/******************\
* MACROBLOCK LAYER *
\******************/
typedef struct {
   BLOCK8x8 P[6];   /* Y1, Y2, Y3, Y4, Cb and Cr, in this order. */
   u_char CBC;      /* CBC value */
   int QUANT;       /* Quantizer value */
   int MODE;        /* INTER or INTRA mode */
   int nb_coeff[6]; /* Number of non-zero coefficient in each block */
   int cpt_inter;
   int last_encoding;
/* int dvm_h, dvm_v; */
 } MACRO_BLOCK;

/***********************\
* GROUP OF BLOCKS LAYER *
\***********************/
typedef MACRO_BLOCK GOB[33];

/*************\
* IMAGE LAYER *
\*************/
typedef GOB IMAGE_CIF[12];

/*
*  HUFFMAN CODING DEFINITIONS ...
*/
typedef struct {
   int bit[2];
   int result[2];
 } etat;

typedef struct {
   int result;
   char chain[L_MAX];
 } mot_code;


/**************\
* MB UTILITIES *
\**************/

static int MBcol[13][34]={
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336},
	{0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 
	0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160},
	{0, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 
	176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336}
};

static int MBlig[13][34]={
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32},
	{0, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 
	80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80},
	{0, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 
	80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80},
	{0, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 
	112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
	{0, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 
	112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
	{0, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 
	160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 
	176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176},
	{0, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 
	160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 
	176, 176, 176, 176, 176, 176, 176, 176, 176, 176, 176},
	{0, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 
	208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 
	224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224},
	{0, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 
	208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 
	224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224},
	{0, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 
	272, 272, 272, 272, 272, 272, 272, 272, 272, 272, 272},
	{0, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 
	272, 272, 272, 272, 272, 272, 272, 272, 272, 272, 272}
};


