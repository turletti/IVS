#
#  IVS Makefile for Linux
#
#
#--------------- X LIBRARY --------------------
X11_LIBDIR=/usr/X11/lib
X11_INCLUDE=/usr/X11/include
#XLIBRARY=$(X11_LIBDIR)/libXaw.a $(X11_LIBDIR)/libXmu.a \
#	 $(X11_LIBDIR)/libXt.a $(X11_LIBDIR)/libXext.a \
#	 $(X11_LIBDIR)/libX11.a
XLIBRARY=-lXaw -lXt -lXext -lX11
SHARE=-DMITSHM
MACHINE=i386

#---------------- MULTICAST OPTION ------------
# Comment this section if no multicast extensions are available
# If defined, requires CONFIG_IP_MULTICAST in kernel.
#
PFLAGS=-DMULTICAST

#----------------- AUDIO OPTIONS ---------------
# Please define: 
#
# -DNO_AUDIO if you don't have audio
# -DI386AUDIO if you have sound card & VoxWare driver in x86
#
OPTIONS=-DI386AUDIO 
#OPTIONS=-DNO_AUDIO 

# 
#----------------- VIDEO BOARD Selection -----------------------------
# Please choose your video boards. If you don't have the 
# ScreenMachine II hardware, comment the corresponding include.
#
#---------------------------------------------------------------------
#
include Makefile.sm
#
#
#--------------- cc standard -------------------
CC=cc
#OPT=-g -O2
OPT=-O2

#--------------- FLAGS -------------------------
CCDEFS= $(SHARE) -DPC $(CARD_DEF) -DNEW_RECTAB -DNO_PROTO
#-DGETSTAT_MODE -DMCDEBUG -DDEBUG -DRECVD_ONLY
CCINCL=-I$(X11_INCLUDE)

CFLAGS= $(OPT) $(OPTIONS)



include ../common/Makefile.common

#
#-------------- ScreenMachine II card specific -------------
#
video_sm.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)video_sm.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)video_sm.c

grab_sm.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)grab_sm.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)grab_sm.c
