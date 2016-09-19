
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -std=c++14 -Wall -O0 -g
LIBS=$(shell sdl2-config --libs) -lSDL2_image -lvulkan -lX11-xcb

.PHONY: all clean force shaders


all: $(OUTFILE) shaders
	@true

clean:
	rm -f $(OUTFILE) *.spirv

force:
	@true

shaders: vertex.spirv fragment.spirv compute.spirv
	@true

vertex.spirv: compute.vert
	glslangValidator -V -o vertex.spirv compute.vert

fragment.spirv: compute.frag
	glslangValidator -V -o fragment.spirv compute.frag

compute.spirv: compute.comp
	glslangValidator -V -o compute.spirv compute.comp

$(OUTFILE): force
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)

