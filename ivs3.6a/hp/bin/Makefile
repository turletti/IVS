#
#  IVS Makefile for hpux 8.0 with audio and RasterRops support
#

#--------------- X LIBRARY --------------------
X11_INCLUDE=/usr/include/X11R5
X11_LIBDIR= -L/lib -L/usr/lib/X11R5 -L/usr/lib/X11R4
XLIBRARY= -lXaw -lXmu -lXt -lXext -lX11 -lm
SHARE=-DMITSHM
MACHINE=hpux

#---------------- MULTICAST OPTION ------------
# Comment this section if no multicast extensions are available
#
#MULTICAST_INCLUDE=/usr/sys/4.1.3.multicast
#CCMULTICASTINCL= -I$(MULTICAST_INCLUDE)
PFLAGS= -DMULTICAST

#----------------- AUDIO/VIDEO OPTIONS --------
# Please define: 
#
# -DHPUX for the HP platform
# -DHP_ROPS_VIDEO for HP VideoLive grabber support
# -DNOHPAUDIO if you don't have any HP audio server in your network
#
OPTIONS         = -DHPUX -DHP_ROPS_VIDEO
LIB_AUDIO=-lAlib
#-DNOHPAUDIO if you have no hp audio server in the network



#--------------- cc standard -------------------
CC=cc
OPT= +O1 +O2 +O3

#--------------- FLAGS -------------------------
CCDEFS= $(SHARE) -DNEW_RECTAB -DNO_PROTO
#-DGETSTAT_MODE -DMCDEBUG 
CCINCL= $(LIB_AUDIO) -I$(X11_INCLUDE)

CFLAGS= $(OPT) $(OPTIONS)



#---------------------------------------------------------
OBJ_VIDEO= videohp_rops.o


include ../common/Makefile.common

videohp_rops.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)videohp_rops.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	-c $(SOURCES)videohp_rops.c

