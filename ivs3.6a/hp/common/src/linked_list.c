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
*  File              : linked_list.c                                       *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  Linked-lists management.                                 *
*                 Lists are used to resequence packets received.           *
*                 Two lists are managed :                                  *
*                   - memo_packet to memorize out of sequence packets.     *
*                   - free_packet to keep free elements.                   *
*                 At the Initialization, all elements are in the           *
*                 free_packet list.                                        *
*                 Procedures & Functions are described below.              *
*                                                                          *
*   Each element of the list is composed as following:                     *
*                                                                          *
*    - len :             RTP packet length                                 *
*    - sequence_number : RTP sequence number                               *
*    - buf:              RTP packet                                        *
*    - prev:             pointer to the previous packet saved              *
*    - next:             pointer to the following packet saved             *
*                                                                          *
*   The sequence number field is used to sort the list (increasing order)  *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/


#include <stdio.h>
#include <math.h>
#include "linked_list.h"

#define BOOLEAN         u_char
#define TRUE            1
#define FALSE           0



void InitMemBuf(), PrintList(), FreePacket(), TestList();
BOOLEAN InsertPacket();
MEMBUF *TakeOutPacket(), *CreatePacket();

void TestList()

{
  MEMBUF memo_packet[MEMOPACKETMAX];
  MEMBUF *pt_free_first, *pt_free_last;
  MEMBUF *pt_memo_first, *pt_memo_last;
  u_short min_memo_seq;
  int nb_free_packet, nb_memo_packet;
  int nb_max = 4;
  u_short cpt=0;
  int len = 500;
  static u_short seq[] = {1000, 1010, 1000, 1005, 1003};

  MEMBUF *packet;

  InitMemBuf(memo_packet, &pt_free_first, &pt_free_last, &pt_memo_first,
	     &pt_memo_last, nb_max, &nb_free_packet, &nb_memo_packet);
  PrintList(stderr, pt_memo_first, "memo_packet", nb_memo_packet);
  PrintList(stderr, pt_free_first, "free_packet", nb_free_packet);

  while(nb_free_packet){
    
    packet = CreatePacket(seq[cpt], len, &pt_free_first, &pt_free_last,
			       &nb_free_packet);
    len +=100;
    cpt ++;
    PrintList(stderr, pt_free_first, "free_packet", nb_free_packet);
    if(!InsertPacket(&pt_memo_first, &pt_memo_last, packet, &min_memo_seq,
		     &nb_memo_packet)){
#ifdef DEBUG      
      fprintf(stderr, "duplicated packet %d\n", packet->sequence_number);
#endif      
      FreePacket(packet, &pt_free_first, &pt_free_last, &nb_free_packet);
    }
#ifdef DEBUG    
    fprintf(stderr, "min_saved : %d\n", min_memo_seq);
#endif    
    PrintList(stderr, pt_memo_first, "memo_packet", nb_memo_packet);
  }
  
  while((packet = TakeOutPacket(&pt_memo_first, &pt_memo_last, &min_memo_seq,
				&nb_memo_packet))
	 != NIL){
#ifdef DEBUG    
    fprintf(stderr, "packet %d, len %d retrieved\n", packet->sequence_number,
	    packet->len);
#endif    
    FreePacket(packet, &pt_free_first, &pt_free_last, &nb_free_packet);
#ifdef DEBUG    
    fprintf(stderr, "min_saved : %d\n", min_memo_seq);
#endif    
    PrintList(stderr, pt_memo_first, "memo_packet", nb_memo_packet);
    PrintList(stderr, pt_free_first, "free_packet", nb_free_packet);
  }
  return;
}





void InitMemBuf(memo_packet, pt_free_first, pt_free_last, pt_memo_first,
		pt_memo_last, nb_max, nb_free_packet, nb_memo_packet)
     MEMBUF *memo_packet, **pt_free_first, **pt_free_last;
     MEMBUF **pt_memo_first, **pt_memo_last;
     int nb_max; /* Maximum elements number in a list */
     int *nb_free_packet; /* Number of free elements */
     int *nb_memo_packet; /* Number of memorized packets */
{
  /*
   * Linked-Lists Initialization
   */
  
  register int i;

  *pt_free_first = &memo_packet[0];
  *pt_free_last = &memo_packet[nb_max-1];
  *pt_memo_first = NIL;
  *pt_memo_last = NIL;
  *nb_free_packet = nb_max;
  *nb_memo_packet = 0;
  for(i=0; i<nb_max; i++){
    memo_packet[i].len = 0;
    memo_packet[i].sequence_number = 0;
    memo_packet[i].len = 0;
  }
  for(i=0; i<(nb_max-1); i++)
    memo_packet[i].next = &memo_packet[i+1];
  memo_packet[nb_max].next = NIL;
  for(i=1; i<nb_max; i++)
    memo_packet[i].prev = &memo_packet[i-1];
  memo_packet[0].prev = NIL;
}



