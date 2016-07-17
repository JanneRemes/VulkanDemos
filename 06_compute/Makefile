
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14 -Wall
LIBS=$(shell sdl2-config --libs) -lSDL2_image -lvulkan -lX11-xcb

.PHONY: all clean force shaders


all: $(OUTFILE) shaders
	@true

clean:
	rm $(OUTFILE) *.spirv

force:
	@true

shaders: vertex.spirv fragment.spirv
	@true

vertex.spirv: compute.vert
	glslangValidator -V -o vertex.spirv compute.vert

fragment.spirv: compute.frag
	glslangValidator -V -o fragment.spirv compute.frag

$(OUTFILE): force
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)

