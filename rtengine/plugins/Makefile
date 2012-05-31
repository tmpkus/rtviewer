SRCS = $(wildcard *.cc)
OBJS = $(patsubst %.cc,%.so,$(SRCS))

all: $(OBJS)

%.so: %.cc
	g++ -o $@ $<  -shared -fPIC


