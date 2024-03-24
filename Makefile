# Compiler
CXX := g++

# FLTK compiler flags
FLTK_CXXFLAGS := $(shell fltk-config --use-images --cxxflags)

# FLTK linker flags
FLTK_LDFLAGS := $(shell fltk-config --use-images --ldflags)

# Source files
SOURCES := image_loader.cpp main.cpp

# Object files
OBJECTS := $(SOURCES:.cpp=.o)

# Executable
EXECUTABLE := dlauncher

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(FLTK_LDFLAGS) -o $@
	strip $@
	rm -f $(OBJECTS)

%.o: %.cxx
	$(CXX) $(FLTK_CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
