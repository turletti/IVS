/*-------------------------------------------------------------*/
#define SKIP_TMP1(port) \
 tmpdata.word.w0 = *(u_int *)port;
#ifndef LDD_SBUS
#define SKIP_TMP2(port) \
 tmpdata.word.w0 = *(u_int *)port;\
 tmpdata.word.w1 = *(u_int *)port
#else
#define SKIP_TMP2(port) \
 tmpdata = *port;
#endif

#define SKIP_TMP4(port)  \
SKIP_TMP2(port); \
SKIP_TMP2(port)
  
#define SKIP_TMP8(port)  \
  SKIP_TMP4(port); \
  SKIP_TMP4(port)
  
#define SKIP_TMP16(port) \
  SKIP_TMP8(port); \
  SKIP_TMP8(port)
/*-------------------------------------------------------------*/

#define   R_COPY16(src, dst, x) \
  *(TYPE_A *)(dst+x)   = *(TYPE_A *)(src+x); \
  *(TYPE_A *)(dst+x+8)   = *(TYPE_A *)(src+x+8);
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#define getY(wi) (*(y2ccir+(tmpdata.word.wi>>25)))
#define getU(wi,s) ((tmpdata.word.wi & GET_UBITS)>>s)
#define getV(wi,s) ((tmpdata.word.wi & GET_VBITS)>>s)

#define tmpUV     u  = getU(w0, 17) | getU(w1, 19);  \
                  v  = getV(w0, 15) | getV(w1, 17)

#define tmpUVor   u |= getU(w0, 21) | getU(w1, 23);  \
                  v |= getV(w0, 19) | getV(w1, 21)

#define putUV(wo, bi) cu.word.wo = *(cuv2ccir+u)<<bi;    \
                      cv.word.wo = *(cuv2ccir+v)<<bi
 
#define putUVor(wi, bi) cu.word.wi |= *(cuv2ccir+u)<<bi; \
                        cv.word.wi |= *(cuv2ccir+v)<<bi

#define dputUVor(wi) u=*(cuv2ccir+u);           \
                     cu.word.wi |= (u<<b2) + u; \
                     v=*(cuv2ccir+v);           \
                     cv.word.wi |= (v<<b2) + v

#define put2UV(wo)     u = *(cuv2ccir+u);    \
                          cu.word.wo = ((u<<8)+u)<<16;  \
                       v  = *(cuv2ccir+v);   \
                      cv.word.wo = ((v<<8)+v)<<16
 
#define put2UVor(wi) u = *(cuv2ccir+u);    \
                        cu.word.wi |= (u<<8)+u;\
                       v  = *(cuv2ccir+v);   \
                        cv.word.wi |= (v<<8)+v

#define putY(wi, wo, bi)   y.word.wo  = getY(wi) <<bi
#define putYor(wi, wo, bi) y.word.wo  |= getY(wi) <<bi
#define dputYor(wi, wo, bi) tmpdata.word.wi = getY(wi); \
    y.word.wo  |= (tmpdata.word.wi<<bi) | (tmpdata.word.wi<<(bi-8))

#define b0 24
#define b1 16
#define b2 8
#define b3 0
/*-------------------------------------------------------------*/
#define GET8Y4(port, wout) \
        SKIP_TMP2(port);      \
        putY(w0, wout, b0);   \
        SKIP_TMP2(port);      \
        putYor(w0, wout, b1); \
			      \
        SKIP_TMP2(port);      \
        putYor(w0, wout, b2); \
        SKIP_TMP2(port);      \
        putYor(w0, wout, b3)
/*-------------------------------------------------------------*/
#define GET6Y4(port, wout) \
        SKIP_TMP2(port);	\
          putY(w0, wout, b0);   \
        putYor(w1, wout, b1);	\
        SKIP_TMP2(port);	\
        putYor(w0, wout, b2);   \
				\
        SKIP_TMP2(port);	\
        putYor(w0, wout, b3)
