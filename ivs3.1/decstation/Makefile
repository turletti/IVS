LIB_AUDIO=/usr/kits/XMedia
NEW_LIB=/usr/lib/X11R5

LIBS= $(NEW_LIB)/libXaw.a $(NEW_LIB)/libXmu.a \
      $(NEW_LIB)/libXt.a $(NEW_LIB)/libXext.a  $(NEW_LIB)/libX11.a
         
INCLUDES=-I/usr/include/X11R5
#INCLUDES=-I/usr/include/mit

SOURCES=../src/

FLAGS= -O2 -DDECSTATION 
#-DMULTICAST (UNICAST ONLY !!!)
#-DDECAUDIO 

CFLAGS=$(FLAGS) $(INCLUDES) 
PFLAGS=$(CFLAGS) 

#
# if you have Video TX Hardware then you must include "Xvlib.h"
# in this directory. You have to set VIDEOTX to
#VIDEOTX= -DVIDEOTX
# Else ...
VIDEOTX=

all: ivs ivsd decode_h261 ivs_record ivs_replay rate
  
ulaw.o: $(SOURCES)ulaw.c
	cc $(FLAGS) -c $(SOURCES)ulaw.c

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

video_decoder_file.o: $(SOURCES)video_decoder.c
	cc $(CFLAGS) -DDEBUG -DINPUT_FILE -c \
	$(SOURCES)video_decoder.c -o video_decoder_file.o

adpcm.o: $(SOURCES)adpcm.c
	cc $(FLAGS) -c $(SOURCES)adpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(FLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(FLAGS) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(FLAGS) -c $(SOURCES)audio_coder.c

videodec.o: $(SOURCES)videodec.c 
	cc $(FLAGS) $(VIDEOTX) -c $(SOURCES)videodec.c

video_coder.o: $(SOURCES)video_coder.c 
	cc $(FLAGS) $(VIDEOTX) -c $(SOURCES)video_coder.c

athena.o: $(SOURCES)athena.c
	cc $(CFLAGS) -D_NOPROTO -c $(SOURCES)athena.c

ivs: protocol.o ifct.o display.o video_decoder.o \
     video_coder.o ffct.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
     audio_coder.o athena.o $(SOURCES)ivs.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOTX) protocol.o \
	ifct.o display.o video_decoder.o video_coder.o \
	ffct.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	audio_coder.o athena.o $(SOURCES)ivs.c $(LIBS) -lm -o $@ 

decode_h261: protocol.o ifct.o display.o video_decoder_file.o \
        athena.o $(SOURCES)decode_h261.c
	cc $(CFLAGS) -D_NO_PROTO $(VIDEOPIX) protocol.o ifct.o display.o \
	athena.o video_decoder_file.o $(SOURCES)decode_h261.c \
	$(LIBS) -lm -o $@

ivsd: $(SOURCES)ivsd.c protocol.o
	cc $(CFLAGS) -D_NO_PROTO protocol.o $(SOURCES)ivsd.c \
        $(LIBS) -o $@


ivs_record: $(SOURCES)ivs_record.c protocol.o athena.o display.o
	cc $(INC) $(CFLAGS) -D_NO_PROTO $(SOURCES)ivs_record.c display.o \
        protocol.o athena.o $(NEW_LIB)/libXaw.a $(NEW_LIB)/libXmu.a \
	$(NEW_LIB)/libXt.a $(NEW_LIB)/libXext.a \
	$(NEW_LIB)/libX11.a -lm -o $@


ivs_replay: protocol.o ifct.o display.o video_decoder.o ulaw.o \
            adpcm.o hadpcm.o audio_decoder.o athena.o $(SOURCES)ivs_replay.c
	cc $(INC) $(CFLAGS) -D_NO_PROTO protocol.o ifct.o display.o \
        video_decoder.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	athena.o $(SOURCES)ivs_replay.c $(NEW_LIB)/libXaw.a \
	$(NEW_LIB)/libXmu.a $(NEW_LIB)/libXt.a $(NEW_LIB)/libXext.a \
	$(NEW_LIB)/libX11.a -lm -o $@


rate: $(SOURCES)rate.c protocol.o
	cc $(INC) $(FLAGS) protocol.o $(SOURCES)rate.c -lm -o $@

clean:
	rm *.o


				   





