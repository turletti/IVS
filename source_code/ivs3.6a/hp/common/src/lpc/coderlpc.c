#include "lpc.h"
#include "../general_types.h"

typedef struct {
  u_int16 period;
  u_int8 gain;
  char k[LPC_FILTORDER];
} lpcparams_t;

typedef u_int8 le_buffer[160]; 

int coder_lpc(indata,outdata,len)
     unsigned char* indata;
     unsigned char* outdata;
     int len;
{
 int i,l;
 int format=160;
 int j=0;
 int boucle=2; 
 int dec=0;
 static char prems = 1;   /*TRUE */
 le_buffer  buff_in;
 lpcparams_t  param;

 if (prems)
   {
    lpc_init(format); 
    prems =0;             /*FALSE*/
   }
 for(l=0;l<boucle;l++)
   {
     for(i=0;i<format;i++,j++) buff_in[i]=indata[j];
     lpc_analyze(buff_in,&param);
     memcpy(outdata+dec,(char *)&(param.period),sizeof(u_int16));
     dec+=sizeof(u_int16);
     memcpy(outdata+dec,(char *)&(param.gain),sizeof(u_int8));
     dec+=sizeof(u_int8);
     memcpy(outdata+dec,param.k,10/*sizeof(char)*LPC_FILTORDER*/);
     dec+=10; /*LPC_FILTORDER*sizeof(int8)); */
   }
 return dec;  /*  26  */
}  

 

