X11_LIBDIR=/usr/lib
X11_INCLUDE=/usr/local/x11r4/include
MULTICAST_INCLUDE=/usr/sys/4.1.3.multicast

#VIDEOPIX CARD includes and libraries

#PIX=/usr/
PIX=/u/jerry/0/rodeo/turletti/
VIDEOPIX_INCLUDE= -I$(PIX)1.0-VFC/sys -I$(PIX)1.0-VFC/vfc_lib/ 
LIB_VIDEOPIX=$(PIX)1.0-VFC/vfc_lib/libvfc.a


SOURCES=../src/

GNU=no
DBG=no
PRF=no
DIR=no


VCARD=vfc

CPU=-DSPARCSTATION

#fixed point arithmetic resolution and binary point position

FFCT_RES=10
FFCT_FIX=6

IFCT_RES=16
IFCT_FIX=7

#--------------- gcc ver 2.4.3 -----------------
CC=gcc
AXL=-L/net/lib/gnu/gcc-2.4.3.1/lib/gcc-lib/sparc-sun-sunos4.1/2.4.3/
VOLA=-fvolatile
OPT1=-traditional
OPT=-O

#--------------- cc standard -------------------
#CC=cc
#VOLA=-Dvolatile=
#OPT=-O2

#---------------------------------------------------------
#VIDEOPIX  CARD DEFINITIONS

VFC_DEF= -DVIDEOPIX -DVIDEOPIX_ASYNC
#new VFC driver only
#VFC_DEF+= -DVIDEOPIX_ASYNC 
#SS2, IPX and CYPRESS 670 only, not allowed for SUPERSPARC !
#VFC_DEF+= -LDD_SBUS
#---------------------------------------------------------

HEADERS=$(SOURCES)adpcm.h  $(SOURCES)ffct.h  $(SOURCES)huffman.h  \
	$(SOURCES)quantizer.h \
	$(SOURCES)ulaw2linear.h $(SOURCES)h261.h  \
	$(SOURCES)huffman_coder.h $(SOURCES)protocol.h $(SOURCES)grab_vfc.h



OBJ=  	protocol.o display.o video_decoder.o \
	soundsparc.o ulaw.o adpcm.o hadpcm.o \
	audio_decoder.o audio_coder.o 
	
OBJ2=  	protocol.o display.o video_decoder_recv.o \
	soundsparc.o ulaw.o adpcm.o hadpcm.o \
	audio_decoder.o audio_coder.o 
	
OBJ1=  	protocol.o display.o video_decoder_file.o \
	soundsparc.o ulaw.o adpcm.o hadpcm.o \
	audio_decoder.o audio_coder.o athena.o
	
OBJ_VFC=videosparc_vfc.o grab_vfc.o video_coder_vfc.o ivs_vfc.o athena.o 

OBJ_VFC_FILE=videosparc_file.o grab_vfc.o video_coder_vfc.o ivs_vfc.o athena.o 

OBJ_DCT=ffct.o ffct_intra.o \
	ffct_chroma.o ffct_chroma_intra.o \
	ifct.o 

DEFS=$(VFC_DEF)
INCL=$(VIDEOPIX_INCLUDE)

SET=-DFFCT_RES=$(FFCT_RES) -DFFCT_FIX=$(FFCT_FIX) -DIFCT_RES=$(IFCT_RES) -DIFCT_FIX=$(IFCT_FIX)
SHARE=-DMITSHM

CCDEFS= $(SHARE) $(CPU) $(SET) $(DEFS) -DNEW_RECTAB -DNO_PROTO 
CCINCL=$(INCL) -I$(X11_INCLUDE)
CCMULTICASTINCL= -I$(MULTICAST_INCLUDE)
CCLIB=

CFLAGS= $(OPT) $(OPT1) $(VOLA) 
SFLAGS= $(OPT) $(OPT1) $(VOLA) 

TFLAGS=$(CFLAGS) #-Dregister= 
PFLAGS=$(FLAGS) -DMULTICAST

all: ivs ivsd ivs_record ivs_replay rate

ulaw.o: $(SOURCES)ulaw.c $(HEADERS)
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)ulaw.c

soundsparc.o: $(HEADERS)  $(SOURCES)soundsparc.c $(HEADERS) 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)soundsparc.c

display.o: $(HEADERS)  $(HEADERS)  $(SOURCES)display.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)display.c 

ifct.o: $(HEADERS)  $(SOURCES)ifct.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)ifct.c  


ffct.o ffct_intra.o ffct_chroma.o ffct_chroma_intra.o: $(HEADERS)  \
	$(SOURCES)ffct.c 
	
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)ffct.c  -o ffct.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-DFFCT_INTRA -c $(SOURCES)ffct.c  -o ffct_intra.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-DFFCT_CHROMA -c $(SOURCES)ffct.c -o ffct_chroma.o
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-DFFCT_INTRA -DFFCT_CHROMA -c $(SOURCES)ffct.c  -o ffct_chroma_intra.o


