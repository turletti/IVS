#LIB_DIR=/net/lib/motif-1.1/sun4OS4/lib
LIB_DIR=/usr/lib
#INCLUDES=-I/net/lib/motif-1.1/include
SOURCES=../src/

#FLAGS= -O2 -DSPARCSTATION
FLAGS=-O -DSPARCSTATION -DINDIGOVIDEO -DMITSHM
CFLAGS=$(FLAGS) $(INCLUDES)
PFLAGS=$(FLAGS) -DMULTICAST
#
# if you have VideoPix Hardware then you have to set VIDEOPIX to
#VIDEOPIX= -DVIDEOPIX -I/usr/1.0-VFC/vfc_lib -I/usr/1.0-VFC/sys
#LIB_VIDEOPIX=/usr/1.0-VFC/vfc_lib/libvfc.a
# Else ...
VIDEOPIX=
LIB_VIDEOPIX=

# SGI audio:
LIB_AUDIO= -laudio

# SGI video:
LIB_VIDEO = -lsvideo $(LIB_DIR)/libXext.a

all: ivs rate

ulaw.o: $(SOURCES)ulaw.c
	cc $(FLAGS) -c $(SOURCES)ulaw.c

soundsgi.o: $(SOURCES)soundsgi.c
	cc $(FLAGS) -c $(SOURCES)soundsgi.c

display.o: $(SOURCES)display.c
	cc $(FLAGS) -c $(SOURCES)display.c 

ifct.o: $(SOURCES)ifct.c 
	cc $(FLAGS) -c $(SOURCES)ifct.c  

ffct.o: $(SOURCES)ffct.c 
	cc $(FLAGS) -c $(SOURCES)ffct.c  

protocol.o: $(SOURCES)protocol.c
	cc $(PFLAGS) -c $(SOURCES)protocol.c

video_decoder.o: $(SOURCES)video_decoder.c
	cc $(FLAGS) -c $(SOURCES)video_decoder.c

adpcm.o: $(SOURCES)adpcm.c
	cc $(FLAGS) -c $(SOURCES)adpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(FLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(FLAGS) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(FLAGS) -c $(SOURCES)audio_coder.c

video_coder.o: $(SOURCES)video_coder.c 
	cc -O $(VIDEOPIX) -c $(SOURCES)video_coder.c

videosgi.o: $(SOURCES)videosgi.c 
	cc $(CFLAGS) -c $(SOURCES)videosgi.c

videoconf.o: $(SOURCES)videoconf.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) -c $(SOURCES)videoconf.c

ivs: protocol.o ifct.o display.o video_decoder.o video_coder.o videosgi.o \
	   ffct.o soundsgi.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	   audio_coder.o videoconf.o
	cc $(CFLAGS) protocol.o ifct.o display.o videosgi.o \
	video_decoder.o video_coder.o ffct.o soundsgi.o  ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o videoconf.o \
	$(LIB_VIDEO) \
	$(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a $(LIB_DIR)/libX11.a \
	$(LIB_VIDEOPIX) $(LIB_AUDIO) -lm -o $@


rate: $(SOURCES)rate.c 
	cc $(FLAGS) $(SOURCES)rate.c -o $@

clean:
	rm *.o
