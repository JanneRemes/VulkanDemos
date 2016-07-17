
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14 -Wall
LIBS=$(shell sdl2-config --libs) -lvulkan -lX11-xcb

.PHONY: all clean force shaders


all: $(OUTFILE) shaders
	@true

clean:
	rm $(OUTFILE) *.spirv

force:
	@true

shaders: vertex.spirv fragment.spirv
	@true

vertex.spirv: pushconsts.vert
	glslangValidator -V -o vertex.spirv pushconsts.vert

fragment.spirv: pushconsts.frag
	glslangValidator -V -o fragment.spirv pushconsts.frag

$(OUTFILE): force
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)

