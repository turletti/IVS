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
*  File    :             	                			   *
*  Date    : 1995/06/29		           				   *
*  Author  : Andres Vega-Garcia						   *
*--------------------------------------------------------------------------*
*  Description :                                                           *
*                                                                          *
*                                                                          *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                       |           |                                      *
*                       |           |                                      *
\**************************************************************************/
#ifndef _adpcm_n_h_
#define _adpcm_n_h_

#include <sys/types.h>

typedef struct {
    short valpred;
    char index;
} dvi_stat;

#define DVI_STAT_SIZE (sizeof(short)+sizeof(char))

#endif
