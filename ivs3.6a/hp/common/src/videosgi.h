/**************************************************************************\
*          Copyright (c) 1995 by the computer center			   *
*				 university of Stuttgart		   *
*				 Germany           			   *
*                                                                          *
*  						                           *
*                                                                          *
* 	                						   *
*  File              : videosgi.h 					   *
*				                                           *
*  Author            : Lei Wang, computer center			   *
*				 university of Stuttgart		   *
*				 Germany  				   *
*									   *
*  Description	     : Library for videosgi.c, videosgi_grab.c,            *
*		       videogalileo.c, videogalileo_grab.c   		   *  
*									   *
*--------------------------------------------------------------------------*
*        Name		   |    Date   |          Modification             *
*--------------------------------------------------------------------------*
* Lei Wang                 |  93/09/15 |          Ver 1.0                  *
* wang@rus.uni-stuttgart.de|	       |				   *
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

#define DEFAULT_SAMPLING_RATE	2
#define TRANSFER_RATE_GALILEO   40

#define	X_OFFSET		128
#define X_OFFSET_CIF4		1536



