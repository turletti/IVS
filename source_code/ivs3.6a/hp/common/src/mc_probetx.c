/* 
 *  Transmitter probe code
 *  This should basically be a timeout that adjusts the template of the
 *  congestion header.  Lock state, increase mask, then reset timer.
 *
 *  A successful response must block the timeout signal coming in, followed 
 *  by a reset of the header to go out, and a reset of the timer
 */

#include "mc_header.h"

void mc_recvpkt(mcstate,hdr,mccp_hdr)
     Mc_TxState *mcstate;
     rtp_hdr_t *hdr;
     app_mccr_t *mccp_hdr;
{
  int percentCongested;
  
  /* 
   * if the message comes from the current round and it is a LOADED
   * or if it is a congestion signal from any time
   */
  if((GETSTATE(mccp_hdr->state) == LOADED && 
     (mccp_hdr->ts_sec << 16 & mccp_hdr->ts_usec) > mcstate->rndStart) || 
     GETSTATE(mccp_hdr->state) == CONGESTED)     {

    /*
     * If we get a congestion signal, and we either aren't congested
     * or this signal comes from the current round
     * send message to reduce rate 
     * Note that this logic allows the possibility of reducing rate 
     * unnecessarily if we have a very delayed congestion signal
     * if at minimum rate signal upwards 
     */
    if(GETSTATE(mccp_hdr->state) == CONGESTED && 
       (GETSTATE(mcstate->hdr.state) != CONGESTED || 
	(mccp_hdr->ts_sec << 16 & mccp_hdr->ts_usec) > mcstate->rndStart)) {
#ifdef MCDEBUG
    /* Dump output state */
      fprintf(stderr,"Congested EoR - ");
      mc_txStatePrint(mcstate,stderr);
#endif
      SETSTATE(mcstate->hdr.state,GETSTATE(mccp_hdr->state));
#ifdef GETSTAT_MODE
      {
	extern BOOLEAN LOG_FILES_OPENED;
	extern int file_rate;

	if(LOG_FILES_OPENED){
	  struct timeval realtime;
      
	  gettimeofday(&realtime, (struct timezone *)NULL);
	  fprintf((FILE *)file_rate,"netState %u.%.6u\t",
		  realtime.tv_sec, realtime.tv_usec);
	  mc_txStatePrint(mcstate,file_rate);
	  fflush((FILE *)file_rate);
	}
      }
#endif
      percentCongested = 100 * exp((mcstate->estRound - 
				    GETKEYSHIFT(mcstate->hdr.keyShift) + 1)/
				   1.40086);
      (*mcstate->decreasefn)(percentCongested);
      /* Reset timer, update state and then carry on */
      mcstate->probeTimeout = 0;
      do
	mcstate->hdr.key = (unsigned short) (rand()>>14);
      while(mcstate->hdr.key == 0);
      SETKEYSHIFT(mcstate->hdr.keyShift,mcstate->startShift);
      mcstate->hdr.sizeSolicit = 1;
      setRttSolicit(mcstate);
      SETSTATE(mcstate->hdr.state,UNLOADED);
    }    
    else {
      SETSTATE(mcstate->hdr.state,GETSTATE(mccp_hdr->state));

#ifdef GETSTAT_MODE
      {
	extern BOOLEAN LOG_FILES_OPENED;
	extern int file_rate;

	if(LOG_FILES_OPENED){
	  struct timeval realtime;
      
	  gettimeofday(&realtime, (struct timezone *)NULL);
	  fprintf((FILE *)file_rate,"netState %u.%.6u\t",
		  realtime.tv_sec, realtime.tv_usec);
	  mc_txStatePrint(mcstate,file_rate);
	  fflush((FILE *)file_rate);
	}
      }
#endif
    }

  }
  /* Check to see if this is a return to an attempt to estimate size */
  if(mccp_hdr->sizeSolicit)      {
    /* Look up estimate of how many listeners there are */
    if(mcstate->solicitedResponses == 0) {
      int curRound = GETKEYSHIFT(mcstate->hdr.keyShift) + 1;
      if(mcstate->hdr.key == 0) curRound++;
      if(mcstate->estRound)
	mcstate->estRound = (double)((15 * mcstate->estRound) + 
				     curRound)/16;
      else mcstate->estRound = curRound;
      /* Lovely equation described in the accompanying paper */
      /* Basically the mapping of average round to number of receivers */
      /* turns out to be very well approximated by the following */
      mcstate->estReceivers = exp((16.246 - mcstate->estRound)/1.40086);

      /* 
       * To speed up the number of rounds that we take in doing things,
       * we adjust the number of shifts we do based on the estimate of 
       * the first round that people come back on
       * Arbitarily at the moment, I'll go for estimate - 2 for the mask used
       */
      
      if((mcstate->startShift = (int)mcstate->estRound - 2) < 0)
	mcstate->startShift = 0;
    }

    /* Increment number of responses */
    mcstate->solicitedResponses++;

    /* Remove sizesolicited */
    mcstate->hdr.sizeSolicit = 0;
      
  }

  /* look to see if this an attempt to give us a rtt */
  if(mccp_hdr->rttSolicit)     {
    struct timeval now;
    unsigned int tsdiff,rtt = 0;

    gettimeofday(&now, (struct timezone *)NULL);

    /* Calculate the rtt */
    /*
     *  For some reason, which I haven't figured out yet, we can sometimes
     * get the delay to be larger than the difference in timestamps, so
     * that the packet arrived at the remote end before it left.  Do a sanity
     * check to prevent this happening, since it is rare.
     */



    if((unsigned)(now.tv_sec & 0xffff) < mccp_hdr->ts_sec)
      tsdiff = 
	(unsigned)((int)(0x10000 + (unsigned)(now.tv_sec & 0xffff) - 
			  (mccp_hdr->ts_sec))*1000 +  
		   (now.tv_usec - 
		    (int)MCGETUSEC((u_int)(mccp_hdr->ts_usec)))/1000);
    else
      tsdiff = 
	(unsigned)((int)((unsigned)(now.tv_sec & 0xffff) - 
			 mccp_hdr->ts_sec)*1000 + 
		   (now.tv_usec - 
		    (int)MCGETUSEC((u_int)(mccp_hdr->ts_usec)))/1000);

    if(tsdiff > (unsigned int)mccp_hdr->rttDelay) 
      rtt = tsdiff - (unsigned int)mccp_hdr->rttDelay;

#ifdef MCDEBUG
    printf("rtt = %d\n",rtt);
#endif
    
    /* 
     * if its greater than the largest yet, use this as our base timeout 
     * Place a ceiling on the maximum rtt of 5 seconds
     */
    if(rtt > mcstate->hdr.maxRtt) mcstate->hdr.maxRtt = 
      (rtt > MAXRTT)? MAXRTT:rtt;
    /* Track the rtt so that we can age out high rtts */
    if(rtt > mcstate->shadowRtt) mcstate->shadowRtt = 
      (rtt > MAXRTT)? MAXRTT:rtt;rtt;

  }
#ifdef MCDEBUG
    /* Dump output state */
  mc_txStatePrint(mcstate,stderr);
#endif
}

