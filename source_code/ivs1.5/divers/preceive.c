#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "in.h"
#include <netdb.h>
#include "protocol.h"

main()
{
  int sock, sock_s; struct in_addr add_in;
  struct sockaddr_in addr, addr_s;
  int len_addr, len_addr_s, lr;
  int type;
  unsigned char buf[50];
  LOCAL_TABLE t;
  BOOLEAN my_video_encoding=TRUE, my_audio_encoding=FALSE;
  char my_name[50];
  u_long my_sinaddr=0;
  struct hostent *host;
  unsigned char ttl=0x02;
  static char port[]="2222";
  static char group[]="224.2.2.2";
  int id, fd[2];
  
  
  /* Initializations */
  InitTable(t);
  strcpy(my_name, "thierry");
  sock = CreateReceiveSocket(&addr, &len_addr, port, group);
  sock_s = CreateSendSocket(&addr_s, &len_addr_s, port, group, &ttl);
  SendDeclarePacket(sock_s, &addr_s, len_addr_s, my_video_encoding, 
			    my_audio_encoding, my_name);

  if(pipe(fd) < 0){
    perror("pipe failed");
    exit(1);
  }

  if(id=fork()){
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
    Audio_Decode(fd, group);
  }
}

