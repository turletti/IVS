/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Video interface for SGI Indigo.
   Replaces video_encode() and vfc_scan_ports() in video_coder.c.
   Author: Guido van Rossum, CWI, Amsterdam.
   Date:   October 1992.
*/

#include <stdio.h>
#include <svideo.h>

#include "h261.h"
#include "protocol.h"

#define LIG_MAX			288	/* Max number of lines and columns */
#define COL_MAX			352

#define CIF_WIDTH               352     /* Width and height of CIF */
#define CIF_HEIGHT		288

#define QCIF_WIDTH              176     /* Width and height of Quarter CIF */
#define QCIF_HEIGHT		144


/* sgi_checkvideo() -- return 1 if video capture is possible, 0 if not */

int sgi_checkvideo() {
	SVhandle dev;

	/* Open the video device.  If this fails, assume we don't have video */
	dev = svOpenVideo();
	if (dev == NULL) {
		svPerror("sgi_checkvideo: svOpenVideo");
		return 0;
	}
	/* Close the device again; sgi_video_encode will re-open it */
	svCloseVideo(dev);
	return 1;
}


/* sgi_get_image() -- grab a CIF image */

static void sgi_get_image(dev, im_data, lig, col, w, h)
	SVhandle dev;
	u_char im_data[][COL_MAX];
	int lig, col;
	int w, h;
{
	char *ptr;
	long field;

	register int pixel;
	register u_char *p_data;
	register char *p_buf;
	register int j;
	register int r, g, b;
	char *buf_even;
	char *buf_odd;
	int i;
	int xoff, yoff;

	while (1) {
		ptr = NULL;
		if (svGetCaptureData(dev, (void **)&ptr, &field) < 0) {
			svPerror("sgi_get_image: svGetCaptureData");
			exit(1);
		}
		if (ptr != NULL)
			break;
		sginap(1);
	}

	/* The capture area is larger than col*lig; use the center */
	xoff = (w-col)/2;
	yoff = (h-lig)/2;
	buf_even = &ptr[(yoff/2)*w + xoff];
	buf_odd = &ptr[(yoff/2 + h/2)*w + xoff];

	for (i = 0; i < lig; i++) {
		if ((i&1) == 0)
			p_buf = &buf_even[(i/2)*w];
		else
			p_buf = &buf_odd[(i/2)*w];
		p_data = &im_data[i][0];
		for (j = col; --j >= 0; ) {
			pixel = *p_buf++;
			/* Y := .30R + .59G + .11B */
			r = (pixel & 0xe0) * (30 * 0x7f) / 0xe0;
			g = (pixel & 0x07) * (59 * 0x7f) / 0x07;
			b = (pixel & 0x18) * (11 * 0x7f) / 0x18;
			*p_data++ = (r+g+b)/100;
		}
	}

	svUnlockCaptureData(dev, ptr);
}


/* sgi_video_encode() -- the video capture, coding and tranmission process */


void sgi_video_encode(group, port, ttl, col, lig, quality, fd, CUTTING, stat)
	char *group, *port;
	u_char ttl;
	int col, lig; /* image's size */
	int quality; /* Boolean: true if we privilege quality and not rate */
	int fd[2];
	BOOLEAN CUTTING;
	BOOLEAN stat; /* boolean: True if statistical mode */
{
	/* in video_coder.c: */
	extern BOOLEAN stat_mode;
	extern FILE *F_loss;

	IMAGE_CIF image_coeff;
	u_char new_image[LIG_MAX][COL_MAX];
	int NG_MAX;
	int FIRST = 1;
	int CIF = 0;
	int sock_recv = 0;
	int QUANT_FIX = 3;
	int threshold = 20;    /* for motion estimation */
	int frequency = 2;

	SVhandle dev;
	svCaptureInfo info;
	int w, h;

	/* Open video device */
	dev = svOpenVideo();
	if (dev == NULL) {
		fprintf(stderr, "sgi_video_encode: svOpenVideo failed\n");
		exit(1);
	}

	/* Choose the threshold for movement detection */
	if(!quality)
		threshold = 23;
	else
		threshold = 15;
	
	if(lig == CIF_HEIGHT) {
		NG_MAX = 12;
		CIF = 1;
	}
	else
		NG_MAX = 5;

	sock_recv = declare_listen(PORT_VIDEO_CONTROL);

	w = col;
	h = lig;
	if (w*3/4 < h)
		w = h*4/3;
	if (h*4/3 < w)
		h = w*3/4;
	svQuerySize(dev, w, h, &w, &h);
	svSetSize(dev, w, h);

	/* Initialize continuous capture mode */
	info.format = SV_RGB8_FRAMES;
	info.width = w;
	info.height = h;
	info.size = 1;
	info.samplingrate = 2;
	svInitContinuousCapture(dev, &info);
	w = info.width;
	h = info.height;
	if (w < col || h < lig) {
		fprintf(stderr, "sgi_encode_video: capture size too small\n");
		exit(1);
	}

	/* Get the first image */
	sgi_get_image(dev, new_image, lig, col, w, h);
	
	if(stat) {
		stat_mode = TRUE;
		if((F_loss = fopen(".videoconf.loss", "w")) == NULL) {
			fprintf(stderr, "cannot create videoconf.loss file\n");
			exit(1);
		}
	}
	encode_h261(group, port, ttl, new_image, image_coeff, threshold,
		    NG_MAX, QUANT_FIX, FIRST, frequency, quality, 
		    sock_recv, CUTTING);
	FIRST = 0;

	/* Next frames, INTRA mode too */

	do {
		sgi_get_image(dev, new_image, lig, col, w, h);
		encode_h261(group, port, ttl, new_image, image_coeff,
			    threshold, NG_MAX,  QUANT_FIX, FIRST, frequency,
			    quality, sock_recv, CUTTING);
	} while (!ItIsTheEnd(fd));

	svEndContinuousCapture(dev);
	svCloseVideo(dev);
	close(fd[0]);
	if(stat_mode)
		fclose(F_loss);
	exit(0);
	/* NOTREACHED */
}
