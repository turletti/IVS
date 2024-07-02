/*
 *
 */
#include "mc_header.h"
#ifdef HPUX
#undef random
#define random rand
#endif
static int signalReceived;

void mc_sendRtt();

static void mc_setSendRtt(null)
     int null;
{
  signalReceived = 1;
}


void mc_recv(mcrState,ghdr, hdr, to_addr,len)
     Mc_RxState *mcrState;
     rtp_hdr_t *ghdr;
     app_mcct_t *hdr;
     struct sockaddr *to_addr;
     int len;
{
  struct timeval now;
  long nowMs,pktTimeMs, lastRsp;
  u_char prevState;
  int keyShift;
  
  
  /* Hand packet to congestion meter and get result */
  /* 
   * Note to myself - we ought to instantiate different state for each
   * source we receive stuff from
   *
   */
  /* First calculate now */
  gettimeofday(&now,NULL);
  nowMs = (now.tv_sec - mcrState->startTime.tv_sec) * 1000 + 
    (now.tv_usec - mcrState->startTime.tv_usec) / 1000;
  
  pktTimeMs = ((long)(ghdr->ts_sec) - 
	       (long)(mcrState->startTime.tv_sec & 0xffff)) * 1000 + 
	       ((long)MCGETUSEC((unsigned int)ghdr->ts_usec) - 
		mcrState->startTime.tv_usec)
	       / 1000;
  
  prevState = mcrState->state;
  mcrState->state = mcrState->congMeter(mcrState->congState, pktTimeMs, 
					nowMs, ghdr->seq,hdr->maxRtt);
  
#ifdef MCDEBUG
  if(prevState != mcrState->state) { 
    printf("%ld ",nowMs);
    switch(mcrState->state) {
    case UNLOADED: printf("UNLOADED\n"); break;
    case LOADED: printf("LOADED\n"); break;
    case CONGESTED: printf("CONGESTED\n"); break;
    default: printf("*** unknown state ***\n");
    }
  }
#endif
  /* IF(CONGESTED && (lastRsp > 1 rtt || lastState != CONGESTED) */
  lastRsp = nowMs - mcrState->lastTime;
  
  keyShift = (int)GETKEYSHIFT(hdr->keyShift);
  
  /*
   * send a response if we're congested and its over an rtt since
   * we last sent one OR
   * any response is solicited to estimate size and we match the key OR
   * we're LOADED and we match the key and its over an rtt since we last sent
   * a response
   */
  if((mcrState->state == CONGESTED && 
      (lastRsp > hdr->maxRtt || prevState != CONGESTED))  ||
     (hdr->sizeSolicit && 
      (KEYMATCH(mcrState->key,hdr->key,keyShift) && 
       lastRsp > hdr->maxRtt)) ||
     (mcrState->state == LOADED && 
      (KEYMATCH(mcrState->key,hdr->key,keyShift))  && 
      (lastRsp > hdr->maxRtt || prevState < LOADED))) {
    /* FIll out the header */
    mcrState->hdr.state = hdr->state;
    SETSTATE(mcrState->hdr.state, mcrState->state);
    /* Only set the solicit flag if we're responding to that */
    if(hdr->sizeSolicit && 
       (KEYMATCH(mcrState->key,hdr->key,keyShift)) &&
       lastRsp > hdr->maxRtt)
      mcrState->hdr.sizeSolicit = 1;
    /* copy the timestamp across */
    mcrState->hdr.ts_sec = ghdr->ts_sec;
    mcrState->hdr.ts_usec = ghdr->ts_usec;
    mc_sendRsp(mcrState,to_addr,len,&now);
    mcrState->lastTime = nowMs;
  }
  
  /* If soliciting rtt */
  if((hdr->rttSolicit) && mcrState->rttDelay == 0) {
#ifdef MCDEBUG
    txpkt_print(hdr);
#endif
    /* Generate random delay using given fields */
    while(mcrState->rttDelay == 0)
      mcrState->rttDelay = (unsigned)random() >> (DELAYSHIFT - 
						GETRTTSHIFT(hdr->rttShift));
    signal(SIGALRM, mc_setSendRtt);
    /* Establish start time */
    gettimeofday(&(mcrState->startDelay),NULL);
    /* Save timestamp in packet */
    mcrState->timeRtt_sec = ghdr->ts_sec;
    mcrState->timeRtt_usec = ghdr->ts_usec;
#ifdef MCDEBUG
    printf("Hanging around for %u ms\n",mcrState->rttDelay);
    mc_rxStatePrint(mcrState);
#endif
    {
      struct itimerval mytimer;
      
      mytimer.it_value.tv_usec = 
	(unsigned)(mcrState->rttDelay * 1000) % 1000000;
      mytimer.it_value.tv_sec = 
	(unsigned)(mcrState->rttDelay / 1000);
      mytimer.it_interval.tv_sec = 0;
      mytimer.it_interval.tv_usec = 0;
      
      setitimer(ITIMER_REAL,&mytimer,(struct itimerval *)NULL);
    }
    
  }
  
  /* is it time to send a response to an rtt solicit */
  if(signalReceived) mc_sendRtt(mcrState,to_addr,len);
  
  
  
  
}


