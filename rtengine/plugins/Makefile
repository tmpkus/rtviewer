SRCS = $(wildcard *.cc)
OBJS = $(patsubst %.cc,%.so,$(SRCS))

CC_OPTIMIZE := -O3 -fopenmp -march=native -msse4 -mfpmath=sse -ffast-math -Wall -fmessage-length=0 

all: $(OBJS) install

%.so: %.cc
	g++ $(CC_OPTIMIZE) -o $@ $<  -shared -fPIC 

install:
	cp *.so ~/.config/RawTherapee/filters/

clean:
	rm -f *.so
