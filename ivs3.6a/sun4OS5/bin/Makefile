#
#  IVS Makefile for SPARC stations Solaris (SunOS 5.3)
#
#
#
#--------------- X LIBRARY ---------------------
X11_LIBDIR=/usr/openwin/lib
X11_INCLUDE=/usr/openwin/include
XLIBRARY= $(X11_LIBDIR)/libXaw.a $(X11_LIBDIR)/libXmu.a \
	 $(X11_LIBDIR)/libXt.a $(X11_LIBDIR)/libX11.a \
	 $(X11_LIBDIR)/libXext.a /usr/ucblib/libucb.a \
        -lsocket -lnsl -ldl -lw 
#XLIBRARY= -R$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -L/usr/ucblib \
#	  -R/usr/ucblib -lucb -lsocket -lnsl -ldl -lw
SHARE=-DMITSHM
LIB_SOLARIS=/usr/ucblib/libucb.a -lsocket -lnsl -ldl -lw
#
#---------------- MULTICAST OPTION --------------
# Comment this section if you don't want multicast option
#
PFLAGS= -DMULTICAST
#
#
#
#-- FOR TEST PROGRAMS
OBJ_VIDEO_FILE=	grab_vfc.o videosparc_file.o 
OBJ_VIDEO_YUV411=grab_vfc.o videosparc_yuv411.o 
OBJ_VIDEO_ORIGI=grab_vfc.o videosparc_origi.o 
OBJ_SUNVIDEO_FILEY=grab_sunvideo.o sunvideosparc_fileY.o 
OBJ_SUNVIDEO_FILEYUV=grab_sunvideo.o sunvideosparc_fileYUV.o 
OBJ_SUNVIDEO_YUV411=grab_sunvideo.o sunvideosparc_yuv411.o \
		dither_sunvideo.o grab_vfc.o videosparc_file.o
OBJ_SUNVIDEO_ORIGY=grab_sunvideo.o sunvideosparc_origY.o \
		dither_sunvideo.o grab_vfc.o videosparc_file.o
OBJ_SUNVIDEO_ORIGYUV=grab_sunvideo.o sunvideosparc_origYUV.o \
		dither_sunvideo.o grab_vfc.o videosparc_file.o
#
#---------------------------------------------------------------------------
#
CPU=-DSPARCSTATION
#
#-- gcc standard
AXL=
VOLA=-fvolatile
OPT=-O2
OPT1=-traditional 
GCC_OPT= $(OPT) $(OPT1) $(VOLA) $(AXL)
#
#-- cc standard 
CC_OPT=-O
#CC_OPT=-xsb
#
# Please comment one of the two following sections;
# Note that gcc is more performant
#
#-- gcc ver 2.4.3 
CC=gcc
CFLAGS= $(GCC_OPT)
#--
#CC=/opt/SUNWspro/bin/cc
#CFLAGS=$(CC_OPT)

#--------------- FLAGS -------------------------
CCDEFS= $(SHARE) $(CPU) $(CARD_DEF) -DSOLARIS -DNEW_RECTAB -DNO_PROTO -DGETSTAT_MODE

#-DGETSTAT_MODE -DMCDEBUG
CCINCL= $(CARD_INCLUDE) -I$(X11_INCLUDE)
INSTALL=/usr/ucb/install
MACHINE=sparc
PX_SYS=solaris_

# 
#----------------- VIDEO BOARD Selection -----------------------------
# Please choose your video board. If you don't have px, sunvideo,
# vfc or vigrapix hardware, comment the corresponding include.
#
#---------------------------------------------------------------------
#
include Makefile.sunvideo
#include Makefile.vfc
include Makefile.px
include Makefile.vigrapix

include ../common/Makefile.common
#
#test: ivs_stat ivs_recv ivs_y ivs_yuv ivs_save_original ivs_save_output

#
#-------------- VigraPix card specific -------------------------------------
#
grab_vigrapix.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
	$(SOURCES)grab_general.h $(SOURCES)grab_vigrapix.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CARD_INCLUDE) \
	-c $(SOURCES)grab_vigrapix.c

videosparc_vigrapix.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_general.h $(SOURCES)videosparc_vigrapix.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CARD_INCLUDE) \
	-c $(SOURCES)videosparc_vigrapix.c

#-------------- Videopix card specific --------------------

