#include "mc_header.h"


void txpkt_print(data)
     char *data;
{
  app_mcct_t *hdr = (app_mcct_t *)data;

  switch(hdr->state) {
  case UNLOADED: printf("U "); break;
  case LOADED:   printf("L "); break;
  case CONGESTED: printf("C "); break;
  default: printf("**Unknown State** ");
  }
  if(hdr->sizeSolicit) printf("s");
  if(hdr->rttSolicit) {
    printf("r");
    printf("%u ",hdr->rttShift);
  }
  
  printf(": %u & ",hdr->keyShift);
  printf("%x %u\n",hdr->key,hdr->maxRtt);
}

void rxpkt_print(data)
     char *data;
{
  app_mccr_t *hdr = (app_mccr_t *)data;


  switch(hdr->state) {
  case UNLOADED: printf("U "); break;
  case LOADED:   printf("L "); break;
  case CONGESTED: printf("C "); break;
  default: printf("**Unknown State** ");
  }
  if(hdr->sizeSolicit) printf("s");
  if(hdr->rttSolicit) {
    printf("r");
    printf("%d.%d ",(u_int)hdr->ts_sec,(u_int)hdr->ts_usec);
    printf("+ %u",hdr->rttDelay);
  }
  printf("\n");
}

