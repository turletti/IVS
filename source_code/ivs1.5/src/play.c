#include <stdio.h>

main(argc, argv)
     int argc;
     char **argv;
{
  FILE *F_in, *F_out;

  if(argc != 2){
    fprintf(stderr, "Usage tplay filename\n");
    exit(1);
  }
  if((F_in = fopen(argv[1], "r")) == NULL){
    fprintf(stderr, "Cannot open input file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }
  if((F_out = popen("/usr/bin/play -f -", "w")) == NULL){
    fprintf(stderr, "Cannot make play -f -\n");
    perror("popen");
    exit(1);
  }
  do{
    lread=fread(buf, 1, 1024, F_in);
    lwrite=fwrite(buf, 1, lread, F_out);
    if(lread != lwrite)
      fprintf(stderr, "read %d bytes, write only %d bytes\n", lread, lwrite);
  }while(lread == 1024);
  fclose(F_in);
  pclose(F_out);
}
