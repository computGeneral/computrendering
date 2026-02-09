/**************************************************************************
 *
 *  CG1 GPU implementation file 
 *  This file implements the simulator(funcmodel) main loop and support functions 
 *
 */

//  CG1 GPU definitions and declarations.  
#include "CG1SIM.h"

#include "CG1CMDL.h"

#if CG_ARCH_MODEL_DEVEL
#include "CG1AMDL.h"
#endif

#include "param_loader.hpp"
#include "CommandLineReader.h"

#include "TraceDriverMeta.h"
#include "TraceDriverOGL.h"
#ifdef _WIN32
#include "TraceDriverD3D.h"
#endif

#include <ctime>
#include <new>
#include <signal.h>
#include <unistd.h>

#ifdef WIN32
#define PARSE_CYCLES(a) _atoi64(a)
#else
#define PARSE_CYCLES(a) atoll(a)
#endif


using namespace std;
using namespace cg1gpu;

//  Trace reader+driver.
cgoTraceDriverBase *TraceDriver;

cgsArchConfig ArchConf;     //  Simulator parameters.

bool multiClock;          //  Stores if the simulated GPU architecture implements multiple clock domains.
bool shaderClockDomain;   //  Stores if the simulated GPU architecture implements a shader clock domain.
bool memoryClockDomain;   //  Stores if the simulated GPU architecture implements a memory clock domain.
U32  gpuClockPeriod;      //  Period of the GPU clock in picoseconds (ps).
U32  shaderClockPeriod;   //  Period of the shader clock in picoseconds (ps):
U32  memoryClockPeriod;   //  Period of the memory clock in picoseconds (ps):

CG1MDLBASE* CG1GPUSIM = nullptr;

//  Static wrapper for panicCallback to invoke createSnapshot through virtual dispatch.
static void panicSnapshotWrapper() {
    if (CG1GPUSIM) CG1GPUSIM->createSnapshot();
}

// from emulation
// #ifdef WIN32
// int cg1gpu::abortSignalHandler(int s)
// {
//     if (s == CTRL_C_EVENT)
//     {
//         CG_INFO("SignalHandler => CTRL+C received.");
//         CG1GPUSIM->abortSimulation();
//         
//     }
//     return 1;
// }
// 
// #else
// //  Signal handler for abort signal mode
// void cg1gpu::abortSignalHandler(int s)
// {
//     if (s == SIGINT)
//     {
//         CG_INFO("SignalHandler => CTRL+C received.");
//         CG1GPUSIM->abortSimulation();
//     }
// }
// 
// #endif

//  Signal handler for abort signal in debug mode
void cg1gpu::abortSignalHandler(int s)
{
    CG1GPUSIM->abortSimulation();
    signal(SIGINT, &abortSignalHandler);        
}

//bool segFaultAlreadyReceived = false;
//void cg1gpu::segFaultSignalHandler(int s)
//{
//    if (s == SIGSEGV)
//    {
//        if (segFaultAlreadyReceived)
//        {
//            signal(SIGSEGV, SIG_DFL);
//        }
//        else
//        {
//            segFaultAlreadyReceived = true;
//            CG_INFO("");
//            CG_INFO("***! Segmentation Fault detected at frame %d batch %d triangle %d !***",
//                   GpuBehavMdl->getFrameCounter(), GpuBehavMdl->getBatchCounter(), GpuBehavMdl->getTriangleCounter());
//            fflush(stdout);
//            signal(SIGSEGV, SIG_DFL);
//        }
//    }
//}
U32 segFaultAlreadyReceived = 0;
void cg1gpu::segFaultSignalHandler(int s)
{
    if (s == SIGSEGV)
    {
        if (segFaultAlreadyReceived == 2)
        {
            signal(SIGSEGV, SIG_DFL);
        }
        else if (segFaultAlreadyReceived == 1)
        {
            segFaultAlreadyReceived++;
            CG_INFO("");
            CG_INFO("***! Segmentation Fault detected on segmentation fault handler !***");
//            CG_INFO("***! Segmentation Fault detected at frame %d batch %d triangle %d !***",
//                   CG1GPUSIM->getFrameCounter(), CG1GPUSIM->getBatchCounter(), CG1GPUSIM->getTriangleCounter());
            fflush(stdout);
            signal(SIGSEGV, SIG_DFL);
        }
        else
        {
            
            
            U32 frameCounter;
            U32 frameBatch;
            U32 batchCounter;
            
            CG1GPUSIM->getCounters(frameCounter, frameBatch, batchCounter);
            
            U64 gpuCycle;
            U64 shaderCycle;
            U64 memCycle;
            
            CG1GPUSIM->getCycles(gpuCycle, shaderCycle, memCycle);
            
            segFaultAlreadyReceived++;
            CG_INFO("");            
            CG_INFO("***! Segmentation Fault detected !***");            
            CG_INFO(" Frame = %d Batch = %d Total Batches = %d", frameCounter, frameBatch, batchCounter);
            if (multiClock)
            {
                CG_INFO(" GPU Cycle = %lld", gpuCycle);
                if (shaderClockDomain)
                    CG_INFO(" Shader Cycle = %lld", shaderCycle);
                if (memoryClockDomain)
                    CG_INFO(" Memory Cycle = %lld", memCycle);
            }
            else
                CG_INFO(" Cycle = %lld", gpuCycle);
            
            CG1GPUSIM->createSnapshot();
                                    
            CG_INFO("***!        END OF REPORT        !***");
            fflush(stdout);
            exit(-1);
            signal(SIGSEGV, SIG_DFL);
        }
    }  
}

