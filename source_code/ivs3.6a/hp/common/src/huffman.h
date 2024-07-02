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
*                                                                          *
*  File              : huffman.h                                           *
*  Last modification : 1995/2/15                                           *
*  Author            : Thierry Turletti                                    *
*--------------------------------------------------------------------------*
*  Description :  Huffman state arrays used by H.261 coder and decoder.    *
*                                                                          *
*                   - state_mba     -> Macroblock address.                 *
*                   - state_typem   -> Type information.                   *
*                   - state_cbc     -> Coded block pattern.                *
*                   - state_dvm     -> Motion vector.                      *
*                   - state_tcoeff  -> DCT coefficient.                    *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name             |    Date   |          Modification              *
*--------------------------------------------------------------------------*
*                         |           | A new result is associated to the  *
*    Thierry Turletti     |  93/3/19  | state_mba table if there are more  *
*                         |           | than 15 zeroes.                    *
*--------------------------------------------------------------------------*
\**************************************************************************/

#undef EXTERN
#ifdef DATA_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif


EXTERN int n_mba
#ifdef DATA_MODULE
=35
#endif
;
EXTERN int n_tcoeff
#ifdef DATA_MODULE
=65
#endif
; 
EXTERN int n_cbc
#ifdef DATA_MODULE
=63
#endif
; 
EXTERN int n_dvm
#ifdef DATA_MODULE
=32
#endif
;

#ifndef NEW_HUFFMAN_TABLES
EXTERN etat state_mba[]
#ifdef DATA_MODULE
= {
      {{1,0},{-99,1}}, {{2,19},{-99,-99}}, {{3,20},{-99,-99}},
      {{4,21},{-99,-99}}, {{5,22},{-99,-99}}, {{6,27},{-99,-99}},
      {{7,38},{-99,-99}}, {{8,16},{-99,-99}}, {{9,-99},{-99,-99}},
      {{10,-99},{-99,-99}}, {{11,-99},{-99,-99}}, {{12,-99},{-99,-99}},
      {{13,-99},{-99,-99}}, {{14,-99},{-99,-99}}, {{15,-99},{-99,-99}},
      {{0,0},{-2,-1}}, {{-99,17},{-99,-99}}, {{-99,18},{-99,-99}},
      {{-99,0},{-99,0}}, {{0,0},{3,2}}, {{0,0},{5,4}},
      {{0,0},{7,6}}, {{24,23},{-99,-99}}, {{0,0},{9,8}},
      {{26,25},{-99,-99}}, {{0,0},{11,10}}, {{0,0},{13,12}},
      {{29,28},{-99,-99}}, {{0,0},{15,14}}, {{33,30},{-99,-99}},
      {{32,31},{-99,-99}}, {{0,0},{17,16}}, {{0,0},{19,18}},
      {{35,34},{-99,-99}}, {{0,0},{21,20}}, {{37,36},{-99,-99}},
      {{0,0},{23,22}}, {{0,0},{25,24}}, {{-99,39},{-99,-99}},
      {{43,40},{-99,-99}}, {{42,41},{-99,-99}}, {{0,0},{27,26}},
      {{0,0},{29,28}}, {{45,44},{-99,-99}}, {{0,0},{31,30}},
      {{0,0},{33,32}}
    }
#endif
;

EXTERN etat state_typem[]
#ifdef DATA_MODULE
= {
      {{1,0},{-99,6}}, {{2,0},{-99,15}}, {{3,0},{-99,9}}, 
      {{4,0},{-99,2}}, {{5,0},{-99,22}}, {{6,0},{-99,31}}, 
      {{7,0},{-99,18}}, {{8,0},{-99,14}}, {{9,0},{-99,8}}, 
      {{-99,0},{-99,30}}
    }
#endif
;

EXTERN etat state_cbc[]
#ifdef DATA_MODULE
= {
      {{1,10},{-99,-99}}, {{6,2},{-99,-99}}, {{3,51},{-99,-99}}, 
      {{5,4},{-99,-99}}, {{0,0},{61,1}}, {{0,0},{62,2}}, 
      {{18,7},{-99,-99}}, {{13,8},{-99,-99}}, {{9,41},{-99,-99}}, 
      {{0,0},{63,3}}, {{23,11},{-99,-99}}, {{12,0},{-99,60}}, 
      {{0,0},{8,4}}, {{16,14},{-99,-99}}, {{34,15},{-99,-99}}, 
      {{0,0},{9,5}}, {{35,17},{-99,-99}}, {{0,0},{10,6}}, 
      {{42,19},{-99,-99}}, {{28,20},{-99,-99}}, {{26,21},{-99,-99}}, 
      {{36,22},{-99,-99}}, {{0,0},{11,7}}, {{24,33},{-99,-99}}, 
      {{37,25},{-99,-99}}, {{0,0},{48,12}}, {{38,27},{-99,-99}}, 
      {{0,0},{49,13}}, {{31,29},{-99,-99}}, {{39,30},{-99,-99}}, 
      {{0,0},{50,14}}, {{40,32},{-99,-99}}, {{0,0},{51,15}}, 
      {{0,0},{32,16}}, {{0,0},{33,17}}, {{0,0},{34,18}}, 
      {{0,0},{35,19}}, {{0,0},{40,20}}, {{0,0},{41,21}}, 
      {{0,0},{42,22}}, {{0,0},{43,23}}, {{0,0},{36,24}}, 
      {{47,43},{-99,-99}}, {{53,44},{-99,-99}}, {{46,45},{-99,-99}}, 
      {{0,0},{37,25}}, {{0,0},{38,26}}, {{48,55},{-99,-99}}, 
      {{49,57},{-99,-99}}, {{-99,50},{-99,-99}}, {{0,0},{39,27}}, 
      {{59,52},{-99,-99}}, {{0,0},{44,28}}, {{60,54},{-99,-99}}, 
      {{0,0},{45,29}}, {{61,56},{-99,-99}}, {{0,0},{46,30}}, 
      {{62,58},{-99,-99}}, {{0,0},{47,31}}, {{0,0},{56,52}}, 
      {{0,0},{57,53}}, {{0,0},{58,54}}, {{0,0},{59,55}}
    }
