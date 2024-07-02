LIB_DIR=/usr/lib
SOURCES=../src/

FLAGS=-O -DINDIGOVIDEO -DINDIGO -DMITSHM
CFLAGS=$(FLAGS) $(INCLUDES)
PFLAGS=$(FLAGS) -DMULTICAST
XLIBS=$(LIB_DIR)/libXaw.a $(LIB_DIR)/libXmu.a $(LIB_DIR)/libXt_s.a \
      $(LIB_DIR)/libXext.a $(LIB_DIR)/libX11_s.a -lPW

# SGI audio:
LIB_AUDIO= -laudio

# SGI video:
LIB_VIDEO = -lsvideo $(LIB_DIR)/libXext.a

all: ivs ivsd ivs_record ivs_replay decode_h261 rate

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
	cc $(FLAGS) -Olimit 1300 -c $(SOURCES)video_decoder.c

video_decoder_file.o: $(SOURCES)video_decoder.c
	cc $(CFLAGS) -DINPUT_FILE -Olimit 1300 -c \
	$(SOURCES)video_decoder.c -o video_decoder_file.o

adpcm.o: $(SOURCES)adpcm.c
	cc $(FLAGS) -c $(SOURCES)adpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(FLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(FLAGS) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(FLAGS) -c $(SOURCES)audio_coder.c

video_coder.o: $(SOURCES)video_coder.c 
	cc -O -c $(SOURCES)video_coder.c

videosgi.o: $(SOURCES)videosgi.c 
	cc $(CFLAGS) -c $(SOURCES)videosgi.c

athena.o: $(SOURCES)athena.c
	cc $(CFLAGS) -c $(SOURCES)athena.c

ivs.o: $(SOURCES)ivs.c
	cc $(CFLAGS) -D_NO_PROTO -c $(SOURCES)ivs.c

ivs: protocol.o ifct.o display.o video_decoder.o video_coder.o videosgi.o \
	   ffct.o soundsgi.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	   audio_coder.o athena.o ivs.o
	cc $(CFLAGS) protocol.o ifct.o display.o videosgi.o \
	video_decoder.o video_coder.o ffct.o soundsgi.o ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o athena.o ivs.o \
	$(LIB_VIDEO) $(XLIBS) $(LIB_AUDIO) -lm -o $@


ivsd: $(SOURCES)ivsd.c protocol.o
	cc $(CFLAGS) -D_NO_PROTO protocol.o $(SOURCES)ivsd.c \
	$(XLIBS) -lm -o $@


ivs_record: $(SOURCES)ivs_record.c protocol.o athena.o display.o
	cc $(PFLAGS) -D_NO_PROTO -DMULTICAST $(SOURCES)ivs_record.c \
	protocol.o athena.o display.o $(XLIBS) -lm -o $@

ivs_replay: protocol.o ifct.o display.o video_decoder.o \
            soundsgi.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
            athena.o $(SOURCES)ivs_replay.c
	cc $(CFLAGS) -D_NO_PROTO protocol.o ifct.o display.o \
        video_decoder.o soundsgi.o ulaw.o adpcm.o hadpcm.o \
        audio_decoder.o athena.o $(SOURCES)ivs_replay.c \
	$(XLIBS) $(LIB_AUDIO) -lm -o $@


decode_h261: protocol.o ifct.o display.o video_decoder_file.o \
        athena.o $(SOURCES)decode_h261.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	athena.o video_decoder_file.o $(SOURCES)decode_h261.c \
	-L$(LIB_DIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o $@


rate: $(SOURCES)rate.c protocol.o
	cc $(FLAGS) protocol.o $(SOURCES)rate.c -lm -o $@

clean:
	rm *.o
