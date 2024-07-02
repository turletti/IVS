#define CT_SIZE (1<<(SCALE+1))

#define FIX_I (1<<FIX)
#define SCALE_I(x) (1<<(SCALE+(x)))
#define XSCALE(x) (SCALE+(x))
#define DIFF (SCALE-FIX)
#define DIFF_I (SCALE_I/FIX_I)

#define SCALED_CONST(i,c,s) (((c)*i)>>(SCALE+s))


#define CT_TYPE int

#define UnSurRac2    ((int)( 0.70710678*SCALE_I(0)))
#define UnSur8Rac2   ((int)(  0.088388348*SCALE_I(3)))
#define Rac2Sur8     ((int)(  0.1767767*SCALE_I(2)))
#define cf1_0        ((int)(  1.9615706*SCALE_I(-1)))
#define cf1_1        ((int)(  1.1111405*SCALE_I(-1)))
#define cf1_2        ((int)(-0.39018064*SCALE_I(1)))
#define cf1_3        ((int)(-1.6629392*SCALE_I(-1)))
#define cf2_0        ((int)(  1.8477591*SCALE_I(-1)))
#define cf2_1        ((int)(-0.76536686*SCALE_I(0)))


#define CF1_0 ((int)( 0.50979556891*SCALE_I(0)))
#define CF1_1 ((int)( 0.89997619562*SCALE_I(0)))
#define CF1_2 ((int)(-2.56291547422*SCALE_I(-2)))
#define CF1_3 ((int)(-0.60134489583*SCALE_I(0)))
#define CF2_0 ((int)( 0.54119608990*SCALE_I(0)))
#define CF2_1 ((int)(-1.30656297295*SCALE_I(-1)))
#define Rac2  ((int)( 1.41421356237*SCALE_I(-1)))


