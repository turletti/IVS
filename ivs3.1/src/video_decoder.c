/**************************************************************************\
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
*  File              : video_decoder.c                                     *
*  Author            : Thierry Turletti                                    *
*  Last modification : 93/4/24                                             *
*--------------------------------------------------------------------------*
*  Description :  H.261 video decoder.                                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Pierre Delamotte    |  93/3/20  | Color display adding.                *
\**************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "h261.h"
#include "protocol.h"
#include "huffman.h"

#define ESCAPE        0
#define EOB          -1
#define L_Y         352
#define L_Y_DIV2    176
#define T_READ_MAX 4096

#define PADDING       0 /* Padding decoding (11 bits) */
#define CDI          -1 /* CDI decoding */
#define MORE0        -2 /* More than 15 zeroes */
#define Y_CCIR_MAX	240
#define MODCOLOR	4
#define MODGREY		2
#define MODNB		1

#define NMemoPacket   4

#define get_bit(octet,i,j) \
    (j==7 ? (j=0, ((i)+1)<lmax ? octet[(i)++]&0x01 : (vali=octet[i], \
			 lmax=ReadBuffer(sock, octet, &i, &j), vali&0x01)) \
          : (octet[(i)]>>(7-(j)++))&0x01)

#define equal_time(s1,us1,s2,us2) \
    ((s1) == (s2) ? ((us1) == (us2) ? 1 : 0) : 0)


#define old_packet(recvd, expctd) \
    ((expctd - recvd + 65536) % 65536 < 32768 ? TRUE : FALSE)

/*------------------*\
* Imported variables *
\*------------------*/

/*
**  From display.c
*/

extern BOOLEAN	        CONTEXTEXIST ;
extern BOOLEAN		encCOLOR ;
extern u_char		map_lut[] ;
extern u_char		affCOLOR ;
extern int              tab_rec1[][255], tab_rec2[][255];

/*----------------*\
* Global variables *
\*----------------*/

int sock, lmax, vali, fd_pipe;
char host_coder[50];
u_long host_sin_addr;

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


static XtAppContext	appCtxt;
static Widget		appShell;
static Display 		*dpy;
static XImage 		*ximage;
static GC 		gc;
static Window 		win;
static XColor 		colors[256];
static int 		depth=0;
static int 		N_LUTH=256; 

static BOOLEAN COLOR_ENCODED;
static BOOLEAN SUPER_CIF;
static BOOLEAN FULL_INTRA_REQUEST;
static BOOLEAN FULL_INTRA; /* True if Full INTRA encoded packet */
static BOOLEAN PACKET_LOST=FALSE; /* loss detected */
static BOOLEAN AFTER_GBSC=FALSE; /* True while resynchronization is ok */
static BOOLEAN TRYING_RESYNC=FALSE;
static int cif_number;
static int format, old_format; /* Image format:
	    [000] QCIF; [001] CIF; [100] upper left corner CIF;
            [101] upper right corner CIF; [110] lower left corner CIF;
            [111] lower right corner CIF. */
static u_short sequence_number;
static int NG; /* Current GOB number */
static int NG_MAX; /* Maximum NG value */
static int L_lig, L_col;
static int L_lig2, L_col2;
static u_short old_ustime, old_stime;
static u_short ustime, sec_stime;
static u_char old_ustime_up, old_ustime_down;
static u_char old_stime_up, old_stime_down;
static u_char ustime_up, ustime_down;
static u_char stime_up, stime_down;
static u_short feedback_port; 
static BOOLEAN FEEDBACK;
static BOOLEAN MV_ENCODED;
static BOOLEAN STAT_MODE;
static FILE *F_dloss;


#ifdef INPUT_FILE
static char filename[100]; /* Input H261 file name */
#endif



int decode_bloc_8x8(chaine_codee, bloc, state, i, j, colmax, inter)
    unsigned char *chaine_codee;
    int bloc [][8];
    etat *state;
    int *i, *j;
    int *colmax; /* bloc[][colmax+1] = 0 */
    BOOLEAN inter; /* True if INTER mode */

  {
    int x, new_x; /* previous and new state */
    int run, level, result;
    int temp, new_bit, negatif;
    int col, maxcol=0;
    register int l, k;
    int ii, jj;
    static BOOLEAN FIRST=TRUE;
    static int block_size;

    if(FIRST){
      block_size = sizeof(int) << 6;
      FIRST = FALSE;
    }
    k = 0; /* Next coefficient block. */
    ii = *i;
    jj = *j;
    bzero((char *)bloc, block_size);
    if(inter){
      if((chaine_codee[ii]>>(7 - jj))&0x01){
	/* Special case: First coeff starts with 1
	*  So, run=0 and abs(level)=1.
	*/
        temp = get_bit(chaine_codee,ii,jj);
	if((col=ZAG[k]) > maxcol)
	    maxcol=col;
        if(get_bit(chaine_codee,ii,jj)){
	  /* level = -1 */
  	  bloc[ZIG[k]][col] = -1;
	  k ++;
        }else{
	  /* level = +1 */
	  bloc[ZIG[k]][col] = 1;
	  k ++;
        }
      }
    }else{ /* INTRA mode and CC */
      level = get_bit(chaine_codee,ii,jj);
      for(l=1; l<8; l++)
	  level = (level<<1) + get_bit(chaine_codee,ii,jj);
      level = (level==255 ? 128 : level);
      if((col=ZAG[k]) > maxcol)
	  maxcol=col;
      bloc[ZIG[k]][col] = level;
      k ++;
    }
    do{
      new_x = 0;
      do{
	x = new_x;
	new_bit = get_bit(chaine_codee,ii,jj);
	new_x = state[x].bit[new_bit];
      }while(new_x > 0);
      if((result=state[x].result[new_bit]) > 0){
	/* It is a Variable Length Code. */
	run = result >> 4;
	level = result & 0x0f;
	k += run;
	if((k > 63) || (new_x < 0)){
	  /* Bit error detected */
#ifdef DEBUG
	  fprintf(stderr, "decode_bloc_8x8: Bit error detected\n");
#endif	  
	  return(-1);
	}
	if((col=ZAG[k]) > maxcol)
	    maxcol=col;
	/* Level's sign. */
	if(get_bit(chaine_codee,ii,jj))
	    bloc[ZIG[k]][col] = -level;
	else
	    bloc[ZIG[k]][col] = level;
	k ++;
	continue;
      }else{
	if(result == ESCAPE){
	  /* It is a fixed length code -> 20 bits. */
	  /* 6 bits(ESCAPE) + 6 bits(RUN) + 8 bits(LEVEL) */
	  run = 0;
	  level = 0;
	  for(l=0; l<6; l++)
	    run = (run << 1) + get_bit(chaine_codee,ii,jj);
	  negatif = get_bit(chaine_codee,ii,jj);
	  for(l=0; l<7; l++)
	    level = (level << 1) + get_bit(chaine_codee,ii,jj);
	  if(negatif)
	    level -= 128;
	  k += run;
	  if((col=ZAG[k]) > maxcol)
	    maxcol=col;
	  bloc[ZIG[k]][col] = level;
	  k ++;
	  continue;
	}else{
	  if(result == EOB){
	    break;
	  }else{
	    /* Bit error detected */
#ifdef DEBUG
	    fprintf(stderr, "decode_bloc_8x8: Bit error detected!!!\n");
#endif
	    return(-1);
	  }
	}
      }
    }while(1);
    *colmax=maxcol;
    *i = ii;
    *j = jj;
    return(1);
  }