#endif
;

EXTERN etat state_dvm[]
#ifdef DATA_MODULE
= {
      {{1,0},{-99,0}}, {{2,33},{-99,-99}}, {{3,32},{-99,-99}}, 
      {{4,31},{-99,-99}}, {{5,26},{-99,-99}}, {{6,15},{-99,-99}}, 
      {{-99,7},{-99,-99}}, {{-99,8},{-99,-99}}, {{9,12},{-99,-99}}, 
      {{10,11},{-99,-99}}, {{-99,0},{-99,-16}}, {{0,0},{15,-15}}, 
      {{13,14},{-99,-99}}, {{0,0},{14,-14}}, {{0,0},{13,-13}}, 
      {{16,25},{-99,-99}}, {{17,22},{-99,-99}}, {{18,21},{-99,-99}}, 
      {{19,20},{-99,-99}}, {{0,0},{12,-12}}, {{0,0},{11,-11}}, 
      {{0,0},{10,-10}}, {{23,24},{-99,-99}}, {{0,0},{9,-9}}, 
      {{0,0},{8,-8}}, {{0,0},{7,-7}}, {{27,30},{-99,-99}}, 
      {{28,29},{-99,-99}}, {{0,0},{6,-6}}, {{0,0},{5,-5}}, 
      {{0,0},{4,-4}}, {{0,0},{3,-3}}, {{0,0},{2,-2}}, 
      {{0,0},{1,-1}}
    }
#endif
;

EXTERN etat state_tcoeff[]
#ifdef DATA_MODULE
= {
      {{2,1},{-99,-99}}, {{0,0},{-1,1}}, {{5,3},{-99,-99}}, 
      {{4,0},{-99,17}}, {{0,0},{2,33}}, {{8,6},{-99,-99}}, 
      {{7,52},{-99,-99}}, {{12,0},{-99,3}}, {{9,42},{-99,-99}}, 
      {{17,10},{-99,-99}}, {{49,11},{-99,-99}}, {{0,0},{4,129}}, 
      {{15,13},{-99,-99}}, {{44,14},{-99,-99}}, {{0,0},{5,161}}, 
      {{16,60},{-99,-99}}, {{0,0},{209,6}}, {{18,0},{-99,0}}, 
      {{22,19},{-99,-99}}, {{20,45},{-99,-99}}, {{56,21},{-99,-99}}, 
      {{0,0},{7,35}}, {{33,23},{-99,-99}}, {{29,24},{-99,-99}}, 
      {{27,25},{-99,-99}}, {{26,58},{-99,-99}}, {{0,0},{51,8}}, 
      {{28,47},{-99,-99}}, {{0,0},{9,305}}, {{30,50},{-99,-99}}, 
      {{32,31},{-99,-99}}, {{0,0},{67,10}}, {{0,0},{11,130}}, 
      {{-99,34},{-99,-99}}, {{39,35},{-99,-99}}, {{36,62},{-99,-99}}, 
      {{38,37},{-99,-99}}, {{0,0},{12,417}}, {{0,0},{14,13}}, 
      {{53,40},{-99,-99}}, {{48,41},{-99,-99}}, {{0,0},{22,15}}, 
      {{57,43},{-99,-99}}, {{0,0},{18,81}}, {{0,0},{50,19}}, 
      {{46,55},{-99,-99}}, {{0,0},{20,241}}, {{0,0},{289,21}}, 
      {{0,0},{37,23}}, {{0,0},{34,145}}, {{51,61},{-99,-99}}, 
      {{0,0},{36,114}}, {{0,0},{65,49}}, {{59,54},{-99,-99}}, 
      {{0,0},{83,52}}, {{0,0},{225,66}}, {{0,0},{257,82}}, 
      {{0,0},{113,97}}, {{0,0},{98,273}}, {{0,0},{162,146}}, 
      {{0,0},{193,177}}, {{0,0},{337,321}}, {{64,63},{-99,-99}}, 
      {{0,0},{369,353}}, {{0,0},{401,385}}
    }
#endif
;

#endif