Mc_RxState *mc_initRecv(congMeter, initMeter)
     int (*congMeter)(/* void *,long,long,u_int32,u_int32 */);
     Mc_CongState *(*initMeter)();
{
  struct timeval now;
  Mc_RxState *thisState = (Mc_RxState *) calloc(sizeof(Mc_RxState),
						sizeof(char));
  
  gettimeofday(&now, NULL);
  srand(now.tv_usec);
  thisState->startTime = now;
  thisState->lastTime = 0;
  thisState->rttDelay = 0;
  thisState->startDelay = now;
  thisState->timeRtt_sec = 0;
  thisState->timeRtt_usec = 0;
  thisState->state = UNLOADED;
  while(thisState->key == 0)
    thisState->key = (unsigned short)rand();
  thisState->congMeter = congMeter;
  thisState->congState = (*initMeter)();
  thisState->hdr.sizeSolicit = 0;
  thisState->hdr.rttSolicit = 0;
  thisState->hdr.state = 0;
  thisState->hdr.rttDelay = 0;
  thisState->hdr.ts_sec = 0;
  thisState->hdr.ts_usec = 0;
  
  thisState->hdr.final = 1;
  thisState->hdr.length = sizeof(app_mccr_t)/4;
  thisState->hdr.type = RTP_APP;
  thisState->hdr.subtype = 0;
  thisState->hdr.name = htonl(RTP_APP_MCCR_VAL);
  
  return(thisState);
}



int mc_sendRsp(mcrState, to_addr, len, now)
     Mc_RxState *mcrState;
     struct sockaddr *to_addr;
     int len;
     struct timeval *now;
{  
  /* 
   * I don't like this - but its quick and more elegance would require too
   * much rewriting at this point 14/1/94
   */
  
  static int sock_report;
  static BOOLEAN FIRST=TRUE;
  static u_int buf[64]; /* u_char removed (memory alignment problem) */
  static int k=8;
  rtp_hdr_t *h;
  rtp_t *r;
  int lw;
  
  h = (rtp_hdr_t *) buf;
  
  if(FIRST){
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    /* Set the fixed RTP header */
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    FIRST = FALSE;
  }
  /* Fill out fields in packet */
  
  h->seq = htons(mcrState->seq++);
  h->ts_sec = htons(now->tv_sec & 0xffff);
  h->ts_usec = htons(( (now->tv_usec) /(USECINSEC>>16) ) & 0xffff);
  r = (rtp_t *) &(buf[sizeof(rtp_hdr_t)/4]);
  
  memcpy((char *)r,(char *)&mcrState->hdr,sizeof(mcrState->hdr));
  
