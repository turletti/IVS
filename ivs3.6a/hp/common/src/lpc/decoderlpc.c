#include <memory.h>
#include "lpc.h"
#include "../general_types.h"
#include <stdio.h>

typedef struct {
  u_int16  period;
  u_int8   gain;
  char k[LPC_FILTORDER];
} lpcparams_t;

typedef u_int8 le_buffer[160]; 

int decoder_lpc(indata,outdata,len)
      unsigned char* indata;
      unsigned char* outdata;
      int  len;
{
 int i,l;
 int format=160;
 int j=0;
 char paquetlpc=13;
 int boucle=len/paquetlpc; 
 int taille=0,dec=0;
 le_buffer  buff_out;
 lpcparams_t  param;
 static char prems = 1;   /*TRUE */
 
  if((len%paquetlpc))
    {
      fprintf(stderr, "illegal length\n");
      return 0;
    }
 
  if(!len)
    {
     fprintf(stderr, "be carrefull the length is null\n");
     return 0;  
    }
  if (prems)
   {
    lpc_init(format); 
    prems =0;             /*FALSE*/
   }
 for(l=0;l<boucle;l++)
   {
     memcpy(&param.period, indata+dec,sizeof(int16));
     dec+=sizeof(int16);
     memcpy(&param.gain,indata+dec,sizeof(int8));
     dec+=sizeof(int8);
     memcpy(param.k,indata+dec,LPC_FILTORDER*(sizeof(int8)));
     dec+=(LPC_FILTORDER*sizeof(int8));
     taille+= lpc_synthesize(&param, buff_out);
     for(i=0;i<format;i++,j++) outdata[j]= buff_out[i];
   }
  return taille;
}  

