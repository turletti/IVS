#include <stdio.h>
#include <sys/types.h>
#include <player.h>

#include "general_types"
#include "protocol.h"

#define COL_MAX  352
#define LIG_MAX  288
#define HALF_COL_MAX (COL_MAX/2)
#define HALF_LIG_MAX (LIG_MAX/2)

FILE *grab_file_fd;
FILE *image_file_fd;
Player *player;
int first_frame;
int n_frames;

/*----------------------------------------------------------*/

open_grab_file(size, color)
int size;
int color;
{

  char fname[20];
switch(size)
  {
  case CIF4_SIZE:
   strcpy(fname, "cif4");
    break;
  case CIF_SIZE:
   strcpy(fname, "cif");
    break;
  case QCIF_SIZE:
   strcpy(fname, "qcif");
    break;
}
  if(color)
    strcat(fname, ".color.grab.");
  else
    strcat(fname, ".bw.grab.");

  sprintf(fname+strlen(fname), "%d", getpid());
  if(NULL == (grab_file_fd = fopen(fname, "w"))){
    fprintf(stderr, "\ncan't open grab file");
    exit (-1);
  }
}
/*----------------------------------------------------------*/
write_grab(ptr, wleft)
register char *ptr;
register int wleft;
{
  register wlen;

    do{
      if(!(wlen = fwrite(ptr, sizeof(char), wleft, grab_file_fd))){
	fprintf(stderr, "\ngrab file write error");
	exit (-1);
      }
    }while(wleft -= wlen);
}
/*----------------------------------------------------------*/

