/*
lpc  --  linear predictive audio codec.
 
Ron Frederick (frederic@parc.xerox.com) or Xerox PARC,  Palo Alto,  CA,
contributed this LPC codec which is based on an implementation done by
Ron Zuckerman (ronzu@isu.comm.mot.com) of Motorola which was posted to
the Usenet group comp.dsp on 26 June 1992. The original code is
available as
   ftp://parcftp.xerox.com/pub/net-research/lpc.tar.Z
 
The code was modified for use within NEVOT by H. Schulzrinne and 
within IVS by Hugues Devries.
*/

#include <stdio.h>
#include <math.h>
#include "lpc.h"
  
#define MAXWINDOW	1000	/* Max analysis window length */
#define FS		8000.0	/* Sampling rate */

#define DOWN		5	/* Decimation for pitch analyzer */
#define PITCHORDER	4	/* Model order for pitch analyzer */
#define FC		600.0	/* Pitch analyzer filter cutoff */
#define MINPIT		50.0	/* Minimum pitch */
#define MAXPIT		300.0	/* Maximum pitch */
   
#define MINPER		(int)(FS/(DOWN*MAXPIT)+.5)	/* Minimum period */
#define MAXPER		(int)(FS/(DOWN*MINPIT)+.5)	/* Maximum period */
   
#define WSCALE		1.5863	/* Energy loss due to windowing */
#define sign(x)         ((x< 0) ? -1 : 1)

typedef struct {
  unsigned short period;
  unsigned char gain;
  char k[LPC_FILTORDER];
} lpcparams_t;
static int framelen, buflen;

static float s[MAXWINDOW], y[MAXWINDOW], h[MAXWINDOW];
static float fa[6], u, u1, yp1, yp2; 

static int pitchctr;
static float Oldper, OldG, Oldk[LPC_FILTORDER+1];
static float b[LPC_FILTORDER+1], bp[LPC_FILTORDER+1], f[LPC_FILTORDER+1];

#define BIAS 0x84 	/* define the add-in bias for 16 bit samples */
#define CLIP 32635

/* tableau de 180*3/2  a cause de la fenetre de 180 mais peut etre etendu a 1000 * 3/2 */
static unsigned char lin_to_ulaw(sample)
  int sample;
{
  static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                             4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
  

int signbit, exponent, mantissa;
  unsigned char ulawbyte;

  /* Get the sample into sign-magnitude. */
  signbit = (sample >> 8) & 0x80;		/* set aside the sign */
  if ( signbit != 0 ) sample = -sample;		/* get magnitude */
  if ( sample > CLIP ) sample = CLIP;		/* clip the magnitude */

  /* Convert from 16 bit linear to ulaw. */
  sample = sample + BIAS;
  exponent = exp_lut[( sample >> 7 ) & 0xFF];
  mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
  ulawbyte = ~ ( signbit | ( exponent << 4 ) | mantissa );

  return ulawbyte;
}

static int ulaw_to_lin(ulawbyte)
  unsigned char ulawbyte;
{
  static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
  int signbit, exponent, mantissa, sample;
  ulawbyte = ~ ulawbyte;
  signbit = ( ulawbyte & 0x80 );
  exponent = ( ulawbyte >> 4 ) & 0x07;
  mantissa = ulawbyte & 0x0F;
  sample = exp_lut[exponent] + ( mantissa << ( exponent + 3 ) );
  if ( signbit != 0 ) sample = -sample;

  return sample;
}

static void
auto_correl(w, n, p, r)
  float w[];
  int n;
  int p;
  float r[];
{
  int i, k, nk;

  for (k=0; k <= p; k++) {
    nk = n-k;
    r[k] = 0.0;
    for (i=0; i < nk; i++) r[k] += w[i] * w[i+k];
  }
}

static void
durbin(r, p, k, g)
  float r[];
  int p;
  float k[], *g;
{
  int i, j;
  float a[LPC_FILTORDER+1], at[LPC_FILTORDER+1], e;
  
  for (i=0; i <= p; i++) a[i] = at[i] = 0.0;
    
  e = r[0];
  for (i=1; i <= p; i++) {
    k[i] = -r[i];
    for (j=1; j < i; j++) {
      at[j] = a[j];
      k[i] -= a[j] * r[i-j];
    }
    k[i] /= e;
    a[i] = k[i];
    for (j=1; j < i; j++) a[j] = at[j] + k[i] * at[i-j];
    e *= 1.0 - k[i]*k[i];
  }

  *g = sqrt(e);
}