void filter_loop(block_in, block_out, L_col)
    u_char *block_in, *block_out;
    int L_col;
  {
    u_char memo[64];
    int offset=L_col-8;

    /* Sum on raws */
      {
	register int j, k;
	register u_char *r, *b1, *b2, *b3;

	bcopy((char *)block_in, (char *)memo, 8);
	b1 = block_in;
	b2 = b1 + L_col;
	b3 = b2 + L_col;
	r = &(memo[8]);
	for(k=0; k<6; k++){
	  for(j=0; j<8; j++)
	      *r++ = ((*b1++) + ((*b2++)<<1) + (*b3++)) >> 2;
	  b1 += offset;
	  b2 += offset;
	  b3 += offset;
	}
	bcopy((char *)b2, (char *)&(memo[56]), 8);
      }
    
    /* Sum on columns */
      {
	register int l;
	register u_char *r, *b1;

	r = block_out;
	b1 = memo;
	for(l=0; l<8; l++){
	  register a, b, c;

	  *r++ = a = *b1++;
	  a += (b = *b1++);
	  b += (c = *b1++);
	  *r++ = (a+b)>>2;
	  c += (a = *b1++);
	  *r++ = (b+c)>>2;
	  a += (b = *b1++);
	  *r++ = (c+a)>>2;
	  b += (c = *b1++);
	  *r++ = (a+b)>>2;
	  c += (a = *b1++);
	  *r++ = (b+c)>>2;
	  a += (b = *b1++);
	  *r++ = (c+a)>>2;
	  *r++ = b;
	  r += offset;
	}
      }
  }



BOOLEAN PacketReceived(sock)
     int sock;
{
  static struct timeval tim_out;
  static int mask;
  BOOLEAN FIRST=TRUE;

  if(FIRST){
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 10000; /* 10 ms */
    mask = 1 << sock;
    FIRST = FALSE;
  }

  return(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out));
}



#ifdef INPUT_FILE

int ReadBuffer(sock, buf, i, j)
    int sock;
    u_char *buf;
    int *i, *j;
{
    static BOOLEAN FIRST=TRUE;
    static int ltot;
    static FILE *file;
    int lr=0;

    if(FIRST){
      if((file = fopen(filename, "r")) == NULL){
	perror("fopen Miss_America file");
	exit(1);
      }
      MV_ENCODED = TRUE;
      FEEDBACK = FALSE;
      COLOR_ENCODED = TRUE;
      SUPER_CIF = FALSE;
      ltot = 0;
      encCOLOR = TRUE;
      FIRST = FALSE;
    }

    while (CONTEXTEXIST && XtAppPending (appCtxt))
      XtAppProcessEvent (appCtxt, XtIMAll);	

    if((lr = fread((char *)buf, 1, 400, file)) <= 0){
      printf("End of file, %d bytes decoded\n", ltot);
      exit(0);
    }
    ltot += lr;
    *i = 0;
    *j = 0;
    return(lr);
}

#else

