SRCS = $(wildcard *.cc)
OBJS = $(patsubst %.cc,%.so,$(SRCS))

CC_OPTIMIZE := -O3 -fomit-frame-pointer -fopenmp -march=native -mtune=native -msse -mfpmath=sse -mssse3 -ffast-math -Wall -fmessage-length=0 -flto

all: $(OBJS) install

%.so: %.cc
	g++ $(CC_OPTIMIZE) -o $@ $<  -shared -fPIC 

install:
	mkdir -p ~/.config/RawTherapee4/filters/
	cp *.so ~/.config/RawTherapee4/filters/

clean:
	rm -f *.so
