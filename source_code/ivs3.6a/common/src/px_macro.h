/*---------------------------------------------------------------------------*/

#define   R_COPY16(src, dst, x) \
  *(TYPE_A *)(dst+x)   = *(TYPE_A *)(src+x); \
  *(TYPE_A *)(dst+x+8)   = *(TYPE_A *)(src+x+8);

/*---------------------------------------------------------------------------*/
#define NOP1 asm ("nop")
#define NOP2 asm ("nop"); asm ("nop");
#define NOP4 asm ("nop"); asm ("nop"); asm ("nop"); asm ("nop")
#define NOP8 NOP4;NOP4
#define NOP16 NOP8; NOP8
#define NOP32 NOP16; NOP16
#define NOP64 NOP32; NOP32

#define NNOP 
#define NNOPmono 
#define NNOPcolor
/*---------------------------------------------------------------------------*/


#define SUN_MAP_mul(v) \
 ((((v&0xff)*65 + ((v>>8)&0xff)*128 + ((v>>16)&0xff)*25)>>8) + 16)


#define STEP 8

#define monoCIF_RD_8_PIX_mul(dst,x,k) \
       px_tmp = *(src_pixel_ptr+k*(x+0));    \
       y.word.w0 = SUN_MAP_mul(px_tmp)<<24;      \
       px_tmp = *(src_pixel_ptr+k*(x+1));    \
       y.word.w0 += SUN_MAP_mul(px_tmp)<<16;     \
       px_tmp = *(src_pixel_ptr+k*(x+2));    \
       y.word.w0 += SUN_MAP_mul(px_tmp)<<8;      \
       px_tmp = *(src_pixel_ptr+k*(x+3));    \
       y.word.w0 += SUN_MAP_mul(px_tmp);         \
                                             \
       px_tmp = *(src_pixel_ptr+k*(x+4));    \
       y.word.w1 = SUN_MAP_mul(px_tmp)<<24;      \
       px_tmp = *(src_pixel_ptr+k*(x+5));    \
       y.word.w1 += SUN_MAP_mul(px_tmp)<<16;     \
       px_tmp = *(src_pixel_ptr+k*(x+6));    \
       y.word.w1 += SUN_MAP_mul(px_tmp)<<8;      \
       px_tmp = *(src_pixel_ptr+k*(x+7));    \
       y.word.w1 += SUN_MAP_mul(px_tmp);         \
       *(TYPE_A*)(dst+(x)) = y

#define monoCIF4_RD_16_PIX_mul(dst ,x) \
        monoCIF_RD_8_PIX_mul(dst, x, CIF4_STEP); \
        monoCIF_RD_8_PIX_mul(dst, (x+8), CIF4_STEP)

#define monoCIF_RD_16_PIX_mul(dst ,x) \
        monoCIF_RD_8_PIX_mul(dst, x, CIF_STEP); \
        monoCIF_RD_8_PIX_mul(dst, (x+8), CIF_STEP)

#define monoQCIF_RD_16_PIX_mul(dst, x) \
        monoCIF_RD_8_PIX_mul(dst, x, QCIF_STEP);  \
        monoCIF_RD_8_PIX_mul(dst, (x+8), QCIF_STEP)
 
/*-------------------------------------------------------------------*/


#define Y    m.word.w0
#define Crb  m.word.w1
#define YCrb m

#define monoSUN_MAP(v) \
Y  = *(u_int *)(map_r + ((v<<3) & 0x7f8));  \
Y += *(u_int *)(map_g + ((v>>5) & 0x7f8));  \
Y += *(u_int *)(map_b + ((v>>13) & 0x7f8))

#define colorSUN_MAP(v) \
YCrb = *(TYPE_A *)(map_r + ((v<<3) & 0x7f8));  \
   x = *(TYPE_A *)(map_g + ((v>>5) & 0x7f8));  \
  Y += x.word.w0; Crb += x.word.w1;	       \
   x = *(TYPE_A *)(map_b + ((v>>13) & 0x7f8)); \
  Y += x.word.w0; Crb += x.word.w1

