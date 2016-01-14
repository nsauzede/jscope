TARGET=jscope.exe
TARGET+=jfft.exe
TARGET+=jsine.exe

CFLAGS=-Wall -Werror
CFLAGS+=-g -O0
CFLAGS+=`sdl-config --cflags`
LDFLAGS+=`sdl-config --libs`

UNAME=$(shell uname)
ifeq ($(UNAME),MINGW32_NT-6.1)
WIN32=1
endif

ifdef WIN32
LDFLAGS+=-mno-windows
JACK="/c/Program Files (x86)/Jack"
CFLAGS+=-I$(JACK)/includes
#LDFLAGS+=-L$(JACK)/lib -ljack
LDFLAGS+=$(JACK)/lib/libjack.lib
FFTP="${HOME}/tmp/build-fftw/the_install"
FFTPinc=$(FFTP)/include
FFTPlib=$(FFTP)/lib
F_LDFLAGS= -L$(FFTPlib)

#YACAPI=~/tmp/build-yacapi-compat/install
#CFLAGS+=`$(YACAPI)/bin/yacapi-config --compat --cflags`
#LDFLAGS+=`$(YACAPI)/bin/yacapi-config --static-libs`
LDFLAGS+=-lpthread
F_CFLAGS+= -I$(FFTPinc)
else
LDFLAGS+=-ljack
endif

F_CFLAGS+= -DFFTW_EXECUTE=fftw_execute
F_CFLAGS+= -DFFTW_PLAN=fftw_plan
F_CFLAGS+= -DFFTW_DESTROY_PLAN=fftw_destroy_plan
F_CFLAGS+= -DFFTW_PLAN_R2R_1D=fftw_plan_r2r_1d
F_CFLAGS+= -DFFTW_TYPE=double
F_CFLAGS+= -DFFT
F_LDFLAGS= -lfftw3 -lm

all:$(TARGET)

jfft.exe:CFLAGS+=$(F_CFLAGS)
jfft.exe:LDFLAGS+=$(F_LDFLAGS)
jsine.exe:LDFLAGS+=$(F_LDFLAGS)

%.exe:	%.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) *.o

clobber: clean
	$(RM) *~
