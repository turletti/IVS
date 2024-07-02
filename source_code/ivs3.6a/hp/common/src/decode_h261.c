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
* 	                						   *
*  File              : decode_h261.c  	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
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

#include "general_types.h"
#include "protocol.h"
#include "h261.h"
#include "huffman.h"
#include "quantizer.h"

#define streq(a, b)        (strcmp((a), (b)) == 0)

/* Exported  Widgets */
Widget main_form;
Widget msg_error;
BOOLEAN UNICAST=TRUE;/*11/10/91*/
/* Global not used variables (for athena module) */
Widget video_coder_panel;
Widget video_grabber_panel;
Widget rate_control_panel;
Widget info_panel;
Widget form_control, scroll_control, value_control;
Widget button_popup1, button_popup2, button_popup3, button_popup4;
Widget form_frms, form_format;
Widget label_band2, label_ref2;
Pixmap mark;
S_PARAM param;

static XtAppContext app_con;


Usage(name)
     char *name;
{
  fprintf(stderr, "Usage: %s [-grey] filename [-d display] \n", name);
  fprintf(stderr, "Display an H.261 encoded file\n");
  exit(1);
}


main(argc, argv)
    int argc;
    char **argv;
    
{
  int fd[2], narg;
  static char port[5]="2222";
  static char group[10]="224.8.8.8";
  static char name[20]="H261file";
  char *filename;
  u_long sin_addr=0;
  static char display[100]="";
  BOOLEAN QCIF2CIF=TRUE;
  BOOLEAN STAT_MODE=FALSE;
  BOOLEAN FORCE_GREY=FALSE;
  BOOLEAN NO_FILENAME=TRUE;
  u_short control_port=0;
  char null=0;
  Widget toplevel;

  if((argc < 2) || (argc > 5)){
    Usage(argv[0]);
  }
  narg = 1;
  while(narg != argc){
    if(streq(argv[narg], "-grey")){
      narg++;
      FORCE_GREY = TRUE;
      continue;
    }else{
      if(streq(argv[narg], "-d")){
	if(++narg == argc)
	  Usage(argv[0]);
	strcpy(display, argv[narg++]);
	continue;
      }else{
	if(argv[narg][0] == '-')
	  Usage(argv[0]);
	else{
	  filename = argv[narg];
	  NO_FILENAME=FALSE;
	  narg++;
	}
      }
    }
  }

  if(NO_FILENAME)
    Usage(argv[0]);

  toplevel = XtAppInitialize(&app_con, "decode_h261", NULL, 0, &argc, argv, 
			     NULL, NULL, 0);

  VideoDecode(fd, port, group, filename, sin_addr, sin_addr, 0, &null, 
	      control_port, display, QCIF2CIF, STAT_MODE, FORCE_GREY, 
	      UNICAST); /* 95/5/29 */
}