  /* send response */  
  /* 
   *  Get some data to send, and wop it out...
   *
   */
  
  if(sendto(sock_report,(char *)buf,sizeof(mcrState->hdr) + sizeof(rtp_hdr_t)
	    ,0,to_addr,len) < 0) 
    perror("mc_sendRsp");
  
  if(mcrState->hdr.rttDelay) {
    mcrState->hdr.rttDelay = 0;
    mcrState->hdr.rttSolicit = 0;
  }
  mcrState->hdr.sizeSolicit = 0;
  
#ifdef MCDEBUG
  printf("Sendt rtt\n");
  mc_rxStatePrint(mcrState);
#endif
}

void mc_sendRtt(mcrState,to_addr,len)
     Mc_RxState *mcrState;
     struct sockaddr *to_addr;
     int len;
{
  struct timeval now;
  static int sock_report;
  static BOOLEAN FIRST=TRUE;
  static u_int buf[64]; /* u_char removed (memory alignment problem) */
  static int k=8;
  rtp_hdr_t *h;
  rtp_t *r;
  int lw;
#ifdef MCDEBUG
  fprintf(stderr, "mc_sendRtt sending response\n");
#endif
  h = (rtp_hdr_t *) buf;
  if(FIRST){
    if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("Cannot create datagram socket");
      exit(-1);
    }
    /* Set the fixed RTP header */
    h->ver = RTP_VERSION;
    h->channel = 0;
    h->p = 1;
    h->s = 0;
    h->format = RTP_H261_CONTENT;
    h->seq = 0;
    h->ts_sec = 0;
    h->ts_usec = 0;
    FIRST = FALSE;
  }  
  
  gettimeofday(&now,NULL);
  h->seq = htons((mcrState->seq)++);
  h->ts_sec = htons(now.tv_sec & 0xffff);
  h->ts_usec = htons(( (now.tv_usec) /(USECINSEC>>16) ) & 0xffff);
  /* Calculate actual delay */
  /* Check if we should fill in the rtt */
  if(signalReceived) {
    signalReceived = 0;
  }
  
  mcrState->hdr.rttDelay = (now.tv_sec - mcrState->startDelay.tv_sec) * 1000 +
    (now.tv_usec - mcrState->startDelay.tv_usec)/1000;
  
  /* fill in packet fields */
  mcrState->hdr.ts_sec = htons(mcrState->timeRtt_sec);
  mcrState->hdr.ts_usec = htons(mcrState->timeRtt_usec);
  mcrState->hdr.rttSolicit = 1;
  mcrState->rttDelay = 0;  
  r = (rtp_t *) &(buf[sizeof(rtp_hdr_t)/4]);
  
#ifdef MCDEBUG
  printf("Telling other end we waited %u ms\n", mcrState->hdr.rttDelay);
#endif
  memcpy((char *)r, (char *)&mcrState->hdr,sizeof(mcrState->hdr));
  
  /* send response */  
  /* 
   *  Get some data to send, and wop it out...
   *
   */
  
  
  if(sendto(sock_report,(char *)buf,sizeof(mcrState->hdr) + sizeof(rtp_hdr_t)
	    ,0,to_addr,len) < 0) 
    perror("mc_sendRsp");
  mcrState->hdr.rttDelay = 0;
  mcrState->hdr.rttSolicit = 0;
  
}

mc_rxStatePrint(mcstate)
     Mc_RxState *mcstate;
{
  /* Dump the current state of the tx machine */
  printf("st %u.%u lst %ul rttDel %d ",
	 mcstate->startTime.tv_sec,mcstate->startTime.tv_usec,
	 mcstate->lastTime,mcstate->rttDelay);
  switch(mcstate->state) {
  case UNLOADED: printf("U "); break;
  case LOADED:   printf("L "); break;
  case CONGESTED: printf("C "); break;
  default: printf("**Unknown State** ");
  }
  printf("key %u ",(unsigned int)mcstate->key);
}

