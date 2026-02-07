
CG1_SOURCE_DIR=../..

INCLUDE_DIRS = -I $(CG1_SOURCE_DIR)/support -I $(CG1_SOURCE_DIR)/behvmodel \
               -I $(CG1_SOURCE_DIR)/sim

EXTRA_OBJECTS=ShaderInstruction.o ShaderOptimization.o DynamicMemoryOpt.o support.o

OBJECTS= assembler disassembler optimizer

all: $(OBJECTS)

$(OBJECTS): % : %.cpp $(EXTRA_OBJECTS)
	g++ $@.cpp $(INCLUDE_DIRS) $(EXTRA_OBJECTS) $(LIBRARY_DIRS) $(LIBS) -o $@

ShaderInstruction.o: $(CG1_SOURCE_DIR)/behvmodel/ShaderInstruction.cpp $(CG1_SOURCE_DIR)/behvmodel/ShaderInstruction.h
	g++ -c $(CG1_SOURCE_DIR)/behvmodel/ShaderInstruction.cpp $(INCLUDE_DIRS) -o $@

ShaderOptimization.o: $(CG1_SOURCE_DIR)/behvmodel/ShaderOptimization.cpp $(CG1_SOURCE_DIR)/behvmodel/ShaderOptimization.h
	g++ -c $(CG1_SOURCE_DIR)/behvmodel/ShaderOptimization.cpp $(INCLUDE_DIRS) -o $@

DynamicMemoryOpt.o: $(CG1_SOURCE_DIR)/support/DynamicMemoryOpt.cpp $(CG1_SOURCE_DIR)/support/DynamicMemoryOpt.h
	g++ -c $(CG1_SOURCE_DIR)/support/DynamicMemoryOpt.cpp $(INCLUDE_DIRS) -o $@

support.o: $(CG1_SOURCE_DIR)/support/support.cpp $(CG1_SOURCE_DIR)/support/support.h
	g++ -c $(CG1_SOURCE_DIR)/support/support.cpp $(INCLUDE_DIRS) -o $@