static inverse_filter(w, k)
  float w[], k[];
{
  int i, j;
  float b[PITCHORDER+1], bp[PITCHORDER+1], f[PITCHORDER+1];
  
  for (i = 0; i <= PITCHORDER; i++) b[i] = f[i] = bp[i] = 0.0;
    
  for (i = 0; i < buflen/DOWN; i++) {
    f[0] = b[0] = w[i];
    for (j = 1; j <= PITCHORDER; j++) {
      f[j] = f[j-1] + k[j] * bp[j-1];
      b[j] = k[j] * f[j-1] + bp[j-1];
      bp[j-1] = b[j-1];
    }
    w[i] = f[PITCHORDER];
  }
}

static calc_pitch(w, per)
  float w[];
  float *per;
{
  int i;
  int j, rpos;
  float d[MAXWINDOW/DOWN], k[PITCHORDER+1], r[MAXPER+1], g, rmax;
  float rval, rm, rp;
  float a, b, c, x, y;
  static int vuv=0;

  for (i=0, j=0; i < buflen; i+=DOWN) d[j++] = w[i];
  auto_correl(d, buflen/DOWN, PITCHORDER, r);
  durbin(r, PITCHORDER, k, &g);
  inverse_filter(d, k);
  auto_correl(d, buflen/DOWN, MAXPER+1, r);
  rpos = 0;
  rmax = 0.0;
  for (i = MINPER; i <= MAXPER; i++) {
    if (r[i] > rmax) {
      rmax = r[i];
      rpos = i;
    }
  }
  
  rm = r[rpos-1];
  rp = r[rpos+1];
  rval = rmax / r[0];
  a = 0.5 * rm - rmax + 0.5 * rp;
  b = -0.5*rm*(2.0*rpos+1.0) + 2.0*rpos*rmax + 0.5*rp*(1.0-2.0*rpos);
  c = 0.5*rm*(rpos*rpos+rpos) + rmax*(1.0-rpos*rpos) + 0.5*rp*(rpos*rpos-rpos);

  x = -b / (2.0 * a);
  y = a*x*x + b*x + c;
  x *= DOWN;

  rmax = y;
  rval = rmax / r[0];
  if (rval >= 0.4 || (vuv == 3 && rval >= 0.3)) {
    *per = x;
    vuv = (vuv & 1) * 2 + 1;
  } else {
    *per = 0.0;
    vuv = (vuv & 1) * 2;      
  }
}

int
lpc_init(len)
  int len;
{
  int i;
  float r, v, w, wcT;
  framelen = len;
  buflen = len*3/2;
  if (buflen > MAXWINDOW) return -1;

  for (i = 0; i < buflen; i++) {
    s[i] = 0.0;
    h[i]=ham[i];
  }
 
  wcT = 2 * M_PI * FC / FS;/* 0.471239;2 * M_PI * FC / FS;*/
  r = 0.173845;            /*0.36891079 * wcT;*/
  v = 0.086923;            /* 0.18445539 * wcT;*/
  w = 0.434990;            /*0.92307712 * wcT;*/
  
  fa[1] = -0.840427; /*-exp(-r);*/
  fa[2] =  0.159573; /*1.0 + fa[1];*/
  fa[3] = -1.662750; /* -2.0 * exp(-v) * cos(w);*/
  fa[4] =  0.840426; /*exp(-2.0 * v);*/
  fa[5] =  0.177677; /* 1.0 + fa[3] + fa[4];*/

  u1 = 0.0;
  yp1 = 0.0;
  yp2 = 0.0;

  Oldper = 0.0;
  OldG = 0.0;
  for (i=1; i <= LPC_FILTORDER; i++) Oldk[i] = 0.0;
  for (i=0; i <= LPC_FILTORDER; i++) b[i] = bp[i] = f[i] = 0.0;
  pitchctr = 0;

  return 0;
}

