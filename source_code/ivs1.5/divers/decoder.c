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
*  Fichier : decoder.c                                                     *
*  Date    : May 1992                                                      *
*  Auteur  : Thierry Turletti                                              *
*  Release : 1.1                                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 decoder procedures.                                *
*                                                                          *
\**************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include "in.h"
#include <netdb.h>
#include <sys/time.h>
#include "h261.h"
#include <assert.h>
#include "ulaw2linear.h"

#define ESCAPE 0
#define EOB   -1
#define L_Y 352
#define T_READ_MAX 4096
#define CDI -1 /* CDI decoding */
#define PADDING 0 /* Padding decoding (11 bits). */
#define VIDEO_PACKET 0
#define AUDIO_PACKET 2
#define NB_PORT_MSG_LOSS 3333

#define get_bit(octet,i,j) \
    (j==7 ? (j=0, ((i)+1)<lmax ? octet[(i)++]&0x01 : (vali=octet[i], \
			 lmax=ReadBuffer(desc, octet, &i), vali&0x01)) \
          : (octet[(i)]>>(7-(j)++))&0x01)


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

struct soundbuf{
  u_char compression;
  struct{
    int buffer_len;
    char buffer_val[8000];
  }buffer;
};


int udp_mode, desc, lmax, vali; /* Global variables */
char host_coder[50];

decode_bloc_8x8(chaine_codee, bloc, state, i, j, colmax, inter)
    unsigned char *chaine_codee;
    int bloc [][8];
    etat *state;
    int *i, *j;
    int *colmax; /* bloc[][colmax+1] = 0 */
    int inter; /* Boolean: true if INTER mode */

  {
    int x, new_x; /* previous and new state */
    int run, level, result;
    int temp, new_bit, negatif;
    int col, maxcol=0;
    register int l, k, jj;
    int ii;

    k = 0; /* Next coefficient block. */
    ii = *i;
    jj = *j;
    bzero((char *)bloc, 256);
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
      }while(new_x != 0);
      if((result=state[x].result[new_bit]) > 0){
	/* It is a Variable Length Code. */
	run = result >> 4;
	level = result & 0x0f;
	k += run;
	if((col=ZAG[k]) > maxcol)
	    maxcol=col;
	/* Level's sign. */
	if(get_bit(chaine_codee,ii,jj))
	    bloc[ZIG[k]][col] = -level;
	else
	    bloc[ZIG[k]][col] = level;
	k ++;
	continue;
      }
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
      }
      if(result == EOB)
	  break;

    }while(1);
    *colmax=maxcol;
    *i = ii;
    *j = jj;
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


