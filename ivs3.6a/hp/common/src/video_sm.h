/**************************************************************************\
*									   *
*          Copyright (c) 1995 Frank Mueller,nig@nig.rhoen.de               *
*                                                                          *
* Permission to use, copy, modify, and distribute this material for any    *
* purpose and without fee is hereby granted, provided that the above       *
* copyright notice and this permission notice appear in all copies.        *
* 								           *
* I MAKE NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS      *
* MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS   *
* OR IMPLIED WARRANTIES.                                                   *
***************************************************************************/
/*          		                                                   *
*                                                                          *
*  						                           *
*                                                                          *
* 	                						   *
*  File              : video_sm.h 					   *
*				                                           *
*  Author            : Frank Mueller,nig@nig.rhoen.de,			   *
*		       Fachhochschule Fulda       			   *
*				  					   *
*									   *
*  Description	     : header-files for video_sm.c			   *
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Frank Mueller            |  95/04/01 |          Ver 1.0                  *
* nig@nig.rhoen.de 	   |	       |				   *
\**************************************************************************/

#define COL_MAX			352	/* Max number of lines and columns */
#define LIG_MAX			288

#define COL_MAX_DIV2		176	
#define LIG_MAX_DIV2		144


#define SCIF_WIDTH              704     /* Width and height of SCIF */
#define SCIF_HEIGHT		576

#define CIF4_WIDTH              704     /* Width and height of CIF4 */
#define CIF4_HEIGHT		576

#define SCIF_WIDTH_DIV2         352
#define SCIF_HEIGHT_DIV2	288

#define CIF4_WIDTH_DIV2         352
#define CIF4_HEIGHT_DIV2        288

#define CIF_WIDTH               352     /* Width and height of CIF */
#define CIF_HEIGHT		288

#define CIF_WIDTH_DIV2          176
#define CIF_HEIGHT_DIV2		144

#define QCIF_WIDTH              176     /* Width and height of Quarter CIF */
#define QCIF_HEIGHT		144

#define QCIF_WIDTH_DIV2         88
#define QCIF_HEIGHT_DIV2	72

#define SCIF_LIG_STEP		3072
#define CIF4_LIG_STEP		3072 
#define CIF_COL_STEP		7
#define CIF_LIG_STEP		3072
#define QCIF_COL_STEP		15
#define QCIF_LIG_STEP		6144			

#define MODCOLOR		4
#define MODGREY			2
#define MODNB			1

#define	X_OFFSET		128
#define X_OFFSET_CIF4		1536