#define colorCIF_RD_8_PIX_map(wx, x, k) \
        px_tmp = *(src_pixel_ptr+k*(x+0));         \
        colorSUN_MAP(px_tmp);                      \
         y.word.w0 = (Y & 0x3fc)<<(24-SH);         \
        cr.word.wx = (Crb & 0x3fc)<<(24-SH);       \
        cb.word.wx = ((Crb>>(16+SH)) & 0xff)<<24;  \
        px_tmp = *(src_pixel_ptr+k*(x+1));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 |= (Y & 0x3fc)<<(16-SH);        \
        px_tmp = *(src_pixel_ptr+k*(x+2));         \
        colorSUN_MAP(px_tmp);                      \
         y.word.w0 |= (Y & 0x3fc)<<(8-SH);         \
        cr.word.wx |= (Crb & 0x3fc)<<(16-SH);      \
        cb.word.wx |= ((Crb>>(16+SH)) & 0xff)<<16; \
        px_tmp = *(src_pixel_ptr+k*(x+3));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 |= (Y & 0x3fc)>>SH;             \
		                                   \
        px_tmp = *(src_pixel_ptr+k*(x+4));         \
        colorSUN_MAP(px_tmp);                      \
         y.word.w1 = (Y & 0x3fc)<<(24-SH);         \
        cr.word.wx |= (Crb & 0x3fc)<<(8-SH);       \
        cb.word.wx |= ((Crb>>(16+SH)) & 0xff)<<8;  \
        px_tmp = *(src_pixel_ptr+k*(x+5));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 |= (Y & 0x3fc)<<(16-SH);        \
        px_tmp = *(src_pixel_ptr+k*(x+6));         \
        colorSUN_MAP(px_tmp);                      \
         y.word.w1 |= (Y & 0x3fc)<<(8-SH);         \
        cr.word.wx |= (Crb & 0x3fc)>>SH;           \
        cb.word.wx |= ((Crb>>(16+SH)) & 0xff);     \
        px_tmp = *(src_pixel_ptr+k*(x+7));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 |= (Y & 0x3fc)>>SH

#define colorCIF4_RD_16_PIX_map(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_map(w0, (0+x), CIF4_STEP);  \
        *(TYPE_A *)(dst_y+x) = y;                     \
        colorCIF_RD_8_PIX_map(w1, (8+x), CIF4_STEP);  \
        *(TYPE_A *)(dst_y+8+x) = y;                   \
        *(TYPE_A *)(dst_cr+x/2) = cr;                 \
        *(TYPE_A *)(dst_cb+x/2) = cb

#define colorCIF_RD_16_PIX_map(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_map(w0, (0+x), CIF_STEP);  \
        *(TYPE_A *)(dst_y+x) = y;                    \
        colorCIF_RD_8_PIX_map(w1, (8+x), CIF_STEP);  \
        *(TYPE_A *)(dst_y+8+x) = y;                  \
        *(TYPE_A *)(dst_cr+x/2) = cr;                \
        *(TYPE_A *)(dst_cb+x/2) = cb

#define colorQCIF_RD_16_PIX_map(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_map(w0, (0+x), QCIF_STEP);  \
        *(TYPE_A *)(dst_y+x) = y;                     \
        colorCIF_RD_8_PIX_map(w1, (8+x), QCIF_STEP);  \
        *(TYPE_A *)(dst_y+8+x) = y;                   \
        *(TYPE_A *)(dst_cr+x/2) = cr;                 \
        *(TYPE_A *)(dst_cb+x/2) = cb

