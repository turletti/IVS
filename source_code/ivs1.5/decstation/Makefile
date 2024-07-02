LIB_AUDIO=/usr/kits/XMedia
LIB_DIR=/net/lib/motif-1.1/decstation/lib
INCLUDES=-I/net/lib/motif-1.1/include
INC=-I/u/tom/0/sys/net
SOURCES=../

FLAGS= -g -DDECSTATION
CFLAGS=$(FLAGS) $(INCLUDES)
PFLAGS=$(FLAGS) -DMULTICAST
LIB_VIDEOPIX=
#
# if you have VideoPix Hardware then you have to set VIDEOPIX to
#VIDEOPIX= -DVIDEOPIX -I/usr/1.0-VFC/vfc_lib -I/usr/1.0-VFC/sys
#LIB_VIDEOPIX=/usr/1.0-VFC/vfc_lib/libvfc.a
# Else ...
VIDEOPIX=

all: videoconf 

ulaw.o: $(SOURCES)ulaw.c
	cc $(FLAGS) -c $(SOURCES)ulaw.c

create_player.o: $(SOURCES)create_player.c
	cc $(FLAGS) -I$(LIB_AUDIO) -c $(SOURCES)create_player.c

soundbyte.o: $(SOURCES)soundbyte.c
	cc $(FLAGS) -c $(SOURCES)soundbyte.c

display.o: $(SOURCES)display.c
	cc $(FLAGS) -c $(SOURCES)display.c 

ifct.o: $(SOURCES)ifct.c 
	cc $(FLAGS) -c $(SOURCES)ifct.c  

ffct.o: $(SOURCES)ffct.c 
	cc $(FLAGS) -c $(SOURCES)ffct.c  

protocol.o: $(SOURCES)protocol.c
	cc $(INC) $(PFLAGS) -c $(SOURCES)protocol.c

video_decoder.o: $(SOURCES)video_decoder.c
	cc $(INC) $(FLAGS) -c $(SOURCES)video_decoder.c

adpcm.o: $(SOURCES)adpcm.c
	cc $(FLAGS) -c $(SOURCES)adpcm.c

vadpcm.o: $(SOURCES)vadpcm.c
	cc $(FLAGS) -c $(SOURCES)vadpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(FLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(INC) $(FLAGS) -I$(LIB_AUDIO) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(INC) $(FLAGS) -I$(LIB_AUDIO) -c $(SOURCES)audio_coder.c

video_coder.o: $(SOURCES)video_coder.c 
	cc $(INC) -O1 $(VIDEOPIX) -c $(SOURCES)video_coder.c

videoconf: protocol.o ifct.o display.o video_decoder.o video_coder.o \
	   ffct.o soundbyte.o ulaw.o create_player.o adpcm.o hadpcm.o \
	   audio_decoder.o audio_coder.o create_player.o $(SOURCES)videoconf.c
	cc $(INC) $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o \
	display.o video_decoder.o video_coder.o ffct.o soundbyte.o \
	create_player.o ulaw.o adpcm.o hadpcm.o audio_decoder.o audio_coder.o \
	$(SOURCES)videoconf.c $(LIB_DIR)/libXm.a \
	$(LIB_DIR)/libXt.a /usr/lib/libX11.a $(LIB_VIDEOPIX) \
	$(VIDEOPIX) -lm -laudio -li -o $@


rate: rate.c protocol.o
	cc $(FLAGS) protocol.o rate.c -o $@


clean:
	rm *.o


