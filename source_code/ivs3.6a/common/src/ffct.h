/**************************************************************************\
*          Copyright (c) 1995 INRIA Sophia Antipolis, FRANCE.              *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* WE MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS     *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
\**************************************************************************/
/**************************************************************************\
* 	                						   *
*  File              : ffct.h       	                		   *
*  Author            : Thierry Turletti					   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Definitions used by ffct() procedures.              *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

#define SL(x) (1<<(x))
#define CT_TYPE int

/* point arithmetic resolution and binary point position */
#define FFCT_RES   10
#define FFCT_FIX   6
#define IFCT_RES   16
#define IFCT_FIX   7

#define FFCTcRES FFCT_RES
#define FFCTxRES FFCT_RES
#define FFCTxSCALE (FFCT_RES-FFCT_FIX)
#define FFCTxFIX   (FFCT_FIX)


#define UnSurRac2    ((int)(          0.70710678118 * SL(FFCTcRES)))
#define UnSur8Rac2   ((int)(0.125 *   0.70710678118 * SL(FFCTcRES)))
#define Rac2Sur8     ((int)(0.250 *   0.70710678118 * SL(FFCTcRES)))
#define UnSur8       ((int)(          0.12500000000 * SL(FFCTcRES)))
#define cf1_0        ((int)(2.0   *   0.98078528040 * SL(FFCTcRES)))
#define cf1_1        ((int)(2.0   *   0.55557023301 * SL(FFCTcRES)))
#define cf1_2        ((int)(2.0   *  -0.19509032201 * SL(FFCTcRES)))
#define cf1_3        ((int)(2.0   *  -0.83146961230 * SL(FFCTcRES)))
#define cf2_0        ((int)(2.0   *   0.92387953251 * SL(FFCTcRES)))
#define cf2_1        ((int)(2.0   *  -0.38268343236 * SL(FFCTcRES)))


#define IFCT_INTER   (13 - 4)
#define IFCTcRES     IFCT_RES
#define IFCTxRES     IFCT_RES
#define IFCT_FACT    (1<<(IFCT_FIX))

#define CF1_0                 ((int)( 0.50979557910 * SL(IFCTcRES)))
#define CF1_1                 ((int)( 0.89997622313 * SL(IFCTcRES)))
#define CF1_2                 ((int)(-2.56291544774 * SL(IFCTcRES)))
#define CF1_3                 ((int)(-0.60134488693 * SL(IFCTcRES)))
#define CF2_0                 ((int)( 0.54119610014 * SL(IFCTcRES)))
#define CF2_1                 ((int)(-1.30656296487 * SL(IFCTcRES)))
#define Rac2                  ((int)( 1.41421356237 * SL(IFCTcRES)))
						      
