/**************************************************************************
 * GPU simulator definition file.
 * This file contains definitions and includes for the CG1 GPU simulator.
 *
 */

#ifndef __CM_UNIFIEDSHADER_H__

#define __CM_UNIFIEDSHADER_H__
#include "CG1MDLBASE.h"

#include "ConfigLoader.h"
#include "TraceDriverBase.h"
#include "GPUReg.h"

#include "cmStatisticsManager.h"
#include "cmSignalBinder.h"
#include "DynamicMemoryOpt.h"

//  behavior Model.
#include "bmUnifiedShader.h"

//  c model.
#include "cmShaderFetch.h"
#include "cmShaderDecodeExecute.h"
#include "cmShaderFetchVector.h"
#include "cmShaderDecodeExecuteVector.h"

#include <vector>

namespace cg1gpu
{

/**
 *  GPU Simulator class.
 *  The class defines structures and attributes to implement a GPU simulator.
 *
 */

class cmoUnifiedShader : public cmoMduBase 
{
//private:
public:
    cgsArchConfig ArchConf;            //  Structure storing the simulator configuration parameters.  */
    
    //  Emulators.
    bmoUnifiedShader* bmShader;       //  Pointer to the array of pointers to fragment shader emulators.  */

    //  Simulation modules.

    cmoShaderFetch* fshFetch;                    //  Pointer to the array of pointers to cmoShaderFetch modules for the simulated unified/fragment shaders.  */
    cmoShaderDecExe* fshDecExec;          //  Pointer to the array of pointers to cmoShaderDecExe modules for the simulated unified/fragment shaders.  */

    cmoShaderFetchVector* vecShFetch;            //  Pointer to the array of pointers to cmoShaderFetchVector modules for the simulated unified shaders.  */
    cmoShaderDecExeVector* vecShDecExec;  //  Pointer to the array of pointers to cmoShaderDecExeVector modules for the simulated unified shaders.  */

    //  Trace reader+driver.
    cgoTraceDriverBase *TraceDriver;    //  Pointer to a cgoTraceDriverBase object that will provide the MetaStreams to simulate.  */


    bool shaderClockDomain;    //  Flag that stores if the simulated architecture implements a shader clock domain.  */
    bool memoryClockDomain;    //  Flag that stores if the simulated architecture implements a memory clock domain.  */
    bool multiClock;           //  Flag that stores if the simulated architecture implements a multiple clock domains.  */

    static cmoUnifiedShader* current;      //  Stores the pointer to the currently executing GPU simulator instance.  */
    
public:

    /**
     *
     *  cmoUnifiedShader constructor.
     *  Creates and initializes the objects and state required to simulate a GPU.
     *
     *  The emulation objects and and simulation modules are created.
     *  The statistics and signal dump state and files are initialized and created if the simulation
     *  as required by the simulator parameters.
     *
     *  @param ArchConf Configuration parameters.
     *
     */
    cmoUnifiedShader(cgsArchConfig* ArchConf, /*bmoTextureProcessor* BmTexUnit,*/ U32 InstanceId);
    /**
     *  cmoUnifiedShader destructor.
     */
    ~cmoUnifiedShader();

    // dummy functions from cmoMduBase, due to the unified shader is a wrapper for now.
    void clock(U64 cycle) override {};
    
};  // class cmoUnifiedShader
};  //  namespace cg1gpu

#endif
