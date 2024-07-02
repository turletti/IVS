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
*  File    : coder.c                                                       *
*  Date    : June 1992                                                     *
*  Author  : Thierry Turletti                                              *
*  Release : 1.1                                                           *
*--------------------------------------------------------------------------*
*  Description :  H.261 Coder.                                             *
*                                                                          *
* Added support for NTSC, and auto format detection,                       *
* David Berry, Sun Microsystems,                                           *
* david.berry@sun.com                                                      *
*                                                                          *
* The VideoPix specific code is originally from work that is               *
* Copyright (c) 1990-1992 by Sun Microsystems, Inc.                        *
* Permission to use, copy, modify, and distribute this software and        * 
* its documentation for any purpose and without fee is hereby granted,     *
* provided that the above copyright notice appear in all copies            * 
* and that both that copyright notice and this permission notice appear    * 
* in supporting documentation.                                             *
*                                                                          *
* This file is provided AS IS with no warranties of any kind.              * 
* The author shall have no liability with respect to the infringement      *
* of copyrights, trade secrets or any patents by this file or any part     * 
* thereof.  In no event will the author be liable for any lost revenue     *
* or profits or other special, indirect and consequential damages.         *
\**************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "in.h"
#include <netdb.h>
#include <math.h>
#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "quantizer.h"

#ifdef VIDEOPIX
#include "vfc_lib.h"

#define NTSC_FIELD_HEIGHT       240     /* Height of a field in NTSC */
#define PAL_FIELD_HEIGHT        287     /* Height of a field in PAL */
#define HALF_YUV_WIDTH          360     /* Half the width of YUV (720/2) */
#define QRTR_YUV_WIDTH          180     /* 1/4 the width of YUV (720/4) */
#define QRTR_PAL_HEIGHT         143     /* 1/4 PAL height */
#define QRTR_NTSC_HEIGHT        120     /* 1/4 NTSC height */
#define CIF_WIDTH               352     /* Width of CIF */
#define QCIF_WIDTH              176     /* Width of Quarter CIF */

#endif

#define LIG_MAX 288
#define COL_MAX 352
#define NBLOCKMAX 1584
#define NB_PORT_MSG_LOSS 3333
#define NO_SOUND 0
#define UNCOMPRESS_SOUND 1
#define COMPRESS_SOUND 2

#define put_bit(octet,bit,i,j) \
    (j==7 ? (octet[(i)++]+=bit, octet[(i)]=0, j=0) : \
     (octet[i]+=(bit<<(7-(j)++))))
#define sqr(x) ((x)*(x))



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



    
int ItIsTheEnd()
  {
    extern int fd[2];
    struct timeval tim_out;
    int mask;

    mask = 1<<fd[0];
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, 
	      (fd_set *)0, &tim_out) != 0)
	return(1);
    else
	return(0);
  }
    

int declare_listen(num_port)
    int num_port;

  {
    int size_of_ad_in;
    struct sockaddr_in ad_in;
    int sock;

    bzero((char *)&ad_in, sizeof(ad_in));
    ad_in.sin_family = AF_INET;
    ad_in.sin_port = htons(num_port);
    ad_in.sin_addr.s_addr = INADDR_ANY;

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
      fprintf(stderr,"declare_listen : cannot create socket-controle UDP\n");
      exit(1);
    }
    size_of_ad_in = sizeof(ad_in);
    if (bind(sock,(struct sockaddr *) &ad_in, size_of_ad_in) < 0){
      fprintf(stderr, "cannot bind socket\n");
      close(sock);
      exit(1);
    }
    if(!num_port){
      getsockname(sock,(struct sockaddr *)&ad_in,&size_of_ad_in);
      printf("Listening at %d port number\n", ad_in.sin_port);
    }
    return(sock);
  }    


int ReceivePacket(sock, buf)
    int sock;
    u_char *buf;

  {
    struct sockaddr_in adr;
    struct hostent *host;
    int er;

    er = sizeof(adr);
    if (recvfrom(sock, (char *)buf, 10, 0, &adr, &er) < 0){
      perror("receive from");
      exit(0);
    }

    host=gethostbyaddr((char *)(&(adr.sin_addr)), sizeof(adr.sin_addr),
                       adr.sin_family);
#ifdef DEBUG
    fprintf(stderr, "** from %s : packet #%d is lost\n", host->h_name, 
	    (int)buf[0]);
#endif

  }


int OnePacketIsLost(sock)
    int sock;
  {
    struct timeval tim_out;
    int mask;
    u_char buf[1];

    mask = 1 << sock;
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    if(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) 
       != 0){
      ReceivePacket(sock, buf);
      mask = 1 << sock;
      while(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		   &tim_out) != 0){
	ReceivePacket(sock, buf);
	mask = 1 << sock;
      }
      return(1);
    }else
        return(0);
  }


  


#ifdef VIDEOPIX

void vfc_get_cif_pal(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    register int i, j;
    register u_char *r_data;
    register u_int tmpdata, *data_port;
    static int grab_ioctl = CAPTRCMD;


    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);

    /* Skip over the non-active samples */
    for (i=0; i<VFC_ESKIP_PAL; i++)
	tmpdata = *data_port;

    /* Read in the odd field data */
    r_data = (u_char *)im_data;

    for (i=0; i<PAL_FIELD_HEIGHT; i++) {
      for (j=0; j<CIF_WIDTH; j++) {
	*r_data++ = VFC_PREVIEW_Y(*data_port, 30);
	tmpdata = *data_port;
      }
      for (; j < HALF_YUV_WIDTH; j++) {
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
    }
  }