int ReadBuffer(sock, buf, i, j)
    int sock;
    u_char *buf;
    int *i, *j;

  {
    static struct sockaddr_in addr_from;
    static int fromlen, lr;
    static int mask0, mask_pipe, fmax;
    static u_short packet_expected;
    static u_char SMASK[8]={0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02};
    static BOOLEAN FIRST=TRUE;
    static int nb_requests=0;
    static BOOLEAN BUF_SAVED=FALSE, E=FALSE;
    static u_char memo_buf[T_MAX];
    static pckt_tot=0; /* STAT: received packets */
    static pckt_lost=0; /* STAT: lost packets */
    static pckt_aos=0; /* STAT: out of sequence packets */
    static int nb_memo_packet=0; /* Saved buffers number for resequencement */
    static int memo_packet_seq[NMemoPacket]; /* sequence number */
    static int memo_packet_lr[NMemoPacket]; /* Packet length */
    static u_char memo_packet[NMemoPacket][T_MAX]; /* Saved buffers */
    
    int lpipe, mask, aux, Ebit, Sbit;
    u_char command[2000], temp;
    BOOLEAN S, SYNC;

    if(FIRST){
      register int l;

      mask_pipe = 1 << fd_pipe;
      mask0 = mask_pipe | (1 << sock);
      fmax=(fd_pipe>sock ? fd_pipe+1 : sock+1);
      fromlen = sizeof(addr_from);
      for(l=0; l<NMemoPacket; l++)
	memo_packet_lr[l] = 0;
    }

#ifndef RECV_ONLY
    if(depth && E && (NG==NG_MAX)){
      /* Display the image without waiting for the next packet */
      if(SUPER_CIF){
	if(cif_number == 3)
	  show(dpy, ximage, L_col2, L_lig2);
      }else
	show(dpy, ximage, ximage->width, ximage->height);
    }
#endif

    if(PACKET_LOST && FEEDBACK){
      /* Packet loss without NG missing --> all the image is lost.
      *  Here, sec_stime == old_stime.
      */
      if(TRYING_RESYNC)
	SendNACK(host_coder, feedback_port, TRUE, NG, NG, format,
		 stime_up, stime_down, ustime_up, ustime_down);
      else
	SendNACK(host_coder, feedback_port, TRUE, 1, NG_MAX, format,
		 stime_up, stime_down, ustime_up, ustime_down);
    }
    PACKET_LOST = FALSE;
    do{
/*
**  Manage X events
*/
      while (CONTEXTEXIST && XtAppPending (appCtxt))
        XtAppProcessEvent (appCtxt, XtIMAll);	

      mask=mask0;
      if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		(struct timeval *)0) <= 0){
	perror("VideoDecode'ReadBuffer'select");
	exit(1);
      }
      if(mask & mask_pipe){
	if((lpipe=read(fd_pipe, (char *)command, 2000)) < 0){
	  exit(1);
	}else{
	  int l=0;
	  register int n;

	  do{
	    switch(command[l]){
	    case PIP_QUIT:
	      if(STAT_MODE){
		if(pckt_tot==0)
		  pckt_tot = 1;
		fprintf(stderr, "Packets lost %d/%d, %2.2f %%\n",
			pckt_lost, pckt_tot, 100.0*pckt_lost/(double)pckt_tot);
		fprintf(stderr, "Packets out of sequence %d/%d, %2.2f %%\n",
			pckt_aos, pckt_tot, 100.0*pckt_aos/(double)pckt_tot);
		fclose(F_dloss);
	      }
	      exit(1);
	    case PIP_TUNING:
	      l++;
	      for(n=0; n<256; n++)
		map_lut[n] = (u_char) colors[command[l+n]].pixel;
	      l += n;
	      break;
	    case PIP_FEEDBACK:
	      feedback_port = (u_short)((command[l+1] << 8) + command[l+2]);
	      FEEDBACK = (feedback_port ? TRUE : FALSE);
	      l += 3;
	      break;
	    default:
	      fprintf(stderr, 
	      "video_decode'ReadBuffer: Received an unknown PIP command %d\n",
		      command[l]);
	      l ++;
	      break;
	    }
	  }while(l < lpipe);
        }
      } /* Pipe reading */
      if(TRYING_RESYNC && (AFTER_GBSC==FALSE)){
#ifdef DEBUG
	fprintf(stderr, "****Resynchronization failed, re-padding..\n");
#endif
	bzero((char *)buf, 10);
	buf[9] = 0x01;
	*i = 0;
	*j = 0;
	TRYING_RESYNC = TRUE;
	return(10);
      }
      if(BUF_SAVED){
	/* After GBSC and a buffer is already saved */
#ifdef DEBUG
	fprintf(stderr, "%d bytes are restored\n", lr);
#endif
	bcopy((char *)memo_buf, (char *)buf, lr);
	BUF_SAVED = FALSE;
	break;
      }else{
	if(nb_memo_packet == 0){
	  if((lr=recvfrom(sock, (char *)buf, T_MAX, 0, 
			  (struct sockaddr *)&addr_from, &fromlen)) < 0){
	    perror("VideoDecode'ReadBuffer'recvfrom");
	    continue;
	  }
	  if(addr_from.sin_addr.s_addr != host_sin_addr){
	    continue;
	  }else{
	    pckt_tot ++;
	    sequence_number = (buf[2] << 8) | buf[3];
	  }
	}else{
	  register int k;
	  BOOLEAN PACKET_FOUND=FALSE;

	  for(k=0; k<NMemoPacket; k++){
	    if(memo_packet_lr[k] && (packet_expected == memo_packet_seq[k])){
	      nb_memo_packet --;
	      fprintf(stderr, "#releasing #%d packet, nb_memo=%d\n", 
		      packet_expected, nb_memo_packet);
	      lr = memo_packet_lr[k];
	      bcopy((char *)&memo_packet[k][0], (char *)buf, lr);
	      memo_packet_lr[k] = 0;
	      PACKET_FOUND = TRUE;
	      break;
	    }
	  }
	  if(!PACKET_FOUND){
	    if((lr=recvfrom(sock, (char *)buf, T_MAX, 0, 
			    (struct sockaddr *)&addr_from, &fromlen)) < 0){
	      perror("VideoDecode'ReadBuffer'recvfrom");
	      continue;
	    }
	    if(addr_from.sin_addr.s_addr != host_sin_addr){
	      continue;
	    }else{
	      pckt_tot ++;
	      sequence_number = (buf[2] << 8) | buf[3];
	    }
	  }
	}
      }

      if((temp = buf[0] >> 6) != RTP_VERSION){
	fprintf(stderr, "Bad RTP version %d\n", temp);
	continue;
      }
      if((temp = buf[1] & 0x3f) != RTP_H261_CONTENT){
	fprintf(stderr, "Bad RTP content type %d\n", temp);
	continue;
      }
      SYNC = (buf[1] & 0x20 ? TRUE : FALSE);
      sequence_number = (buf[2] << 8) | buf[3];

      if(FIRST){
	encCOLOR = COLOR_ENCODED = ((buf[9] & 0x80) ? TRUE : FALSE);
       	SUPER_CIF = ((buf[9] & 0x04) ? TRUE : FALSE);
	if(FULL_INTRA_REQUEST == FALSE){
	  TRYING_RESYNC= TRUE;
	  packet_expected = sequence_number;
	}
      }else{
	old_ustime = ustime;
	old_stime = sec_stime;
	old_ustime_up = ustime_up;
	old_ustime_down = ustime_down;
	old_stime_up = stime_up;
	old_stime_down = stime_down;
      }
      FULL_INTRA = ((buf[9] & 0x40) ? TRUE : FALSE);
      if(FULL_INTRA_REQUEST){
	if(!FULL_INTRA){
	  if(nb_requests % 10 == 0)
	    SendFIR(host_coder, feedback_port);
	  nb_requests ++;
	  continue;
	}else{
	  FULL_INTRA_REQUEST = FALSE;
	  packet_expected = (sequence_number + 1) % 65536;
	  break;
	}
      }else{
	if(FIRST){
	  /* The first packet to decode must have Sbit=1 */
	  if((buf[8] & 0x80) == 0){
	    packet_expected = (sequence_number + 1) % 65536;
	    continue;
	  }
	}
	if(packet_expected == sequence_number){
	  packet_expected = (sequence_number + 1) % 65536;
	  break;
	}else{
	  register int l;

	  if(old_packet(sequence_number, packet_expected)){
#ifdef DEBUG
	    fprintf(stderr, "old packet received %d, expected %d\n",
		    sequence_number, packet_expected);
#endif
	    if(STAT_MODE){
	      pckt_aos ++;
	      fprintf(F_dloss, "old packet received %d, expected %d\n",
		      sequence_number, packet_expected);
	      fflush(F_dloss);
	    }
	    continue;
	  }
#ifdef DEBUG
	  fprintf(stderr, "LOSS: packet expected: %d, packet received : %d\n", 
		  packet_expected, sequence_number);
#endif

	  for(l=0; l<NMemoPacket; l++){
	    if(memo_packet_lr[l] == 0)
	      break;
	  }
	  
	  nb_memo_packet ++;
	  memo_packet_seq[l] = sequence_number;
	  memo_packet_lr[l] = lr;
	  bcopy((char *)buf, (char *)&memo_packet[l][0], lr);
	  fprintf(stderr, "saving #%d packet, nb_memo=%d\n", sequence_number,
		  nb_memo_packet);
	  
	  if((nb_memo_packet < (NMemoPacket-1)) && PacketReceived(sock)){
	    continue;
	  }else{
	    BOOLEAN CONTINUE=TRUE;
	    register int k;
	    int packet_exp= packet_expected;

	    do{
	      packet_exp = (packet_exp + 1) % 65536;
	      if(STAT_MODE){
		fprintf(F_dloss, 
			"LOSS: packet expected: %d, packet received : %d\n",
			packet_expected, sequence_number);
		fflush(F_dloss);
	      }
	      pckt_lost ++;
	      pckt_tot ++;
	      for(k=0; k<NMemoPacket; k++){
		if(packet_exp == memo_packet_seq[k]){
		  sequence_number = packet_exp;
		  lr = memo_packet_lr[k];
		  bcopy((char *)&memo_packet[k][0], (char *)buf, lr);
		  memo_packet_lr[k] = 0;
		  nb_memo_packet --;
		  CONTINUE = FALSE;
		  fprintf(stderr, "LOST: releasing #%d packet, nb_memo=%d\n", 
			  sequence_number, nb_memo_packet);
		  break;
		}
	      }
	    }while(CONTINUE);
	    SYNC = (buf[1] & 0x20 ? TRUE : FALSE);
	    FULL_INTRA = ((buf[9] & 0x40) ? TRUE : FALSE);

	  }
	  
	  PACKET_LOST = TRUE;
	  if(E == FALSE){
	    /* The previous decoded packet was only a part of GOB.
	    *  We must now synchronize with start of the next GOB.
	    */
	    if(AFTER_GBSC == FALSE){
	      /* We have to be at "after_gbsc" line to synchronize.
	      *  Padding with enough zeroes should be enough...
	      */
	      if(buf[8] & 0x80){
		/* The next packet has Sbit == 1. So we have to save it.
		*/
		BUF_SAVED = TRUE;
		bcopy((char *)buf, (char *)memo_buf, lr);
#ifdef DEBUG
		printf("Save %d bytes since Sbit =1\n", lr);
#endif
		packet_expected = sequence_number;
	      }else
		packet_expected = (sequence_number + 1) % 65536;
	      bzero((char *)buf, 10);
	      buf[9] = 0x01;
	      *i = 0;
	      *j = 0;
	      TRYING_RESYNC = TRUE;
	      return(10);
	    }else{
	      /* We are at the "after_gbsc" line. So we only need to
	      *  decode the next packet with Sbit == 1.
	      */
	      packet_expected = (sequence_number + 1) % 65536;
	      if(buf[8] & 0x80){
		break;
	      }else{
		continue;
	      }
	    }
	  }else{
	    /* Following code to decode must be a GBSC. So we only need to
            *  decode the next packet with Sbit == 1.
	    */
	    packet_expected = (sequence_number + 1) % 65536;
	    if(buf[8] & 0x80){
	      break;
	    }else{
	      continue;
	    }
	  }
	}
      }
    }while(1);

    old_format = format;
    format = buf[9] & 0x07;
    if(SUPER_CIF)
      cif_number = format & 0x03;
    ustime_down = buf[7];
    ustime_up = buf[6];
    stime_down = buf[5];
    stime_up = buf[4];
    ustime = (u_short)((ustime_up << 8) + ustime_down);
    sec_stime = (u_short)((stime_up << 8) + stime_down);

    if(FIRST){
      old_ustime = ustime;
      old_stime = sec_stime;
      old_ustime_up = ustime_up;
      old_ustime_down = ustime_down;
      old_stime_up = stime_up;
      old_stime_down = stime_down;
      old_format = format;
      NG_MAX = ((buf[9] & 0x07) ? 12 : 5);
      MV_ENCODED = ((buf[9] & 0x20) ? TRUE : FALSE);
      if(SUPER_CIF && MV_ENCODED){
	fprintf(stderr, "Sorry, Super CIF is not full standard\n");
	exit(1);
      }
      FIRST = FALSE;
    }

