# 
# Makefile for ivs on hpux 8.0 with audio and RasterOps support
#  

# system-dependend things
 
LIB_DIR       = /usr/lib/X11R5
X_LIBS	      = -lXaw -lXmu -lXt -lXext -lX11
INCLUDES      = -I/usr/include/X11R5
SOURCES       = ../newsrc/

# please define: 
#
# -DHPUX for the HP platform
# -DHP_ROPS_VIDEO for HP VideoLive grabber support
# -DNOHPAUDIO if you don't have any HP audio server in your network
#
FLAGS         = +O1 +O2 -DHPUX -DHP_ROPS_VIDEO 
#-DNOHPAUDIO if you have no hp audio server in the network
CFLAGS        = $(FLAGS) $(INCLUDES)
PFLAGS        = $(FLAGS) $(INCLUDES)

LIBS          = -lAlib

all: ivs ivsd ivs_record ivs_replay decode_h261 rate

ulaw.o: $(SOURCES)ulaw.c
	cc $(CFLAGS) -c $(SOURCES)ulaw.c

soundhpux.o: $(SOURCES)soundhpux.c
	cc $(CFLAGS) -c $(SOURCES)soundhpux.c

display.o: $(SOURCES)display.c
	cc $(CFLAGS) -c $(SOURCES)display.c 

ifct.o: $(SOURCES)ifct.c 
	cc $(CFLAGS) -c $(SOURCES)ifct.c  

ffct.o: $(SOURCES)ffct.c 
	cc $(CFLAGS) -c $(SOURCES)ffct.c  

protocol.o: $(SOURCES)protocol.c
	cc $(PFLAGS) -c $(SOURCES)protocol.c

videohp_rops.o: $(SOURCES)videohp_rops.c
	cc $(CFLAGS) -c $(SOURCES)videohp_rops.c

video_decoder.o: $(SOURCES)video_decoder.c
	cc $(CFLAGS) -c $(SOURCES)video_decoder.c

video_decoder_file.o: $(SOURCES)video_decoder.c
		      cc $(PFLAGS) -DINPUT_FILE -c \
		      $(SOURCES)video_decoder.c -o
		      video_decoder_file.o

adpcm.o: $(SOURCES)adpcm.c
	cc $(CFLAGS) -c $(SOURCES)adpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(CFLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(CFLAGS) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(CFLAGS) -c $(SOURCES)audio_coder.c

video_coder.o: $(SOURCES)video_coder.c 
	cc $(CFLAGS) -c $(SOURCES)video_coder.c

athena.o: $(SOURCES)athena.c
	cc $(CFLAGS) -c $(SOURCES)athena.c

ivs: protocol.o ifct.o display.o video_decoder.o video_coder.o \
     videohp_rops.o ffct.o soundhpux.o ulaw.o adpcm.o hadpcm.o \
     audio_decoder.o audio_coder.o athena.o $(SOURCES)ivs.c
	cc $(CFLAGS) protocol.o ifct.o display.o videohp_rops.o \
	video_decoder.o video_coder.o ffct.o soundhpux.o  ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o athena.o \
	$(SOURCES)ivs.c -L$(LIB_DIR) $(X_LIBS) -lAlib -lm -o $@

ivsd: $(SOURCES)ivsd.c protocol.o
	cc $(CFLAGS) -D_NO_PROTO protocol.o $(SOURCES)ivsd.c \
      -L$(LIB_DIR) $(X_LIBS) -o $@

ivs_replay.o: $(SOURCES)ivs_replay.c
	cc $(CFLAGS) -c $(SOURCES)ivs_replay.c


ivs_replay: protocol.o ifct.o display.o video_decoder.o \
	soundhpux.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	athena.o ivs_replay.o 
	cc protocol.o ifct.o display.o video_decoder.o soundhpux.o \
	ulaw.o adpcm.o hadpcm.o audio_decoder.o athena.o ivs_replay.o \
	-L$(LIB_DIR) $(X_LIBS) $(LIBS) -lm -o $@


ivs_record: $(SOURCES)ivs_record.c protocol.o athena.o display.o
	cc $(PFLAGS) $(SOURCES)ivs_record.c protocol.o athena.o \
	display.o -L$(LIB_DIR) $(X_LIBS) -lm -o $@


decode_h261: protocol.o ifct.o display.o video_decoder_file.o \
        athena.o $(SOURCES)decode_h261.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	athena.o video_decoder_file.o $(SOURCES)decode_h261.c \
	-L$(LIB_DIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o $@



rate: $(SOURCES)rate.c protocol.o
	cc $(CFLAGS) protocol.o $(SOURCES)rate.c -lm -o $@

clean:
	rm *.o

veryclean:
	rm *.o rate ivs ivs_replay ivs_record core



