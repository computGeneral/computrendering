###########################################################################
#  CG1GPU Makefile. 

TOPDIR = ../..

include $(TOPDIR)/Makefile.defs

#########################################################################

# All source files below this subdirectories
# will be compiled automatically
SRC_DIRS = . 

# Source files to be excluded from SRC_DIRS
SRC_EXCLUDE = ./*
	
# Extra source files to be included
SRC_INCLUDE = \
	testCompressor.cpp

# Directories where compiler will search for includes
INC_DIRS = $(SRC_DIRS) \
	$(COMDIR) $(EMUDIR) $(SIMDIR) \
	$(SIMDIR)/CommandProcessor $(SIMDIR)/MemoryController \
	$(SIMDIR)/MemoryControllerV2 $(SIMDIR)/Shader $(SIMDIR)/Streamer \
	$(SIMDIR)/PrimitiveAssembly $(SIMDIR)/Clipper \
	$(SIMDIR)/Rasterizer $(SIMDIR)/FragmentOperations \
	$(SIMDIR)/DisplayController $(SIMDIR)/Cache $(BASEDIR) $(TRACEDIR) \
	$(TRACEDIR)/OGL14 $(TRACEDIR)/HAL \
	$(TRACEDIR)/utils $(TRACEDIR)/utils/TraceReader \
	$(TRACEDIR)/utils/Gen $(TRACEDIR)/OGL14/ARBP \
	$(TRACEDIR)/utils/log $(TRACEDIR)/OGL14/ARBP/VertexProgram \
	$(TRACEDIR)/OGL14/ARBP/FragmentProgram $(GLDIR) \
	$(TRACEDIR)/OGL14/GLObject $(TRACEDIR)/OGL14/Texture \
	$(TRACEDIR)/OGL14/BufferObject

# C++ compilation flags
CXXFLAGS += -O0 -g

INTERNAL_LIBS = \
	MetaStream \
	d3dpixrunplayer D3D9 \
	gl OGL HAL GALx GAL \
	sim gpu behvmodel support
	
# Library dependences
LIBS += $(INTERNAL_LIBS:%=-l%) -lz

# PROGRAM dependences
PROGRAM_DEPS = $(INTERNAL_LIBS:%=$(LIBDIR)/lib%.a)

# Directories where MAKE will search for target pre-requisites
VPATH = $(SRC_DIRS)

# Generated binary
PROGRAM = testCompressor

#########################################################################

include $(TOPDIR)/Makefile.common