/*-------------------------------------------------------------*/
#define GET6Y4m(port, wout) \
        SKIP_TMP2(port);	\
          putY(w0, w0, b0);	\
        SKIP_TMP2(port);	\
        putYor(w1, w0, b1);	\
        putYor(w0, w0, b2);     \
				\
        SKIP_TMP2(port);	\
        putYor(w0, w0, b3)
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*                            B&W  CIF                         */
/*-------------------------------------------------------------*/
#define RD8_Y_CIF_PAL_0(port, x)  \
/* 0 - 2 - 4 - 6 -8 - 10 -12 - 14 */          \
        GET8Y4(port, w0);                     \
        GET8Y4(port, w1);                     \
        *(TYPE_A *)(y_data+x) = y

#define RD8_Y_CIF_PAL_1(port, x)  \
/* 16 - 17 - 18 - 20 - 22 - 24 - 26 - 28 */	\
        GET6Y4(port, w0);                       \
        GET8Y4(port, w1);                       \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/

#define RD8_Y_CIF_PAL_2(port, x)  \
  /* 30 - 32 - 34 - 36 - 38 - 40 - 42 - 44 */	      \
        GET8Y4(port, w0);                             \
        GET8Y4(port, w1);                             \
        *(TYPE_A *)(y_data+x) = y	

#define RD8_Y_CIF_PAL_3(port, x)  \
/* 46 - 48 - 49 - 50 -  52 - 54 - 56 - 58 */          \
        GET6Y4m(port, w0);                            \
        GET8Y4(port, w1);                             \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#define QGET8Y2(port, wout, fun, bi) \
        SKIP_TMP2(port);            \
        fun(w0, wout, bi);          \
        SKIP_TMP2(port);	    \
				    \
        SKIP_TMP2(port);	    \
        putYor(w0, wout, (bi-8));   \
        SKIP_TMP2(port)
/*-------------------------------------------------------------*/
#define QGET6Y2(port, wout) \
        SKIP_TMP2(port);	\
          putY(w0, wout, b0);   \
        SKIP_TMP2(port);	\
				\
        SKIP_TMP2(port);	\
        putYor(w0, wout, b1)
/*-------------------------------------------------------------*/
#define QGET6Y2m(port, wout) \
        SKIP_TMP2(port);	\
          putY(w0, wout, b0);	\
        SKIP_TMP2(port);	\
				\
        SKIP_TMP2(port);	\
        putYor(w0, wout, b1)
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*                        B&W  QCIF                            */
/*-------------------------------------------------------------*/
#define RD8_Y_QCIF_PAL_0(port, x)  \
        QGET8Y2(port, w0, putY, b0);                  \
        QGET8Y2(port, w0, putYor, b2);                \
        QGET6Y2(port, w1);                            \
        QGET8Y2(port, w1, putYor, b2);                \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/

#define RD8_Y_QCIF_PAL_1(port, x)  \
        QGET8Y2(port, w0, putY, b0);                  \
        QGET8Y2(port, w0, putYor, b2);                \
        QGET6Y2m(port, w1);                           \
        QGET8Y2(port, w1, putYor, b2);                \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*                    CIF COLOR                                */
/*-------------------------------------------------------------*/
#define CRD8_Y_CIF_PAL_0(port, x)  \
/* 0 - 2 - 4 - 6 */                                   \
        SKIP_TMP2(port);                              \
          putY(w0, w0, b0); tmpUV;                    \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b1); tmpUVor; putUV(w0, b0);   \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b2);	tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b3); tmpUVor; putUVor(w0, b1); \
/* 8 - 10 - 12 - 14 */				      \
        SKIP_TMP2(port);			      \
        putY(w0, w1, b0); tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b1); tmpUVor; putUVor(w0, b2); \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2);	tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3);  tmpUVor; putUVor(w0, b3);\
        *(TYPE_A *)(y_data+x) = y

#define CRD8_Y_CIF_PAL_1(port, x)  \
/* 16 - 17 - 18 - 20 */				      \
						      \
        SKIP_TMP2(port);			      \
          putY(w0, w0, b0); tmpUV;		      \
        putYor(w1, w0, b1);			      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b2); tmpUVor; putUV(w1, b0);   \
						      \
        SKIP_TMP2(port);		tmpUV;	      \
        putYor(w0, w0, b3);			      \
						      \