#define colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, x, k) \
        px_tmp = *(src_pixel_ptr+k*(x+0));         \
        colorSUN_MAP(px_tmp);                      \
        *(dst_y+x+0)  = Y >> SH;                   \
        *(dst_cr+x/2+0) = (Crb >> SH) & 0xff;      \
        *(dst_cb+x/2+0) = (Crb >> (16+SH)) & 0xff; \
        px_tmp = *(src_pixel_ptr+k*(x+1));         \
        monoSUN_MAP(px_tmp);                       \
        *(dst_y+x+1)  = Y >> SH;                   \
        px_tmp = *(src_pixel_ptr+k*(x+2));         \
        colorSUN_MAP(px_tmp);                      \
        *(dst_y+x+2)  = Y >> SH;                   \
        *(dst_cr+x/2+1) = (Crb >> SH) & 0xff;      \
        *(dst_cb+x/2+1) = (Crb >> (16+SH)) & 0xff; \
        px_tmp = *(src_pixel_ptr+k*(x+3));         \
        monoSUN_MAP(px_tmp);                       \
        *(dst_y+x+3)  = Y >> SH;                   \
		                                   \
        px_tmp = *(src_pixel_ptr+k*(x+4));         \
        colorSUN_MAP(px_tmp);                      \
        *(dst_y+x+4)  = Y >> SH;                   \
        *(dst_cr+x/2+2) = (Crb >> SH) & 0xff;      \
        *(dst_cb+x/2+2) = (Crb >> (16+SH)) & 0xff; \
        px_tmp = *(src_pixel_ptr+k*(x+5));         \
        monoSUN_MAP(px_tmp);                       \
        *(dst_y+x+5)  = Y >> SH;                   \
        px_tmp = *(src_pixel_ptr+k*(x+6));         \
        colorSUN_MAP(px_tmp);                      \
        *(dst_y+x+6)  = Y >> SH;                   \
        *(dst_cr+x/2+3) = (Crb >> SH) & 0xff;      \
        *(dst_cb+x/2+3) = (Crb >> (16+SH)) & 0xff; \
        px_tmp = *(src_pixel_ptr+k*(x+7));         \
        monoSUN_MAP(px_tmp);                       \
        *(dst_y+x+7)  = Y >> SH;

#define colorCIF4_RD_16_PIX_byte(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (0+x), CIF4_STEP); \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (8+x), CIF4_STEP)

#define colorCIF_RD_16_PIX_byte(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (0+x), CIF_STEP); \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (8+x), CIF_STEP)

#define colorQCIF_RD_16_PIX_byte(dst_y, dst_cr, dst_cb, x) \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (0+x), QCIF_STEP); \
        colorCIF_RD_8_PIX_byte(dst_y, dst_cr, dst_cb, (8+x), QCIF_STEP)

#define monoCIF_RD_8_PIX_map(x, k) \
        px_tmp = *(src_pixel_ptr+k*(x+0));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 = (Y & 0x3fc)<<(24-SH);         \
        px_tmp = *(src_pixel_ptr+k*(x+1));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 |= (Y & 0x3fc)<<(16-SH);        \
        px_tmp = *(src_pixel_ptr+k*(x+2));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 |= (Y & 0x3fc)<<(8-SH);         \
        px_tmp = *(src_pixel_ptr+k*(x+3));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w0 |= (Y & 0x3fc)>>SH;             \
		                                   \
        px_tmp = *(src_pixel_ptr+k*(x+4));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 = (Y & 0x3fc)<<(24-SH);         \
        px_tmp = *(src_pixel_ptr+k*(x+5));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 |= (Y & 0x3fc)<<(16-SH);        \
        px_tmp = *(src_pixel_ptr+k*(x+6));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 |= (Y & 0x3fc)<<(8-SH);         \
        px_tmp = *(src_pixel_ptr+k*(x+7));         \
        monoSUN_MAP(px_tmp);                       \
         y.word.w1 |= (Y & 0x3fc)>>SH

#define monoCIF_RD_16_PIX_map(dst_y, x)  \
        monoCIF_RD_8_PIX_map((0+x), 1);      \
        *(TYPE_A *)(dst_y+x) = y;        \
        monoCIF_RD_8_PIX_map((8+x), 1);      \
        *(TYPE_A *)(dst_y+8+x) = y

