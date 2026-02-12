/**************************************************************************
 *
 * GPU simulator implementation file.
 * This file contains the implementation of functions for the CG1 GPU simulator.
 */

#include "cmUnifiedShader.h"
#include "TraceDriverMeta.h"
#include "support.h"
#include <ctime>

using namespace std;

namespace arch
{

//  Constructor.
cmoUnifiedShader::cmoUnifiedShader(cgsArchConfig* ArchConf, /*bmoTextureProcessor* bmTextureProcessor,*/ U32 InstanceId) :
    cmoMduBase("US", this)
{
    char **vshPrefix;
    char **fshPrefix;
    char mduName[128];

    //  Allocate pointer array for the vertex shader prefixes.
    vshPrefix = new char*[(ArchConf->gpu.numFShaders > ArchConf->gpu.numVShaders) ? ArchConf->gpu.numFShaders : ArchConf->gpu.numVShaders];
    CG_ASSERT_COND((vshPrefix != NULL), "Error allocating vertex shader prefix array.");//  Check allocation.
    U32 numVShaders = (ArchConf->gpu.numFShaders > ArchConf->gpu.numVShaders) ? ArchConf->gpu.numFShaders : ArchConf->gpu.numVShaders;
    //  Create prefixes for the virtual vertex shaders.
    for(U32 i = 0; i < numVShaders; i++)
    {
        vshPrefix[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((vshPrefix[i] != NULL), "Error allocating vertex shader prefix.");
        sprintf(vshPrefix[i], "VS%d", i); //  Write prefix.
    }

    //  Allocate pointer array for the fragment shader prefixes.
    fshPrefix = new char*[ArchConf->gpu.numFShaders];
    CG_ASSERT_COND((fshPrefix != NULL), "Error allocating fragment shader prefix array.");//  Check allocation.
    //  Create prefixes for the fragment shaders.
    for(U32 i = 0; i < ArchConf->gpu.numFShaders; i++)
    {
        fshPrefix[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((fshPrefix[i] != NULL), "Error allocating fragment shader prefix.");
        sprintf(fshPrefix[i], "FS%d", i); //  Write prefix.
    }

    //  Allocate behaviorModel and mdu arrays for the fragment shaders.
    //  Check allocation.
    ////  Create the fragment shader boxes and emulators.
    //for(U32 i = 0; i < ArchConf->gpu.numFShaders; i++)
    //{
        //  Creat Behavior Model name.
        sprintf(mduName,"FragmentShaderEmulator%d", InstanceId);
        //  Check if vector shader model has been enabled.
        if (ArchConf->ush.useVectorShader)
        {
            //  Create and initialize shader emulation object for FS1.
            bmShader = new bmoUnifiedShader( mduName,                                                //  Shader name.
                                       ArchConf->ras.microTrisAsFragments? UNIFIED_MICRO : UNIFIED, //  Shader model. 
                                       CG_BEHV_MODEL,
                                       ArchConf->ush.vectorThreads * ArchConf->ush.vectorLength,         //  Threads supported by the shader.
                                       false,                                                  //  Store decoded instructions.
                                       //bmTextureProcessor,                                              //  Pointer to the texture behaviorModel attached to the shader.
                                       STAMP_FRAGMENTS,                                        //  Fragments per stamp for texture accesses.
                                       ArchConf->ras.subPixelPrecision                              //  subpixel precision for shader fixed point operations. 
                                       );
            bmShader->SetTP(bmTextureProcessor);
            //  Creat Behavior Model name.
            sprintf(mduName,"cmoShaderFetchVector%d", InstanceId);
            //  Create and initialize vector shader fetch stage mdu.
            vecShFetch = new cmoShaderFetchVector(
                *bmShader,                     //  Fragment Shader behaviorModel.
                ArchConf->ush.vectorThreads,          //  Number of shader threads.
                ArchConf->ush.vectorResources,       //  Number of thread resources.
                ArchConf->ush.vectorLength,          //  Vector length.
                ArchConf->ush.vectorALUWidth,        //  Vector ALU width.
                ArchConf->ush.vectorALUConfig,       //  Configuration of the vector array ALUs.
                ArchConf->ush.fetchDelay,            //  Delay in cycles until the next fetch for a shader thread.
                ArchConf->ush.swapOnBlock,           //  Enable swap thread on block or swap thread always modes.
                ArchConf->ush.textureUnits,          //  Texture Units attached to the shader.
                ArchConf->ush.inputsCycle,           //  Inputs that can be received per cycle.
                ArchConf->ush.outputsCycle,          //  Ouputs that can be sent per cycle.
                ArchConf->ush.outputLatency,         //  Shader output signal maximum latency.
                ArchConf->ras.useMicroPolRast && ArchConf->ras.microTrisAsFragments,  //  Process microtriangle fragment shader inputs and export z values (MicroPolygon cmoRasterizer).
                mduName,
                fshPrefix[InstanceId],
                vshPrefix[InstanceId],
                NULL);



            //  Creat Behavior Model name.
            sprintf(mduName,"VectorShaderDecExec%d", InstanceId);

            //  Compute the ratio between the texture unit clock and the shader clock.
            U32 textureClockRatio = shaderClockDomain ? ((U32) ceilf((F32) ArchConf->gpu.gpuClock / (F32) ArchConf->gpu.shaderClock)) : 1;
            
            //  Create and initialize vector shader decode/execute stages mdu.
            vecShDecExec = new cmoShaderDecExeVector(
                *bmShader,
                ArchConf->ush.vectorThreads,         //  Number of supported threads. 
                ArchConf->ush.vectorLength,          //  Vector length.
                ArchConf->ush.vectorALUWidth,        //  Vector ALU width.
                ArchConf->ush.vectorALUConfig,       //  Configuration of the vector array ALUs.
                ArchConf->ush.vectorWaitOnStall,     //  Flag that defines if an instruction waits until stall conditions are cleared or is discarded.
                ArchConf->ush.vectorExplicitBlock,   //  Flag that defines if threads are blocked by an explicit trigger in the instruction stream or automatically on texture load.  */
                textureClockRatio,              //  Ratio between the texture unit clock and the shader clock.  Used for buffering between Texture Unit and Shader Decode Execute.
                ArchConf->ush.textureUnits,          //  Texture Units attached to the shader.
                ArchConf->ush.textRequestRate,       //  Texture request rate.  
                ArchConf->ush.textRequestGroup,      //  Size of a group of texture requests sent to a texture unit.
                mduName,
                fshPrefix[InstanceId],
                vshPrefix[InstanceId],
                NULL);

           
        }
        else
        {
            //  Create and initialize shader emulation object for FS1.
            bmShader = new bmoUnifiedShader(
                mduName,                                                //  Shader name.
                ArchConf->ras.microTrisAsFragments? UNIFIED_MICRO : UNIFIED, //  Shader model.
                CG_BEHV_MODEL,
                ArchConf->ush.numThreads + ArchConf->ush.numInputBuffers,         //  Threads supported by the shader.
                false,                                                  //  Store decoded instructions.
                //bmTextureProcessor,                                              //  Pointer to the texture behaviorModel attached to the shader.
                STAMP_FRAGMENTS,                                        //  Fragments per stamp for texture accesses.
                ArchConf->ras.subPixelPrecision                              //  Subpixel precision for shader fixed point operations.
                );
            bmShader->SetTP(bmTextureProcessor);

            //  Creat Behavior Model name.
            sprintf(mduName,"FragmentShaderFetch%d", InstanceId);

            //  Create and initialize shader fetch mdu for FS1.
            fshFetch = new cmoShaderFetch(
                *bmShader,                     //  Fragment Shader behaviorModel.
                ArchConf->ush.numThreads,            //  Number of shader threads.
                ArchConf->ush.numInputBuffers,       //  Number of input buffers.
                ArchConf->ush.numResources,          //  Number of thread resources.
                ArchConf->ush.threadRate,            //  Thread from where to fetch per cycle.
                ArchConf->ush.fetchRate,             //  Instructions to fetch per cycle and thread.
                ArchConf->ush.threadGroup,           //  Number of threads that form a group.
                ArchConf->ush.lockedMode,            //  Lock step execution mode of a group of threads.
                ArchConf->ush.scalarALU,             //  Support fetching a scalar ALU in parallel with the SIMD ALU.
                ArchConf->ush.threadWindow,          //  Enable searching for a ready thread in the thread window.
                ArchConf->ush.fetchDelay,            //  Delay in cycles until the next fetch for a shader thread.
                ArchConf->ush.swapOnBlock,           //  Enable swap thread on block or swap thread always modes.
                ArchConf->ush.textureUnits,          //  Texture Units attached to the shader.
                ArchConf->ush.inputsCycle,           //  Inputs that can be received per cycle.
                ArchConf->ush.outputsCycle,          //  Ouputs that can be sent per cycle.
                ArchConf->ush.outputLatency,         //  Shader output signal maximum latency.
                ArchConf->ras.useMicroPolRast && ArchConf->ras.microTrisAsFragments,  //  Process microtriangle fragment shader inputs and export z values (MicroPolygon cmoRasterizer). 
                mduName,
                fshPrefix[InstanceId],
                vshPrefix[InstanceId],
                NULL);


            //  Creat Behavior Model name.
            sprintf(mduName,"FragmentShaderDecExec%d", InstanceId);

            //  Create and initialize shader decode/execute mdu for FS1.
            fshDecExec = new cmoShaderDecExe(
                *bmShader,
                ArchConf->ush.numThreads + ArchConf->ush.numInputBuffers, //  Number of supported threads.
                ArchConf->ush.threadRate,                            //  Thread from where to fetch per cycle.
                ArchConf->ush.fetchRate,                             //  Instructions to fetch per cycle and thread.
                ArchConf->ush.threadGroup,                           //  Number of threads per group.
                ArchConf->ush.lockedMode,                            //  Execute in lock step mode a group of threads.
                ArchConf->ush.textureUnits,                          //  Texture Units attached to the shader.
                ArchConf->ush.textRequestRate,                       //  Texture request rate.
                ArchConf->ush.textRequestGroup,                      //  Size of a group of texture requests sent to a texture unit.
                ArchConf->ush.scalarALU,                             //  Support a scalar ALU in parallel with the SIMD ALU.
                mduName,
                fshPrefix[InstanceId],
                vshPrefix[InstanceId],
                NULL);


        }
        
    //}

}

cmoUnifiedShader::~cmoUnifiedShader()
{
}



}   // namespace arch

