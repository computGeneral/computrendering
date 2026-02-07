###########################################################################
# 
#  traceExtractor Makefile. 
# 
# 

TOPDIR = ../../..

include $(TOPDIR)/Makefile.defs

#########################################################################

# All source files below this subdirectories
# will be compiled automatically
SRC_DIRS = .

# Source files to be excluded from SRC_DIRS
SRC_EXCLUDE = 
	
# Extra source files to be included
SRC_INCLUDE = 

# Directories where compiler will search for includes
INC_DIRS = $(SRC_DIRS) \
	$(CG1DIR) \
	$(COMDIR) $(EMUDIR) $(SIMDIR) \
	$(SIMDIR)/CommandProcessor $(SIMDIR)/MemoryController \
	$(SIMDIR)/MemoryControllerV2 $(SIMDIR)/Shader $(SIMDIR)/Streamer \
	$(SIMDIR)/PrimitiveAssembly $(SIMDIR)/Clipper \
	$(SIMDIR)/Rasterizer $(SIMDIR)/FragmentOperations \
	$(SIMDIR)/DisplayController $(SIMDIR)/Cache $(BASEDIR) $(MetaStreamDIR) \
	$(OGLDIR)/OGL14 $(HALDIR) \
	$(DRIVDIR)/utils $(DRIVDIR)/utils/TraceReader \
	$(DRIVDIR)/utils/Gen $(OGLDIR)/OGL14/ARBP \
	$(DRIVDIR)/utils/log $(OGLDIR)/OGL14/ARBP/VertexProgram \
	$(OGLDIR)/OGL14/ARBP/FragmentProgram $(GLDIR) \
	$(OGLDIR)/OGL14/GLObject $(OGLDIR)/OGL14/Texture \
	$(OGLDIR)/OGL14/BufferObject

# C++ compilation flags
ifeq ($(UNIFIED),yes)
	CXXFLAGS += -D UNIFIEDSHADER
else
	CXXFLAGS += 
endif

# Library dependences
LIBS += -lMetaStream -lCG1CMDL -lCG1Common -lz

# Directories where MAKE will search for target pre-requisites
VPATH = $(SRC_DIRS)

# Generated binary
PROGRAM = traceExtractor

#########################################################################

include $(TOPDIR)/Makefile.common

