
OUTFILE=test
SOURCES=main.cpp

CXX=clang++
CPPFLAGS=$(shell sdl2-config --cflags) -O2 -std=c++14 -Wall
LIBS=$(shell sdl2-config --libs) -lvulkan -lX11-xcb

.PHONY: all clean force shaders


all: $(OUTFILE) shaders
	@true

clean:
	rm -f $(OUTFILE) *.spirv

force:
	@true

shaders: vertex.spirv fragment.spirv
	@true

vertex.spirv: triangle.vert
	glslangValidator -V -o vertex.spirv triangle.vert

fragment.spirv: triangle.frag
	glslangValidator -V -o fragment.spirv triangle.frag

$(OUTFILE): force
	$(CXX) $(CPPFLAGS) $(SOURCES) -o $(OUTFILE) $(LIBS)