#define monoQCIF_RD_16_PIX_map(dst_y, x)  \
        monoCIF_RD_8_PIX_map((0+x), 2);      \
        *(TYPE_A *)(dst_y+x) = y;        \
        monoCIF_RD_8_PIX_map((8+x), 2);      \
        *(TYPE_A *)(dst_y+8+x) = y

#define monoCIF_RD_1_PIX_byte(dst_y, x, k) \
        px_tmp = *(src_pixel_ptr+k*x);   \
        monoSUN_MAP(px_tmp);             \
        *(dst_y+x)  = Y >> SH

#define monoCIF_RD_8_PIX_byte(dst_y, x, k) \
        monoCIF_RD_1_PIX_byte(dst_y, (x+0), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+1), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+2), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+3), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+4), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+5), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+6), k);   \
        monoCIF_RD_1_PIX_byte(dst_y, (x+7), k)

#define monoCIF4_RD_16_PIX_byte(dst_y, x) \
         monoCIF_RD_8_PIX_byte(dst_y, x, CIF4_STEP);   \
         monoCIF_RD_8_PIX_byte(dst_y, (x+8), CIF4_STEP)
 
#define monoCIF_RD_16_PIX_byte(dst_y, x) \
         monoCIF_RD_8_PIX_byte(dst_y, x, CIF_STEP);   \
         monoCIF_RD_8_PIX_byte(dst_y, (x+8), CIF_STEP)
 
#define monoQCIF_RD_16_PIX_byte(dst_y, x) \
         monoCIF_RD_8_PIX_byte(dst_y, x, QCIF_STEP);   \
         monoCIF_RD_8_PIX_byte(dst_y, (x+8), QCIF_STEP)
 
/*---------------------------------------------------------------------------*/
/*                          NO LOCAL DISPLAY                                 */
/*---------------------------------------------------------------------------*/
#define monoCIF_RD_8_PIX_nld(dst_y, x, k) \
        y.word.w0  = *(src_pixel_ptr+k*(x+0)) << 24;       \
        NNOP;                                              \
        y.word.w0 |= (*(src_pixel_ptr+k*(x+1))&0xff)<< 16; \
        NNOPmono;                                          \
        y.word.w0 |= (*(src_pixel_ptr+k*(x+2))&0xff)<< 8;  \
        NNOPmono;                                          \
        y.word.w0 |= (*(src_pixel_ptr+k*(x+3))&0xff);      \
        NNOPmono;                                          \
                                                           \
        y.word.w1  = *(src_pixel_ptr+k*(x+4)) << 24;       \
        NNOPmono;                                          \
        y.word.w1 |= (*(src_pixel_ptr+k*(x+5))&0xff)<< 16; \
        NNOPmono;                                          \
        y.word.w1 |= (*(src_pixel_ptr+k*(x+6))&0xff)<< 8;  \
        NNOPmono;                                          \
        y.word.w1 |= (*(src_pixel_ptr+k*(x+7))&0xff);      \
        NNOPmono;                                          \
         *(TYPE_A *)(dst_y+x) = y

