#include "mc_header.h"

void printHole(hole)
Mc_Hole *hole;
{
  printf("%ld %u -> %u\n",hole->time,hole->start,hole->end);
}

int  mc_lossMeter(congState, timestamp,  now,
		       seq,  rtt)
Mc_CongState *congState;
long timestamp;
long now;
u_int32 seq,  rtt;
/*
 * timestamp comes in milliseconds since start of connection according to
 * remote clock
 * now is milliseconds since start of connection
 * rtt in milliseconds
 * This congestion meter is not terribly good at figuring out when the net is 
 * loaded, since the loss of a packet over a rtt is a transitory event
 * Eventually we ought to have a memory thing, that records state once a 
 * maxrtt, with thresholds to decide current state

 */
{
  Mc_LossState  *lossState = (Mc_LossState *)&(congState->lossState);
  long timeDiff;  /* in milliseconds */

  /* Update the estimated interpacket gap */
  if(lossState->ipg) {
    if((lossState->ipg = 
        (15*lossState->ipg + now -lossState->lastPkt)>>4) == 0) 
      lossState->ipg = 1;
  }
  else lossState->ipg = now;
  lossState->lastPkt = now;
  
  timeDiff = now - timestamp;
   /*

    * The congestion signal is calculated here by measuring the loss in a 
    * given period of packets - if the threshold for lost packets is passed
    * then signal Congested.  If there are no lost packets, then we are at
    * UNLOADED, else LOADED
    */
  /* if sequence number is next, increase expected number */
    
  if(lossState->nextSeq == 0) {
    lossState->nextSeq = seq + 1;
  }
  else  if(seq == lossState->nextSeq) { lossState->nextSeq++; }
  else if(seq > lossState->nextSeq) {
    /* This is definitely a hole */
    Mc_Hole *hole;

    hole = (Mc_Hole *)calloc(sizeof(Mc_Hole), sizeof(char));
    hole->time = now;
    hole->start = lossState->nextSeq;
    hole->end = seq - 1;
    hole->next = (Mc_Hole *)NULL;
    /* Now add it to the list */
    if(lossState->head == NULL) {
      lossState->head = hole; lossState->tail = hole;
    } else {
      lossState->tail->next = hole;
      lossState->tail = hole;
    }
    lossState->nextSeq = seq + 1;
/*
#ifdef DEBUG
    printf("now = %ld ts =%ld rtt = %u\n",now,timestamp,rtt);
    printLossMeter(lossState);
#endif
*/
  }
  else {
    /* 

     * First we have to locate the correct hole,
     * then insert it into the list, or delete the hole
     */
    Mc_Hole *cur = lossState->head,*prev = NULL;
    while(cur != NULL && cur->end < seq ) {prev = cur; cur = cur->next; }
    if(cur != NULL) {
      if(cur->end != seq && cur->start != seq) {
	/* we have to partition this hole */
	Mc_Hole *hole;
	
	hole = (Mc_Hole *)calloc(sizeof(Mc_Hole), sizeof(char));
	hole->time = now;
	hole->start = seq + 1;
	hole->end = cur->end;
	hole->next = cur->next;
	cur->next = hole;
	cur->end = seq - 1;
	if(lossState->tail == cur) lossState->tail = hole;
      } else if(cur->end == cur->start == seq) {
	/* just filled a hole - release it */
	if(prev == NULL) lossState->head = cur->next;


	else prev->next = cur->next;
	free((char *)cur);
      } else if(cur->start == seq) cur->start++;
      else if(cur->end == seq) cur->end--;
    }
/*
#ifdef DEBUG
    printLossMeter(lossState);
#endif
*/

  }

  /* update the calculation of the variance in the rtt */
  /* get the time averaged mean of the difference */
  if(lossState->timeMean == 0) lossState->timeMean = timeDiff;
  else   lossState->timeMean = (7*lossState->timeMean + timeDiff)>>3;
  if((lossState->timeMean - timeDiff) < 0) 
    timeDiff = timeDiff - lossState->timeMean;
  else timeDiff = lossState->timeMean - timeDiff;
  lossState->timeVar  = (7*lossState->timeVar + timeDiff)>>3;

  /* 
   * Check down the list of holes, discarding those that before now-rttvar-rtt,
   * Counting those that fall within now-rttvar to now-rttvar-rtt
   */
  if(lossState->head != NULL){
    Mc_Hole *cur = lossState->head, *prev = NULL;
    long validStart,validEnd;

    validEnd = now - 2*lossState->timeVar;
    validStart = validEnd - rtt;

/*
#ifdef DEBUG   
    printf("now = %ld ts = %ld rtt = %u\n",now,timestamp,rtt);
    printLossMeter(lossState);

#endif
*/  
    /* for each hole, if it is older than required, dump it  */
    /* If it is valid, add the size to the loss count */
    /* Go to the next hole */

    lossState->lostPkts = 0;
    
    while(cur != NULL) {
      if(cur->time < validStart) {
	if(prev == NULL) lossState->head = cur->next;
	else prev->next = cur->next;
#ifdef PRINT_HOLE	
	printHole(cur);
#endif	
	free((char *)cur);
	if(prev == NULL) cur = lossState->head;
	else cur = prev->next;
      } else {
	if(cur->time < validEnd) lossState->lostPkts += cur->end - 
	   cur->start + 1;
	prev = cur;
     	cur = cur->next;
      }
    }
  }



  /*
   * Update the moving average calculation of the number of holes, if
   * nowMs is another rtt away
   */
  /*
   * if(lossState->nextUpdate < nowMs)
   *  lossState->estHoles = (lossState->estHoles + lossState->lostPkts)/2;
   */
  {
    long pps;

    if(lossState->ipg != 0)
      pps = rtt/lossState->ipg;
    else
      pps = 0;

    /* If the rtt is smaller than the ipg, set the thresholds to 0,1,2 */
    if((pps * LOSSCONGTH) / 100 < 2) pps = 200/LOSSCONGTH;
    if(lossState->lostPkts <= (LOSSLOADTH * pps)/100) return UNLOADED;
    else if(lossState->lostPkts <= (LOSSCONGTH * pps)/100) return LOADED;

    else return CONGESTED;
  }

  /*
   * Update the moving average calculation of the number of holes, if
   * nowMs is another rtt away
   */
  /*

   * if(lossState->nextUpdate < nowMs)
   * lossState->estHoles = (lossState->estHoles + lossState->lostPkts)/2;
   */
}


void printLossMeter(ptr)
Mc_LossState *ptr;
{
  Mc_Hole *cur = ptr->head;

  printf("mean = %ld var = %ld \n",ptr->timeMean,ptr->timeVar);
  printf("HOLES ");
  while(cur != NULL) {
    printf("%.lu %u : %u ->",cur->time,cur->start,cur->end);
    cur = cur->next;
  }
  printf("\n");
  
}
  
Mc_CongState *mc_initLossMeter()
{
  Mc_LossState *ptr = (Mc_LossState *)calloc(sizeof(Mc_LossState),
					     sizeof(char));

  ptr->lostPkts = 0;
  ptr->timeMean = 0;
  ptr->timeVar = 0;

  ptr->lastPkt = 0;
  ptr->ipg = 0;
  ptr->nextSeq = 0;
  ptr->tail = NULL;
  ptr->head = NULL;
  return((Mc_CongState*)ptr);
}

int mc_delayMeter(pkt)
Pkt *pkt;
{
  /*
    * The congestion signal here is calculated by measuring the queueing
    * delays, using thresholds on a suitable EWMA to figure out what is
    * happening
    */
  fprintf(stderr,"mc_delayMeter function not implemented\n");
}




