#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define SoundFile       "/dev/bba"

static int audiof = -1; 	      /* Audio device file descriptor */


int soundinit(iomode)
  int iomode;
{
    iomode |= O_NDELAY;
    if ((audiof = open(SoundFile, iomode)) >= 0) {
      return TRUE;
    }
    perror(SoundFile);
    return FALSE;
}


void soundplay(len, buf)
  int len;
  unsigned char *buf;
{
    int ios;

    while (TRUE) {
	ios = write(audiof, buf, len);
	if (ios < 0) {
	  fprintf(stderr, "write returns %d\n", ios);
	  
	} else {
	    if (ios < len) {
		buf += ios;
		len -= ios;
	    } else {
		break;
	    }
	}
    }
}

main(argc, argv)
     int argc;
     char **argv;
{     
  char buf[1000];
  FILE *F;
  
  soundinit(O_WRONLY);
  if((F=fopen(argv[1], "r")) == NULL){
    fprintf(stderr, "cannot open %s\n", argv[1]);
    exit(1);
  }
  fread(buf, 1000, 1, F);
  soundplay(1000, buf);
  close(audiof);
}