#define colorCIF_RD_8_PIX_nld(wx, dst_y, dst_cr, dst_cb, x, k)  \
        px_tmp = *(src_pixel_ptr+k*(x+0));                 \
        NOP8;NOP2;                                         \
        y.word.w0  = px_tmp << 24;                         \
       cr.word.wx  = (px_tmp >> 8)<<24;                    \
       cb.word.wx  = (px_tmp >>16)<<24;                    \
        y.word.w0 |= (*(src_pixel_ptr+k*(x+1))&0xff)<< 16; \
        NNOP;                                              \
        px_tmp = *(src_pixel_ptr+k*(x+2));                 \
        NNOPcolor;                                         \
        y.word.w0 |= (px_tmp & 0xff) << 8;                 \
       cr.word.wx |= ((px_tmp >> 8) & 0xff) << 16;         \
       cb.word.wx |= (px_tmp & 0xff0000);                  \
        y.word.w0 |= (*(src_pixel_ptr+k*(x+3))&0xff);      \
        NNOP;                                              \
        px_tmp = *(src_pixel_ptr+k*(x+4));                 \
        NNOPcolor;                                         \
        y.word.w1  = px_tmp << 24;                         \
       cr.word.wx |= ((px_tmp >> 8) & 0xff) <<  8;         \
       cb.word.wx |= ((px_tmp >>16) & 0xff) <<  8;         \
        y.word.w1 |= (*(src_pixel_ptr+k*(x+5))&0xff)<< 16; \
        NNOP;                                              \
        px_tmp = *(src_pixel_ptr+k*(x+6));                 \
        NNOPcolor;                                         \
        y.word.w1 |= (px_tmp & 0xff) << 8;                 \
       cr.word.wx |= ((px_tmp >> 8) & 0xff);               \
       cb.word.wx |= (px_tmp >> 16) & 0xff;                \
        y.word.w1 |= (*(src_pixel_ptr+k*(x+7)) & 0xff);    \
        NNOP

#define  monoCIF4_RD_16_PIX_nld(dst_y, x)  \
         monoCIF_RD_8_PIX_nld(dst_y, (x), CIF4_STEP);     \
         monoCIF_RD_8_PIX_nld(dst_y, (x+8), CIF4_STEP)


#define  monoCIF_RD_16_PIX_nld(dst_y, x)  \
         monoCIF_RD_8_PIX_nld(dst_y, (x), CIF_STEP);     \
         monoCIF_RD_8_PIX_nld(dst_y, (x+8), CIF_STEP)


#define  monoQCIF_RD_16_PIX_nld(dst_y, x)  \
         monoCIF_RD_8_PIX_nld(dst_y, (x), QCIF_STEP);     \
         monoCIF_RD_8_PIX_nld(dst_y, (x+8), QCIF_STEP)


#define  colorCIF4_RD_16_PIX_nld(dst_y, dst_cr, dst_cb, x)  \
         colorCIF_RD_8_PIX_nld(w0, dst_y, dst_cr, dst_cb, (x+0), CIF4_STEP); \
         *(TYPE_A*)(dst_y+x) = y;                                            \
         colorCIF_RD_8_PIX_nld(w1, dst_y, dst_cr, dst_cb, (x+8), CIF4_STEP); \
         *(TYPE_A*)(dst_y+x+8) = y;                                          \
         *(TYPE_A*)(dst_cr+x/2) = cr;                                        \
         *(TYPE_A*)(dst_cb+x/2) = cb

#define  colorCIF_RD_16_PIX_nld(dst_y, dst_cr, dst_cb, x)  \
         colorCIF_RD_8_PIX_nld(w0, dst_y, dst_cr, dst_cb, (x+0), CIF_STEP); \
         *(TYPE_A*)(dst_y+x) = y;                                           \
         colorCIF_RD_8_PIX_nld(w1, dst_y, dst_cr, dst_cb, (x+8), CIF_STEP); \
         *(TYPE_A*)(dst_y+x+8) = y;                                         \
         *(TYPE_A*)(dst_cr+x/2) = cr;                                       \
         *(TYPE_A*)(dst_cb+x/2) = cb

#define  colorQCIF_RD_16_PIX_nld(dst_y, dst_cr, dst_cb, x)  \
         colorCIF_RD_8_PIX_nld(w0, dst_y, dst_cr, dst_cb, (x+0), QCIF_STEP); \
         *(TYPE_A*)(dst_y+x) = y;                                            \
         colorCIF_RD_8_PIX_nld(w1, dst_y, dst_cr, dst_cb, (x+8), QCIF_STEP); \
         *(TYPE_A*)(dst_y+x+8) = y;                                          \
         *(TYPE_A*)(dst_cr+x/2) = cr;                                        \
         *(TYPE_A*)(dst_cb+x/2) = cb

