#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "in.h"
#include <netdb.h>
#include "protocol.h"


Usage(name)
char *name;
  {
    fprintf(stderr,
	   "Usage : %s [-i @IP] [-n name] [-d display] \n", name);
    exit(1);
  }




main(argc, argv)
     int argc;
     char **argv;
{
  int narg;
  int sock, sock_s; 
  struct sockaddr_in addr, addr_s;
  int len_addr, len_addr_s, lr;
  int type;
  unsigned char buf[50];
  LOCAL_TABLE t;
  BOOLEAN my_video_encoding=FALSE, my_audio_encoding=FALSE;
  char my_name[L_cuserid];
  u_long my_sinaddr=0;
  struct hostent *host;
  unsigned char ttl=0x02;
  BOOLEAN new_ip_group=FALSE;
  char temp[256];
  char AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
  char IP_GROUP[16];
  FILE *F_aux;
  int id, fd[2];


  cuserid(my_name);
  if((argc > 7) || (argc % 2 == 0))
    Usage(argv[0]);
  narg = 1;
  while(narg != argc){
    if(strcmp(argv[narg], "-i") == 0){
      strcpy(IP_GROUP, argv[++narg]);
      narg ++;
      new_ip_group = TRUE;
      continue;
    }else{
      if(strcmp(argv[narg], "-n") == 0){
	strcpy(my_name,argv[++narg]);
	narg ++;
	continue;
      }else{
	if(strcmp(argv[narg], "-d") == 0){
	  strcpy(my_name,argv[++narg]);
	  narg ++;
	  continue;
	}else
	  Usage(argv[0]);
      }
    }
  }
    
  if((F_aux=fopen("videoconf_default.var","r")) != NULL){
    fscanf(F_aux, "%s%s", temp, AUDIO_PORT);
    fscanf(F_aux, "%s%s", temp, VIDEO_PORT);
    fscanf(F_aux, "%s%s", temp, CONTROL_PORT);
    if(!new_ip_group)
      fscanf(F_aux, "%s%s", temp, IP_GROUP);
    fclose(F_aux);
  }else{
    fprintf(stderr, "Cannot open input file videoconf_default.var\n");
    exit(-1);
  }

  /* Initializations */
  InitTable(t);
  sock = CreateReceiveSocket(&addr, &len_addr, CONTROL_PORT, IP_GROUP);
  sock_s = CreateSendSocket(&addr_s, &len_addr_s, CONTROL_PORT, IP_GROUP, 
			    &ttl);
  SendDeclarePacket(sock_s, &addr_s, len_addr_s, my_video_encoding, 
			    my_audio_encoding, my_name);

  if(pipe(fd) < 0){
    perror("pipe failed");
    exit(1);
  }

  if(!(id=fork())){
    close(fd[0]);
    do{
      if ((lr = recvfrom(sock, (char *)buf, 1000, 0, (struct sockaddr *)&addr, 
			 &len_addr)) < 0){
	perror("receive from");
	return(-1);
      }

      if((type=GetType(buf)) != CONTROL_TYPE){
	fprintf(stderr, "Received a packet type : %d\n", type);
	continue;
      }else{
	int subtype;
	BOOLEAN video_encoding, audio_encoding;
	char name[100];
	u_short *pt; 
  
	subtype = GetSubType(buf);
	switch(subtype){
        case P_DECLARATION:
	  host=gethostbyaddr((char *)(&(addr.sin_addr)), sizeof(addr.sin_addr),
			     addr.sin_family);
          video_encoding = ((buf[0] & 0x02) ? TRUE : FALSE);
	  audio_encoding = ((buf[0] & 0x01) ? TRUE : FALSE);
	  strcpy(name, (char *) &(buf[1]));
	  strcat(name, "@");
	  strcat(name, host->h_name);
	  if(AddStation(t, name, addr.sin_addr.S_un.S_addr, video_encoding, 
		     audio_encoding)){
	    SendPP_NEW_TABLE(t, fd);
	    printf("sending new table, %s modified ...\n", name);
	  }
	  break; 

        case P_ASK_INFO:
#ifdef DEBUG
	  printf("Received an askInfo packet\n");
#endif
	  SendDeclarePacket(sock_s, &addr_s, len_addr_s, my_video_encoding, 
			    my_audio_encoding, my_name);
	  break;
	  
        case P_ABORT:
	  if(RemoveStation(t, addr.sin_addr.S_un.S_addr)){
	    SendPP_NEW_TABLE(t, fd);
	    printf("sending new table, one station aborted ...\n");
	  }
	  break;

	default:
	  fprintf(stderr, "Received a control packet type : %d\n", subtype);
	}
      }
    }while(1);
  }else{
    close(fd[1]);
    Audio_Decode(fd, AUDIO_PORT, IP_GROUP);
  }
}