/* 22 - 24 - 26 - 28 */				      \
						      \
        SKIP_TMP2(port);                              \
        putY(w0, w1, b0);  tmpUVor; putUVor(w1, b1);  \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b1); tmpUV;		      \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2); tmpUVor; dputUVor(w1);    \
                                                      \
        *(TYPE_A *)(u_data)   = cu;                   \
        *(TYPE_A *)(v_data)   = cv;                   \
                                                      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3);	tmpUV;		      \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/

#define CRD8_Y_CIF_PAL_2(port, x)  \
/* 30 - 32 - 34 - 36 */				      \
        SKIP_TMP2(port);			      \
          putY(w0, w0, b0); tmpUVor; putUV(w0, b0);   \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b1); tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b2); tmpUVor; putUVor(w0, b1); \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b3); tmpUV;       	      \
/* 38 - 40 - 42 - 44 */				      \
        SKIP_TMP2(port);			      \
        putY(w0, w1, b0);   tmpUVor; putUVor(w0, b2); \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b1); tmpUV;		      \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2); tmpUVor; putUVor(w0, b3); \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3);	tmpUV;		      \
        *(TYPE_A *)(y_data+x) = y

#define CRD8_Y_CIF_PAL_3(port, x)  \
/* 46 - 48 - 49 - 50 */                               \
						      \
        SKIP_TMP2(port);			      \
          putY(w0, w0, b0); tmpUVor; putUV(w1, b0);   \
        SKIP_TMP2(port);			      \
        putYor(w1, w0, b1);			      \
        putYor(w0, w0, b2); tmpUV;		      \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b3); tmpUVor; putUVor(w1, b1); \
						      \
/*  52 - 54 - 56 - 58 */			      \
						      \
        SKIP_TMP2(port);			      \
        putY(w0, w1, b0);	tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b1); tmpUVor; putUVor(w1, b2); \
                                                      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2); tmpUV;		      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3); tmpUVor; putUVor(w1, b3); \
        *(TYPE_A *)(y_data+x) = y;                    \
        *(TYPE_A *)(u_data+8)   = cu;                 \
        *(TYPE_A *)(v_data+8)   = cv
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*                   QCIF COLOR                                */
/*-------------------------------------------------------------*/
#define CRD8_Y_QCIF_PAL_0(port, x)  \
/* 0 - 2 - 4 - 6 */                                   \
        SKIP_TMP2(port);                              \
          putY(w0, w0, b0); tmpUV;                    \
        SKIP_TMP2(port);			      \
	                    tmpUVor; putUV(w0, b0);   \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b1);                           \
        SKIP_TMP2(port);                              \
/* 8 - 10 - 12 - 14 */                                \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b2); tmpUV;		      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUVor(w0, b1); \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b3);                           \
        SKIP_TMP2(port);			      \
/* 16 - 18 - 20 */       		              \
        SKIP_TMP2(port);			      \
          putY(w0, w1, b0); tmpUV;		      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUVor(w0, b2); \
						      \
        SKIP_TMP2(port);	          	      \
        putYor(w0, w1, b1);			      \
						      \
/* 22 - 24 - 26 - 28 */				      \
						      \
        SKIP_TMP2(port);                              \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2); tmpUV;		      \
						      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUVor(w0, b3); \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3);                           \
        *(TYPE_A *)(y_data+x) = y

/*-------------------------------------------------------------*/

#define CRD8_Y_QCIF_PAL_1(port, x)  \
/* 30 - 32 - 34 - 36 */				      \
        SKIP_TMP2(port);			      \
        SKIP_TMP2(port);			      \
        putY(w0, w0, b0);   tmpUV;		      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUV(w1, b0);   \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b1);                           \
/* 38 - 40 - 42 - 44 */				      \
        SKIP_TMP2(port);			      \
        SKIP_TMP2(port);	    tmpUV;	      \
        putYor(w0, w0, b2);                           \
						      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUVor(w1, b1); \
        SKIP_TMP2(port);			      \
        putYor(w0, w0, b3);                           \