void out_of_memory()
{
    cerr << "CG1GpuSim::out_of_memory -> Memory exhausted. Aborting" << endl;
    exit(-1);
}

bool cg1gpu::MetaTraceSignChecker(MetaStreamHeader *metaTraceHeader)
{
    string str(metaTraceHeader->signature, strlen(MetaStreamTRACEFILE_SIGNATURE));
    bool signatureFound = (str.compare(MetaStreamTRACEFILE_SIGNATURE) == 0);
    return signatureFound;
}

bool cg1gpu::fileExtensionTester(const string file, const string expExtension) {
    size_t namePos = file.find_last_of("/");
    if(namePos == 0xffffffff)
        namePos = file.find_last_of("\\");
    string fileName = file.substr(namePos + 1);
    size_t extPos = fileName.find_first_of(".");
    if(extPos == fileName.npos)
        return false;
    else {
        string realExtension = fileName.substr(extPos + 1);
        string extension_uc = expExtension;
        if (realExtension.length() != expExtension.length())
            return false;
        for(int i=0; i!=realExtension.length(); ++i)
            extension_uc[i] = toupper(expExtension[i]);
        for(int i=0; i!=realExtension.length(); ++i)
            realExtension[i] = toupper(realExtension[i]);
        return realExtension.compare(extension_uc) == 0;
    }
}

bool cg1gpu::MetaTraceParamChecker(MetaStreamHeader *metaTraceHeader)
{
    bool allParamsOK = (ArchConf.mem.memSize == metaTraceHeader->parameters.memSize) &&
                       (ArchConf.mem.mappedMemSize == metaTraceHeader->parameters.mappedMemSize) &&
                       (ArchConf.ush.texBlockDim == metaTraceHeader->parameters.texBlockDim) &&
                       (ArchConf.ush.texSuperBlockDim == metaTraceHeader->parameters.texSuperBlockDim) &&
                       (ArchConf.ras.scanWidth == metaTraceHeader->parameters.scanWidth) &&
                       (ArchConf.ras.scanHeight == metaTraceHeader->parameters.scanHeight) &&
                       (ArchConf.ras.overScanWidth == metaTraceHeader->parameters.overScanWidth) &&
                       (ArchConf.ras.overScanHeight == metaTraceHeader->parameters.overScanHeight) &&
                       (ArchConf.sim.colorDoubleBuffer == metaTraceHeader->parameters.doubleBuffer) &&
                       (ArchConf.ush.fetchRate == metaTraceHeader->parameters.fetchRate) &&
                       (ArchConf.mem.memoryControllerV2 == metaTraceHeader->parameters.memoryControllerV2) &&
                       (ArchConf.mem.v2SecondInterleaving == metaTraceHeader->parameters.v2SecondInterleaving);

    return allParamsOK;
}

