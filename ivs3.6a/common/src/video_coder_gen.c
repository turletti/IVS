/**************************************************************************
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
*  File              : video_coder_gen.c                                   *
*  Author            : Thierry Turletti                                    *
*  Last modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 video coder.                                       *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*  Winston Dang         | 92/12/8   | Some alterations made for speed      *
*  wkd@Hawaii.Edu       |           | purposes.                            *
*--------------------------------------------------------------------------*
*  Andrzej Wozniak      | 93/12/1   | Merge encode_h261_cif4 and           *
*  wozniak@inria.fr     |           | encode_h261 in a same function.      *
*                       |           | Replace put_bit() and put_value()    *
*                       |           | macros by Mput_value_ptr(),          *
*                       |           | Mput_bit_ptr() macros. Split some    *
*                       |           | functions in video_coder3.c          *
\**************************************************************************/

#include <memory.h>

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "idx_huffman_coder.h"
#include "mc_header.h"
#include "video_coder.h"

#ifdef CRYPTO
#include "ivs_secu.h"
#endif /* CRYPTO */


#if defined(__sgi) || defined(__STDC__)
static int  put_value(unsigned char *, int, int, int *, int *);
#endif

#ifndef SPARCSTATION
static usleep(usec_wait)
     int usec_wait;
{
  struct timeval wait_tv;

  wait_tv.tv_sec = usec_wait / 1000000;
  wait_tv.tv_usec = usec_wait % 1000000;
  select(0, NULL, NULL, NULL, &wait_tv);
}

#endif

#ifdef ENCODE_CIF4
/* The maximum rate the coder should use */
int coder_rate_max;
double actual_rate;


mc_incfn()
{
#define INCREMENT_MC 10  
  int new_rate_max = (int) (coder_rate_max + INCREMENT_MC);

  coder_rate_max = (new_rate_max > max_rate_max) ? max_rate_max : new_rate_max;
  if(DEBUG_MODE && coder_rate_max == new_rate_max)
    fprintf(stderr, "Max rate increased to %d\n", coder_rate_max);
}

mc_decfn(percentCongested)
     int percentCongested; 
{
  int half;
  /* 
   * if too few are congested, then return
   */
  if(percentCongested < MC_PERCENTCONGESTEDTHRESH)
    return;
  
  /* 
   * Reduce by a half if we have to
   */
  half = coder_rate_max/2;

  coder_rate_max = (half > min_rate_max ? half : min_rate_max);
  if(DEBUG_MODE) fprintf(stderr, "Max rate reduced to %d\n", coder_rate_max);
}
     

void encode_h261_cif4(param, new_Y, new_Cb, new_Cr, image_coeff, cif_number)
     S_PARAM *param;
     u_char *new_Y, *new_Cb, *new_Cr;
     IMAGE_CIF image_coeff;
     int cif_number;      /* select the CIF number in the CIF4 (0,1,2 or 3) */
#else
     
     extern int coder_rate_max;
     extern double actual_rate;
     
     void mc_incfn();
void mc_decfn();

void encode_h261(param, new_Y, new_Cb, new_Cr, image_coeff, NG_MAX)
     S_PARAM *param;
     u_char *new_Y, *new_Cb, *new_Cr;
     IMAGE_CIF image_coeff;
     int NG_MAX;       /* Maximum NG number */
#endif
     