#define monoCIF_RD_8_PIX_byte_nld(dst_y, x, k) \
        *(dst_y+x+0)  = (u_char)*(src_pixel_ptr+k*(x+0));  \
        *(dst_y+x+1)  = (u_char)*(src_pixel_ptr+k*(x+1));  \
        *(dst_y+x+2)  = (u_char)*(src_pixel_ptr+k*(x+2));  \
        *(dst_y+x+3)  = (u_char)*(src_pixel_ptr+k*(x+3));  \
        *(dst_y+x+4)  = (u_char)*(src_pixel_ptr+k*(x+4));  \
        *(dst_y+x+5)  = (u_char)*(src_pixel_ptr+k*(x+5));  \
        *(dst_y+x+6)  = (u_char)*(src_pixel_ptr+k*(x+6));  \
        *(dst_y+x+7)  = (u_char)*(src_pixel_ptr+k*(x+7));

  
#define colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, x, k)   \
        px_tmp = *(src_pixel_ptr+k*(x+0));                 \
        *(dst_y+x+0)  = (u_char)px_tmp;                    \
        *(dst_cr+x/2+0) = (px_tmp >> 8) & 0xff;            \
        *(dst_cb+x/2+0) = (px_tmp >>16) & 0xff;            \
        *(dst_y+x+1)    = (u_char)*(src_pixel_ptr+k*(x+1));\
        px_tmp = *(src_pixel_ptr+k*(x+2));                 \
        *(dst_y+x+2)  = (u_char)px_tmp;                    \
        *(dst_cr+x/2+1) = (px_tmp >> 8) & 0xff;            \
        *(dst_cb+x/2+1) = (px_tmp >>16) & 0xff;            \
        *(dst_y+x+3)  = (u_char)*(src_pixel_ptr+k*(x+3));  \
                                                           \
        px_tmp = *(src_pixel_ptr+k*(x+4));                 \
        *(dst_y+x+4)  = (u_char)px_tmp;                    \
        *(dst_cr+x/2+2) = (px_tmp >> 8) & 0xff;            \
        *(dst_cb+x/2+2) = (px_tmp >>16) & 0xff;            \
        *(dst_y+x+5)    = (u_char)*(src_pixel_ptr+k*(x+5));\
        px_tmp = *(src_pixel_ptr+k*(x+6));                 \
        *(dst_y+x+6)  = (u_char)px_tmp;                    \
        *(dst_cr+x/2+3) = (px_tmp >> 8) & 0xff;            \
        *(dst_cb+x/2+3) = (px_tmp >>16) & 0xff;            \
        *(dst_y+x+7)  = (u_char)*(src_pixel_ptr+k*(x+7))


#define monoCIF4_RD_16_PIX_byte_nld(dst_y, x)   \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+0), CIF4_STEP); \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+8), CIF4_STEP)

#define monoCIF_RD_16_PIX_byte_nld(dst_y, x)   \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+0), CIF_STEP); \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+8), CIF_STEP)

#define monoQCIF_RD_16_PIX_byte_nld(dst_y, x)   \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+0), QCIF_STEP); \
        monoCIF_RD_8_PIX_byte_nld(dst_y, (x+8), QCIF_STEP)


#define colorCIF4_RD_16_PIX_byte_nld(dst_y, dst_cr, dst_cb, x)   \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+0), CIF4_STEP); \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+8), CIF4_STEP)

#define colorCIF_RD_16_PIX_byte_nld(dst_y, dst_cr, dst_cb, x)   \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+0), CIF_STEP); \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+8), CIF_STEP)

#define colorQCIF_RD_16_PIX_byte_nld(dst_y, dst_cr, dst_cb, x)   \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+0), QCIF_STEP); \
        colorCIF_RD_8_PIX_byte_nld(dst_y, dst_cr, dst_cb, (x+8), QCIF_STEP)

