# This makefile handles the compilation of the OpenCL matrix operations

.PHONY: all clean prep debug release

EXE := MatrixOpenCL

# OpenCL header directory
HEADERS := CL/

# Library for OpenCL.
OpenCL_Lib := 

# Debug directory and executable
DBGDIR := debug
DBGEXE := $(DBGDIR)/$(EXE)

RELDIR := release
RELEXE := $(RELDIR)/$(EXE)

all: prep debug release

# Clean up any previous builds
clean:
	rm -rf debug/ release/

# Prepare by creating the debug and release directories
prep:
	mkdir -p $(DBGDIR) $(RELDIR)


# Compile the debug version of the code
debug: prep $(HEADERS)/*.h matrixOperations.cpp *.cl
	g++ -DDEBUG -g -fopenmp -lm $(OpenCL_Lib) matrixOperations.cpp -o $(DBGEXE)