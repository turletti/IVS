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
*  File              : video_decoder1.c                                    *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 video decoder.                                     *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*   Pierre Delamotte    |  93/3/20  | Color display adding.                *
*   Andrzej Wozniak     |  93/10/1  | get_bit() macro changes.             *
\**************************************************************************/

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "linked_list.h"
#include "video_decoder.h"



VideoDecode(fd, port, group, name, sin_addr, sin_addr_coder, identifier,
	    ni, control_port, display, QCIF2CIF, STAT, FORCE_GREY, UNICAST)
     int fd[2];
     char *port, *group;
     char *name; /* Window's name. */
     u_long sin_addr; /* addr from (bridge or coder) */
     u_long sin_addr_coder; /* addr coder */
     u_short identifier; /* set to 0 if no bridge */
     char *ni; /* Network Interface name */
     u_short control_port; /* Feedback port number */
     char *display;
     BOOLEAN QCIF2CIF; /* True if CIF display. */
     BOOLEAN STAT; /* True if STAT mode set */
     BOOLEAN FORCE_GREY; /* True if Gray display only */
     BOOLEAN UNICAST; /* True if unicast mode */

{
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
    register int l;
    struct sockaddr_in addr;
    int len_addr;
    char *pt;
    int NG_EXPECTED=0; /* Next GOB number expected */
    int full_cbc;
    int temp;
    TYPE_dec_bit_state bit_state;
    char file_loss[200];
    int x_mb_max, y_mb_max;

    register  u_char *ptr_chaine_codee;  
    register  u_int byte4_codee;         
    register int rj;                     
    register int chaine_count;           

    bzero((char *)&addr_coder, sizeof(addr_coder));
    addr_coder.sin_addr.s_addr = sin_addr_coder;
    addr_coder.sin_port = control_port;
    addr_coder.sin_family = AF_INET;
    len_addr_coder = sizeof(addr_coder);
    host_sin_addr_from = sin_addr;
    host_identifier = identifier;
    fd_pipe = fd[0];
    sock = CreateReceiveSocket(&addr, &len_addr, port, group, UNICAST, ni);
#ifdef INPUT_FILE
    strcpy(filename, name);
    strcpy(host_coder, filename);
#else
    if((pt = (char *)index(name, '@')) != NULL)
      strcpy(host_coder, (char *) (pt+1));
#endif

    if(STAT){
	char spid[8];

      fprintf(stderr, "STAT_MODE enabled\n");
      strcpy(file_loss, ".ivs_loss_");
      strcat(file_loss, host_coder);
      sprintf(spid, "%d", getpid());
      strcat(file_loss, spid);
      if((F_dloss=fopen(file_loss, "w")) == NULL){
	fprintf(stderr, "cannot create %s file\n", file_loss);
	STAT_MODE=FALSE;
      }else
	STAT_MODE=TRUE;
    }

#ifdef RECVD_ONLY
    FULL_INTRA_REQUEST = FALSE;
    do{
      AFTER_GBSC=TRUE;
      ReadBuffer(sock, &bit_state);
    }while(1);
#endif


    after_gbsc:
    AFTER_GBSC = TRUE;

    rj = 1; chaine_count = 4;
    
    do{
      l = 0;
      do{
	l ++;
      }while(get_bit(ptr_chaine_codee, rj) == 0);
      if(l < 16){
	continue;
      }
#ifdef DEBUG
      if(l>16)
	fprintf(stderr, "Nota ****** GBSC with %d zeroes\n", l-1);
#endif
      NG = 0;
      for(l=0; l<4; l++)
	NG = (NG << 1) + get_bit(ptr_chaine_codee, rj);
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
      static u_char *tab_Y, *tab_Cb, *tab_Cr;
      static int cif_size, qcif_size;

      do{ /* IMAGE LOOP */

	do{ /* GOB LOOP */

	  if(NG == 0){
	    /*************\
	    * IMAGE LAYER *
	    \*************/

	    RT = get_bit(ptr_chaine_codee, rj);
	    for(l=1; l<5; l++)
	      RT = (RT << 1) + get_bit(ptr_chaine_codee, rj);
	
	    PART_ECRAN = get_bit(ptr_chaine_codee, rj);
	    CAMERA_DOC = get_bit(ptr_chaine_codee, rj);
	    GEL_IMAGE = get_bit(ptr_chaine_codee, rj);
	    CIF = get_bit(ptr_chaine_codee, rj);
	    if(FIRST){
	      L_col = 352;
	      L_lig = 288;
	      L_col2 = L_col << 1;
	      L_lig2 = L_lig << 1;
	      NG_MAX = 12;
	      x_mb_max = 336;
	      y_mb_max = 272;
	      strcat(name, " Super CIF");

	      init_window(L_lig2, L_col2, name, display, &dec_appCtxt,
			  &dec_appShell, &dec_depth, &dec_dpy, &dec_win, 
			  &dec_icon, &dec_gc, &ximage, FORCE_GREY);
	      
	      full_cbc = (COLOR_ENCODED ? 63 : 60);
	      cif_size = 101376; /* 352*288 */
	      qcif_size = 25344; /* 176*144 */
	      tab_Y = (u_char *) malloc(4*cif_size*sizeof(char));
	      tab_Cb = (u_char *) malloc(4*qcif_size*sizeof(char));
	      tab_Cr = (u_char *) malloc(4*qcif_size*sizeof(char));
	      Y_pt = (u_char *) tab_Y ;
	      Cb_pt = (u_char *) tab_Cb ;
	      Cr_pt = (u_char *) tab_Cr ;
	      FIRST = FALSE;
	    }
	    temp = get_bit(ptr_chaine_codee, rj); /* reserved */
	    temp = get_bit(ptr_chaine_codee, rj); /* reserved */
	    while (get_bit(ptr_chaine_codee, rj))
	      /* RESERVEI option unused */
	      for(l=0; l<8; l++)
		temp = get_bit(ptr_chaine_codee, rj);
	    temp = get_bit(ptr_chaine_codee, rj);
	    l = 0;
	    do{
	      l ++;
	    }while(get_bit(ptr_chaine_codee, rj) == 0);
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
	    NG = get_bit(ptr_chaine_codee, rj);
	    for(l=1; l<4; l++)
	      NG = (NG << 1) + get_bit(ptr_chaine_codee, rj);
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

	  quant = get_bit(ptr_chaine_codee, rj);
	  for(l=1; l<5; l++)
	    quant = (quant<<1) + get_bit(ptr_chaine_codee, rj);
	  tab_quant1 = tab_rec1[quant-1];
	  tab_quant2 = tab_rec2[quant-1];
	  while (get_bit(ptr_chaine_codee, rj))
	    /* RESERVEG option unused */
	    for(l=0; l<8; l++)
	      temp = get_bit(ptr_chaine_codee, rj); 

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
		new_bit = get_bit(ptr_chaine_codee, rj);
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
		    while(get_bit(ptr_chaine_codee, rj) == 0);
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
	      new_bit = get_bit(ptr_chaine_codee, rj);
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
	      quant = get_bit(ptr_chaine_codee, rj);
	      for(l=1; l<5; l++)
	        quant = (quant<<1) + get_bit(ptr_chaine_codee, rj);
	      tab_quant1 = tab_rec1[quant-1];
	      tab_quant2 = tab_rec2[quant-1];
	    }
	    if(CBC){
	      /* Looking for CBC value */
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(ptr_chaine_codee, rj);
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
	      /* block[][cm+1] = 0 */
	      /* Block decoding in the order :
	      *  Y1, Y2, Y3, and Y4. If COLOR_ENCODED, Cb and Cr are encoded.
	      */
	      int cm;

	      PUT_bit_state(bit_state, ptr_chaine_codee,
			    byte4_codee, chaine_count, rj); 


	      if(P & 0x20){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+cif_number*cif_size+352*y_mb+x_mb), cm, L_Y);
	      }
	      if(P & 0x10){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+cif_number*cif_size+352*y_mb+x_mb+8), cm, L_Y);
	      }
	      if(P & 0x08){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+cif_number*cif_size+352*(y_mb+8)+x_mb), cm,
			 L_Y);
	      }
	      if(P & 0x04){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+cif_number*cif_size+352*(y_mb+8)+x_mb+8), cm,
			 L_Y);
	      }
	      if(COLOR_ENCODED){
		if(P & 0x02){
		  
		  if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				     CBC)){
		    goto after_gbsc;
		  }  
		  if(affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			    (tab_Cb+cif_number*qcif_size+176*y_mb2+x_mb2), cm,
			     L_Y_DIV2);
		}
		if(P & 0x01){
		  
		  if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				     CBC)){
		    goto after_gbsc;
		  }    
		  if(affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			    (tab_Cr+cif_number*qcif_size+176*y_mb2+x_mb2), cm,
			     L_Y_DIV2);
		}
	      }
	      GET_bit_state(bit_state, ptr_chaine_codee, byte4_codee,
			    chaine_count, rj); 
	    }


	    
            build_image((u_char *) (tab_Y+cif_number*cif_size+352*y_mb+x_mb), 
                        (u_char *)
			(tab_Cb+cif_number*qcif_size+176*y_mb2+x_mb2), 
                        (u_char *)
			(tab_Cr+cif_number*qcif_size+176*y_mb2+x_mb2), 
                        FALSE, ximage, L_Y, 
			x_mb + y_mb*ximage->width +
			L_col*(cif_number & 1) + 
			L_col2*(ximage->height>>1)*(cif_number > 1), 
                        16, 16);

	  }while(1); /* MB LOOP */

	  NG_EXPECTED = (NG==NG_MAX ? 0 : NG+1);
	  temp = get_bit(ptr_chaine_codee, rj);
	  for(l=1; l<4; l++)
	  temp = (temp << 1) + get_bit(ptr_chaine_codee, rj);
	  if(temp == 0){
	    /* It's the next CIF image's header */
	    GO = FALSE;
	  }
	  if(temp != NG_EXPECTED && NACK_FIR_ALLOWED){
#ifdef DEBUG
	    fprintf(stderr, 
               "CIF:%d, NG expected: %d, NG received: %d, oldt: %d, newt:%d\n",
		    cif_number, NG_EXPECTED, temp, old_ts_usec, ts_usec);
#endif
	    if(!PACKET_LOST){
#ifdef DEBUG
	      fprintf(stderr, "Some GOBs are missing without packet loss..\n");
#endif
	    }else{
	      PACKET_LOST = FALSE;
	      if(format == old_format){
		SendNACK(&addr_coder, len_addr_coder, TRUE,
			 NG_EXPECTED, temp-1, format, ts_sec, ts_usec);
	      }else{
		if(temp == 0){
		  NG_EXPECTED = (NG_EXPECTED==0 ? 1 : NG_EXPECTED);
		  SendNACK(&addr_coder, len_addr_coder, TRUE,
			   NG_EXPECTED, NG_MAX, old_format, old_ts_sec,
			   old_ts_usec);
		}else{
		  if(NG_EXPECTED != 0)
		    SendNACK(&addr_coder, len_addr_coder, FALSE,
			     NG_EXPECTED, NG_MAX, old_format, old_ts_sec,
			     old_ts_usec);
		  SendNACK(&addr_coder, len_addr_coder, TRUE, 1,
			   temp-1, format, ts_sec, ts_usec);
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
      static u_char *tab_Y, *tab_Cb, *tab_Cr;
      static u_char *memo_Y, *memo_Cb, *memo_Cr;
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

	    if(last_RT_displayed != RT){
	      /* We didn't display the latest picture. This
	      * may happen when several images are sent inside
	      * same packet (e.g. GPT H.261 packetization).
	      */
#ifndef SAVE_FILE	      
	      show(dec_dpy, dec_win, ximage);
	      nb_frame ++;
	      last_RT_displayed = RT;
#endif
	    }
	    RT = get_bit(ptr_chaine_codee, rj);
	    for(l=1; l<5; l++)
	      RT = (RT << 1) + get_bit(ptr_chaine_codee, rj);

	    PART_ECRAN = get_bit(ptr_chaine_codee, rj);
	    CAMERA_DOC = get_bit(ptr_chaine_codee, rj);
	    GEL_IMAGE = get_bit(ptr_chaine_codee, rj);
	    CIF = get_bit(ptr_chaine_codee, rj);
#ifdef SAVE_FILE
	    CIF_MODE = CIF;
#endif	    
	    if(TRYING_RESYNC)
	      goto after_gbsc;
	    if(!FIRST && (LAST_CIF != CIF))
	      NEW_FORMAT = TRUE;
	    if(FIRST || NEW_FORMAT){
	      if(CIF){
		size = CIF_SIZE;
		L_col = 352;
		L_lig = 288;
		NG_MAX = 12;
		strcat(name, " CIF");
		INC_NG = 1;
		x_mb_max = 336;
		y_mb_max = 272;
	      }
	      else{
		size = QCIF_SIZE;
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
		x_mb_max = 160;
		y_mb_max = 128;
	      }
	      if(NEW_FORMAT){
		/* New format detected */
#ifdef DEBUG
		fprintf(stderr, "new format detected\n");
#endif
		kill_window(dec_dpy, ximage);
		if(!FULL_INTRA){
		  /* Waiting for Full INTRA image */
		  FULL_INTRA_REQUEST = TRUE;
		  ReadBuffer(sock, (TYPE_dec_bit_state *)0);
		  rj = 1; chaine_count = 4;
		}
	      }
	      init_window(L_lig, L_col, name, display, &dec_appCtxt,
			  &dec_appShell, &dec_depth, &dec_dpy, &dec_win,
			  &dec_icon, &dec_gc, &ximage, FORCE_GREY);

              if (affCOLOR & MODNB) DOUBLE = TRUE ;
#ifdef GREY_NO_STANDARD	      
	      full_cbc = (COLOR_ENCODED ? 63 : 60);
#else
	      full_cbc = 63;
#endif	      
#ifdef SAVE_FILE
	      dim_image = L_col*L_lig;
	      if((F_decoded_file = fopen("decoded_file", "w")) == NULL){
		perror("fopen decoded_file");
	      }
	      if((F_time = fopen("time_file", "w")) == NULL){
		perror("fopen time_file");
	      }
	      if((F_GOB = fopen("GOB_file", "w")) == NULL){
		perror("fopen GOB_file");
	      }
	      GOB_lost = 0;
	      cpt_image = 1;
	      gettimeofday(&realtime, (struct timezone *)NULL);
	      oldtime = realtime.tv_sec +realtime.tv_usec/1000000.0;
#endif
	      if(!NEW_FORMAT){
		tab_Y = (u_char *) malloc(352*288*sizeof(char));
		tab_Cb = (u_char *) malloc(176*144*sizeof(char));
		tab_Cr = (u_char *) malloc(176*144*sizeof(char));
		memo_Y = (u_char *) malloc(352*288*sizeof(char));
		memo_Cb = (u_char *) malloc(176*144*sizeof(char));
		memo_Cr = (u_char *) malloc(176*144*sizeof(char));
		Y_pt = (u_char *) tab_Y ;
		Cb_pt = (u_char *) tab_Cb ;
		Cr_pt = (u_char *) tab_Cr ;
	      }
	      FIRST = FALSE;

	    }else{
	      if(MV_ENCODED){
		bcopy((char *)tab_Y, (char *)memo_Y, 101376);
		if(COLOR_ENCODED){
		  bcopy((char *)tab_Cb, (char *)memo_Cb, 25344);
		  bcopy((char *)tab_Cr, (char *)memo_Cr, 25344); 
		}
	      }
	    }
	    temp = get_bit(ptr_chaine_codee, rj); /* reserved */
	    temp = get_bit(ptr_chaine_codee, rj); /* reserved */
	    while (get_bit(ptr_chaine_codee, rj))
	      /* RESERVEI option unused */
	      for(l=0; l<8; l++)
		temp = get_bit(ptr_chaine_codee, rj);
	    temp = get_bit(ptr_chaine_codee, rj);
	    l = 0;
	    do{
	      l ++;
	    }while(get_bit(ptr_chaine_codee, rj) == 0);
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
	    NG = get_bit(ptr_chaine_codee, rj);
	    for(l=1; l<4; l++)
	      NG = (NG << 1) + get_bit(ptr_chaine_codee, rj);
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

	  quant = get_bit(ptr_chaine_codee, rj);
	  for(l=1; l<5; l++)
	    quant = (quant<<1) + get_bit(ptr_chaine_codee, rj);
	  tab_quant1 = tab_rec1[quant-1];
	  tab_quant2 = tab_rec2[quant-1];
	  while (get_bit(ptr_chaine_codee, rj))
	    /* RESERVEG option unused */
	    for(l=0; l<8; l++)
	      temp = get_bit(ptr_chaine_codee, rj); 

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
		new_bit = get_bit(ptr_chaine_codee, rj);
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
		    while(get_bit(ptr_chaine_codee, rj) == 0);
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

	    if((x_mb > x_mb_max) || (y_mb > y_mb_max)){
#ifdef DEBUG
	      fprintf(stderr,
		      "Bit error detected while decoding x_mb, y_mb\n");
	      fprintf(stderr, "x_mb: %d, y_mb: %d\n", x_mb, y_mb);
#endif	      
	      goto after_gbsc;
	    }
	    x_mb2 = x_mb >> 1;
	    y_mb2 = y_mb >> 1;

	    /* Looking for TYPEM value */
	    new_x = 0;
	    do{
	      x = new_x;
	      new_bit = get_bit(ptr_chaine_codee, rj);
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
	      quant = get_bit(ptr_chaine_codee, rj);
	      for(l=1; l<5; l++)
	        quant = (quant<<1) + get_bit(ptr_chaine_codee, rj);
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
		  new_bit = get_bit(ptr_chaine_codee, rj);
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
		  new_bit = get_bit(ptr_chaine_codee, rj);
		  new_x = state_dvm[x].bit[new_bit];
		}while(new_x > 0);
		result = state_dvm[x].result[new_bit];
		if((result < -16) || (result > 15)){
#ifdef DEBUG
		  fprintf(stderr, "Bit error detected while decoding VDVM\n");
#endif
		  goto after_gbsc;
		}
		if(result > 1)
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
		    bcopy((char *)(memo_Y+352*(y+dvm_verti)+xx), 
			  (char *)(tab_Y+352*y+x_mb), 16);
		  if(COLOR_ENCODED){
		    dvm_C_verti = dvm_verti / 2;
		    aux = y_mb2 + 8;
		    for(y=y_mb2; y<aux; y++)
		      bcopy((char *)(memo_Cb+176*(y+dvm_C_verti)+xx2), 
			    (char *)(tab_Cb+176*y+x_mb2), 8);
		    for(y=y_mb2; y<aux; y++)
		      bcopy((char *)(memo_Cr+176*(y+dvm_C_verti)+xx2), 
			    (char *)(tab_Cr+176*y+x_mb2), 8);
		  }
		}

		if(FIL){
		  /*
		  *  Loop-filter applied on the macroblock.
		  */
		  filter_loop((memo_Y+352*yy+xx), (tab_Y+352*y_mb+x_mb), L_Y);
 		  filter_loop((memo_Y+352*yy+xx+8), (tab_Y+352*y_mb+x_mb+8),
			      L_Y);
		  filter_loop((memo_Y+352*(yy+8)+xx),
			      (tab_Y+352*(y_mb+8)+x_mb), L_Y);
		  filter_loop((memo_Y+352*(yy+8)+xx+8),
			      (tab_Y+352*(y_mb+8)+x_mb+8), L_Y);
		  if(COLOR_ENCODED){
		    filter_loop((memo_Cb+176*yy2+xx2),
				(tab_Cb+176*y_mb2+x_mb2), L_Y/2);
		    filter_loop((memo_Cr+176*yy2+xx2),
				(tab_Cr+176*y_mb2+x_mb2), L_Y/2);
		  }
		}
	      } /* fi DVM */
	    } /* fi MV_ENCODED */

	    if(CBC){
	      /* Looking for CBC value */
	      new_x = 0;
	      do{
		x = new_x;
		new_bit = get_bit(ptr_chaine_codee, rj);
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
	      PUT_bit_state(bit_state, ptr_chaine_codee, byte4_codee,
			    chaine_count, rj); 

	      if(P & 0x20){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+352*y_mb+x_mb), cm, L_Y);
	      }
	      if(P & 0x10){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+352*y_mb+x_mb+8), cm, L_Y);
	      }
	      if(P & 0x08){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+352*(y_mb+8)+x_mb), cm, L_Y);
	      }
	      if(P & 0x04){
		
		if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				    CBC)){
		  goto after_gbsc;
		}
		ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			 (tab_Y+352*(y_mb+8)+x_mb+8), cm, L_Y);
	      }