grab_into_file(size, color, new_Y, new_Cr, new_Cb)
int size;
int color;
char new_Y[][LIG_MAX][COL_MAX];
char new_Cb[][HALF_LIG_MAX][HALF_COL_MAX];
char new_Cr[][HALF_LIG_MAX][HALF_COL_MAX];
{

  register char *ptr, *ptr1;
  register int i;

#define STEP 4

switch(size)
  {
  case CIF4_SIZE:
    
    i = LIG_MAX;
    ptr = &new_Y[0][0][0];
    ptr1= &new_Y[1][0][0];
    do{
      write_grab(ptr, COL_MAX);
      ptr += COL_MAX;
      write_grab(ptr1, COL_MAX);
      ptr1 += COL_MAX;
    }while(--i);
    
    i = LIG_MAX;
    ptr = &new_Y[2][0][0];
    ptr1= &new_Y[3][0][0];
    do{
      write_grab(ptr, COL_MAX);
      ptr += COL_MAX;
      write_grab(ptr1, COL_MAX);
      ptr1 += COL_MAX;
    }while(--i);

    if(!color)
      return;
    
    i = HALF_LIG_MAX;
    ptr = &new_Cr[0][0][0];
    ptr1= &new_Cr[1][0][0];
    do{
      write_grab(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      write_grab(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX;
    ptr = &new_Cr[2][0][0];
    ptr1= &new_Cr[3][0][0];
    do{
      write_grab(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      write_grab(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);
    
    i = HALF_LIG_MAX;
    ptr = &new_Cb[0][0][0];
    ptr1= &new_Cb[1][0][0];
    do{
      write_grab(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      write_grab(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX;
    ptr = &new_Cb[2][0][0];
    ptr1= &new_Cb[3][0][0];
    do{
      write_grab(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      write_grab(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    break;

  case CIF_SIZE:
    write_grab(new_Y, COL_MAX*LIG_MAX);
    if(!color)
      return;
    write_grab(new_Cr, HALF_COL_MAX*HALF_LIG_MAX);
    write_grab(new_Cb, HALF_COL_MAX*HALF_LIG_MAX); 
    break;

  case QCIF_SIZE:
    
    i = HALF_LIG_MAX;
    ptr = (char *)new_Y;
    do{
      write_grab(ptr, HALF_COL_MAX);
      ptr += COL_MAX;
    }while(--i);

    if(!color)
      return;

    i = HALF_LIG_MAX/2;
    ptr = (char *)new_Cr;
    do{
      write_grab(ptr, HALF_COL_MAX/2);
      ptr += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX/2;
    ptr = (char *)new_Cb;
    do{
      write_grab(ptr, HALF_COL_MAX/2);
      ptr += HALF_COL_MAX;
    }while(--i);
    break;
}

}
/*----------------------------------------------------------*/
int  init_laser_disk()
{
#define DEFAULT_SPEED 0
  int cc;
  printf("\nenter init laser\n");
  if(!(player = InitializeVideo( "/dev/ttyb", SONY_MODEL, DEFAULT_SPEED))){
    fprintf(stderr, "\ncan't open player device on ttyb\n");
    exit (-1);
  }
  printf("\ninitial frame number? =");
  cc = scanf(" %d", &first_frame);
  printf("\nnumber of frames? =");
  cc = cc && scanf(" %d", &n_frames);
  if(!cc){
    first_frame = 1;
    n_frames = 100;
  }
  
  ShowSonyFrame(first_frame);
/*  PlayerFrameNumber(player, 1);*/
  printf("\npass init laser\n");
}
/*----------------------------------------------------------*/
int  init_frame_rate()
{

  int cc;
  do{
  printf("\nframe rate n (take 1 of n)? =");
  }while(!scanf(" %d", &cc));
  return cc;
}
/*----------------------------------------------------------*/
int next_player_frame()
{
  if(--n_frames<=0){
    close(grab_file_fd);
    exit(0);
  }
  PlayerStepForward(player);
}
/*----------------------------------------------------------*/
open_image_file(size, color)
int size;
int color;
{

  char fname[40] = {"IMAGE."};

switch(size)
  {
  case CIF4_SIZE:
   strcat(fname, "cif4");
    break;
  case CIF_SIZE:
   strcat(fname, "cif");
    break;
  case QCIF_SIZE:
   strcat(fname, "qcif");
    break;
}
  if(NULL == (image_file_fd = fopen(fname, "r"))){
    fprintf(stderr, "\ncan't open image file\n");
    exit (-1);
  }
  sleep(10);
}
/*----------------------------------------------------------*/
read_image(ptr, wleft)
register char *ptr;
register int wleft;
{
  register wlen;

    do{
      if(!(wlen = fread(ptr, sizeof(char), wleft, image_file_fd))){
	fprintf(stderr, "\nend of file\n");
	sleep(10);
	exit (0);
      }
    }while(wleft -= wlen);
}
/*----------------------------------------------------------*/
image_from_file(size, color, new_Y, new_Cr, new_Cb)
int size;
int color;
char new_Y[][LIG_MAX][COL_MAX];
char new_Cb[][HALF_LIG_MAX][HALF_COL_MAX];
char new_Cr[][HALF_LIG_MAX][HALF_COL_MAX];
{

  register char *ptr, *ptr1;
  register int i;

#define STEP 4

switch(size)
  {
  case CIF4_SIZE:
    
    i = LIG_MAX;
    ptr = &new_Y[0][0][0];
    ptr1= &new_Y[1][0][0];
    do{
      read_image(ptr, COL_MAX);
      ptr += COL_MAX;
      read_image(ptr1, COL_MAX);
      ptr1 += COL_MAX;
    }while(--i);
    
    i = LIG_MAX;
    ptr = &new_Y[2][0][0];
    ptr1= &new_Y[3][0][0];
    do{
      read_image(ptr, COL_MAX);
      ptr += COL_MAX;
      read_image(ptr1, COL_MAX);
      ptr1 += COL_MAX;
    }while(--i);

    if(!color)
      return;
    
    i = HALF_LIG_MAX;
    ptr = &new_Cr[0][0][0];
    ptr1= &new_Cr[1][0][0];
    do{
      read_image(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      read_image(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX;
    ptr = &new_Cr[2][0][0];
    ptr1= &new_Cr[3][0][0];
    do{
      read_image(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      read_image(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);
    
    i = HALF_LIG_MAX;
    ptr = &new_Cb[0][0][0];
    ptr1= &new_Cb[1][0][0];
    do{
      read_image(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      read_image(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX;
    ptr = &new_Cb[2][0][0];
    ptr1= &new_Cb[3][0][0];
    do{
      read_image(ptr, HALF_COL_MAX);
      ptr += HALF_COL_MAX;
      read_image(ptr1, HALF_COL_MAX);
      ptr1 += HALF_COL_MAX;
    }while(--i);

    break;

  case CIF_SIZE:
    read_image(new_Y, COL_MAX*LIG_MAX);
    if(!color)
      return;
    read_image(new_Cr, HALF_COL_MAX*HALF_LIG_MAX);
    read_image(new_Cb, HALF_COL_MAX*HALF_LIG_MAX); 
    break;

  case QCIF_SIZE:
    
    i = HALF_LIG_MAX;
    ptr = (char *)new_Y;
    do{
      read_image(ptr, HALF_COL_MAX);
      ptr += COL_MAX;
    }while(--i);

    if(!color)
      return;

    i = HALF_LIG_MAX/2;
    ptr = (char *)new_Cr;
    do{
      read_image(ptr, HALF_COL_MAX/2);
      ptr += HALF_COL_MAX;
    }while(--i);

    i = HALF_LIG_MAX/2;
    ptr = (char *)new_Cb;
    do{
      read_image(ptr, HALF_COL_MAX/2);
      ptr += HALF_COL_MAX;
    }while(--i);
    break;
}

}
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/
