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
*  File              : identifier.c   	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Handle identifier field and SSRC option.            *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

#include "general_types.h"
#include "protocol.h"
#include "rtp.h"

extern BOOLEAN DEBUG_MODE;



u_short ManageIdentifier(tab, sin_addr, id_from, last_identifier)
     u_long tab[N_MAX_STATIONS][N_MAX_STATIONS];
     u_long sin_addr;
     u_short id_from;
     u_short *last_identifier;
{
  register int i;
  int max;
  u_long *pt_tab;
  static int next_id=0;
  static u_short id_value[N_MAX_STATIONS][N_MAX_STATIONS];

  if(id_from > N_MAX_STATIONS){
    /* error */
    return(0);
  }
  pt_tab = &tab[id_from][1];
  max = last_identifier[id_from];
  for(i=1; i<=max; i++){
    /* identifier found */
    if(*pt_tab++ == sin_addr)
      return(id_value[id_from][i]);
  }
  if(i == N_MAX_STATIONS){
    /* Too many stations */
    return(0);
  }
  *pt_tab = sin_addr;
  next_id ++;
  last_identifier[id_from] = i;
  id_value[id_from][i] = next_id;
  return(next_id);
}




void ChangeIdValue(buf, identifier, next_option)
     u_int *buf;
     u_short identifier;
     int next_option;
{
   rtp_t *r; /* RTP options */

   r = (rtp_t *) &buf[next_option];
   r->ssrc.final = 1;
   r->ssrc.id = htons(identifier);
   return;
}




void AddSSRCOption(buf, identifier, next_option, lr)
     u_int *buf;
     u_short identifier;
     int next_option;
     int *lr;
{
  u_int *pt = buf;
  rtp_hdr_t *h; /* fixed RTP header */
  rtp_t *r; /* RTP options */
  register int j;
  int max=(next_option-1);

  for(j=0; j<max; j++){
    *pt = *(pt+1);
    pt ++;
  }
  h = (rtp_hdr_t *) buf;
  h->p = 1; /* Set the P bit */
  r = (rtp_t *) &buf[max];
  r->ssrc.final = 1;
  r->ssrc.type = RTP_SSRC;
  r->ssrc.length = 1;
  r->ssrc.id = htons(identifier);
  *lr = *lr + 4;
  return;
}






