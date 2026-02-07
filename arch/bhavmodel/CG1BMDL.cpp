/**************************************************************************
 *
 * GPU behaviorModel implementation file.
 *  This file contains the implementation of functions for the CG1 GPU behaviorModel.
 *
 */
#include "CG1BMDL.h"
#include "GlobalProfiler.h"
#include "ImageSaver.h"
#include "bmClipper.h"
#include <iostream>
#include <cstring>

using namespace std;

namespace cg1gpu
{

CG1BMDL::CG1BMDL(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver) :
    GpuBMdl(ArchConf, TraceDriver),
    ArchConf(ArchConf), 
    TraceDriver(TraceDriver),
    AbortSim(false)
{
}

void CG1BMDL::simulationLoop(cgeModelAbstractLevel MAL)
{
    GLOBAL_PROFILER_ENTER_REGION("simulationLoop", "", "")
    traceEnd = false;
    TraceDriver->startTrace(); //  Start the trace driver.
    GpuBMdl.resetState();
    while(!traceEnd && (GpuBMdl.getFrameCounter() < (ArchConf.sim.startFrame + ArchConf.sim.simFrames)) && !AbortSim)
    {
        cgoMetaStream *CurMetaStream;
        GLOBAL_PROFILER_ENTER_REGION("driver", "", "")
        CurMetaStream = TraceDriver->nxtMetaStream(); //  Get next transaction from the driver
        GLOBAL_PROFILER_EXIT_REGION()
        if (CurMetaStream != NULL)
        {
            GpuBMdl.emulateCommandProcessor(CurMetaStream);
        }
        else
        {
            traceEnd = true;
            CG_INFO("End of trace.");
        }
    }
    CG_INFO_COND(AbortSim, "Simulation aborted");
    GLOBAL_PROFILER_EXIT_REGION()
}

void CG1BMDL::abortSimulation()
{
    AbortSim = true;
}

//  Save a snapshot of the current behaviorModel state.
void CG1BMDL::saveSnapshot()
{
    ofstream out;

    out.open("bm.registers.snapshot", ios::binary);

    if (!out.is_open())
        CG_ASSERT("Error creating register snapshot file.");

    //out.write((char *) &state, sizeof(state));
    out.write((char *) GpuBMdl.GetGpuState(), GpuBMdl.GetGpuStateSize());
    out.close();
    
    out.open("bm.gpumem.snapshot", ios::binary);
    
    if (!out.is_open())
        CG_ASSERT("Error creating gpu memory snapshot file.");
    
    out.write((char *) GpuBMdl.GetGpuMemBaseAddr(), ArchConf.mem.memSize * 1024 * 1024);
    out.close();
    
    out.open("bm.sysmem.snapshot", ios::binary);
   
    if (!out.is_open())
        CG_ASSERT("Error creating system memory snapshot file.");
        
    out.write((char *) GpuBMdl.GetSysMemBaseAddr(), ArchConf.mem.mappedMemSize * 1024 * 1024);
    
    out.close();
}

//  Load an behaviorModel state snapshot.
void CG1BMDL::loadSnapshot()
{
    ifstream input;

    input.open("bm.registers.snapshot", ios::binary);

    if (!input.is_open())
        CG_ASSERT("Error opening register snapshot file.");

    //input.read((char *) &state, sizeof(state));
    input.read((char *) GpuBMdl.GetGpuState(), GpuBMdl.GetGpuStateSize());
    input.close();
    
    input.open("bm.gpumem.snapshot", ios::binary);
    
    if (!input.is_open())
        CG_ASSERT("Error opening gpu memory snapshot file.");
    
    input.read((char *) GpuBMdl.GetGpuMemBaseAddr(), ArchConf.mem.memSize * 1024 * 1024);
    input.close();
    
    input.open("bm.sysmem.snapshot", ios::binary);
   
    if (!input.is_open())
        CG_ASSERT("Error opening system memory snapshot file.");
        
    input.read((char *) GpuBMdl.GetSysMemBaseAddr(), ArchConf.mem.mappedMemSize * 1024 * 1024);
    
    input.close();
}

//  Set skip draw call mode.
//
//
//  TODO:
//
//   - bypass attributes in StreamController stage
//   - two sided lighting at setup
//


}  // namespace cg1gpu
