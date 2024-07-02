#
#  IVS Makefile for SGI stations 
#
#----------------- VIDEO BOARD Selection -----------------------------
# Please choose your video boards. If you don't have Indigo or 
# Galileo video hardware, comment the corresponding include.
# For the Indy version, select the galileo Makefile.
#
#---------------------------------------------------------------------
#
include Makefile.galileo
#include Makefile.indigo
#
#--------------- X LIBRARY --------------------
X11_LIBDIR=/usr/lib/
X11_INCLUDE=/usr/include
XLIBRARY= -L/usr/lib -lXaw -lXmu -lXt -lXext -lX11 -laudio -lm
#XLIBRARY=$(X11_LIBDIR)/libXaw.a $(X11_LIBDIR)/libXmu.a \
	 $(X11_LIBDIR)/libXt.a $(X11_LIBDIR)/libXext.a \
	 $(X11_LIBDIR)/X11R5/libX11.a /usr/lib/libw.a 
#-Bstatic 
SHARE=-DMITSHM
MACHINE=sgi
#
#---------------- MULTICAST OPTION ------------
# Comment this section if no multicast extensions are available
#
PFLAGS= -DMULTICAST
#
#----------------- AUDIO LIBRARY --------------
LIB_AUDIO=$(X11_LIBDIR)/libaudio.a
#FLAG_AUDIO= -DNO_AUDIO
#
#--------------- cc standard -------------------
CC=cc
#OPT= -g
OPT= -cckr -O 
#
#--------------- FLAGS -------------------------
CCDEFS= $(SHARE) -DSGISTATION $(CARD_DEF) -DNEW_RECTAB \
	-DNO_PROTO $(FLAG_AUDIO)
#-DGETSTAT_MODE -DMCDEBUG
CCINCL= $(CARD_INCLUDE) $(AUDIO_INCLUDE) -I$(X11_INCLUDE) 
CFLAGS= $(OPT) 
#
#
#---------------------------------------------------------
#
LIB_VIDEO=$(LIB_VIDEO_INDIGO) $(LIB_VIDEO_GALILEO)
CARD_INCLUDE=$(CARD_INCLUDE_INDIGO) $(CARD_INCLUDE_GALILEO)
CARD_DEF=$(CARD_DEF_INDIGO) $(CARD_DEF_GALILEO) $(CARD_DEF_INDY)
OBJ_VIDEO=$(OBJ_VIDEO_INDIGO) $(OBJ_VIDEO_GALILEO)
#
#---------------------------------------------------------
#
include ../common/Makefile.common

#
#-------------- Indigo card specific -------------------------------------
#
grab_indigovideo.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)grab_indigovideo.c
	$(CC) $(CFLAGS) \
        $(CCDEFS) \
	-c $(SOURCES)grab_indigovideo.c

videosgi_indigo.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)videosgi_indigo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)videosgi_indigo.c

#
#-------------- Galileo card specific -------------------------------------
#
grab_galileovideo.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)grab_galileovideo.c
	$(CC) $(CFLAGS) \
        $(CCDEFS) \
	-c $(SOURCES)grab_galileovideo.c

videosgi_galileo.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)videosgi_galileo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)videosgi_galileo.c