{
  
#ifdef ENCODE_CIF4
    static int NG_MAX=12;
    int off_cif_number;
#else    
#define cif_number 0
    int INC_NG, CIF, k;
    static int nblock_qcif_max; 
#endif
    
    int sock=param->sock;
    BOOLEAN COLOR=param->COLOR;
    BOOLEAN PRIVILEGE_QUALITY=param->PRIVILEGE_QUALITY;
    int conf_size=param->conf_size;
    int nb_stations=param->nb_stations;
    int rate_control=param->rate_control;
    int default_quantizer=param->default_quantizer;
    int fd = param->s2f_fd[1];
    
    int lig, col, ligdiv2, coldiv2;
    int NG, MBA, Nblock;
    GOB *gob;
    MACRO_BLOCK *mb;
    int quant_mb;
    int bnul=0, mbnul=0;
    int cmax[6];
    int *tab_quant1, *tab_quant2;
    static int lwrite;
    struct timeval realtime;
    static double inittime, synctime, last_displaytime, first_displaytime;
    int relative_time;
    static int RT;
    static int nb_frame=1;
    int max_coeff, max; 
    static int max_inter; /* max number of successive INTER mode encoding */
    static int cpt=0;
    BOOLEAN GobEncoded[12];
    static int nblock_cif_max;  
    BOOLEAN VIDEO_PACKET_HEADER; /* True if it is set */
    static int BperMB, full_cbc;
    static int len_addr=sizeof(param->addr);
    static int max_value; /* A block is forced every 30 images */
    static int threshold=DEFAULT_THRESHOLD; /* For image change estimation */
    static int quantizer;
    static int length_image=0;
    static double lasttime, fps, bandwidth;
    char data[LEN_DATA];
    static u_short sequence_number=0;
    static BOOLEAN FIRST=TRUE; /* True if it the first image */
    BOOLEAN FIRST_GOB_IS_ENCODED=TRUE;
    static BOOLEAN FULL_INTRA=TRUE;
    BOOLEAN NACK_FIR_ALLOWED=(conf_size == SMALL_SIZE);
    register u_char *ptr_chaine_codee; 
    register u_int byte4_codee; 
    register int jj;
    TYPE_bit_state bit_state;
    rtp_hdr_t *h; /* fixed RTP header */
    rtp_t *r; /* RTP option, here fixed H.261 RTP header */
    static int old_nb_stations;
    BOOLEAN USING_MC;
    struct timeval current_time;
    long elapsed_usec, min_dt, av_dt;
    static struct timeval time_mark;
#ifdef MICE_VERSION
    extern int logprefix;
#endif
    static FILE *file_ratemax, *file_fps;
#ifdef CRYPTO
    u_int buf_crypt[T_MAX/4 + 2];
#endif /* CRYPTO */    
    int len_packet, memo_len_packet;
    char *buf_to_send;
    
#define offsetY (COL_MAX - 8)
#define offsetC (COL_MAX_DIV2 - 8)    

#ifdef DEBUG_ARG
    printf("PRIV_QUAL=%d, rate_control:%d\n", PRIVILEGE_QUALITY, rate_control);
    printf("conf_size = %d, USING_MC=%d\n", conf_size, USING_MC);
#endif
    min_rate_max = param->rate_min;
    max_rate_max = param->rate_max;

    USING_MC = (rate_control == AUTOMATIC_CONTROL);
    
/*--------------- max_inter AND max_value INITIALIZATIONS -------------------*/

    if(FIRST){
      old_nb_stations = param->nb_stations;
      if(USING_MC){
	/* Initialise the structure for the MC congestion control */
	/* Note modified probeinit that assumes we just fill in structures */
	mcTxState = (Mc_TxState *) mc_probeinit(mc_incfn, mc_decfn);
      }
      gettimeofday(&time_mark, (struct timezone *)NULL);
      time_mark.tv_sec -= 1; /* do not wait first */
    }
    
    if(old_nb_stations != nb_stations){
      if(DEBUG_MODE)
	fprintf(stderr, "N stations = %d\n", nb_stations);
      old_nb_stations = nb_stations;
    }
    {
      int max, vmin, vmax;
      static int oldmax_inter=0;
      static char *STATEOF[]= {
	  "UNLOADED ", " LOADED  ", "CONGESTED"
      };

      network_state = (coder_rate_max == min_rate_max ? CONGESTED :
		       (coder_rate_max == max_rate_max ? UNLOADED : LOADED));

#ifdef ENCODE_CIF4
      off_cif_number = 13 * cif_number;
      vmin = 10;
      vmax = 30;
#else
      CIF = (NG_MAX != 5);
      if(CIF){
	vmin = 20;
	vmax = 50;
	INC_NG = 1;
      }else{
	vmin = 30;
	vmax = 100;
	INC_NG = 2;
      }
#endif
      max = NACK_FIR_ALLOWED ? 10 : 3;
      if(USING_MC){
	if(network_state == UNLOADED) /* if UNLOADED */
	  max_inter = max;
	else{
	  if(network_state == CONGESTED) /* if CONGESTIONNED */
	    max_inter = 1;
	  else{
	    /* linear variation between 1 and max */
	    max_inter = (max + 2)/2;
	  }
	}
	if(network_state == UNLOADED) /* if UNLOADED */
	  max_value = vmax;
	else{
	  if(network_state == CONGESTED) /* if CONGESTIONNED */
	    max_value = vmin;
	  else{
	    /* linear variation between vmin and vmax */
	    max_value = (vmax + vmin)/2;
	  }
	}
      }else{
	max_inter = max;
	max_value = vmax;
      }
#ifdef DEBUG_MODE
      if(oldmax_inter != max_inter){
	if(DEBUG_MODE){
	  fprintf(stderr, "STATE %s, switching max_inter to %d\n", 
		  STATEOF[network_state], max_inter);
	}
	oldmax_inter = max_inter;
      }
#endif
    }
/*-------------------------- Frame rate limitation --------------------------*/
    if(param->MAX_FR_RATE_TUNED){
      min_dt = 1000000 / param->max_frame_rate;
      gettimeofday(&current_time, (struct timezone *)NULL);
      elapsed_usec = (current_time.tv_sec - time_mark.tv_sec)*1000000
	+ (current_time.tv_usec-time_mark.tv_usec);
      av_dt = min_dt - elapsed_usec;
      if(av_dt > 0){
	usleep((unsigned)av_dt);
	gettimeofday(&time_mark, (struct timezone *)NULL);
      }else{
	time_mark.tv_sec = current_time.tv_sec;
	time_mark.tv_usec = current_time.tv_usec;
      }
    }  
/*---------------------------------------------------------------------------*/
    if(rate_control == MANUAL_CONTROL)
      coder_rate_max = max_rate_max;

    /* Set the fixed RTP header */
    h = (rtp_hdr_t *) chaine_codee;
    h->ver = 1;
    h->channel = 0;

    h->seq = htons(sequence_number);

    sequence_number = (int)(sequence_number + 1) % 65536;
    gettimeofday(&realtime, (struct timezone *)NULL);
    h->ts_sec = htons(realtime.tv_sec & 0xffff);
    h->ts_usec = htons((realtime.tv_usec >> 4) & 0xffff);

    if(USING_MC){
      /* Reserve space for the mc header */
      r = (rtp_t *) &chaine_codee[sizeof(rtp_hdr_t) + sizeof(app_mcct_t)];
    }else{
      /* Use the inria original */
      r = (rtp_t *) &chaine_codee[8];
    }
    r->h261.S = r->h261.E = 1; /* By default, E = S = 1 */
    r->h261.Sbit = 0; /* always null */
    r->h261.C = COLOR;
#ifdef ENCODE_CIF4    
    r->h261.I = cif_number ? 0 : FULL_INTRA;
#else
    r->h261.I = FULL_INTRA;
#endif    
    r->h261.V = 0;
    r->h261.F = NACK_FIR_ALLOWED;
    r->h261.Q = TRUE;
#ifdef ENCODE_CIF4
    r->h261.size = 0x04 | cif_number;
#else
    r->h261.size = CIF ? 1 : 0;
#endif    

    timestamps[cpt%256].sec = (u_short)(realtime.tv_sec & 0xffff);
    timestamps[cpt%256].usec = (u_short)(realtime.tv_usec >> 4);

    if(USING_MC){
      chaine_codee[MCDATA_START] = 0x00;
      cPUT_ij(ptr_chaine_codee, jj, MCDATA_START, 7);
    }else{
      chaine_codee[10] = 0x00;
      cPUT_ij(ptr_chaine_codee, jj, 10, 7);
    }

    VIDEO_PACKET_HEADER = TRUE;
  
    /*************\
    * IMAGE LAYER *
    \*************/

    if(FIRST){
	extern BOOLEAN RATE_STAT;

      /* Some initializations ... */
      inittime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      RT = 0;
      bzero((char *)image_coeff, sizeof(IMAGE_CIF));
      coder_rate_max = max_rate_max;
      signal(SIGUSR1, ToggleCoderDebug);
      signal(SIGUSR2, ToggleCoderStat);

#ifdef GETSTAT_MODE
      if(RATE_STAT){
	char spid[8];
	char host[100];
	char filename[150];

	if(gethostname(host, 100) != 0){
	    if(DEBUG_MODE)
		fprintf(stderr, "Cannot get local hostname\n");
	    strcpy(host, "local");
	}
	sprintf(spid, "%d", getpid());

	strcpy(filename, ".ivs_rate_");
	strcat(filename, host);
	strcat(filename, ".");
	strcat(filename, spid);
	if((file_rate=fopen(filename, "w")) == NULL){
	  if(DEBUG_MODE){
	    fprintf(stderr, "cannot create %s file\n", filename);
	  }
	}else{
	  LOG_FILES_OPENED = TRUE;
	}
	if(LOG_FILES_OPENED){
	  strcpy(filename, ".ivs_rate_max_");
	  strcat(filename, host);
	  strcat(filename, ".");
	  strcat(filename, spid);
	  if((file_ratemax=fopen(filename, "w")) == NULL){
	    if(DEBUG_MODE){
	      fprintf(stderr, "cannot create %s file\n", filename);
	    }
	    LOG_FILES_OPENED = FALSE;
	  }
	}
	if(LOG_FILES_OPENED){
	  strcpy(filename, ".ivs_fps_");
	  strcat(filename, host);
	  strcat(filename, ".");
	  strcat(filename, spid);
	  if((file_fps=fopen(filename, "w")) == NULL){
	    if(DEBUG_MODE){
	      fprintf(stderr, "cannot create %s file\n", filename);
	    }
	    LOG_FILES_OPENED = FALSE;
	  }
	}
      }
#endif /* GETSTAT_MODE */
      {
	int nb_cif;

#ifdef ENCODE_CIF4
	nb_cif = 4;
#else
	nb_cif = 1;
#endif	
	if((old_Y = (u_char *) malloc(sizeof(char) *nb_cif * LIG_MAX * 
				      COL_MAX)) == NULL) {
	  perror("h261_encode'malloc");
	  exit(1);
	}
	if((old_Cb = (u_char *) malloc(sizeof(char)* nb_cif * LIG_MAX_DIV2 
				       * COL_MAX_DIV2)) == NULL){
	  perror("h261_encode'malloc");
	  exit(1);
	}
	if((old_Cr = (u_char *) malloc(sizeof(char)*nb_cif * LIG_MAX_DIV2 * 
				       COL_MAX_DIV2)) == NULL){
	  perror("h261_encode'malloc");
	  exit(1);
	}
	
	if((diff_Y = (int *) malloc(sizeof(int) * nb_cif * LIG_MAX * COL_MAX))
	   == NULL){
	  perror("h261_encode'malloc");
	  exit(1);
	}
	if((diff_Cb = (int *) malloc(sizeof(int)*nb_cif*LIG_MAX_DIV2*
				      COL_MAX_DIV2)) == NULL){
	  perror("h261_encode'malloc");
	  exit(1);
	}
	if((diff_Cr = (int *) malloc(sizeof(int)*nb_cif*LIG_MAX_DIV2*
				      COL_MAX_DIV2)) == NULL){
	  perror("h261_encode'malloc");
	  exit(1);
	}
      }
#ifdef ENCODE_CIF4
      if(cif_number == 0){
	lasttime = 1000000.0*realtime.tv_sec + realtime.tv_usec;
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
#else
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

#endif
      FIRST = FALSE; 
    }else{ /* NOT FIRST */
      
#ifdef ENCODE_CIF4
      if(cif_number == 0){
#endif
	synctime = realtime.tv_sec + realtime.tv_usec/1000000.0;
	{
	  static BOOLEAN FIRST=TRUE;
	  if(FIRST){
	    first_displaytime = last_displaytime = synctime;
	    FIRST=FALSE;
	  }
	}
	RT = ((RT + (int)(29.97*(synctime-inittime))) & 0x1f); 
	actual_rate = ((double)(0.008*length_image) / (synctime-inittime));

	if(PRIVILEGE_QUALITY && (rate_control != WITHOUT_CONTROL)){
	  quantizer = LOW_QUANTIZER;
	  threshold = LOW_THRESHOLD;
	}else{
/*-------- CONTROL OUTPUT RATE: QUANTIZER AND THRESHOLD ARE ADAPTED ---------*/
	  if(rate_control == WITHOUT_CONTROL){
	    quantizer = default_quantizer;
	    threshold = DEFAULT_THRESHOLD;
	  }else{
	    static double mem_rate;
	    static int step=1;
	    int delta;
	    int min_rate;
	    static int cpt_min=0;

	    min_rate = (int)(0.7*coder_rate_max); /* Hysteresis 30% */

	    /* Crude average over the last two frames */
	    if(cpt % 2 == 1){
	      actual_rate = (mem_rate + actual_rate) / 2.0;
	      mem_rate = 0.0;
	    }else{
	      mem_rate = actual_rate;
	    }
	    
	    if(actual_rate < min_rate){
	      cpt_min ++;
	      if(cpt_min > 3){
		delta = 1;
		cpt_min = 0;
		step = (step-delta >=0 ? step-delta : 0);
	      }
	    }else{
	      if(actual_rate > coder_rate_max){
		delta = ((actual_rate - coder_rate_max)/10 + 1);
		step = (step+delta <= step_max ? step+delta : step_max);
	      }
	      cpt_min = 0;
	    }
	    quantizer = Step[step].quant;
	    threshold = Step[step].threshold;
/*---------------------------------------------------------------------------*/
	  } /* rate_control <> WITHOUT_CONTROL */
	} /* ! PRIVILEGE_QUALITY */

/* Should add messages that encode congestions state and receiver estimates */
#ifdef ENCODE_CIF4
	if(cpt > 4){
	  fps = (1.0 / (synctime-inittime));
	  bandwidth = ((double)(0.008*length_image) / (synctime-inittime));
	  bzero(data, LEN_DATA);
	  sprintf(data, "%3.1f", bandwidth);
	  sprintf(&data[6], "%3.1f", fps);
	  sprintf(&data[12], "%3d", coder_rate_max);
	  if((lwrite=write(fd, data, LEN_DATA)) != LEN_DATA){
	    if(DEBUG_MODE)
	      perror("encode_h261'sending bw/fps/coder_rate_max");
	  }
	} 
	length_image = 0;
	inittime = synctime;
      } /* cif_number == 0 */
      Nblock = 0;
    } /* ! FIRST */
#else
      fps += (1.0 / (synctime-inittime));
      bandwidth += ((double)(0.008*length_image) / (synctime-inittime));
#ifdef RATE_DUMP
      fprintf(stderr, "%d\t%f\n", cpt, 
	      (double)(0.008*length_image) / (synctime-inittime));
#endif
#ifdef GETSTAT_MODE
      if(synctime - last_displaytime > 3 ) {
	relative_time = (int)rint(synctime - first_displaytime);
        printf("time : %5d s\n", relative_time);
#else
      if(synctime - last_displaytime > 0.5){
#endif
	bzero(data, LEN_DATA);
	sprintf(data, "%3.1f", bandwidth/(double)nb_frame);
	sprintf(&data[6], "%3.1f", fps/(double)nb_frame);
	sprintf(&data[12], "%3d", coder_rate_max);
	if((lwrite=write(fd, data, LEN_DATA)) != LEN_DATA){
	  if(DEBUG_MODE)
	    perror("encode_h261'sending fps/bw/coder_rate_max");
	}

#ifdef GETSTAT_MODE
	if(LOG_FILES_OPENED){
	    fprintf(file_rate, "%d\t%g\n", relative_time,
		    bandwidth/(double)nb_frame);
	    fflush(file_rate);
	    fprintf(file_ratemax, "%d\t%d\n", relative_time,
		    coder_rate_max);
	    fflush(file_ratemax);
	    fprintf(file_fps, "%d\t%g\n", relative_time,
		    fps/(double)nb_frame);
	    fflush(file_fps);
	}
#endif
	  
	fps = 0.0;
	bandwidth = 0.0;
	nb_frame = 1;
	last_displaytime = synctime;
      }else{
	nb_frame ++;
      }
      Nblock = 0;
      length_image = 0;
      inittime=synctime;
  } /* ! FIRST */
#endif

    Mput_value_ptr(ptr_chaine_codee,0,4,jj) /* NG only */
    Mput_value_ptr(ptr_chaine_codee,RT,5,jj) /* RT */
#ifdef ENCODE_CIF4
    Mput_value_ptr(ptr_chaine_codee,4,6,jj) /* TYPEI = CIF */
#else
    if(CIF)
	Mput_value_ptr(ptr_chaine_codee,4,6,jj) /* TYPEI = CIF */
    else
	Mput_value_ptr(ptr_chaine_codee,0,6,jj) /* TYPEI = QCIF */
#endif
    Mput_bit_ptr(ptr_chaine_codee,0,jj) /* ISI1 */

/*--------------------------- MOVEMENT DETECTION ----------------------------*/
#ifdef ENCODE_CIF4
#define  IMAGE_TYPE CIF4_IMAGE_TYPE
#else
#define  IMAGE_TYPE CIF
#endif
Nblock = ImageChangeDetect(new_Y, new_Cb, new_Cr, image_coeff, cif_number,
			   NG_MAX, &GobEncoded[0], COLOR, FULL_INTRA,
			   max_inter, max_value, threshold, BperMB,
			   IMAGE_TYPE, NACK_FIR_ALLOWED);

/*---------------------------------------------------------------------------*/

/*----------- FULL INTRA DECISION IN CASE OF CHANGE OF SCENE DETECTED -------*/
#ifdef ENCODE_CIF4
    if(Nblock > nblock_cif_max){
      if(DEBUG_MODE)
	fprintf(stderr, "Full intra encoding\n");
      FULL_INTRA = TRUE;
      r->h261.I = TRUE;
      Nblock = 1584;
    }

    for(NG=1; NG<=NG_MAX; NG++){
#else
    if(FULL_INTRA == FALSE){
      if(CIF){
	if(Nblock > nblock_cif_max){
	  if(DEBUG_MODE)
	    fprintf(stderr, "Full intra encoding\n");
          FULL_INTRA = TRUE;
	  r->h261.I = TRUE;
	  Nblock = 1584;
      }
      }else{
	if(Nblock > nblock_qcif_max){
	  if(DEBUG_MODE)
	    fprintf(stderr, "Full intra encoding\n");
          FULL_INTRA = TRUE;
	  r->h261.I = TRUE;
	  Nblock = 396;
	}
      }
    }

    k=1;
    for(NG=1; NG<=NG_MAX; NG+=INC_NG){
#endif
/*---------------------------------------------------------------------------*/

      /* GOB LOOP */
      FULL_INTRA=TRUE; /* For test */
      if(FULL_INTRA || GobEncoded[NG-1]){
	mbnul=0;

#ifdef ENCODE_CIF4
	gob = (GOB *) (image_coeff)[NG-1];
#else
	gob = CIF ? (GOB *)(image_coeff)[NG-1] : (GOB *)(image_coeff)[NG-k++]; 
#endif

	for(MBA=1; MBA<34; MBA++){

	  /* MACROBLOCK LOOP */

	  col = MBcol[NG][MBA];
	  lig = MBlig[NG][MBA];

	  coldiv2 = col >> 1;
	  ligdiv2 = lig >> 1;

	  mb = &((*gob)[MBA-1]);
	  if((FULL_INTRA == FALSE) && (mb->MODE == INTER)){
/*------------------------------- INTER FFCT --------------------------------*/

	   max_coeff = CodeInterMode(new_Y, new_Cb, new_Cr, mb, 
		cmax, lig, col, cif_number, COLOR);

/*---------------------------------------------------------------------------*/

	  }else{
/*------------------------------- INTRA FFCT  -------------------------------*/
	    mb->MODE = INTRA;
	    mb->CBC = full_cbc;
	    mb->last_encoding = 1;
	    mb->cpt_inter = 0;
	    max_coeff = 0; 
	    max=ffct_intra_8x8((new_Y+COL_MAX*lig+col), mb->P[0], COL_MAX,
			       &cmax[0]);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=ffct_intra_8x8((new_Y+COL_MAX*lig+col+8), mb->P[1], COL_MAX,
			       &cmax[1]);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=ffct_intra_8x8((new_Y+COL_MAX*(lig+8)+col), mb->P[2], COL_MAX,
			       &cmax[2]);
	    if(max>max_coeff)
	      max_coeff=max;
	    max=ffct_intra_8x8((new_Y+COL_MAX*(lig+8)+col+8), mb->P[3],
			       COL_MAX, &cmax[3]);
	    if(max>max_coeff)
	      max_coeff=max;
	    if(COLOR){
	      max=chroma_ffct_intra_8x8((new_Cb+COL_MAX_DIV2*ligdiv2+coldiv2),
					mb->P[4], COL_MAX_DIV2, &cmax[4]);
	      if(max>max_coeff)
		max_coeff=max;
	      max=chroma_ffct_intra_8x8((new_Cr+COL_MAX_DIV2*ligdiv2+coldiv2),
					mb->P[5], COL_MAX_DIV2, &cmax[5]);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	  }
/*-------------------------- QUANTIZATION -----------------------------------*/
	  if(mb->CBC)
	    quant_mb = Quantize(mb, max_coeff, &bnul, &mbnul, 
				quantizer, FULL_INTRA, BperMB,
				GobEncoded, NG, default_quantizer);
/*---------------------------------------------------------------------------*/

	  if(bnul != BperMB){
/*----------------- FEEDBACK LOOP: INVERSE QUANTIZATION + IFCT --------------*/
	    int inter = mb->MODE;
	    tab_quant1 = tab_rec1[quant_mb-1];
	    tab_quant2 = tab_rec2[quant_mb-1];
	    
	    if((mb->CBC) & 0x20)
	      ifct_8x8(mb->P[0], tab_quant1, tab_quant2, inter,
		       (old_Y+cif_number*cif_size+COL_MAX*lig+col),cmax[0],
		       COL_MAX);
	  
	    if((mb->CBC) & 0x10)
	      ifct_8x8(mb->P[1], tab_quant1, tab_quant2, inter, 
		       (old_Y+cif_number*cif_size+COL_MAX*lig+col+8),cmax[1],
		       COL_MAX);
	  
	    if((mb->CBC) & 0x08)
	      ifct_8x8(mb->P[2], tab_quant1, tab_quant2, inter,
		       (old_Y+cif_number*cif_size+COL_MAX*(lig+8)+col),
		       cmax[2], COL_MAX);

	    if((mb->CBC) & 0x04)
	      ifct_8x8(mb->P[3], tab_quant1, tab_quant2, inter,
		       (old_Y+cif_number*cif_size+COL_MAX*(lig+8)+col+8),
		       cmax[3], COL_MAX);
	    if(COLOR){
	      if((mb->CBC) & 0x02)
		ifct_8x8(mb->P[4], tab_quant1, tab_quant2, inter,
			 (old_Cb+cif_number*qcif_size+COL_MAX_DIV2*ligdiv2
			  +coldiv2), cmax[4], COL_MAX_DIV2);
	      if((mb->CBC) & 0x01)
		ifct_8x8(mb->P[5], tab_quant1, tab_quant2, inter,
			 (old_Cr+cif_number*qcif_size+COL_MAX_DIV2*ligdiv2+
			  coldiv2), cmax[5], COL_MAX_DIV2);
	    }
/*---------------------------------------------------------------------------*/
	  }
	} /* MACROBLOCK LOOP */
      }
#ifndef ENCODE_CIF4
else
	k ++;
#endif

      {
	int last_AM, quant=quantizer;
	register int NM, b;

	if(!VIDEO_PACKET_HEADER){
	  h->seq = htons(sequence_number);
	  sequence_number = (int)(sequence_number + 1) % 65536;
	  r->h261.C = COLOR;
	  r->h261.V = 0;
	  r->h261.F = NACK_FIR_ALLOWED;
	  r->h261.Q = TRUE;
#ifdef ENCODE_CIF4
	  r->h261.size = 0x04 | cif_number;
#else
	  r->h261.size = CIF ? 1 : 0;
#endif
	  if(USING_MC){
	    chaine_codee[MCDATA_START] = 0x00;
	    cPUT_ij(ptr_chaine_codee, jj, MCDATA_START,7);
	  }else{
	    chaine_codee[10] = 0x00;
	    cPUT_ij(ptr_chaine_codee, jj, 10, 7);
	  }
	  VIDEO_PACKET_HEADER = TRUE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}

	/*********************\
	* H.261 CODING OF GOB *
	\*********************/

	if(FIRST_GOB_IS_ENCODED){
	  Mput_value_ptr(ptr_chaine_codee,1,16,jj)
	}else
	  FIRST_GOB_IS_ENCODED = TRUE;

	/* The GBSC not be encoded (S bit used) */
	Mput_value_ptr(ptr_chaine_codee,NG,4,jj)
	Mput_value_ptr(ptr_chaine_codee,quant,5,jj)
	Mput_bit_ptr(ptr_chaine_codee,0,jj) /* ISG */

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
	      {
		Midx_chain_forecast(idx_tab_mba, OFFSET_tab_mba, MAX_tab_mba,
				    (NM-last_AM), ptr_chaine_codee, jj,
				    "Bad MBA value: %d\n")
	      }
	      
	    CBC_IS_CODED = (mb->MODE ==INTER);    
	    if(mb->QUANT != quant){
	      quant=mb->QUANT;
	      if(CBC_IS_CODED)
		/*TYPEM = INTER + CBC + QUANTM */
		Mput_value_ptr(ptr_chaine_codee,1,5,jj)
	      else
		/* TYPEM = INTRA + QUANTM */
		Mput_value_ptr(ptr_chaine_codee,1,7,jj)
	      /* Coding of QUANTM */
	      Mput_value_ptr(ptr_chaine_codee,quant,5,jj)
	    }
	    else{
	      if(CBC_IS_CODED)
		/* TYPEM = INTER + CBC */
		Mput_bit_ptr(ptr_chaine_codee,1,jj)
	      else
		/* TYPEM = INTRA */
		Mput_value_ptr(ptr_chaine_codee,1,4,jj)
	    }
	    if(CBC_IS_CODED){
	      {
		Midx_chain_forecast(idx_tab_cbc, OFFSET_tab_cbc, MAX_tab_cbc,
				    (int)mb->CBC, ptr_chaine_codee, jj,
				    "Bad CBC value: %d\n")
	      }
	    }
	    /*************\
	    * BLOCK LAYER *
	    \*************/
	      for(b=0; b<BperMB; b++){
		if(mb->CBC & MASK_CBC[b]){
		  PUT_bit_state(bit_state, ptr_chaine_codee, byte4_codee, jj)
		  idx_code_bloc_8x8(mb->P[b], mb->nb_coeff[b],
				    mb->MODE, &bit_state);
		  GET_bit_state(bit_state, ptr_chaine_codee, byte4_codee, jj)
	        }
	      }
#if !defined(GREY_NO_STANDARD) && !defined(ENCODE_CIF4)
	      if(!CBC_IS_CODED && !COLOR){
		Mput_value_ptr(ptr_chaine_codee, 0x1fe, 10, jj);
                /* 127 CC value for Cb + EOB */
		Mput_value_ptr(ptr_chaine_codee, 0x1fe, 10, jj);
               /* 127 CC value for Cr + EOB */
	      }
#endif
	      last_AM = NM;
	    } 
	}else{
	    ; /* This GOB is unchanged */
	}

	/* If there is enough data or it is the last GOB,
	* write data on socket.
	*/


	if((((u_int)ptr_chaine_codee-(u_int)&chaine_codee[0]) > 500)
	   || (NG == NG_MAX)){
	  int Ebit, i;

	  if(USING_MC){
	    /* Set up the app_mcct header for this packet */
	    /* increment through to set up the h261 stuff */
	    
	    mc_txpkt(mcTxState, h, (rtp_t *) &chaine_codee[sizeof(rtp_hdr_t)]);
	  }
	  
	  /* Set the H.261 RTP header */
	  Ebit = jj % 8;
	  r->h261.S = r->h261.E = 1; /* By default, E = S = 1 */
	  r->h261.Sbit = 0; /* always null */
	  r->h261.Ebit = Ebit;
	  r->h261.C = COLOR;
	  r->h261.V = 0;
	  r->h261.F = NACK_FIR_ALLOWED;
	  r->h261.Q = TRUE;
#ifdef ENCODE_CIF4
	  r->h261.size = 0x04 | cif_number;
#else
	  r->h261.size = CIF ? 1 : 0;
#endif
	  if(Ebit != 0){
	    /* We have to nullify the following bits */
	    Mput_value_ptr(ptr_chaine_codee,0,Ebit,jj)
	  }
	  i = ((u_int)ptr_chaine_codee-(u_int)&chaine_codee[0]) +
	    (32 - jj)/8;
	  STORE_CODE(ptr_chaine_codee, byte4_codee<<jj);
  
	  /* Sending packet */
	  if(i < MAX_BSIZE){
	    h = (rtp_hdr_t *) chaine_codee;
	    h->p = (USING_MC ? 1 : 0);
	    h->s = (NG==NG_MAX ? 1 : 0);
	    h->format = RTP_H261_CONTENT;

	    len_packet = i;
	    buf_to_send = (char *)chaine_codee;
#ifdef CRYPTO
	    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
	      test_key("video_coder");
#endif
	      len_packet = method[current_method].crypt_rtp(
		  (char *)chaine_codee, len_packet, s_key, len_key, 
		  (char *)buf_crypt);
	      buf_to_send = (char *)buf_crypt;
	    }
#endif /* CRYPTO */  

	    if((lwrite=sendto(sock, buf_to_send, len_packet, 0, 
			      (struct sockaddr *) &param->addr, len_addr))
	       != len_packet){
	      perror("sendto: Cannot send data on socket");
	    }
	    length_image += lwrite;
	  }else{
	    int rest, nsend, aux;

	    /* Set the H.261 RTP header */

	    h = (rtp_hdr_t *) chaine_codee;
	    h->p = (USING_MC ? 1 : 0);
	    h->s = (NG==NG_MAX ? 1 : 0);
	    h->format = RTP_H261_CONTENT;

	    Ebit = jj % 8;
	    r->h261.S = 1;
	    r->h261.E = 0;
	    r->h261.Sbit = 0; 
	    r->h261.Ebit = 0;
	    r->h261.C = COLOR;
	    r->h261.V = 0;
	    r->h261.F = NACK_FIR_ALLOWED;
	    r->h261.Q = TRUE;
#ifdef ENCODE_CIF4
	    r->h261.size = 0x04 | cif_number;
#else
	    r->h261.size = CIF ? 1 : 0;
#endif
	    rest = i;

	    len_packet = memo_len_packet = MAX_BSIZE;
	    buf_to_send = (char *)chaine_codee;
#ifdef CRYPTO
	    if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
	      test_key("video_coder");
#endif
	      len_packet = method[current_method].crypt_rtp(
		  (char *)chaine_codee, len_packet, s_key, len_key, 
		  (char *)buf_crypt);
	      buf_to_send = (char *)buf_crypt;
	    }
#endif /* CRYPTO */  

	    if((lwrite=sendto(sock, buf_to_send, len_packet, 0, 
			      (struct sockaddr *) &param->addr, len_addr))
	       != len_packet){
	      perror("sendto: Cannot send data on socket");
	    }
	    r->h261.S = r->h261.E = 0;
	    r->h261.Sbit = r->h261.Ebit = 0;
	    nsend = memo_len_packet;
	    rest -= nsend;
	    while(rest != 0){
	      if(USING_MC){
		/* Need to reset the mc header stuff at this point */
		aux = MAX_BSIZE - MCDATA_START;
		bcopy((char *)&chaine_codee[nsend], 
		      (char *)&chaine_codee[MCDATA_START],
		      min(rest, aux));
		aux = rest + MCDATA_START;
	      }else{
		aux = MAX_BSIZE - 10;
		bcopy((char *)&chaine_codee[nsend], (char *)&chaine_codee[10],
		      min(rest, aux));
		
		aux = rest + 10;	  
	      }

	      if(aux <= MAX_BSIZE){
		/* E=1, Ebit */
		h = (rtp_hdr_t *) chaine_codee;
		h->p = (USING_MC ? 1 : 0);
		h->s = (NG==NG_MAX ? 1 : 0);
		h->format = RTP_H261_CONTENT;
		  
		r->h261.S = 0;
		r->h261.E = 1;
		r->h261.Sbit = 0; 
		r->h261.Ebit = Ebit;
		r->h261.C = COLOR;
		r->h261.V = 0;
		r->h261.F = NACK_FIR_ALLOWED;
		r->h261.Q = TRUE;
#ifdef ENCODE_CIF4
		r->h261.size = 0x04 | cif_number;
#else
		r->h261.size = CIF ? 1 : 0;
#endif
	      }
	      h->seq = htons(sequence_number);

	      sequence_number = (int)(sequence_number + 1) % 65536;
	      if(USING_MC){
		mc_txpkt(mcTxState, h, (rtp_t *)
			 &chaine_codee[sizeof(rtp_hdr_t)]);
	      }
	      
	      len_packet = memo_len_packet = min(aux, MAX_BSIZE);
	      buf_to_send = (char *)chaine_codee;
#ifdef CRYPTO
	      if(ENCRYPTION_ENABLED()){
#ifdef DEBUG_CRYPTO
		test_key("video_coder");
#endif
		len_packet = method[current_method].crypt_rtp(
		    (char *)chaine_codee, len_packet, s_key, len_key, 
		    (char *)buf_crypt);
		buf_to_send = (char *)buf_crypt;
	      }
#endif /* CRYPTO */  

	      if((lwrite=sendto(sock, buf_to_send, len_packet, 0,
				(struct sockaddr *) &param->addr, len_addr))
		 != len_packet){
		perror("sendto: Cannot send data on socket");
	      }

	      aux = USING_MC ? (memo_len_packet - MCDATA_START) : 
		  (memo_len_packet - 10);
	      nsend += aux;
	      rest -= aux;
	    }
	    length_image += i;
	  }
	  VIDEO_PACKET_HEADER = FALSE;
	  FIRST_GOB_IS_ENCODED = FALSE;
	}
	/* PUT_ij(ptr_chaine_codee, jj);*/
	
      } /* gob encoding */
    } /* GOB LOOP */
    {
      struct timeval realtime;
      double newt, deltat, min_dt;
      BOOLEAN TEMPO;
      static double oldt;
      static BOOLEAN FIRST=TRUE;

#ifdef ENCODE_CIF4
      if(cif_number == 3){
	FULL_INTRA = FALSE;
      }
#else
      FULL_INTRA = FALSE;
#endif      
      do{
	gettimeofday(&realtime, (struct timezone *)NULL);
	newt = 1000000.0*realtime.tv_sec + realtime.tv_usec;
	if(PRIVILEGE_QUALITY && (rate_control != WITHOUT_CONTROL)){
	  if(FIRST){
	    deltat = min_dt = 0.0;
	    FIRST = FALSE;
	  }else{
	    min_dt = 8000.0*length_image/coder_rate_max;
	    deltat = (newt - oldt);
	  }
	  TEMPO = TRUE;
	}else{
	  TEMPO = FALSE;
	  deltat = min_dt = 0.0;
	}
#ifdef ENCODE_CIF4
	if(cif_number == 3){
	  if(rate_control == AUTOMATIC_CONTROL || NACK_FIR_ALLOWED){
	    CheckControlPacket(sock, cpt, timestamps, ForceGobEncoding, 
			       1, &FULL_INTRA, TRUE, NACK_FIR_ALLOWED,
			       &realtime, TEMPO);
	  }
	}
#else
	if(rate_control == AUTOMATIC_CONTROL || NACK_FIR_ALLOWED){
	  CheckControlPacket(sock, cpt, timestamps, ForceGobEncoding, 
			     INC_NG, &FULL_INTRA, FALSE, NACK_FIR_ALLOWED,
			     &realtime, TEMPO);
	}
#endif
      }while(deltat < min_dt);
#ifdef BYTES_DUMP
      fprintf(stdout, "%d\t%d\n", cpt, length_image);
#endif
      oldt = newt;
    }
    cpt ++;
  }