protocol.o: $(HEADERS)  $(SOURCES)protocol.c
	$(CC) $(PFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	$(CCMULTICASTINCL) \
	-c $(SOURCES)protocol.c

video_decoder.o: $(HEADERS) $(SOURCES)video_decoder.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)video_decoder.c 

video_decoder_recv.o: $(HEADERS)  $(SOURCES)video_decoder.c
	$(CC) $(CFLAGS) -DRECV_ONLY \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)video_decoder.c -o video_decoder_recv.o

adpcm.o: $(HEADERS)  $(SOURCES)adpcm.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)adpcm.c

hadpcm.o: $(HEADERS)  $(SOURCES)hadpcm.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)hadpcm.c

audio_decoder.o: $(HEADERS)  $(SOURCES)audio_decoder.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)audio_decoder.c

audio_coder.o: $(HEADERS)  $(SOURCES)audio_coder.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)audio_coder.c

video_coder_$(VCARD).o: $(HEADERS)  $(SOURCES)video_coder.c 
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-DDEBUG \
	-c -DADAPTATION $(SOURCES)video_coder.c -o video_coder_$(VCARD).o
	
videosparc_px.o: $(HEADERS)  $(SOURCES)videosparc_px.c 
	$(CC)  $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)videosparc_px.c

grab_vfc.o: $(HEADERS) $(SOURCES)vfc_macro.h $(SOURCES)grab_vfc.c 
	cc -O2 \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)grab_vfc.c

videosparc_vfc.o: $(HEADERS)  $(SOURCES)videosparc_vfc.c 
	$(CC) $(VOLA1) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)videosparc_vfc.c

videosparc_file.o: $(HEADERS) $(SOURCES)videosparc_vfc.c 
	$(CC) $(VOLA1) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-DINPUT_FILE -c $(SOURCES)videosparc_vfc.c -o videosparc_file.o

athena.o: $(HEADERS)  $(SOURCES)athena.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	 $(CCINCL) \
	-c $(SOURCES)athena.c -o athena.o

ivs_$(VCARD).o: $(HEADERS)     $(SOURCES)ivs.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c $(SOURCES)ivs.c -o ivs_$(VCARD).o

ivs:	$(HEADERS) $(OBJ) $(OBJ_DCT) $(OBJ_VFC)
	$(CC) $(CFLAGS) \
	$(OBJ) \
	$(OBJ_DCT) \
	$(OBJ_VFC) \
	$(LIB_VIDEOPIX) \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lX11 -lXext -lm -o $@ 

ivs_recv: $(HEADERS) $(OBJ2) $(OBJ_DCT) $(OBJ_VFC)
	$(CC) $(CFLAGS) \
	$(OBJ2) \
	$(OBJ_DCT) \
	$(OBJ_VFC) \
	$(LIB_VIDEOPIX) \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lX11 -lXext -lm -o $@ 

ivs_file: $(HEADERS) $(OBJ) $(OBJ_DCT) $(OBJ_VFC_FILE)
	$(CC) $(CFLAGS) \
	$(OBJ) \
	$(OBJ_DCT) \
	$(OBJ_VFC_FILE) \
	$(LIB_VIDEOPIX) \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lX11 -lXext -lm -o $@ 

video_decoder_file.o: $(HEADERS)  $(SOURCES)video_decoder.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
	$(CCINCL) \
	-c -DINPUT_FILE $(SOURCES)video_decoder.c -o video_decoder_file.o


decode_h261: $(HEADERS) $(OBJ1) $(OBJ_DCT) $(SOURCES)decode_h261.c
	$(CC) $(CFLAGS) \
	$(CCDEFS) \
        $(OBJ1) \
        $(OBJ_DCT) \
	$(SOURCES)decode_h261.c \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lX11 -lXext -lm -o $@

ivsd: $(SOURCES)ivsd.c protocol.o
	cc $(PFLAGS) -D_NO_PROTO protocol.o $(SOURCES)ivsd.c \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o $@

ivs_record: $(SOURCES)ivs_record.c protocol.o athena.o display.o
	cc $(PFLAGS) -D_NO_PROTO $(SOURCES)ivs_record.c protocol.o \
	athena.o display.o -L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext \
	-lX11 -lm -o $@

h261_record: $(SOURCES)h261_record.c protocol.o athena.o display.o
	cc $(PFLAGS) -D_NO_PROTO $(SOURCES)h261_record.c protocol.o \
	athena.o display.o -L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext \
	-lX11 -lm -o $@

ivs_replay: protocol.o ifct.o display.o video_decoder.o \
            soundsparc.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
            athena.o $(SOURCES)ivs_replay.c
	cc $(PFLAGS) -D_NO_PROTO protocol.o ifct.o display.o \
        video_decoder.o soundsparc.o ulaw.o adpcm.o hadpcm.o \
        audio_decoder.o athena.o $(SOURCES)ivs_replay.c \
        -L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o $@

rate: $(SOURCES)rate.c protocol.o
	cc $(PFLAGS) protocol.o $(SOURCES)rate.c -lm -o $@

clean:
	rm -f *.o core [adir]* *~







