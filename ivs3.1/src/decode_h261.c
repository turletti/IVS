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
*  File              : decode_h261.c  	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 93/4/23                                             *
*--------------------------------------------------------------------------*
*  Description       : Decode and display a H.261 encoded file.            *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "h261.h"
#include "huffman.h"
#include "quantizer.h"
#include "protocol.h"

#define streq(a, b)        (strcmp((a), (b)) == 0)

/* Global variables unused for athena.c */

Widget main_form;
Widget form_brightness, form_contrast, button_default;
Widget menu_video[5];
Widget msg_error;
BOOLEAN UNICAST=TRUE;
Pixmap mark;


static XtAppContext app_con;


Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-grey] filename\n", name);
  fprintf(stderr, "Display an H.261 encoded file\n");
  exit(1);
}


main(argc, argv)
    int argc;
    char **argv;
    
{
  int fd[2];
  static char port[5]="2222";
  static char group[10]="224.8.8.8";
  static char name[20]="H261@file";
  char *filename;
  u_long sin_addr=0;
  static char display[10]="";
  BOOLEAN QCIF2CIF=TRUE;
  BOOLEAN STAT_MODE=FALSE;
  BOOLEAN FORCE_GREY;
  u_short control_port=0;
  Widget toplevel;

  if((argc < 2) || (argc > 3)){
    Usage(argv[0]);
  }
  if(argc == 3){
    if(streq(argv[1], "-grey")){
      filename = argv[2];
    }else{
      if(streq(argv[2], "-grey")){
	filename = argv[1];
      }else{
	Usage(argv[0]);
      }
    }
    FORCE_GREY = TRUE;
  }else{
    FORCE_GREY = FALSE;
    filename = argv[1];
  }

  toplevel = XtAppInitialize(&app_con, "decode_h261", NULL, 0, &argc, argv, 
			     NULL, NULL, 0);

  VideoDecode(fd, port, group, filename, sin_addr, display, QCIF2CIF, 
	      STAT_MODE, FORCE_GREY, control_port);
}
