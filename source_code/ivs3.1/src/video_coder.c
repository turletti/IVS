/**************************************************************************
*          Copyright (c) 1992 INRIA Sophia Antipolis, FRANCE.              *
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
*  File              : video_coder.c                                       *
*  Author            : Thierry Turletti                                    *
*  Last modification : 93/3/16                                             *
*--------------------------------------------------------------------------*
*  Description :  H.261 video coder.                                       *
*                                                                          *
*--------------------------------------------------------------------------*
* Modified by Winston Dang  wkd@Hawaii.Edu	92/12/8                    *
* alterations were made for speed purposes.                                *
\**************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <memory.h>
#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "protocol.h"

#ifdef VIDEOPIX
#include <vfc_lib.h>
#endif

#define MAX_BSIZE               1000
#define LIG_MAX                 288
#define COL_MAX                 352
#define LIG_MAX_DIV2            144
#define COL_MAX_DIV2            176
#define NBLOCKMAX              1584

#define DEFAULT_THRESHOLD        30
#define LOW_THRESHOLD            20


typedef struct {
  u_char quant;
  u_char threshold;
} STEP;

extern int tab_rec1[][255], tab_rec2[][255];
static int step_max = 6;

static STEP Step[7] = {
  {3, 15}, {3, 25}, {5, 25}, {7, 30}, {9, 40}, {11, 50}, {13, 60}
};

/* With old grabbing procedures */
/*
static STEP Step[7] = {
  {3, 10}, {3, 15}, {5, 15}, {7, 20}, {9, 25}, {11, 31}, {13, 35}
};
*/

/* mod- change + to | operation - much faster  also eliminated the 7-i op */
#define put_bit(octet,bit,i,j) \
    (!j ? (octet[(i)++] |=bit, octet[(i)]=0, j=7) : \
     (octet[i] |=(bit<< j--)))

/* mod -pointers are faster than array lookup */
#define put_bit_ptr(octet,bit,i,j) \
    (!j ? (*octet++ |=bit, *octet=0, i++, j=7) : \
     (*octet |=(bit<< j--)))

#define equal_time(s1,us1,s2,us2) \
    ((s1) == (s2) ? ((us1) == (us2) ? 1 : 0) : 0)

#define min(x, y) ((x) < (y) ? (x) : (y))

#define maxi(x, y) ((x) > (y) ? (x) : (y))

static unsigned char ZIG[] =
  { /* Raw coefficient block in ZIGZAG scan order */
   0,0,1,2,1,0,0,1,2,3,4,3,2,1,0,0,1,2,3,4,5,6,5,4,3,2,1,0,0,1,2,3,
   4,5,6,7,7,6,5,4,3,2,1,2,3,4,5,6,7,7,6,5,4,3,4,5,6,7,7,6,5,6,7,7
  };

static unsigned char ZAG[] =
  { /* Column coefficient block in ZIGZAG scan order */
   0,1,0,0,1,2,3,2,1,0,0,1,2,3,4,5,4,3,2,1,0,0,1,2,3,4,5,6,7,6,5,4,
   3,2,1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,3,4,5,6,7,7,6,5,4,5,6,7,7,6,7
  };


FILE *F_loss;
BOOLEAN STAT_MODE;
#ifdef ADAPTATION
FILE *file_rate, *file_loss;
double dump_rate=0.0;
int n_rate=0;
#endif


int declare_listen(num_port)
    int num_port;

  {
    int size_of_ad_in;
    struct sockaddr_in ad_in;
    int sock;

    bzero((char *)&ad_in, sizeof(ad_in));
    ad_in.sin_family = AF_INET;
    ad_in.sin_port = htons((u_short)num_port);
    ad_in.sin_addr.s_addr = INADDR_ANY;

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      fprintf(stderr,"declare_listen : cannot create socket-controle UDP\n");
      exit(1);
    }
    size_of_ad_in = sizeof(ad_in);
    if (bind(sock, (struct sockaddr *) &ad_in, size_of_ad_in) < 0){
      perror("declare_listen'bind");
      close(sock);
      exit(1);
    }
    if(!num_port){
      getsockname(sock, (struct sockaddr *)&ad_in, &size_of_ad_in);
      fprintf(stderr, "Listening at %d port number\n", ad_in.sin_port);
    }
    return(sock);
  }    



int how_long_ago(sec, usec, timestamps, cpt)
     u_short sec, usec;
     TIMESTAMP timestamps[256];
     int cpt;
{
  /*
  *  Return n if (n-1) images have been encoded since the lost image.
  */
  register int i, j;

  for(i=0; i<255; i++){
    j = (cpt + 256 - i) % 256;
    if(equal_time(sec, usec, timestamps[j].sec, timestamps[j].usec))
      break;
  }
  return(i+1);
}



BOOLEAN CheckControlPacket(sock, cpt, timestamps, ForceGobEncoding, INC_NG, 
			   FULL_INTRA, SUPER_CIF)
     int sock, cpt;
     TIMESTAMP timestamps[256];
     u_char *ForceGobEncoding;
     int INC_NG;
     BOOLEAN *FULL_INTRA, SUPER_CIF;
     
{
    int er, mask, force_value, k, n;
    static int mask0;
    static struct timeval tim_out;
    static BOOLEAN FIRST=TRUE;
    struct sockaddr_in adr;
    u_char buf[200];
    u_char Length, Type, Flow, CN, FGOBL, LGOBL;
    u_short timestamp_sec, timestamp_usec;
    BOOLEAN F;
    BOOLEAN LOSS=FALSE;

    if(FIRST){
      mask0 = 1 << sock;
      tim_out.tv_sec = 0;
      tim_out.tv_usec = 0;
      FIRST = FALSE;
    }

    if(SUPER_CIF){
      bzero((char *) ForceGobEncoding, 52);
    }else{
      bzero((char *) ForceGobEncoding, 13);
    }
    mask = mask0;

    /*
    *   Reverse control: Feedback info from video decoders.
    *
    * There are two control packets:
    *
    *
    * Full Intra Request : Type = FIR (0000)
    *
    *   0                   1                   2                   3
    *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *  |F|    RAD      |  Length = 1   |      Type     |MBZ|   Flow    |
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *
    * With:
    *       F = [1]
    *       RAD = [65] Reverse Application Data.
    *       Flow = [0] Only one video flow per station.
    *       MBZ = [0]  Reserved.
    *
    * Negative Acknowledge : Type = NACK (0001)
    *
    *   0                   1                   2                   3
    *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *  |F|    RAD      |  Length = 3   |      Type     |MBZ|   Flow    |
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *  |     FGOBL     |     LGOBL     |         MBZ             | FMT |
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *  | timestamp (seconds)           | timestamp (fraction)          |
    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *
    * With:
    *       F = [0] if another NACK is following; [1] else.
    *       FGOBL = First GOB number lost.
    *       LGOBL = Last GOB number lost.
    *       FMT = Lost image format.
    *       timestamp: timestamp of the lost image.
    */

    er = sizeof(adr);

    while(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) 
       != 0){

      if (recvfrom(sock, (char *)buf, 200, 0, (struct sockaddr *)&adr, &er) 
	  < 0){
	perror("recvfrom control packet");
	return(FALSE);
      }
      if(*FULL_INTRA == FALSE){
	k = 0;
	do{
	  F = buf[k] >> 7;
	  if((buf[k] & 0x7f) != RTP_RAD)
	    fprintf(stderr, "Bad RAD field data in control packet: %d\n",
		    buf[k] & 0x8f);
	  Length = buf[k+1];
	  Type = buf[k+2];
	  Flow = buf[k+3] & 0x3f;
	  if(Flow != 0)
	    fprintf(stderr, "Bad flow in control packet data: %d\n", Flow);
	  switch(Type){
	  case RTP_RAD_FIR:
	    *FULL_INTRA = TRUE;
	    break;
	  case RTP_RAD_NACK:
	    LOSS = TRUE;
	    FGOBL = buf[k+4];
	    LGOBL = buf[k+5];
	    CN = buf[k+7] & 0x03;
	    timestamp_sec = (buf[k+8] << 8) + buf[k+9];
	    timestamp_usec = (buf[k+10] << 8) + buf[k+11];
	    force_value = how_long_ago(timestamp_sec, timestamp_usec, 
				       timestamps, cpt);
	    if(force_value > 255){
	      fprintf(stderr, " Couln't find %ds, %dus timestamps image\n",
		      timestamp_sec, timestamp_usec);
	    }else{
	      if(SUPER_CIF){
		int aux=CN*13;
		for(n=FGOBL; n<=LGOBL; n+=INC_NG){
		  if(ForceGobEncoding[aux+n] < force_value)
		    ForceGobEncoding[aux+n] = force_value;
#ifdef DEBUG
		  printf("ForceGobEncoding[%d][%2d] = %d\n", CN, n, 
			 ForceGobEncoding[aux+n]);	
#endif
		}      
	      }else{
		for(n=FGOBL; n<=LGOBL; n+=INC_NG){
		  if(ForceGobEncoding[n] < force_value)
		    ForceGobEncoding[n] = force_value;
#ifdef DEBUG
		  printf("ForceGobEncoding[%2d] = %d\n", n, 
			 ForceGobEncoding[n]);	      
#endif
		}
	      }
	    }
	    break;
	  default:
	    fprintf(stderr, "Bad Type control packet: %d\n", Type);
	  }
/*	  
	  if(Type == RTP_RAD_NACK){
	    if(FGOBL != LGOBL)
	      fprintf(stderr, 
		      "**NACK CIF %d, GOB %d to %d, %ds, %dus are lost\n", 
		      CN, FGOBL, LGOBL, timestamp_sec, timestamp_usec);
	    else
	      fprintf(stderr, 
		      "*NACK CIF %d, GOB %d, %ds, %dus is lost\n",
		      CN, FGOBL, timestamp_sec, timestamp_usec);
	  }
*/
	  k += (Length << 2);
	}while(!F && (*FULL_INTRA==FALSE));
      }
      mask = mask0;
    }
    return(LOSS);
  }



void coffee_break(delay)
     long delay;
{
  struct timeval timer;
  static BOOLEAN First=TRUE;
  static int sock;

  if(First){
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      perror("coffee_break creation socket ");
      return;
    }
    First = FALSE;
  }    
  timer.tv_sec = (delay/1000000);
  timer.tv_usec = (delay % 1000000);
  select(sock+1, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timer);
  return;

}



