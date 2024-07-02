#undef EXTERN
#ifdef GRAB_SUNVIDEO
#define EXTERN
#else
#define EXTERN extern
#endif
#define NTSC_WIDTH		640
#define NTSC_HEIGHT		480

#define PAL_WIDTH		768
#define PAL_HEIGHT		576

#define COL_MAX                 352
#define LIG_MAX                 288
#define LIG_MAX_DIV2            144
#define COL_MAX_DIV2            176

#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

#undef EXTERN
