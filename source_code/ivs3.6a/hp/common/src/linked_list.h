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
*  File              : linked_list.h                                       *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  Linked-lists declarations.                               *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#ifndef u_char
#define u_char   unsigned char
#define u_short  unsigned short
#define u_int    unsigned int
#endif

#define TMAX            2000
#define MEMOPACKETMAX   10   /* Max list length */
#define NIL             (MEMBUF *) 0

#define GreaterPacket(recvd, expctd) \
    ((int)((expctd) - (recvd) + 65536) % 65536 > 32768 ? TRUE : FALSE)


typedef struct membuf{
  struct membuf *prev;
  struct membuf *next;
  u_int buf[TMAX/4];
  u_short len;
  u_short sequence_number;
} MEMBUF; /* An element of a list */
