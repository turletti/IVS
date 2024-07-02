#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <sys/param.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
 
#include <math.h>

#include "general_types.h"
#include "rtp.h"

#ifndef _mc_header_h
#define _mc_header_h

/* Defines header formats, and various macros for handling them */

/*
#define SIZESOLICITED 0x01
#define RTTSOLICITED  0x02
#define NETSTATE      0x30
#define NETSHIFT      4

#define KEYSHIFT      0xf0
#define KEYSHIFTSHIFT 4

#define RTTSHIFT      0x0f
#define RTTSHIFTSHIFT 0
*/
#define GETSTATE( state) (state)
#define SETSTATE(value,state) (value = state)

#define GETKEYSHIFT(shift) (shift)
#define SETKEYSHIFT(value,shift) (value = shift)

#define GETRTTSHIFT(shift) (shift)
#define SETRTTSHIFT(value,shift) (value = shift)

#define KEYMATCH(value,key,shift) ((key)?(value)>>(shift) == (key)>>(shift):1)

/* Assuming bsd returns a 32 bit random number... */
#ifdef SYSV
#define DELAYSHIFT 6
#else
#define DELAYSHIFT 22
#endif

#define MSECINSEC 1000
#define USECINSEC 1000000

#define UNLOADED 0
#define LOADED 1
#define CONGESTED 2

#define LOSSCONGTH 15
#define LOSSLOADTH 5
#define DEF_GROUPADDR "224.2.226.27"
#define DEF_PORT 29876
#define DEF_SAMPLETIME  1
#define STARTRATE 200000          /* 5 packets a second */

#define MAXMSGLEN 1500
#define INCRATE  0.000001
#define DECRATE 2
#define MAXRTT  5000              /* upper limit rtt to 5 seconds */

/* macros to manipulate time stamps */
#define MCGETSEC(timestamp) (u_int32)((timestamp) >> 16)

#define MCGETUSEC(timestamp) (u_int32)( (timestamp & 0xffff) * (USECINSEC >> 16))

#define MCGETTIME(sec,usec) ((((sec) << 16) + (( (usec) /(USECINSEC>>16) ) & \
					       0x10000)) |  \
			     (( (usec) /(USECINSEC>>16) ) & 0xffff))

/* time2 - time1 */
#define MCGETMSECDIFF(time1,time2) ((((time2)->tv_sec) - ((time1)->tv_sec)) * \
  MSECINSEC + (((time2)->tv_usec) - ((time1)->tv_usec))/MSECINSEC)


typedef struct mc_hole {
  long time;
  u_int32 start,end;
  struct mc_hole *next;
} Mc_Hole;

typedef struct mc_lossState {
  int lostPkts;
  long timeMean,timeVar,lastPkt,ipg;
  u_int32 nextSeq;
  Mc_Hole *tail, *head;
} Mc_LossState;

typedef union mc_congstate {
  Mc_LossState  *lossState;
} Mc_CongState;

typedef
  struct mccp_send {
#if defined(littleendian)
    u_char   state;	/* state */
    u_char   shift; /* shift */
#endif

#if defined(bigendian)
    u_char   state; /* state */
    u_char   shift;
#endif
    u_short  key;
    u_int32  maxrtt;		/* maximum rtt found */
  }  Mccp_Send;


typedef
  struct mccp_recv {	
    u_char   command;	/* commands */
    u_char   state;		/* state */
    u_int32  delay;		/* Random delay added in ms */
    u_int32  time; /* Time returned in middle 32 ntp format */
} Mccp_Recv;

/* Generic header */
typedef struct mc_header {
  u_int32  time;    /* Timestamp - 16 bits of seconds, 16 bits below */
  u_int32  seq;     /* our favourite monotonically increasing sequence num */
} Mc_Header;

#define MCCPOFFSET sizeof(Mc_Header)

/* Protocol state */
typedef struct mc_txstate {
  struct timeval  startTime;		/* Start time of connection  */
  long            timeLastPacket;  /* Time last pkt in msecs from start */
  int             ipgEst;
  int             estReceivers;
  int             solicitedResponses;
  int             startShift;          /* Initial shift used for a round */
  u_int32         shadowRtt;
  u_int32         rndStart;
  double          estRound;

  long            probeTimeout;	/* Timeout on probe */
  long            rttTimeout;
  struct sockaddr_in sendAddr;

  void            (*increasefn)();
  void            (*decreasefn)(/* int estimated percent in congestion */);

  Mc_Header       ghdr;
  app_mcct_t      hdr;		/* Header template */
  

} Mc_TxState;

/* This should be maintained on a per source basis */
typedef struct mc_rxstate {
  struct timeval  startTime;		/* Start time of connection  */
  long            lastTime;  /* Time last pkt in msecs from start */
  int              rttDelay;	/* Timeout on probe */
  struct timeval   startDelay;  /* Time we started waiiting */
  short  timeRtt_sec;   /* Rtt record Time returned in middle 32 ntp format */
  short  timeRtt_usec;  /* Rtt record Time returned in middle 32 ntp format */
  u_char state;
  unsigned short key;			/* key used in probabilistic poll */

  int            (*congMeter)(/* void *,long,long,u_int32,u_int32 */ );
  Mc_CongState   *congState;
  short              seq;

  app_mccr_t       hdr;		/* Header template */
  

} Mc_RxState;


/* The generic packet type */

typedef char Pkt[];

int  mc_lossMeter(/* Mc_CongState *, long , long , u_int32 , u_int32 */);

Mc_CongState *mc_initLossMeter();

#endif


