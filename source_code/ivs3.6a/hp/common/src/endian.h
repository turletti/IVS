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
*  File              : endian.h	                           		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : BIG/LITTLE ENDIAN definitions.                      *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifndef BYTE_ORDER

#include <sys/param.h>
#ifndef IPPROTO_IP
#include <netinet/in.h>
#endif

#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */

/* SGI platforms */
#ifdef _MIPSEB
#define	BYTE_ORDER	BIG_ENDIAN	/* byte order on SGI 4D */
#endif /* _MIPSEB */
#ifdef _MIPSEL
#define	BYTE_ORDER	LITTLE_ENDIAN	/* byte order on tahoe */
#endif /* _MIPSEL */

/* Linux platforms */
#ifdef __i386__
#define BYTE_ORDER      LITTLE_ENDIAN
#endif /* __i386__ */
#ifdef __mc68000__
#define BYTE_ORDER      BIG_ENDIAN
#endif /* __mc68000__ */

/* DEC platforms */
#if defined(ultrix) || defined(__alpha)
#define BYTE_ORDER      LITTLE_ENDIAN
#endif

/* Others ? */
#ifndef BYTE_ORDER
#define BYTE_ORDER      BIG_ENDIAN
#endif

#endif /* BYTE_ORDER */

#if BYTE_ORDER == LITTLE_ENDIAN
#define BIT_ZERO_ON_RIGHT
#else
#define BIT_ZERO_ON_LEFT
#endif