/*---------------------------------------------------------------------------*/
#define FY_CIF4_RD_16_PIX(dst, src, x) \
        *(int *)(dst+x+ 0) = *(u_int *)(src+x+ 0); \
        *(int *)(dst+x+ 4) = *(u_int *)(src+x+ 4); \
        *(int *)(dst+x+ 8) = *(u_int *)(src+x+ 8); \
        *(int *)(dst+x+12) = *(u_int *)(src+x+12)


#define FY_CIF_RD_4_PIX(dst, src, x)     \
        m = *(u_int *)(src+2*x+ 0);         \
        tmp = (m & ~0xff) | 0x00ff0000;   \
        y = tmp & (tmp<<8);               \
        m = *(u_int *)(src+2*x+ 4);         \
        tmp = m | 0x00ff0000;             \
        *(int *)(dst+x+ 0) = y | ((tmp>>16) & (tmp>>8))

#define FY_CIF_RD_16_PIX(dst, src, x)     \
        FY_CIF_RD_4_PIX(dst, src, x);     \
        FY_CIF_RD_4_PIX(dst, src, (x+ 4)); \
        FY_CIF_RD_4_PIX(dst, src, (x+ 8)); \
        FY_CIF_RD_4_PIX(dst, src, (x+12))

#define FY_CIF_RD_8_PIX(dst, src, x)     \
        FY_CIF_RD_4_PIX(dst, src, x);     \
        FY_CIF_RD_4_PIX(dst, src, (x+ 4)); \

#define FY_QCIF_RD_4_PIX_G(dst, src, x, s)   \
         *(u_int *)(dst+x+ 0) =               \
            (*(u_char *)(src+s*x+0))<<24    \
         |  (*(u_char *)(src+s*(x+1))<<16)  \
         |  (*(u_char *)(src+s*(x+2))<<8)   \
         |   *(u_char *)(src+s*(x+3))

#define FY_QCIF_RD_16_PIX_G(dst, src, x, s)       \
        FY_QCIF_RD_4_PIX_G(dst, src, x, s);       \
        FY_QCIF_RD_4_PIX_G(dst, src, (x+ 4), s);  \
        FY_QCIF_RD_4_PIX_G(dst, src, (x+ 8), s);  \
        FY_QCIF_RD_4_PIX_G(dst, src, (x+12), s)

#define FY_QCIF_RD_8_PIX_G(dst, src, x, s)       \
        FY_QCIF_RD_4_PIX_G(dst, src, x, s);       \
        FY_QCIF_RD_4_PIX_G(dst, src, (x+ 4), s);  \


#define FY_QCIF_RD_16_PIX(dst, src, x)  FY_QCIF_RD_16_PIX_G(dst, src, x, 4)

#define FCrb_CIF4_RD_16_PIX(dst, src, x)   FY_CIF_RD_16_PIX(dst, src, x)
#define FCrb_CIF_RD_16_PIX(dst, src, x)    FY_QCIF_RD_16_PIX_G(dst, src, x, 4)
#define FCrb_QCIF_RD_8_PIX(dst, src, x)    FY_QCIF_RD_8_PIX_G(dst, src, x, 8)

#define FY_CIF_RD_16_PIX_SQUEEZE(dst, src, x)    FY_CIF4_RD_16_PIX(dst, src, x) 
#define FCrb_CIF_RD_16_PIX_SQUEEZE(dst, src, x)  FY_CIF_RD_16_PIX(dst, src, x)

#define FY_QCIF_RD_16_PIX_SQUEEZE(dst, src, x)    FY_CIF_RD_16_PIX(dst, src, x)
#define FCrb_QCIF_RD_16_PIX_SQUEEZE(dst, src, x)    FY_QCIF_RD_16_PIX_G(dst, src, x, 4)

#define FCrb_QCIF_RD_8_PIX_SQUEEZE(dst, src, x)    \
        FY_QCIF_RD_4_PIX_G(dst, src, x, 4);       \
        FY_QCIF_RD_4_PIX_G(dst, src, (x+ 4), 4)



