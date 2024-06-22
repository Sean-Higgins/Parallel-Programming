# This makefile handles the compilation of the matrix operations written in CUDA.

.PHONY: all clean prep debug release

EXE := MatrixCUDA

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
debug: prep *.h matrixOperations.cu


# Compile the release version of the code without any debugging features.
release: prep *.h matrixOperations.cu