void vfc_get_qcif_pal(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];

  {
    register int i, j;
    register u_char *r_data;
    register u_int tmpdata, *data_port;
    static int grab_ioctl = CAPTRCMD;
    static int DROPP = VFC_YUV_WIDTH + 16;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port2;

    /*
    * Grab the image
    */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);

    /* Skip over the non-active samples */
    for (i = 0; i < VFC_ESKIP_PAL; i++)
	tmpdata = *data_port;

    /* Read in the odd field data */
    for (i=0; i<PAL_FIELD_HEIGHT; i++) {
      r_data = &(im_data[i][0]);
      for (j=0; j<QCIF_WIDTH; j++) {
	*r_data++ = VFC_PREVIEW_Y(*data_port, 30);
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
      for(j=0; j<DROPP; j++)
	  tmpdata = *data_port;
    }
  }

void vfc_get_cif_ntsc(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];
  {
    register int i, j;
    register u_char *r_data;
    register u_int tmpdata, *data_port;
    static int grab_ioctl = CAPTRCMD;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port1;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);

    /* Skip over the non-active samples */
    for (i=0; i<VFC_OSKIP_NTSC; i++)
	tmpdata = *data_port;

    /* Read in the odd field data */
    r_data = (u_char *)im_data;

    for (i=0; i<NTSC_FIELD_HEIGHT; i++) {
      for (j=0; j<CIF_WIDTH; j++) {
	*r_data++ = VFC_PREVIEW_Y(*data_port, 30);
	tmpdata = *data_port;
      }
      for (; j < HALF_YUV_WIDTH; j++) {
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
    }
  }

void vfc_get_qcif_ntsc(vfc_dev, im_data)
    VfcDev *vfc_dev;
    u_char im_data[][COL_MAX];

  {
    register int i, j;
    register u_char *r_data;
    register u_int tmpdata, *data_port;
    static int grab_ioctl = CAPTRCMD;
    static int DROPP = VFC_YUV_WIDTH + 16;

    /*
    * Local register copy of the data port
    * improves performance
    */
    data_port = vfc_dev->vfc_port1;

    /*
     * Grab the image
     */
    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl);

    /* Skip over the non-active samples */
    for (i = 0; i < VFC_OSKIP_NTSC; i++)
	tmpdata = *data_port;

    /* Read in the odd field data */

    for (i=0; i<NTSC_FIELD_HEIGHT; i++) {
      r_data = &(im_data[i][0]);
      for (j=0; j<QCIF_WIDTH; j++) {
	*r_data++ = VFC_PREVIEW_Y(*data_port, 30);
	tmpdata = *data_port;
	tmpdata = *data_port;
	tmpdata = *data_port;
      }
      for(j=0; j<DROPP; j++)
	  tmpdata = *data_port;
    }
  }
#endif    


void encode_sequence(file_in, file_out, host, num_port, col, lig, threshold, 
		     QUANT_FIX, UDP, STATS, frequency, quality, audio) 

    char *file_in, *file_out, *host;
    int num_port; /* UDP port number */
    int col, lig; /* image's size */
    int threshold; /* for motion estimation */
    int QUANT_FIX;
    int UDP; /* Boolean: TRUE if output->UDP */
    int STATS; /* Boolean: True if SNR process*/
    int frequency;
    int quality; /* Boolean: true if we privilege quality and not rate */
    int audio; 

