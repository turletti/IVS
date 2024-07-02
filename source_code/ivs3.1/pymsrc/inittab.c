#include <stdio.h>

#define u_char unsigned char


void InitTable(tab, off)
     int *tab;
     int off;
{
  register int i, j;

  memset((char *)tab, (char)0, 64*sizeof(int));
  tab[4] = 2;
  tab[8] = off << 1;
  tab[12] = tab[8] + 2;
  tab[16] = 4;
  tab[20] = 6;
  tab[24] = tab[8] + 4;
  tab[28] = tab[8] + 6;
  tab[32] = off << 2;
  tab[36] = tab[32] + 2;
  tab[40] = 6*off;
  tab[44] = tab[40] + 2;
  tab[48] = tab[32] + 4;
  tab[52] = tab[32] + 6;
  tab[56] = tab[40] + 4;
  tab[60] = tab[40] + 6;

  fprintf(stderr, "static int tab_offsetXX[] = {\n");
  for(i=0; i<8; i++){
    fprintf(stderr, "\t");
    for(j=0; j<8; j++)
      fprintf(stderr, "%4d, ", *tab++);
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "}\n");
}

main(argc, argv)
     int argc;
     char **argv;

{
  int tab[64];
  int offset = atoi(argv[1]);

  InitTable(tab, offset);
}
