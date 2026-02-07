
#ifndef __SHADER_SCHEDULER_H__
#define __SHADER_SCHEDULER_H__

#include <map>
#include <vector>
#include "GPUReg.h"

class HAL;

class ShaderProgramSched
{
public:

    struct Statistics
    {
        U32 selectHits; // a program does not require a LOAD command
        U32 selectMisses; // a program requires a LOAD command
        U32 totalClears; // shader instruction memory flushes
        U32 programsInMemory; // current programs in shader instruction memory
        F32 programsInMemoryMean; // Mean of programs in memory (sampled at each select method call)
        U32 shaderInstrMemoryUsage; // Currently
        F32 shaderInstrMemoryUsageMean; // Mean of shader instruction memory used (sampled at each select method call)
    };
    
    static const U32 InstructionSize = 16; // 16 bytes
    
    ShaderProgramSched(HAL* driver, U32 maxInstrSlots);

    Statistics getStatistics() const;
    
    void resetStatistics();

    void clear(); // reset the shader program sched state (equals to remove all md's with remove method)
    void remove(U32 md); // must be called every time a md is removed from GPU driver
    void select(U32 md, U32 nInstr, cg1gpu::ShaderTarget target); // add (if required) and select as current
    void loadShaderProgram (cg1gpu::ShaderTarget target, U32 nInstr, U32 startPC, U32 md);
    
    U32 instructionSlots() const;
    
    // defaults to clear the shader instruction memory when is full
    // you can rewrite this method in subclasses to allow a more
    // complex behaviour
    virtual U32 reclaimRoom(U32 nInstrSlots, cg1gpu::ShaderTarget target);
    
    virtual void dump() const;
    
    void dumpStatistics() const;

protected:

    // Used to release&allocate&check room in the subclasses
    //void release(U32 md);
    //bool allocate(U32 md, U32 startSlot, U32 nInstrSlots);
    bool isFree(U32 instructionSlot);
    

private:

    std::vector<bool> free;

    struct ProgInfoStruct
    {
        U32 startPC; // Start slots
        U32 nInstr; // # of instruction slots
        cg1gpu::ShaderTarget target;    //  Shader target of the program.
    };

    typedef std::map<U32,ProgInfoStruct> ProgInfo;
    typedef ProgInfo::iterator ProgInfoIt;
    typedef ProgInfo::const_iterator ProgInfoConstIt;
    // <descriptor, progInfo>
    ProgInfo progInfo;

    HAL* driver;
    
    // internal statistics
    U32 selectHits;
    U32 totalSelects;
    U32 totalClears;
    U32 programsInMemoryAccum; // Computed each time that select is called    
    U32 shaderInstrMemoryUsage;
    U32 shaderInstrMemoryUsageAccum;
    U32 freeSlots;
    U32 lastVertexShaderMD;

};

#endif // __SHADER_SCHEDULER_H__