grab_vfc.o: $(SOURCES)h261.h $(SOURCES)protocol.h $(SOURCES)grab_vfc.h \
        $(SOURCES)vfc_macro.h $(SOURCES)grab_vfc.c 
	$(CC) \
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
	$(CCDEFS) $(CCINCL) -DINPUT_FILE -DYUV_4_1_1 \
	-c $(SOURCES)videosparc_vfc.c -o videosparc_yuv411.o

#-------------- Parallax card specific --------------------

grab1_px.o:= PX_GRAB_DEF = $(PX_G_DEF)
grab2_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD
grab3_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_SQEEZE
grab4_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD_SQEEZE
grab5_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NTSC
grab6_px.o:= PX_GRAB_DEF = $(PX_G_DEF)_NLD_NTSC
grab7_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_FAST
grab8_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_NTSC_FAST
grab9_px.o:= PX_GRAB_DEF = $(PX_G_DEF_F)_NLD_SQEEZE_FAST

$(OBJ_GRAB_PX): $(GRAB_PX_HEADERS) $(SOURCES)grab_px.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(PX_GRAB_DEF) \
	$(CCINCL) \
	-c $(SOURCES)grab_px.c -o $@

videosparc_px.o: $(HEADERS)  $(PX_HEADERS)  $(SOURCES)videosparc_px.c 
	echo $(OBJ_PX_VIDEO)
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) -DNO_FAST_GRAB\
	 -c $(SOURCES)videosparc_px.c
#
#-------------- SunVideo card specific --------------------
videosparc_sunvideo.o: $(HEADERS)  $(SOURCES)videosparc_sunvideo.c \
	$(SOURCES)grab_sunvideo.h
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)videosparc_sunvideo.c

grab_sunvideo.o: $(HEADERS)  $(SOURCES)grab_sunvideo.c 
	gcc $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)grab_sunvideo.c

dither_sunvideo.o: $(HEADERS)  $(SOURCES)dither_sunvideo.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)dither_sunvideo.c


sunvideosparc_fileY.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_sunvideo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DINPUT_FILE \
	-c $(SOURCES)videosparc_sunvideo.c -o sunvideosparc_fileY.o


sunvideosparc_origY.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_sunvideo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DOUTPUT_FILE \
	-c $(SOURCES)videosparc_sunvideo.c -o $@

sunvideosparc_origYUV.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_sunvideo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DOUTPUT_FILE -DGRAB_COLOR \
	-c $(SOURCES)videosparc_sunvideo.c -o $@

sunvideosparc_yuv411.o: $(SOURCES)h261.h $(SOURCES)protocol.h \
        $(SOURCES)grab_vfc.h $(SOURCES)videosparc_sunvideo.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) $(CCINCL) -DINPUT_FILE -DYUV_4_1_1 \
	-c $(SOURCES)videosparc_sunvideo.c -o sunvideosparc_yuv411.o
#	
#----------------------------------------------------------
# TESTS PROGRAMS 
#
ivs_recv: $(HEADERS) $(OBJ_IVS_RECV) $(OBJ_DCT) $(OBJ_VIDEO) $(OBJ_CODER) \
	$(OBJ_MC) ivs.o  
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS_RECV) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o audio.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_y: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_FILE) $(OBJ_CODER) \
	$(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_FILE) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_yuv: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_YUV411) $(OBJ_CODER) \
	$(OBJ_MC) audio.o ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_YUV411) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	audio.o ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) $(LIB_EXTRA) -lm -o $@ 


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
	$(OBJ_CODER) $(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS_SAVE_RECV) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 


rivs_y: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_VIDEO_FILE) $(OBJ_CODER) \
	$(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_VIDEO_FILE) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

rivs_yuv: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_SUNVIDEO_YUV411) \
	$(OBJ_CODER) \
	$(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_SUNVIDEO_YUV411) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_save_y: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_SUNVIDEO_ORIGY) \
	$(OBJ_CODER) $(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_SUNVIDEO_ORIGY) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

ivs_save_yuv: $(HEADERS) $(OBJ_IVS) $(OBJ_DCT) $(OBJ_SUNVIDEO_ORIGYUV) \
	$(OBJ_CODER) $(OBJ_MC) ivs.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(OBJ_IVS) \
	$(OBJ_DCT) \
	$(OBJ_SUNVIDEO_ORIGYUV) \
	$(OBJ_CODER) \
	$(OBJ_MC) \
	ivs.o \
	$(LIB_VIDEO) \
	-L$(X11_LIBDIR) $(XLIBRARY) $(LIB_AUDIO) -lm -o $@ 

