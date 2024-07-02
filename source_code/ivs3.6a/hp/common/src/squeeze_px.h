typedef struct{
int col;        
int lig;        
int sqzx;      /* squeeze factor in 1/4096 */
int sqzy;      /* squeeze factor in 1/4096 */
int startline;
int stopline;
int x_offset;  /* x pixel display offset */
int y_offset;  /* y pixel display offset */
int noninterlace;
} TYPE_GRAB_WINDOW;

#define SQZE_INTERLACE     0
#define SQZE_NON_INTERLACE 1
#define SQZE_SINGLE        1
#define SQZE_CONTINOUS     0




