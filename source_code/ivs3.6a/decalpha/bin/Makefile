#
#
#  IVS Makefile for DEC stations
#
#
#--------------- X LIBRARY --------------------
X11_LIBDIR=/usr/lib
#X11_LIBDIR=/usr/lib
X11_INCLUDE=/usr/include
#X11_INCLUDE=/usr/include
XLIBRARY=$(X11_LIBDIR)/libXaw.a $(X11_LIBDIR)/libXmu.a \
	 $(X11_LIBDIR)/libXt.a $(X11_LIBDIR)/libXext.a \
	 $(X11_LIBDIR)/libX11.a $(X11_LIBDIR)/libdnet_stub.a 
SHARE=-DMITSHM

#----------------- AUDIO LIBRARY --------------
#FLAG_AUDIO= -DNO_AUDIO
LIB_AUDIO=/usr/lib/libAF.a
FLAG_AUDIO= -DAF_AUDIO
AUDIO_INCLUDE=-I/net1/decstation/kits/XMedia/

#---------------- MULTICAST OPTION ------------
# Comment this section if no multicast extensions are available
#
#MULTICAST_INCLUDE=/usr/sys/4.1.3.multicast
#CCMULTICASTINCL= -I$(MULTICAST_INCLUDE)
PFLAGS= -DMULTICAST

#----------------- VIDEO BOARD ----------------
# Comment this section if no VIDEOTX board is installed
#LIB_VIDEO=$(X11_LIBDIR)/libXv.a
#VIDEO_DEV=	-DVIDEOTX
#CCVIDEO=	
#---------------------------------------------------------

#--------------- gcc ver 2.4.3 -----------------
#CC=gcc
#AXL=-L/net/lib/gnu/gcc-2.4.3.1/lib/gcc-lib/mips-dec-ultrix4.3/2.4.3/
#OPT1=-traditional
#OPT=-O

#--------------- cc standard -------------------
CC=cc
#OPT=-g
OPT=-O -Olimit 2000

#--------------- FLAGS -------------------------
CCDEFS= $(SHARE) $(CPU) $(FLAG_AUDIO) $(VIDEO_DEV) \
	-DNEW_RECTAB -DNO_PROTO
#-DGETSTAT_MODE 
CCINCL= -I$(X11_INCLUDE)

CFLAGS= $(OPT)

CPU=-DDECSTATION
MACHINE=dec

#---------------------------------------------------------
OBJ_VIDEO= videodec.o


include ../common/Makefile.common

videodec.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)videodec.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)videodec.c