/* 46 - 48 - 49 - 50 */                               \
        SKIP_TMP2(port);			      \
        SKIP_TMP2(port);			      \
        putY(w0, w1, b0);   tmpUV;		      \
						      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b1); tmpUVor; putUVor(w1, b2); \
/*  52 - 54 - 56 - 58 */			      \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b2); tmpUV;		      \
        SKIP_TMP2(port);			      \
                            tmpUVor; putUVor(w1, b3); \
        SKIP_TMP2(port);			      \
        putYor(w0, w1, b3);                           \
        SKIP_TMP2(port);			      \
        *(TYPE_A *)(y_data+x) = y;                    \
        *(TYPE_A *)(u_data)   = cu;                   \
        *(TYPE_A *)(v_data)   = cv
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#define GET4Y4(port, wout) \
        SKIP_TMP2(port);   putY(w0, wout, b0);  \
                         putYor(w1, wout, b1);  \
        SKIP_TMP2(port); putYor(w0, wout, b2);  \
                         putYor(w1, wout, b3)

#define GET4Y4S(port, wout) \
                           putY(w1, wout, b0);  \
        SKIP_TMP2(port); putYor(w0, wout, b1);  \
                         putYor(w1, wout, b2);  \
        SKIP_TMP2(port); putYor(w0, wout, b3);  \

/*-------------------------------------------------------------*/
/*                     CIF4 B&W                                */
/*-------------------------------------------------------------*/
#define GET30Y32(port, y_data, x)    \
/* 0 - 15 */                                  \
        GET4Y4(port, w0);                     \
        GET4Y4(port, w1);                     \
        *(TYPE_A *)(y_data+x) = y;    	      \
        GET4Y4(port, w0);                     \
        SKIP_TMP2(port);   putY(w0, w1, b0);  \
                         putYor(w1, w1, b1);  \
        SKIP_TMP2(port);dputYor(w0, w1, b2);  \
        *(TYPE_A *)(y_data+x+8) = y;          \
/* 16 - 31 */ 				      \
        GET4Y4S(port, w0);                    \
        GET4Y4S(port, w1);                    \
        *(TYPE_A *)(y_data+x+16) = y;	      \
        GET4Y4S(port, w0);                    \
                           putY(w1, w1, b0);  \
        SKIP_TMP2(port); putYor(w0, w1, b1);  \
                        dputYor(w1, w1, b2);  \
        *(TYPE_A *)(y_data+x+24) = y
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#define CGET4Y4(port, wout) \
        SKIP_TMP2(port);   putY(w0, wout, b0);  \
                         putYor(w1, wout, b1);  \
                tmpUV;                          \
        SKIP_TMP2(port); putYor(w0, wout, b2);  \
                         putYor(w1, wout, b3);  \
	        tmpUVor

#define CGET4Y4S(port, wout) \
                           putY(w1, wout, b0);  \
        SKIP_TMP2(port); putYor(w0, wout, b1);  \
                         putYor(w1, wout, b2);  \
                tmpUV;                          \
        SKIP_TMP2(port); putYor(w0, wout, b3);  \
	        tmpUVor

/*-------------------------------------------------------------*/
/*                     CIF4 COLOR                              */
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#define colorGET30Y32(port, y_data, u_data, v_data, x) \
/* 0 - 15 */                                  \
        CGET4Y4(port, w0);                    \
              put2UV(w0);                     \
        CGET4Y4(port, w1);		      \
            put2UVor(w0);                     \
   *(TYPE_A *)(y_data+x) = y;	              \
        CGET4Y4(port, w0);		      \
            put2UV(w1);                       \
        SKIP_TMP2(port);   putY(w0, w1, b0);  \
                         putYor(w1, w1, b1);  \
            tmpUV;                            \
        SKIP_TMP2(port);                      \
            tmpUVor;    dputYor(w0, w1, b2);  \
            put2UVor(w1);                     \
   *(TYPE_A *)(y_data+x+8) = y;               \
   *(TYPE_A *)(u_data+x/2)   = cu;            \
   *(TYPE_A *)(v_data+x/2)   = cv;            \
