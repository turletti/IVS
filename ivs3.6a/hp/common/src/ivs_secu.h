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
*  File              : ivs_secu.h      	                		   *
*  Author            : Christian Huitema				   *
*  Last Modification : 1995/2/15                                           *
*--------------------------------------------------------------------------*
*  Description       : Some declarations...                                *
*                                                                          *
*--------------------------------------------------------------------------*
*        Name	        |    Date   |          Modification                *
*--------------------------------------------------------------------------*
*                                                                          *
\**************************************************************************/

/*
 * Before sending an RTP packet, if there is a key associated 
 * to the group, apply:
 *     l_to_send = method[method_number].crypt_rtp(mess_in_clear, l_mess,
 *                                                 key, l_key, mess_to_send);
 *     send_msg(socket, mess_to_send, l_to_send, address);
 *
 * Do the reverse when receiving. Apply the RTCP functions instead
 * when sending control packets.
 */

struct crypto_method {
    char *name;
    int (*crypt_rtp)();
    int (*uncrypt_rtp)();
    int (*crypt_rtcp)();
    int (*uncrypt_rtcp)();
    int (*convertkey)();
    int key_len;
};

#define RTP_DEFAULT_CRYPT     0
#define RTP_NO_CRYPT        255
#define LEN_KEY              16
#define LEN_MAX_STR          64
#define LEN_HDR_RTCP_PROTECT  4

#ifndef SETCONFIG
extern int nb_methods;
extern int current_method;
extern int len_key;
extern char s_key[LEN_MAX_STR];
extern struct crypto_method method[];
#endif

#define ENCRYPTION_ENABLED() (current_method > nb_methods ? 0 : 1)