void PrintList(file, pt_first, name_list, nb_element)
     FILE *file;
     MEMBUF *pt_first;
     char *name_list;
     int nb_element;
{
  /*
   * Print all the list MEMBUF.
   */

  MEMBUF *pt = pt_first;
  int cpt = 0;

  if(pt == NIL){
    fprintf(file, "*** %s is free ***\n", name_list);
    return;
  }
  fprintf(file, "*** %s list %d elements ***\n\n", name_list, nb_element);

  do{
    fprintf(file, "%s # %d-->", name_list, cpt);
    fprintf(file, "\tsequence_number: %d\tlen: %d\n", pt->sequence_number,
	    pt->len);
    cpt++;
    pt = pt->next;
  }while(pt != NIL);
  fprintf(file, "\n");
}




BOOLEAN InsertPacket(pt_memo_first, pt_memo_last, packet, min_memo_seq,
		     nb_memo_packet)
     MEMBUF **pt_memo_first, **pt_memo_last, *packet;
     u_short *min_memo_seq;
     int *nb_memo_packet;
{
  /*
   * Insert the packet in the MEMBUF list in the increasing order.
   * Antecedent: There is enough place in the list.
   * Return FALSE if the packet is duplicated. 
   */
  
  MEMBUF *pt = *pt_memo_last;
  u_short sequence_number = packet->sequence_number;
  BOOLEAN INSERTED = FALSE;

  if(pt != NIL){
    do{
      if(GreaterPacket(sequence_number, pt->sequence_number)){
	if(pt == *pt_memo_last){
	  /* This packet is the greatest in the list */
	  pt->next = packet;
	  packet->prev = pt;
	  *pt_memo_last = packet;
	  INSERTED = TRUE;
	  break;
	}else{
	  /* This packet must be inserted between 2 packets */
	  packet->next = pt->next;
	  packet->prev = pt;
	  (pt->next)->prev = packet;
	  pt->next = packet;
	  INSERTED = TRUE;
	  break;
	}
      }else{
	if(sequence_number == pt->sequence_number)
	  /* This is a duplicated packet. Drop it... */
	  return(FALSE);
      }
      pt = pt->prev;
    }while(pt != NIL);
  }
  if(!INSERTED){
    /* This is the smallest packet in the list. */
    *min_memo_seq = packet->sequence_number;
    packet->prev = NIL;
    if(*pt_memo_first == NIL){
      /* The list was empty */
      packet->next = NIL;
      *pt_memo_first = packet;
      *pt_memo_last = packet;
    }else{
      (*pt_memo_first)->prev = packet;
      packet->next = *pt_memo_first;
      *pt_memo_first = packet;
    }
  }
  (*nb_memo_packet) ++;
  return(TRUE);
}




MEMBUF *TakeOutPacket(pt_memo_first, pt_memo_last, min_memo_seq,
		      nb_memo_packet)
     MEMBUF **pt_memo_first, **pt_memo_last;
     u_short *min_memo_seq;
     int *nb_memo_packet;
{
  /* Take out the first packet (smallest) in the list */

  MEMBUF *pt = *pt_memo_first;
  
  if(pt == NIL)
    return(NIL);

  if(pt == *pt_memo_last){
    /* The list has only one packet */
    *pt_memo_last = NIL;
  }else{
    (pt->next)->prev = NIL;
    *min_memo_seq = (pt->next)->sequence_number;
  }
  *pt_memo_first = pt->next;
  pt->next = pt->prev = NIL;
  (*nb_memo_packet) --;
  return(pt);
}
  


MEMBUF *CreatePacket(sequence_number, len, pt_free_first, pt_free_last,
		     nb_free_packet)
     u_short sequence_number;
     int len;
     MEMBUF **pt_free_first, **pt_free_last;
     int *nb_free_packet;
{
  /* Malloc a packet from the free list */

  MEMBUF *pt = *pt_free_first;

  (*nb_free_packet) --;
  if(! *nb_free_packet){
    /* The list had only one packet */
    *pt_free_first = *pt_free_last = NIL;
  }else{
    *pt_free_first = pt->next;
    (*pt_free_first)->prev = NIL;
  }
  pt->next = NIL;
  pt->sequence_number = sequence_number;
  pt->len = len;
  return(pt);
}


void FreePacket(packet, pt_free_first, pt_free_last, nb_free_packet)
     MEMBUF *packet, **pt_free_first, **pt_free_last;
     int *nb_free_packet;
{
  /* Add the packet in the list of free packets */

  packet->next = NIL;
  if(! *nb_free_packet){
    /* The list of free packets was empty */
    packet->prev = NIL;
    *pt_free_first = packet;
  }else{
    packet->prev = *pt_free_last;
    (*pt_free_last)->next = packet;
  }
  *pt_free_last = packet;
  (*nb_free_packet) ++;
}
