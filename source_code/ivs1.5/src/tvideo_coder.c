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
*  File    : tvideo_coder.c                                                *
*  Date    : Septmeber 1992                                                *
*  Author  : Thierry Turletti                                              *
*--------------------------------------------------------------------------*
*  Description :  Video Coder with quantizer & ffct() choices.             *
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
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include "h261.h"
#include "huffman.h"
#include "huffman_coder.h"
#include "quantizer.h"
#include "protocol.h"

#ifdef VIDEOPIX
#include <vfc_lib.h>

#define NTSC_FIELD_HEIGHT       240     /* Height of a field in NTSC */
#define PAL_FIELD_HEIGHT        287     /* Height of a field in PAL */
#define HALF_YUV_WIDTH          360     /* Half the width of YUV (720/2) */
#define QRTR_YUV_WIDTH          180     /* 1/4 the width of YUV (720/4) */
#define QRTR_PAL_HEIGHT         143     /* 1/4 PAL height */
#define QRTR_NTSC_HEIGHT        120     /* 1/4 NTSC height */
#define CIF_WIDTH               352     /* Width of CIF */
#define QCIF_WIDTH              176     /* Width of Quarter CIF */

#endif

#define MAXBSIZE        8192

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

FILE *F_loss;

    
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
    int num;

    er = sizeof(adr);
    if (recvfrom(sock, (char *)buf, 10, 0, (struct sockaddr *)&adr, &er) < 0){
      perror("receive from");
      exit(0);
    }
    num=(int)buf[0];
    host=gethostbyaddr((char *)(&(adr.sin_addr)), sizeof(adr.sin_addr),
                       adr.sin_family);
#ifdef DEBUG
    if(num)
      fprintf(stderr, "** from %s : packet #%d is lost\n", host->h_name, num);
    else
      fprintf(stderr, "From %s : First image required\n", host->h_name);
#endif
    if(num)
      fprintf(F_loss, "** from %s : packet #%d is lost\n", host->h_name, num);
    else
      fprintf(F_loss, "From %s : First image required\n", host->h_name);
    return(num);
  }


