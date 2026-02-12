/**************************************************************************
 * GPU simulator definition file.
 * This file contains definitions and includes for the CG1 GPU simulator.
 *
 */

#ifndef __CM_GPUTOP_H__

#define __CM_GPUTOP_H__
#include "CG1MDLBASE.h"

#include "ArchConfig.h"
#include "TraceDriverBase.h"
#include "GPUReg.h"

#include "cmStatisticsManager.h"
#include "cmSignalBinder.h"
#include "DynamicMemoryOpt.h"

//  behavior Model.
#include "bmUnifiedShader.h"
#include "bmRasterizer.h"
#include "bmTextureProcessor.h"
#include "bmFragmentOperator.h"
#include "ColorCompressor.h"
#include "DepthCompressor.h"
#include "CG1BMDL.h" //TODO to be replaced by bmGpuTop.h

//  c model.
//#include "cmShaderFetch.h"
//#include "cmShaderDecodeExecute.h"
//#include "cmShaderFetchVector.h"
//#include "cmShaderDecodeExecuteVector.h"
#include "cmUnifiedShader.h"
#include "cmCommandProcessor.h"
#include "cmMemoryController.h"
#include "MemoryControllerSelector.h"
#include "cmStreamer.h"
#include "cmPrimitiveAssembly.h"
#include "cmClipper.h"
#include "cmRasterizer.h"
#include "cmTextureProcessor.h"
#include "cmZStencilTestV2.h"
#include "cmColorWriteV2.h"
#include "cmDisplayController.h"

#include <vector>

namespace arch
{

/**
 *  GPU Simulator class.
 *  The class defines structures and attributes to implement a GPU simulator.
 *
 */

class cmoGpuTop //: public cmoMduBase 
{
//private:
public:
    cgsArchConfig ArchConf;            //  Structure storing the simulator configuration parameters.  */
    
    //  Emulators.
    bmoUnifiedShader **bmVtxShader;       //  Pointer to the array of pointers to vertex shader emulators.  */
    bmoUnifiedShader **bmFragShader;       //  Pointer to the array of pointers to fragment shader emulators.  */
    bmoTextureProcessor **bmTexture;      //  Pointer to the array of pointers to texture emulators.  */
    bmoRasterizer *bmRaster;   //  Pointer to a rasterizer behaviorModel.  */
    bmoFragmentOperator **bmFragOp;  //  Pointer to the array of pointers to fragment operation emulators.  */

    //  Simulation modules.
    cmoCommandProcessor* CP;                //  Pointer to the cmoCommandProcessor module.  */
    cmoMduBase* MC;                        //  Pointer to the MemoryController module (top level).  */
    cmoStreamController* StreamController;                        //  Pointer to the cmoStreamController module (top level).  */
    cmoPrimitiveAssembler* PrimAssembler;              //  Pointer to the PrimitiveAssembly module.  */
    cmoClipper* clipper;                          //  Pointer to the cmoClipper module.  */
    cmoRasterizer* Raster;                          //  Pointer to the cmoRasterizer module (top level).  */

    //cmoShaderFetch **fshFetch;                    //  Pointer to the array of pointers to cmoShaderFetch modules for the simulated unified/fragment shaders.  */
    //cmoShaderDecExe **fshDecExec;          //  Pointer to the array of pointers to cmoShaderDecExe modules for the simulated unified/fragment shaders.  */
    //cmoShaderFetchVector **vecShFetch;            //  Pointer to the array of pointers to cmoShaderFetchVector modules for the simulated unified shaders.  */
    //cmoShaderDecExeVector **vecShDecExec;  //  Pointer to the array of pointers to cmoShaderDecExeVector modules for the simulated unified shaders.  */
    cmoUnifiedShader** cmUnifiedShader;

    cmoTextureProcessor **cmTexture;                    //  Pointer to the array of pointers to cmoTextureProcessor modules.  */
    cmoDepthStencilTester **zStencilV2;               //  Pointer to the array of pointers to ZStencilTest modules.  */
    cmoColorWriter **colorWriteV2;               //  Pointer to the array of pointers to ColorWrite modules.  */
    cmoDisplayController *dac;                                  //  Pointer to the cmoDisplayController module.  */

    //  Trace reader+driver.
    cgoTraceDriverBase *TraceDriver;    //  Pointer to a cgoTraceDriverBase object that will provide the MetaStreams to simulate.  */


    bool shaderClockDomain;    //  Flag that stores if the simulated architecture implements a shader clock domain.  */
    bool memoryClockDomain;    //  Flag that stores if the simulated architecture implements a memory clock domain.  */
    bool multiClock;           //  Flag that stores if the simulated architecture implements a multiple clock domains.  */

    std::vector<cmoMduBase*> MduArray;  // Stores a pointer to all the modules in the simulated architecture.
                                 // 0: CP, memCtrl, StreamCtrl, primAsm, Cliper, Raster, 
                                 // vecShFetch[i], vecShExec[i], texUnit[i], zStencilV2[i], colorWrite[i],
                                 // DispCtrl
    std::vector<cmoMduBase*> GpuDomainMduArray;              //  Stores a pointer to all the modules in the GPU clock domain (unified clock).  */
    std::vector<cmoMduMultiClk*> ShaderDomainMduArray; //  Stores a pointer to all the modules with GPU and shader clock domain (multiple domains).  */
    std::vector<cmoMduMultiClk*> MemoryDomainMduArray; //  Stores a pointer to all the modules with GPU and memory clock domain (multiple domains).  */



    static cmoGpuTop* current;      //  Stores the pointer to the currently executing GPU simulator instance.  */
    
public:

    /**
     *
     *  cmoGpuTop constructor.
     *  Creates and initializes the objects and state required to simulate a GPU.
     *
     *  The emulation objects and and simulation modules are created.
     *  The statistics and signal dump state and files are initialized and created if the simulation
     *  as required by the simulator parameters.
     *
     *  @param ArchConf Configuration parameters.
     *  @param TraceDriver Pointer to the driver that provides MetaStreams for simulation.
     *  @param unified  Boolean value that defines if the simulated architecture implements unified shaders.
     *  @param d3dTrace Boolean value that defines if the input trace is a D3D9 API trace (PIX).
     *  @param oglTrace Boolean value that defines if the input trace is an OpenGL API trace.
     *  @param MetaTrace Boolean value that defines if the input trace is an MetaStream trace.
     *
     */
    cmoGpuTop(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver);

    /**
     *  cmoGpuTop destructor.
     */
    ~cmoGpuTop();
    
};  // class cmoGpuTop
};  //  namespace arch

#endif
