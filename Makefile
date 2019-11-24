CXX = g++
CXXFLAGS = -std=c++14 $(INCLUDE)
INCLUDE = -Iglutils/include/
LDLIBS = -lpng -lX11 -lGL -lGLEW

all:
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LDLIBS) main.cpp -o run
