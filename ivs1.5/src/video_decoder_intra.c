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
*  File    : video_decoder.c                                               *
*  Date    : July 1992                                                     *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  H.261 decoder procedures.                                *
*                                                                          *
\**************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "quantizer.h"


#define ESCAPE 0
#define EOB   -1
#define L_Y 352
#define T_READ_MAX 4096
#define CDI -1 /* CDI decoding */
#define PADDING 0 /* Padding decoding (11 bits). */


#define get_bit(octet,i,j) \
    (j==7 ? (j=0, ((i)+1)<lmax ? octet[(i)++]&0x01 : (vali=octet[i], \
			 lmax=ReadBuffer(sock, octet, &i), vali&0x01)) \
          : (octet[(i)]>>(7-(j)++))&0x01)


                           /* Global variables */


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


int sock, lmax, vali, fd_pipe;
char host_coder[50];
u_long host_sin_addr;
Display *dpy;
GC gc;
Window win;
XImage *ximage;
Bool does_shm=FALSE;
u_char map_lut[256]; 
XColor colors[256];
int N_LUTH=256; 





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
      addr_coder.sin_port = htons(PORT_VIDEO_CONTROL);
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



BOOLEAN OldPacket(received, expected)
     int received, expected;
{
  register int i;

  for(i=0; i<30; i++)
    if((received=(received==63 ? 0 : received+1)) == expected)
      return(TRUE);
  return(FALSE);
}


int ReadBuffer(sock, buf, i)
    int sock;
    u_char *buf;
    int *i;

  {
    int lr, len_addr, fromlen;
    struct sockaddr_in addr, from;
    int num, type;
    static int number_packet;
    static BOOLEAN FIRST=TRUE;
    int mask, mask0, mask_pipe, fmax;
    register int n;
    u_char memo[256];

    mask_pipe = 1 << fd_pipe;
    mask0 = mask_pipe | (1 << sock);
    fmax=(fd_pipe>sock ? fd_pipe+1 : sock+1);
    
    len_addr = sizeof(addr);
    fromlen = sizeof(from);
    do{
      mask=mask0;
      if(select(fmax, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		(struct timeval *)0) <= 0){
	fprintf(stderr, "VideoDecode, select returned -1\n");
	exit(1);
      }
      if(mask & mask_pipe){
	if((lr=read(fd_pipe, memo, N_LUTH)) != N_LUTH){
	  exit(1);
	}else{
	  for(n=0; n<256; n++)
	    map_lut[n] = (u_char) colors[memo[n]].pixel;
	}
      }
	
      if((lr=recvfrom(sock, (char *)buf, T_MAX, 0, &from, &fromlen)) < 0){
	fprintf(stderr, "VideoDecode: error while receiving\n");
	continue;
      }
      if((type=GetType(buf)) != VIDEO_TYPE){
	fprintf(stderr, "Received and dropped a packet type %d\n", type);
	continue;
      }
      if(from.sin_addr.s_addr != host_sin_addr){
#ifdef DEBUG
	printf("VideoDecode: Received and dropped a video packet\n");
#endif
	continue;
      }
      num = buf[0] & 0x3f;
      if(FIRST){
	if(num != 0){
	  /*
	   * We have to start with an image encoded in full ...
	   */
	  ReportToCoder(0);
	  continue;
	}else{
	  FIRST=FALSE;
	  number_packet=1;
	  *i = 1;
	  break;
	}
      }else{
	if(num != 0){
	  if(OldPacket(num, number_packet)){
#ifdef DEBUG
	    fprintf(stderr, 
		    "VideoDecode: old packet #%d received and dropped\n", num);
#endif
	    /* Throw away this old packet */
	    continue;
	  }else
	    while(num != number_packet){
	      /* Some packets have been lost, reports to the encoder 
	       * and continue decoding with this new packet.
	       */
#ifdef DEBUG
	      fprintf(stderr, "VideoDecode: packet #%d is lost\n", 
		      number_packet);
#endif
	      ReportToCoder(number_packet);
	      number_packet=(number_packet==63 ? 1 : number_packet+1);
	    }
	}
	number_packet=(num==63 ? 1 : num+1);
	*i = 1;
	break;
      }
    }while(1);
    return(lr);
  }




    

VideoDecode(fd, port, group, name, sin_addr, display, QCIF2CIF)
     int fd[2];
     char *port, *group;
     char *name; /* Window's name. */
     u_long sin_addr;
     char *display;
     BOOLEAN QCIF2CIF; /* Boolean : true if CIF display. */
    

{
    extern XImage *ximage;
    static u_char tab_Y[288][352], tab_Cb[144][176], tab_Cr[144][176];
    u_char *im_pix;
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
    struct sockaddr_in addr;
    int len_addr;
    char *pt;

    fd_pipe = fd[0];
    sock=CreateReceiveSocket(&addr, &len_addr, port, group);
    pt = (char *)index(name, '@');
    strcpy(host_coder, (char *) (pt+1));
    host_sin_addr = sin_addr;

    do{
      lmax=ReadBuffer(sock, chaine_codee, &i);
      temp=get_bit(chaine_codee,i,j);
      for(l=1; l<20; l++)
	temp= (temp<<1) + get_bit(chaine_codee,i,j);
      if(temp != 16){
	fprintf(stderr,"decode: Bad First Header Image value\n");
	ReportToCoder(0);
      }
    }while(temp != 16);
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
	  if(CIF){
	    L_col = 352;
	    L_lig = 288;
	    NG_MAX = 12;
	    strcat(name, " CIF");
	  }
	  else{
	    if(QCIF2CIF){
	      DOUBLE=1;
	      strcat(name, " QCIF -> CIF");
	      L_col = 352;
	      L_lig = 288;
	    }else{
	      strcat(name, " QCIF");
	      L_col = 176;
	      L_lig = 144;
	    }
	    NG_MAX = 5;
	  }
	  init_window(L_lig, L_col, name, map_lut, display);
	  im_pix = (u_char *) ximage->data;
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
	   decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, &(tab_Y[y_mb][x_mb]), 
		    cm, L_Y);
	 }
         if(P & 0x10){
	   decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb][x_mb+8]), cm, L_Y);
	 }
         if(P & 0x08){
	   decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb+8][x_mb]), cm, L_Y);
	 }
         if(P & 0x04){
	   decode_bloc_8x8(chaine_codee, block, state_tcoeff, &i, &j, &cm, 0);
	   ifct_8x8(block, tab_quant1, tab_quant2, 0, 
		    &(tab_Y[y_mb+8][x_mb+8]), cm, L_Y);
	 }
        }
       
	{
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
	show(L_col, L_lig);
      }
	
     }while(GO); /* BOUCLE GOB */

     GO=1;
     
   }while(1); /* BOUCLE IMAGE */

  } /* decode() */