#ifndef RECV_ONLY
    if(PACKET_LOST && (ustime_down != old_ustime_down)){
      /*
      *  We didn't display the previous image...
      */
      if(SUPER_CIF){
	show(dpy, ximage, L_col2, L_lig2);
      }else
	show(dpy, ximage, ximage->width, ximage->height);
    }
#endif
      
    Ebit = buf[8] & 0x07;
    Sbit = (buf[8] >> 4) & 0x07;
    E = ((buf[8] & 0x08) ? TRUE : FALSE);
    S = ((buf[8] & 0x80) ? TRUE : FALSE);
    if(S){
      /* It is a beginning of GOB, so a GBSC must be added */
      *i = 8;
      buf[8] = 0;
      if(Sbit == 0){
	*j = 0;
	buf[9] = 0x01;
      }else{
	buf[9] = 0x00;
	buf[10] |= SMASK[Sbit];
	*j = Sbit;
      }
    }else{
      /* It is not a beginning of GOB */
      *i = 10;
      *j = Sbit;
    }
    return(lr);
  }

#endif /* NOT INPUT_FILE */
    



VideoDecode(fd, port, group, name, sin_addr, display, QCIF2CIF, STAT,
	    FORCE_GREY, control_port)
     int fd[2];
     char *port, *group;
     char *name; /* Window's name. */
     u_long sin_addr;
     char *display;
     BOOLEAN QCIF2CIF; /* True if CIF display. */
     BOOLEAN STAT; /* True if STAT mode set */
     BOOLEAN FORCE_GREY; /* True if Gray display only */
     u_short control_port; /* Feedback port number */