//  Main Function.
int main(int argc, char *argv[])
{
    GLOBAL_PROFILER_ENTER_REGION("other", "", "")
    set_new_handler(out_of_memory);

    // Arguments parsing and configuration loading.
    cgeModelAbstractLevel MAL = CG_BEHV_MODEL;

    bool   debugMode = false;
    bool   validMode = false; // validation mode

    int    argIndex = 1;
    int    argPos = 0;
    int    argCount = 0;
    char **argList = new char*[argc];
    const char *paramFile = nullptr;             // path to CG1GPU.csv (optional)
    const char *archName = "CG1GPU.ini";         // ARCH_VERSION column to use

    // First pass: extract --config (ARCH_VERSION column) and --param (CSV path) options.
    while (argIndex < argc) {
        if (strcmp(argv[argIndex], "--config") == 0) {
            // --config selects which ARCH_VERSION column of CG1GPU.csv to use.
            if (++argIndex < argc)
                archName = argv[argIndex];
        }
        else if (strcmp(argv[argIndex], "--param") == 0) {
            // --param overrides the default CSV parameter file path.
            if (++argIndex < argc)
                paramFile = argv[argIndex];
        }
        else
            argList[argCount++] = argv[argIndex++];
    }

    // Load parameters from CSV via ArchParams singleton.
    // Default: search CG1GPU.csv in cwd, then fall back to relative path from build dir.
    if (!paramFile) {
        if (access("CG1GPU.csv", F_OK) == 0)
            paramFile = "CG1GPU.csv";
        else if (access("../../../arch/common/params/CG1GPU.csv", F_OK) == 0)
            paramFile = "../../../arch/common/params/CG1GPU.csv";
        else if (access("../../arch/common/params/CG1GPU.csv", F_OK) == 0)
            paramFile = "../../arch/common/params/CG1GPU.csv";
        else if (access("arch/common/params/CG1GPU.csv", F_OK) == 0)
            paramFile = "arch/common/params/CG1GPU.csv";
        else {
            cerr << "ERROR: CG1GPU.csv not found. Use --param <path> to specify." << endl;
            exit(1);
        }
    }
    ArchParams::init(paramFile, archName);
    ArchParams::instance().populateArchConfig(&ArchConf);

    // Second pass: parse the rest of arguments and override configuration.
    argIndex = 0;
    argPos = 0;

    while (argIndex < argCount) {
        if (strcmp(argList[argIndex], "--fm") == 0)
            MAL = CG_FUNC_MODEL;
        else if (strcmp(argList[argIndex], "--am") == 0)
            MAL = CG_ARCH_MODEL;
        else if (strcmp(argList[argIndex], "--debug") == 0)
            debugMode = true;
        else if (strcmp(argList[argIndex], "--valid") == 0)
            validMode = true;
        else if (strcmp(argList[argIndex], "--start") == 0 && ++argIndex < argCount)
            ArchConf.sim.startFrame = atoi(argList[argIndex]);
        else if (strcmp(argList[argIndex], "--frames") == 0 && ++argIndex < argCount)
            ArchConf.sim.simFrames = atoi(argList[argIndex]);
        else if (strcmp(argList[argIndex], "--cycles") == 0 && ++argIndex < argCount)
            ArchConf.sim.simCycles = PARSE_CYCLES(argList[argIndex]);
        else if (strcmp(argList[argIndex], "--trace") == 0 && ++argIndex < argCount) {
            ArchConf.sim.inputFile = new char[strlen(argList[argIndex]) + 1];
            strcpy(ArchConf.sim.inputFile, argList[argIndex]);
        } else { // traditional arguments style
            switch (argPos) {
                case 0: // trace file
                    ArchConf.sim.inputFile = new char[strlen(argList[argIndex]) + 1];
                    strcpy(ArchConf.sim.inputFile, argList[argIndex]);
                    break;
                case 1: // num frames/cycles
                    if (PARSE_CYCLES(argList[argIndex]) >= 10000) {
                        ArchConf.sim.simFrames = 0;
                        ArchConf.sim.simCycles = PARSE_CYCLES(argList[argIndex]);
                    } else {
                        ArchConf.sim.simFrames = U32(PARSE_CYCLES(argList[argIndex]));
                        ArchConf.sim.simCycles = 0;
                    }
                    break;
                case 2: // start frame
                    ArchConf.sim.startFrame = atoi(argList[argIndex]);
                    break;
                default:
                    CG_INFO("Illegal argument at position %i: %s", argIndex, argList[argIndex]);
                    exit(-1);
            }
            argPos++;
        }
        argIndex++;
    }

    //  Check if the vector alu configuration is scalar (Scalar).
    string aluConf(ArchConf.ush.vectorALUConfig);    
    bool vectorScalarALU = ArchConf.ush.useVectorShader && (aluConf.compare("scalar") == 0);

    //  Compute the number of shader instructions to schedule per cycle that is passed to the driver.
        //  Only the simd4+scalar architecture requires scheduling more than 1 instruction per cycle.
        //  Use the 'FetchRate' (instructions fetched per cycle) parameter for the old shader model.
    U32 instructionsToSchedulePerCycle = (ArchConf.ush.useVectorShader) ? ((aluConf.compare("simd4+scalar") == 0) ? 2 : 1)
                                                                    : ArchConf.ush.fetchRate;

    //  Check configuration parameters.
    CG_ASSERT_COND(!(ArchConf.ush.vAttrLoadFromShader && !ArchConf.sim.enableDriverShaderTranslation),
                  "Vertex attribute load from shader requires driver shader program translation to be enabled.");
    CG_ASSERT_COND(!(ArchConf.ush.useVectorShader && vectorScalarALU && !ArchConf.sim.enableDriverShaderTranslation),
                  "Vector Shader Scalar architecture requires driver shader program translation to be enabled.");
    
    //  Determine clock operation mode.
    shaderClockDomain = (ArchConf.gpu.gpuClock != ArchConf.gpu.shaderClock);
    memoryClockDomain = (ArchConf.gpu.gpuClock != ArchConf.gpu.memoryClock);
    multiClock = shaderClockDomain || memoryClockDomain;
    
    //  Compute clock period for multiple clock domain mode.
    if (multiClock)
    {
        CG_ASSERT_COND(multiClock && (MAL == CG_BEHV_MODEL), "No timing abstraction exist in MAL5");
        gpuClockPeriod    = (U32) (1E6 / (F32) ArchConf.gpu.gpuClock);
        shaderClockPeriod = (U32) (1E6 / (F32) ArchConf.gpu.shaderClock);
        memoryClockPeriod = (U32) (1E6 / (F32) ArchConf.gpu.memoryClock);
    }

    //  Print loaded parameters.
    CG_INFO("CG1 GPU Simulation Parameters.");
    CG_INFO("Input File = %s", ArchConf.sim.inputFile);
    CG_INFO("Signal Trace File = %s", ArchConf.sim.signalDumpFile);
    CG_INFO("Statistics File = %s", ArchConf.sim.statsFile);
    CG_INFO("Statistics (Per Frame) File = %s", ArchConf.sim.statsFilePerFrame);
    CG_INFO("Statistics (Per Batch) File = %s", ArchConf.sim.statsFilePerBatch);
    CG_INFO("Simulation Cycles = %lld", ArchConf.sim.simCycles);
    CG_INFO("Simulation Frames = %d", ArchConf.sim.simFrames);
    CG_INFO("Simulation Start Frame = %d", ArchConf.sim.startFrame);
    CG_INFO("Signal Trace Dump = %s", ArchConf.sim.dumpSignalTrace?"enabled":"disabled");
    CG_INFO("Signal Trace Start Cycle = %lld", ArchConf.sim.startDump);
    CG_INFO("Signal Trace Dump Cycles = %lld", ArchConf.sim.dumpCycles);
    CG_INFO("Statistics Generation = %s", ArchConf.sim.statistics?"enabled":"disabled");
    CG_INFO("Statistics (Per Cycle) Generation = %s", ArchConf.sim.perCycleStatistics?"enabled":"disabled");
    CG_INFO("Statistics (Per Frame) Generation = %s", ArchConf.sim.perFrameStatistics?"enabled":"disabled");
    CG_INFO("Statistics (Per Batch) Generation = %s", ArchConf.sim.perBatchStatistics?"enabled":"disabled");
    CG_INFO("Statistics Rate = %d", ArchConf.sim.statsRate);
    CG_INFO("Dectect Stalls = %s", ArchConf.sim.detectStalls?"enabled":"disabled");
    CG_INFO("EnableDriverShaderTranslation = %s", ArchConf.sim.enableDriverShaderTranslation ? "true" : "false");
    CG_INFO("VertexAttributeLoadFromShader = %s", ArchConf.ush.vAttrLoadFromShader ? "true" : "false");
    CG_INFO("VectorALUConfig = %s", ArchConf.ush.vectorALUConfig);
    CG_INFO_COND(multiClock, "GPU Clock Period = %d ps", gpuClockPeriod);
    CG_INFO_COND((multiClock && shaderClockDomain), "Shader Clock Period = %d ps", shaderClockPeriod);
    CG_INFO_COND((multiClock && memoryClockDomain), "Memory Clock Period = %d ps", memoryClockPeriod);

    // Configure HAL
    HAL::getHAL()->setGPUParameters(ArchConf.mem.memSize * 1024,
                                    ArchConf.mem.mappedMemSize * 1024,
                                    ArchConf.ush.texBlockDim,
                                    ArchConf.ush.texSuperBlockDim,
                                    ArchConf.ras.scanWidth,
                                    ArchConf.ras.scanHeight,
                                    ArchConf.ras.overScanWidth,
                                    ArchConf.ras.overScanHeight,
                                    ArchConf.sim.colorDoubleBuffer,
                                    ArchConf.sim.forceMSAA,
                                    ArchConf.sim.msaaSamples,
                                    ArchConf.sim.forceFP16ColorBuffer,
                                    instructionsToSchedulePerCycle,
                                    ArchConf.mem.memoryControllerV2,
                                    ArchConf.mem.v2SecondInterleaving,
                                    ArchConf.ush.vAttrLoadFromShader,
                                    vectorScalarALU,
                                    ArchConf.sim.enableDriverShaderTranslation,
                                    (ArchConf.ras.useMicroPolRast && ArchConf.ras.microTrisAsFragments)
                    );

    //  Set the shader architecture to use.
    string shaderArch = (ArchConf.ush.fixedLatencyALU) ?
                        (ArchConf.ush.useVectorShader && vectorScalarALU) ? "ScalarFixLat" : "SIMD4FixLat" :
                        (ArchConf.ush.useVectorShader && vectorScalarALU) ? "ScalarVarLat" : "SIMD4VarLat";
    ShaderArchParam::getShaderArchParam()->selectArchitecture(shaderArch);          

    //  Initialize the optimized dynamic memory system.
    DynamicMemoryOpt::initialize(ArchConf.sim.objectSize0, 
                                 ArchConf.sim.bucketSize0, 
                                 ArchConf.sim.objectSize1, 
                                 ArchConf.sim.bucketSize1, 
                                 ArchConf.sim.objectSize2, 
                                 ArchConf.sim.bucketSize2);

    gzifstream ProfilingFile;    //  Create and initialize the Trace Driver.
#ifdef _WIN32
    if (fileExtensionTester(ArchConf.sim.inputFile, "pixrun" ) ||
        fileExtensionTester(ArchConf.sim.inputFile, "pixrunz") ||
        fileExtensionTester(ArchConf.sim.inputFile, "wpix"   ))
    {
        TraceDriver = new TraceDriverD3D(ArchConf.sim.inputFile, ArchConf.sim.startFrame);//  The simulator input is a D3D9 PIXRun trace.  
        cout << "Using Direct3D Trace File as simulation input." << endl;
        cout << "Using CG1 Graphics Abstraction Layer (GAL) Library." << endl;
    }
    else
#endif
    if (fileExtensionTester(ArchConf.sim.inputFile, "ogl.txt.gz") ||
        fileExtensionTester(ArchConf.sim.inputFile, "txt.gz") ||
        fileExtensionTester(ArchConf.sim.inputFile, "txt"))
    {
        cout << "Using OpenGL Trace File as simulation input." << endl;
        cout << "Using CG1 Graphics Abstraction Layer (GAL) Library." << endl;
        //  Initialize a trace for an OpenGL trace file
        TraceDriver = new TraceDriverOGL(ArchConf.sim.inputFile,
                                      HAL::getHAL(),
                                      ArchConf.ras.shadedSetup,
                                      ArchConf.sim.startFrame);//  The simulator input is an GLInterceptor OpenGL trace.  
    }
    else // MetaStream trace file extension: (.metaStream.txt.gz)
    {
        ProfilingFile.open(ArchConf.sim.inputFile, ios::in | ios::binary); //  Check if the input trace file is an MetaStream trace file.
        CG_ASSERT_COND((ProfilingFile.is_open()), "Error opening input trace file."); //  Check if the input file is open
        MetaStreamHeader metaTraceHeader;
        ProfilingFile.read((char *) &metaTraceHeader, sizeof(metaTraceHeader)); //  Read the metaStream trace header.
        CG_ASSERT_COND((MetaTraceSignChecker(&metaTraceHeader)),"MetaStream Trace Signature check failed");
        cout << "Using MetaStream Trace File as simulation input." << endl; //  The simulator input is an MetaStream trace.  
        CG_WARN_COND((!MetaTraceParamChecker(&metaTraceHeader)), "Current parameters and the parameters of the trace file differ!");//  Check parameters of the MetaStream trace file.
        TraceDriver = new TraceDriverMeta(&ProfilingFile, ArchConf.sim.startFrame, metaTraceHeader.parameters.startFrame); //  Initialize a trace driver for an MetaStream trace file.
        ArchConf.sim.startFrame += metaTraceHeader.parameters.startFrame; //  The start frame an offset to the first frame in the MetaStream trace.
        ProfilingFile.close(); //  Close ProfilingFile.
    }
   
    switch(MAL)
    {
        case CG_BEHV_MODEL: 
            CG1GPUSIM = new CG1BMDL(ArchConf, TraceDriver);
            CG_INFO("CG1 MAL5 BMDL Enabled for Simulation");
            break;
        case CG_FUNC_MODEL: 
            CG1GPUSIM = new CG1CMDL(ArchConf, TraceDriver); 
            panicCallback = &panicSnapshotWrapper;    //  Define the call back for the panic function to save a snapshot on simulator errors.
            CG_INFO("CG1 MAL3 CMDL Enabled for Simulation");
            break;
#if CG_ARCH_MODEL_DEVEL
        case CG_ARCH_MODEL:
            sc_core::sc_report_handler::set_log_file_name("./CG_ARCH_MODEL_SC.log");
            sc_core::sc_report_handler::set_actions(SC_INFO, SC_LOG);
            sc_core::sc_report_handler::set_actions(SC_WARNING, SC_LOG);
            //sc_report_handler::set_actions(SC_ID_INSTANCE_EXISTS_, SC_DO_NOTHING);
            sc_core::sc_report_handler::set_actions(SC_ID_INSTANCE_EXISTS_, SC_WARNING, SC_DO_NOTHING);
            CG1GPUSIM = new CG1AMDL(ArchConf, TraceDriver);
            panicCallback = &panicSnapshotWrapper;    //  Define the call back for the panic function to save a snapshot on simulator errors.
            CG_INFO("CG1 MAL1 AMDL Enabled for Simulation");
            break;
#endif
        default: CG_ASSERT("UNDEFINED/UNIMPLEMENTED MAL Detected!");
    }

    CG_WARN_COND((ArchConf.sim.startFrame != 0), "Note: The first frame producing output (start frame) is frame %d", ArchConf.sim.startFrame);
    // from behvmodel
//#ifdef WIN32
//    SetConsoleCtrlHandler(PHANDLER_ROUTINE(abortSignalHandler), true);
//#else
//    signal(SIGINT, &abortSignalHandler);
//#endif

    signal(SIGSEGV, &segFaultSignalHandler);    //  Add a segmentation fault handler to print some information.

    if (debugMode || validMode)    //  Check for debug mode to enable the interactive debug loop.
    {
        signal(SIGINT, &abortSignalHandler);        
        cout << "CG1>> Starting simulator debug mode ---- " << endl << endl;
        CG1GPUSIM->debugLoop(validMode);
        cout << "CG1>> Exiting simulator debug mode ---- " << endl;
    }
    else
    {
        if (ArchConf.sim.simCycles != 0)
            CG_INFO("Simulating %lld cycles (1 dot : 10K cycles).", ArchConf.sim.simCycles);
        else
            CG_INFO("Simulating %d frames (1 dot : 10K cycles).", ArchConf.sim.simFrames);

        time_t startTime = time(NULL);
        if (multiClock) //  Call the simulation main loop.
            CG1GPUSIM->simulationLoopMultiClock();
        else
            CG1GPUSIM->simulationLoop(CG_BEHV_MODEL);
        F64 elapsedTime = time(NULL) - startTime;

        cout << "Simulation clock time = " << elapsedTime << " seconds" << endl;

        if (ProfilingFile.is_open())        //  Close input file
            ProfilingFile.close();
        delete CG1GPUSIM;
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
    GLOBAL_PROFILER_GENERATE_REPORT("profile.txt")

    system("pause");
    return 0;
}
 