{
    extern int n_mba, n_tcoeff, n_cbc, n_dvm;
    extern int tab_rec1[][255], tab_rec2[][255]; /* for quantization */
    extern int fd[2];

    IMAGE_CIF image_coeff;
    u_char new_image[LIG_MAX][COL_MAX];
    FILE *F_in, *F_out;
    int NG_MAX;
    int FIRST=1;
    int CIF=0;
    int sock_recv=0;

    if(lig == 288){
      NG_MAX = 12;
      CIF =1;
    }else
      NG_MAX = 5;

    if(!UDP){
      if((F_out=fopen(file_out,"w"))==NULL){
        fprintf(stderr,"encode_sequence failed : Cannot open file %s\n",
                file_out);
        exit(1);
      }
    }else
	sock_recv=declare_listen(NB_PORT_MSG_LOSS);

    if(strcmp(file_in, "VIDEOPIX")){
      do{
	int end=0;
	register int l;

	if((F_in=fopen(file_in,"r"))==NULL){
	  fprintf(stderr,"encode_sequence failed : Cannot open file %s\n",
		  file_in);
	  exit(1);
	}

	/*
	 *  First image, INTRA mode.
	 */

	for(l=0; l<lig; l++)
	    if(fread((char *)&(new_image[l][0]), 1, col, F_in) != col){
	      fprintf(stderr, "encode_sequence : Cannot read first image\n");
	      exit(1);
	    }

	encode_h261(new_image, image_coeff, host, num_port, threshold, 
		    NG_MAX, QUANT_FIX, FIRST, F_out, UDP, STATS, frequency, 
		    quality, sock_recv);
	FIRST=0;
      
	/*
	 *  Next images, INTRA mode too.
	 */
	while(1){

	  if(ItIsTheEnd()){
	    close(fd[0]);
	    exit(0);
	  }

	  for(l=0; l<lig; l++)
	      if(fread((char *)&(new_image[l][0]), 1, col, F_in) != col){
		fclose(F_in);
		if(!UDP)
		    exit(0);
		else
		    end=1;
	      }
	  if(!end)
	      encode_h261(new_image, image_coeff, host, num_port, 
			  threshold, NG_MAX, QUANT_FIX, FIRST, F_out, UDP, 
			  STATS, frequency, quality, sock_recv);
	  else
	      break;
	}

      }while(1);
    }else{

#ifdef VIDEOPIX    

      /* Video in comes from VIDEOPIX */
      if(UDP){
	int ida;

	if(audio != NO_SOUND)
	  ida=fork();
	
	if(ida == 0 && audio){
	  /* son processes audio */

	  struct soundbuf{
	    u_char compression;
	    struct{
	      int buffer_len;
	      char buffer_val[8000];
	    }buffer;
	  };
	  struct soundbuf netbuf;
#define buf netbuf.buffer.buffer_val
	  int compressing;  /* Compress sound buffers */
	  int squelch = 50; 	/* Squelch level if > 0 */
	  int ring = 0; /* Force speaker & level on next pkt ? */
	  int agc = 0;  /* Automatic gain control active ? */
	  int rgain = 33;  /* Current recording gain level */
	  int debugging = 0;  /* Debugging enabled here and there ? */
	  int loopback = 0;   /* Remote loopback mode */

	  if (!soundinit(O_RDONLY /* | O_NDELAY */ )) {
            fprintf(stderr, "Unable to initialise audio.\n");
	    exit(1);
	  }
	  if (agc) {
	    soundrecgain(rgain);      /* Set initial record level */
	  }
	  if (soundrecord()) {
	    struct sockaddr_in addr_decodeur;
	    int sock, len_addr;
	    u_char ttl=2;


	    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	      fprintf(stderr, "Cannot create datagram socket\n");
	      exit(-1);
	    }
	    addr_decodeur.sin_port = htons(num_port);
	    addr_decodeur.sin_family = AF_INET;
	    addr_decodeur.sin_addr.s_addr = (224<<24) | (2<<16) | (2<<8) | 2;
	    addr_decodeur.sin_addr.s_addr=htonl(addr_decodeur.sin_addr.s_addr);
	    len_addr = sizeof(addr_decodeur);
	    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	    compressing=(audio==COMPRESS_SOUND ? 1 : 0);

	    do{
	      int soundel = soundgrab(buf, sizeof buf);
	      unsigned char *bs = (unsigned char *) buf;

	      if (soundel > 0) {
		register unsigned char *start = bs;
		register int j;
		int squelched = (squelch > 0);

		/* If entire buffer is less than squelch, ditch it. */

		if (squelch > 0) {
		  for (j = 0; j < soundel; j++) {
		    if (((*start++ & 0x7F) ^ 0x7F) > squelch) {
		      squelched = 0;
		      break;
		    }
		  }
		}

		if (squelched) {
		  if (debugging) {
		    printf("Entire buffer squelched.\n");
		  }
		} else {
		  netbuf.compression = 128 | (ring ? 4 : 0);
		  netbuf.compression |= debugging ? 2 : 0;
		  netbuf.compression |= loopback ? 16 : 0;

		  /* If automatic gain control is enabled,
		     ride the gain pot to fill the dynamic range
		     optimally. */

		  if (agc) {
		    register unsigned char *start = bs;
		    register int j;
		    long msamp = 0;
		    
		    for (j = 0; j < soundel; j++) {
		      int tsamp = ((*start++ & 0x7F) ^ 0x7F);
		      
		      msamp += tsamp;
		    }
		    msamp /= soundel;
		    if (msamp < 0x30) {
		      if (rgain < 100) {
			soundrecgain(++rgain);
		      }
		    } else if (msamp > 0x35) {
		      if (rgain > 1) {
			soundrecgain(--rgain);
		      }
		    }
		  }
		  
		  ring = 0;
		  if (compressing) {
		    int i;

		    soundel /= 2;
		    for (i = 1; i < soundel; i++) {
		      buf[i] = buf[i * 2];
		    }
		    netbuf.compression |= 1;
		  }
		  netbuf.buffer.buffer_len = soundel;
		  if (sendto(sock, (char *)&netbuf, (sizeof(struct soundbuf))- 
			     (8000 - soundel), 0, 
			   (struct sockaddr *) &addr_decodeur, len_addr) < 0) {
		    perror("sending datagram message");
		    exit(0);
		  }
		}
	      } else {
		usleep(100000L);  /* Wait for some sound to arrive */
	      }
	    }while(!ItIsTheEnd());
	    printf("son audio process aborted\n");
	    exit(0);
	  }
	}else{

	  if(ida == -1)
	    fprintf(stderr,"%s : Sorry, cannot fork audio process ...\n");

	  /* father processes video */
	  {
	    VfcDev *vfc_dev;
	    int format;

	    /* Open the hardware */
	    if((vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV)) == NULL){
	      fprintf(stderr, "Could not open hardware VideoPix\n");
	      exit(1);
	    }

	    /* Set the input to be Port 1 */
	    vfc_set_port(vfc_dev, VFC_PORT1);

	    /* Set the correct video type */
	    if(vfc_set_format(vfc_dev, VFC_AUTO, &format) < 0){
	      fprintf(stderr, "Could not set video format automatically\n");
	      fprintf(stderr, "Check your camera ...\n");
	      exit(1);
	    }
	    if(format == NO_LOCK){
	      /* No video signal detected, vfc_video_format set up */
	      fprintf(stderr, "No video signal in detected, Waiting ...\n");
	      while(format == NO_LOCK){
		if(vfc_set_format(vfc_dev, VFC_AUTO, &format) < 0){
		  fprintf(stderr,"Could not set video format automatically\n");
		  exit(1);
		}
		if(ItIsTheEnd()){
		  vfc_destroy(vfc_dev);
		  close(fd[0]);
		  exit(0);
		}
	      }
	    }
	    format = vfc_video_format(format);
	    
	    /* We use the Preview Display mode. Routines directly access
	     *  the hardware and return a 7-bit grayscale image.
	     */

	    if(CIF) {
	      (format == VFC_PAL ? vfc_get_cif_pal(vfc_dev, new_image) :
	       vfc_get_cif_ntsc(vfc_dev, new_image));
	    } else {
	      (format == VFC_PAL ? vfc_get_qcif_pal(vfc_dev, new_image) :
	       vfc_get_qcif_ntsc(vfc_dev, new_image));
	    }
	    
	    encode_h261(new_image, image_coeff, host, num_port, threshold,
			NG_MAX, QUANT_FIX, FIRST, F_out, UDP, STATS, 
			frequency, quality, sock_recv);
	    FIRST=0;
	    
	    /* Next frames, INTRA mode too */
	  
	    while(1){
#ifdef DEBUG
	      int inittime, synctime;

	      inittime=clock();
#endif
	      if(CIF) {
		(format == VFC_PAL ? vfc_get_cif_pal(vfc_dev, new_image) :
		 vfc_get_cif_ntsc(vfc_dev, new_image));
	      } else {
		(format == VFC_PAL ? vfc_get_qcif_pal(vfc_dev, new_image) :
		 vfc_get_qcif_ntsc(vfc_dev, new_image));
	      }

#ifdef DEBUG
	      synctime=clock();
	      printf("\n***(cpu) grabbing time: %d ms\n", 
		     (synctime-inittime)/1000);
#endif
	      encode_h261(new_image, image_coeff, host, num_port, threshold, 
			  NG_MAX,  QUANT_FIX, FIRST, F_out, UDP, STATS, 
			  frequency, quality, sock_recv);

	      if(ItIsTheEnd()){
		vfc_destroy(vfc_dev);
		close(fd[0]);
		exit(0);
	      }
	    }
	  }
	}
      }
