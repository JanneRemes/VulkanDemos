
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14 -Wall
LIBS=$(shell sdl2-config --libs) -lvulkan -lX11-xcb

.PHONY: all clean


all: $(OUTFILE)
	@true

clean:
	rm $(OUTFILE)

$(OUTFILE): $(SOURCES)
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)