int declare_listen(num_port)
    int num_port;

  {
    int size_of_ad_in, one=1;
    struct sockaddr_in ad_in;
    int sock;
    struct ip_mreq imr;
    struct hostent *hp;
    char hostname[64];


    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      fprintf(stderr, "Cannot create socket-controle UDP\n");
      exit(1);
    }
    gethostname(hostname, 64);
    if((hp=gethostbyname(hostname)) == 0){
      fprintf(stderr, "Unknown host %s\n", hostname);
      exit(0);
    }
    bcopy(hp->h_addr, (char *)&imr.imr_interface.s_addr, hp->h_length);
    printf("Joining the (224.2.2.2) group\n");
    imr.imr_multiaddr.s_addr = (224<<24) | (2<<16) | (2<<8) | 2;
    imr.imr_multiaddr.s_addr = htonl(imr.imr_multiaddr.s_addr);
    imr.imr_interface.s_addr = htonl(imr.imr_interface.s_addr);
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr,
		  sizeof(struct ip_mreq)) == -1){
      fprintf(stderr, "Cannot join group\n");
      exit(1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    ad_in.sin_family = AF_INET;
    ad_in.sin_port = htons(num_port);
    ad_in.sin_addr.s_addr = INADDR_ANY;

    size_of_ad_in = sizeof(ad_in);
    if (bind(sock,(struct sockaddr *) &ad_in, size_of_ad_in) < 0){
      fprintf(stderr, "Cannot bind socket to %d port\n", num_port);
      close(sock);
      exit(1);
    }
    
    return(sock);
  }    

  
int ReceivePacket(host_coder, sock, buf)
    char *host_coder;
    int sock;
    u_char *buf;

  {
    struct sockaddr_in adr;
    struct hostent *host;
    int er, len;

    er = sizeof(adr);
    if ((len = recvfrom(sock, (char *)buf, T_MAX, 0, &adr, &er)) < 0){
      perror("receive from");
      exit(0);
    }

    host=gethostbyaddr((char *)(&(adr.sin_addr)), sizeof(adr.sin_addr),
                       adr.sin_family);
    strcpy(host_coder, host->h_name);

    printf("Received %d bytes from %s\n",len, host_coder);

    return(len);
  }


ReportToCoder(num)
    int num;
  {
    static struct sockaddr_in addr_coder;
    static struct hostent *hp;
    static int sock_report, len_addr, first_report=1;
    u_char buf[1];
    
    if(first_report){
      if((hp=gethostbyname(host_coder))==0){
	fprintf(stderr, "Unknown host : %s\n", host_coder);
	exit(1);
      }
      bzero((char *)&addr_coder, sizeof(addr_coder));
      bcopy(hp->h_addr, (char *)&addr_coder.sin_addr, hp->h_length);
      addr_coder.sin_family = hp->h_addrtype;
      addr_coder.sin_port = htons(NB_PORT_MSG_LOSS);
      addr_coder.sin_family = AF_INET;
      len_addr = sizeof(addr_coder);
      if((sock_report = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	fprintf(stderr, "Cannot create datagram socket\n");
	exit(-1);
      }
      first_report=0;
    }
    buf[0] = (u_char)num;
    if(sendto(sock_report, (char *)buf, 1, 0, &addr_coder, len_addr) < 0){
      fprintf(stderr, "sendto: Cannot send loss message on socket\n");
      exit(-1);
    }
  }


static void PlayAudio(msg)
  struct soundbuf *msg;
{
    char *val;
    int len;
    char auxbuf[8002];
    static int FIRST=1;
    int debugging;

    debugging = (msg->compression & 2) ? TRUE : FALSE;

    if(FIRST){
      if(!soundinit(O_WRONLY)) {
	perror("opening audio output device");
	return;  /* Can't acquire sound device */
      }
      FIRST=0;
    }

    if (debugging) {
        printf("Playing %d%s bytes.\n", msg->buffer.buffer_len,
                (msg->compression & 1) ? " compressed" : "");
    }
    len = msg->buffer.buffer_len;
    val = msg->buffer.buffer_val;

    /* If the 4 bit is on, use the 8 bit to re-route the sound.  This
       is normally used to divert the sound to the speaker to  get  an
       individual's attention.  */

    if (msg->compression & 4) {
	sounddest((msg->compression & 8) ? 1 : 0);
	if (!(msg->compression & 8)) {
	    soundplayvol(50);	      /* Make sure volume high enough */
	}
    }

    /* If the buffer contains compressed sound samples, reconstruct an
       approximation of the original  sound  by  performing  a	linear
       interpolation between each pair of adjacent compressed samples.
       Note that since the samples are logarithmically scaled, we must
       transform  them	to  linear  before interpolating, then back to
       log before storing the calculated sample. */

    if (msg->compression & 1) {
	int i;
	register char *ab = auxbuf;

	assert(len < sizeof auxbuf);
	for (i = 0; i < len; i++) {
	    *ab++ = i == 0 ? *val :
		(audio_s2u((audio_u2s(*val) + audio_u2s(val[-1])) / 2));
	    *ab++ = *val++;
	}
	len *= 2;
	val = auxbuf;
    }

    soundplay(len, val);
}


int ReadBuffer(desc, buf, i)
    int desc;
    u_char *buf;
    int *i;

  {
    int lread=0;
    static int number_packet;
    int num, type;

    if(udp_mode){
      char host[50];
      do{
	lread=ReceivePacket(host, desc, buf);
	if(strcmp(host, host_coder)){
	  if(strcmp(host_coder, "nobody")){
	    fprintf(stderr, "Receive and throw away %d bytes from %s\n",
		    lmax, host);
	  }else{
	    /* It's the first packet, save coder's hostname */
	    strcpy(host_coder, host);
	    number_packet = buf[0] & 0x3f;
	  }
	}
	type = buf[0] >> 6;
	if(type == VIDEO_PACKET){
	  num = buf[0] & 0x3f;
	  while(num != number_packet){
#ifdef DEBUG
	    fprintf(stderr, "packet #%d is lost\n", number_packet);
#endif
	    ReportToCoder(number_packet);
	    number_packet=(number_packet==63 ? 0 : number_packet+1);
	  }
	  number_packet=(num==63 ? 0 : num+1);
	  *i = 1;
	  break;
	}else{
	  if(type == AUDIO_PACKET)
	    PlayAudio((struct soundbuf *)buf);
	  else
	    fprintf(stderr, "Received a packet type %d\n", type);
	}
      }while(1);
    }else{
      if((lread=read(desc, (char *)buf, T_READ_MAX)) <= 0)
	  exit(1);
      *i = 0;
    }
    return(lread);
  }



    

decode(im_pix, name_win, state_mba, state_typem, state_cbc, state_dvm, 
       state_coeff, tab_rec1, tab_rec2, map_lut, NB, BCH, UDP, display, 
       num_port, QCIF2CIF)
    u_char *im_pix;
    char *name_win; /* Window's name. */
    etat *state_mba, *state_typem, *state_cbc, *state_dvm, *state_coeff;
    int tab_rec1[][255], tab_rec2[][255];
    u_char map_lut[];
    int NB; /* Boolean : true if W&B display */
    int BCH; /* Boolean : true if BCH encoding */
    int UDP; /* Boolean : true if data come from UDP. */
    char *display;
    int num_port;
    int QCIF2CIF; /* Boolean : true if CIF display. */
    

  {
    extern int depth; /* Depth of the screen */
    static u_char tab_Y[288][352], tab_Cb[144][176], tab_Cr[144][176];
    static u_char memo_Y[288][352], memo_Cb[144][176], memo_Cr[144][176];
    int RT=0; /* Temporal reference */
    int PART_ECRAN, CAMERA_DOC, GEL_IMAGE; /* Booleans */
    int CIF; /* Boolean : true if CIF format */
    int NG; /* Current GOB number*/
    int quant; /* Current quantizer */
    int temp;
    int mba; /* Current macroblock number */
    int x, new_x, new_bit, result;
    int QUANTM, DVM, CBC, COEFFT, FIL; /* booleans for TYPEM value */
    u_char P; /* Booleans for CBC : P&0x01->Cr, P&0x20->Y1 */
    BLOCK8x8 block; /* current DCT coefficients block */
    int *tab_quant1, *tab_quant2;
    int x_gob, y_gob; /* GOB's offset */
    int x_mb, y_mb; /* MB's offset */
    int x_mb2, y_mb2; 
    int L_col; 
    int L_lig; 
    int NG_MAX; /* Maximum NG value */
    int dvm_horiz, dvm_verti; /* motion vector components */
    u_char chaine_codee[T_MAX];
    int memo_RT;
    int FIRST=1; /* Boolean: true if it's the first image. */
    register int l;
    int i = 0; /* Current byte of chaine_codee. */
    int j = 0; /* Current bit of i */
    int GO=1;
    int DOUBLE=0; /* Boolean : true if QCIF && QCIF2CIF */
    extern int desc, lmax, udp_mode;


    if(UDP){
      udp_mode = 1;
      desc=declare_listen(num_port);
      strcpy(host_coder, "nobody");
    }else{
      udp_mode = 0;
      if((desc=open(name_win, 0)) < 0){
	fprintf(stderr, "decode : Cannot open %s\n", name_win);
	exit(1);
      }
    }
    lmax=ReadBuffer(desc, chaine_codee, &i);
    temp=get_bit(chaine_codee,i,j);
    for(l=1; l<20; l++)
	temp= (temp<<1) + get_bit(chaine_codee,i,j);
    if(temp != 16){
	fprintf(stderr,"decode: Bad First Header Image value\n");
	if(UDP){
	  ReportToCoder(0);
	}
	exit(-1);
    }else
      NG=0;


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
	  int id;
	  extern int fd[2], fdd[2];

	  if(CIF){
	    L_col = 352;
	    L_lig = 288;
	    NG_MAX = 12;
	    strcat(name_win, " CIF");
	  }
	  else{
	    if(QCIF2CIF){
	      DOUBLE=1;
	      strcat(name_win, " QCIF -> CIF");
	      L_col = 352;
	      L_lig = 288;
	    }else{
	      strcat(name_win, " QCIF");
	      L_col = 176;
	      L_lig = 144;
	    }
	    NG_MAX = 5;
	  }
	  if(UDP){

	    init_window(L_lig, L_col, NB, name_win, map_lut, &depth, display);
	    
	    /* pipe for map_luth */
	    if(pipe(fd)<0){
	      fprintf(stderr,"decode : error pipe Luth \n");
	      exit(1);
	    }

	    /* pipe for control */
	    if(pipe(fdd)<0){
	      fprintf(stderr,"%s : error pipe Control\n");
	      exit(1);
	    }

	    if((id=fork()) == -1){
	      fprintf(stderr,"%s : Cannot fork !\n");
	      exit(1);
	    }
	    if(id){
	      /* Father process is going to decode H.261 stream*/
	      close(fd[1]);
	      close(fdd[1]);
	    }else{
	      /* Le processus fils s'occupe du Control Video Panel. */
	      close(fd[0]);
	      close(fdd[0]);
	      if(NB)
		  New_Luth(UDP, host_coder);
	      else
		  exit(0);
	    }
	  }
	  FIRST=0;
	}
	temp = get_bit(chaine_codee,i,j); /* bit de reserve */
        temp = get_bit(chaine_codee,i,j); /* bit de reserve */
	while (get_bit(chaine_codee,i,j))
	    /* RESERVEI option : We have to ignore 8 bits. */
	    for(l=0; l<8; l++)
		temp = get_bit(chaine_codee,i,j);
        temp = get_bit(chaine_codee,i,j);
        for(l=1; l<16; l++)
	    temp = (temp<<1) + get_bit(chaine_codee,i,j);
        if(temp != 1){
	    fprintf(stderr,"decode: Bad Header GOB value after image\n");
    	    exit(-1);
        }
	NG = get_bit(chaine_codee,i,j);
	for(l=1; l<4; l++)
	    NG = (NG<<1) + get_bit(chaine_codee,i,j);
	continue;
      }
      /***********************\
      * GROUP OF BLOCKS LAYER *
      \***********************/

      if(NG > NG_MAX){
	fprintf(stderr,"decode: Bad Header GOB value, NG=%d\n", NG);
    	      exit(-1);
      }
      quant = get_bit(chaine_codee,i,j);
      for(l=1; l<5; l++)
	  quant = (quant<<1) + get_bit(chaine_codee,i,j);
      tab_quant1 = tab_rec1[quant-1];
      tab_quant2 = tab_rec2[quant-1];
      while (get_bit(chaine_codee,i,j))
	  /* Un champ optionnel de 8 bits suit */
	  /* On ignore l'option RESERVEG */
	  for(l=0; l<8; l++)
	      temp = get_bit(chaine_codee,i,j); 
      if(NG % 2)
	  x_gob = 0;
      else
	  x_gob = 176;
      y_gob = ((NG-1)/2)*48;

      /******************\
      * MACROBLOCK LAYER *
      \******************/

      mba = 0;
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
		else{
		  fprintf(stderr, "decode : Bad MBA decoded : %d\n", result);
		  exit(1);
		}
	    }
	  }while(1);

	if(result == CDI)
	    break;
	if(result != 1){
	    /* We have to reset the motion vector */
	    dvm_horiz = 0;
	    dvm_verti = 0;
	}
	if(mba > 33){
	  fprintf(stderr,"decode : Bad MB number : %d\n", mba);
	  exit(1);
	}
	if(mba > 22){ 
	  x_mb = x_gob + (mba-23)*16;
	  y_mb = y_gob + 32;
	}else{
	  if(mba>11){
	    x_mb = x_gob + (mba-12)*16;
	    y_mb = y_gob + 16;
	  }
	  else{
	    x_mb = x_gob + (mba-1)*16;
	    y_mb = y_gob;
	  }
	}
	x_mb2 = x_mb/2;
	y_mb2 = y_mb/2;
	/* Looking for TYPEM value */
	new_x = 0;
	do{
	  x = new_x;
	  new_bit = get_bit(chaine_codee,i,j);
	  new_x = state_typem[x].bit[new_bit];
	}while(new_x != 0);
	if((result = state_typem[x].result[new_bit]) > 0){
	  QUANTM = result & 0x10;
	  DVM = result & 0x08;
	  CBC = result & 0x04;
	  COEFFT = result & 0x02;
	  FIL = result & 0x01;
	}else{
	  fprintf(stderr,"Bad Value of CLV_TYPEM\n");
	  exit(-2);
	}
	
	if((!DVM) || (mba==1) || (mba==12) || (mba==23)){
	    /* We have to reset the motion vector */
	    dvm_horiz = 0;
	    dvm_verti = 0;
	}
	if(QUANTM){
            quant = get_bit(chaine_codee,i,j);
            for(l=1; l<5; l++)
	        quant = (quant<<1) + get_bit(chaine_codee,i,j);
            tab_quant1 = tab_rec1[quant-1];
            tab_quant2 = tab_rec2[quant-1];
	}
	if(DVM){
	    int xx, yy, xx2, yy2;

            /* Looking for horizontal DVM value. */
            new_x = 0;
            do{
              x = new_x;
              new_bit = get_bit(chaine_codee,i,j);
              new_x = state_dvm[x].bit[new_bit];
            }while(new_x != 0);
            result = state_dvm[x].result[new_bit];
	    if(result>1)
		temp = result - 32;
	    else
		temp = result + 32;
	    if(abs(dvm_horiz+result)<16)
		dvm_horiz += result;
	    else
		dvm_horiz += temp;

            /* Looking for vertical DVM value. */
            new_x = 0;
            do{
              x = new_x;
              new_bit = get_bit(chaine_codee,i,j);
              new_x = state_dvm[x].bit[new_bit];
            }while(new_x != 0);
            result = state_dvm[x].result[new_bit];
	    if(result>1)
		temp = result - 32;
	    else
		temp = result + 32;
	    if(abs(dvm_verti+result)<16)
		dvm_verti += result;
	    else
		dvm_verti += temp;

	    xx = x_mb + dvm_horiz;
	    yy = y_mb + dvm_verti;
	    xx2 = x_mb2 + (dvm_horiz / 2);
	    yy2 = y_mb2 + (dvm_verti / 2);

	    if((dvm_horiz||dvm_verti)&&(!FIL)){
	      /*
	      *  We have to move the Macroblock.
	      */
	      register int y;
	      int dvm_C_verti, aux;
	      
	      aux = y_mb + 16;
	      for(y=y_mb; y<aux; y++)
		  bcopy((char *)&(memo_Y[y+dvm_verti][xx]), 
			(char *)&(tab_Y[y][x_mb]), 16);
	      if(!NB){
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
	      if(!NB){
		filter_loop(&memo_Cb[yy2][xx2],&tab_Cb[y_mb2][x_mb2],L_Y/2);
		filter_loop(&memo_Cr[yy2][xx2],&tab_Cr[y_mb2][x_mb2],L_Y/2);
	      }
	    }
	}	    
	if(CBC){
	  /* Looking for CBC value */
          new_x = 0;
	  do{
	    x = new_x;
	    new_bit = get_bit(chaine_codee,i,j);
	    new_x = state_cbc[x].bit[new_bit];
          }while(new_x != 0);
	  P = state_cbc[x].result[new_bit];
	}else
	    P = 0x3c;

	/*************\
	* BLOCK LAYER *
	\*************/
	if(COEFFT){
	 int cm; /* block[][cm+1] = 0 */
	 /* Block decoding in the order :
	 *  Y1, Y2, Y3, Y4, Cb and Cr.
	 */
         if(P & 0x20){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, &(tab_Y[y_mb][x_mb]), 
		    cm, L_Y);
	 }
         if(P & 0x10){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb][x_mb+8]), cm, L_Y);
	 }
         if(P & 0x08){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb+8][x_mb]), cm, L_Y);
	 }
         if(P & 0x04){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb+8][x_mb+8]), cm, L_Y);
	 }
         if(P & 0x02){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   if(!NB)
	       ifct_8x8(block, tab_quant1, tab_quant2, 0, 
			&(tab_Cb[y_mb2][x_mb2]), cm, L_Y/2);
	 }
         if(P & 0x01){
	   decode_bloc_8x8(chaine_codee, block, state_coeff, &i, &j, &cm, 0);
	   if(!NB)
	       ifct_8x8(block, tab_quant1, tab_quant2, 0, 
			&(tab_Cr[y_mb2][x_mb2]), cm, L_Y/2);
	 }
        }

	if(!NB){
	  /* COLOR MACROBLOCK ENCODING (Y,Cr,Cb -> Pixel)
	  * R, V, B are encoded with 3 bits (0 to 5).
	  * One pixel contains (R|V|B) in this order and use 9 bits.
	  */
	  int l, c;
	  int Y, cB, cR;
	  int MAX = 1044480; /* 255x4096 */
	  register int x, y, yy;
	  register int R, V, B;
	  int value;

	  l = y_mb+16;
	  c = x_mb+16;
	  for(y=y_mb, yy=y*L_col; y<l; y++, yy+=L_col)
	    for(x=x_mb; x<c; x++){
	      Y = 4788*(tab_Y[y][x] - 16);
	      cB = (tab_Cb[y/2][x/2] - 128);
	      cR = (tab_Cr[y/2][x/2] - 128);
	      R = 2*(Y + 6563*cR);
	      R = (R>MAX ? 5 : (R<0 ? 0 : R/174370));
	      V = 2*(Y - 1611*cB - 3343*cR);
	      V = (V>MAX ? 5 : (V<0 ? 0 : V/174370));
	      B = 2*(Y + 8295*cB);
	      B = (B>MAX ? 5 : (B<0 ? 0 : B/174370));
	      value= (R<<6) + (V<<3) + B;
	      im_pix[yy+x] = map_lut[value];
	    }

	}else{
         /* B&W MACROBLOCK ENCODING (Y -> Pixel)
	 *  One pixel is encoded with 6 bits (64 shades of gray)
	 */
	 if(DOUBLE){
	   register int offset1, offset2;
	   register int l, c;
	   register u_char aux, *tab_Y_x, *im_pix1, *im_pix2;

	   offset1 = L_Y - 16;
	   offset2 = 2*L_col - 32;
	   tab_Y_x = &(tab_Y[y_mb][x_mb]);
	   im_pix1 = &(im_pix[(2*y_mb*L_col)+2*x_mb]);
	   im_pix2 = &(im_pix[((2*y_mb+1)*L_col)+2*x_mb]);
	   for(l=0; l<16; l++){
	     for(c=0; c<16; c++){
	       aux=map_lut[(*tab_Y_x++)];
	       *im_pix1++ = aux;
	       *im_pix1++ = aux;
	       *im_pix2++ = aux;
	       *im_pix2++ = aux;
	     }
	     tab_Y_x += offset1;
	     im_pix1 += offset2;
	     im_pix2 += offset2;
	   }
	 }else{
	   register int l, c, offset1, offset2;
	   register u_char *tab_Y_x, *im_pix_x;

	   offset1 = L_Y - 16;
	   offset2 = L_col - 16;
	   tab_Y_x = &(tab_Y[y_mb][x_mb]);
	   im_pix_x = &(im_pix[(y_mb*L_col)+x_mb]);
	   for(l=0; l<16; l++){
	     for(c=0; c<16; c++)
		 *im_pix_x++ = map_lut[(*tab_Y_x++)];
	     im_pix_x += offset2;
	     tab_Y_x  += offset1;
	   }
	 }
       }

      }while(1); /* MB LOOP */

      NG=get_bit(chaine_codee,i,j);
      for(l=1; l<4; l++)
	  NG = (NG<<1) + get_bit(chaine_codee,i,j);
      if(NG == 0){
	/* It's the next image's header */
	GO=0;
	/* Display the image */
	if(!show(L_col, L_lig, im_pix, map_lut))
	    exit(1);
	bcopy((char *)tab_Y, (char *)memo_Y, 101376);
	if(!NB){
	  bcopy((char *)tab_Cb, (char *)memo_Cb, 25344);
	  bcopy((char *)tab_Cr, (char *)memo_Cr, 25344);
	}
      }
	
     }while(GO); /* BOUCLE GOB */

     GO=1;
     
   }while(1); /* BOUCLE IMAGE */

   close(desc);
   return;

  } /* decode() */