encode_h261(group, port, ttl, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX, 
	    sock_recv, FIRST, COLOR, FEEDBACK, rate_control, drate_max,
	    default_quantizer, fd)
     char *group, *port;
     u_char ttl;       /* ttl value */
     u_char new_Y[][COL_MAX], new_Cb[][COL_MAX_DIV2], new_Cr[][COL_MAX_DIV2];
     IMAGE_CIF image_coeff;
     int NG_MAX;       /* Maximum NG number */
     int sock_recv;    /* socket used for loss messages */
     BOOLEAN FIRST;    /* True if it is the first image */
     BOOLEAN COLOR;    /* True if color encoded */
     BOOLEAN FEEDBACK; /* True if feedback is allowed */
     int rate_control; /* WITHOUT_CONTROL or REFRESH_CONTROL 
			  or QUALITY_CONTROL */
     int drate_max;     /* Maximum bandwidth allowed */
     int default_quantizer;
     int fd;           /* Pipe son to father for bandwidth & rate display */

  {
    int lig, col, ligdiv2, coldiv2;
    int NG, MBA, INC_NG, CIF;
    GOB *gob;
    MACRO_BLOCK *mb;
    static u_char old_Y[LIG_MAX][COL_MAX];
    static u_char old_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
    static u_char old_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
    int diff_Y[LIG_MAX][COL_MAX];
    int diff_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
    int diff_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
    int cc, quant_mb;
    int k;
    int bnul, mbnul;
    int cmax0, cmax1, cmax2, cmax3, cmax4, cmax5;
    int *tab_quant1, *tab_quant2;
    u_char chaine_codee[T_MAX];
    int i, j;
    static int lwrite;
    struct timeval realtime;
    static double inittime, synctime;
    static int RT;
    static int LIG[16]={0, 2, 0, 1, 3, 1, 3, 2, 0, 3, 2, 1, 3, 0, 2, 1};
    static int COL[16]={0, 2, 3, 1, 0, 2, 3, 1, 2, 1, 3, 0, 2, 1, 0, 3};
    static int MASK_CBC[6]={0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    static int n_im=0;
    int lig11, lig12, lig21, lig22, col11, col12, col21, col22;
    static int Nblock=0;
    int max_coeff, max;
    static int cpt=0;
    BOOLEAN GobEncoded[12];
    static u_char ForceGobEncoding[13];
    extern int ffct_4x4(), ffct_8x8();
    extern int ffct_intra_4x4(), ffct_intra_8x8();
    extern int chroma_ffct_8x8(), chroma_ffct_intra_8x8();
    int (*ffct)()=ffct_8x8;
    int (*ffct_intra)()=ffct_intra_8x8;
    int nblock_cif_max;  /* CIF  : if nb(blocks) detected > this value */
    int nblock_qcif_max; /* QCIF :    then use INTRA mode              */
    BOOLEAN VIDEO_PACKET_HEADER; /* True if it is set */
    static int BperMB, full_cbc;
    static struct sockaddr_in addr;
    static int sock, len_addr;
    static int max_value; /* A block is forced every max_value images */
    static int max_inter; /* max number of successive INTER mode encoding */
    int force_value;
    static int offsetY, offsetC; /* Useful to pointers alignment */
    static int threshold=DEFAULT_THRESHOLD; /* For motion estimation */
    static int quantizer;
    static int length_image=0;
    static int len_data=11;
    static double fps, bandwidth;
    char data[11];
    static u_short sequence_number=0;
    BOOLEAN FIRST_GOB_IS_ENCODED=TRUE;
    TIMESTAMP timestamps[256];
    static BOOLEAN FULL_INTRA=TRUE;
    static int rate_max;

#ifndef ADAPTATION
    rate_max = drate_max;
#endif

    CIF = (NG_MAX != 5);
    if(CIF){
      max_value = (FEEDBACK ? 50 : 15);
      INC_NG = 1;
    }else{
      /* QCIF */
      max_value = (FEEDBACK ? 100 : 30);
      INC_NG = 2;
    }
    max_inter = (FEEDBACK ? 20 : 3);

/*
*   The RTP Header...

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Ver| flow      |0|S|  content  | sequence number               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp (seconds)           | timestamp (fraction)          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |S|Sbit |E|Ebit |C|I|V|MBZ| FMT |         H.261 stream...       :
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

With:

Ver:  (2 bits) RTP version number. [01]
flow: (6 bits) flow number. [000000]
S:(1 bit) Synchronized bit: [1] if it is the first packet of a GOB.
                              Note that the GBSC is not encoded and 
			      must be appended before decoding.
			    [0] else.
content: (6 bits) H.261 type [011111]
sequence number: (16 bits)
timestamp: (32 bits)
S: (1 bit) [1] if beginning of GOB; [0] else.
Sbit: (3 bits) Indicates the number of bits that should be ignored in the
               first (start) data octet. (Here always 0...)
E: (1 bit) [1] if end of GOB; [0] else.
Ebit: (3 bits) Indicates the number of bits that should be ignored in the 
               last (end) data octet.
C: (1 bit) [1] if color encoded; [0] else.
I: (1 bit) [1] if it is the first packet a full intra encoded image;
           [0] else.
V: (1 bit) [1] if movement vectors are encoded; [0] else.
FMT: (3 bits) Image format:
            [000] QCIF; [001] CIF; [100] upper left corner CIF;
            [101] upper right corner CIF; [110] lower left corner CIF;
            [111] lower right corner CIF.
*
*/
    chaine_codee[0] = RTP_BYTE1;
    chaine_codee[1] = RTP_BYTE2_SYNC;
    chaine_codee[2] = (u_char)(sequence_number >> 8);
    chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
    sequence_number = (sequence_number + 1) % 65536;
    gettimeofday(&realtime, (struct timezone *)NULL);
    chaine_codee[4] = (u_char)((realtime.tv_sec >> 8) & 0x0000ff);
    chaine_codee[5] = (u_char)(realtime.tv_sec & 0x000000ff);
    chaine_codee[6] = (u_char)((realtime.tv_usec >> 12) & 0xff);
    chaine_codee[7] = (u_char)((realtime.tv_usec >> 4) & 0xff);
    chaine_codee[8] = 0x88; /* By default, E = S = 1 */

    timestamps[cpt%256].sec = (u_short)(realtime.tv_sec & 0xffff);
    timestamps[cpt%256].usec = (u_short)(realtime.tv_usec >> 4);

    if(COLOR){
      if(CIF){
	if(FULL_INTRA == FALSE)
	  chaine_codee[9] = 0x81; /* color + cif */
	else
	  chaine_codee[9] = 0xc1; /* color + full intra + cif */
      }else{
	if(FULL_INTRA == FALSE)
	  chaine_codee[9] = 0x80; /* color + qcif */
	else
	  chaine_codee[9] = 0xc8; /* color + full intra + qcif */
      }
    }else{
      if(CIF){
	if(FULL_INTRA == FALSE)
	  chaine_codee[9] = 0x01; /* cif */
	else
	  chaine_codee[9] = 0x41; /* cif + full_intra */
      }else{
	if(FULL_INTRA == FALSE)
	  chaine_codee[9] = 0x00; /* qcif */
	else
	  chaine_codee[9] = 0x40; /* qcif + full_intra */
      }
    }
    chaine_codee[10] = 0x00;
    i = 10;
    j = 7;

    VIDEO_PACKET_HEADER=TRUE;

    /*************\
    * IMAGE LAYER *
    \*************/

    if(FIRST){
      /* Some initializations ... */
      offsetY = COL_MAX - 8;
      offsetC = COL_MAX_DIV2 - 8;
      inittime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      RT=0;
      bzero((char *)image_coeff, sizeof(IMAGE_CIF));
      sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
      rate_max = drate_max;

#ifdef ADAPTATION
      {
	char host[100];
	char filename[150];

	if(gethostname(host, 100) != 0){
	  fprintf(stderr, "Cannot get local hostname\n");
	  exit(1);
	}
	strcpy(filename, "rate_max.");
	strcat(filename, host);
	if((file_rate=fopen(filename, "w")) == NULL){
	  fprintf(stderr, "cannot create %s file\n", filename);
	  exit(1);
	}
	strcpy(filename, "loss.");
	strcat(filename, host);
	if((file_loss=fopen(filename, "w")) == NULL){
	  fprintf(stderr, "cannot create %s file\n", filename);
	  exit(1);
	}
      }
#endif 
      if(COLOR){
	BperMB = 6;
	full_cbc = 63;
	nblock_cif_max = 2200;
	nblock_qcif_max = 500;
      }else{
	BperMB = 4;
	full_cbc = 60;
	nblock_cif_max = 1500;
	nblock_qcif_max = 350;
      }

      FIRST = FALSE;
    }else{
      double rate;

      synctime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      rate = ((double)(0.008*length_image) / (synctime-inittime));
#ifdef ADAPTATION
      n_rate ++;
      dump_rate += rate;
#endif
      RT=((RT+(int)(33*(synctime-inittime))) & 0x1f);
#ifdef DEBUG
      fprintf(stderr, "Encoding time: %d ms\n", 
	      (int)(1000*(synctime-inittime)));
#endif

      if(rate_control == QUALITY_CONTROL){

	static double mem_rate;
	static int step=1;
	int delta;
	int rate_min;
	static int cpt_min=0;

	rate_min = (int)(0.7*rate_max); /* Hysteresis 30% */
	if(cpt % 2 == 1){
	  mem_rate = (mem_rate + rate) / 2.0;
	  mem_rate = 0.0;
	}else{
	  mem_rate = rate;
	}
	if(rate < rate_min){
	  cpt_min ++;
	  if(cpt_min > 3){
	    delta = 1;
	    cpt_min = 0;
	    step = (step-delta >=0 ? step-delta : 0);
	  }
	}else{
	  if(rate > rate_max){
	    delta = ((rate-rate_max)/10 + 1);
	    step = (step+delta <= step_max ? step+delta : step_max);
	  }
	  cpt_min = 0;
	}
	quantizer = Step[step].quant;
	threshold = Step[step].threshold;
      }else{
	quantizer = default_quantizer;
	threshold = ((rate_control == REFRESH_CONTROL) ? LOW_THRESHOLD :
		     DEFAULT_THRESHOLD);
      }
      
      fps += (1.0 / (synctime-inittime));
      bandwidth += ((double)(0.008*length_image) / (synctime-inittime));
      if((cpt > 0) && (cpt % 3 == 0)){
	bzero(data, len_data);
	sprintf(data, "%2.1f", bandwidth/3.0);
	sprintf(&data[6], "%2.1f", fps/3.0);
	if((lwrite=write(fd, data, len_data)) != len_data){
#ifdef DEBUG
	  perror("encode_h261: Cannot send bandwidth and rate data");
#endif
	}
	fps = 0.0;
	bandwidth = 0.0;
      }
      Nblock = 0;
      length_image = 0;
      inittime=synctime;
    }
#ifdef DEBUG
    fprintf(stderr, "QUANT: %d, RT: %d\n", quantizer, RT);
#endif

    put_value(&chaine_codee[i],0,4,&i,&j); /* NG only */

    put_value(&chaine_codee[i],RT,5,&i,&j); /* RT */
    if(CIF)
	put_value(&chaine_codee[i],4,6,&i,&j); /* TYPEI = CIF */
    else
	put_value(&chaine_codee[i],0,6,&i,&j); /* TYPEI = QCIF */
    put_bit(chaine_codee,0,i,j); /* ISI1 */

    for(k=0; k<NG_MAX; k+=INC_NG)
      GobEncoded[k]=TRUE;
    
    if(FULL_INTRA == FALSE){
      lig11=LIG[n_im];
      col11=COL[n_im];
      n_im=(n_im+1)%16;
      lig12=lig11+4;
      col12=col11+4;
      lig21=lig11+8;
      lig22=lig12+8;
      col21=col11+8;
      col22=col12+8;
      k=1;

      for(NG=1; NG<=NG_MAX; NG+=INC_NG){

	/* GOB LOOP */

	mbnul=0;
	gob = CIF ? (GOB *)(image_coeff)[NG-1] : (GOB *)(image_coeff)[NG-k++];
	force_value = ForceGobEncoding[NG];

	for(MBA=1; MBA<34; MBA++){
	  register int sum;
	  int lig_11, lig_12, lig_21, lig_22, col_11, col_12, col_21, col_22;

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];
	  lig_11=lig+lig11;
	  lig_12=lig+lig12;
	  lig_21=lig+lig21;
	  lig_22=lig+lig22;
	  col_11=col+col11;
	  col_12=col+col12;
	  col_21=col+col21;
	  col_22=col+col22;

	  bnul=0;
	  mb = &((*gob)[MBA-1]);

	  /* We choose MB INTRA encoding when a MB has been lost
	  * or more than max_inter INTER encoding have been done or
	  * every max_value images (in order to prevent loss packets).
	  */

	  if((mb->last_encoding > force_value) &&
	     (mb->last_encoding < max_value) && (mb->cpt_inter < max_inter)){

	    mb->CBC = 0;
	    mb->MODE = INTER;

	    /* B movement detection */
	    
	    sum =  abs((int)new_Y[lig_11][col_11] - 
		       (int)old_Y[lig_11][col_11]);
	    sum += abs((int)new_Y[lig_11][col_12] - 
		       (int)old_Y[lig_11][col_12]);
	    sum += abs((int)new_Y[lig_12][col_11] - 
		       (int)old_Y[lig_12][col_11]);
	    sum += abs((int)new_Y[lig_12][col_12] - 
		       (int)old_Y[lig_12][col_12]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 32;
	    }else
	      bnul ++;
	  
	    sum =  abs((int)new_Y[lig_11][col_21] - 
		       (int)old_Y[lig_11][col_21]);
	    sum += abs((int)new_Y[lig_11][col_22] - 
		       (int)old_Y[lig_11][col_22]);
	    sum += abs((int)new_Y[lig_12][col_21] - 
		       (int)old_Y[lig_12][col_21]);
	    sum += abs((int)new_Y[lig_12][col_22] - 
		       (int)old_Y[lig_12][col_22]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 16;
	    }else
	      bnul ++;

	    sum =  abs((int)new_Y[lig_21][col_11] - 
		       (int)old_Y[lig_21][col_11]);
	    sum += abs((int)new_Y[lig_21][col_12] - 
		       (int)old_Y[lig_21][col_12]);
	    sum += abs((int)new_Y[lig_22][col_11] - 
		       (int)old_Y[lig_22][col_11]);
	    sum += abs((int)new_Y[lig_22][col_12] - 
		       (int)old_Y[lig_22][col_12]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 8;
	    }else
	      bnul ++;

	    sum =  abs((int)new_Y[lig_21][col_21] - 
		       (int)old_Y[lig_21][col_21]);
	    sum += abs((int)new_Y[lig_21][col_22] - 
		       (int)old_Y[lig_21][col_22]);
	    sum += abs((int)new_Y[lig_22][col_21] - 
		       (int)old_Y[lig_22][col_21]);
	    sum += abs((int)new_Y[lig_22][col_22] - 
		       (int)old_Y[lig_22][col_22]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 4;
	    }else
	      bnul ++;

	    if(COLOR){
	      if(mb->CBC){
		Nblock += 2;
		mb->CBC |= 3;
	      }else
		bnul += 2;
	    }

	    if(bnul == BperMB){
	      mbnul ++;
	      (mb->last_encoding)++;
	      if(mbnul == 33){
		GobEncoded[NG-1]=FALSE;
	      }
	    }else{
	      mb->last_encoding = 1;
	      (mb->cpt_inter)++;
	    }
	  }else{
	    /* INTRA MB encoding */
	    mb->MODE = INTRA;
	    Nblock += BperMB;
	  }
	} /* MACROBLOCK LOOP */
      } /* GOB LOOP */
    } /* if(!FULL_INTRA) */

    /* If there is a change of scene,
    * a full Intra image encoding is done.
    */

    if(FULL_INTRA == FALSE){
      if(CIF){
	if(Nblock > nblock_cif_max){
#ifdef DEBUG
	  fprintf(stderr, "Full intra encoding\n");
#endif
          FULL_INTRA = TRUE;
	  chaine_codee[9] |= 0x40;
	  Nblock = 1584;
      }
      }else{
	if(Nblock > nblock_qcif_max){
#ifdef DEBUG
	  fprintf(stderr, "Full intra encoding\n");
#endif
          FULL_INTRA = TRUE;
	  chaine_codee[9] |= 0x40;
	  Nblock = 396;
	}
      }
    }

    k=1;
    for(NG=1; NG<=NG_MAX; NG+=INC_NG){

      /* GOB LOOP */

      if(FULL_INTRA || GobEncoded[NG-1]){
	mbnul=0;
	gob = CIF ? (GOB *)(image_coeff)[NG-1] : (GOB *)(image_coeff)[NG-k++]; 

	for(MBA=1; MBA<34; MBA++){

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];

	  coldiv2 = col >> 1;
	  ligdiv2 = lig >> 1;
	  mb = &((*gob)[MBA-1]);

	  if((FULL_INTRA == FALSE) && (mb->MODE == INTER)){

	    max_coeff=0;
	    if((mb->CBC) & 0x20){
	      register int l, c;
	      register int *pt_image = &diff_Y[lig][col];
	      register u_char *pt_new = &new_Y[lig][col];
	      register u_char *pt_old = &old_Y[lig][col];
	      for(l=0; l<7; l++){
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=0; c<8; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[lig][col], mb->P[0], COL_MAX, TRUE, &cmax0);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x10){
	      register int l, c;
	      int auxc=col+8;
	      register int *pt_image = &diff_Y[lig][auxc];
	      register u_char *pt_new = &new_Y[lig][auxc];
	      register u_char *pt_old = &old_Y[lig][auxc];
	      for(l=0; l<7; l++){
		for(c=8; c<16; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=8; c<16; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[lig][auxc], mb->P[1], COL_MAX, TRUE, 
			  &cmax1);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x08){
	      register int l, c;
	      int auxl = lig+8;
	      register int *pt_image = &diff_Y[auxl][col];
	      register u_char *pt_new = &new_Y[auxl][col];
	      register u_char *pt_old = &old_Y[auxl][col];
	      for(l=8; l<15; l++){
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=0; c<8; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[auxl][col], mb->P[2], COL_MAX, TRUE, 
			  &cmax2);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x04){
	      register int l, c;
	      int auxl = lig+8;
	      int auxc = col+8;
	      register int *pt_image = &diff_Y[auxl][auxc];
	      register u_char *pt_new = &new_Y[auxl][auxc];
	      register u_char *pt_old = &old_Y[auxl][auxc];
	      for(l=8; l<15; l++){
		for(c=8; c<16; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=8; c<16; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[auxl][auxc], mb->P[3], COL_MAX, TRUE, 
			  &cmax3);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if(COLOR){
	      if((mb->CBC) & 0x02){
		register int l, c;
		register int *pt_image = &diff_Cb[ligdiv2][coldiv2];
		register u_char *pt_new = &new_Cb[ligdiv2][coldiv2];
		register u_char *pt_old = &old_Cb[ligdiv2][coldiv2];
		for(l=0; l<7; l++){
		  for(c=0; c<8; c++)
		    *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		  pt_image += offsetC;
		  pt_new += offsetC;
		  pt_old += offsetC;
		}
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		max=chroma_ffct_8x8(&diff_Cb[ligdiv2][coldiv2], mb->P[4], 
				    COL_MAX_DIV2, TRUE, &cmax4);
		if(max>max_coeff)
		  max_coeff=max;
	      }
	      if((mb->CBC) & 0x01){
		register int l, c;
		register int *pt_image = &diff_Cr[ligdiv2][coldiv2];
		register u_char *pt_new = &new_Cr[ligdiv2][coldiv2];
		register u_char *pt_old = &old_Cr[ligdiv2][coldiv2];
		for(l=0; l<7; l++){
		  for(c=0; c<8; c++)
		    *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		  pt_image += offsetC;
		  pt_new += offsetC;
		  pt_old += offsetC;
		}
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		max=chroma_ffct_8x8(&diff_Cr[ligdiv2][coldiv2], mb->P[5], 
			    COL_MAX_DIV2, TRUE, &cmax5);
		if(max>max_coeff)
		  max_coeff=max;
	      }
	    }
	  }else{
	    /* INTRA encoding mode */

	    mb->MODE = INTRA;
	    mb->CBC = full_cbc;
	    mb->last_encoding = 1;
	    mb->cpt_inter = 0;
	    max_coeff = 0;
	    max=(*ffct_intra)(&new_Y[lig][col], mb->P[0], COL_MAX, &cmax0);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig][col+8], mb->P[1], COL_MAX, &cmax1);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig+8][col], mb->P[2], COL_MAX, &cmax2);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig+8][col+8], mb->P[3], COL_MAX, &cmax3);
	    if(max>max_coeff)
	      max_coeff=max;
	    if(COLOR){
	      max=chroma_ffct_intra_8x8(&new_Cb[ligdiv2][coldiv2], mb->P[4], 
				COL_MAX_DIV2, &cmax4);
	      if(max>max_coeff)
		max_coeff=max;
	      max=chroma_ffct_intra_8x8(&new_Cr[ligdiv2][coldiv2], mb->P[5], 
				COL_MAX_DIV2, &cmax5);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	  }
	  if(mb->CBC){
	    register int n, l, c;

#ifdef QUANT_FIX
	    quant_mb = default_quantizer;
#else
	    quant_mb = (max_coeff >> 8) + 1;
	    if(quantizer > quant_mb){
	      quant_mb = (FULL_INTRA ? 5 : quantizer); 
	    }
#endif
	    mb->QUANT = quant_mb;
	    /*
	     *  We have to quantize mb->P[n].
	     */
	    bnul=0;
	    if(quant_mb & 0x01){
	      /* odd quantizer */
	      for(n=0; n<BperMB; n++){
		register int *pt_P = &(mb->P[n][0][0]);
		register int nb_coeff=0;

		if((mb->CBC) & MASK_CBC[n]){
		  cc = *pt_P;
		  for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=(*pt_P / quant_mb - 1) / 2)){
			  nb_coeff ++;
			  if(*pt_P > 127)
			    *pt_P = 127;
			}
		      }else{
			if(*pt_P < 0)
			  if((*pt_P=(*pt_P /quant_mb + 1) / 2)){
			    nb_coeff ++;
			    if(*pt_P < (-127))
			      *pt_P = -127;
			  }
		      }
		      pt_P ++;
		    }
		  if(mb->MODE == INTRA){
		    if((mb->P[n][0][0]==0) && cc)
		      nb_coeff ++;
		    mb->P[n][0][0] = cc;
		  }
		  if(!nb_coeff){
		    bnul ++;
		    (mb->CBC) &= (~MASK_CBC[n]);
		  }
		}else
		  bnul ++;
		mb->nb_coeff[n] = nb_coeff;
	      }
	    }else{
	      /* even quantizer */
	      for(n=0; n<BperMB; n++){
		register int *pt_P = &(mb->P[n][0][0]);
		register int nb_coeff=0;

		if((mb->CBC) & MASK_CBC[n]){
		  cc = *pt_P;
		  for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=((*pt_P + 1)/quant_mb - 1)/2 )){
			  nb_coeff ++;
			  if(*pt_P > 127)
			    *pt_P = 127;
			}
		      }else{
			if(*pt_P < 0)
			  if((*pt_P=((*pt_P - 1)/quant_mb - 1)/2 )){
			    nb_coeff ++;
			    if(*pt_P < (-127))
			      *pt_P = -127;
			  }
		      }
		      pt_P ++;
		    }
		  if (mb->MODE == INTRA) {
		      if((!mb->P[n][0][0]) && cc)
			  nb_coeff ++;
		      mb->P[n][0][0] = cc;
		  }
		  if(! nb_coeff){
		      bnul ++;
		      (mb->CBC) &= (~MASK_CBC[n]);
		  }
		}else
		  bnul ++;
		mb->nb_coeff[n] = nb_coeff;
	      }		      
	    }
	    if(bnul == BperMB){
	      mbnul ++;
	      if(mbnul == 33){
		GobEncoded[NG-1] = FALSE;
	      }
	    }
	  }
	  if(bnul != BperMB){
	    /************\
	    * IFCT of MB *
	    \************/
	    int inter = mb->MODE;
	    tab_quant1 = tab_rec1[quant_mb-1];
	    tab_quant2 = tab_rec2[quant_mb-1];
	    
	    if((mb->CBC) & 0x20)
	      ifct_8x8(mb->P[0], tab_quant1, tab_quant2, inter,
		       &(old_Y[lig][col]), cmax0, COL_MAX);
	  
	    if((mb->CBC) & 0x10)
	      ifct_8x8(mb->P[1], tab_quant1, tab_quant2, inter, 
		       &(old_Y[lig][col+8]), cmax1, COL_MAX);
	  
	    if((mb->CBC) & 0x08)
	      ifct_8x8(mb->P[2], tab_quant1, tab_quant2, inter,
		       &(old_Y[lig+8][col]), cmax2, COL_MAX);

	    if((mb->CBC) & 0x04)
	      ifct_8x8(mb->P[3], tab_quant1, tab_quant2, inter,
		       &(old_Y[lig+8][col+8]), cmax3, COL_MAX);
	    if(COLOR){
	      if((mb->CBC) & 0x02)
		ifct_8x8(mb->P[4], tab_quant1, tab_quant2, inter,
			 &(old_Cb[ligdiv2][coldiv2]), cmax4, COL_MAX_DIV2);
	      if((mb->CBC) & 0x01)
		ifct_8x8(mb->P[5], tab_quant1, tab_quant2, inter,
			 &(old_Cr[ligdiv2][coldiv2]), cmax5, COL_MAX_DIV2);
	    }
	  }
	} /* MACROBLOCK LOOP */
      }else
	k ++;
      {
	int last_AM, quant=quantizer;
	register int NM, b;

	if(!VIDEO_PACKET_HEADER){
	  chaine_codee[2] = (u_char)(sequence_number >> 8);
	  chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
	  sequence_number = (sequence_number + 1) % 65536;
	  if(COLOR){
	    if(CIF)
	      chaine_codee[9] = 0x81; /* color + cif */
	    else
	      chaine_codee[9] = 0x80; /* color + qcif */
	  }else{
	    if(CIF)
	      chaine_codee[9] = 0x01; /* cif */
	    else
	      chaine_codee[9] = 0x00; /* qcif */
	  }
	  chaine_codee[10] = 0x00;
	  i = 10;
	  j = 7;
	  VIDEO_PACKET_HEADER = TRUE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}

	/*********************\
	* H.261 CODING OF GOB *
	\*********************/

	if(FIRST_GOB_IS_ENCODED){
	  put_value(&chaine_codee[i],1,16,&i,&j); 
	}else
	  FIRST_GOB_IS_ENCODED = TRUE;

	/* The GBSC not be encoded (S bit used) */
	put_value(&chaine_codee[i],NG,4,&i,&j); 
	put_value(&chaine_codee[i],quant,5,&i,&j);
	put_bit(chaine_codee,0,i,j); /* ISG */

	if(GobEncoded[NG-1]){ 

	  /******************\
          * MACROBLOCK LAYER *
	  \******************/
	
          last_AM = 0;
	  for(NM=1; NM<34; NM++){
	      BOOLEAN CBC_IS_CODED;
	    
	    mb = &((*gob)[NM-1]);
	    if(!(mb->CBC)) /* If MB is entirely null, continue ... */
	      continue;
	    if(!chain_forecast(tab_mba,n_mba,(NM-last_AM),chaine_codee,&i,&j)){
	      fprintf(stderr,"Bad MBA value: %d\n", NM-last_AM);
	      exit(-1);
	    }
	    CBC_IS_CODED = (mb->MODE ==INTER);    
	    if(mb->QUANT != quant){
	      quant=mb->QUANT;
	      if(CBC_IS_CODED)
		/*TYPEM=INTRA+CBC+QUANTM */
		put_value(&chaine_codee[i],1,5,&i,&j);
	      else
		/* TYPEM = INTRA+QUANTM */
		put_value(&chaine_codee[i],1,7,&i,&j);
	      /* Coding of QUANTM */
	      put_value(&chaine_codee[i],quant,5,&i,&j); 
	    }
	    else{
	      if(CBC_IS_CODED)
		/* TYPEM = INTRA+CBC */
		put_bit(chaine_codee,1,i,j); 
	      else
		/* TYPEM = INTRA */
		put_value(&chaine_codee[i],1,4,&i,&j); 
	    }
	    if(CBC_IS_CODED){
	      if(!chain_forecast(tab_cbc, n_cbc, (int)mb->CBC, chaine_codee,
				 &i, &j)){
		fprintf(stderr,"Bad CBC value: %d\n", mb->CBC);
		exit(-1);
	      }
	    }
	    /*************\
	    * BLOCK LAYER *
	    \*************/
	    for(b=0; b<BperMB; b++){
	      if(mb->CBC & MASK_CBC[b]){
		code_bloc_8x8(mb->P[b], chaine_codee, tab_tcoeff, 
			      n_tcoeff, mb->nb_coeff[b], &i, &j, mb->MODE);
	      }
	    }
	    last_AM = NM;
	  } 
	}else{
	    ; /* This GOB is unchanged */
	}

	/* If there is enough data or it is the last GOB,
	* write data on socket
	*/

	if((i > 500) || (NG == NG_MAX)){
	  int Ebit;
	  register int k;

	  /* No padding is required when using Ebit... */

	  Ebit = (j + 1) % 8;
	  chaine_codee[8] = 0x88 | Ebit;

	  if(Ebit != 0){
	    /* We have to nullify the following bits */
	    for(k=0; k<Ebit; k++)
	      put_bit(chaine_codee,0,i,j);
	  }
	  
	  if(rate_control == REFRESH_CONTROL){
	    /* Control rate : Privilege quality rather than refreshment. Add
	    *                 if necessary a delay between each packet sent.
	    */
	    static BOOLEAN FIRST=TRUE;
	    static double lasttime;
	    struct timeval realtime;
	    double newtime, deltat;

	    if(FIRST){
	      FIRST=FALSE;
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      lasttime = 1000000.0*realtime.tv_sec + realtime.tv_usec;
	    }else{
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      newtime = 1000000.0*realtime.tv_sec + realtime.tv_usec;
	      deltat = 8000.0*i/rate_max - (newtime - lasttime);
	      if(deltat > 0){
		coffee_break((long)deltat);
		lasttime = newtime + deltat;
	      }else
		lasttime = newtime;
	    } 
	  }

	  /* Sending packet */
	  if(i < MAX_BSIZE){
	    if((lwrite=sendto(sock, (char *)chaine_codee, i, 0, 
			      (struct sockaddr *) &addr, len_addr)) < 0){
	      perror("sendto: Cannot send data on socket");
	      exit(1);
	    }
	    length_image += lwrite;
	  }else{
	    int rest, nsend, aux, Ebit;

	    /* E=0, Ebit=0 and S=1 */
	    chaine_codee[8] = 0x80;
	    rest = i;
	    if((lwrite=sendto(sock, (char *)chaine_codee, MAX_BSIZE, 0, 
			      (struct sockaddr *) &addr, len_addr)) < 0){
	      perror("sendto: Cannot send data on socket");
	      exit(1);
	    }
	    chaine_codee[8] = 0x00;
	    nsend = lwrite;
	    rest -= nsend;
	    while(rest != 0){
	      aux = MAX_BSIZE - 10;
	      bcopy((char *)&chaine_codee[nsend], (char *)&chaine_codee[10],
		    min(rest, aux));
	      aux = rest + 10;
	      if(aux <= MAX_BSIZE){
		/* E=1, Ebit */
		Ebit=(j + 1) % 8;
		chaine_codee[8] = 8 | Ebit;
	      }
	      chaine_codee[2] = (u_char)(sequence_number >> 8);
	      chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
	      sequence_number = (sequence_number + 1) % 65536;
	      if((lwrite=sendto(sock, (char *)chaine_codee, 
				min(aux, MAX_BSIZE), 0, 
				(struct sockaddr *) &addr, len_addr)) < 0){
		perror("sendto: Cannot send data on socket");
		exit(1);
	      }
	      aux = lwrite - 10;
	      nsend += aux;
	      rest -= aux;
	    }
	    length_image += i;
	  }
	  VIDEO_PACKET_HEADER = FALSE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}
      } /* gob encoding */
    } /* GOB LOOP */
    FULL_INTRA = FALSE;
    if(FEEDBACK){
      if(CheckControlPacket(sock_recv, cpt, timestamps, ForceGobEncoding, 
			    INC_NG, &FULL_INTRA, FALSE)){
#ifdef ADAPTATION
	rate_max = maxi(rate_max/2, 10);
	fprintf(file_loss, "%d\t10\n", cpt);
	fflush(file_loss);
#endif	  
      }else{
#ifdef ADAPTATION
	rate_max = min(rate_max+5, drate_max);
	fprintf(file_loss, "%d\t0\n", cpt);
	fflush(file_loss);
#endif
      }
#ifdef ADAPTATION
	fprintf(file_rate, "%d\t%d\n", cpt, rate_max);
	fflush(file_rate);
#endif
    }/* FEEDBACK allowed */
    cpt ++;
  }







  
