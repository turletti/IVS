#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "in.h"
#include <netdb.h>
#include <fcntl.h>
#include "protocol.h"


Usage(name)
char *name;
  {
    fprintf(stderr,
	   "Usage : %s [-i @IP] [-n name] [-t time] [-c]\n", name);
    exit(1);
  }




main(argc, argv)
     int argc;
     char **argv;
{
  int narg;
  int sock_a, sock_c; 
  struct sockaddr_in addr_a, addr_c;
  int len_addr_a, len_addr_c, lr;
  int type;
  unsigned char buf[50];
  LOCAL_TABLE t;
  BOOLEAN my_video_encoding=FALSE, my_audio_encoding=TRUE;
  char my_name[L_cuserid];
  u_long my_sinaddr=0;
  struct hostent *host;
  unsigned char ttl=0x02;
  BOOLEAN new_ip_group=FALSE;
  char temp[256];
  char AUDIO_PORT[5], VIDEO_PORT[5], CONTROL_PORT[5];
  char IP_GROUP[16];
  FILE *F_aux;
  int id, status, fd[2];
  BOOLEAN COMPRESS_SOUND=FALSE;
  int time=20;

  cuserid(my_name);
  if((argc > 6))
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
	if(strcmp(argv[narg], "-c") == 0){
	  COMPRESS_SOUND = TRUE;
	  narg ++;
	  continue;
	}else{
	  if(strcmp(argv[narg], "-t") == 0){
	    time = atoi(argv[++narg]);
	    narg ++;
	    continue;
	  }else
	    Usage(argv[0]);
	}
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
  sock_c = CreateSendSocket(&addr_c, &len_addr_c, CONTROL_PORT, IP_GROUP);
  sock_a = CreateSendSocket(&addr_a, &len_addr_a, AUDIO_PORT, IP_GROUP, 
			    &ttl);
  SendDeclarePacket(sock_c, &addr_c, len_addr_c, my_video_encoding, 
			    my_audio_encoding, my_name);

  if(pipe(fd) < 0){
    fprintf(stderr, "pipe failed\n");
    exit(1);
  }

  if(!(id=fork())){

    struct soundbuf netbuf;
#define buf netbuf.buffer.buffer_val
      int compressing;  /* Compress sound buffers */
      int squelch = 50; 	/* Squelch level if > 0 */
      int ring = 0; /* Force speaker & level on next pkt ? */
      int agc = 0;  /* Automatic gain control active ? */
      int rgain = 33;  /* Current recording gain level */
      int debugging = 0;  /* Debugging enabled here and there ? */
      int loopback = 0;   /* Remote loopback mode */

      netbuf.header = 0x80;

      if (!soundinit(O_RDONLY /* | O_NDELAY */ )) {
	fprintf(stderr, "Unable to initialise audio.\n");
	exit(1);
      }
      if (agc) {
	soundrecgain(rgain);      /* Set initial record level */
      }
      if (soundrecord()) {
	struct timeval tim;
	int mask, mask0;

	compressing=(COMPRESS_SOUND ? 1 : 0);
	close(fd[1]);
	mask0 = 1 << fd[0];
	tim.tv_sec = 0;
	tim.tv_usec = 0;

	do{
	  int soundel = soundgrab(buf, sizeof buf);
	  unsigned char *bs = (unsigned char *) buf;

	  mask = mask0;
	  if(select(fd[0]+1, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, 
		    &tim) > 0){
	    printf("son: Received End of encoding\n");
	    exit(0);
	  }
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
	      if (sendto(sock_a, (char *)&netbuf, (sizeof(netbuf))- 
			 (8000 - soundel), 0, 
			 (struct sockaddr *) &addr_a, len_addr_a) < 0) {
		perror("sending datagram message");
		exit(0);
	      }
	    }
	  } /*else {
	    usleep(100000L);
	  }*/
	}while(1);
      }
    }else{
      close(fd[0]);
      printf("father is sleeping during %d s...\n", time);
      sleep(time);
      SendPP_QUIT(fd);
      printf("father sends ABORT packet to his son\n");
      SendAbortPacket(sock_c, &addr_c, len_addr_c);
      printf("father broadcasts ABORT packet and exits\n");
      waitpid(id, &status, 0);
    }
}

