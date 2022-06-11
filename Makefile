TARGET:=
TARGET+=jscope.exe
TARGET+=jfft.exe
TARGET+=jsine.exe

CFLAGS=-Wall -Werror
CFLAGS+=-g -O0
SDL_CFLAGS+=`sdl2-config --cflags`
SDL_LDFLAGS+=`sdl2-config --libs`

UNAME=$(shell uname)
ifeq ($(UNAME),MINGW64_NT-6.3)
WIN32=1
endif

ifdef WIN32
LDFLAGS+=-mno-windows
JACK="/c/Program Files (x86)/Jack"
CFLAGS+=-I$(JACK)/includes
#LDFLAGS+=-L$(JACK)/lib -ljack
LDFLAGS+=$(JACK)/lib/libjack64.lib
FFTP="${HOME}/tmp/build-fftw/the_install"
FFTPinc=$(FFTP)/include
FFTPlib=$(FFTP)/lib
F_LDFLAGS= -L$(FFTPlib)

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

LDLIBS+=-lm

all:$(TARGET)

jfft.exe:CFLAGS+=$(F_CFLAGS) $(SDL_CFLAGS)
jscope.exe:CFLAGS+=$(SDL_CFLAGS)
jscope.exe:LDFLAGS+=$(SDL_LDFLAGS)
jfft.exe:LDFLAGS+=$(F_LDFLAGS) -lfftw3 -lm $(SDL_LDFLAGS)
jsine.exe:LDFLAGS+=$(F_LDFLAGS)

%.exe:	%.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(TARGET) *.o

clobber: clean
	$(RM) *~
