CXX = g++
CXXFLAGS = -std=c++14 $(INCLUDE)
INCLUDE = -Iglutils/include/
LDLIBS = -lpng -lX11 -lGL -lGLEW


gl:
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LDLIBS) gl/main.cpp -o bin/gl

all: gl