#else
    fprintf(stderr,
	    "VIDEOPIX is not defined so, only input file is allowed\n");
    exit(1);
#endif
    }
  }



encode_h261(new_image, image_coeff, host, num_port, threshold, NG_MAX, 
	    QUANT_FIX, FIRST, F_out, UDP, STATS, frequency, quality, sock_recv)
    u_char new_image[][COL_MAX];
    IMAGE_CIF image_coeff;
    char *host; /* Decoder hostname */
    int num_port; /* UDP port number */
    int threshold; /* for motion estimation */
    int NG_MAX;
    int QUANT_FIX;
    int FIRST;
    FILE *F_out;
    int UDP;
    int STATS; /* Boolean: True if SNR process */
    int frequency;
    int quality; /* Boolean: True if we privelege quality and not rate */
    int sock_recv; /* socket used for loss messages */

  {
    int lig, col, x_gob, y_gob;
    int NG, MBA, INC_NG, CIF;
    GOB *gob;
    MACRO_BLOCK *mb;
    static u_char old[LIG_MAX][COL_MAX];
    int cc, quant_mb;
    int k;
    int bnul, mbnul;
    int cmax0, cmax1, cmax2, cmax3;
    int *tab_quant1, *tab_quant2;
    u_char chaine_codee[T_MAX];
    int i=0, j=0;
    static int lwrite;
    struct timeval realtime;
    static double inittime, synctime;
    static int RT;
    static int n_padd[8]={4, 1, 6, 3, 0, 5, 2, 7};
    static int p_padd[8]={0, 5, 2, 7, 4, 1, 6, 3};
    static int LIG[8]={0, 2, 0, 1, 3, 1, 3, 2};
    static int COL[8]={0, 2, 3, 1, 0, 2, 3, 1};
    static int n_im=0;
    int lig11, lig12, lig21, lig22, col11, col12, col21, col22;
    int N_PIX;
    int val_gob[12];
    int Nblock=0;
    int max_coeff;
    static int cpt=0;
    static int N8x8;
    int T_im=1000/frequency;
    int FFCT4x4=0, FFCT8x8=1; /* Booleans */
    static int INC_QUANT=0;
    static int num_packet=0;
    static int PARTIAL = 0;

    QUANT_FIX += INC_QUANT;
    if(UDP){
      i = 1;
      chaine_codee[0] = num_packet;
      num_packet = (num_packet == 63 ? 0 : num_packet+1);
      chaine_codee[1] = (u_char) 0;
    }else{
      i = 0;
      chaine_codee[0] = (u_char) 0;
    }
    CIF=(NG_MAX==12);
    if(CIF){
      INC_NG=1;
      N_PIX=12672;
    }else{
      INC_NG=2;
      N_PIX=3168;
    }
    /*************\
    * IMAGE LAYER *
    \*************/

    if(!PARTIAL){
      gettimeofday(&realtime, (struct timezone *)NULL);
      inittime=realtime.tv_sec + realtime.tv_usec / 1000000.0;
      RT=0;
      put_value(chaine_codee,16,20,&i,&j); /* CDI */
    }else{
      gettimeofday(&realtime, (struct timezone *)NULL);
      synctime = realtime.tv_sec + realtime.tv_usec / 1000000.0;
      RT=((RT + (int)(33*(synctime-inittime))) % 32);
#ifdef DEBUG
      printf("Encoding time : %d ms", (int)(1000*(synctime-inittime)));
#endif
      inittime=synctime;

    }
#ifdef DEBUG
    printf("\n Quant : %d\n", QUANT_FIX);
    printf("\nRT: %d,\t", RT);
#endif
    /* le CDI est deja code' */
    put_value(chaine_codee,RT,5,&i,&j); /* RT */
    if(CIF)
	put_value(chaine_codee,4,6,&i,&j); /* TYPEI = CIF */
    else
	put_value(chaine_codee,0,6,&i,&j); /* TYPEI = QCIF */
    put_bit(chaine_codee,0,i,j); /* ISI1 */

    if(PARTIAL){
      lig11=LIG[n_im];
      col11=COL[n_im];
      n_im=(n_im +1)%8;
      lig12=lig11+4;
      col12=col11+4;
      lig21=lig11+8;
      lig22=lig12+8;
      col21=col11+8;
      col22=col12+8;
      k=1;

      for(NG=1; NG<=NG_MAX; NG+=INC_NG){

	/* GOB LOOP */

	val_gob[NG-1]=1;
	mbnul=0;
	if(NG % 2)
	    x_gob = 0;
	else
	    x_gob = 176;
	y_gob = ((NG-1)/2)*48;
	if(CIF)
	    gob = (GOB *) (image_coeff)[NG-1];
	else
	    gob = (GOB *) (image_coeff)[NG-k++];

	for(MBA=1; MBA<34; MBA++){
	  register int diff, sum;
	  int lig_11, lig_12, lig_21, lig_22, col_11, col_12, col_21, col_22;

	  /* MACROBLOCK LOOP */

	  if(MBA > 22){ 
	    col = x_gob + (MBA-23)*16;
	    lig = y_gob + 32;
	  }else{
	    if(MBA>11){
	      col = x_gob + (MBA-12)*16;
	      lig = y_gob + 16;
	    }
	    else{
	      col = x_gob + (MBA-1)*16;
	      lig = y_gob;
	    }
	  }
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
	  mb->CBC = 0;

	  diff=(int)new_image[lig_11][col_11]
	      - (int)old[lig_11][col_11];
	  sum=abs(diff);
	  diff=(int)new_image[lig_11][col_12]
	      - (int)old[lig_11][col_12];
	  sum += abs(diff);
	  diff=(int)new_image[lig_12][col_11]
	      - (int)old[lig_12][col_11];
	  sum += abs(diff);
	  diff=(int)new_image[lig_12][col_12]
	      - (int)old[lig_12][col_12];
	  sum += abs(diff);
	  if(sum>threshold){
	    Nblock ++;
	    mb->CBC |= 32;
	  }else
	      bnul ++;
	  
	  diff=(int)new_image[lig_11][col_21]
	      - (int)old[lig_11][col_21];
	  sum=abs(diff);
	  diff=(int)new_image[lig_11][col_22]
	      - (int)old[lig_11][col_22];
	  sum += abs(diff);
	  diff=(int)new_image[lig_12][col_21]
	      - (int)old[lig_12][col_21];
	  sum += abs(diff);
	  diff=(int)new_image[lig_12][col_22]
	      - (int)old[lig_12][col_22];
	  sum += abs(diff);
	  if(sum>threshold){
	    Nblock ++;
	    mb->CBC |= 16;
	  }else
	      bnul ++;

	  diff=(int)new_image[lig_21][col_11]
	      - (int)old[lig_21][col_11];
	  sum=abs(diff);
	  diff=(int)new_image[lig_21][col_12]
	      - (int)old[lig_21][col_12];
	  sum += abs(diff);
	  diff=(int)new_image[lig_22][col_11]
	      - (int)old[lig_22][col_11];
	  sum += abs(diff);
       	  diff=(int)new_image[lig_22][col_12]
	      - (int)old[lig_22][col_12];
	  sum += abs(diff);
	  if(sum>threshold){
	    Nblock ++;
	    mb->CBC |= 8;
	  }else
	      bnul ++;

	  diff=(int)new_image[lig_21][col_21]
	      - (int)old[lig_21][col_21];
	  sum=abs(diff);
	  diff=(int)new_image[lig_21][col_22]
	      - (int)old[lig_21][col_22];
	  sum += abs(diff);
	  diff=(int)new_image[lig_22][col_21]
	      - (int)old[lig_22][col_21];
	  sum += abs(diff);
	  diff=(int)new_image[lig_22][col_22]
	      - (int)old[lig_22][col_22];
	  sum += abs(diff);
	  if(sum>threshold){
	    Nblock ++;
	    mb->CBC |= 4;
	  }else
	      bnul ++;
	  if(bnul==4){
	    mbnul ++;
	    if(mbnul == 33){
	      val_gob[NG-1]=0;
	    }
	  }
	} /* MACROBLOCK LOOP */
      } /* GOB LOOP */
    } 
  
    if(!quality){
      /*
       * If there are not a lot of detected blocks, choose ffct8x8() transform.
       */
      if(cpt < 3){
	if(cpt == 1)
	    FFCT8x8=1;
	else
	    FFCT4x4=1;
      }else{
	if(Nblock > N8x8){
	  FFCT8x8=0;
	  FFCT4x4=1;
#ifdef DEBUG
	  printf("NB= %d > %d ---> using ifct4x4() transformation \n", Nblock, 
		 N8x8);
#endif
	}else{
	  FFCT8x8=1;
	  FFCT4x4=0;
#ifdef DEBUG 
	  printf("NB= %d < %d ---> using ifct8x8() transformation \n", Nblock, 
		 N8x8);
#endif
	}
      }
    }

    k=1;
    for(NG=1; NG<=NG_MAX; NG+=INC_NG){

      /* GOB LOOP */

      if(PARTIAL && !val_gob[NG-1])
	  continue;
      mbnul=0;
      if(NG % 2)
	  x_gob = 0;
      else
	  x_gob = 176;
      y_gob = ((NG-1)/2)*48;
      if(CIF)
	  gob = (GOB *) (image_coeff)[NG-1];
      else
	  gob = (GOB *) (image_coeff)[NG-k++];

      for(MBA=1; MBA<34; MBA++){
	register int n, l, c;

	/* MACROBLOCK LOOP */

	if(MBA > 22){ 
	  col = x_gob + (MBA-23)*16;
	  lig = y_gob + 32;
	}else{
	  if(MBA>11){
	    col = x_gob + (MBA-12)*16;
	    lig = y_gob + 16;
	  }
	  else{
	    col = x_gob + (MBA-1)*16;
	    lig = y_gob;
	  }
	}
      
	mb = &((*gob)[MBA-1]);
	if(PARTIAL){
	  int max;

	  max_coeff=0;
	  if((mb->CBC) & 0x20){
	    if(FFCT4x4)
		max=ffct_4x4(new_image, mb->P[0], lig, col, 0, &cmax0);
	    else
		max=ffct_8x8(new_image, mb->P[0], lig, col, 0, &cmax0);
	  }	  
	  if(max>max_coeff)
	      max_coeff=max;

	  if((mb->CBC) & 0x10){
	    if(FFCT4x4)
		max=ffct_4x4(new_image, mb->P[1], lig, col+8, 0, &cmax1);
	    else
		max=ffct_8x8(new_image, mb->P[1], lig, col+8, 0, &cmax1);
	  }
	  if(max>max_coeff)
	      max_coeff=max;

	  if((mb->CBC) & 0x08){
	    if(FFCT4x4)
		max=ffct_4x4(new_image, mb->P[2], lig+8, col, 0, &cmax2);
	    else
		max=ffct_8x8(new_image, mb->P[2], lig+8, col, 0, &cmax2);
	  }
	  if(max>max_coeff)
	      max_coeff=max;

	  if((mb->CBC) & 0x04){
	    if(FFCT4x4)
		max=ffct_4x4(new_image, mb->P[3], lig+8, col+8, 0, &cmax3);
	    else
		max=ffct_8x8(new_image, mb->P[3], lig+8, col+8, 0, &cmax3);
	  }
	  if(max>max_coeff)
	      max_coeff=max;
	
	}else{
	  /* First image, all the blocks are coded */
	  mb->CBC = 60;
	  ffct_8x8(new_image, mb->P[0], lig, col, 0, &cmax0);
	  ffct_8x8(new_image, mb->P[1], lig, col+8, 0, &cmax1);
	  ffct_8x8(new_image, mb->P[2], lig+8, col, 0, &cmax2);
	  ffct_8x8(new_image, mb->P[3], lig+8, col+8, 0, &cmax3);
	  /* Chrominance composantes are not encoded (not decoded) */
	}
	if(mb->CBC){
	  quant_mb = max_coeff/256 - 1;
	  if(QUANT_FIX>quant_mb)
	      quant_mb=QUANT_FIX;

	  mb->QUANT=quant_mb;
	  /*
	   *  We have to quantize mb->P[n].
	   */
	  bnul=0;
	  if(quant_mb % 2){
	    /* odd quantizer */
	    for(n=0; n<4; n++){
	      register int *pt_P = &(mb->P[n][0][0]);
	      register int nb_coeff=0;

	      if((mb->CBC) && (1<<(5-n))){
		cc = *pt_P;
		for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=(*pt_P / quant_mb - 1)/2)!=0)
			    nb_coeff ++;
		      }else{
			if(*pt_P < 0)
			    if((*pt_P=(*pt_P / quant_mb + 1)/2)!=0)
				nb_coeff ++;
		      }
		      pt_P ++;
		    }
		if((mb->P[n][0][0]==0) && cc)
		    nb_coeff ++;
		mb->P[n][0][0] = cc;
		if(nb_coeff==0){
		  bnul ++;
		  (mb->CBC) &= (~(1<<(5-n)));
		}
	      }else
		  bnul ++;
	      mb->nb_coeff[n] = nb_coeff;
	    }
	  }else{
	    /* even quantizer */
	    for(n=0; n<4; n++){
	      register int *pt_P = &(mb->P[n][0][0]);
	      register int nb_coeff=0;

	      if((mb->CBC) && (1<<(5-n))){
		cc = *pt_P;
		for(l=0; l<8; l++)
		    for(c=0; c<8; c++){
		      if(*pt_P > 0){
			if((*pt_P=((*pt_P + 1)/quant_mb - 1)/2)!=0)
			    nb_coeff ++;
		      }else{
			if(*pt_P < 0)
			    if((*pt_P=((*pt_P - 1)/quant_mb - 1)/2)!=0)
				nb_coeff ++;
		      }
		      pt_P ++;
		    }
		if((mb->P[n][0][0]==0) && cc)
		    nb_coeff ++;
		mb->P[n][0][0] = cc;
		if(nb_coeff==0){
		  bnul ++;
		  (mb->CBC) &= (~(1<<(5-n)));
		}
	      }else
		  bnul ++;
	      mb->nb_coeff[n] = nb_coeff;
	    }		      
	  }
	  if(bnul==4){
	    mbnul ++;
	    if(mbnul == 33){
	      continue;
	    }
	  }
	}
	if(bnul != 4){
	  /************\
	  * IFCT of MB *
	  \************/
	  tab_quant1 = tab_rec1[quant_mb-1];
	  tab_quant2 = tab_rec2[quant_mb-1];

	  if((mb->CBC) & 0x20)
	      ifct_8x8(mb->P[0], tab_quant1, tab_quant2, 0,
		       &(old[lig][col]), cmax0, COL_MAX);
	  
	  if((mb->CBC) & 0x10)
	      ifct_8x8(mb->P[1], tab_quant1, tab_quant2, 0, 
		       &(old[lig][col+8]), cmax1, COL_MAX);
	  
	  if((mb->CBC) & 0x08)
	      ifct_8x8(mb->P[2], tab_quant1, tab_quant2, 0,
		       &(old[lig+8][col]), cmax2, COL_MAX);

	  if((mb->CBC) & 0x04)
	      ifct_8x8(mb->P[3], tab_quant1, tab_quant2, 0,
		       &(old[lig+8][col+8]), cmax3, COL_MAX);
	}
	if(MBA==33){

	  int last_AM, CBC, quant=QUANT_FIX;
	  register int NM, b;
	  
	  /*********************\
	  * H.261 CODING OF GOB *
	  \*********************/

	  put_value(chaine_codee,1,16,&i,&j); /* CDGB = 1 */
	  put_value(chaine_codee,NG,4,&i,&j); 
	  put_value(chaine_codee,quant,5,&i,&j);
	  put_bit(chaine_codee,0,i,j); /* ISG */

	  /******************\
	  * MACROBLOCK LAYER *
	  \******************/
	  last_AM=0;
	  for(NM=1; NM<=33; NM++){
	    mb = &((*gob)[NM-1]);
	    if(!(mb->CBC)) /* If MB is entirely null, continue ... */
		continue;
	    if(!chain_forecast(tab_mba,n_mba,(NM-last_AM),chaine_codee,&i,&j)){
	      fprintf(stderr,"Bad MBA value: %d\n", NM-last_AM);
	      exit(-1);
	    }
	    CBC = (mb->CBC==60 ? 0 : 1);    
	    if(mb->QUANT != quant){
	      quant=mb->QUANT;
	      if(CBC)
		  put_value(chaine_codee,1,5,&i,&j); /*TYPEM=INTRA+CBC+QUANTM*/
	      else
		  put_value(chaine_codee,1,7,&i,&j); /* TYPEM = INTRA+QUANTM */
	      put_value(chaine_codee,quant,5,&i,&j); /* Coding of QUANTM */
	    }
	    else{
	      if(CBC)
		  put_bit(chaine_codee,1,i,j); /* TYPEM = INTRA+CBC */
	      else
		  put_value(chaine_codee,1,4,&i,&j); /* TYPEM = INTRA */
	    }
	    if(CBC){
	      if(!chain_forecast(tab_cbc, n_cbc, (int)mb->CBC, chaine_codee,
				 &i, &j)){
		fprintf(stderr,"Bad CBC value: %d\n", mb->CBC);
		exit(-1);
	      }
	    }
	  
	    /*************\
	    * BLOCK LAYER *
	    \*************/
	    for(b=0; b<6; b++)
		if((mb->CBC >> (5-b)) & 0x01)
		    code_bloc_8x8(mb->P[b], chaine_codee, tab_tcoeff, 
				  n_tcoeff, mb->nb_coeff[b], &i, &j, 0);
	    last_AM = NM;

	  } 

	}
	
      } /* MACROBLOCK LOOP */
      
    } /* GOB LOOP */


    /******************\
    * N8x8 CALCULATION *
    \******************/
    {
      int deltat;

      gettimeofday(&realtime, (struct timezone *)NULL);
      synctime = realtime.tv_sec + realtime.tv_usec / 1000000.0;
      deltat=1000*(synctime-inittime);

      if(FFCT8x8)
	  N8x8=(Nblock*T_im)/deltat;
      else
	  N8x8=(Nblock*T_im)/deltat - 20;

#ifdef DEBUG
      printf("cpt: %d, Nblocs(8x8) = %d, deltat : %d ms\n", cpt, N8x8, deltat);
#endif
    }

    /***************\
    * IMAGE WRITING *
    \***************/

    if(OnePacketIsLost(sock_recv)){
      /* One packet is lost, all the next image will be coded */
      PARTIAL=0;
      if(j){
	int max=p_padd[j];
	register int b;

	for(b=0; b<max; b++)
	  put_value(chaine_codee, 0x0f, 11, &i, &j);
      }
    }else{
      /* We have to encode the next image header */
      PARTIAL=1;
      if(j!=4){
	int max=n_padd[j];
	register int b;

	for(b=0; b<max; b++)
	  put_value(chaine_codee, 0x0f, 11, &i, &j);
      }
      /* Next image header encoding */
      put_value(chaine_codee,16,20,&i,&j); /* CDI */
    }

    if(UDP){
      static struct sockaddr_in addr_decodeur;
      static int sock, len_addr, one=1;
      u_char ttl=2;

      if(FIRST){
	if((sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
	  fprintf(stderr, "Cannot create datagram socket\n");
	  exit(-1);
	}
	addr_decodeur.sin_port = htons(num_port);
	addr_decodeur.sin_addr.s_addr = (224<<24) | (2<<16) | (2<<8) | 2;
	addr_decodeur.sin_addr.s_addr = htonl(addr_decodeur.sin_addr.s_addr);
	addr_decodeur.sin_family = AF_INET;
	len_addr = sizeof(addr_decodeur);
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
      }

      if((lwrite=sendto(sock, (char *)chaine_codee, i, 0, &addr_decodeur, 
			len_addr)) < 0){
	fprintf(stderr, "sendto: Cannot send data on socket\n");
	exit(-1);
      }
    }else{
	if((lwrite=fwrite((char *)chaine_codee, 1, i, F_out)) != i){
	  fprintf(stderr,
		  "encode: only %d bytes write on file_h261 instead of %d\n", 
		  lwrite, i);
	  exit(-1);
	}
    }
#ifdef DEBUG
    printf("len: %d bytes, %1.2f bit/pixel, ", lwrite, 
	   (float)lwrite/(float)N_PIX);
#endif

    if(!quality){
      /*************\
      * New QUANT ? *
      \*************/

      double time;
      int deltat;
      static int cptq=0;

      gettimeofday(&realtime, (struct timezone *)NULL);
      time = realtime.tv_sec + realtime.tv_usec / 1000000.0;
      deltat=1000*(time-synctime);
      if(deltat > 500){
	/* We have to decrease rate of bits/s */
	INC_QUANT += (QUANT_FIX<15 ? 3 : 0);
	cptq=0;
      }else{
	if(cptq<6)
	    cptq ++;
	else{
	  /* We may increase rate of bits/s */
	  INC_QUANT -= (QUANT_FIX>3 ? 3 : 0);
	  cptq=0;
	}
      }
    }
    /*****************\
    * SNR CALCULATION *
    \*****************/
    if(STATS && !FIRST){
      extern double log10(), sqrt();
      int lmax, cmax, nmax;
      double rms, snr;
      register int lig, col, Perr=0;

      if(CIF){
	lmax=288;
	cmax=352;
      }else{
	lmax=144;
	cmax=176;
      }
      nmax=lmax*cmax;
      for(lig=0; lig<lmax; lig++)
	  for(col=0; col<cmax; col++)
	      Perr += sqr(old[lig][col] - new_image[lig][col]);
      rms = sqrt((double)Perr/(double)nmax);
      snr = 20.0*log10(255.0/rms);
      printf("SNR = %2.2f dB, ", snr);
    }

    cpt ++;

  }