/* 16 - 31 */ 				      \
        CGET4Y4S(port, w0);                   \
             put2UV(w0);                      \
        CGET4Y4S(port, w1);                   \
           put2UVor(w0);                      \
   *(TYPE_A *)(y_data+x+16) = y;	      \
        CGET4Y4S(port, w0);                   \
           put2UV(w1);                        \
                           putY(w1, w1, b0);  \
        SKIP_TMP2(port); putYor(w0, w1, b1);  \
            tmpUV;      dputYor(w1, w1, b2);  \
   *(TYPE_A *)(y_data+x+24) = y;              \
        SKIP_TMP2(port);   putY(w0, w0, b0);  \
                         putYor(w1, w0, b1);  \
            tmpUVor; put2UVor(w1);            \
        *(TYPE_A *)(u_data+x/2+8)   = cu;     \
        *(TYPE_A *)(v_data+x/2+8)   = cv

#define colorGET30Y32S(port, y_data, u_data, v_data, x) \
/* 0 - 15 */                                  \
      cu.word.w0 = cu.word.w1<<16;            \
      cv.word.w0 = cv.word.w1<<16;            \
        SKIP_TMP2(port); putYor(w0, w0, b2);  \
                         putYor(w1, w0, b3);  \
                tmpUV;                        \
        SKIP_TMP2(port);   putY(w0, w1, b0);  \
                         putYor(w1, w1, b1);  \
            tmpUVor; put2UVor(w0);            \
        SKIP_TMP2(port); putYor(w0, w1, b2);  \
                         putYor(w1, w1, b3);  \
                tmpUV;                        \
   *(TYPE_A *)(y_data+x) = y;	              \
        SKIP_TMP2(port);   putY(w0, w0, b0);  \
                         putYor(w1, w0, b1);  \
            tmpUVor; put2UV(w1);              \
        SKIP_TMP2(port); putYor(w0, w0, b2);  \
                         putYor(w1, w0, b3);  \
                tmpUV;                        \
        SKIP_TMP2(port);   putY(w0, w1, b0);  \
                         putYor(w1, w1, b1);  \
            tmpUVor; put2UVor(w1);            \
        SKIP_TMP2(port);                      \
                tmpUV;  dputYor(w0, w1, b2);  \
   *(TYPE_A *)(y_data+x+8) = y;               \
   *(TYPE_A *)(u_data+x/2)   = cu;            \
   *(TYPE_A *)(v_data+x/2)   = cv;            \
/* 16 - 31 */ 				      \
                           putY(w1, w0, b0);  \
        SKIP_TMP2(port); putYor(w0, w0, b1);  \
            tmpUVor;   put2UV(w0);            \
                         putYor(w1, w0, b2);  \
        SKIP_TMP2(port); putYor(w0, w0, b3);  \
                tmpUV;                        \
                           putY(w1, w1, b0);  \
        SKIP_TMP2(port); putYor(w0, w1, b1);  \
            tmpUVor;   put2UVor(w0);          \
                         putYor(w1, w1, b2);  \
        SKIP_TMP2(port); putYor(w0, w1, b3);  \
                tmpUV;                        \
   *(TYPE_A *)(y_data+x+16) = y;	      \
                           putY(w1, w0, b0);  \
        SKIP_TMP2(port); putYor(w0, w0, b1);  \
            tmpUVor;   put2UV(w1);            \
                         putYor(w1, w0, b2);  \
        SKIP_TMP2(port); putYor(w0, w0, b3);  \
                tmpUV;                        \
                           putY(w0, w1, b0);  \
        SKIP_TMP2(port); putYor(w1, w1, b1);  \
              tmpUVor;  dputYor(w0, w1, b2);  \
              put2UVor(w1);                   \
   *(TYPE_A *)(y_data+x+24) = y;              \
   *(TYPE_A *)(u_data+x/2+8)   = cu;          \
   *(TYPE_A *)(v_data+x/2+8)   = cv