int NbPacketsLost(sock, ForceGobEncoding, GobEncoded, PARTIAL, NG_MAX)
     int sock;
     BOOLEAN ForceGobEncoding[12];
     BOOLEAN GobEncoded[64][12];
     BOOLEAN *PARTIAL;
     int NG_MAX;
  {
    struct timeval tim_out;
    int mask, mask0;
    u_char buf[1];
    BOOLEAN PacketLost[64];
    int num, nb_lost=0;
    int INC_NG=(NG_MAX==12 ? 1 : 2);
    register int i, j;

    for(i=0; i<64; i++)
      PacketLost[i]=FALSE;
    mask = mask0 = 1 << sock;
    tim_out.tv_sec = 0;
    tim_out.tv_usec = 0;
    while(select(sock+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &tim_out) 
       != 0){
      num=ReceivePacket(sock, buf);
      if(!PacketLost[num]){
	PacketLost[num]=TRUE;
	if(num)
	  nb_lost ++;
      }
      mask = mask0;
    }
    if(PacketLost[0]){
      *PARTIAL=FALSE;
    }else{
      *PARTIAL=TRUE;
      if(!nb_lost){
	for(j=0; j<NG_MAX; j+=INC_NG)
	  ForceGobEncoding[j]=FALSE;
      }else{
	for(i=1; i<64; i++)
	  if(PacketLost[i])
	    for(j=0; j<NG_MAX; j+=INC_NG)
	      ForceGobEncoding[j]=GobEncoded[i][j];
#ifdef DEBUG
	for(j=0; j<NG_MAX; j+=INC_NG)
	  if(ForceGobEncoding[j])
	    printf("***** Force %d GOB encoding\n", j);
#endif
      }
    }
    return(nb_lost);
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

void video_encode(group, port, ttl, col, lig, quality, fd, CUTTING, video_port,
		  quantizer, nffct)
     char *group, *port;
     u_char ttl;
     int col, lig; /* image's size */
     int quality; /* Boolean: true if we privilege quality and not rate */
     int fd[2];
     BOOLEAN CUTTING;
     int video_port;
     int quantizer, nffct;

{
    extern int n_mba, n_tcoeff, n_cbc, n_dvm;
    extern int tab_rec1[][255], tab_rec2[][255]; /* for quantization */


    IMAGE_CIF image_coeff;
    u_char new_image[LIG_MAX][COL_MAX];
    int NG_MAX;
    int FIRST=1;
    int CIF=0;
    int sock_recv=0;
    int QUANT_FIX=3;
    int threshold=13;    /* for motion estimation */
    int frequency=2;

    if(lig == 288){
      NG_MAX = 12;
      CIF =1;
    }else
      NG_MAX = 5;

    sock_recv=declare_listen(PORT_VIDEO_CONTROL);

#ifdef VIDEOPIX    

    /* Video in comes from VIDEOPIX */
    {
      VfcDev *vfc_dev;
      int format;

      /* Open the hardware */
      if((vfc_dev = vfc_open("/dev/vfc0", VFC_LOCKDEV)) == NULL){
	fprintf(stderr, "Could not open hardware VideoPix\n");
	exit(1);
      }

      /* Set the input to be Port 1 */
      vfc_set_port(vfc_dev, video_port);
      
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
	  if(ItIsTheEnd(fd)){
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
	    
      if((F_loss=fopen(".videoconf.loss", "w")) == NULL){
	fprintf(stderr, "cannot create videoconf.loss file\n");
	exit(0);
      }

      encode_h261(group, port, ttl, new_image, image_coeff, threshold,
		  NG_MAX, QUANT_FIX, FIRST, frequency, quality, 
		  sock_recv, CUTTING, quantizer, nffct);
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
	encode_h261(group, port, ttl, new_image, image_coeff, threshold, 
		    NG_MAX,  QUANT_FIX, FIRST, frequency, quality, 
		    sock_recv, CUTTING, quantizer, nffct);

	if(ItIsTheEnd(fd)){
	  vfc_destroy(vfc_dev);
	  close(fd[0]);
	  fclose(F_loss);
	  exit(0);
	}
      }
    }
#else
      fprintf(stderr,
	      "VIDEOPIX is not defined so, only input file is allowed\n");
      exit(1);
#endif
  }



encode_h261(group, port, ttl, new_image, image_coeff, threshold, NG_MAX, 
	    QUANT_FIX, FIRST, frequency, quality, sock_recv, CUTTING,
	    quantizer, nffct)
    char *group, *port;
    u_char ttl;
    u_char new_image[][COL_MAX];
    IMAGE_CIF image_coeff;
    int threshold; /* for motion estimation */
    int NG_MAX;
    int QUANT_FIX;
    int FIRST;
    int frequency;
    int quality; /* Boolean: True if we privelege quality and not rate */
    int sock_recv; /* socket used for loss messages */
    BOOLEAN CUTTING;
    int quantizer, nffct;

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
    int i=1, j=0;
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
    int Nblock=0;
    int max_coeff;
    static int cpt=0;
    static int N8x8=300;
    int T_im=1000/frequency;
    static int INC_QUANT=0;
    static int num_packet=0;
    static BOOLEAN PARTIAL=FALSE;
    static BOOLEAN ForceGobEncoding[12];
    static BOOLEAN GobEncoded[64][12];
    BOOLEAN ThisGobEncoded[12];
    BOOLEAN FFCT8x8;
    int nb_lost;
    extern int ffct_4x4();
    extern int ffct_8x8();
    int (*ffct)();
    int nb_packet=1;
    int first_packet=num_packet;
    BOOLEAN DIV2=FALSE;
    QUANT_FIX=quantizer;


    if(nffct == 8){
      FFCT8x8=TRUE;
      ffct = ffct_8x8;
    }else{
      FFCT8x8=FALSE;
      ffct = ffct_4x4;
    }

    chaine_codee[0] = num_packet;
    chaine_codee[1] = (u_char) 0;

    CIF=(NG_MAX==12);
    INC_NG=(CIF ? 1 : 2);

    /*************\
    * IMAGE LAYER *
    \*************/
    gettimeofday(&realtime, (struct timezone *)NULL);
    if(FIRST){
      inittime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      RT=0;
      bzero((char *)image_coeff, sizeof(IMAGE_CIF));
    }else{
      synctime = realtime.tv_sec + realtime.tv_usec/1000000.0;
      RT=((RT+(int)(33*(synctime-inittime))) % 32);
#ifdef DEBUG
      printf("Encoding time: %d ms", (int)(1000*(synctime-inittime)));
#endif
      inittime=synctime;
    }
#ifdef DEBUG
    printf("QUANT: %d, RT: %d\n", QUANT_FIX, RT);
#endif
    put_value(chaine_codee,16,20,&i,&j); /* CDI */
    put_value(chaine_codee,RT,5,&i,&j); /* RT */
    if(CIF)
	put_value(chaine_codee,4,6,&i,&j); /* TYPEI = CIF */
    else
	put_value(chaine_codee,0,6,&i,&j); /* TYPEI = QCIF */
    put_bit(chaine_codee,0,i,j); /* ISI1 */

    for(k=0; k<NG_MAX; k+=INC_NG)
      ThisGobEncoded[k]=TRUE;
    
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

	if(!ForceGobEncoding[NG-1]){
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
		ThisGobEncoded[NG-1]=FALSE;
	      }
	    }
	  } /* MACROBLOCK LOOP */
	} /* if(!ForceGobEncoding[NG-1]) */
      } /* GOB LOOP */
    } /* if(PARTIAL) */
  

    k=1;
    for(NG=1; NG<=NG_MAX; NG+=INC_NG){

      /* GOB LOOP */

      if(!PARTIAL || ThisGobEncoded[NG-1]){
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
	  if(PARTIAL && !ForceGobEncoding[NG-1]){
	    int max;

	    max_coeff=0;
	    if((mb->CBC) & 0x20){
	      max=(*ffct)(new_image, mb->P[0], lig, col, 0, &cmax0);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x10){
	      max=(*ffct)(new_image, mb->P[1], lig, col+8, 0, &cmax1);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x08){
	      max=(*ffct)(new_image, mb->P[2], lig+8, col, 0, &cmax2);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	    if((mb->CBC) & 0x04){
	      max=(*ffct)(new_image, mb->P[3], lig+8, col+8, 0, &cmax3);
	      if(max>max_coeff)
		max_coeff=max;
	    }
	  }else{
	    /* All the blocks are coded */
	    mb->CBC = 60;
	    ffct_4x4(new_image, mb->P[0], lig, col, 0, &cmax0);
	    ffct_4x4(new_image, mb->P[1], lig, col+8, 0, &cmax1);
	    ffct_4x4(new_image, mb->P[2], lig+8, col, 0, &cmax2);
	    ffct_4x4(new_image, mb->P[3], lig+8, col+8, 0, &cmax3);
	    /* Chrominance composantes are not encoded (not decoded) */
	  }
	  if(mb->CBC){
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
		ThisGobEncoded[NG-1]=FALSE;
		break;
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
	} /* MACROBLOCK LOOP */
      }
      {
	int last_AM, CBC, quant=QUANT_FIX;
	register int NM, b;

	if(CUTTING || DIV2){
	  chaine_codee[0] = num_packet;
	  chaine_codee[1] = (u_char)0;
	  i=1;
	  j=0;
	  nb_packet ++;
	  DIV2=FALSE;
	}
	if(NG==7){
	  if(i>3500){
	    DIV2=TRUE;
	  }
	}
	/*********************\
	* H.261 CODING OF GOB *
	\*********************/

	put_value(chaine_codee,1,16,&i,&j); /* CDGB = 1 */
	put_value(chaine_codee,NG,4,&i,&j); 
	put_value(chaine_codee,quant,5,&i,&j);
	put_bit(chaine_codee,0,i,j); /* ISG */

	if(ThisGobEncoded[NG-1]){
	  /******************\
          * MACROBLOCK LAYER *
	  \******************/
	  GobEncoded[num_packet][NG-1]=TRUE;
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
	}else{
#ifdef DEBUG
	  printf("GOB %d, ", NG);
#endif
	}
	if(CUTTING || (NG==NG_MAX) || DIV2){
	  if(ThisGobEncoded[NG-1] || (NG==NG_MAX) || DIV2){
	    static struct sockaddr_in addr;
	    static int sock, len_addr;
	    
	    /* Possible padding */
	    if(j){
	      int max=p_padd[j];
	      register int b;

	      for(b=0; b<max; b++)
		put_value(chaine_codee, 0x0f, 11, &i, &j);
	    }
	    /* Next number packet */
	    if(NG==NG_MAX){
	      if(nb_packet>1)
		fprintf(F_loss, "Image %d: packet%d to %d\n", cpt, 
			first_packet, first_packet+nb_packet);
	      else
		fprintf(F_loss, "Image %d: packet%d\n", cpt, first_packet);
	    }
	    num_packet = (num_packet == 63 ? 1 : num_packet+1);
	    for(b=0; b<NG_MAX; b+=INC_NG)
	      GobEncoded[num_packet][b]=FALSE;
	    /* Sending packet */
	    if(FIRST){
	      sock = CreateSendSocket(&addr, &len_addr, port, group, &ttl);
	      FIRST = 0;
	    }
	    if(i<MAXBSIZE){
	      if((lwrite=sendto(sock, (char *)chaine_codee, i, 0, 
				(struct sockaddr *) &addr, len_addr)) < 0){
		perror("sendto: Cannot send data on socket");
		exit(-1);
	      }
	    }
#ifdef DEBUG
	    printf("GOB %d, NUM_PACKET:%d, NB_BYTES:%d\n", NG, 
		   num_packet, lwrite);
	    if(NG==NG_MAX)
	      printf("Nb(Blocks)= %d\n", Nblock);
#endif
	  }
	}
      } /* Image writing */
    } /* GOB LOOP */
    
    
    nb_lost=NbPacketsLost(sock_recv, ForceGobEncoding, GobEncoded, 
			       &PARTIAL, NG_MAX);
    if(!PARTIAL)
      num_packet=0;

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

