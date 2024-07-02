LIB_DIR=/net/lib/motif-1.1/sun4OS4/lib
INCLUDES=-I/net/lib/motif-1.1/include

FLAGS= -DDEBUG -g
CFLAGS=$(FLAGS) $(INCLUDES)
LIBS=-L$(LIB_DIR) -lXm -lXt -lX11
LIB_VIDEOPIX=
#
# if you have VideoPix Hardware then you must include "vfc_lib.h" and
# "vfc_ioctls.h" in this directory. You have to set VIDEOPIX to
VIDEOPIX= -DVIDEOPIX
LIB_VIDEOPIX=/usr/1.0-VFC/vfc_lib/libvfc.a
# Else ...
# VIDEOPIX=

all: audiosend audioconf videoconf

soundbyte.o: soundbyte.c
	cc $(FLAGS) -c soundbyte.c

display.o: display.c
	cc $(FLAGS) -c display.c 

ifct.o: ifct.c 
	cc $(FLAGS) -c ifct.c  

ffct.o: ffct.c 
	cc $(FLAGS) -c ffct.c  

protocol.o: protocol.c
	cc $(FLAGS) -c protocol.c

psend: protocol.o psend.c
	cc $(FLAGS) protocol.o psend.c -o $@

preceive: protocol.o audio_decode.o soundbyte.o preceive.c
	cc $(FLAGS) protocol.o audio_decode.o soundbyte.o preceive.c\
 	 libaudio.a -lm -o $@

video_decoder.o: video_decoder.c
	cc $(FLAGS) -c video_decoder.c


audioconf: protocol.o soundbyte.o audio_decoder.o audioconf.c
	cc $(FLAGS) protocol.o soundbyte.o audio_decoder.o\
 	 audioconf.c libaudio.a -lm -o $@

audiosend: protocol.o soundbyte.o audio_decoder.o audio_send.c
	cc $(FLAGS) protocol.o soundbyte.o audio_decoder.o\
 	 audio_send.c libaudio.a -lm -o $@


audio_decoder.o: audio_decoder.c
	cc $(FLAGS) -c audio_decoder.c

audio_coder.o: audio_coder.c
	cc $(FLAGS) -c audio_coder.c

video_coder.o: video_coder.c 
	cc -O1  $(VIDEOPIX) -c video_coder.c



videoconf: protocol.o ifct.o display.o video_decoder.o video_coder.o \
	   ffct.o soundbyte.o audio_decoder.o audio_coder.o videoconf.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o \
	display.o video_decoder.o video_coder.o ffct.o soundbyte.o \
	audio_decoder.o audio_coder.o videoconf.c $(LIB_DIR)/libXm.a \
	$(LIB_DIR)/libXt.a $(LIB_DIR)/libX11.a $(LIB_VIDEOPIX) \
	libaudio.a $(VIDEOPIX) -lm -o $@


clean:
	rm *.o


