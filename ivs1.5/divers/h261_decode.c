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
* 	                						   *
*  File    : h261_decode.c    	                			   *
*  Date    : May 1992       						   *
*  Author  : Thierry Turletti						   *
*  Release : 1.0.0						           *
*--------------------------------------------------------------------------*
*  Description : H.261 decoder program.                                    *
*                                                                          *
\**************************************************************************/


#include <sys/wait.h>
#include <sys/time.h>
#include "h261.h"
#include "huffman.h"
#include "quantizer.h"


Display *dpy;
GC gc;
Window win;
XImage *ximage;
unsigned int depth;
u_char map_lut[256]; 
XColor colors[256];
int fd[2];
int fdd[2];
int N_LUTH=256; 

usage(name)
char *name;
  {
    fprintf(stderr,
	   "Usage : %s [-p num_port | -f filename] [-d display] [-o]\n", name);
    exit(1);
  }
    
main(argc, argv)
int argc;
char **argv;
  {
    u_char im_pix[101376];
    int NB=1; /* Boolean : True if Grayscale mode */
    int BCH=0; 
    int TCP=1; 
    int id, idd, narg;
    u_char format[4];
    FILE *F_in, *F_aux;
    int lread, L_lig, L_col;
    static char filename[40]="";
    static char display[20]="";
    int num_port;
    int QCIF2CIF=1; /* Boolean : True if CIF display */
    char temp[40];

    
    if(argc > 6)
	usage(argv[0]);

    if((F_aux=fopen("h261_default.var","r"))!=NULL){
      fscanf(F_aux, "%s%i", temp, &num_port);
      fclose(F_aux);
    }
    narg = 1;
    while(narg != argc){
      if(strcmp(argv[narg],"-o") == 0){
	QCIF2CIF=0;
	narg ++;
      }else{
	if((argc-narg)<2)
	    usage(argv[0]);
	if(strcmp(argv[narg],"-d") == 0){
	  strcpy(display, argv[++narg]);
	  narg ++;
	}else{
	  if(strcmp(argv[narg],"-p") == 0){
	    num_port = atoi(argv[++narg]);
	    narg ++;
	  }else{
	    if(strcmp(argv[narg],"-f") == 0){
	      TCP=0;
	      strcpy(filename, argv[++narg]);
	      narg ++;
	    }else
		usage(argv[0]);
	  }
	}
      }
    }

    if(!TCP){
      /*
      *  CIF or Quarter Common Intermediate Format 
      */
      if((F_in=fopen(filename,"r"))==NULL){
	fprintf(stderr,"%s failed, Cannot open input file %s\n",argv[0],
		filename);
	exit(2);
      }
      lread = fread((char *)format, 1, 4, F_in);
      if(lread != 4){
	fprintf(stderr,"%s failed, Bad first header value\n",argv[0]);
	exit(3);
      }
      if((format[3]>>((BCH ? 1 : 3)&0x01)) || (QCIF2CIF)){ /* FORMAT CIF */
	L_col = 352;
	L_lig = 288;
      }
      else{ /* FORMAT QCIF */
	L_col = 176;
	L_lig = 144;
      }
      fclose(F_in);
      
      if(pipe(fd)<0){
	fprintf(stderr,"%s : error pipe Luth \n");
	exit(1);
      }
      
      if(pipe(fdd)<0){
	fprintf(stderr,"%s : error pipe Control\n");
	exit(1);
      }

      init_window(L_lig, L_col, NB, filename, map_lut, &depth, display);
    
      if((id=fork()) == -1){
	fprintf(stderr,"%s : Cannot fork !\n");
	exit(1);
      }
      if(id){
	u_char com;
	struct timeval tim_out;
	int mask;
	
	close(fd[1]);
	close(fdd[1]);
	mask = 1<<fdd[0];
	tim_out.tv_sec = 0;
	tim_out.tv_usec = 0;
	while(read(fdd[0], (char *)&com, 1) == 1){
	  mask = 1<<fdd[0];
	  if(com==PLAY){
	    if((idd=fork()) == -1){
	      fprintf(stderr,"%s : Cannot fork !\n",argv[0]);
	      exit(1);
	    }
	    if(!idd)
		decode(im_pix, filename, state_mba, state_typem, state_cbc, 
		       state_dvm, state_tcoeff, tab_rec1, tab_rec2, map_lut, 
		       NB, BCH, TCP, display, num_port, QCIF2CIF);
	  }else{
	    if(com==LOOP){
	      int status;

	      mask = 1<<fdd[0];
	      while(select(fdd[0]+1, (fd_set *)&mask, (fd_set *)0, 
				(fd_set *)0, &tim_out) <= 0){
		if((idd=fork()) == -1){
		  fprintf(stderr,"%s : Cannot fork !\n", argv[0]);
		  exit(1);
		}
		if(!idd)
		    decode(im_pix, filename, state_mba, state_typem, 
			   state_cbc, state_dvm, state_tcoeff, tab_rec1, 
			   tab_rec2, map_lut, NB, BCH, TCP, display, num_port,
			   QCIF2CIF);
		else
		    waitpid(idd, &status, 0);
		mask = 1<<fdd[0];
	      }
	    }else
		if(com==QUIT)
		    return;
	  }
	}
      }else{
	close(fd[0]);
	close(fdd[0]);
	New_Luth(TCP);
      }
    }else{
      int status;

      while(1){
	if((id=fork()) == -1){
	  fprintf(stderr,"%s : Cannot fork !\n");
	  exit(1);
	}
	if(!id){
	  /* son's process is going to decode */
	  decode(im_pix, argv[0], state_mba, state_typem, state_cbc, 
		 state_dvm, state_tcoeff, tab_rec1, tab_rec2, map_lut, 
		 NB, BCH, TCP, display, num_port, QCIF2CIF);
	}else
	    /* father's process wait his son */
	    waitpid(id, &status, 0);
      }
    }
  }

 
