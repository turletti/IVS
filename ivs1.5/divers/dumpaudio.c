#include <stdio.h>
#include "ulaw2linear.h"

main()
{

  register int i;

  printf("********* audio_c2u *********\n");
  for(i=0; i<256; i++){
    printf("%d, ", audio_c2u(i));
    if((i+1)%10 == 0)
      printf("\n");
  }

  printf("\n\n\n********* audio_s2u *********\n");
  for(i=0; i<65536; i++){
    printf("%d, ", audio_s2u(i));
    if((i+1)%10 == 0)
      printf("\n");
  }

  printf("\n\n\n********* audio_u2c *********\n");
  for(i=0; i<256; i++){
    printf("%d, ", audio_u2c(i));
    if((i+1)%10 == 0)
      printf("\n");
  }

  printf("\n\n\n********* audio_u2s *********\n");
  for(i=0; i<65536; i++){
    printf("%d, ", audio_u2s(i));
    if((i+1)%10 == 0)
      printf("\n");
  }

}