encode_h261_cif4(group, port, ttl, new_Y, new_Cb, new_Cr, image_coeff, 
		 sock_recv, FIRST, COLOR, FEEDBACK, cif_number, rate_control,
		 drate_max, default_quantizer, fd)
     char *group, *port;  /* Socket address */
     u_char ttl;          /* ttl value */
     u_char new_Y[][COL_MAX], new_Cb[][COL_MAX_DIV2], new_Cr[][COL_MAX_DIV2];
     IMAGE_CIF image_coeff;
     int sock_recv;       /* socket used for loss messages */
     BOOLEAN FIRST;       /* True if it is the first image */
     BOOLEAN COLOR;       /* True if color encoded */
     BOOLEAN FEEDBACK;    /* True if feedback is allowed */
     int cif_number;      /* select the CIF number in the CIF4 (0,1,2 or 3) */
     int rate_control;    /* WITHOUT_CONTROL or REFRESH_CONTROL 
			   or QUALITY_CONTROL */
     int drate_max;        /* Maximum bandwidth allowed */
     int default_quantizer;
     int fd;              /* Pipe son to father for bandwidth & rate display */


  {
    int lig, col, ligdiv2, coldiv2;
    int NG, MBA;
    GOB *gob;
    MACRO_BLOCK *mb;
    static u_char old_Y[4][LIG_MAX][COL_MAX];
    static u_char old_Cb[4][LIG_MAX_DIV2][COL_MAX_DIV2];
    static u_char old_Cr[4][LIG_MAX_DIV2][COL_MAX_DIV2];
    int diff_Y[LIG_MAX][COL_MAX];
    int diff_Cb[LIG_MAX_DIV2][COL_MAX_DIV2];
    int diff_Cr[LIG_MAX_DIV2][COL_MAX_DIV2];
    int cc, quant_mb;
    int bnul, mbnul;
    int cmax0, cmax1, cmax2, cmax3, cmax4, cmax5;
    int *tab_quant1, *tab_quant2;
    u_char chaine_codee[T_MAX];
    int i, j;
    static int lwrite;
    struct timeval realtime;
    static double inittime, synctime;
    static int RT;
    static int LIG[16]={0, 2, 0, 1, 3, 1, 3, 2, 0, 3, 2, 1, 3, 0, 2, 1};
    static int COL[16]={0, 2, 3, 1, 0, 2, 3, 1, 2, 1, 3, 0, 2, 1, 0, 3};
    static int MASK_CBC[6]={0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    static int n_im=0;
    static int lig11, lig12, lig21, lig22, col11, col12, col21, col22;
    int Nblock;
    int max_coeff, max_inter, max;
    static int cpt=0;
    static BOOLEAN ForceGobEncoding[52];
    BOOLEAN GobEncoded[12];
    extern int ffct_4x4(), ffct_8x8();
    extern int ffct_intra_4x4(), ffct_intra_8x8();
    extern int chroma_ffct_8x8(), chroma_ffct_intra_8x8();
    int (*ffct)()=ffct_8x8;
    int (*ffct_intra)()=ffct_intra_8x8;
    int nblock_cif_max; 
    BOOLEAN VIDEO_PACKET_HEADER; /* True if it is set */
    static struct sockaddr_in addr;
    static int sock, len_addr;
    static BOOLEAN first_cif=TRUE;
    static int NG_MAX=12;
    static int BperMB, full_cbc;
    static int max_value; /* A block is forced every 30 images */
    int force_value;
    static int offsetY, offsetC; /* Useful to pointers alignment */
    static int threshold=DEFAULT_THRESHOLD; /* For motion estimation */
    static int quantizer;
    static int length_image=0;
    static int len_data=11;
    static double fps, bandwidth;
    char data[11];
    int off_cif_number;
    static u_short sequence_number=0;
    BOOLEAN FIRST_GOB_IS_ENCODED=TRUE;
    TIMESTAMP timestamps[256];
    static BOOLEAN FULL_INTRA=TRUE;
    static int rate_max;

#ifndef ADAPTATION
    rate_max = drate_max;
#endif

/*
*   The RTP Header...

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Ver| flow      |0|S|  content  | sequence number               |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp (seconds)           | timestamp (fraction)          |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |S|Sbit |E|Ebit |C|I|V|MBZ| FMT |         H.261 stream...       :
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

With:

Ver:  (2 bits) RTP version number. [01]
flow: (6 bits) flow number. [000000]
S:(1 bit) Synchronized bit: [1] if it is the first packet of a GOB.
                              Note that the GBSC is not encoded and 
			      must be appended before decoding.
			    [0] else.
content: (6 bits) H.261 type [011111]
sequence number: (16 bits)
timestamp: (32 bits)
S: (1 bit) [1] if beginning of GOB; [0] else.
Sbit: (3 bits) Indicates the number of bits that should be ignored in the
               first (start) data octet. (Here always 0...)
E: (1 bit) [1] if end of GOB; [0] else.
Ebit: (3 bits) Indicates the number of bits that should be ignored in the 
               last (end) data octet.
C: (1 bit) [1] if color encoded; [0] else.
I: (1 bit) [1] if it is the first packet a full intra encoded image;
           [0] else.
V: (1 bit) [1] if movement vectors are encoded; [0] else.
FMT: (3 bits) Image format:
            [000] QCIF; [001] CIF; [100] upper left corner CIF;
            [101] upper right corner CIF; [110] lower left corner CIF;
            [111] lower right corner CIF.
*
*/
    if(FEEDBACK){
	max_value = 50;
	max_inter = 20;
    }else{
	max_value = 15;
	max_inter = 2;
	FULL_INTRA = (FIRST ? TRUE : FALSE);
    }
    off_cif_number = 13 * cif_number;
    
    chaine_codee[0] = RTP_BYTE1;
    chaine_codee[1] = RTP_BYTE2_SYNC;
    chaine_codee[2] = (u_char)(sequence_number >> 8);
    chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
    sequence_number = (sequence_number + 1) % 65536;
    gettimeofday(&realtime, (struct timezone *)NULL);
    chaine_codee[4] = (u_char)((realtime.tv_sec >> 8) & 0x0000ff);
    chaine_codee[5] = (u_char)(realtime.tv_sec & 0x000000ff);
    chaine_codee[6] = (u_char)((realtime.tv_usec >> 12) & 0xff);
    chaine_codee[7] = (u_char)((realtime.tv_usec >> 4) & 0xff);
    chaine_codee[8] = 0x88; /* By default, E = S = 1 */

    timestamps[cpt%256].sec = (u_short)(realtime.tv_sec & 0xffff);
    timestamps[cpt%256].usec = (u_short)(realtime.tv_usec >> 4);

    if(COLOR){
      if(FULL_INTRA && (cif_number == 0))
	chaine_codee[9] = 0xc4 | cif_number; /* color + full intra
						+ cif number */
      else
	chaine_codee[9] = 0x84 | cif_number; /* color + cif number */
    }else{
      if(FULL_INTRA && (cif_number == 0))
	chaine_codee[9] = 0x44 | cif_number; /* cif number + full_intra */
      else
	chaine_codee[9] = 0x04 | cif_number; /* cif number */
    }
    chaine_codee[10] = 0x00;
    i = 10;
    j = 7;

    VIDEO_PACKET_HEADER=TRUE;
  

    /*************\
    * IMAGE LAYER *
    \*************/

    if(FIRST){
      /* Some initializations ... */
      offsetY = COL_MAX - 8;
      offsetC = COL_MAX_DIV2 - 8;
      inittime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      RT=0;
      bzero((char *)image_coeff, sizeof(IMAGE_CIF));
      rate_max = drate_max;
#ifdef ADAPTATION
      {
	char host[100];
	char filename[150];

	if(gethostname(host, 100) != 0){
	  fprintf(stderr, "Cannot get local hostname\n");
	  exit(1);
	}
	strcpy(filename, "rate_max.");
	strcat(filename, host);
	if((file_rate=fopen(filename, "w")) == NULL){
	  fprintf(stderr, "cannot create %s file\n", filename);
	  exit(1);
	}
	strcpy(filename, "loss.");
	strcat(filename, host);
	if((file_loss=fopen(filename, "w")) == NULL){
	  fprintf(stderr, "cannot create %s file\n", filename);
	  exit(1);
	}
      }
#endif 
      if(cif_number == 0){
	sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
	if(COLOR){
	  BperMB = 6;
	  full_cbc = 63;
	  nblock_cif_max = 2200;
        }else{
	  BperMB = 4;
	  full_cbc = 60;
	  nblock_cif_max = 1500;
        }
      }
      if(cif_number == 3)
	FIRST = FALSE;
    }else{
      if(cif_number == 0){
	synctime = realtime.tv_sec + realtime.tv_usec/1000000.0;
	RT = ((RT + (int)(33*(synctime-inittime))) & 0x1f);
#ifdef DEBUG
	fprintf(stderr, "Encoding time: %d ms\n", 
		(int)(1000*(synctime-inittime)));
#endif

	if(rate_control == QUALITY_CONTROL){

	  double rate;
	  static double mem_rate;
	  static int step=1;
	  int delta;
	  int rate_min;
	  static int cpt_min=0;

	  rate_min = (int)(0.7*rate_max); /* Hysteresis 30% */
	  rate = ((double)(0.008*length_image) / (synctime-inittime));
	  if(cpt % 2 == 1){
	    mem_rate = (mem_rate + rate) / 2.0;
	    mem_rate = 0.0;
	  }else{
	    mem_rate = rate;
	  }
	  if(rate < rate_min){
	    cpt_min ++;
	    if(cpt_min > 3){
	      delta = 1;
	      cpt_min = 0;
	      step = (step-delta >=0 ? step-delta : 0);
	    }
	  }else{
	    if(rate > rate_max){
	      delta = ((rate-rate_max)/10 + 1);
	      step = (step+delta <= step_max ? step+delta : step_max);
	    }
	    cpt_min = 0;
	  }
	  quantizer = Step[step].quant;
	  threshold = Step[step].threshold;
	}else{
	  quantizer = default_quantizer;
	  threshold = ((rate_control == REFRESH_CONTROL) ? LOW_THRESHOLD :
		       DEFAULT_THRESHOLD);
	}

	fps = (1.0 / (synctime-inittime));
	bandwidth = ((double)(0.008*length_image) / (synctime-inittime));
	bzero(data, len_data);
	sprintf(data, "%2.1f", bandwidth);
	sprintf(&data[6], "%2.1f", fps);
	if((lwrite=write(fd, data, len_data)) != len_data){
#ifdef DEBUG
	  perror("encode_h261: Cannot send bandwidth and rate data");
#endif
	}
	length_image = 0;
	inittime = synctime;
      }
      Nblock = 0;
    }
#ifdef DEBUG
    fprintf(stderr, "QUANT: %d, RT: %d\n", quantizer, RT);
#endif
    put_value(&chaine_codee[i],0,4,&i,&j); /* NG only */
    put_value(&chaine_codee[i],RT,5,&i,&j); /* RT */
    put_value(&chaine_codee[i],4,6,&i,&j); /* TYPEI = CIF */
    put_bit(chaine_codee,0,i,j); /* ISI1 */

    memset((char *)GobEncoded, TRUE, NG_MAX);

    if(FULL_INTRA == FALSE){
      if(cif_number == 0){
	lig11=LIG[n_im];
	col11=COL[n_im];
	n_im=(n_im+1)%16;
	lig12=lig11+4;
	col12=col11+4;
	lig21=lig11+8;
	lig22=lig12+8;
	col21=col11+8;
	col22=col12+8;
      }

      for(NG=1; NG<=NG_MAX; NG++){

	/* GOB LOOP */

	mbnul=0;
	gob = (GOB *) (image_coeff)[NG-1];
	force_value = (FEEDBACK ? ForceGobEncoding[off_cif_number+NG] : 0);

	for(MBA=1; MBA<34; MBA++){
	  register int sum;
	  int lig_11, lig_12, lig_21, lig_22, col_11, col_12, col_21, col_22;

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];
	  lig_11=lig+lig11;
	  lig_12=lig+lig12;
	  lig_21=lig+lig21;
	  lig_22=lig+lig22;
	  col_11=col+col11;
	  col_12=col+col12;
	  col_21=col+col21;
	  col_22=col+col22;

	  bnul=0;
	  mb = &((*gob)[MBA-1]);

	  /* We choose MB INTRA encoding when a MB has been lost
	  * or more than max_inter INTER encoding have been done or
	  * every max_value images (in order to prevent loss packets).
	  */

	  if((mb->last_encoding > force_value) &&
	     (mb->last_encoding < max_value) && (mb->cpt_inter < max_inter)){
	    mb->CBC = 0;
	    mb->MODE = INTER;
	    /* B movement detection */

	    sum =  abs((int)new_Y[lig_11][col_11] - 
		       (int)old_Y[cif_number][lig_11][col_11]);
	    sum += abs((int)new_Y[lig_11][col_12] - 
		       (int)old_Y[cif_number][lig_11][col_12]);
	    sum += abs((int)new_Y[lig_12][col_11] - 
		       (int)old_Y[cif_number][lig_12][col_11]);
	    sum += abs((int)new_Y[lig_12][col_12] - 
		       (int)old_Y[cif_number][lig_12][col_12]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 32;
	    }else
	      bnul ++;
	  
	    sum =  abs((int)new_Y[lig_11][col_21] - 
		       (int)old_Y[cif_number][lig_11][col_21]);
	    sum += abs((int)new_Y[lig_11][col_22] - 
		       (int)old_Y[cif_number][lig_11][col_22]);
	    sum += abs((int)new_Y[lig_12][col_21] - 
		       (int)old_Y[cif_number][lig_12][col_21]);
	    sum += abs((int)new_Y[lig_12][col_22] - 
		       (int)old_Y[cif_number][lig_12][col_22]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 16;
	    }else
	      bnul ++;

	    sum =  abs((int)new_Y[lig_21][col_11] - 
		       (int)old_Y[cif_number][lig_21][col_11]);
	    sum += abs((int)new_Y[lig_21][col_12] - 
		       (int)old_Y[cif_number][lig_21][col_12]);
	    sum += abs((int)new_Y[lig_22][col_11] - 
		       (int)old_Y[cif_number][lig_22][col_11]);
	    sum += abs((int)new_Y[lig_22][col_12] - 
		       (int)old_Y[cif_number][lig_22][col_12]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 8;
	    }else
	      bnul ++;

	    sum =  abs((int)new_Y[lig_21][col_21] - 
		       (int)old_Y[cif_number][lig_21][col_21]);
	    sum += abs((int)new_Y[lig_21][col_22] - 
		       (int)old_Y[cif_number][lig_21][col_22]);
	    sum += abs((int)new_Y[lig_22][col_21] - 
		       (int)old_Y[cif_number][lig_22][col_21]);
	    sum += abs((int)new_Y[lig_22][col_22] - 
		       (int)old_Y[cif_number][lig_22][col_22]);
	    if(sum>threshold){
	      Nblock ++;
	      mb->CBC |= 4;
	    }else
	      bnul ++;

	    if(COLOR){
	      if(mb->CBC){
		Nblock += 2;
		mb->CBC |= 3;
	      }else
		bnul += 2;
	    }

	    if(bnul == BperMB){
	      mbnul ++;
	      (mb->last_encoding)++;
	      if(mbnul == 33){
		GobEncoded[NG-1] = FALSE;
	      }
	    }else{
	      mb->last_encoding = 1;
	      (mb->cpt_inter) ++;
	    }
	  }else{
	    /* INTRA MB encoding */
	    mb->MODE = INTRA;
	    Nblock += BperMB;
	  }
	} /* MACROBLOCK LOOP */
      } /* GOB LOOP */
    } /* if(PARTIAL) */

    /* If there is a change of scene,
    * a full Intra image encoding is done.
    */

    if(Nblock > nblock_cif_max){
	FULL_INTRA = TRUE;
	chaine_codee[9] |= 0x40;
	Nblock = 1584;
    }

    for(NG=1; NG<=NG_MAX; NG++){

      /* GOB LOOP */

      if(FULL_INTRA || GobEncoded[NG-1]){
	mbnul=0;
	gob = (GOB *) (image_coeff)[NG-1];

	for(MBA=1; MBA<34; MBA++){

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];

	  coldiv2 = col >> 1;
	  ligdiv2 = lig >> 1;

	  mb = &((*gob)[MBA-1]);
	  if((FULL_INTRA == FALSE) && (mb->MODE == INTER)){

	    max_coeff=0;
	    if((mb->CBC) & 0x20){
	      register int l, c;
	      register int *pt_image = &diff_Y[lig][col];
	      register u_char *pt_new = &new_Y[lig][col];
	      register u_char *pt_old = &old_Y[cif_number][lig][col];
	      for(l=0; l<7; l++){
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=0; c<8; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[lig][col], mb->P[0], COL_MAX, TRUE, &cmax0);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x10){
	      register int l, c;
	      int auxc=col+8;
	      register int *pt_image = &diff_Y[lig][auxc];
	      register u_char *pt_new = &new_Y[lig][auxc];
	      register u_char *pt_old = &old_Y[cif_number][lig][auxc];
	      for(l=0; l<7; l++){
		for(c=8; c<16; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=8; c<16; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[lig][auxc], mb->P[1], COL_MAX, TRUE, 
			  &cmax1);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x08){
	      register int l, c;
	      int auxl = lig+8;
	      register int *pt_image = &diff_Y[auxl][col];
	      register u_char *pt_new = &new_Y[auxl][col];
	      register u_char *pt_old = &old_Y[cif_number][auxl][col];
	      for(l=8; l<15; l++){
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=0; c<8; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[auxl][col], mb->P[2], COL_MAX, TRUE, 
			  &cmax2);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x04){
	      register int l, c;
	      int auxl = lig+8;
	      int auxc = col+8;
	      register int *pt_image = &diff_Y[auxl][auxc];
	      register u_char *pt_new = &new_Y[auxl][auxc];
	      register u_char *pt_old = &old_Y[cif_number][auxl][auxc];
	      for(l=8; l<15; l++){
		for(c=8; c<16; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		pt_image += offsetY;
		pt_new += offsetY;
		pt_old += offsetY;
	      }
	      for(c=8; c<16; c++)
		*pt_image++ = (int)*pt_new++ - (int)*pt_old++;
	      max=(*ffct)(&diff_Y[auxl][auxc], mb->P[3], COL_MAX, TRUE, 
			  &cmax3);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if(COLOR){
	      if((mb->CBC) & 0x02){
		register int l, c;
		register int *pt_image = &diff_Cb[ligdiv2][coldiv2];
		register u_char *pt_new = &new_Cb[ligdiv2][coldiv2];
		register u_char *pt_old = 
		  &old_Cb[cif_number][ligdiv2][coldiv2];
		for(l=0; l<7; l++){
		  for(c=0; c<8; c++)
		    *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		  pt_image += offsetC;
		  pt_new += offsetC;
		  pt_old += offsetC;
		}
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		max=chroma_ffct_8x8(&diff_Cb[ligdiv2][coldiv2], mb->P[4], 
			    COL_MAX_DIV2, TRUE, &cmax4);
		if(max>max_coeff)
		  max_coeff=max;
	      } 
	      if((mb->CBC) & 0x01){
		register int l, c;
		register int *pt_image = &diff_Cr[ligdiv2][coldiv2];
		register u_char *pt_new = &new_Cr[ligdiv2][coldiv2];
		register u_char *pt_old = 
		  &old_Cr[cif_number][ligdiv2][coldiv2];
		for(l=0; l<7; l++){
		  for(c=0; c<8; c++)
		    *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		  pt_image += offsetC;
		  pt_new += offsetC;
		  pt_old += offsetC;
		}
		for(c=0; c<8; c++)
		  *pt_image++ = (int)*pt_new++ - (int)*pt_old++;
		max=chroma_ffct_8x8(&diff_Cr[ligdiv2][coldiv2], mb->P[5], 
			    COL_MAX_DIV2, TRUE, &cmax5);
		if(max>max_coeff)
		  max_coeff=max;
	      }
	    }
	  }else{
	    /* INTRA encoding mode */
	    mb->MODE = INTRA;
	    mb->CBC = full_cbc;
	    mb->last_encoding = 1;
	    mb->cpt_inter = 0;
	    max_coeff = 0; 
	    max=(*ffct_intra)(&new_Y[lig][col], mb->P[0], COL_MAX, &cmax0);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig][col+8], mb->P[1], COL_MAX, &cmax1);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig+8][col], mb->P[2], COL_MAX, &cmax2);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=(*ffct_intra)(&new_Y[lig+8][col+8], mb->P[3], COL_MAX, &cmax3);
	    if(max>max_coeff)
	      max_coeff=max;
	    if(COLOR){
	      max=chroma_ffct_intra_8x8(&new_Cb[ligdiv2][coldiv2], mb->P[4], 
				COL_MAX_DIV2, &cmax4);
	      if(max>max_coeff)
		max_coeff=max;
	      max=chroma_ffct_intra_8x8(&new_Cr[ligdiv2][coldiv2], mb->P[5], 
				COL_MAX_DIV2, &cmax5);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	  }
	  if(mb->CBC){
	    register int n, l, c;

	    quant_mb = (max_coeff >> 8) + 1;
	    if(quantizer>quant_mb)
	      quant_mb = (FULL_INTRA ? 5 : quantizer);
	    mb->QUANT = quant_mb;
	    /*
	     *  We have to quantize mb->P[n].
	     */
	    bnul=0;
	    if(quant_mb & 0x01){
	      /* odd quantizer */
	      for(n=0; n<BperMB; n++){
		register int *pt_P = &(mb->P[n][0][0]);
		register int nb_coeff=0;

		if((mb->CBC) & MASK_CBC[n]){
		  cc = *pt_P;
		  for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=(*pt_P / quant_mb - 1)/2))
			  nb_coeff ++;
		      }else{
			if(*pt_P < 0)
			  if((*pt_P=(*pt_P /quant_mb + 1)/2))
			    nb_coeff ++;
		      }
		      pt_P ++;
		    }
		  if(mb->MODE == INTRA){
		    if((mb->P[n][0][0]==0) && cc)
		      nb_coeff ++;
		    mb->P[n][0][0] = cc;
		  }
		  if(!nb_coeff){
		    bnul ++;
		    (mb->CBC) &= (~MASK_CBC[n]);
		  }
		}else
		  bnul ++;
		mb->nb_coeff[n] = nb_coeff;
	      }
	    }else{
	      /* even quantizer */
	      for(n=0; n<BperMB; n++){
		register int *pt_P = &(mb->P[n][0][0]);
		register int nb_coeff=0;

		if((mb->CBC) & MASK_CBC[n]){
		  cc = *pt_P;
		  for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=((*pt_P + 1)/quant_mb - 1)/2))
			  nb_coeff ++;
		      }else{
			if(*pt_P < 0)
			  if((*pt_P=((*pt_P - 1)/quant_mb - 1)/2))
			    nb_coeff ++;
		      }
		      pt_P ++;
		    }
		  if (mb->MODE == INTRA) {
		      if((!mb->P[n][0][0]) && cc)
			  nb_coeff ++;
		      mb->P[n][0][0] = cc;
		  }
		  if(! nb_coeff){
		      bnul ++;
		      (mb->CBC) &= (~MASK_CBC[n]);
		  }
	      }else
		  bnul ++;
		mb->nb_coeff[n] = nb_coeff;
	      }		      
	    }
	    if(bnul == BperMB){
	      mbnul ++;
	      if(mbnul == 33){
		GobEncoded[NG-1]=FALSE;
	      }
	    }
	  }
	  if(bnul != BperMB){
	    /************\
	    * IFCT of MB *
	    \************/
	    int inter = mb->MODE;
	    tab_quant1 = tab_rec1[quant_mb-1];
	    tab_quant2 = tab_rec2[quant_mb-1];
	    
	    if((mb->CBC) & 0x20)
	      ifct_8x8(mb->P[0], tab_quant1, tab_quant2, inter,
		       &(old_Y[cif_number][lig][col]), cmax0, COL_MAX);
	  
	    if((mb->CBC) & 0x10)
	      ifct_8x8(mb->P[1], tab_quant1, tab_quant2, inter, 
		       &(old_Y[cif_number][lig][col+8]), cmax1, COL_MAX);
	  
	    if((mb->CBC) & 0x08)
	      ifct_8x8(mb->P[2], tab_quant1, tab_quant2, inter,
		       &(old_Y[cif_number][lig+8][col]), cmax2, COL_MAX);

	    if((mb->CBC) & 0x04)
	      ifct_8x8(mb->P[3], tab_quant1, tab_quant2, inter,
		       &(old_Y[cif_number][lig+8][col+8]), cmax3, COL_MAX);
	    if(COLOR){
	      if((mb->CBC) & 0x02)
		ifct_8x8(mb->P[4], tab_quant1, tab_quant2, inter,
			 &(old_Cb[cif_number][ligdiv2][coldiv2]), cmax4, 
			 COL_MAX_DIV2);
	      if((mb->CBC) & 0x01)
		ifct_8x8(mb->P[5], tab_quant1, tab_quant2, inter,
			 &(old_Cr[cif_number][ligdiv2][coldiv2]), cmax5, 
			 COL_MAX_DIV2);
	    }
	  }
	} /* MACROBLOCK LOOP */
      }
      {
	int last_AM, quant=quantizer;
	register int NM, b;

	if(!VIDEO_PACKET_HEADER){
	  chaine_codee[2] = (u_char)(sequence_number >> 8);
	  chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
	  sequence_number = (sequence_number + 1) % 65536;
	  if(COLOR){
	    chaine_codee[9] = 0x84 | cif_number; /* color + cif number */ 
	  }else{
	    chaine_codee[9] = 0x04 | cif_number; /* cif number */
	  }
	  chaine_codee[10] = 0x00;
	  i = 10;
	  j = 7;
	  VIDEO_PACKET_HEADER = TRUE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}

	/*********************\
	* H.261 CODING OF GOB *
	\*********************/

	if(FIRST_GOB_IS_ENCODED){
	  put_value(&chaine_codee[i],1,16,&i,&j); 
	}else
	  FIRST_GOB_IS_ENCODED = TRUE;

	/* The GBSC not be encoded (S bit used) */
	put_value(&chaine_codee[i],NG,4,&i,&j); 
	put_value(&chaine_codee[i],quant,5,&i,&j);
	put_bit(chaine_codee,0,i,j); /* ISG */

	if(GobEncoded[NG-1]){

	  /******************\
          * MACROBLOCK LAYER *
	  \******************/

	  last_AM = 0;
	  for(NM=1; NM<34; NM++){
	      BOOLEAN CBC_IS_CODED;
	    
	    mb = &((*gob)[NM-1]);
	    if(!(mb->CBC)) /* If MB is entirely null, continue ... */
	      continue;
	    if(!chain_forecast(tab_mba,n_mba,(NM-last_AM),chaine_codee,&i,&j)){
	      fprintf(stderr,"Bad MBA value: %d\n", NM-last_AM);
	      exit(-1);
	    }
	    CBC_IS_CODED = (mb->MODE ==INTER);    
	    if(mb->QUANT != quant){
	      quant=mb->QUANT;
	      if(CBC_IS_CODED)
		/*TYPEM=INTRA+CBC+QUANTM */
		put_value(&chaine_codee[i],1,5,&i,&j);
	      else
		/* TYPEM = INTRA+QUANTM */
		put_value(&chaine_codee[i],1,7,&i,&j);
	      /* Coding of QUANTM */
	      put_value(&chaine_codee[i],quant,5,&i,&j); 
	    }
	    else{
	      if(CBC_IS_CODED)
		/* TYPEM = INTRA+CBC */
		put_bit(chaine_codee,1,i,j); 
	      else
		/* TYPEM = INTRA */
		put_value(&chaine_codee[i],1,4,&i,&j); 
	    }
	    if(CBC_IS_CODED){
	      if(!chain_forecast(tab_cbc, n_cbc, (int)mb->CBC, chaine_codee,
				 &i, &j)){
		fprintf(stderr,"Bad CBC value: %d\n", mb->CBC);
		exit(-1);
	      }
	    }
	    /*************\
	    * BLOCK LAYER *
	    \*************/
	    for(b=0; b<BperMB; b++)
	      if(mb->CBC & MASK_CBC[b])
		code_bloc_8x8(mb->P[b], chaine_codee, tab_tcoeff, 
			      n_tcoeff, mb->nb_coeff[b], &i, &j, mb->MODE);
	    last_AM = NM;
	  } 
	}else{
	    ; /* This GOB is unchanged */
	}

	/* If there is enough data or it is the last GOB,
	* write data on socket.
	*/

	if((i > 500) || (NG == NG_MAX)){
	  int Ebit;
	  register int k; 

	  /* No padding is required when using Ebit... */

	  Ebit = (j + 1) % 8;
	  chaine_codee[8] = 0x88 | Ebit;

	  if(Ebit != 0){
	    /* We have to nullify the following bits */
	    for(k=0; k<Ebit; k++)
	      put_bit(chaine_codee,0,i,j);
	  }
	  if(rate_control == REFRESH_CONTROL){
	    /* Control rate : Privilege quality rather than refreshment.
	    *  Add if necessary a delay between each packet sent.
	    */
	    static BOOLEAN FIRST=TRUE;
	    static double lasttime;
	    struct timeval realtime;
	    double newtime, deltat;

	    if(FIRST){
	      FIRST=FALSE;
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      lasttime = 1000000.0*realtime.tv_sec + realtime.tv_usec;
	    }else{
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      newtime = 1000000.0*realtime.tv_sec + realtime.tv_usec;
	      deltat = 8000.0*i/rate_max - (newtime - lasttime);
	      if(deltat > 0){
		coffee_break((long)deltat);
		lasttime = newtime + deltat;
	      }else
		lasttime = newtime;
	    } 
	  }
  
	  /* Sending packet */
	  if(i < MAX_BSIZE){
	    if((lwrite=sendto(sock, (char *)chaine_codee, i, 0, 
			      (struct sockaddr *) &addr, len_addr)) < 0){
	      perror("sendto: Cannot send data on socket");
	      exit(-1);
	    }
	    length_image += lwrite;
	  }else{
	    int rest, nsend, aux, Ebit;

	    /* E=0, Ebit=0 and S=1 */
	    chaine_codee[8] = 0x80;
	    rest = i;
	    if((lwrite=sendto(sock, (char *)chaine_codee, MAX_BSIZE, 0, 
			      (struct sockaddr *) &addr, len_addr)) < 0){
	      perror("sendto: Cannot send data on socket");
	      exit(1);
	    }
	    chaine_codee[8] = 0x00;
	    nsend = lwrite;
	    rest -= nsend;
	    while(rest != 0){
	      aux = MAX_BSIZE - 10;
	      bcopy((char *)&chaine_codee[nsend], (char *)&chaine_codee[10],
		    min(rest, aux));
	      aux = rest + 10;
	      if(aux <= MAX_BSIZE){
		/* E=1, Ebit */
		Ebit=(j + 1) % 8;
		chaine_codee[8] = 8 | Ebit;
	      }
	      chaine_codee[2] = (u_char)(sequence_number >> 8);
	      chaine_codee[3] = (u_char)(sequence_number & 0x00ff);
	      sequence_number = (sequence_number + 1) % 65536;
	      if((lwrite=sendto(sock, (char *)chaine_codee, 
				min(aux, MAX_BSIZE), 0, 
				(struct sockaddr *) &addr, len_addr)) < 0){
		perror("sendto: Cannot send data on socket");
		exit(1);
	      }
	      aux = lwrite - 10;
	      nsend += aux;
	      rest -= aux;
	    }
	    length_image += i;
	  }
	  VIDEO_PACKET_HEADER = FALSE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}
      } /* gob encoding */
    } /* GOB LOOP */
    if(cif_number == 3){
      FULL_INTRA = FALSE;
      if(FEEDBACK){
	if(CheckControlPacket(sock_recv, cpt, timestamps, ForceGobEncoding, 
			      1, &FULL_INTRA, TRUE)){
#ifdef ADAPTATION
	  rate_max = maxi(rate_max/2, 10);
	  fprintf(file_loss, "%d\t10\n", cpt);
	  fflush(file_loss);
#endif	  
	}else{
#ifdef ADAPTATION
	  rate_max = min(rate_max+5, drate_max);
	  fprintf(file_loss, "%d\t0\n", cpt);
	  fflush(file_loss);
#endif	  
	}
#ifdef ADAPTATION
	fprintf(file_rate, "%d\t%d\n", cpt, rate_max);
	fflush(file_rate);
#endif
      }/* FEEDBACK allowed */
    } /* CIF number == 3 */
    cpt ++;
  }




