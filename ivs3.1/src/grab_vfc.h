#ifdef GRAB_VFC
#define EXTERN
#else
#define EXTERN extern
#endif
/*-------------------------------------------------------------*/
#define bornCCIR(x) ((x) > 235 ? 235 : ((x) < 16 ? 16 : (x)))

#define GET_UBITS        0x00c00000
#define GET_VBITS        0x00300000
#define NTSC_FIELD_HEIGHT       240     /* Height of a field in NTSC */
#define PAL_FIELD_HEIGHT        287     /* Height of a field in PAL */
#define PAL_FIELD_HEIGHT_DIV2	143
#define HALF_YUV_WIDTH          360     /* Half the width of YUV (720/2) */
#define CIF_WIDTH               352     /* Width of CIF */
#define CIF_HEIGHT              288     /* Height of CIF */
#define QCIF_WIDTH              176     /* Width of Quarter CIF */
#define QCIF_WIDTH_DIV2         88      /* Half Width of Quarter CIF */
#define COL_MAX                 352
#define LIG_MAX                 288
#define LIG_MAX_DIV2            144
#define COL_MAX_DIV2            176

#define Y_CCIR_MAX		240
#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

/*---------------------------------------------------------------*/
#define ZTIME 3

#ifdef TIME_PRINT

#ifdef USER_TIME
#define CLOCK_DCL   UCLOCK_DCL
#define CLOCK       UCLOCK
#define CLOCK_PRINT UCLOCK_PRINT
#else
#define CLOCK_DCL   RCLOCK_DCL
#define CLOCK       RCLOCK
#define CLOCK_PRINT RCLOCK_PRINT
#endif

#define UCLOCK_DCL      unsigned int my_time
#define UCLOCK	       my_time = uclock(&utime, ZTIME)
#define UCLOCK_PRINT(A) printf("\nuser time: %s = %u", A, uclock(&utime, ZTIME)-my_time) 

#define RCLOCK_DCL    struct timeval tp; \
		     struct timezone tzp; \
                             unsigned int time

#define RCLOCK     gettimeofday(&tp, &tzp); \
			       time = tp.tv_sec*1000000 + tp.tv_usec

#define RCLOCK_PRINT(A) gettimeofday(&tp, &tzp); \
             printf("\nreal time: %s = %u", A, tp.tv_sec*1000000 + tp.tv_usec-time)
#else
#define CLOCK_DCL   
#define CLOCK       
#define CLOCK_PRINT
 
#define UCLOCK_DCL   
#define UCLOCK      
#define UCLOCK_PRINT

#define RCLOCK_DCL   
#define RCLOCK      
#define RCLOCK_PRINT
#endif
/*-------------------------------------------------------------*/

#ifdef TIME_PRINT
#ifdef VIDEOPIX_ASYNC

#define WAIT_GRAB_END  {  CLOCK_DCL; CLOCK; \
		  ioctl(vfc_dev->vfc_fd, VFCSCTRL, &wait_ioctl);\
			  CLOCK_PRINT("wait grab end async"); } \
                    /* ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl);*/


#define START_NEXT_GRAB   { CLOCK_DCL; CLOCK; \
		    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &start_ioctl); \
			    CLOCK_PRINT("start next grab"); } 
#else

#define WAIT_GRAB_END  {  CLOCK_DCL; CLOCK; \
			   ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl); \
			    CLOCK_PRINT("wait grab end sync"); } \
                   /*  ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); */

#define START_NEXT_GRAB 

#endif
#else
#ifdef VIDEOPIX_ASYNC

#define WAIT_GRAB_END    ioctl(vfc_dev->vfc_fd, VFCSCTRL, &wait_ioctl); 
                    /*   ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl); */

#define START_NEXT_GRAB   ioctl(vfc_dev->vfc_fd, VFCSCTRL, &start_ioctl);

#else
#define WAIT_GRAB_END   ioctl(vfc_dev->vfc_fd, VFCSCTRL, &grab_ioctl); \
                      /*  ioctl(vfc_dev->vfc_fd, VFCSCTRL, &reset_ioctl);*/
#define START_NEXT_GRAB 
#endif
#endif

/*-------------------------------------------------------------*/

#ifdef GRAB_VFC
#ifdef VIDEOPIX_ASYNC
 int start_ioctl = CAPTRSTR;
 int wait_ioctl  = CAPTRWAIT;
#endif
 int grab_ioctl  = CAPTRCMD;
 int reset_ioctl = MEMPRST;
#else
#ifdef VIDEOPIX_ASYNC
extern int start_ioctl;
#endif
extern int wait_ioctl;
extern int grab_ioctl;
extern int reset_ioctl;
#endif

/* Transform the Cr,Cb videopix output to CCIR signals.
*  Cr, Cb outputs are twos complement values in [-63,63]
*  CCIR are unsigned values in [16, 240] with 128 <-> 0.
*/
EXTERN u_char  COMPUV2CCIR[128]
#ifdef GRAB_VFC
={

  128, 129, 131, 133, 134, 136, 138, 139, 141, 143,
  145, 146, 148, 150, 151, 153, 155, 156, 158, 160,
  162, 163, 165, 167, 168, 170, 172, 173, 175, 177,
  179, 180, 182, 184, 185, 187, 189, 190, 192, 194,
  196, 197, 199, 201, 202, 204, 206, 207, 209, 211,
  213, 214, 216, 218, 219, 221, 223, 224, 226, 228,
  230, 231, 233, 235, 19, 20, 22, 24, 26, 27,
  29, 31, 32, 34, 36, 37, 39, 41, 43, 44,
  46, 48, 49, 51, 53, 54, 56, 58, 60, 61,
  63, 65, 66, 68, 70, 71, 73, 75, 77, 78,
  80, 82, 83, 85, 87, 88, 90, 92, 94, 95,
  97, 99, 100, 102, 104, 105, 107, 109, 111, 112,
  114, 116, 117, 119, 121, 122, 124, 126
  }
#endif
;

EXTERN u_char YTOCCIR[128]
#ifdef GRAB_VFC
={
  16, 17, 19, 21, 22, 24, 26, 28,
  29, 31, 33, 34, 36, 38, 40, 41,
  43, 45, 47, 48, 50, 52, 53, 55,
  57, 59, 60, 62, 64, 66, 67, 69,
  71, 72, 74, 76, 78, 79, 81, 83,
  84, 86, 88, 90, 91, 93, 95, 97,
  98, 100, 102, 103, 105, 107, 109, 110,
  112, 114, 116, 117, 119, 121, 122, 124,
  126, 128, 129, 131, 133, 134, 136, 138,
  140, 141, 143, 145, 147, 148, 150, 152,
  153, 155, 157, 159, 160, 162, 164, 166,
  167, 169, 171, 172, 174, 176, 178, 179,
  181, 183, 184, 186, 188, 190, 191, 193,
  195, 197, 198, 200, 202, 203, 205, 207,
  209, 210, 212, 214, 216, 217, 219, 221,
  222, 224, 226, 228, 229, 231, 233, 235
}
#endif
;

EXTERN u_char  COMPY2CCIR[128]; /* Dynamic Y to CCIR transformation */

