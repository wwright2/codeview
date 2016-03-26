# Set your program name
PROGRAM = kbhit


# list of source files
CXXSOURCES := main.cpp
CXXSOURCES += keyboard.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files

CXXFLAGS = 			# compiliation flags
CXX = g++			# compiler

LDFLAGS =			# linker flags

# First target is the default "all"
all: $(PROGRAM)

# Link target: automatically builds its object dependencies before
# executing its link command.
$(PROGRAM): $(CXXOBJECTS)
	$(CXX) -o $@ $(CXXOBJECTS) $(LDFLAGS)


# Clean target: "make -f Makefile.Linux clean" to remove unwanted objects and executables.
clean:
	$(RM) -f $(CXXOBJECTS) $(PROGRAM)