{
    int RT=0; /* Temporal reference */
    BOOLEAN PART_ECRAN, CAMERA_DOC, GEL_IMAGE; 
    BOOLEAN CIF; /* True if CIF format */
    BOOLEAN QUANTM, DVM, CBC, COEFFT, FIL; /* booleans for TYPEM value */
    BOOLEAN P; /* Booleans for CBC : P&0x01->Cr, .., P&0x20->Y1 */
    BOOLEAN FIRST=TRUE; /* True if it is the first image. */
    BOOLEAN GO=TRUE;
    BOOLEAN DOUBLE=FALSE; /* True if QCIF && QCIF2CIF */
    BOOLEAN BIT_ERROR;
    int quant; /* Current quantizer */
    int mba; /* Current macroblock number */
    int x, new_x, new_bit, result;
    BLOCK8x8 block; /* current DCT coefficients block */
    int *tab_quant1, *tab_quant2;
    int x_mb, y_mb; /* MB's offset */
    int x_mb2, y_mb2;
    u_char chaine_codee[T_MAX];
    int i = 0; /* Current byte of chaine_codee. */
    int j = 0; /* Current bit of i */
    int memo_RT;
    register int l;
    struct sockaddr_in addr;
    int len_addr;
    char *pt;
    int NG_EXPECTED=0; /* Next GOB number expected */
    int full_cbc;
    int temp;
    char file_loss[200]; 


    feedback_port = control_port;
    FEEDBACK = (feedback_port ? TRUE : FALSE);
    fd_pipe = fd[0];
    sock=CreateReceiveSocket(&addr, &len_addr, port, group);
#ifdef INPUT_FILE
    strcpy(filename, name);
    strcpy(host_coder, filename);
#else
    if((pt = (char *)index(name, '@')) != NULL)
      strcpy(host_coder, (char *) (pt+1));
#endif
    if(STAT){
      strcpy(file_loss, ".ivs_dloss_");
      strcat(file_loss, host_coder);
      if((F_dloss=fopen(file_loss, "w")) == NULL){
	fprintf(stderr, "cannot create .ivs.dloss file\n");
	STAT_MODE=FALSE;
      }else
	STAT_MODE=TRUE;
    }
    host_sin_addr = sin_addr;

#ifdef RECV_ONLY
    FULL_INTRA_REQUEST = FALSE;
    do{
      AFTER_GBSC=TRUE;
      lmax=ReadBuffer(sock, chaine_codee, &i, &j);
    }while(1);
#endif

    FULL_INTRA_REQUEST = (FEEDBACK ? TRUE : FALSE);

    after_gbsc:
    AFTER_GBSC = TRUE;
    lmax=ReadBuffer(sock, chaine_codee, &i, &j);

    do{
      l = 0;
      do{
	l ++;
      }while(get_bit(chaine_codee,i,j) == 0);
      if(l < 16){
	continue;
      }
#ifdef DEBUG
      if(l>16)
	fprintf(stderr, "Nota ****** GBSC with %d zeroes\n", l-1);
#endif
      NG = 0;
      for(l=0; l<4; l++)
	NG = (NG << 1) + get_bit(chaine_codee,i,j);
      if(FIRST && (NG != 0)){
	continue;
      }
      if(NG > NG_MAX){
#ifdef DEBUG	
	fprintf(stderr,"decode: Bad NG value\n", NG);
#endif
      }else
	break;
    }while(TRUE);
    AFTER_GBSC = FALSE;
    TRYING_RESYNC = FALSE;


    /* Using two separate modules is better for CIF & QCIF 
    *  coding performances. Avoid many reference pointers...
    */

    if(SUPER_CIF){
      /*--------------------------------------------------------------------*\
      *                        SUPER CIF ENCODING                            *
      \*--------------------------------------------------------------------*/
      static u_char tab_Y[4][288][352], tab_Cb[4][144][176], 
                    tab_Cr[4][144][176];

      do{ /* IMAGE LOOP */

	do{ /* GOB LOOP */

	  if(NG == 0){
	    /*************\
	    * IMAGE LAYER *
	    \*************/

	    memo_RT = RT;
	    RT = get_bit(chaine_codee,i,j);
	    for(l=1; l<5; l++)
	      RT = (RT<<1) + get_bit(chaine_codee,i,j);
	
	    PART_ECRAN = get_bit(chaine_codee,i,j);
	    CAMERA_DOC = get_bit(chaine_codee,i,j);
	    GEL_IMAGE = get_bit(chaine_codee,i,j);
	    CIF = get_bit(chaine_codee,i,j);
	    if(FIRST){
	      L_col = 352;
	      L_lig = 288;
	      L_col2 = L_col << 1;
	      L_lig2 = L_lig << 1;
	      NG_MAX = 12;
	      strcat(name, " Super CIF");

	      temp = lmax;
	      init_window(L_lig2, L_col2, name, display, &appCtxt, &appShell,
		    &depth, &dpy, &win, &gc, &ximage, FORCE_GREY);
	      lmax = temp;
	      full_cbc = (COLOR_ENCODED ? 63 : 60);
	      FIRST = FALSE;
	    }
	    temp = get_bit(chaine_codee,i,j); /* reserved */
	    temp = get_bit(chaine_codee,i,j); /* reserved */
	    while (get_bit(chaine_codee,i,j))
	      /* RESERVEI option unused */
	      for(l=0; l<8; l++)
		temp = get_bit(chaine_codee,i,j);
	    temp = get_bit(chaine_codee,i,j);
	    l = 0;
	    do{
	      l ++;
	    }while(get_bit(chaine_codee,i,j) == 0);
	    if(l < 15){
#ifdef DEBUG
	      fprintf(stderr,
		      "decode: Bad Header GOB value after image, l=%d\n", l);
#endif
	      goto after_gbsc;
	    }
	    if(l > 15){
	      if(TRYING_RESYNC){
		goto after_gbsc;
	      }
	    }
	    NG_EXPECTED = 1;
	    NG = get_bit(chaine_codee,i,j);
	    for(l=1; l<4; l++)
	      NG = (NG << 1) + get_bit(chaine_codee,i,j);
	    if(NG != NG_EXPECTED){
#ifdef DEBUG
	      fprintf(stderr, "Bad NG number %d after image layer\n", NG);
#endif
	      goto after_gbsc;
	    }
	    continue;
	  }
	  /***********************\
	  * GROUP OF BLOCKS LAYER *
	  \***********************/

	  quant = get_bit(chaine_codee,i,j);
	  for(l=1; l<5; l++)
	    quant = (quant<<1) + get_bit(chaine_codee,i,j);
	  tab_quant1 = tab_rec1[quant-1];
	  tab_quant2 = tab_rec2[quant-1];
	  while (get_bit(chaine_codee,i,j))
	    /* RESERVEG option unused */
	    for(l=0; l<8; l++)
	      temp = get_bit(chaine_codee,i,j); 

	  /******************\
	  * MACROBLOCK LAYER *
	  \******************/

	  mba = 0;
	  BIT_ERROR = FALSE;
	  do{ /* MB LOOP */

	    /* Looking for MBA value */
	    do{
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(chaine_codee,i,j);
		new_x = state_mba[x].bit[new_bit];
	      }while(new_x != 0);
	      if((result = state_mba[x].result[new_bit]) > 0){
		mba += result;
		break;
	      }else{
		if(result == PADDING)
		  continue;
		if(result == CDI)
		  break;
		if(result == MORE0){
		  if(TRYING_RESYNC){
#ifdef DEBUG
		    fprintf(stderr, 
			    "goto after_gbsc because more than 15 zeroes\n");
#endif
		    goto after_gbsc;
		  }else{
		    while(get_bit(chaine_codee,i,j) == 0);
		    result = CDI;
		    break;
		  }
		}else{
		  BIT_ERROR=TRUE;
		  break;
		}
	      }
	    }while(1);

	    if(BIT_ERROR || (mba > 33)){
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding MBA value\n");
#endif
	      goto after_gbsc;
	    }
	    if(result == CDI)
	      break;
	    
	    x_mb = MBcol[NG][mba];
	    y_mb = MBlig[NG][mba];

	    x_mb2 = x_mb >> 1;
	    y_mb2 = y_mb >> 1;

	    /* Looking for TYPEM value */
	    new_x = 0;
	    do{
	      x = new_x;
	      new_bit = get_bit(chaine_codee,i,j);
	      new_x = state_typem[x].bit[new_bit];
	    }while(new_x > 0);
	    if(new_x != 0){
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding TYPEM\n");
#endif
	      goto after_gbsc;
	    }
	    if((result = state_typem[x].result[new_bit]) > 0){
	      QUANTM = result & 0x10;
	      DVM = result & 0x08;
	      CBC = result & 0x04;
	      COEFFT = result & 0x02;
	      FIL = result & 0x01;
	    }else{
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding TYPEM\n");
#endif
	      goto after_gbsc;
	    }
	
	    if(QUANTM){
	      quant = get_bit(chaine_codee,i,j);
	      for(l=1; l<5; l++)
	        quant = (quant<<1) + get_bit(chaine_codee,i,j);
	      tab_quant1 = tab_rec1[quant-1];
	      tab_quant2 = tab_rec2[quant-1];
	    }
	    if(CBC){
	      /* Looking for CBC value */
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(chaine_codee,i,j);
		new_x = state_cbc[x].bit[new_bit];
	      }while(new_x > 0);
	      if(new_x < 0){
#ifdef DEBUG
		fprintf(stderr, "Bit error detected while decoding CBC\n");
#endif
		goto after_gbsc;
	      }
	      P = state_cbc[x].result[new_bit];
	    }else
	      P = full_cbc;

	    /*************\
	    * BLOCK LAYER *
	    \*************/
	    if(COEFFT){
	      int cm; /* block[][cm+1] = 0 */
	      /* Block decoding in the order :
	      *  Y1, Y2, Y3, and Y4. If COLOR_ENCODED, Cb and Cr are encoded.
	      */
	      if(P & 0x20){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[cif_number][y_mb][x_mb]), cm, L_Y);
	      }
	      if(P & 0x10){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[cif_number][y_mb][x_mb+8]), cm, L_Y);
	      }
	      if(P & 0x08){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[cif_number][y_mb+8][x_mb]), cm, L_Y);
	      }
	      if(P & 0x04){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[cif_number][y_mb+8][x_mb+8]), cm, L_Y);
	      }
	      if(COLOR_ENCODED){
		if(P & 0x02){
		  if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, 
				     &j, &cm, CBC) < 0){
		    goto after_gbsc;
		  }  
		  if(affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			    &(tab_Cb[cif_number][y_mb2][x_mb2]), cm, L_Y_DIV2);
		}
		if(P & 0x01){
		  if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, 
				     &j, &cm, CBC) < 0){
		    goto after_gbsc;
		  }    
		  if(affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			    &(tab_Cr[cif_number][y_mb2][x_mb2]), cm, L_Y_DIV2);
		}
	      }
	    }

            build_image((u_char *) &tab_Y[cif_number][y_mb][x_mb], 
                        (u_char *) &tab_Cb[cif_number][y_mb2][x_mb2], 
                        (u_char *) &tab_Cr[cif_number][y_mb2][x_mb2], 
                        FALSE, ximage, L_Y, 
                        x_mb + y_mb*L_col2 + L_col*(cif_number & 1) + 
			L_col2*L_lig*(cif_number > 1), 
                        16, 16 ) ;

	  }while(1); /* MB LOOP */

	  NG_EXPECTED = (NG==NG_MAX ? 0 : NG+1);
	  temp = get_bit(chaine_codee,i,j);
	  for(l=1; l<4; l++)
	  temp = (temp << 1) + get_bit(chaine_codee,i,j);
	  if(temp == 0){
	    /* It's the next CIF image's header */
	    GO = FALSE;
	  }
	  if(temp != NG_EXPECTED && FEEDBACK){
#ifdef DEBUG
	    fprintf(stderr, 
               "CIF:%d, NG expected: %d, NG received: %d, oldt: %d, newt:%d\n",
		    cif_number, NG_EXPECTED, temp, old_ustime, ustime);
#endif
	    if(!PACKET_LOST){
#ifdef DEBUG
	      fprintf(stderr, "Some GOBs are missing without packet loss..\n");
#endif
	    }else{
	      PACKET_LOST = FALSE;
	      if(format == old_format){
		SendNACK(host_coder, feedback_port, TRUE, NG_EXPECTED, 
			 temp-1, format, stime_up, stime_down, ustime_up, 
			 ustime_down);
	      }else{
		if(temp == 0){
		  NG_EXPECTED = (NG_EXPECTED==0 ? 1 : NG_EXPECTED);
		  SendNACK(host_coder, feedback_port, TRUE, NG_EXPECTED, 
			   NG_MAX, old_format, old_stime_up,
			   old_stime_down, old_ustime_up, old_ustime_down);
		}else{
		  if(NG_EXPECTED != 0)
		    SendNACK(host_coder, feedback_port, FALSE, NG_EXPECTED, 
			     NG_MAX, old_format, old_stime_up, old_stime_down,
			     old_ustime_up, old_ustime_down);
		  SendNACK(host_coder, feedback_port, TRUE, 1, temp-1, 
			   format, stime_up, stime_down, ustime_up, 
			   ustime_down);
		}
	      }
	    }
	  }
	  NG = temp;
	  GO = (NG==0 ? FALSE : TRUE);
	}while(GO); /* GOB LOOP */
	
	GO = TRUE;
	
      }while(1); /* IMAGE LOOP */

    }else{
      /*------------------------------------------------------------------*\
      *                      CIF or QCIF encoding                          *
      \*------------------------------------------------------------------*/

      int INC_NG;
      static u_char tab_Y[288][352], tab_Cb[144][176], tab_Cr[144][176];
      static u_char memo_Y[288][352], memo_Cb[144][176], memo_Cr[144][176];
      int dvm_horiz, dvm_verti; /* Motion vectors components */
      int delta_mba;

      do{ /* IMAGE LOOP */

	do{ /* GOB LOOP */

	  if(NG == 0){
	    /*************\
	    * IMAGE LAYER *
	    \*************/
	    BOOLEAN NEW_FORMAT=FALSE;
	    BOOLEAN LAST_CIF=CIF;
      
	    memo_RT = RT;
	    RT = get_bit(chaine_codee,i,j);
	    for(l=1; l<5; l++)
	      RT = (RT<<1) + get_bit(chaine_codee,i,j);
	
	    PART_ECRAN = get_bit(chaine_codee,i,j);
	    CAMERA_DOC = get_bit(chaine_codee,i,j);
	    GEL_IMAGE = get_bit(chaine_codee,i,j);
	    CIF = get_bit(chaine_codee,i,j);
	    if(TRYING_RESYNC)
	      goto after_gbsc;
	    if(!FIRST && (LAST_CIF != CIF))
	      NEW_FORMAT = TRUE;
	    if(FIRST || NEW_FORMAT){
	      if(CIF){
		L_col = 352;
		L_lig = 288;
		NG_MAX = 12;
		strcat(name, " CIF");
		INC_NG = 1;
	      }
	      else{
		if(QCIF2CIF){
		  DOUBLE = TRUE;
		  strcat(name, " QCIF -> CIF");
		  L_col = 352;
		  L_lig = 288;
		}else{
		  strcat(name, " QCIF");
		  L_col = 176;
		  L_lig = 144;
		}
		NG_MAX = 5;
		INC_NG = 2;
	      }
	      if(NEW_FORMAT){
		/* New format detected */
#ifdef DEBUG
		fprintf(stderr, "new format detected\n");
#endif
		kill_window(dpy, ximage);
		if(!FULL_INTRA){
		  /* Waiting for Full INTRA image */
		  i = j = 0;
		  FULL_INTRA_REQUEST = TRUE;
		  lmax = ReadBuffer(sock, chaine_codee, &i, &j);
		}
	      }
	      temp = lmax;
	      init_window(L_lig, L_col, name, display, &appCtxt, &appShell,
		    &depth, &dpy, &win, &gc, &ximage, FORCE_GREY);
	      lmax = temp;

              if (affCOLOR & MODNB) DOUBLE = TRUE ;
	      full_cbc = (COLOR_ENCODED ? 63 : 60);
	      FIRST = FALSE;
	    }else{
	      bcopy((char *)tab_Y, (char *)memo_Y, 101376);
	      if(COLOR_ENCODED){
		bcopy((char *)tab_Cb, (char *)memo_Cb, 25344);
	        bcopy((char *)tab_Cr, (char *)memo_Cr, 25344); 
	      }
	    }
	    temp = get_bit(chaine_codee,i,j); /* reserved */
	    temp = get_bit(chaine_codee,i,j); /* reserved */
	    while (get_bit(chaine_codee,i,j))
	      /* RESERVEI option unused */
	      for(l=0; l<8; l++)
		temp = get_bit(chaine_codee,i,j);
	    temp = get_bit(chaine_codee,i,j);
	    l = 0;
	    do{
	      l ++;
	    }while(get_bit(chaine_codee,i,j) == 0);
	    if(l < 15){
#ifdef DEBUG
	      fprintf(stderr,"decode: Bad Header GOB value after image\n");
#endif
	      goto after_gbsc;
	    }
	    if(l > 15){
	      if(TRYING_RESYNC){
#ifdef DEBUG
		fprintf(stderr,
			"more than 15 zeroes decoded while image decoding\n");
#endif
		goto after_gbsc;
	      }
	    }
	    NG_EXPECTED = 1;
	    NG = get_bit(chaine_codee,i,j);
	    for(l=1; l<4; l++)
	      NG = (NG << 1) + get_bit(chaine_codee,i,j);
	    if(NG != NG_EXPECTED){
#ifdef DEBUG
	      fprintf(stderr, "Bad NG number %d after image layer\n", NG);
#endif
	      goto after_gbsc;
	    }
	    continue;
	  }

	  /***********************\
	  * GROUP OF BLOCKS LAYER *
	  \***********************/

	  quant = get_bit(chaine_codee,i,j);
	  for(l=1; l<5; l++)
	    quant = (quant<<1) + get_bit(chaine_codee,i,j);
	  tab_quant1 = tab_rec1[quant-1];
	  tab_quant2 = tab_rec2[quant-1];
	  while (get_bit(chaine_codee,i,j))
	    /* RESERVEG option unused */
	    for(l=0; l<8; l++)
	      temp = get_bit(chaine_codee,i,j); 

 	  /******************\
          * MACROBLOCK LAYER *
	  \******************/

	  mba = 0;	  
	  BIT_ERROR = FALSE;
	  do{ /* MB LOOP */
	    /* Looking for MBA value */
	    do{
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(chaine_codee,i,j);
		new_x = state_mba[x].bit[new_bit];
	      }while(new_x != 0);
	      if((result = state_mba[x].result[new_bit]) > 0){
		mba += result;
		break;
	      }else{
		if(result == PADDING)
		  continue;
		if(result == CDI)
		  break;
		if(result == MORE0){
		  if(TRYING_RESYNC){
#ifdef DEBUG
		    fprintf(stderr, 
			    "goto after_gbsc because more than 15 zeroes\n");
#endif
		    goto after_gbsc;
		  }else{
		    while(get_bit(chaine_codee,i,j) == 0);
		    result = CDI;
		    break;
		  }
		}else{
		  BIT_ERROR=TRUE;
		  break;
		}
	      }
	    }while(1);

	    if(BIT_ERROR || (mba > 33)){
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding MBA value\n");
#endif
	      goto after_gbsc;
	    }
	    if(result == CDI){
	      break;
	    }

	    delta_mba = result;
	    x_mb = MBcol[NG][mba];
	    y_mb = MBlig[NG][mba];
	
	    x_mb2 = x_mb >> 1;
	    y_mb2 = y_mb >> 1;

	    /* Looking for TYPEM value */
	    new_x = 0;
	    do{
	      x = new_x;
	      new_bit = get_bit(chaine_codee,i,j);
	      new_x = state_typem[x].bit[new_bit];
	    }while(new_x > 0);
	    if(new_x != 0){
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding TYPEM\n");
#endif
	      goto after_gbsc;
	    }
	    if((result = state_typem[x].result[new_bit]) > 0){
	      QUANTM = result & 0x10;
	      DVM = result & 0x08;
	      CBC = result & 0x04;
	      COEFFT = result & 0x02;
	      FIL = result & 0x01;
	    }else{
#ifdef DEBUG
	      fprintf(stderr, "Bit error detected while decoding TYPEM\n");
#endif 
	      goto after_gbsc;
	    }

	    if(QUANTM){
	      quant = get_bit(chaine_codee,i,j);
	      for(l=1; l<5; l++)
	        quant = (quant<<1) + get_bit(chaine_codee,i,j);
	      tab_quant1 = tab_rec1[quant-1];
	      tab_quant2 = tab_rec2[quant-1];
	    }

	    if(MV_ENCODED){
	      if((!DVM) || (mba % 11 == 1) || (delta_mba != 1)){
		/* We have to reset the motion vectors */
		dvm_horiz = 0;
		dvm_verti = 0;
	      }
	      if(DVM){
		int xx, yy, xx2, yy2;

		/* Looking for horizontal DVM value. */
		new_x = 0;
		do{
		  x = new_x;
		  new_bit = get_bit(chaine_codee,i,j);
		  new_x = state_dvm[x].bit[new_bit];
		}while(new_x > 0);
		result = state_dvm[x].result[new_bit];
		if(result < -16){
#ifdef DEBUG
		  fprintf(stderr, "Bit error detected while decoding HDVM\n");
#endif
		  goto after_gbsc;
		}
		if(result > 1)
		  temp = result - 32;
		else
		  temp = result + 32;
		if(abs(dvm_horiz + result) < 16)
		  dvm_horiz += result;
		else
		  dvm_horiz += temp;

		/* Looking for vertical DVM value. */
		new_x = 0;
		do{
		  x = new_x;
		  new_bit = get_bit(chaine_codee,i,j);
		  new_x = state_dvm[x].bit[new_bit];
		}while(new_x > 0);
		result = state_dvm[x].result[new_bit];
		if(result < -16){
#ifdef DEBUG
		  fprintf(stderr, "Bit error detected while decoding VDVM\n");
#endif
		  goto after_gbsc;
		}
		if(result>1)
		  temp = result - 32;
		else
		  temp = result + 32;
		if(abs(dvm_verti + result) < 16)
		  dvm_verti += result;
		else
		  dvm_verti += temp;

		xx = x_mb + dvm_horiz;
		yy = y_mb + dvm_verti;
		xx2 = x_mb2 + (dvm_horiz / 2);
		yy2 = y_mb2 + (dvm_verti / 2);

		if((dvm_horiz || dvm_verti) && (!FIL)){
		  /*
		  *  We have to move the Macroblock.
		  */
		  register int y;
		  int dvm_C_verti, aux;

		  aux = y_mb + 16;
		  for(y=y_mb; y<aux; y++)
		    bcopy((char *)&(memo_Y[y+dvm_verti][xx]), 
			  (char *)&(tab_Y[y][x_mb]), 16);
		  if(COLOR_ENCODED){
		    dvm_C_verti = dvm_verti / 2;
		    aux = y_mb2 + 8;
		    for(y=y_mb2; y<aux; y++)
		      bcopy((char *)&(memo_Cb[y+dvm_C_verti][xx2]), 
			    (char *)&(tab_Cb[y][x_mb2]), 8);
		    for(y=y_mb2; y<aux; y++)
		      bcopy((char *)&(memo_Cr[y+dvm_C_verti][xx2]), 
			    (char *)&(tab_Cr[y][x_mb2]), 8);
		  }
		}

		if(FIL){
		  /*
		  *  Loop-filter applied on the macroblock.
		  */
		  filter_loop(&(memo_Y[yy][xx]),&tab_Y[y_mb][x_mb],L_Y);
		  filter_loop(&(memo_Y[yy][xx+8]),&tab_Y[y_mb][x_mb+8],L_Y);
		  filter_loop(&memo_Y[yy+8][xx],&tab_Y[y_mb+8][x_mb],L_Y);
		  filter_loop(&memo_Y[yy+8][xx+8],&tab_Y[y_mb+8][x_mb+8],L_Y);
		  if(COLOR_ENCODED){
		    filter_loop(&memo_Cb[yy2][xx2],&tab_Cb[y_mb2][x_mb2],
				L_Y/2);
		    filter_loop(&memo_Cr[yy2][xx2],&tab_Cr[y_mb2][x_mb2],
				L_Y/2);
		  }
		}
	      } /* fi DVM */
	    } /* fi MV_ENCODED */

	    if(CBC){
	      /* Looking for CBC value */
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(chaine_codee,i,j);
		new_x = state_cbc[x].bit[new_bit];
	      }while(new_x > 0);
	      if(new_x < 0){
#ifdef DEBUG
		fprintf(stderr, "Bit error detected while decoding CBC\n");
#endif
		goto after_gbsc;
	      }
	      P = state_cbc[x].result[new_bit];
	    }else
	      P = full_cbc;

	    /*************\
	    * BLOCK LAYER *
	    \*************/
	    if(COEFFT){
	      int cm; /* block[][cm+1] = 0 */
	      /* Block decoding in the order :
	      *  Y1, Y2, Y3, and Y4. If COLOR_ENCODED, Cb and Cr are encoded.
	      */
	      if(P & 0x20){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[y_mb][x_mb]), cm, L_Y);
	      }
	      if(P & 0x10){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[y_mb][x_mb+8]), cm, L_Y);
	      }
	      if(P & 0x08){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[y_mb+8][x_mb]), cm, L_Y);
	      }
	      if(P & 0x04){
		if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, 
				&cm, CBC) < 0){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 &(tab_Y[y_mb+8][x_mb+8]), cm, L_Y);
	      }
	      if(COLOR_ENCODED){
		if(P & 0x02){
		  if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, 
				     &j, &cm, CBC) < 0){
		    goto after_gbsc;
		  }
	          if (affCOLOR & MODCOLOR)
                    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			   &(tab_Cb[y_mb2][x_mb2]), cm, L_Y_DIV2);
		}
		if(P & 0x01){
		  if(decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, 
				     &j, &cm, CBC) < 0){
		    goto after_gbsc;
		  }
	          if (affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			   &(tab_Cr[y_mb2][x_mb2]), cm, L_Y_DIV2);
		}
	      }
	    }


            build_image((u_char *) &tab_Y[y_mb][x_mb], 
                        (u_char *) &tab_Cb[y_mb2][x_mb2], 
                        (u_char *) &tab_Cr[y_mb2][x_mb2], 
                        DOUBLE, ximage, 
                        L_Y, y_mb*ximage->width + x_mb, 16, 16 ) ;


	  }while(1); /* MB LOOP */

	  NG_EXPECTED = (NG==NG_MAX ? 0 : NG+INC_NG);
	  temp = get_bit(chaine_codee,i,j);
	  for(l=1; l<4; l++)
	  temp = (temp << 1) + get_bit(chaine_codee,i,j);
	  if(temp == 0){
	    /* It's the next image's header */
#ifdef INPUT_FILE
	    show(dpy, ximage, ximage->width, ximage->height);
#endif
	    GO = FALSE;
	  }
	  if(temp != NG_EXPECTED && FEEDBACK){
#ifdef DEBUG
	    fprintf(stderr, 
		    "NG expected: %d, NG received: %d, oldt: %d, newt:%d\n",
		    NG_EXPECTED, temp, old_ustime, ustime);
#endif
	    if(!PACKET_LOST){
#ifdef DEBUG
	      fprintf(stderr, "Some GOBs are missing without packet loss..\n");
#endif
	    }else{
	      PACKET_LOST = FALSE;
	      if(equal_time(old_stime, old_ustime, sec_stime, ustime)){
		SendNACK(host_coder, feedback_port, TRUE, NG_EXPECTED, 
			 temp-INC_NG, format, stime_up, stime_down, ustime_up, 
			 ustime_down);
	      }else{
		if(temp == 0){
		  NG_EXPECTED = (NG_EXPECTED==0 ? 1 : NG_EXPECTED);
		  SendNACK(host_coder, feedback_port, TRUE, NG_EXPECTED, 
			   NG_MAX, old_format, old_stime_up, 
			   old_stime_down, old_ustime_up, old_ustime_down);
		}else{
		  if(NG_EXPECTED != 0)
		    SendNACK(host_coder, feedback_port, FALSE, NG_EXPECTED, 
			     NG_MAX, old_format, old_stime_up, old_stime_down,
			     old_ustime_up, old_ustime_down);
		  SendNACK(host_coder, feedback_port, TRUE, 1, temp-INC_NG, 
			   format, stime_up, stime_down, ustime_up, 
			   ustime_down);
		}
	      }
	    }
	  }
	  NG = temp;
	}while(GO); /* GOB LOOP */
	GO = TRUE;
      }while(1); /* IMAGE LOOP */
    } /* CIF or QCIF */
  } /* VideoDecode() */