Mc_TxState *mc_probeinit(incfn,decfn)
     void (*incfn)();
     void (*decfn)();
{
  
  Mc_TxState *mcstate = (Mc_TxState *)malloc(sizeof(Mc_TxState));

   /* Set up the requisite state */
  gettimeofday(&mcstate->startTime, (struct timezone *)NULL);
  mcstate->timeLastPacket = 0;
  mcstate->ipgEst = 0;
  mcstate->estRound = 0;	/* Assume about 100 receivers initially */
  mcstate->rndStart = 0;
  mcstate->increasefn = incfn;
  mcstate->decreasefn = decfn;
  mcstate->estReceivers = 0;
  mcstate->startShift = 0;
  mcstate->solicitedResponses = 0;
  mcstate->hdr.maxRtt = 0;	
  mcstate->shadowRtt = 0;

  srand( mcstate->startTime.tv_usec);

  do
    mcstate->hdr.key = (unsigned short) (rand() >> 14);
    while(mcstate->hdr.key == 0);
  mcstate->hdr.sizeSolicit = 1;

  /* 
   * assume 100 receivers (in testing) and aim for 10 responses a second 
   * so we have a shift of 4 originally
   */
  SETRTTSHIFT(mcstate->hdr.rttShift,4);
  mcstate->hdr.rttSolicit= 1;
  mcstate->hdr.type = RTP_APP;
  mcstate->hdr.final = 1;
  mcstate->hdr.length = sizeof(app_mcct_t)/4;
  mcstate->hdr.subtype = 0;
  mcstate->hdr.name = htonl(RTP_APP_MCCT_VAL);

  return(mcstate);
}


