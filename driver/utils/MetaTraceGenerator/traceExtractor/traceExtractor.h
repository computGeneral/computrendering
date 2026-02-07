/**************************************************************************
 *
 * traceExtractor definition file
 *
 */
 
 
/**
 *
 *  @file traceExtractor.h
 *
 *  This file contains definitions the tool that extracts MetaStreams for a frame region of
 *  the input tracefile.
 *
 */
 
#ifndef _EXTRACTTRACEREGION_

#define _EXTRACTTRACEREGION_

#include "GPUType.h"
#include "zfstream.h"
#include "MetaStream.h" 

struct UploadRegionIdentifier
{
    U32 startAddress;
    U32 endAddress;

    UploadRegionIdentifier(U32 start, U32 end) :
        startAddress(start), endAddress(end)
    {}
    
    friend bool operator<(const UploadRegionIdentifier& lv,
                          const UploadRegionIdentifier& rv)
    {
        return (lv.endAddress < rv.startAddress);
    }

    friend bool operator==(const UploadRegionIdentifier& lv, 
                           const UploadRegionIdentifier& rv)
    {
        return ((lv.startAddress <= rv.startAddress) && (lv.endAddress >= rv.startAddress)) ||
               ((lv.startAddress <= rv.endAddress) && (lv.endAddress >= rv.endAddress)) || 
               ((lv.startAddress >= rv.startAddress) && (lv.startAddress <= rv.endAddress));
    }
};

/**
 *
 *  This structure holds information about a GPU memory region where data has been uploaded
 *  by an MetaStream.
 *
 */
 
struct UploadRegion
{
    U32 address;         //  Start address of the upload region.  
    U32 size;            //  Size in bytes of the upload region.  
    U32 lastMD;          //  Last memory descriptor identifier associated with the upload region.  
    U32 lastMetaStreamTransID;  //  Last MetaStream identifier associated with the upload region.  

    //  Constructor.
    UploadRegion(U32 addr, U32 sz, U32 md, U32 agpID) :
        address(addr), size(sz), lastMD(md), lastMetaStreamTransID(agpID)
    {}
};
 

/**
 *
 *  This structure holds information about a shader program or a block of splitted code uploaded into the
 *  GPU by an MetaStream.
 *
 */
 
struct ShaderProgram
{
    U32 pc;                  //  Start PC (address in shader program memory) of the shader program or code block.  
    U32 address;             //  Address in memory from which the Shader Program was loaded.  
    U32 size;                //  Size in bytes of the shader program or code block.    
    U32 instructions;        //  Number of instructions in the shader program or code block.  
    U32 lastMetaStreamTransID;      //  Last MetaStream identifier associated with the upload region.  

    //  Size of a shader instruction in bytes.
    static const U32 SHADERINSTRUCTIONSIZE = 16;

    //  Number of instructions that can be stored in the shader program memory
    static const U32 MAXSHADERINSTRUCTIONS = 512;
    
    //  Constructor.
    ShaderProgram(U32 pc_, U32 addr, U32 instr, U32 agpID) :
        pc(pc_), address(addr), size(instr * SHADERINSTRUCTIONSIZE), instructions(instr), lastMetaStreamTransID(agpID)
    {
    }
};


#define SHADER_PROGRAM_HASH_SIZE 1025

typedef map<U32, ShaderProgram> ShaderProgramMap;

bool checkAndCopyMetaStreamTraceHeader(gzifstream *in, gzofstream *out, U32 startFrame, U32 extractFrames);
void insertUploadRegion(U32 address, U32 size, U32 agpTransID, U32 md);
bool checkUploadRegion(U32 address, U32 size, U32 md, U32 agpTransID);
void insertTouchedRegion(UploadRegion &upRegion);
bool checkRegionTouched(U32 md);
void insertFragmentProgram();
void insertVertexProgram();
void insertShaderProgram(U32 pc, U32 address, U32 size, ShaderProgramMap *shaderPrograms);
void checkFragmentProgram(cg1gpu::cgoMetaStream *agpTrans, U32 agpTransID);
void checkVertexProgram(cg1gpu::cgoMetaStream *agpTrans, U32 agpTransID);
void checkShaderProgram(U08 *data, U32 agpTransID, ShaderProgramMap *shaderPrograms, U08 *shaderCache);
void loadFragmentShaderPrograms(gzofstream *outTrace);
void loadVertexShaderPrograms(gzofstream *outTrace);
void loadShaderPrograms(gzofstream *outTrace, U08 *shaderProgramCache, cg1gpu::GPURegister addressReg,
                        cg1gpu::GPURegister pcReg, cg1gpu::GPURegister sizeReg, cg1gpu::GPUCommand loadCommand);



#endif