void
lpc_analyze(buf, params)
  unsigned char *buf;
  lpcparams_t *params;
{
  int i, j;
  float w[MAXWINDOW], r[LPC_FILTORDER+1];
  float per, G, k[LPC_FILTORDER+1];
  
  for (i=0, j=buflen-framelen; i < framelen; i++, j++) {
    s[j] = ulaw_to_lin(buf[i])*0.00003051757;     /* /32768.0 */
    u = fa[2] * s[j] - fa[1] * u1;
    y[j] = fa[5] * u1 - fa[3] * yp1 - fa[4] * yp2;
    u1 = u;
    yp2 = yp1;
    yp1 = y[j];  
  }

  calc_pitch(y, &per);

  for (i=0; i < buflen; i++) w[i] = s[i] * h[i];
  auto_correl(w, buflen, LPC_FILTORDER, r);
  durbin(r, LPC_FILTORDER, k, &G);
  
  

  params->period = per * (1<<8);
  params->gain = G * (1<<8);

  for (i=0; i < LPC_FILTORDER; i++){
    /* transformation of reflection coefficients to L A R (approximation)*/
    
   if ((k[i+1]< 0.675)&&(k[i+1]>(-0.675))) params->k[i]=(int)(k[i+1]*78.769231);
                                         /* normalisation    128 /1.625 = 78.769231*/   
   else if(((k[i+1]<=-0.675)&&(k[i+1]>- 0.950))||((k[i+1]>=0.675)&&(k[i+1]< 0.950)))
	params->k[i]=(int)((sign(k[i+1])*(2*fabs(k[i+1])-0.675))*78.769231);
        else              /*  0.950< |k[i+1]|< 1.000 */
	 params->k[i]=(int)((sign(k[i+1])*(8*fabs(k[i+1])-6.375))*78.769231);
  }
  bcopy(s+framelen, s, (buflen-framelen)*sizeof(s[0]));
  bcopy(y+framelen, y, (buflen-framelen)*sizeof(y[0]));
}

int
lpc_synthesize(params, buf)
     lpcparams_t *params;
     unsigned char *buf;
{
  int i, j;
  static tab=0;
  int flen=framelen;
  float per, G,val, k[LPC_FILTORDER+1];
  float u, NewG, Ginc, Newper, perinc;
  float Newk[LPC_FILTORDER+1], kinc[LPC_FILTORDER+1];

  per = (float)params->period / (1<<8);
  G = (float)params->gain / (1<<8);
  k[0] = 0.0;
  
  for (i=0; i < LPC_FILTORDER; i++) /* k[i+1]=(exp10(val)-1)/(exp10(val)+1);*/
   {
    val=(float)(params->k[i])*0.012695312;  /* 1.625/(1<<7)=0.012695312 */
    if ((val< 0.675)&&(val>- 0.675))
	k[i+1]=val;
    else if (((val<=-0.675)&&(val>- 1.225))||((val>=0.675)&&(val< 1.225)))
	  k[i+1] = sign(val)*(0.500*fabs(val)+0.337500);
          else /*1.225<|val|< 1.625*/ k[i+1] = sign(val)*(0.125*fabs(val)+0.796875);
   }
  if (per == 0.0) {
    G /= sqrt(buflen/3.0);
  } else {
    i = buflen / per;
    if (i == 0) i = 1;
    G /= sqrt((float)i);
  }
  Newper = Oldper;
  NewG = OldG;
  for (i=1; i <= LPC_FILTORDER; i++) Newk[i] = Oldk[i];
    
  if (Oldper != 0 && per != 0) {
    perinc = (per-Oldper) / flen;
    Ginc = (G-OldG) / flen;   
    for (i=1; i <= LPC_FILTORDER; i++) kinc[i] = (k[i]-Oldk[i]) / flen;
  } else {
    perinc = 0.0;
    Ginc = 0.0;
    for (i=1; i <= LPC_FILTORDER; i++) kinc[i] = 0.0;
  }
    
  if (Newper == 0) pitchctr = 0;
    
  for (i=0; i < flen; i++) {
    if (Newper == 0) {
     
      tab=((tab++)%1000);
      u=(rando[tab]* NewG);     /* u = (random()/2147483647.0) * NewG; */ 
    } else {
      if (pitchctr == 0) {
        u = NewG;
        pitchctr = (int) Newper;
      } else {
        u = 0.0;
        pitchctr--;
      }
    }
      
    f[LPC_FILTORDER] = u;
    for (j=LPC_FILTORDER; j >= 1; j--) {
      f[j-1] = f[j] - Newk[j] * bp[j-1];
      b[j] = Newk[j] * f[j-1] + bp[j-1];
      bp[j] = b[j];
    }
    b[0] = bp[0] = f[0];   
    buf[i] = lin_to_ulaw((int) (b[0]* 32768.0));  /* <<15*/
    
    Newper += perinc;
    NewG += Ginc;
    for (j=1; j <= LPC_FILTORDER; j++) Newk[j] += kinc[j];
  }
  
  Oldper = per;
  OldG = G;
  for (i=1; i <= LPC_FILTORDER; i++)   Oldk[i] = k[i];   
  return flen;
}