void mc_txpkt(mcstate, hdr, mccp_hdr)
     Mc_TxState *mcstate;
     rtp_hdr_t *hdr;
     app_mcct_t *mccp_hdr;
{
  struct timeval now;
  long pktTime;  

  /* 
   * Keep a track of the bandwidth
   */

  gettimeofday(&now, (struct timezone *) NULL);

  pktTime = MCGETMSECDIFF(&(mcstate->startTime),&now);

  if(mcstate->timeLastPacket == pktTime) pktTime++;

  if(mcstate->ipgEst)
    mcstate->ipgEst = (7 * mcstate->ipgEst + 
		     (pktTime - mcstate->timeLastPacket) *1000)>>3;
  else mcstate->ipgEst = ((pktTime - mcstate->timeLastPacket) *1000)>>3;

  /*
   * If we have passed a timeout, set the new timeout and send the data
   */

  mcstate->timeLastPacket = pktTime;
  
  /* 
   * Record the start of the round to ensure we associate 
   * responses correctly 
   */
  
  if(mcstate->probeTimeout == 0) mcstate->rndStart = 
    ((hdr->ts_sec << 16) & (hdr->ts_usec));

  if(pktTime > mcstate->probeTimeout) {
    /* if the mask is set to 15 */
    if(GETKEYSHIFT(mcstate->hdr.keyShift) == 0xf) {
      if(mcstate->hdr.key == 0) {
#ifdef MCDEBUG
    /* Dump output state */
	fprintf(stderr,"UNLOADED EoR - ");
	mc_txStatePrint(mcstate,stderr);
#endif
	if(mcstate->solicitedResponses == 0) mcstate->estReceivers = 0;
	/* Got through without being LOADED - increase rate if receivers */
	if(GETSTATE(mcstate->hdr.state) == UNLOADED)(*mcstate->increasefn)();
	/* Reset keys et al */
	mcstate->hdr.sizeSolicit = 1;
	SETSTATE(mcstate->hdr.state,UNLOADED);

	setRttSolicit(mcstate);
#ifdef GETSTAT_MODE
	{
	  extern BOOLEAN LOG_FILES_OPENED;
	  extern int file_rate;

	  if(LOG_FILES_OPENED){
	    struct timeval realtime;
      
	    gettimeofday(&realtime, (struct timezone *)NULL);
	    fprintf((FILE *)file_rate,"netState %u.%.6u\t",
		    realtime.tv_sec, realtime.tv_usec);
	    mc_txStatePrint(mcstate,file_rate);
	    fflush((FILE *)file_rate);
	  }
	}
#endif

	mcstate->solicitedResponses = 0;
	SETKEYSHIFT(mcstate->hdr.keyShift,mcstate->startShift);
	do 
	  mcstate->hdr.key = (unsigned short)(rand() >> 14);
	while(mcstate->hdr.key == 0);

      } else { mcstate->hdr.key = 0; }
    } else {
      if(mcstate->probeTimeout > 0) /* its not the start of a new round */
	SETKEYSHIFT(mcstate->hdr.keyShift,
		    GETKEYSHIFT(mcstate->hdr.keyShift)+1);
    }
    mcstate->probeTimeout = pktTime + 2*mcstate->hdr.maxRtt;

  }

  
  /* 
   *  Get some data to send 
   *  Copy the header into the space at the front
   *
   */
#ifdef __NetBSD__
  memcpy(mccp_hdr, (const void *)&mcstate->hdr, sizeof(app_mcct_t));
#else
  memcpy(mccp_hdr, &mcstate->hdr, sizeof(app_mcct_t));
#endif
  return; 
}



mc_txStatePrint(mcstate, fd)
     Mc_TxState *mcstate;
     int fd;
{
  /* Dump the current state of the tx machine to the fd */  
  switch(mcstate->hdr.state) {
  case UNLOADED: fprintf((FILE *)fd,"U "); break;
  case LOADED:   fprintf((FILE *)fd,"L "); break;
  case CONGESTED: fprintf((FILE *)fd,"C "); break;
  default: fprintf((FILE *)fd,"**Unknown State** ");
  }
  fprintf((FILE *)fd,"rcvrs %d estRnd %f shadowRtt %u rtt %u\n",
	  mcstate->estReceivers,
	 mcstate->estRound,mcstate->shadowRtt,mcstate->hdr.maxRtt);
}

setRttSolicit(mcstate)
Mc_TxState *mcstate;
{
  struct timeval now;

  gettimeofday(&now, (struct timezone *)NULL);
  /* did we ask for it last time */
  if(mcstate->hdr.rttSolicit) {
  /* then turn it off and set a timeout for when we ask for it again */
    mcstate->hdr.rttSolicit = 0;
    mcstate->rttTimeout = now.tv_sec + (1 << GETRTTSHIFT(mcstate->hdr.rttShift));
/*
#ifdef MCDEBUG
    printf("Set rtt solicit to of %u s \n",mcstate->rttTimeout - now.tv_sec);
#endif
*/
  }  else
  /* else have we passed the timeout */
    if(now.tv_sec > mcstate->rttTimeout) {

      /* shift log2[receivers/10] */
      int shift = (mcstate->estReceivers > 10) ?
	(int)log((double)mcstate->estReceivers/10)/log(2) + 1 : 1;

      /* ageing out the maxrtt, to prevent one blip killing us */
      mcstate->hdr.maxRtt -= ( mcstate->hdr.maxRtt - mcstate->shadowRtt)/2;
      mcstate->shadowRtt = 0;

      /* set rtt bit and shift */
      SETRTTSHIFT(mcstate->hdr.rttShift,shift);
      mcstate->hdr.rttSolicit = 1;
    }
}

int mc_getEstReceivers(mcstate)
Mc_TxState *mcstate;
{
  return((int)mcstate->estReceivers);
}

int mc_getState(mcstate)
Mc_TxState *mcstate;
{
  return((int)mcstate->hdr.state);
}

