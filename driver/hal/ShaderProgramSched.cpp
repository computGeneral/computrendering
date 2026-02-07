
#include "ShaderProgramSched.h"
#include "HAL.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace cg1gpu;

void ShaderProgramSched::clear()
{   
    totalClears++; // STATISTIC
    shaderInstrMemoryUsage = 0; // After CLEAR, memory not used at all - STATISTIC
    
    progInfo.clear(); // clear map information
    free.assign(free.size(), true); // clear    
    freeSlots = free.size();
}

void ShaderProgramSched::remove(U32 md)
{        
    ProgInfoIt it = progInfo.find(md);
    if ( it != progInfo.end() )
    {
        // deallocate instruction slots
        U32 count = it->second.nInstr;
        U32 offset = it->second.startPC;
        for ( U32 i = 0; i < count; i++ )
            free[i+offset] = true;
         
        shaderInstrMemoryUsage -= count; // STATISTIC  
        // release program information
        progInfo.erase(it);
    }
    // else: md is not a program or it is a program that is not in shader instruction memory
}


bool ShaderProgramSched::isFree(U32 instructionSlot)
{
    if ( instructionSlot >= free.size() )
        CG_ASSERT("Instruction slot too high");
    return free[instructionSlot];
}

ShaderProgramSched::ShaderProgramSched(HAL* driver,U32 maxInstrSlots) :
    driver(driver), free(maxInstrSlots, true), freeSlots(maxInstrSlots), lastVertexShaderMD(0),
    // Statistics
    selectHits(0), totalSelects(0), totalClears(0), programsInMemoryAccum(0), 
    shaderInstrMemoryUsage(0), shaderInstrMemoryUsageAccum(0)
{
}


U32 ShaderProgramSched::reclaimRoom(U32 nInstrSlots, ShaderTarget target)
{    
    U32 count = 0;
    U32 start = 0;
    U32 i;

    if (freeSlots >= nInstrSlots)
    {
        // Enough
        for (i = 0; i < free.size() && count < nInstrSlots; i++)
        {
            if (free[i])
            {
                count++;
                free[i] = false;
            }
            else
            {
                start = i + 1;
                count = 0;
            }
        }
    }
    else
    {
        if (target == VERTEX_TARGET)
        {
            clear();
            start = 0;
            for(U32 i = 0; i < nInstrSlots; i++)
                free[i] = false;
        }
        else
        {
            // Save progInfo

            int md;
            int nInstr;

            ProgInfoIt it = progInfo.find(lastVertexShaderMD);
            if ( it != progInfo.end() )
            {
                md = it->first;
                nInstr = it->second.nInstr;
            }
            else
                CG_ASSERT("Vertex shader is not in the cache");

            // Flush slot cache
            clear(); // free the whole space
            start = 0; // startPC is 0

            select(md, nInstr, VERTEX_TARGET);
            
            start = reclaimRoom(nInstrSlots, target);
        } 
    }

    //  Update the number of free instruction slots.
    freeSlots -= nInstrSlots;
    
    shaderInstrMemoryUsage += nInstrSlots;
          
    return start;
}


void ShaderProgramSched::select(U32 md, U32 nInstr, ShaderTarget target)
{
    totalSelects++; // STATISTIC
    
    GPURegData data;

    ProgInfoIt it = progInfo.find(md);
    if ( it != progInfo.end() ) 
    {         
        selectHits++; // STATISTIC
        programsInMemoryAccum += progInfo.size();
        shaderInstrMemoryUsageAccum += shaderInstrMemoryUsage;
        
        // the program is already in the shader instruction memory
        data.uintVal = it->second.startPC;
        
        //  Check if current target and stored target are the same.
        GPU_ASSERT(
            if (target != it->second.target)
            {
                CG_ASSERT("Stored target and current target are different.");
            }
        )
        
        switch(target)
        {
            case VERTEX_TARGET:
                //  Legacy.
                lastVertexShaderMD = md;
                driver->writeGPURegister(GPU_VERTEX_PROGRAM_PC, 0, data);
                break;
            case FRAGMENT_TARGET:
                //  Legacy.
                driver->writeGPURegister(GPU_FRAGMENT_PROGRAM_PC, 0, data);
                break;
            default:
                driver->writeGPURegister(GPU_SHADER_PROGRAM_PC, target, data);
                break;
        }
        return ;
    }
    // the program is not in the shader instruction memory

    if ( nInstr >= free.size() )
        CG_ASSERT("The selected program is bigger than shader instruction memory");
        
    U32 startPC = reclaimRoom(nInstr, target);
    
    ProgInfoStruct pis;
    pis.startPC = startPC;

    pis.nInstr = nInstr;    
    pis.target = target;
    progInfo.insert(make_pair(md, pis));

    programsInMemoryAccum += progInfo.size(); // STATISTIC
    shaderInstrMemoryUsageAccum += shaderInstrMemoryUsage; // STATISTIC

    loadShaderProgram(target, nInstr, startPC, md);
    
       
}

