X11_LIBDIR=/usr/lib/
X11_INCLUDE=/opt/X11R5/include

VIDEOPIX_LIBDIR=/usr/1.0-VFC/vfc_lib
VIDEOPIX_INCLUDE=$(VIDEOPIX_LIBDIR) -I/usr/1.0-VFC/sys

PARALLAX_HOME=/usr/parallax/components/Xclients_sun4.ow3_1.15
PARALLAX_LIBDIR=$(PARALLAX_HOME)/lib
PARALLAX_INCLUDE=$(PARALLAX_HOME)/include

INCLUDE=-I/usr/sys/4.1.3.multicast
SOURCES=../pymsrc/

OFLAG= -O2
WFLAGS= -DSPARCSTATION -DMITSHM
FLAGS= $(OFLAG) $(WFLAGS)
CFLAGS=$(FLAGS) $(INCLUDE)
PFLAGS=$(CFLAGS) -DMULTICAST

ALL=makewhat

all: $(ALL)

pix: ivs.pix ivsd ivs_record ivs_replay rate decode_h261

pax: ivs.pax ivsd ivs_record ivs_replay rate decode_h261

makewhat:
	@echo ''
	@echo '	Make what? You must tell which frame grabber to make IVS for.'
	@echo '	Currently supported boards for SPARCstations running SunOS4'
	@echo '	are Parallax Xvideo (make pax) and Sun VideoPix (make pix).'

ulaw.o: $(SOURCES)ulaw.c
	cc $(PFLAGS) -c $(SOURCES)ulaw.c

soundsparc.o: $(SOURCES)soundsparc.c
	cc $(PFLAGS) -c $(SOURCES)soundsparc.c

display.o: $(SOURCES)display.c
	cc $(PFLAGS) -c $(SOURCES)display.c 

ifct.o: $(SOURCES)ifct.c 
	cc $(PFLAGS) -DSCALE=12 -c $(SOURCES)ifct.c  

ffct.o: $(SOURCES)ffct.c 
	cc $(PFLAGS) -DSCALE=12 -DFIX=0 -c $(SOURCES)ffct.c  

protocol.o: $(SOURCES)protocol.c
	cc $(PFLAGS) -c $(SOURCES)protocol.c

video_decoder.o: $(SOURCES)video_decoder.c
	cc $(PFLAGS) -c $(SOURCES)video_decoder.c

video_decoder_file.o: $(SOURCES)video_decoder.c
	cc $(PFLAGS) -DINPUT_FILE -c \
	$(SOURCES)video_decoder.c -o video_decoder_file.o

adpcm.o: $(SOURCES)adpcm.c
	cc $(PFLAGS) -c $(SOURCES)adpcm.c

hadpcm.o: $(SOURCES)hadpcm.c
	cc $(PFLAGS) -c $(SOURCES)hadpcm.c

audio_decoder.o: $(SOURCES)audio_decoder.c
	cc $(PFLAGS) -c $(SOURCES)audio_decoder.c

audio_coder.o: $(SOURCES)audio_coder.c
	cc $(PFLAGS) -c $(SOURCES)audio_coder.c

videosparc_vfc.o: $(SOURCES)videosparc_vfc.c 
	cc $(PFLAGS) -DVIDEOPIX -I$(VIDEOPIX_INCLUDE) -c \
	$(SOURCES)videosparc_vfc.c 

videosparc_px.o: $(SOURCES)videosparc_px.c
	cc $(PFLAGS) -DLOW_LEVEL -DSUN_PX_VIDEO -I$(PARALLAX_INCLUDE) -c \
	$(SOURCES)videosparc_px.c

video_coder.o: $(SOURCES)video_coder.c $(SOURCES)pyramidal.h
	cc $(PFLAGS) -c $(SOURCES)video_coder.c

athena.o: $(SOURCES)athena.c
	cc $(PFLAGS) -D_NO_PROTO -c $(SOURCES)athena.c

ivs.pax: protocol.o ifct.o display.o video_decoder.o video_coder.o ffct.o \
     soundsparc.o ulaw.o adpcm.o hadpcm.o audio_decoder.o audio_coder.o \
     videosparc_px.o athena.o $(SOURCES)ivs.c
	cc $(CFLAGS) -D_NO_PROTO -DLOW_LEVEL -DSUN_PX_VIDEO protocol.o ifct.o \
	display.o video_decoder.o video_coder.o ffct.o soundsparc.o ulaw.o \
	adpcm.o hadpcm.o audio_decoder.o videosparc_px.o audio_coder.o \
	athena.o $(SOURCES)ivs.c -L$(PARALLAX_LIBDIR) -lXvid -L$(X11_LIBDIR) \
	-lXaw -lXmu -lXt -lXext -lX11 -lm -o ivs


ivs.pix: protocol.o ifct.o display.o video_decoder.o videosparc_vfc.o \
     video_coder.o ffct.o soundsparc.o ulaw.o adpcm.o hadpcm.o \
     audio_decoder.o audio_coder.o athena.o $(SOURCES)ivs.c
	cc $(PFLAGS) -D_NO_PROTO -DVIDEOPIX -I$(VIDEOPIX_INCLUDE) protocol.o \
	ifct.o display.o video_decoder.o videosparc_vfc.o video_coder.o \
	ffct.o soundsparc.o ulaw.o adpcm.o hadpcm.o audio_decoder.o \
	audio_coder.o athena.o $(SOURCES)ivs.c -L$(VIDEOPIX_LIBDIR) \
	-lvfc -L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o ivs

decode_h261: protocol.o ifct.o display.o video_decoder_file.o \
        athena.o $(SOURCES)decode_h261.c
	cc $(PFLAGS) -D_NO_PROTO protocol.o ifct.o display.o \
	athena.o video_decoder_file.o $(SOURCES)decode_h261.c \
	-L$(X11_LIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -lm -o $@

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
	@rm -f *.o core [adir]* *~


