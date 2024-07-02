LIB_DIR=/net/lib/motif-1.1/sun4OS4/lib
INCLUDES=-I/net/lib/motif-1.1/include
SOURCES=../src/

FLAGS= -O2 -DSPARCSTATION
CFLAGS=$(FLAGS) $(INCLUDES)
PFLAGS=$(FLAGS) -DMULTICAST
#
# if you have VideoPix Hardware then you have to set VIDEOPIX to
VIDEOPIX= -DVIDEOPIX -I/usr/1.0-VFC/vfc_lib -I/usr/1.0-VFC/sys
LIB_VIDEOPIX=/usr/1.0-VFC/vfc_lib/libvfc.a
# Else ...
# VIDEOPIX=
# LIB_VIDEOPIX=

all: ivs rate clean

ulaw.o: $(SOURCES)ulaw.c
	cc $(FLAGS) -c $(SOURCES)ulaw.c

soundbyte.o: $(SOURCES)soundbyte.c
	cc $(FLAGS) -c $(SOURCES)soundbyte.c

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
	cc -O1 $(VIDEOPIX) -c $(SOURCES)video_coder.c


ivs: protocol.o ifct.o display.o video_decoder.o video_coder.o \
	   ffct.o soundbyte.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	   audio_coder.o $(SOURCES)videoconf.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	video_decoder.o video_coder.o ffct.o soundbyte.o  ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o $(SOURCES)videoconf.c \
	$(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a $(LIB_DIR)/libX11.a \
	$(LIB_VIDEOPIX) -lm -o $@


rate: $(SOURCES)rate.c 
	cc $(FLAGS) $(SOURCES)rate.c -o $@

clean:
	rm *.o


