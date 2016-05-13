
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14 -Wall
LIBS=$(shell sdl2-config --libs) -lvulkan -lX11-xcb

.PHONY: all clean force


all: $(OUTFILE)
	@true

clean:
	rm $(OUTFILE)

force:
	@true

$(OUTFILE): force
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)