int chain_forecast(tab_code,n_code,result,chain,i,j)
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

    d = n_code - 1;
    g = 0;
    do{
      m = (g+d)/2;
      aux = tab_code[m].result;
      if(result <= aux)
	  d = m - 1;
      if(result >= aux)
	  g = m + 1;
    }while(g <= d);
    if(aux == result){
      register int k=0;
      do{
	register int bit;

	bit = (tab_code[m].chain[k] == '0' ? 0 : 1);
	put_bit(chain,bit,ii,jj);
	k ++;
      }while(tab_code[m].chain[k] != '\0');
      *i=ii; *j=jj;
      return(1);
    }
    *i=ii; *j=jj;
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
    unsigned char run = 0; 
    register int level; 
    register int val, a, k;
    int plus, N=0;
    int first_coeff = 1; /* Boolean */
    int ii = *i, jj = *j;

    for(a=0; a<64; a++){
      if(N==nb_coeff)
	  break;
      if((val=bloc[ZIG[a]][ZAG[a]]) == 0){
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
	  if((level == 1)&&(run == 0)){
	    /* Peculiar case : First coeff is 1 */
	    put_bit(chaine_codee,1,ii,jj);
	    if(plus)
	      put_bit(chaine_codee,0,ii,jj);
	    else
	      put_bit(chaine_codee,1,ii,jj);
	    continue; 
	  }
	}else{
	  /* INTRA mode and CC --> encoding with an uniform quantizer */
	  if(level == 128)
	      level = 255;
	  put_value(chaine_codee, level, 8, &ii, &jj);
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
      *  This code is not an Variable Length Code, so we have to encode it 
      *  with 20 bits.
      *  - 6 bits for ESCAPE (000001)
      *  - 6 bits for RUN
      *  - 8 bits for LEVEL
      */
      for(k=0; k<5; k++)
	  put_bit(chaine_codee,0,ii,jj);
      put_bit(chaine_codee,1,ii,jj);
      for(k=5;k>=0;k--)
	  put_bit(chaine_codee,((run>>k)&0x01),ii,jj);
      if(plus)
	  for(k=7;k>=0;k--)
	      put_bit(chaine_codee,((level>>k)&0x01),ii,jj);
      else{
	put_bit(chaine_codee,1,ii,jj);
	level = 128 - level;
	for(k=6;k>=0;k--)
	    put_bit(chaine_codee,((level>>k)&0x01),ii,jj);
	}
      run = 0;
    }
    /* EOB encoding (10) */
    put_bit(chaine_codee,1,ii,jj);
    put_bit(chaine_codee,0,ii,jj);
    *i=ii; *j=jj;
  }
	
	

int put_value(chain, value, n_bits, i, j) 
    unsigned char *chain;
    int value, n_bits;
    int *i, *j;

  {
    register int aux, l;
    register int ii = *i, jj = *j;


    for(l=n_bits-1; l>=0; l--){
	aux = (value>>l) & 0x01;
	put_bit(chain, aux, ii, jj);
    }
    *i=ii; *j=jj;
  }

