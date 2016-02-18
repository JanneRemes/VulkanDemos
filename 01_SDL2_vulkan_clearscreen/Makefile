
OUTFILE=test
SOURCES=main.cpp

LIBS=$(shell sdl2-config --libs) -lvulkan -lxcb

CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14

.PHONY: all clear

all: $(OUTFILE)
	@true

clear:
	rm $(OUTFILE)

$(OUTFILE): $(SOURCES)
	clang++ $(SOURCES) -o $(OUTFILE) $(LIBS) $(CPPFLAGS)