void ShaderProgramSched::loadShaderProgram(ShaderTarget target, U32 nInstr, U32 startPC, U32 md)
{

    GPURegData data;

    switch(target)
    {
        case VERTEX_TARGET:
            // Legacy.
            lastVertexShaderMD = md;

            // Address where the program is located in the local gpu memory
            driver->writeGPUAddrRegister(GPU_VERTEX_PROGRAM, 0, md);

            // Program's size (in bytes)
            data.uintVal = nInstr * InstructionSize;
            driver->writeGPURegister(GPU_VERTEX_PROGRAM_SIZE, 0, data);
            
            // Offset within the shader instruction memory
            data.uintVal = startPC; // startPC is used as an offset (where to load the program)
            driver->writeGPURegister(GPU_VERTEX_PROGRAM_PC, 0, data);

            // Fetch the whole program from gpu memory to shader instruction memory
            driver->sendCommand(GPU_LOAD_VERTEX_PROGRAM);
            
            break;
            
        case FRAGMENT_TARGET:
            // Legacy.
            
            // Address where the program is located in the local gpu memory
            driver->writeGPUAddrRegister(GPU_FRAGMENT_PROGRAM, 0, md);

            // Program's size (in bytes)
            data.uintVal = nInstr * InstructionSize;
            driver->writeGPURegister(GPU_FRAGMENT_PROGRAM_SIZE, 0, data);
            
            // Offset within the shader instruction memory
            data.uintVal = startPC; // startPC is used as an offset (where to load the program)
            driver->writeGPURegister(GPU_FRAGMENT_PROGRAM_PC, 0, data);

            // Fetch the whole program from gpu memory to shader instruction memory
            driver->sendCommand(GPU_LOAD_FRAGMENT_PROGRAM);
            
            break;
            
        default:
            
            // Address where the program is located in the local gpu memory
            driver->writeGPUAddrRegister(GPU_SHADER_PROGRAM_ADDRESS, 0, md);

            // Program's size (in bytes)
            data.uintVal = nInstr * InstructionSize;
            driver->writeGPURegister(GPU_SHADER_PROGRAM_SIZE, 0, data);
            
            // Offset within the shader instruction memory
            data.uintVal = startPC; // startPC is used as an offset (where to load the program)
            driver->writeGPURegister(GPU_SHADER_PROGRAM_LOAD_PC, 0, data);
            driver->writeGPURegister(GPU_SHADER_PROGRAM_PC, target, data);

            // Fetch the whole program from gpu memory to shader instruction memory
            driver->sendCommand(GPU_LOAD_SHADER_PROGRAM);
            
            break;
    }     
}

void ShaderProgramSched::dump() const
{
    cout << "-------------------------------------------------\n";
    cout << "Layout in Shader Program Sched: ";    
    ProgInfoConstIt it = progInfo.begin();
    for ( ; it != progInfo.end(); it++ )
    {
        cout << "* program memory descriptor: " << it->first;
        cout << "  -> begin: " << it->second.startPC << " last: " 
             << (it->second.startPC + it->second.nInstr - 1) << "   size: " << it->second.nInstr
             << " target : " << it->second.target;
        cout << "\n";
    }
    
    dumpStatistics();
    cout << "-------------------------------------------------";
    
    cout << endl;
}

ShaderProgramSched::Statistics ShaderProgramSched::getStatistics() const
{
    Statistics s;
    s.selectHits = selectHits;
    s.selectMisses = totalSelects - selectHits;
    s.totalClears = totalClears;
    s.programsInMemory = progInfo.size();
    s.programsInMemoryMean = (F32)programsInMemoryAccum / (F32)totalSelects;
    s.shaderInstrMemoryUsage = shaderInstrMemoryUsage;
    s.shaderInstrMemoryUsageMean = (F32)shaderInstrMemoryUsageAccum / (F32)totalSelects;
    return s;
}

void ShaderProgramSched::resetStatistics()
{
    selectHits = 0;
    totalSelects = 0;
    totalClears = 0;
    programsInMemoryAccum = 0;
    shaderInstrMemoryUsageAccum= 0;
}

void ShaderProgramSched::dumpStatistics() const
{
    Statistics s = getStatistics();
    cout << "--------------------------------\n";
    cout << "ShaderProgramSched STATS:\n";
    cout << "HITS in shader instruction memory when selecting a program: " << s.selectHits << "\n";
    cout << "MISSES in shader instruction memory when selecting a program: " << s.selectMisses << "\n";
    cout << "Shader instruction memory flushes (clears): " << s.totalClears << "\n";
    cout << "Current programs in shader instruction memory: " << s.programsInMemory << "\n";
    cout << "Mean of programs in shader instruction memory: " << s.programsInMemoryMean << "\n";
    cout << "Shader instruction memory usage: " << s.shaderInstrMemoryUsage
         << " (" << (U32)(100.0f * ((F32)s.shaderInstrMemoryUsage / (F32)(free.size()))) << "%)\n";
    cout << "Mean of Shader instruction memory usage: " << s.shaderInstrMemoryUsageMean
         << " (" << (U32)(100.0f*(s.shaderInstrMemoryUsageMean / free.size())) << "%)" << endl;
}
