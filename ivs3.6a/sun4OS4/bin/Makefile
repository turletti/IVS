#
#  IVS Makefile for SPARC stations
#
#
#
#
#--------------- X LIBRARY -------------------------------------------
X11_LIBDIR=/usr/lib/X11R5
X11_INCLUDE=/usr/include
XLIBRARY=$(X11_LIBDIR)/libXaw.a $(X11_LIBDIR)/libXmu.a \
	 $(X11_LIBDIR)/libXt.a $(X11_LIBDIR)/libXext.a \
	 $(X11_LIBDIR)/libX11.a
SHARE=-DMITSHM
#
#
#---------------- MULTICAST OPTION -----------------------------------
# Comment this section if no multicast extensions are available
#
MULTICAST_INCLUDE=/usr/sys/sys.4.1.3_U1.multicast
CCMULTICASTINCL= -I$(MULTICAST_INCLUDE)
PFLAGS= -DMULTICAST
#
#
#---------------------------------------------------------------------------
#
#-- FOR TEST PROGRAMS
OBJ_VIDEO_FILE=	grab_vfc.o videosparc_file.o 
OBJ_VIDEO_YUV411=grab_vfc.o videosparc_yuv411.o 
OBJ_VIDEO_ORIGI= grab_vfc.o videosparc_origi.o 
#
#---------------------------------------------------------------------------
#
CPU=-DSPARCSTATION
AXL=-L/net/lib/gnu/gcc-2.4.3.1/lib/gcc-lib/sparc-sun-sunos4.1/2.4.3/
VOLA=-fvolatile
OPT1=-traditional 
#OPT= -g -static 
OPT= -O -static
GCC_OPT= $(OPT1) $(OPT) $(VOLA) $(AXL)
#
#-- cc standard 
CC_OPT= -O2
#
# Please comment one of the two following sections;
# Note that gcc is more performant
#
#-- gcc ver 2.4.3 
CC=gcc
CFLAGS= $(GCC_OPT) -DLOG_STAT
#--
#CC=cc
#CFLAGS= $(CC_OPT)
#
#--------------- FLAGS -----------------------------------------------------
CCDEFS= $(SHARE) $(CPU) $(CARD_DEF) -DNEW_RECTAB -DNO_PROTO 
#-DGETSTAT_MODE -DMCDEBUG
CCINCL= $(CARD_INCLUDE) -I$(X11_INCLUDE)
INSTALL=install
MACHINE=sparc
PX_SYS=sunos_


# 
#----------------- VIDEO BOARD Selection -----------------------------
# Please choose your video boards. If you don't have vfc, px or
# VigraPix hardware, comment the corresponding include.
#
#---------------------------------------------------------------------
#
#include Makefile.vfc
#include Makefile.vigrapix
#include Makefile.px
#

include ../common/Makefile.common

#test: ivs_stat ivs_recv ivs_y ivs_yuv ivs_save_original ivs_save_output

#
#-------------- VigraPix card specific -------------------------------------
#
grab_vigrapix.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)grab_general.h $(SOURCES)grab_vigrapix.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) \
	-c $(SOURCES)grab_vigrapix.c

videosparc_vigrapix.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_general.h $(SOURCES)videosparc_vigrapix.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) \
	-c $(SOURCES)videosparc_vigrapix.c

#
#-------------- Videopix card specific -------------------------------------
#
grab_vfc.o: $(SOURCES)h261.h $(SOURCES)protocol.h $(SOURCES)grab_vfc.h \
        $(SOURCES)vfc_macro.h $(SOURCES)grab_vfc.c 
	cc -O2 \
	$(CCDEFS) $(CCINCL) \
	-c $(SOURCES)grab_vfc.c

videosparc_vfc.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_vfc.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) \
	-c $(SOURCES)videosparc_vfc.c

videosparc_file.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_vfc.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DINPUT_FILE \
	-c $(SOURCES)videosparc_vfc.c -o videosparc_file.o

videosparc_origi.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_vfc.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DOUTPUT_FILE \
	-c $(SOURCES)videosparc_vfc.c -o $@

videosparc_yuv411.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_vfc.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DINPUT_FILE -DYUV_4_1_1 -DOUTPUT_FILE \
	-c $(SOURCES)videosparc_vfc.c -o videosparc_yuv411.o
#
#-------------- Parallax card specific ------------------------------------
#
grab1_px.o:= PX_GRAB_DEF = $(PX_G_DEF)
grab2_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD
grab3_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_SQEEZE
grab4_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD_SQEEZE
grab5_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NTSC
grab6_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD_NTSC
grab7_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_FAST
grab8_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_NTSC_FAST
grab9_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_SQEEZE_FAST
#
$(OBJ_GRAB_PX): $(GRAB_PX_HEADERS) $(SOURCES)grab_px.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(PX_GRAB_DEF) \
	$(CCINCL) \
	-c $(SOURCES)grab_px.c -o $@

videosparc_px.o: $(HEADERS)  $(PX_HEADERS)  $(SOURCES)videosparc_px.c 
	echo $(OBJ_PX_VIDEO)
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) -DNO_FAST_GRAB \
	 -c $(SOURCES)videosparc_px.c

#
#-------------------------------------------------------------------------
# TESTS PROGRAMS 
#
ivs_recv: $(HEADERS) $(OBJ_IVS_RECV) $(OBJ_DCT) $(OBJ_VIDEO) $(OBJ_CODER) \
	$(OBJ_MC) audio.o ivs.o  
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS_RECV) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	audio.o ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_y: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_FILE) $(OBJ_CODER) \
	$(OBJ_MC) audio_receive.o ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_FILE) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	audio_receive.o ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_yuv: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_YUV411) $(OBJ_CODER) \
	$(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_YUV411) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_save_original: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_ORIGI) \
	$(OBJ_CODER) $(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_ORIGI) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_save_output: $(HEADERS) $(OBJ_IVS_SAVE_RECV) $(OBJ_DCT) $(OBJ_VIDEO) \
	$(OBJ_CODER) $(OBJ_MC) audio_receive.o ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS_SAVE_RECV) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	audio_receive.o ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 


