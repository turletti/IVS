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
*  File              : general_types.h	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : General types definitions.                          *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#ifndef _general_types
#define _general_types

typedef unsigned char       u_int8;  /* 8 bit unsigned int  */
typedef unsigned short      u_int16; /* 16 bit unsigned int */
typedef unsigned int        u_int32; /* 32 bit unsigned int */

typedef char                int8;  /* 8 bit unsigned int  */
typedef short               int16; /* 16 bit unsigned int */
typedef int                 int32; /* 32 bit unsigned int */

typedef u_int8             BOOLEAN; /* 32 bits unsigned int */

#ifndef TRUE
#define TRUE                1
#endif /* TRUE */
#ifndef FALSE                   
#define FALSE               0
#endif /* FALSE */


#endif /* _general_types */