int chain_forecast(tab_code, n_code, result, chain, i, j)
    /*
    *  Look for associated code to value "result" in the sorted array 
    *  "tab_code". If this code exists then :
    *    - append it to chain (big endian).
    *    - return 1
    *  Else
    *    - return 0.
    */

    mot_code *tab_code;
    int n_code;
    int result;
    unsigned char *chain;
    int *i; /* current byte */
    int *j; /* next bit to code */

  {
    register int d, g, m, aux;
    register int ii = *i, jj = *j;
    u_char *chain_ptr;

    d = n_code - 1;
    g = 0;
    m = (g+d) >> 3;
    chain_ptr = &chain[ii];
    do{
      aux = tab_code[m].result;
      if(result < aux)
	  d = m - 1;
      else if(result > aux)
	  g = m + 1;
      else {
	  register int bit;
	  register char zero='0';
	  register u_char *tab_ptr;
	  tab_ptr = (u_char *) tab_code[m].chain;
	  while((bit = *tab_ptr++)){
	      put_bit_ptr(chain_ptr,(bit ^ zero),ii,jj);
	  }
	  *i=ii; *j=jj;
	  return(1);
      }
      m = (g+d) >> 1;            
    }while(g <= d);
    return(0);
  }


code_bloc_8x8(bloc, chaine_codee, tab_code, n_code, nb_coeff, i, j, inter)
 
    /*
    *  H.261 encoding of a 8x8 block of DCT coefficients.
    */

    int bloc[][8];
    unsigned char *chaine_codee;
    mot_code *tab_code;
    int n_code, nb_coeff;
    int *i, *j;
    int inter; /* Boolean: True if INTER mode*/
      
  {
    int run = 0; 
    register int level; 
    register int val, a;
    int plus, N=0;
    int first_coeff = 1; /* Boolean */
    int ii = *i, jj = *j;
    static unsigned char *zig_ptr, *zag_ptr;
    
    zig_ptr = &ZIG[0];
    zag_ptr = &ZAG[0];
    
    for(a=0; a<64; a++){
      if(N==nb_coeff)
	  break;
      if(!(val=bloc[*zig_ptr++][*zag_ptr++])){
	run ++;
	continue;
      }else
	  N ++;
      if(val>0){
	level = val;
	plus = 1;
      }else{
	level = -val;
	plus = 0;
      }
      val = (run<<4) + level; /* run is encoded with 5 bits (to the left)
			      *  and level with 4 bits (to the right)
			      */
      if(first_coeff){
	first_coeff = 0;
	if(inter){
	  if((level == 1)&&(!run)){
	    /* Peculiar case : First coeff is 1 */
	    put_bit(chaine_codee,1,ii,jj);
	    if(plus)
	      put_bit(chaine_codee,0,ii,jj);
	    else
	      put_bit(chaine_codee,1,ii,jj);
	    continue; 
	  }
	}else{
	  if(a != 0)
	    fprintf(stderr, "CC INTRA = 0 !!!\n");
	  /* INTRA mode and CC --> encoding with an uniform quantizer */
	  if(level == 128)
	      level = 255;
	  put_value(&chaine_codee[ii], level, 8, &ii, &jj);
	  continue;
	}
      }
      if(level<=15) {
	  if(chain_forecast(tab_code,n_code,val,chaine_codee,&ii,&jj)){
	    /* Sign encoding */
	    if(plus)
		put_bit(chaine_codee,0,ii,jj);
	    else
		put_bit(chaine_codee,1,ii,jj);
	  run = 0;
	  continue;
	  }
      }
      /*
      *  This code is not a Variable Length Code, so we have to encode it 
      *  with 20 bits.
      *  - 6 bits for ESCAPE (000001)
      *  - 6 bits for RUN
      *  - 8 bits for LEVEL
      */

      put_value(&chaine_codee[ii], 1, 6, &ii, &jj);
      put_value(&chaine_codee[ii], run, 6, &ii, &jj);

      if(plus)
	  put_value(&chaine_codee[ii],level, 8, &ii, &jj);
      else{
	  put_bit(chaine_codee,1,ii,jj);
	  level = 128 - level;
	  put_value(&chaine_codee[ii],level, 7, &ii, &jj);

      }
      run = 0;
    }
    /* EOB encoding (10) */
    put_value(&chaine_codee[ii], 2, 2, &ii, &jj);
    *i=ii; *j=jj;
  }
	
	
/* This routine sets bits in the chain eight at a time */
static int  put_value(chain_ptr, value, n_bits, i, j) 
     unsigned char *chain_ptr;
     int value;
     int n_bits;
     int *i, *j;
{
  while ( *j <= (n_bits - 1)){
    *chain_ptr++ |= ( value >> ((n_bits - 1 ) - *j));
    n_bits -= (*j + 1);
    *chain_ptr = 0;
    *j = 7;
    *i += 1;
  }
  *chain_ptr |=(value << ( *j - (n_bits - 1)));
  *j -= n_bits;
}
