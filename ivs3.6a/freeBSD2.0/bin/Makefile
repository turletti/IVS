#
#
#  IVS Makefile for FreeBSD 2.0
#
#
#--------------- X LIBRARY --------------------
X11_LIBDIR=/usr/X11R6/lib
X11_INCLUDE=/usr/X11R6/include
XLIBRARY=-lXaw -lXmu -lXt -lXext -lICE -lX11
LIB_EXTRA=-lcompat
#SHARE=-DMITSHM
MACHINE=i386


#---------------- MULTICAST OPTION ------------
# Comment this section if no multicast extensions are available
#
PFLAGS= -DMULTICAST

#----------------- AUDIO/VIDEO OPTIONS --------
# Please define: 
#
# -DNO_AUDIO if you don't have audio
# -DI386AUDIO if you have sound card & VoxWare driver in x86
#
#OPTIONS=-DI386AUDIO 
OPTIONS=-DNO_AUDIO  

#--------------- cc standard -------------------
CC=cc
#OPT=-g
OPT=-O

#--------------- FLAGS -------------------------
INSTALL=install
CPU=-DFreeBSD_$(MACHINE)
CCDEFS= $(SHARE) $(CPU) $(OPTIONS) -DNEW_RECTAB -DFUNCPROTO=15 
#-DGETSTAT_MODE -DMCDEBUG -DDEBUG -DRECVD_ONLY
CCINCL= -I$(X11_INCLUDE)

CFLAGS= $(OPT)


#---------------------------------------------------------
#OBJ_VIDEO=

.include "../common/Makefile.common"








