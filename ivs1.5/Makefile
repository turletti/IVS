LIB_DIR=/net/lib/motif/sun4OS4/lib
INCLUDES=-I/net/lib/motif/sun4OS4/include

FLAGS= -g -DSPARCSTATION 
CFLAGS=$(FLAGS) $(INCLUDES)
PFLAGS=$(FLAGS) -DMULTICAST
EFLAGS=$(FLAGS) -DMITSHM

#
# if you have VideoPix Hardware then you have to set VIDEOPIX to
VIDEOPIX= -DVIDEOPIX -I/usr/1.0-VFC/vfc_lib -I/usr/1.0-VFC/sys
LIB_VIDEOPIX=/usr/1.0-VFC/vfc_lib/libvfc.a
# Else ...
# VIDEOPIX=
# LIB_VIDEOPIX=

all: videoconf rate

ulaw.o: ulaw.c
	cc $(FLAGS) -c ulaw.c

soundbyte.o: soundbyte.c
	cc $(FLAGS) -c soundbyte.c

display.o: display.c
	cc $(EFLAGS) -c display.c 

ifct.o: ifct.c 
	cc $(FLAGS) -c ifct.c  

ffct.o: ffct.c 
	cc $(FLAGS) -c ffct.c  

protocol.o: protocol.c
	cc $(PFLAGS) -c protocol.c

video_decoder.o: video_decoder.c
	cc $(FLAGS) -c video_decoder.c

adpcm.o: adpcm.c
	cc $(FLAGS) -c adpcm.c

vadpcm.o: vadpcm.c
	cc $(FLAGS) -c vadpcm.c

hadpcm.o: hadpcm.c
	cc $(FLAGS) -c hadpcm.c

audio_decoder.o: audio_decoder.c
	cc $(FLAGS) -c audio_decoder.c

audio_coder.o: audio_coder.c
	cc $(FLAGS) -c audio_coder.c

video_coder.o: video_coder.c 
	cc -O1 $(VIDEOPIX) -c video_coder.c


videoconf: protocol.o ifct.o display.o video_decoder.o video_coder.o \
	   ffct.o soundbyte.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	   audio_coder.o videoconf.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	video_decoder.o video_coder.o ffct.o soundbyte.o  ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o videoconf.c \
	$(LIB_DIR)/libXext.a $(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a \
	$(LIB_DIR)/libX11.a $(LIB_VIDEOPIX) -lm -o $@



ivs_record: ivs_record.c
	cc $(PFLAGS) -D_NO_PROTO ivs_record.c \
	$(LIB_DIR)/libXext.a $(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a \
	$(LIB_DIR)/libX11.a -lm -o $@


ivs_replay: protocol.o ifct.o display.o video_decoder.o \
	    soundbyte.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	    ivs_replay.c
	cc $(CFLAGS) -D_NO_PROTO protocol.o ifct.o display.o \
	video_decoder.o soundbyte.o ulaw.o adpcm.o hadpcm.o \
	audio_decoder.o ivs_replay.c \
	$(LIB_DIR)/libXext.a $(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a \
	$(LIB_DIR)/libX11.a -lm -o $@



tvideo_coder.o: tvideo_coder.c 
	cc -O1 $(VIDEOPIX) -c tvideo_coder.c


tvideoconf: protocol.o ifct.o display.o video_decoder.o tvideo_coder.o \
	   ffct.o soundbyte.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	   audio_coder.o tvideoconf.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	video_decoder.o tvideo_coder.o ffct.o soundbyte.o  ulaw.o adpcm.o \
	hadpcm.o audio_decoder.o audio_coder.o tvideoconf.c \
	$(LIB_DIR)/libXext.a $(LIB_DIR)/libXm.a $(LIB_DIR)/libXt.a \
	$(LIB_DIR)/libX11.a $(LIB_VIDEOPIX) -lm -o $@



rate: rate.c protocol.o
	cc $(FLAGS) protocol.o rate.c -o $@


stat: stat.c protocol.o
	cc $(FLAGS) protocol.o stat.c -o $@


clean:
	rm *.o