#ifdef GREY_NO_STANDARD	      
	      if(COLOR_ENCODED){
#endif		
		if(P & 0x02){
		  
		  if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				     CBC)){
		    goto after_gbsc;
		  }
	          if (affCOLOR & MODCOLOR)
                    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			   (tab_Cb+176*y_mb2+x_mb2), cm, L_Y_DIV2);
		}
		if(P & 0x01){
		  
		  if(!decode_bloc_8x8(&bit_state, block, state_tcoeff, &cm,
				     CBC)){
		    goto after_gbsc;
		  }
	          if (affCOLOR & MODCOLOR)
		    ifct_8x8(block, tab_quant1, tab_quant2, CBC, 
			   (tab_Cr+176*y_mb2+x_mb2), cm, L_Y_DIV2);
		}
#ifdef GREY_NO_STANDARD		
	      }
#endif	      
	      GET_bit_state(bit_state, ptr_chaine_codee, byte4_codee,
			    chaine_count, rj); 
	    }

            build_image((u_char *) (tab_Y+352*y_mb+x_mb), 
                        (u_char *) (tab_Cb+176*y_mb2+x_mb2), 
                        (u_char *) (tab_Cr+176*y_mb2+x_mb2), 
                        DOUBLE, ximage, 
                        L_Y, y_mb*ximage->width + x_mb, 16, 16 ) ;


	  }while(1); /* MB LOOP */

	  NG_EXPECTED = (NG==NG_MAX ? 0 : NG+INC_NG);
	  temp = get_bit(ptr_chaine_codee, rj);
	  for(l=1; l<4; l++)
	  temp = (temp << 1) + get_bit(ptr_chaine_codee, rj);
	  if(temp == 0){
	    /* It's the next image's header */
#ifdef INPUT_FILE
	    show(dec_dpy, dec_win, ximage);
	    nb_frame ++;
#endif
	    GO = FALSE;
	  }
	  if(temp != NG_EXPECTED && NACK_FIR_ALLOWED){
#ifdef DEBUG
	    fprintf(stderr, 
		    "NG expected: %d, NG received: %d, oldt: %d, newt:%d\n",
		    NG_EXPECTED, temp, old_ts_usec, ts_usec);
#endif
	    if(!PACKET_LOST){
#ifdef DEBUG
	      fprintf(stderr,
		      "Some GOBs are missing without packet loss..\n");
#endif
	    }else{
	      PACKET_LOST = FALSE;
	      if(equal_time(old_ts_sec, old_ts_usec, ts_sec, ts_usec)){
		SendNACK(&addr_coder, len_addr_coder, TRUE,
			 NG_EXPECTED, temp-INC_NG, format, ts_sec,
			 ts_usec);
#ifdef  SAVE_FILE		
		{
		  int n;
		  for(n=NG_EXPECTED; n<temp; n+=INC_NG)
		    GOB_lost ++;
		}
#endif		  
	      }else{
		if(temp == 0){
		  NG_EXPECTED = (NG_EXPECTED==0 ? 1 : NG_EXPECTED);
		  SendNACK(&addr_coder, len_addr_coder, TRUE,
			   NG_EXPECTED, NG_MAX, old_format, old_ts_sec,
			   old_ts_usec);
#ifdef SAVE_FILE		  
		  {
		    int n;
		    for(n=NG_EXPECTED; n<=NG_MAX; n+=INC_NG)
		    GOB_lost ++;
		  }
#endif		  
		}else{
		  if(NG_EXPECTED != 0)
		    SendNACK(&addr_coder, len_addr_coder, FALSE,
			     NG_EXPECTED, NG_MAX, old_format, old_ts_sec,
			     old_ts_usec);
		  SendNACK(&addr_coder, len_addr_coder, TRUE, 1,
			   temp-INC_NG, format, ts_sec, ts_usec);
#ifdef SAVE_FILE
		  {
		    int n;
		    for(n=1; n<temp; n+=INC_NG)
		      GOB_lost ++;
		  }
#endif		  
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






