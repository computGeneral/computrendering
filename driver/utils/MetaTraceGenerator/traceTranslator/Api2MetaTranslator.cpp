/**************************************************************************
 *
 * traceTranslator implementation file
 *  This file contains the implementation code for the preprocessor tool that translates a trace of OpenGL API calls
 *  in a trace of MetaStreams for the CG1 GPU simulator.
 *
 */

#include <cstdio> 
#include <ctime>
#include <iostream>
//#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>
//#endif
#include <new>
#include <cstdlib>
#include <cstring>

#include "support.h"
#include "zfstream.h"
#include "ConfigLoader.h"
#include "DynamicMemoryOpt.h"
#include "TraceDriverOGL.h"
#include "TraceDriverD3D.h"

#include "MetaStreamTrace.h"
#include "ShaderArchParam.h"

using namespace std;
using namespace cg1gpu;

//  Trace reader+driver.
cgoTraceDriverBase *TraceDriver;

//  Simulator parameters.
cgsArchConfig simP;

//  Counts the frames (swap commands).
U32 frameCounter;

//  Stores the number of batches rendered.
U32 batchCounter;

//  Out of memory handler function.
void out_of_memory()
{
    cerr << "CG1GpuSim::out_of_memory -> Memory exhausted. Aborting" << endl;
    exit(-1);
}


bool has_extension(const string file_name, const string extension)
{
    size_t pos = file_name.find_last_of(".");
    if(pos == file_name.npos)
        return false;
    else
    {
        string file_extension = file_name.substr(pos + 1);
        string extension_uc = extension;
        for(int i=0; i!=file_extension.length(); ++i)
            extension_uc[i] = toupper(extension[i]);
        for(int i=0; i!=file_extension.length(); ++i)
            file_extension[i] = toupper(file_extension[i]);
        return file_extension.compare(extension_uc) == 0;
    }
}


