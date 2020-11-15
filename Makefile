CXX = g++
CXXFLAGS = -std=c++14 $(INCLUDE)
INCLUDE = -Iglutils/include/
LDLIBS = -lpng -lX11 -lGL -lGLEW

.PHONY: gl all clean

gl: gl/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LDLIBS) gl/main.cpp -o bin/gl

test: gl
	cat data/image.png | bin/gl > test.c.png
	python py/radon.py data/image.png test.py.png
	diff test.c.png test.py.png

all: gl