//  Main function
int main(int argc, char *argv[])
{
    //  Set function handles for out of memory event.    
    set_new_handler(out_of_memory);

    ConfigLoader *cl;

    //  Open the configuration file.
    cl = new ConfigLoader("../../arch/config/CG1GPU.ini");

    //  Get all the simulator from the configuration file.
    cl->getParameters(&simP);

    //  Delete the loader.
    delete cl;

    //  Check if the vector alu configuration is scalar (Scalar).
    string aluConf(simP.fsh.vectorALUConfig);    
    bool vectorScalarALU = simP.fsh.useVectorShader && (aluConf.compare("scalar") == 0);

    U32 instructionsToSchedulePerCycle = 0;
    
    //  Compute the number of shader instructions to schedule per cycle that is passed to the driver.
    if (simP.fsh.useVectorShader)
        //  Only the simd4+scalar architecture requires scheduling more than 1 instruction per cycle.
        instructionsToSchedulePerCycle = (aluConf.compare("simd4+scalar") == 0) ? 2 : 1;
    else
        //  Use the 'FetchRate' (instructions fetched per cycle) parameter for the old shader model.
        instructionsToSchedulePerCycle = simP.fsh.fetchRate;
    
    //  Check configuration parameters.
    if (simP.fsh.vAttrLoadFromShader && !simP.enableDriverShTrans)
    {
        CG_ASSERT("Vertex attribute load from shader requires driver shader program translation to be enabled.");
    }
    if (simP.fsh.useVectorShader && vectorScalarALU && !simP.enableDriverShTrans)
    {
        CG_ASSERT("Vector Shader Scalar architecture requires driver shader program translation to be enabled.");
    }

    if (simP.fsh.fixedLatencyALU)
    {
        if (simP.fsh.useVectorShader && vectorScalarALU)
            ShaderArchParam::getShaderArchParam()->selectArchitecture("ScalarFixLat");          
        else
            ShaderArchParam::getShaderArchParam()->selectArchitecture("SIMD4FixLat");
    }
    else
    {
        if (simP.fsh.useVectorShader && vectorScalarALU)
            ShaderArchParam::getShaderArchParam()->selectArchitecture("ScalarVarLat");          
        else
            ShaderArchParam::getShaderArchParam()->selectArchitecture("SIMD4VarLat");
    }
    
    // Configure HAL memory size in Kbytes and block and sblock sizes
    HAL::getHAL()->setGPUParameters(
                                    simP.mem.memSize * 1024,
                                    simP.mem.mappedMemSize * 1024,
                                    simP.fsh.texBlockDim,
                                    simP.fsh.texSuperBlockDim,
                                    simP.ras.scanWidth,
                                    simP.ras.scanHeight,
                                    simP.ras.overScanWidth,
                                    simP.ras.overScanHeight,
                                    simP.doubleBuffer,
                                    simP.forceMSAA,
                                    simP.msaaSamples,
                                    simP.forceFP16ColorBuffer,
                                    instructionsToSchedulePerCycle,
                                    simP.mem.memoryControllerV2,
                                    simP.mem.v2SecondInterleaving,
                                    simP.fsh.vAttrLoadFromShader,
                                    vectorScalarALU,
                                    //simP.enableDriverShTrans  // Disabled when creating/processing MetaStreamTrans traces
                                    false,
                                    //(simP.ras.useMicroPolRast && simP.ras.microTrisAsFragments) // Disabled when creating/processing MetaStreamTrans traces
                                    false
                    );


    //  Obtain command line parameters.
    //  First parameters is the trace filename. 
    //  Second is the number of GPU frames or cycles to simulate. 
    //  Third is the start frame for simulation.
    
    //  First parameter present?
    if(argc > 1)
    {
        //  First parameter is the trace filename.
        simP.inputFile = new char[strlen(argv[1]) + 1];

        //  Read parameter.
        strcpy(simP.inputFile, argv[1]);
    }

    U64 secondParam;
    
    //  Second parameter present?
    if(argc > 2)
    {
        //  Second parameter is frames to convert from OpenGL to CG1 GPU MetaStream commands

        //  Read second parameter from the arguments.
#ifdef WIN32
            secondParam = _atoi64(argv[2]);
#else
            secondParam = atoll(argv[2]);
#endif

        //  As frames to simulate
        simP.simCycles = 0;
        simP.simFrames = U32(secondParam);
    }

    //  Third parameter present?
    if (argc > 3)
    {
        //  Read start simulation frame.
        simP.startFrame = atoi(argv[3]);
    }

    //  Print loaded parameters.
    printf("Simulator Parameters.\n");

    printf("--------------------\n\n");

    printf("Input File = %s\n", simP.inputFile);
    printf("Simulation Frames = %d\n", simP.simFrames);
    printf("Simulation Start Frame = %d\n", simP.startFrame);


    //  Initialize the optimized dynamic memory system.
    DynamicMemoryOpt::initialize(512,
                                       1024,
                                       1024,
                                       1024,
                                       2048,
                                       1024);
                                       
    //  Create and initialize the Trace Driver.
    if(has_extension(simP.inputFile, "pixrun"))
    {
        TraceDriver = new TraceDriverD3D(simP.inputFile, simP.startFrame);
        cout << "Using Direct3D Trace File as simulation input." << endl;
    }
    else
    {
        TraceDriver = new TraceDriverOGL(simP.inputFile, HAL::getHAL(), simP.ras.shadedSetup, simP.startFrame);
        cout << "Using OpenGL Trace File as simulation input." << endl;
    }

    //  Set frame counter as start frame.
    frameCounter = simP.startFrame;

    if ( simP.startFrame != 0 )
        printf("Note: The first frame producing output (start frame) is frame %d\n", simP.startFrame);

    printf("Converting %d frames.\n\n", simP.simFrames);

    //  Reset batch counter
    batchCounter = 0;

    gzofstream outFile;
    
    //  Open output file
    outFile.open("CG1.MetaStream.tracefile.gz", ios::out | ios::binary);
    
    //  Check if output file was correctly created.
    if (!outFile.is_open())
    {
        CG_ASSERT("Error opening output MetaStream trace file.");
    }
    
    MetaStreamHeader metaTraceHeader;
    
    //  Create header for the MetaStream Trace file.
    
    //  Write file type stamp
    //metaTraceHeader.signature = MetaStreamTRACEFILE_SIGNATURE;
    for(U32 i = 0; i < sizeof(MetaStreamTRACEFILE_SIGNATURE); i++)
        metaTraceHeader.signature[i] = MetaStreamTRACEFILE_SIGNATURE[i];
        
    metaTraceHeader.version = MetaStreamTRACEFILE_VERSION;
    
    metaTraceHeader.parameters.startFrame = simP.startFrame;
    metaTraceHeader.parameters.traceFrames = simP.simFrames;
    metaTraceHeader.parameters.memSize = simP.mem.memSize;
    metaTraceHeader.parameters.mappedMemSize = simP.mem.mappedMemSize;
    metaTraceHeader.parameters.texBlockDim = simP.fsh.texBlockDim;
    metaTraceHeader.parameters.texSuperBlockDim = simP.fsh.texSuperBlockDim;
    metaTraceHeader.parameters.scanWidth = simP.ras.scanWidth;
    metaTraceHeader.parameters.scanHeight = simP.ras.scanHeight;
    metaTraceHeader.parameters.overScanWidth = simP.ras.overScanWidth;
    metaTraceHeader.parameters.overScanHeight = simP.ras.overScanHeight;
    metaTraceHeader.parameters.doubleBuffer = simP.doubleBuffer;
    metaTraceHeader.parameters.fetchRate = simP.fsh.fetchRate;
    metaTraceHeader.parameters.memoryControllerV2 = simP.mem.memoryControllerV2;
    metaTraceHeader.parameters.v2SecondInterleaving = simP.mem.v2SecondInterleaving;

    //  Write header (configuration parameters related with the OpenGL to CG1 MetaStream commands translation).
    outFile.write((char *) &metaTraceHeader, sizeof(metaTraceHeader));    
  
    clock_t startTime = clock();

    bool end = false;
    
    //  Start TraceDriver.
    TraceDriver->startTrace();
    
    while(!end)
    {
        cgoMetaStream *nxtMetaStream;
        
        nxtMetaStream = TraceDriver->nxtMetaStream();
        
        //  Check if the end of the OpenGL trace has been reached.
        if (nxtMetaStream != NULL)
        {
            //  Check transaction type.
            if (nxtMetaStream->GetMetaStreamType() == META_STREAM_COMMAND)
            {
                if (nxtMetaStream->getGPUCommand() == GPU_DRAW)
                    batchCounter++;
                    
                if (nxtMetaStream->getGPUCommand() == GPU_SWAPBUFFERS)
                {
                    frameCounter++;
//#ifdef _DEBUG                    
//                    _CrtDumpMemoryLeaks();                    
//#endif                    
                }
            }
            
            //  Save MetaStream in the output file.
            nxtMetaStream->save(&outFile);
            
            //  Delete MetaStream
            delete nxtMetaStream;
        }
        else
        {
            end = true;
        }
        
        //  Determine end of conversion process.
        if (simP.simFrames != 0)
        {
            //  End if the number of requested frames has been rendered.
            //  If the simulation didn't start with the first frame it must count the
            //  false frame rendered (only swap command) to display the cycle at which
            //  frame skipping was disabled
            end = end || ((frameCounter - simP.startFrame) == (simP.simFrames + ((simP.startFrame == 0)?0:1)));
        }
    }

    F64 elapsedTime = ((F64)(clock() - startTime))/ CLOCKS_PER_SEC;

    cout << "\nSimulation clock time = " << elapsedTime << " seconds" << endl;

    //  Close output file.
    outFile.close();
    
    //  Print end message.
    printf("\n\n");
    printf("End of converion\n");
    printf("\n\n");
    
    return 0;
}








