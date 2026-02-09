/**************************************************************************
 *
 * GPU simulator implementation file.
 * This file contains the implementation of functions for the CG1 GPU simulator.
 */

#include "CG1CMDL.h"
#include "CommandLineReader.h"
#include "TraceDriverMeta.h"
#include "ColorCompressor.h"
#include "DepthCompressor.h"
#include "cmStatisticsManager.h"
#include "support.h"
#include <ctime>

using namespace std;

//  Static pointer to the active CG1CMDL instance, used by the panic snapshot callback.
static cg1gpu::CG1MDLBASE* s_panicSnapshotInstance = nullptr;
static void panicSnapshotCallback() { if (s_panicSnapshotInstance) s_panicSnapshotInstance->createSnapshot(); }

namespace cg1gpu
{

bool NaN(F32 f)
{
    U32 fr = *((U32 *) &f);
    return ((fr & 0x7F800000) == 0x7F800000) && ((fr & 0x007FFFFF) != 0);
};

CG1CMDL *CG1CMDL::current = NULL;

//  Constructor.
CG1CMDL::CG1CMDL(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver) :
    GpuCMdl(ArchConf, TraceDriver),
    ArchConf(ArchConf),
    TraceDriver(TraceDriver),
    sigBinder(cmoSignalBinder::getBinder()),
    simulationStarted(false)
{
    // Note: ArchParams singleton is already initialized in CG1SIM.cpp main().
    // Sub-modules (cmoGpuTop etc.) still receive cgsArchConfig for backward compat.
    // New code should prefer ArchParams::get<T>("MODULE_Param") over ArchConf.xxx.yyy.

    //  Initializations.
    cyclesCounter = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("CycleCounter", U32(0), "GPU", 0); // Create statistic to count cycles.

    //  Compute clock period for multiple clock domain mode.
    if (GpuCMdl.multiClock)
    {
        gpuClockPeriod    = (U32) (1E6 / (F32) ArchParams::get<uint32_t>("GPU_GPUClock"));
        shaderClockPeriod = (U32) (1E6 / (F32) ArchParams::get<uint32_t>("GPU_ShaderClock"));
        memoryClockPeriod = (U32) (1E6 / (F32) ArchParams::get<uint32_t>("GPU_MemoryClock"));
    }

    //  Check that all the signals are well defined.
    if (!sigBinder.checkSignalBindings())
    {
        sigBinder.dump(); //  Dump content of the signal binder.
        CG_ASSERT("Signals Undefined.");
    }

    //  Check if signal trace dump is enabled.
    if (ArchParams::get<bool>("SIMULATOR_DumpSignalTrace"))
    {
        CG_ASSERT_COND((!GpuCMdl.multiClock), "Signal Trace Dump not supported with multiple clock domains."); //  Signal tracing not supported with multiple clock domains.
        sigTraceFile.open(ArchParams::get<std::string>("SIMULATOR_SignalDumpFile").c_str(), ios::out | ios::binary); //  Try to open the signal trace file.
        CG_ASSERT_COND(sigTraceFile.is_open(), "Error opening signal trace file.");
        sigBinder.initSignalTrace(&sigTraceFile); //  Initialize the signal tracer.  
        CG_INFO("!!! GENERATING SIGNAL TRACE !!!\n");
    }

    //  Check if statistics generation is enabled.
    if (ArchParams::get<bool>("SIMULATOR_Statistics"))
    {
        gpuStatistics::StatisticsManager::instance().setDumpScheduling(0, ArchParams::get<uint32_t>("SIMULATOR_StatisticsRate")); //  Set statistics rate 
        //  Check if per cycle statistics are enabled
        if (ArchParams::get<bool>("SIMULATOR_PerCycleStatistics"))
        {
            outCycle.open(ArchParams::get<std::string>("SIMULATOR_StatsFile").c_str(), ios::out | ios::binary); //  Initialize the statistics manager.
            CG_ASSERT_COND(outCycle.is_open(), "Error opening per cycle statistics file");
            gpuStatistics::StatisticsManager::instance().setOutputStream(outCycle);
        }

        //  Check if per frame statistics are enabled.
        if (ArchParams::get<bool>("SIMULATOR_PerFrameStatistics"))
        {
            outFrame.open(ArchParams::get<std::string>("SIMULATOR_StatsFilePerFrame").c_str(), ios::out | ios::binary);
            CG_ASSERT_COND(outFrame.is_open(), "Error opening per frame statistics file");
            gpuStatistics::StatisticsManager::instance().setPerFrameStream(outFrame);
        }

        //  Check if per batch statistics are enabled.
        if (ArchParams::get<bool>("SIMULATOR_PerBatchStatistics"))
        {
            outBatch.open(ArchParams::get<std::string>("SIMULATOR_StatsFilePerBatch").c_str(), ios::out | ios::binary);
            CG_ASSERT_COND(outBatch.is_open(), "Error opening per batch statistics file");
            gpuStatistics::StatisticsManager::instance().setPerBatchStream(outBatch);
        }
    }

    //  Allocate the pointers for the fragment latency maps.
    U32 numStampPipes = ArchParams::get<uint32_t>("GPU_NumStampPipes");
    latencyMap = new U32*[numStampPipes];
    CG_ASSERT_COND((latencyMap != NULL), "Error allocating latency map array."); //  Check allocation.
    //  Initialize the latency map pointers.

    for(U32 i = 0; i < numStampPipes; i++)
        latencyMap[i] = NULL;

    frameCounter = ArchParams::get<uint32_t>("SIMULATOR_StartFrame"); //  Set frame counter as start frame.
    //  Reset batch counters.
    batchCounter = 0;
    frameBatch = 0;
    //  Auto snapshot variables.
    pendingSaveSnapshot = false;
    autoSnapshotEnable = false;    
    snapshotFrequency = 1;
}

CG1CMDL::~CG1CMDL()
{
    //   Close all output files.
    if (sigTraceFile.is_open())
        sigTraceFile.close();
    if (outCycle.is_open())
        outCycle.close();
    if (outFrame.is_open())
        outFrame.close();
    if (outBatch.is_open())
        outBatch.close();

}

void CG1CMDL::createSnapshot()
{
    // Check if the simulation started.
    if (current->simulationStarted)
    {    
        //  Create a directory for the snapshot files.
        U32 snapshotID = 0;
        bool directoryCreated = false;
        bool tooManySnapshots = false;
        bool error = false;
        char directoryName[100];
        while (!directoryCreated && !error && !tooManySnapshots)
        {
            sprintf(directoryName, "CG1snapshot%02d", snapshotID);
            
            S32 res = createDirectory(directoryName);                
            
            if (res == 0)
                directoryCreated = true;
            else if (res == DIRECTORY_ALREADY_EXISTS)
            {
                if (snapshotID < 99)
                    snapshotID++;
                else
                    tooManySnapshots = true;
            }
            else
                error = true;         
        }
        
        //  Check errors creating the snapshot directory.
        if (error)
            printf(" Error creating snapshot directory: %s\n", directoryName);
        
        if (tooManySnapshots)
            printf(" Too many snapshots already present in the current working directory\n");

        if (directoryCreated)
        {
            char workingDirectory[1024];

            if (getCurrentDirectory(workingDirectory, 1024) != 0)
                CG_ASSERT("Error obtaining current working directory.");
                
            //  Change working directory to the snapshot directory.            
            if (changeDirectory(directoryName) == 0)
            {
                printf(" Saving a simulator snapshot in %s directory\n", directoryName);
                 
                stringstream comstream;

                printf(" Saving simulator state.\n");
                current->saveSimState();
                
                printf(" Saving configuration.\n");
                current->saveSimConfig();
                
                printf(" Saving GPU Registers snapshot.\n");
                comstream.clear();
                comstream.str("_saveregisters");                    
                current->GpuCMdl.CP->execCommand(comstream);
                
                printf(" Saving Hierarchical Z Buffer snapshot.\n");
                current->GpuCMdl.Raster->saveHZBuffer();

                for(U32 i = 0; i < current->ArchConf.gpu.numStampUnits; i++)
                {
                    printf(" Saving Z Stencil Cache %d Block State Memory snapshot\n", i);
                    current->GpuCMdl.zStencilV2[i]->saveBlockStateMemory();
                    printf(" Saving Color Cache %d Block State Memory snapshot\n", i);
                    current->GpuCMdl.colorWriteV2[i]->saveBlockStateMemory();
                }

                printf(" Saving Memory snapshot\n");
                comstream.clear();
                comstream.str("_savememory");
                current->GpuCMdl.MC->execCommand(comstream);


                if (changeDirectory(workingDirectory) != 0)
                    CG_ASSERT("Error changing back to working directory.");
            }
            else
            {
                printf(" Error changing working directory to snapshot directory.\n");
            }
        }        
    }
}

void CG1CMDL::saveSimState()
{
    ofstream out;
    
    out.open("state.snapshot");
    
    if (!out.is_open())
        CG_ASSERT("Error creating file for simulator state snapshot.");
        
    out << "Frame = " << frameCounter << endl;
    out << "Batch = " << batchCounter << endl;
    out << "Frame Batch = " << frameBatch << endl; 

    if (GpuCMdl.multiClock)
    {
        out << "MultiClock" << endl;
        out << "GPU Cycle = " << gpuCycle << endl;
        if (GpuCMdl.shaderClockDomain)
            out << "Shader Cycle = " << shaderCycle << endl;
        if (GpuCMdl.memoryClockDomain)
            out << "Memory Cycle = " << memoryCycle << endl;
    }
    else
        out << "Cycle = " << cycle << endl;

    if (TraceDriver->getTraceTyp() == TraceTypD3d)
    {
        out << "D3D9 PIXRun tracefile" << endl;
        out << "Last processed EID = " << TraceDriver->getTracePosition();
    }
    
    if (TraceDriver->getTraceTyp() == TraceTypOgl)
    {
        out << "GLInterceptor OpenGL tracefile" << endl;
        out << "Last processed line = " << TraceDriver->getTracePosition();
    }    

    if (TraceDriver->getTraceTyp() == TraceTypCgp)
    {    
        out << "MetaStream tracefile" << endl;
        out << "Last processed transaction = " << TraceDriver->getTracePosition();
    }
    
    out.close();    
}

void CG1CMDL::saveSimConfig()
{
    ofstream out;
    
    out.open("config.snapshot");
    
    if (!out.is_open())
        CG_ASSERT("Error creating file for simulator config snapshot.");

    out.write((char *) &ArchConf, sizeof(ArchConf));
    out.close();    
}


// Simulator debug loop.
void CG1CMDL::debugLoop(bool validate)
{
    //  Check if validation mode is enabled.
    if (validate)
    {
        validationMode = true;
        skipValidation = false;
        GpuBehavMdl = new CG1BMDL(ArchConf, TraceDriver); //  Create the GPU behaviorModel.
        GpuBehavMdl->GpuBMdl.resetState(); //  Reset the GPU behaviorModel.
        GpuBehavMdl->GpuBMdl.setValidationMode(true); //  Enable validation mode.
        GpuCMdl.CP->setValidationMode(true); //  Enable the validation mode in the simulator Command Processor.
        GpuCMdl.StreamController->setValidationMode(true); //  Enable the validation mode in the cmoStreamController.
        for(U32 rop = 0; rop < ArchConf.gpu.numStampUnits; rop++) //  Enable validation mode in the Z Stencil Test and Color Write units.
        {
            GpuCMdl.zStencilV2[rop]->setValidationMode(true);
            GpuCMdl.colorWriteV2[rop]->setValidationMode(true);
        }
    }
    
    string line;
    string command;
    stringstream lineStream;
    CommandLineReader *lr = &CommandLineReader::instance();

    endDebug = false;
    simulationStarted = true;

    lr->initialize();

    current = this;
    s_panicSnapshotInstance = this;

    //  Initialize clock state.
    if (GpuCMdl.multiClock)
    {
        nextGPUClock = gpuClockPeriod;
        nextShaderClock = shaderClockPeriod;
        nextMemoryClock = memoryClockPeriod;
        gpuCycle = 0;
        shaderCycle = 0;
        memoryCycle = 0;
    }
    else
    {
        cycle = 0;
    }
    
    while(!endDebug)
    {
        lr->readLine("CG1> ", line);

        lineStream.clear();
        lineStream.str(line);

        command.clear();
        lineStream >> ws;
        lineStream >> command;

        if (!command.compare("quit"))
            endDebug = true;
        else if (!command.compare("run"))
            runCommand(lineStream);
        else if (!command.compare("runbatch"))
            runBatchCommand(lineStream);
        else if (!command.compare("runframe"))
            runFrameCommand(lineStream);
        else if (!command.compare("runcom"))
            runComCommand(lineStream);
        else if (!command.compare("skipbatch"))
            skipBatchCommand(lineStream);
        else if (!command.compare("skipframe"))
            skipFrameCommand(lineStream);
        else if (!command.compare("state"))
            stateCommand(lineStream);
        else if (!command.compare("help"))
            helpCommand(lineStream);
        else if (!command.compare("listboxes"))
            listMdusCommand(lineStream);
        else if (!command.compare("listcommands"))
            listMduCmdCommand(lineStream);
        else if (!command.compare("execcommand"))
            execMduCmdCommand(lineStream);
        else if (!command.compare("debugmode"))
            debugModeCommand(lineStream);
        else if (!command.compare("emulatortrace"))
            emulatorTraceCommand(lineStream);
        else if (!command.compare("tracevertex"))
            traceVertexCommand(lineStream);
        else if (!command.compare("tracefragment"))
            traceFragmentCommand(lineStream);
        else if (!command.compare("memoryUsage"))
            driverMemoryUsageCommand(lineStream);
        else if (!command.compare("listMDs"))
            driverListMDsCommand(lineStream);
        else if (!command.compare("infoMD"))
            driverInfoMDCommand(lineStream);
        else if (!command.compare("dumpMetaStreamBuffer"))
            driverDumpMetaStreamBufferCommand(lineStream);
        else if (!command.compare("memoryAlloc"))
            driverMemAllocCommand(lineStream);
        //else if (!command.compare("glcontext"))
        //    libraryGLContextCommand(lineStream);
        //else if (!command.compare("dumpStencil"))
        //    libraryDumpStencilCommand(lineStream);
        else if (!command.compare("savesnapshot"))
            saveSnapshotCommand();
        else if (!command.compare("loadsnapshot"))
            loadSnapshotCommand(lineStream);
        else if (!command.compare("autosnapshot"))
            autoSnapshotCommand(lineStream);
        else
            cout << "Unsupported command." << endl;
    }

    lr->restore();
}

//  Displays the simulator debug inline help
void CG1CMDL::helpCommand(stringstream &comStream)
{
    cout << "Supported commands: " << endl << endl;

    cout << "help          - Displays information about the simulator debug commands" << endl;
    cout << "listboxes     - List the names of all the simulation boxes" << endl;
    cout << "listcommands  - List the commands available for a mdu" << endl;
    cout << "execcommand   - Executes a mdu command" << endl;
    cout << "debugmode     - Sets the debug mode in a mdu" << endl;
    cout << "emulatortrace - Enables/disables behaviorModel trace logs (validation mode only)" << endl;
    cout << "tracevertex   - Enables/disables tracing the execution of a defined vertex in the shader processors" << endl;
    cout << "tracefragment - Enables/disables tracing the execution of a defined fragment in the shader processors" << endl;
    cout << "run           - Starts the simulation of n cycles" << endl;
    cout << "runframe      - Starts the simulation of n frames" << endl;
    cout << "runbatch      - Starts the simulation of n batches" << endl;
    cout << "runcom        - Starst the simulation of n MetaStream commands" << endl;
    cout << "skipframe     - Skips the simulation of n frames (DRAW commands ignored)" << endl;
    cout << "skipbatch     - Skips the simulation of n batches (DRAW commands ignored)" << endl;
    cout << "state         - Displays the current simulator state" << endl;
    cout << "              - Displays information about the simulator boxes state" << endl;
    cout << "savesnapshot  - Saves a snapshot of the GPU state to disk" << endl;
    cout << "loadsnapshot  - Loads a snapshot of the GPU state from disk" << endl;
    cout << "autosnapshot  - Sets automatic snapshot saves" << endl;
    cout << "memoryUsage   - Displays information about the memory used" << endl;
    cout << "listMDs       - Lists the memory descriptors in the GPU Driver" << endl;
    cout << "infoMD        - Displays the information about a memory descriptor" << endl;
    cout << "dumpMetaStreamBuffer - Dumps the content of the MetaStream Buffer in the GPU Driver" << endl;
    cout << "memoryAlloc   - Dumps the content of the memory allocation structure in the GPU Driver" << endl;
    //cout << "glcontext     - Displays the state of the OpenGL context" << endl;
    //cout << "dumpStencil   - Dumps the stencil buffer as the next frame using a OpenGL library hack" << endl;
    cout << endl;
}

void CG1CMDL::listMdusCommand(stringstream &comStream)
{
    cmoMduBase::dumpMduName();
    cout << endl;
}

//  Simulator debug run command function.  Simulates n cycles
void CG1CMDL::runCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U64 simCycles = 0;

    comStream >> ws;

    //  Check if there is a parameter after the command.
    if (comStream.eof())
    {
        simCycles = 1;
    }
    else
    {
        //  Read the parameter as a 64-bit integer.
        comStream >> simCycles;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
            simCycles = 0;
    }

    //  Check if there cycles to simulate
    if (simCycles == 0)
        cout << "Usage: run <simulation cycles>" << endl;
    else
    {
            cout << "-> Simulating " << simCycles << " cycles" << endl;

        //  Reset abort flag
        abortDebug = false;

        bool end = false;
        U64 firstCycle = GpuCMdl.multiClock ? gpuCycle : cycle;
        
        //  Simulate n cycles
        while(!end && !endOfTrace && !abortDebug)
        {       
            // Clock all the boxes.
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);
            
            //  Check if all the requested cycles were simulated.
            end = ( GpuCMdl.multiClock && (gpuCycle == (firstCycle + simCycles))) ||
                  (!GpuCMdl.multiClock && (cycle == (firstCycle + simCycles)));
            
                
            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        cout << endl;
    }
}

//  Simulator debug runframe command function.  Simulates n frames
void CG1CMDL::runFrameCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U32 simFrames = 0;

    //  Skip white spaces
    comStream >> ws;

    //  Check if there is a parameter for the command.
    if (comStream.eof())
    {
        simFrames = 1;
    }
    else
    {
        //  Read the parameter as a 32-bit integer.
        comStream >> simFrames;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
        {
            simFrames = 0;
        }
    }

    //  Check if there are frames to simulate.
    if (simFrames == 0)
        cout << "Usage: runframe <simulation frames>" << endl;
    else
    {
        cout << "-> Simulating " << simFrames << " frames" << endl;

        //  Reset abort flag
        abortDebug = false;

        bool end = false;
        
        //  Simulate n frames.
        while((simFrames > 0) && (!end) && (!endOfTrace) && (!abortDebug))
        {
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);

            //  Check of end of frame.
            if (endOfFrame)
            {
                //  Update the number of simulated frames.
                simFrames--;
                
                //  Reset end of frame flag.
                endOfFrame = false;
            }

            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        cout << endl;
    }
}


//  Simulator debug runbatch command function.  Simulates n batches
void CG1CMDL::runBatchCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U32 simBatches = 0;

    //  Skip white spaces
    comStream >> ws;

    //  Check if there is a parameter for the command.
    if (comStream.eof())
    {
        simBatches = 1;
    }
    else
    {
        //  Read the parameter as a 32-bit integer.
        comStream >> simBatches;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
        {
            simBatches = 0;
        }
    }

    //  Check if there are frames to simulate.
    if (simBatches == 0)
        cout << "Usage: runbatch <simulation batches>" << endl;
    else
    {
        cout << "-> Simulating " << simBatches << " batches" << endl;

        //  Reset abort flag
        abortDebug = false;

        bool end = false;
        
        //  Simulate n batches.
        while((simBatches > 0) && (!end) && (!endOfTrace) && (!abortDebug))
        {
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);
          
            //  Check for end of batch.
            if (endOfBatch)
            {
                //  Reset the end of batch flag.
                endOfBatch = false;
                
                //  Update the number of batches to simulate.
                simBatches--;
            }

            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        cout << endl;
    }
}

//  Simulator debug runcom command function.  Simulates n MetaStream commands.
void CG1CMDL::runComCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U32 simCommands = 0;

    //  Skip white spaces
    comStream >> ws;

    //  Check if there is a parameter for the command.
    if (comStream.eof())
    {
        simCommands = 1;
    }
    else
    {
        //  Read the parameter as a 32-bit integer.
        comStream >> simCommands;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
        {
            simCommands = 0;
        }
    }

    //  Check if there are frames to simulate.
    if (simCommands == 0)
        cout << "Usage: runcom <simulation MetaStream commands>" << endl;
    else
    {
        cout << "-> Simulating " << simCommands << " MetaStream commands" << endl;

        //  Reset abort flag
        abortDebug = false;
        
        bool end = false;

        //  Simulate n batches.
        while((simCommands > 0) && (!end) && (!endOfTrace) && (!abortDebug))
        {
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);

            //  Check if it's the end of a MetaStream command.
            if (GpuCMdl.CP->endOfCommand())
            {
                //  Update the counter of simulated commands.
                simCommands--;
            }
            
            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        cout << endl;
    }
}


//  Simulator debug skipframe command function.  Skips n frames
void CG1CMDL::skipFrameCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U32 simFrames = 0;

    //  Skip white spaces
    comStream >> ws;

    //  Check if there is a parameter for the command.
    if (comStream.eof())
    {
        simFrames = 1;
    }
    else
    {
        //  Read the parameter as a 32-bit integer.
        comStream >> simFrames;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
        {
            simFrames = 0;
        }
    }

    //  Check if there are frames to simulate.
    if (simFrames == 0)
        cout << "Usage: skipframe <frames to skip>" << endl;
    else
    {
        cout << "-> Skipping " << simFrames << " frames" << endl;

        //  Reset abort flag
        abortDebug = false;

        bool end = false;
        
        //  Set the skip draw command flag in the Command Processor.
        GpuCMdl.CP->setSkipFrames(true);

        //  Simulate n frames.
        while((simFrames > 0) && (!end) && (!endOfTrace) && (!abortDebug))
        {
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);

            //  Check if the current frame has finished.
            if (endOfFrame)
            {
                //  Reset the end of frame flag.
                endOfFrame = false;
                
                //  Update number of frames to skip.
                simFrames--;
            }

            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        //  Unset the skip draw command flag in the Command Processor.
        GpuCMdl.CP->setSkipFrames(false);

        cout << endl;
    }
}

//  Simulator debug skipbatch command function.  Skips the simulation of n batches
void CG1CMDL::skipBatchCommand(stringstream &comStream)
{
    bool endOfBatch = false;
    bool endOfFrame = false;
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    U64 simCycles = 0;
    U32 simFrames = 0;
    U32 simBatches = 0;

    //  Skip white spaces
    comStream >> ws;

    //  Check if there is a parameter for the command.
    if (comStream.eof())
    {
        simBatches = 1;
    }
    else
    {
        //  Read the parameter as a 32-bit integer.
        comStream >> simBatches;

        //  Check if the parameter was correctly read.
        if (comStream.fail())
        {
            simBatches = 0;
        }
    }

    //  Check if there are frames to simulate.
    if (simBatches == 0)
        cout << "Usage: skipbatch <batches to skip>" << endl;
    else
    {
        cout << "-> Skipping " << simBatches << " batches" << endl;

        //  Reset abort flag
        abortDebug = false;
        
        bool end = false;

        //  Set the skip draw command flag in the Command Processor.
        GpuCMdl.CP->setSkipDraw(true);

        //  Simulate n batches.
        while((simBatches > 0) && (!end) && (!endOfTrace) && (!abortDebug))
        {
            advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);

            //  Check if the current batch has finished
            if (endOfBatch)
            {
                //  Reset the end of batch flag.
                endOfBatch = false;
                
                //  Update number of batches to skip.
                simBatches--;
            }

            //  End the simulation if a stall was detected.                  
            end = end || gpuStalled || validationError;
        }

        //  Unset the skip draw command flag in the Command Processor.
        GpuCMdl.CP->setSkipDraw(false);

        cout << endl;
    }
}

//  Simulator debug state command function.  Returns the state of the simulator and
//  the simulator boxes.
void CG1CMDL::stateCommand(stringstream &comStream)
{
    string mduName;
    cmoMduBase *stateBox;
    string stateLine;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: state <boxname> | all" << endl;

        if (!GpuCMdl.multiClock)
            cout << "Simulated cycles = " << cycle;
        else
        {
            cout << "Simulated GPU cycles = " << gpuCycle << " | Simulated Shader cycles = " << shaderCycle;
            cout << " Simulated Memory cycles = " << memoryCycle;
        }
        cout << " | Simulated batches = " << batchCounter;
        cout << " | Simulated frames = " << frameCounter;
        cout << " | Simulated batches in frame = " << frameBatch << endl << endl;
    }
    else
    {
        //  Read the parameter as a string.
        comStream >> mduName;

        if (!GpuCMdl.multiClock)
            cout << "Simulated cycles = " << cycle;
        else
        {
            cout << "Simulated GPU cycles = " << gpuCycle << " | Simulated Shader cycles = " << shaderCycle;
            cout << " Simulated Memory cycles = " << memoryCycle;
        }
        cout << " | Simulated batches = " << batchCounter;
        cout << " | Simulated frames = " << frameCounter;
        cout << " | Simulated batches in frame = " << frameBatch << endl << endl;       

        if (!mduName.compare("all"))
        {
            cout << "all option not implemented yet" << endl;
        }
        else
        {
            //  Try to get the mdu using the provided name.
            stateBox = cmoMduBase::getMdu(mduName.c_str());

            //  Check if the mdu was found.
            if (stateBox != NULL)
            {
                //  Get state line from the mdu.
                stateBox->getState(stateLine);
                cout << "cmoMduBase " << mduName << " state -> " << stateLine << endl;
            }
            else
            {
                cout << "cmoMduBase " << mduName << " not found." << endl;
            }
        }
    }
}

//  Simulator debug mode command function.  Sets the debug mode flag in a mdu.
void CG1CMDL::debugModeCommand(stringstream &comStream)
{
    string mduName;
    string modeS;
    bool modeB;
    cmoMduBase *stateBox;
    string stateLine;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: debugmode <boxname | all> <on | off>" << endl;
    }
    else
    {
        //  Read the parameter as a string.
        comStream >> mduName;

        //  Read the debug mode to set.
        comStream >> modeS;

        //  Check if requested to change all boxes.
        if (!mduName.compare("all"))
        {
            bool error = false;
            
            //  Convert the debug mode string
            if (!modeS.compare("on"))
                modeB = true;
            else if (!modeS.compare("off"))
                modeB = false;
            else
            {
                cout << "Usage: debugmode <boxname | all> <on | off>" << endl;
                error = true;
            }
            
            if (!error)
            {
                for(U32 b = 0; b < GpuCMdl.MduArray.size(); b++)
                    GpuCMdl.MduArray[b]->setDebugMode(modeB);
            }
        }
        else
        {                
            //  Try to get the mdu using the provided name.
            stateBox = cmoMduBase::getMdu(mduName.c_str());

            //  Check if the mdu was found.
            if (stateBox != NULL)
            {
                bool error = false;
                
                //  Convert the debug mode string
                if (!modeS.compare("on"))
                    modeB = true;
                else if (!modeS.compare("off"))
                    modeB = false;
                else
                {
                    error = true;
                    cout << "Usage: debugmode <boxname | all> <on | off>" << endl;
                }
                
                if (!error)
                {
                    //  Get state line from the mdu.
                    stateBox->setDebugMode(modeB);
                    cout << "cmoMduBase " << mduName << " setting debug mode to -> " << modeS << endl;
                }
            }
            else
            {
                cout << "cmoMduBase " << mduName << " not found." << endl;
            }
        }
    }
}

//  Simultor debug mode execute mdu command command.  Executes a mdu command.
void CG1CMDL::execMduCmdCommand(stringstream &comStream)
{
    string mduName;
    cmoMduBase *mdu;
    string commandLine;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: execcommand <boxname>" << endl;
    }
    else
    {
        //  Read the parameter as a string.
        comStream >> mduName;

        //  Try to get the mdu using the provided name.
        mdu = cmoMduBase::getMdu(mduName.c_str());

        //  Check if the mdu was found.
        if (mdu != NULL)
        {
            //  Issue the command to the mdu
            mdu->execCommand(comStream);
        }
        else
        {
            cout << "cmoMduBase " << mduName << " not found." << endl;
        }
    }
}

//  Simulator debug mode list mdu commands command.  Lists the commands available for a given mdu.
void CG1CMDL::listMduCmdCommand(stringstream &comStream)
{
    string mduName;
    string commandList;
    cmoMduBase *mdu;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: list <boxname>" << endl;
    }
    else
    {
        //  Read the parameter as a string.
        comStream >> mduName;

        //  Try to get the mdu using the provided name.
        mdu = cmoMduBase::getMdu(mduName.c_str());

        //  Check if the mdu was found.
        if (mdu != NULL)
        {
            //  Get the list of mdu commands.
            mdu->getCommandList(commandList);
            cout << "cmoMduBase " << mduName << " command list : " << endl << commandList.c_str() << endl;
        }
        else
        {
            cout << "cmoMduBase " << mduName << " not found." << endl;
        }
    }
}

//  Simulator behaviorModel trace command function.  Enable/disable behaviorModel trace logs in validation mode.
void CG1CMDL::emulatorTraceCommand(stringstream &comStream)
{
    string traceMode;
    string traceEnableS;
    bool traceEnable;

    if (!validationMode)
    {
        cout << "Command only available in validation mode" << endl;
    }
    else
    {
        // Skip white spaces.
        comStream >> ws;

        // Check if there is a parameter.
        if (comStream.eof())
        {
            cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
        }
        else
        {
            //  Read the parameter as a string.
            comStream >> traceMode;

            // Skip white spaces.
            comStream >> ws;
            
            if (comStream.eof())
            {
                cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
            }
            else
            {
                //  Read the enable/disable parameter.
                comStream >> traceEnableS;

                bool error = false;
                
                //  Convert the enable/disable parameter string
                if (!traceEnableS.compare("on"))
                    traceEnable = true;
                else if (!traceEnableS.compare("off"))
                    traceEnable = false;
                else
                {
                    error = true;
                    cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
                }
                
                if (!error)
                {
                    //  Check the behaviorModel trace mode to disable/enable.
                    if (!traceMode.compare("all"))
                    {
                        if (traceEnable)
                            cout << "Enabling full behaviorModel trace log." << endl;
                        else
                            cout << "Disabling full behaviorModel trace log." << endl;
                            
                        //  Enable/disable full behaviorModel trace log.
                        GpuBehavMdl->GpuBMdl.setFullTraceLog(traceEnable);
                    }
                    else if (!traceMode.compare("batch"))
                    {
                        U32 batchIndex;
                        
                        comStream >> ws;
                        
                        if (comStream.eof())
                        {
                            cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
                        }
                        else
                        {
                            //  Read the batch index as an unsigned integer.
                            comStream >> batchIndex;
                            
                            if (traceEnable)
                            {
                                cout << "Enabling behaviorModel batch trace log for batch " << batchIndex << "." << endl;
                            }
                            else
                            {
                                cout << "Disabling behaviorModel batch trace log." << endl; 
                            }

                            //  Enable/disable behaviorModel trace log for a defined vertex.
                            GpuBehavMdl->GpuBMdl.setBatchTraceLog(traceEnable, batchIndex);
                        }
                    }
                    else if (!traceMode.compare("vertex"))
                    {
                        U32 vIndex;
                        
                        comStream >> ws;
                        
                        if (comStream.eof())
                        {
                            cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
                        }
                        else
                        {
                            //  Read the vertex index as an unsigned integer.
                            comStream >> vIndex;
                            
                            if (traceEnable)
                            {
                                cout << "Enabling behaviorModel vertex trace log for index " << vIndex << "." << endl;
                            }
                            else
                            {
                                cout << "Disabling behaviorModel vertex trace log." << endl; 
                            }
                            
                            //  Enable/disable behaviorModel trace log for a defined vertex.
                            GpuBehavMdl->GpuBMdl.setVertexTraceLog(traceEnable, vIndex);
                        }
                    }
                    else if (!traceMode.compare("pixel"))
                    {
                        U32 xPos;
                        U32 yPos;
                        
                        comStream >> ws;
                        
                        if (comStream.eof())
                        {
                            cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
                        }
                        else
                        {
                            //  Read the vertex index as an unsigned integer.
                            comStream >> xPos;
                            
                            comStream >> ws;
                            
                            if (comStream.eof())
                            {
                                cout << "Usage: emulatortrace <all | batch | vertex | pixel> <on | off> [<index>] [<x>] [<y]" << endl;
                            }
                            else
                            {
                                comStream >> yPos;
                                
                                if (traceEnable)
                                {
                                    cout << "Enabling behaviorModel pixel trace log for coordinates (" << xPos << ", " << yPos << ")." << endl;
                                }
                                else
                                {
                                    cout << "Disabling behaviorModel pixel trace log." << endl; 
                                }
                                
                                //  Enable/disable behaviorModel trace log for a defined pixel.
                                GpuBehavMdl->GpuBMdl.setPixelTraceLog(traceEnable, xPos, yPos);
                            }
                        }
                    }
                }
            }
        }
    }
}

//  Simulator simulator vertex trace command function.  Enable/disable simulator vertex trace logs in validation mode.
void CG1CMDL::traceVertexCommand(stringstream &comStream)
{
    string enableParam;
    bool traceEnable;

    if (!ArchConf.ush.useVectorShader)
    {
        cout << "Command only available when vector shader is enabled." << endl;
    }
    else
    {
        // Skip white spaces.
        comStream >> ws;

        // Check if there is a parameter.
        if (comStream.eof())
        {
            cout << "Usage: tracevertex <on | off> <index>" << endl;
        }
        else
        {
            //  Read the parameter as a string.
            comStream >> enableParam;

            // Skip white spaces.
            comStream >> ws;
            
            bool error = false;
            
            //  Convert the enable/disable parameter string
            if (!enableParam.compare("on"))
                traceEnable = true;
            else if (!enableParam.compare("off"))
                traceEnable = false;
            else
                error = true;
            
            if (error || comStream.eof())
            {
                cout << "Usage: tracevertex <on | off> <index>" << endl;
            }
            else
            {
                U32 vIndex;
                
                comStream >> ws;
                
                if (comStream.eof())
                {
                    cout << "Usage: tracevertex <on | off> <index>" << endl;
                }
                else
                {
                    //  Read the vertex index as an unsigned integer.
                    comStream >> vIndex;
                    
                    if (traceEnable)
                    {
                        cout << "Enabling vertex trace in the shader processors for index " << vIndex << "." << endl;
                    }
                    else
                    {
                        cout << "Disabling vertex trace in the shader processors." << endl; 
                    }
                    
                    for(U32 u = 0; u < ArchConf.gpu.numFShaders; u++)
                    {
                        stringstream commStr;
                        
                        commStr << "tracevertex " << enableParam << " " << vIndex;
                                                
                        GpuCMdl.cmUnifiedShader[u]->vecShFetch->execCommand(commStr);
                    }
                }
            }
        }
    }
}

//  Simulator simulator fragment trace command function.  Enable/disable simulator fragment trace logs in validation mode.
void CG1CMDL::traceFragmentCommand(stringstream &comStream)
{
    string enableParam;
    bool traceEnable;

    if (!ArchConf.ush.useVectorShader)
    {
        cout << "Command only available when vector shader is enabled." << endl;
    }
    else
    {
        // Skip white spaces.
        comStream >> ws;

        // Check if there is a parameter.
        if (comStream.eof())
        {
            cout << "Usage: tracefragment <on | off> <x> <y>" << endl;
        }
        else
        {
            //  Read the parameter as a string.
            comStream >> enableParam;

            // Skip white spaces.
            comStream >> ws;
            
            bool error = false;
            
            //  Convert the enable/disable parameter string.
            if (!enableParam.compare("on"))
                traceEnable = true;
            else if (!enableParam.compare("off"))
                traceEnable = false;
            else
                error = true;
            
            if (error || comStream.eof())
            {
                cout << "Usage: tracefragment <on | off> <x> <y>" << endl;
            }
            else
            {
                U32 fragX;
                U32 fragY;
                
                comStream >> ws;
                
                if (comStream.eof())
                {
                    cout << "Usage: tracefragment <on | off> <x> <y>" << endl;
                }
                else
                {
                    //  Read the fragment x coordinate index as an unsigned integer.
                    comStream >> fragX;

                    comStream >> ws;
                                        
                    if (comStream.eof())
                    {                  
                        cout << "Usage: tracefragment <on | off> <x> <y>" << endl;
                    }
                    else
                    {
                        comStream >> fragY;
                        
                        if (traceEnable)
                        {
                            cout << "Enabling fragment trace in the shader processors at coordinate (" << fragX << ", " << fragY << ")." << endl;
                        }
                        else
                        {
                            cout << "Disabling fragment trace in the shader processors." << endl; 
                        }
                        
                        for(U32 u = 0; u < ArchConf.gpu.numFShaders; u++)
                        {
                            stringstream commStr;
                            
                            commStr << "tracefragment " << enableParam << " " << fragX << " " << fragY;
                                                    
                            GpuCMdl.cmUnifiedShader[u]->vecShFetch->execCommand(commStr);
                        }
                        
                        for(U32 u = 0; u < ArchConf.gpu.numStampUnits; u++)
                        {
                            stringstream commStr;
                            
                            commStr << "tracefragment " << enableParam << " " << fragX << " " << fragY;
                            GpuCMdl.zStencilV2[u]->execCommand(commStr);
                            
                            commStr.clear();
                            commStr << "tracefragment " << enableParam << " " << fragX << " " << fragY;
                            GpuCMdl.colorWriteV2[u]->execCommand(commStr);
                        }
                        
                    }
                }
            }
        }
    }
}

void CG1CMDL::driverMemoryUsageCommand(stringstream &comStream)
{
    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (!comStream.eof())
    {
        cout << "Usage: memoryUsage" << endl;
    }
    else
    {
        cout << "GPU Driver => Memory Usage : " << endl;
        HAL::getHAL()->printMemoryUsage();
        cout << endl;
    }
}

void CG1CMDL::driverListMDsCommand(stringstream &comStream)
{
    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (!comStream.eof())
    {
        cout << "Usage: listMDs" << endl;
    }
    else
    {
        cout << "GPU Driver => Memory Descriptors List: " << endl;
        HAL::getHAL()->dumpMDs();
        cout << endl;
    }
}

void CG1CMDL::driverInfoMDCommand(stringstream &comStream)
{
    U32 md;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: infoMD <md identifier>" << endl;
    }
    else
    {
        comStream >> md;

        comStream >> ws;

        if (!comStream.eof())
        {
            cout << "Usage: infoMD <md identifier>" << endl;
        }
        else
        {
            cout << "GPU Driver => Info Memory Descriptor " << md << endl;
            HAL::getHAL()->dumpMD(md);
            cout << endl;
        }
    }
}

void CG1CMDL::driverDumpMetaStreamBufferCommand(stringstream &comStream)
{
    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (!comStream.eof())
    {
        cout << "Usage: dumpMetaStreamBuffer" << endl;
    }
    else
    {
        cout << "GPU Driver => MetaStream Buffer Content: " << endl;
        HAL::getHAL()->dumpMetaStreamBuffer();
        cout << endl;
    }
}

void CG1CMDL::driverMemAllocCommand(stringstream &comStream)
{
    string boolParam;
    bool dumpContent;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "GPU Driver => Memory Allocation (simple) " << endl;
        HAL::getHAL()->dumpMemoryAllocation(false);
        cout << endl;
    }
    else
    {
        comStream >> boolParam;

        if (!boolParam.compare("true"))
            dumpContent = true;
        else if (!boolParam.compare("false"))
            dumpContent = false;
        else
        {
            cout << "Usage: memoryAlloc [true|false]" << endl;
            return;
        }

        comStream >> ws;

        if (!comStream.eof())
        {
            cout << "Usage: memoryAlloc [true|false]" << endl;
        }
        else
        {
            cout << "GPU Driver => Memory Allocation " << (dumpContent?"(full)":"(simple)") << endl;
            HAL::getHAL()->dumpMemoryAllocation(dumpContent);
            cout << endl;
        }
    }
}

/*
  DEPRECATED
  
void CG1CMDL::libraryGLContextCommand(stringstream &comStream)
{
    libgl::GLContext *ctx;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (!comStream.eof())
    {
        cout << "Usage: glcontext" << endl;
    }
    else
    {
        cout << "GPU Driver => MetaStream Buffer Content: " << endl;
        ctx = (libgl::GLContext *) HAL::getHAL()->getContext();
        ctx -> dump();
        cout << endl;
    }
}

void CG1CMDL::libraryDumpStencilCommand(stringstream &comStream)
{
    libgl::GLContext *ctx;

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (!comStream.eof())
    {
        cout << "Usage: dumpStencil" << endl;
    }
    else
    {
        cout << "GPU Driver => Dumping stencil buffer " << endl;
        ctx = (libgl::GLContext *) HAL::getHAL()->getContext();
        libgl::afl::dumpStencilBuffer(ctx);
    }
}*/

//  Simulate until a force command finishes.
bool CG1CMDL::simForcedCommand()
{
    bool endOfBatch = false;
    bool endOfFrame = false;    
    bool endOfTrace = false;
    bool gpuStalled = false;
    bool validationError = false;
    bool endOfCommand = false;

    //  Simulate until the end of the initialization phase.
    while(!endOfCommand && !endOfTrace && !gpuStalled && !validationError)
    {
        advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);

        //  Check if the forced command has finished.
        endOfCommand = GpuCMdl.CP->endOfForcedCommand();
    }

    return !endOfTrace;
}

void CG1CMDL::saveSnapshotCommand()
{
    pendingSaveSnapshot = true;
    
    //  Create a directory for the snapshot files.
    U32 snapshotID = 0;
    bool directoryCreated = false;
    bool tooManySnapshots = false;
    bool error = false;
    char directoryName[100];
    while (!directoryCreated && !error && !tooManySnapshots)
    {
        sprintf(directoryName, "CG1snapshot%02d", snapshotID);
        
        S32 res = createDirectory(directoryName);                
        
        if (res == 0)
            directoryCreated = true;
        else if (res == DIRECTORY_ALREADY_EXISTS)
        {
            if (snapshotID < 99)
                snapshotID++;
            else
                tooManySnapshots = true;
        }
        else
            error = true;         
    }
    
    //  Check errors creating the snapshot directory.
    if (error)
    {
        printf(" ERROR: Coul not create snapshot directory: %s\n", directoryName);
        return;
    }
    
    if (tooManySnapshots)
    {
        printf(" ERROR: Too many snapshots already present in the current working directory\n");
        return;
    }
    
    //  Disable create snapshot on call back.
    panicCallback = NULL;
    
    if (directoryCreated)
    {
        char workingDirectory[1024];

        if (getCurrentDirectory(workingDirectory, 1024) != 0)
            CG_ASSERT("Error obtaining current working directory.");

        //  Change working directory to the snapshot directory.            
        if (changeDirectory(directoryName) == 0)
        {

            stringstream commandStream;
            
            printf(" Saving a simulator snapshot in %s directory\n", directoryName);
             
            //  Flush color caches.
            commandStream.clear();
            commandStream.str("flushcolor");
            GpuCMdl.CP->execCommand(commandStream);

            if (!simForcedCommand())
                return;

            //  Flush z and stencil caches.
            commandStream.clear();
            commandStream.str("flushzst");
            GpuCMdl.CP->execCommand(commandStream);

            //  Check for end of trace.
            if (!simForcedCommand())
                return;
                
            saveSimState();
            saveSimConfig();
            
            commandStream.clear();
            commandStream.str("_saveregisters");                    
            GpuCMdl.CP->execCommand(commandStream);
            
            TraceDriverMeta *metaTraceDriver = dynamic_cast<TraceDriverMeta*>(TraceDriver);
            
            //  Dump MetaStream Trace Driver state.
            if (TraceDriver->getTraceTyp() == TraceTypCgp)
            {
                fstream out;

                out.open("MetaStream.snapshot", ios::binary | ios::out);

                if (!out.is_open())
                    CG_ASSERT("Error creating MetaStream Trace Driver snapshot file.");

                metaTraceDriver->saveTracePosition(&out);

                out.close();
            }

            GpuCMdl.Raster->saveHZBuffer();

            for(U32 i = 0; i < ArchConf.gpu.numStampUnits; i++)
            {
                GpuCMdl.zStencilV2[i]->saveBlockStateMemory();
                GpuCMdl.colorWriteV2[i]->saveBlockStateMemory();
            }

            commandStream.clear();
            commandStream.str("_savememory");
            GpuCMdl.MC->execCommand(commandStream);
        
            if (validationMode)
                GpuBehavMdl->saveSnapshot();
            
            if (changeDirectory(workingDirectory) != 0)
                CG_ASSERT("Error changing back to working directory.");
        }
        else
        {
            printf(" ERROR: Could not access working directory to snapshot directory.\n");
        }
    }

    //  Renable create snapshot on panic.
    panicCallback = &panicSnapshotCallback;

    pendingSaveSnapshot = false;    
}

void CG1CMDL::loadSnapshotCommand(stringstream &comStream)
{
    U32 snapshotID;

    //  Check if a snapshot can be loaded.
    if ((!GpuCMdl.multiClock && (cycle != 0)) || (GpuCMdl.multiClock && ((gpuCycle != 0) || (shaderCycle != 0) || (memoryCycle != 0))))
    {
        cout << "ERROR : loadsnapshot can only be used at cycle 0." << endl;
        return;
    }

    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    if (comStream.eof())
    {
        cout << "Usage: loadsnapshot <snapshot identifier>" << endl;
    }
    else
    {
        comStream >> snapshotID;
        comStream >> ws;

        if (!comStream.eof())
        {
            cout << "Usage: loadsnapshot <snapshot identifier>" << endl;
        }
        else
        {
            //  Disable save snapshot on panic.
            panicCallback = NULL;
            
            //  Change to the snapshot directory.
            char directoryName[100];
            sprintf(directoryName, "CG1snapshot%02d", snapshotID);
            
            char workingDirectory[1024];
            char snapshotDirectory[1024];

            if (getCurrentDirectory(workingDirectory, 1024) != 0)
                CG_ASSERT("Error obtaining current working directory.");
            
            printf(" Current working directory path: %s\n", workingDirectory);
            printf(" Loading a simulator snapshot from %s directory\n", directoryName);

            //  Change working directory to the snapshot directory.            
            if (changeDirectory(directoryName) == 0)
            {

                if (getCurrentDirectory(snapshotDirectory, 1024) != 0)
                    CG_ASSERT("Error obtaining snapshot directory.");
                    
                printf(" Snapshot directory path: %s\n", snapshotDirectory);

                //  Check the type of tracefile being used.
                if (TraceDriver->getTraceTyp() == TraceTypCgp)
                {
                    //  Load MetaStream trace snapshot file
                    fstream in;

                    in.open("MetaStream.snapshot", ios::binary | ios::in);

                    if (!in.is_open())
                        CG_ASSERT("Error opening MetaStream Trace Driver snapshot file.");

                    TraceDriverMeta *metaTraceDriver = dynamic_cast<TraceDriverMeta*>(TraceDriver);

                    //  Load the MetaStream trace start position information from the snapshot file.
                    metaTraceDriver->loadStartTracePosition(&in);

                    in.close();

                    bool endOfBatch = false;
                    bool endOfFrame = false;
                    bool endOfTrace = false;
                    bool gpuStalled = false;
                    bool validationError = false;

                    if (changeDirectory(workingDirectory) != 0)
                        CG_ASSERT("Error changing back to working directory.");
                    
                    //  Simulate until the end of the initialization phase.
                    while(!GpuCMdl.CP->endOfInitialization() && !endOfTrace)
                    {
                        advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);
                    }

                    if (changeDirectory(directoryName) != 0)
                        CG_ASSERT("Error changing to snapshot directory.");

                }
                
                if (TraceDriver->getTraceTyp() == TraceTypOgl || TraceDriver->getTraceTyp() == TraceTypD3d)
                {
                    //  Read the frame and batches to skip from the state snapshot file.
                    ifstream in;
                    
                    in.open("state.snapshot");
                    
                    U32 savedFrame;
                    U32 savedBatch;
                    U32 savedFrameBatch;
                    
                    in.ignore(8);       //  Skip "Frame = ".
                    in >> savedFrame;   //  Read the saved frame.
                    in.ignore();        //  Skip newline.
                    in.ignore(8);       //  Skip "Batch = ".
                    in >> savedBatch;   //  Read the saved batch.
                    in.ignore();        //  Skip newline.
                    in.ignore(14);      //  Skip "Frame Batch = "
                    in >> savedFrameBatch;  //  Read the saved frame batch.
                    in.close(); 
                                                            
                    bool endOfBatch = false;
                    bool endOfFrame = false;
                    bool endOfTrace = false;
                    bool gpuStalled = false;
                    bool validationError = false;

                    cout << " Skipping up to frame " << savedFrame << " batch " << savedBatch << " frame batch " << savedFrameBatch << endl;
                    
                    //  Skip rendering until reaching the saved frame and batch.
                    GpuCMdl.CP->setSkipFrames(true);
                    
                    if (validationMode)
                        GpuBehavMdl->GpuBMdl.setSkipBatch(true);
                    
                    if (changeDirectory(workingDirectory) != 0)
                        CG_ASSERT("Error changing back to working directory.");

                    //  Skip validation until reaching the point where the snapshot was captured.
                    skipValidation = true;

                    //  Simulate until the end of the initialization phase.
                    while(((frameCounter != savedFrame) || ((batchCounter != savedBatch)) && (frameBatch != savedFrameBatch)) && !endOfTrace)
                    {
                        advanceTime(endOfBatch, endOfFrame, endOfTrace, gpuStalled, validationError);
                    }
                    
                    //  Reenable validation.
                    skipValidation = false;
                    
                    //  Restart rendering.
                    GpuCMdl.CP->setSkipFrames(false);

                    if (validationMode)
                        GpuBehavMdl->GpuBMdl.setSkipBatch(false);

                    if (changeDirectory(directoryName) != 0)
                        CG_ASSERT("Error changing to snapshot directory.");
                }            

                cout << " Loading state from snapshot" << endl;
                
                //  Load the Hierarchical Z Buffer content from the snapshot file.
                GpuCMdl.Raster->loadHZBuffer();

                //  Load block state memory from the snapshot files.
                for(U32 i = 0; i < ArchConf.gpu.numStampUnits; i++)
                {
                    GpuCMdl.zStencilV2[i]->loadBlockStateMemory();
                    GpuCMdl.colorWriteV2[i]->loadBlockStateMemory();
                }

                stringstream commandStream;
                
                //  Load memory from the snapshot file.
                commandStream.clear();
                commandStream.str("_loadmemory");
                GpuCMdl.MC->execCommand(commandStream);

                if (validationMode)
                {
                    //  Load behaviorModel snapshot.
                    GpuBehavMdl->loadSnapshot();
                }
                                
                if (changeDirectory(workingDirectory) != 0)
                    CG_ASSERT("Error changing back to working directory.");
            }            
            else
            {
                printf(" ERROR: Could not access working directory to snapshot directory.\n");
            }

            //  Re-enable save snapshot on panic.
            panicCallback = &panicSnapshotCallback;
        }
    }
}

void CG1CMDL::autoSnapshotCommand(stringstream &comStream)
{
    bool errorInParsing = false;
    
    bool enableParam;
    U32 freqParam;
    
    // Skip white spaces.
    comStream >> ws;

    // Check if there is a parameter.
    errorInParsing = comStream.eof();

    if (!errorInParsing)
    {
        string paramStr;
        
        comStream >> paramStr;

        if (!paramStr.compare("on"))
            enableParam = true;
        else if (!paramStr.compare("off"))
            enableParam = false;
        else
            errorInParsing = true;

        if (!errorInParsing)
        {
            comStream >> ws;
        
            errorInParsing = comStream.eof();
            
            if (!errorInParsing)
            {
                comStream >> freqParam;
                
                comStream >> ws;
                
                errorInParsing = !comStream.eof();
                
                if (!errorInParsing)
                {
                    if (freqParam == 0)
                    {
                        cout << "Auto snapshot frequency must be at leat 1 minute" << endl;
                        errorInParsing = true;
                    }
                    
                    if (!errorInParsing)
                    {
                        if (enableParam)
                            cout << "Setting auto snapshot with a frequency of " << freqParam << " minutes" << endl;
                        else
                            cout << "Disabling auto snapshot." << endl;
                            
                        autoSnapshotEnable = enableParam;
                        snapshotFrequency = freqParam;
                        
                        startTime = time(NULL);
                    }
                }
            }
        }
    }
    
    if (errorInParsing)
    {
        cout << "Usage: autosnapshot <on|off> <frequency in minutes>" << endl;
    }
}

void CG1CMDL::advanceTime(bool &endOfBatch, bool &endOfFrame, bool &endOfTrace, bool &gpuStalled, bool &validationError)
{
    current = this;

    bool checkStalls;
    
    if (!GpuCMdl.multiClock)
    {
        //  Increment the statistics cycle counter.
        cyclesCounter->inc();
        
        //  Issue a clock for all the simulation boxes.
        for(U32 mdu = 0; mdu < GpuCMdl.MduArray.size(); mdu++)
            GpuCMdl.MduArray[mdu]->clock(cycle);
        
        //  Update the simulator cycle counter.
        cycle++;
        
        //  Evaluate if stall detection must be performed.
        checkStalls = ArchConf.sim.detectStalls && ((cycle % 100000) == 99999);
    }
    else
    {
        bool end = false;
        
        //  Run at least one cycle of the main (GPU) clock.
        while(!end)
        {
            //  Determine the picoseconds to the next clock.
            U32 nextStep = GPU_MIN(GPU_MIN(nextGPUClock, nextShaderClock), nextMemoryClock);
            
            //printf("Clock -> Next GPU Clock = %d | Next Shader Clock = %d | Next Memory Clock = %d| Next Step = %d | GPU Cycle = %lld | Shader Cycle = %lld | Memory Cycle = %lld\n",
            //    nextGPUClock, nextShaderClock, nextStep, gpuCycle, shaderCycle, memoryCycle);
                            
            //  Update step counters for both clock domains.
            nextGPUClock -= nextStep;
            nextShaderClock -= nextStep;
            nextMemoryClock -= nextStep;
            
            //  Check if a gpu domain tick has to be issued.
            if (nextGPUClock == 0)
            {
                GPU_DEBUG(
                    printf("GPU Domain. Cycle %lld ----------------------------\n", gpuCycle);
                )
                
                // Clock all the boxes in the GPU Domain.
                for(U32 mdu = 0; mdu < GpuCMdl.GpuDomainMduArray.size(); mdu++)
                    GpuCMdl.GpuDomainMduArray[mdu]->clock(gpuCycle);
                    
                //  Clock boxes with multiple domains.
                for(U32 mdu = 0; mdu < GpuCMdl.ShaderDomainMduArray.size(); mdu++)
                    GpuCMdl.ShaderDomainMduArray[mdu]->clock(GPU_CLOCK_DOMAIN, gpuCycle);
                    
                for(U32 mdu = 0; mdu < GpuCMdl.MemoryDomainMduArray.size(); mdu++)
                    GpuCMdl.MemoryDomainMduArray[mdu]->clock(GPU_CLOCK_DOMAIN, gpuCycle);

                //  Update GPU domain clock state.
                gpuCycle++;
                nextGPUClock = gpuClockPeriod;
                
                //  One cycle of the GPU clock was simulated.
                end = true;
            }
            
            //  Check if a shader domain tick must be issued.
            if (nextShaderClock == 0)
            {
                if (GpuCMdl.shaderClockDomain)
                {
                    GPU_DEBUG(
                        printf("Shader Domain. Cycle %lld ----------------------------\n", shaderCycle);
                    )

                    //  Clock boxes with multiple domains.
                    for(U32 mdu = 0; mdu < GpuCMdl.ShaderDomainMduArray.size(); mdu++)
                        GpuCMdl.ShaderDomainMduArray[mdu]->clock(SHADER_CLOCK_DOMAIN, shaderCycle);

                    //  Update shader domain clock and step counter.
                    shaderCycle++;
                }
                
                nextShaderClock = shaderClockPeriod;
            }

            //  Check if a memory domain tick must be issued.
            if (nextMemoryClock == 0)
            {
                if (GpuCMdl.memoryClockDomain)
                {
                    GPU_DEBUG(
                        printf("Memory Domain. Cycle %lld ----------------------------\n", memoryCycle);
                    )

                    //  Clock boxes with multiple domains.
                    for(U32 mdu = 0; mdu < GpuCMdl.MemoryDomainMduArray.size(); mdu++)
                        GpuCMdl.MemoryDomainMduArray[mdu]->clock(MEMORY_CLOCK_DOMAIN, memoryCycle);

                    //  Update memory domain clock and step counter.
                    memoryCycle++;
                }
                
                nextMemoryClock = memoryClockPeriod;
            }
        }

        //  Evaluate if stall detection must be performed.
        checkStalls = ArchConf.sim.detectStalls && ((gpuCycle % 100000) == 99999);
    }

    //  Check if the current batch has finished
    if (GpuCMdl.CP->endOfBatch())
    {
        //  Update rendered batches counter.
        batchCounter++;
        
        //  Update rendered batches in the current frame.
        frameBatch++;
        
        //  Set the end batch flag.
        endOfBatch = true;
        
        //  Check if validation mode is enabled.
        if (validationMode)
        {
            //  Get the log of MetaStreams processed by the Command Processor.
            vector<cgoMetaStream *> &metaStreamLog = GpuCMdl.CP->getcgoMetaStreamLog();
            
            //  Issue the MetaStreams to the behaviorModel.
            for(U32 trans = 0; trans < metaStreamLog.size(); trans++)
            {
                GpuBehavMdl->GpuBMdl.emulateCommandProcessor(metaStreamLog[trans]);
            }
            
            //  Clear the MetaStream log.
            metaStreamLog.clear();
                       
            //  Check if validation must be performed.
            if (!skipValidation)
            {
                VertexInputMap simVertexInputLog;
                
                //  Get the read vertex log from the simulator.            
                for(U32 u = 0; u < ArchConf.str.streamerLoaderUnits; u++)
                {
                    //  Get the read vertex log from one of the cmoStreamController Loader units.
                    VertexInputMap &simVertexInputStLLog = GpuCMdl.StreamController->getVertexInputInfo(u);
                    
                    //  Rebuild as a single map.
                    VertexInputMap::iterator itStL;
                    for(itStL = simVertexInputStLLog.begin(); itStL != simVertexInputStLLog.end(); itStL++)
                    {
                        //  Search in the rebuild map.
                        VertexInputMap::iterator itGlobal;
                        itGlobal = simVertexInputLog.find(itStL->second.vertexID);
                        
                        if (itGlobal != simVertexInputLog.end())
                        {
                            //  Check and update.
                            itGlobal->second.timesRead += itStL->second.timesRead;
                            
                            //  Compare the two versions.
                            bool attributesAreDifferent = false;
                            
                            for(U32 a = 0; (a < MAX_VERTEX_ATTRIBUTES) && !attributesAreDifferent; a++)
                            {
                                //  Evaluate if the attribute values are different.
                                attributesAreDifferent = (itGlobal->second.attributes[a][0] != itGlobal->second.attributes[a][0]) ||
                                                         (itGlobal->second.attributes[a][1] != itGlobal->second.attributes[a][1]) ||
                                                         (itGlobal->second.attributes[a][2] != itGlobal->second.attributes[a][2]) ||
                                                         (itGlobal->second.attributes[a][3] != itGlobal->second.attributes[a][3]);
                            }
                            
                            itGlobal->second.differencesBetweenReads = itGlobal->second.differencesBetweenReads || 
                                                                       itStL->second.differencesBetweenReads ||
                                                                       attributesAreDifferent;
                        }
                        else
                        {
                            //  Add a new read vertex to the rebuild map.
                            simVertexInputLog.insert(make_pair(itStL->second.vertexID, itStL->second));                        
                        }
                    }
                    
                    //  Clear the map from the cmoStreamController Loader unit.
                    simVertexInputStLLog.clear();
                }
                
                //  Get the read vertex log from the behaviorModel.
                VertexInputMap &emuVertexInputLog = GpuBehavMdl->GpuBMdl.getVertexInputLog();

//printf("Validation >> Simulator Read Vertices = %d | Behavior Model Read Vertices = %d\n", simVertexInputLog.size(),
//emuVertexInputLog.size());

                //  Compare the two lists.
                if (simVertexInputLog.size() != emuVertexInputLog.size())
                {
                    printf("Validation => Number of read vertices is different: behaviorModel = %d simulator = %d\n",
                        emuVertexInputLog.size(), simVertexInputLog.size());
                        
                    validationError = true;               
                    
/*VertexInputMap::iterator itVertexInput;
printf("Validation => Behavior Model read vertex list\n");
for(itVertexInput = emuVertexInputLog.begin(); itVertexInput != emuVertexInputLog.end(); itVertexInput++)
printf("Validation => Index %d Instance %d\n", itVertexInput->second.vertexID.index, itVertexInput->second.vertexID.instance);
printf("Validation => Simulator read vertex list\n");
for(itVertexInput = simVertexInputLog.begin(); itVertexInput != simVertexInputLog.end(); itVertexInput++)
printf("Validation => Index %d Instance %d TimesRead %d Diffs Between Reads %s\n",
itVertexInput->second.vertexID.index, itVertexInput->second.vertexID.instance,
itVertexInput->second.timesRead, itVertexInput->second.differencesBetweenReads ? "T" : "F");*/
                }
                
                VertexInputMap::iterator itVertexInputEmu;
                
                for(itVertexInputEmu = emuVertexInputLog.begin(); itVertexInputEmu != emuVertexInputLog.end(); itVertexInputEmu++)
                {
                    //  Find index in the simulator log.
                    VertexInputMap::iterator itVertexInputSim;
                    
                    itVertexInputSim = simVertexInputLog.find(itVertexInputEmu->second.vertexID);
                    
                    //  Check if the index was found in the simulator log.
                    if (itVertexInputSim != simVertexInputLog.end())
                    {
                        //  Compare all the attributes.
                        for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                        {
                            bool attributeIsEqual =                             
                                ((itVertexInputEmu->second.attributes[a][0] == itVertexInputSim->second.attributes[a][0]) ||
                                 (NaN(itVertexInputEmu->second.attributes[a][0]) && NaN(itVertexInputSim->second.attributes[a][0]))) &&
                                ((itVertexInputEmu->second.attributes[a][1] == itVertexInputSim->second.attributes[a][1]) ||
                                 (NaN(itVertexInputEmu->second.attributes[a][1]) && NaN(itVertexInputSim->second.attributes[a][1]))) &&
                                ((itVertexInputEmu->second.attributes[a][2] == itVertexInputSim->second.attributes[a][2]) ||
                                 (NaN(itVertexInputEmu->second.attributes[a][2]) && NaN(itVertexInputSim->second.attributes[a][2]))) &&
                                ((itVertexInputEmu->second.attributes[a][3] == itVertexInputSim->second.attributes[a][3]) ||
                                 (NaN(itVertexInputEmu->second.attributes[a][3]) && NaN(itVertexInputSim->second.attributes[a][3])));

                            if (!attributeIsEqual)
                            {
                                F32 attrib[4];

                                attrib[0] = itVertexInputEmu->second.attributes[a][0];
                                attrib[1] = itVertexInputEmu->second.attributes[a][1];
                                attrib[2] = itVertexInputEmu->second.attributes[a][2];
                                attrib[3] = itVertexInputEmu->second.attributes[a][3];
                                
                                printf("Validation => For read vertex at instance %d with index %d attribute %d is different:\n",
                                    itVertexInputEmu->second.vertexID.instance, itVertexInputEmu->second.vertexID.index, a);
                                printf("   emuAttrib[%d] = {%f, %f, %f, %f} | (%08x, %08x, %08x, %08x)\n", a,
                                    itVertexInputEmu->second.attributes[a][0], itVertexInputEmu->second.attributes[a][1],
                                    itVertexInputEmu->second.attributes[a][2], itVertexInputEmu->second.attributes[a][3],
                                    ((U32 *)attrib)[0], ((U32 *)attrib)[1], ((U32 *)attrib)[2], ((U32 *)attrib)[3]);

                                attrib[0] = itVertexInputSim->second.attributes[a][0];
                                attrib[1] = itVertexInputSim->second.attributes[a][1];
                                attrib[2] = itVertexInputSim->second.attributes[a][2];
                                attrib[3] = itVertexInputSim->second.attributes[a][3];

                                printf("   simAttrib[%d] = {%f, %f, %f, %f} | (%08x, %08x, %08x, %08x)\n", a,
                                    itVertexInputSim->second.attributes[a][0], itVertexInputSim->second.attributes[a][1],
                                    itVertexInputSim->second.attributes[a][2], itVertexInputSim->second.attributes[a][3],
                                    ((U32 *)attrib)[0], ((U32 *)attrib)[1], ((U32 *)attrib)[2], ((U32 *)attrib)[3]);
                                
                                validationError = true;
                            }
                        }
                    }
                    else
                    {
                        printf("Validation => Instance %d index %d was not found in the simulator read vertex log.\n",
                            itVertexInputEmu->second.vertexID.instance, itVertexInputEmu->second.vertexID.index);

                        validationError = true;
                    }
                }
                
                if (simVertexInputLog.size() > emuVertexInputLog.size())
                {
                    VertexInputMap::iterator itVertexInputSim;
                    
                    for(itVertexInputSim = simVertexInputLog.begin(); itVertexInputSim != simVertexInputLog.end(); itVertexInputSim++)
                    {
                        //  Find index in the behaviorModel log.
                        itVertexInputEmu = emuVertexInputLog.find(itVertexInputSim->second.vertexID);
                        
                        //  Check if the index was found in the behaviorModel log.
                        if (itVertexInputEmu == emuVertexInputLog.end())
                        {
                            printf("Validation => Instance %d index %d was not found in the behaviorModel read vertex log.\n",
                                itVertexInputSim->second.vertexID.instance, itVertexInputSim->second.vertexID.index);

                            validationError = true;
                        }
                    }
                }
                
                //  Clear the log maps.
                emuVertexInputLog.clear();
                simVertexInputLog.clear();
            }
            else
            {
                //  Get the read vertex log from the behaviorModel.
                VertexInputMap &emuVertexInputLog = GpuBehavMdl->GpuBMdl.getVertexInputLog();
                
                //  Clear the log maps.
                emuVertexInputLog.clear();
            }
            

            //  Check if validation must be performed.
            if (!skipValidation)
            {
                //  Get the shaded vertex log from the simulator.
                ShadedVertexMap &simShadedVertexLog = GpuCMdl.StreamController->getShadedVertexInfo();
                
                //  Get the shaded vertex log from the behaviorModel.
                ShadedVertexMap &emuShadedVertexLog = GpuBehavMdl->GpuBMdl.getShadedVertexLog();
                
                
//printf("Validation >> Simulator Shaded Vertices = %d | Behavior Model Shaded Vertices = %d\n", simShadedVertexLog.size(),
//emuShadedVertexLog.size());

                //  Compare the two lists.
                if (simShadedVertexLog.size() != emuShadedVertexLog.size())
                {
                    printf("Validation => Number of shaded vertices is different: behaviorModel = %d simulator = %d\n",
                        emuShadedVertexLog.size(), simShadedVertexLog.size());
                    validationError = true;
                }

                ShadedVertexMap::iterator itEmu;
                
                for(itEmu = emuShadedVertexLog.begin(); itEmu != emuShadedVertexLog.end(); itEmu++)
                {
                    //  Find index in the simulator log.
                    ShadedVertexMap::iterator itSim;
                    
                    itSim = simShadedVertexLog.find(itEmu->second.vertexID);
                    
                    //  Check if the index was found in the simulator log.
                    if (itSim != simShadedVertexLog.end())
                    {
                        //  Compare all the attributes.
                        for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                        {
                            bool attributeIsEqual =
                                ((itEmu->second.attributes[a][0] == itSim->second.attributes[a][0]) ||
                                 (NaN(itEmu->second.attributes[a][0]) && NaN(itSim->second.attributes[a][0]))) &&
                                ((itEmu->second.attributes[a][1] == itSim->second.attributes[a][1]) ||
                                 (NaN(itEmu->second.attributes[a][1]) && NaN(itSim->second.attributes[a][1]))) &&
                                ((itEmu->second.attributes[a][2] == itSim->second.attributes[a][2]) ||
                                 (NaN(itEmu->second.attributes[a][2]) && NaN(itSim->second.attributes[a][2]))) &&
                                ((itEmu->second.attributes[a][3] == itSim->second.attributes[a][3]) ||
                                 (NaN(itEmu->second.attributes[a][3]) && NaN(itSim->second.attributes[a][3])));

                            if (!attributeIsEqual)
                            {
                                F32 attrib[4];

                                attrib[0] = itEmu->second.attributes[a][0];
                                attrib[1] = itEmu->second.attributes[a][1];
                                attrib[2] = itEmu->second.attributes[a][2];
                                attrib[3] = itEmu->second.attributes[a][3];
                                
                                printf("Validation => For shaded vertex at instance %d index %d attribute %d is different:\n",
                                    itEmu->second.vertexID.instance, itEmu->second.vertexID.index, a);
                                printf("   emuAttrib[%d] = {%f, %f, %f, %f} | (%08x, %08x, %08x, %08x)\n", a,
                                    itEmu->second.attributes[a][0], itEmu->second.attributes[a][1],
                                    itEmu->second.attributes[a][2], itEmu->second.attributes[a][3],
                                    ((U32 *)attrib)[0], ((U32 *)attrib)[1], ((U32 *)attrib)[2], ((U32 *)attrib)[3]);
                                printf("   simAttrib[%d] = {%f, %f, %f, %f} | (%08x, %08x, %08x, %08x)\n", a,
                                    itSim->second.attributes[a][0], itSim->second.attributes[a][1],
                                    itSim->second.attributes[a][2], itSim->second.attributes[a][3],
                                    ((U32 *)attrib)[0], ((U32 *)attrib)[1], ((U32 *)attrib)[2], ((U32 *)attrib)[3]);

                                validationError = true;
                            }
                        }
                    }
                    else
                    {
                        printf("Validation => Instance %d Index %d was not found in the simulator shaded vertex log.\n",
                            itEmu->second.vertexID.instance, itEmu->second.vertexID.index);

                        validationError = true;
                    }
                }
                
                if (simShadedVertexLog.size() > emuShadedVertexLog.size())
                {
                    ShadedVertexMap::iterator itSim;
                    
                    for(itSim = simShadedVertexLog.begin(); itSim != simShadedVertexLog.end(); itSim++)
                    {
                        //  Find index in the behaviorModel log.
                        itEmu = emuShadedVertexLog.find(itSim->second.vertexID);
                        
                        //  Check if the index was found in the behaviorModel log.
                        if (itEmu == emuShadedVertexLog.end())
                        {
                            printf("Validation => Instance %d Index %d was not found in the behaviorModel shaded vertex log.\n",
                                itSim->second.vertexID.instance, itSim->second.vertexID.index);

                            validationError = true;
                        }
                    }
                }
                
                
                //  Clear the log maps.
                emuShadedVertexLog.clear();
                simShadedVertexLog.clear();

                //  Get the z stencil memory update log from the behaviorModel.
                FragmentQuadMemoryUpdateMap &emuZStencilUpdateLog = GpuBehavMdl->GpuBMdl.getZStencilUpdateMap();
                               
                FragmentQuadMemoryUpdateMap simZStencilUpdateLog;
                
                //  Get the z stencil memory update logs from the simulator Z Stencil Test units.
                for(U32 rop = 0; rop < ArchConf.gpu.numStampUnits; rop++)
                {
                    //  Get the z stencil memory update log from the z stencil test unit.
                    FragmentQuadMemoryUpdateMap &simZStencilUpdateROPLog = GpuCMdl.zStencilV2[rop]->getZStencilUpdateMap();
                    
                    FragmentQuadMemoryUpdateMap::iterator it;

                    //  Retrieve the updates from the z stencil unit.
                    for(it = simZStencilUpdateROPLog.begin(); it != simZStencilUpdateROPLog.end(); it++)
                        simZStencilUpdateLog.insert(make_pair(it->first, it->second));
                    
                    //  Clear the z stencil update log.
                    simZStencilUpdateROPLog.clear();
                }
                
//printf("Validation >> Simulator ZST Updates = %d | Behavior Model ZST Updates = %d\n", simZStencilUpdateLog.size(),
//emuZStencilUpdateLog.size());

                //  Check the number of updates.
                if (simZStencilUpdateLog.size() != emuZStencilUpdateLog.size())
                {
                    printf("Validation => Number of updates to Z Stencil Buffer differs: behaviorModel %d simulator %d\n",
                        emuZStencilUpdateLog.size(), simZStencilUpdateLog.size());
                
                    validationError = true;
                }
                
                
                FragmentQuadMemoryUpdateMap::iterator itZStencilUpdateEmu;
                
                U32 maxDifferences = 30;
                
                U32 checkedQuads = 0;
                
                //  Traverse the whole update log and search differences.
                for(itZStencilUpdateEmu = emuZStencilUpdateLog.begin();
                    itZStencilUpdateEmu != emuZStencilUpdateLog.end() && (maxDifferences > 0); itZStencilUpdateEmu++)
                {
                    FragmentQuadMemoryUpdateMap::iterator itZStencilUpdateSim;
                    
                    //  Search the update in the simulator log.
                    itZStencilUpdateSim = simZStencilUpdateLog.find(itZStencilUpdateEmu->first);
                    
                    if (itZStencilUpdateSim == simZStencilUpdateLog.end())
                    {
                        printf("Validation => Z Stencil memory update on quad at (%d, %d) triangle %d not found in the simulator log.\n",   
                            itZStencilUpdateEmu->first.x, itZStencilUpdateEmu->first.y, itZStencilUpdateEmu->first.triangleID);
                        
                        maxDifferences--;
                    }
                    else
                    {
                        //  Compare the two updates.
                        
                        bool diffInReadData = false;                        
                        U32 bytesPixel = 4;
                        
                        //  Compare read data.
                        for(U32 qw = 0; (qw < ((bytesPixel * STAMP_FRAGMENTS)) >> 3) && !diffInReadData; qw++)
                            diffInReadData = (((U64 *) itZStencilUpdateEmu->second.readData)[qw] != ((U64 *) itZStencilUpdateSim->second.readData)[qw]);
                        
                        bool diffInWriteData = false;
                        
                        //  Compare write data.
                        for(U32 qw = 0; (qw < ((bytesPixel * STAMP_FRAGMENTS)) >> 3) && !diffInWriteData; qw++)
                            diffInWriteData = (((U64 *) itZStencilUpdateEmu->second.writeData)[qw] != ((U64 *) itZStencilUpdateSim->second.writeData)[qw]);
                    
                        if (diffInReadData)
                            printf("Validation => Difference found in z stencil read data for the update on quad at (%d, %d) triangle %d\n",
                                itZStencilUpdateEmu->first.x, itZStencilUpdateEmu->first.y, itZStencilUpdateEmu->first.triangleID);

                        if (diffInWriteData)
                            printf("Validation => Difference found in z stencil write data for the update on quad at (%d, %d) triangle %d\n",
                                itZStencilUpdateEmu->first.x, itZStencilUpdateEmu->first.y, itZStencilUpdateEmu->first.triangleID);
                                
                        if (diffInReadData || diffInWriteData)
                        {
                            printf("  Behavior Model Update : \n");
                            printf("    Read Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateEmu->second.readData)[dw]);
                            }
                            printf("\n");
                            printf("    Input Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateEmu->second.inData)[dw]);
                            }
                            printf("\n");
                            printf("    Write Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateEmu->second.writeData)[dw]);
                            }
                            printf("\n");
                            printf("    Write Mask : ");
                            for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                            {
                                printf(" %c", itZStencilUpdateEmu->second.writeMask[b] ? 'W' : '_');
                            }
                            printf("\n");
                            printf("    Cull Mask : ");
                            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                printf(" %c", itZStencilUpdateEmu->second.cullMask[f] ? 'T' : 'F');
                            }
                            printf("\n");
                            
                            printf("  Simulator Update : \n");
                            printf("    Read Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateSim->second.readData)[dw]);
                            }
                            printf("\n");
                            printf("    Input Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateSim->second.inData)[dw]);
                            }
                            printf("\n");
                            printf("    Write Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itZStencilUpdateSim->second.writeData)[dw]);
                            }
                            printf("\n");
                            printf("    Write Mask : ");
                            for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                            {
                                printf(" %c", itZStencilUpdateSim->second.writeMask[b] ? 'W' : '_');
                            }
                            printf("\n");
                            printf("    Cull Mask : ");
                            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                printf(" %c", itZStencilUpdateSim->second.cullMask[f] ? 'T' : 'F');
                            }
                            printf("\n");

                            maxDifferences--;
                            
                            validationError = true;
                        }
                    }
                    
                    if (maxDifferences == 0)
                        printf("-- Reached maximum difference log limit  --\n");
                        
                    checkedQuads++;
                }
                
                //printf("Validation => Checked %d quads at Z Stencil stage\n", checkedQuads);

                //  Check the number of updates.
                if (simZStencilUpdateLog.size() > emuZStencilUpdateLog.size())
                {
                    FragmentQuadMemoryUpdateMap::iterator itZStencilUpdateSim;
                    
                    U32 maxDifferences = 30;
                    
                    U32 checkedQuads = 0;
                    
                    //  Traverse the whole update log and search differences.
                    for(itZStencilUpdateSim = simZStencilUpdateLog.begin();
                        itZStencilUpdateSim != simZStencilUpdateLog.end() && (maxDifferences > 0); itZStencilUpdateSim++)
                    {
                        //  Search the update in the behaviorModel log.
                        itZStencilUpdateEmu = emuZStencilUpdateLog.find(itZStencilUpdateSim->first);
                        
                        if (itZStencilUpdateEmu == emuZStencilUpdateLog.end())
                        {
                            printf("Validation => Z Stencil memory update on quad at (%d, %d) triangle %d not found in the behaviorModel log.\n",   
                                itZStencilUpdateSim->first.x, itZStencilUpdateSim->first.y, itZStencilUpdateSim->first.triangleID);
                            
                            maxDifferences--;
                        }
                    }
                }                
                
                //  Clear the z stencil memory update log.
                emuZStencilUpdateLog.clear();
                simZStencilUpdateLog.clear();
                
                for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
                {
                    //  Get the color memory update log from the behaviorModel.
                    FragmentQuadMemoryUpdateMap &emuColorUpdateLog = GpuBehavMdl->GpuBMdl.getColorUpdateMap(rt);
                    
                    FragmentQuadMemoryUpdateMap simColorUpdateLog;

                    //  Get the color memory update logs from the simulator Color Write units.
                    for(U32 rop = 0; rop < ArchConf.gpu.numStampUnits; rop++)
                    {
                        //  Get the color memory update log from the color test unit.
                        FragmentQuadMemoryUpdateMap &simColorUpdateROPLog = GpuCMdl.colorWriteV2[rop]->getColorUpdateMap(rt);
                        
                        FragmentQuadMemoryUpdateMap::iterator it;

                        //  Retrieve the updates from the color write unit.
                        for(it = simColorUpdateROPLog.begin(); it != simColorUpdateROPLog.end(); it++)
                            simColorUpdateLog.insert(make_pair(it->first, it->second));
                        
                        //  Clear the z stencil update log.
                        simColorUpdateROPLog.clear();
                    }
                                       
//printf("Validation >> Render Target = %d | Simulator CW Updates = %d | Behavior Model CW Updates = %d\n", rt, simColorUpdateLog.size(),
//emuColorUpdateLog.size());

                    //  Check the number of updates.
                    if (simColorUpdateLog.size() != emuColorUpdateLog.size())
                    {
                        printf("Validation => Number of updates to Color Write Buffer differs: behaviorModel %d simulator %d\n",
                            emuColorUpdateLog.size(), simColorUpdateLog.size());
                    
                        validationError = true;
                    }
                    
                    FragmentQuadMemoryUpdateMap::iterator itColorUpdateEmu;
                    
                    U32 maxDifferences = 30;
                    
                    U32 checkedQuads = 0;
                    
                    TextureFormat rtFormat = GpuBehavMdl->GpuBMdl.getRenderTargetFormat(rt);
                    
                    //  Traverse the whole update log and search differences.
                    for(itColorUpdateEmu = emuColorUpdateLog.begin();
                        itColorUpdateEmu != emuColorUpdateLog.end() && (maxDifferences > 0); itColorUpdateEmu++)
                    {
                        FragmentQuadMemoryUpdateMap::iterator itColorUpdateSim;
                        
                        //  Search the update in the simulator log.
                        itColorUpdateSim = simColorUpdateLog.find(itColorUpdateEmu->first);
                        
                        if (itColorUpdateSim == simColorUpdateLog.end())
                        {
                            printf("Validation => Color memory update on quad at (%d, %d) triangle %d not found in the simulator log.\n",   
                                itColorUpdateEmu->first.x, itColorUpdateEmu->first.y, itColorUpdateEmu->first.triangleID);
                        
                            //  Compare the two updates.
                            
                            U32 bytesPixel;
                            
                            switch(rtFormat)
                            {
                                case GPU_RGBA8888:
                                case GPU_RG16F:
                                case GPU_R32F:
                                    bytesPixel = 4;
                                    break;
                                case GPU_RGBA16:
                                case GPU_RGBA16F:
                                    bytesPixel = 8;
                                    break;
                                default:
                                    CG_ASSERT("Unsupported render target format\n");
                            }
                            
                            printf("  Behavior Model Update : \n");
                            printf("    Read Data : ");
                            for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                            {
                                printf("  %08x", ((U32 *) itColorUpdateEmu->second.readData)[dw]);
                            }
                            printf("\n");
                            printf("    Input Data : ");
                            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                printf("  (%f, %f, %f, %f) ", ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 0],
                                                              ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 1],
                                                              ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 2],
                                                              ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 3]);
                            }
                            printf("\n");
                            printf("    Write Data : ");
                            if (rtFormat == GPU_RGBA8888)
                            {
                                for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                {
                                    printf("  %08x", ((U32 *) itColorUpdateEmu->second.writeData)[dw]);
                                }
                            }
                            printf("\n");
                            printf("    Write Mask : ");
                            for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                            {
                                printf(" %c", itColorUpdateEmu->second.writeMask[b] ? 'W' : '_');
                            }
                            printf("\n");
                            printf("    Cull Mask : ");
                            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                printf(" %c", itColorUpdateEmu->second.cullMask[f] ? 'T' : 'F');
                            }
                            printf("\n");
                            
                            maxDifferences--;
                        }
                        else
                        {
                            //  Compare the two updates.
                            
                            U32 bytesPixel;
                            
                            switch(rtFormat)
                            {
                                case GPU_RGBA8888:
                                case GPU_RG16F:
                                case GPU_R32F:
                                    bytesPixel = 4;
                                    break;
                                case GPU_RGBA16:
                                case GPU_RGBA16F:
                                    bytesPixel = 8;
                                    break;
                                default:
                                    CG_ASSERT("Unsupported render target format\n");
                            }

                            bool diffInReadData = false;
                            
                            //  Compare read data.
                            for(U32 qw = 0; (qw < ((bytesPixel * STAMP_FRAGMENTS)) >> 3) && !diffInReadData; qw++)
                                diffInReadData = (((U64 *) itColorUpdateEmu->second.readData)[qw] != ((U64 *) itColorUpdateSim->second.readData)[qw]);
                                                        
                            bool diffInWriteData = false;
                            
                            //  Compare write data.
                            for(U32 qw = 0; (qw < ((bytesPixel * STAMP_FRAGMENTS)) >> 3) && !diffInWriteData; qw++)
                                diffInWriteData = (((U64 *) itColorUpdateEmu->second.writeData)[qw] != ((U64 *) itColorUpdateSim->second.writeData)[qw]);
                        
                            if (diffInReadData)
                                printf("Validation => Difference found in color read data for the update on quad at (%d, %d) triangle %d\n",
                                    itColorUpdateEmu->first.x, itColorUpdateEmu->first.y, itColorUpdateEmu->first.triangleID);

                            if (diffInWriteData)
                                printf("Validation => Difference found in color write data for the update on quad at (%d, %d) triangle %d\n",
                                    itColorUpdateEmu->first.x, itColorUpdateEmu->first.y, itColorUpdateEmu->first.triangleID);
                                    
                            if (diffInReadData || diffInWriteData)
                            {
                                printf("  Behavior Model Update : \n");
                                printf("    Read Data : ");
                                for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                {
                                    printf("  %08x", ((U32 *) itColorUpdateEmu->second.readData)[dw]);
                                }
                                printf("\n");
                                printf("    Input Data : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf("  (%f, %f, %f, %f) ", ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 0],
                                                                  ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 1],
                                                                  ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 2],
                                                                  ((F32 *) itColorUpdateEmu->second.inData)[f * 4 + 3]);
                                }
                                printf("\n");
                                printf("    Write Data : ");
                                if (rtFormat == GPU_RGBA8888)
                                {
                                    for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                    {
                                        printf("  %08x", ((U32 *) itColorUpdateEmu->second.writeData)[dw]);
                                    }
                                }
                                printf("\n");
                                printf("    Write Mask : ");
                                for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                                {
                                    printf(" %c", itColorUpdateEmu->second.writeMask[b] ? 'W' : '_');
                                }
                                printf("\n");
                                printf("    Cull Mask : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf(" %c", itColorUpdateEmu->second.cullMask[f] ? 'T' : 'F');
                                }
                                printf("\n");
                                
                                printf("  Simulator Update : \n");
                                printf("    Read Data : ");
                                if (rtFormat == GPU_RGBA8888)
                                {
                                    for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                    {
                                        printf("  %08x", ((U32 *) itColorUpdateSim->second.readData)[dw]);
                                    }
                                }
                                printf("\n");
                                printf("    Input Data : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf("  (%f, %f, %f, %f) ", ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 0],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 1],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 2],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 3]);
                                }
                                printf("\n");
                                printf("    Write Data : ");
                                if (rtFormat == GPU_RGBA8888)
                                {
                                    for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                    {
                                        printf("  %08x", ((U32 *) itColorUpdateSim->second.writeData)[dw]);
                                    }
                                }
                                printf("\n");
                                printf("    Write Mask : ");
                                for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                                {
                                    printf(" %c", itColorUpdateSim->second.writeMask[b] ? 'W' : '_');
                                }
                                printf("\n");
                                printf("    Cull Mask : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf(" %c", itColorUpdateSim->second.cullMask[f] ? 'T' : 'F');
                                }
                                printf("\n");

                                maxDifferences--;
                                
                                validationError = true;
                            }
                        }
                        
                        checkedQuads++;
                    
                        if (maxDifferences == 0)
                            printf("-- Reached maximum difference log limit  --\n");
                    }
                    
                    //  Check the number of updates.
                    if (simColorUpdateLog.size() > emuColorUpdateLog.size())
                    {
                        FragmentQuadMemoryUpdateMap::iterator itColorUpdateSim;
                        
                        U32 maxDifferences = 30;
                        
                        U32 checkedQuads = 0;
                        
                        TextureFormat rtFormat = GpuBehavMdl->GpuBMdl.getRenderTargetFormat(rt);
                        
                        //  Traverse the whole update log and search differences.
                        for(itColorUpdateSim = simColorUpdateLog.begin();
                            itColorUpdateSim != simColorUpdateLog.end() && (maxDifferences > 0); itColorUpdateSim++)
                        {
                            //  Search the update in the behaviorModel log.
                            itColorUpdateEmu = emuColorUpdateLog.find(itColorUpdateSim->first);
                            
                            if (itColorUpdateEmu == emuColorUpdateLog.end())
                            {
                                printf("Validation => Color memory update on quad at (%d, %d) triangle %d not found in the behaviorModel log.\n",   
                                    itColorUpdateSim->first.x, itColorUpdateSim->first.y, itColorUpdateSim->first.triangleID);
                            
                                //  Compare the two updates.
                                
                                U32 bytesPixel;
                                
                                switch(rtFormat)
                                {
                                    case GPU_RGBA8888:
                                    case GPU_RG16F:
                                    case GPU_R32F:
                                        bytesPixel = 4;
                                        break;
                                    case GPU_RGBA16:
                                    case GPU_RGBA16F:
                                        bytesPixel = 8;
                                        break;
                                    default:
                                        CG_ASSERT("Unsupported render target format\n");
                                }
                                
                                printf("  Simulator Update : \n");
                                printf("    Read Data : ");
                                for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                {
                                    printf("  %08x", ((U32 *) itColorUpdateSim->second.readData)[dw]);
                                }
                                printf("\n");
                                printf("    Input Data : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf("  (%f, %f, %f, %f) ", ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 0],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 1],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 2],
                                                                  ((F32 *) itColorUpdateSim->second.inData)[f * 4 + 3]);
                                }
                                printf("\n");
                                printf("    Write Data : ");
                                if (rtFormat == GPU_RGBA8888)
                                {
                                    for(U32 dw = 0; dw < ((bytesPixel * STAMP_FRAGMENTS) >> 2); dw++)
                                    {
                                        printf("  %08x", ((U32 *) itColorUpdateSim->second.writeData)[dw]);
                                    }
                                }
                                printf("\n");
                                printf("    Write Mask : ");
                                for(U32 b = 0; b < (bytesPixel * STAMP_FRAGMENTS); b++)
                                {
                                    printf(" %c", itColorUpdateSim->second.writeMask[b] ? 'W' : '_');
                                }
                                printf("\n");
                                printf("    Cull Mask : ");
                                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                                {
                                    printf(" %c", itColorUpdateSim->second.cullMask[f] ? 'T' : 'F');
                                }
                                printf("\n");
                                
                                maxDifferences--;
                            }
                        }
                    }
                    
                    //printf("Validation => Checked %d quads at Color Write stage\n", checkedQuads);
                    
                    //  Clear the color memory update log.
                    emuColorUpdateLog.clear();
                    simColorUpdateLog.clear();
                }
            }
            else
            {
                //  Get the shaded vertex log from the behaviorModel.
                ShadedVertexMap &emuShadedVertexLog = GpuBehavMdl->GpuBMdl.getShadedVertexLog();
                
                //  Clear the log maps.
                emuShadedVertexLog.clear();

                //  Get the z stencil memory update log from the behaviorModel.
                FragmentQuadMemoryUpdateMap &emuZStencilUpdateLog = GpuBehavMdl->GpuBMdl.getZStencilUpdateMap();
                
                //  Clear the z stencil memory update log.
                emuZStencilUpdateLog.clear();
                
                for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
                {
                    //  Get the color memory update log from the behaviorModel.
                    FragmentQuadMemoryUpdateMap &emuColorUpdateLog = GpuBehavMdl->GpuBMdl.getColorUpdateMap(rt);
                    
                    //  Clear the color memory ypdate log.
                    emuColorUpdateLog.clear();
                }
            }                          
        }
        
        //  Check if auto snapshot is enabled and there is no pending snapshot.
        if (autoSnapshotEnable && !pendingSaveSnapshot)
        {
            //  Get new time.
            U64 newTime = time(NULL);
            
            //  Check if enough time has passed to save a new snapshot.
            if ((newTime - startTime) >= (snapshotFrequency * 60))
            {
            
                printf("Autosaving at time %d.  Auto save frequency %d minutes.  Times since last save in seconds %d\n",
                    newTime, snapshotFrequency, newTime - startTime);
                    
                //  Save a new snapshot.
                saveSnapshotCommand();
                
                //  Update start time.
                startTime = time(NULL);
            }
        }
    }
    
    //  Check if the current frame has finished.
    if (GpuCMdl.CP->isSwap())
    {
        //  Update rendered frames counter.
        frameCounter++;

        //  Reset batch counter for the current batch.
        frameBatch = 0;
        
        //  Set the end of frame flag.
        endOfFrame = true;

        //  In validation mode process all the transactions up to the swap.
        if (validationMode)
        {
            //  Get the log of MetaStreams processed by the Command Processor.
            vector<cgoMetaStream *> &metaStreamLog = GpuCMdl.CP->getcgoMetaStreamLog();
            
            //  Issue the MetaStreams to the behaviorModel.
            for(U32 trans = 0; trans < metaStreamLog.size(); trans++)
            {
                GpuBehavMdl->GpuBMdl.emulateCommandProcessor(metaStreamLog[trans]);
            }
            
            //  Clear the MetaStream log.
            metaStreamLog.clear();
        }
    }
    
    //  Check if trace has finished
    if(!endOfTrace && GpuCMdl.CP->isEndOfTrace())
    {
        //  Set the end of trace flag.
        endOfTrace = true;
        cout << "--- end of simulation trace ---" << endl;

        //  In validation mode process all the transactions up to the end of the trace.
        if (validationMode)
        {
            //  Get the log of MetaStreams processed by the Command Processor.
            vector<cgoMetaStream *> &metaStreamLog = GpuCMdl.CP->getcgoMetaStreamLog();
            
            //  Issue the MetaStreams to the behaviorModel.
            for(U32 trans = 0; trans < metaStreamLog.size(); trans++)
            {
                GpuBehavMdl->GpuBMdl.emulateCommandProcessor(metaStreamLog[trans]);
            }
            
            //  Clear the MetaStream log.
            metaStreamLog.clear();
            
            cout << "--- end of emulation trace ---" << endl;
        }
    }

    
    //  Check stalls every 100K cycles.
    if (checkStalls)
    {
        gpuStalled = false;
        
        if (!GpuCMdl.multiClock)
        {
            for(U32 b = 0; b < GpuCMdl.MduArray.size() && !gpuStalled; b++)
            {
                bool stallDetectionEnabled;
                bool boxStalled;

                GpuCMdl.MduArray[b]->detectStall(cycle, stallDetectionEnabled, boxStalled);
                
                gpuStalled = gpuStalled || (stallDetectionEnabled && boxStalled);
                
                if (gpuStalled)
                {
                    printf("Stall detected on mdu %s at cycle %lld\n", GpuCMdl.MduArray[b]->getName(), cycle);
                }
            }
        }
        else
        {
                for(U32 mdu = 0; mdu < GpuCMdl.GpuDomainMduArray.size() && !gpuStalled; mdu++)
                {
                    bool stallDetectionEnabled;
                    bool boxStalled;
                       
                    GpuCMdl.GpuDomainMduArray[mdu]->detectStall(gpuCycle, stallDetectionEnabled, boxStalled);
                    
                    gpuStalled = gpuStalled || (stallDetectionEnabled && boxStalled);
                    
                    if (gpuStalled)
                    {
                        printf("Stall detected on mdu %s at cycle %lld\n", GpuCMdl.GpuDomainMduArray[mdu]->getName(), gpuCycle);
                    }
                }
                   
                //for(U32 mdu = 0; mdu < GpuCMdl.ShaderDomainMduArray.size(); mdu++)
                //    GpuCMdl.ShaderDomainMduArray[mdu]->clock(GPU_CLOCK_DOMAIN, gpuCycle);
                   
                //for(U32 mdu = 0; mdu < GpuCMdl.MemoryDomainMduArray.size(); mdu++)
                //    GpuCMdl.MemoryDomainMduArray[mdu]->clock(GPU_CLOCK_DOMAIN, gpuCycle);
        }
                
        if (gpuStalled)
        {
            ofstream reportFile;
            
            reportFile.open("StallReport.txt");                               
            
            if (!reportFile.is_open())
                cout << "ERROR: Couldn't create report file." << endl;
            
            cout << "Reporting stall for all boxes : " << endl;
            
            if (reportFile.is_open())
                reportFile <<  "Reporting stall for all boxes : " << endl;
            
            if (!GpuCMdl.multiClock)
            {
                //  Dump stall reports for all boxes.
                for(U32 b = 0; b < GpuCMdl.MduArray.size(); b++)
                {
                    string report;
                    GpuCMdl.MduArray[b]->stallReport(cycle, report);
                                        
                    cout << report << endl;
                    
                    if (reportFile.is_open())
                        reportFile << report << endl;
                }
            }
            else
            {
                for(U32 mdu = 0; mdu < GpuCMdl.GpuDomainMduArray.size() && !gpuStalled; mdu++)
                {
                    string report;
                    GpuCMdl.GpuDomainMduArray[mdu]->stallReport(gpuCycle, report);
                                        
                    cout << report << endl;
                    
                    if (reportFile.is_open())
                        reportFile << report << endl;
                }
            }
                    
            cout << "Stall report written into StallReport.txt file." << endl;    
            
            reportFile.close();
            
            createSnapshot();
        }
    }

    //  Update progress counter.  
    dotCount++;

    //  Display progress dots.  
    if (dotCount == 10000)
    {
        dotCount = 0;
        cout << '.';
        cout << flush;
    }
}


void CG1CMDL::simulationLoop(cgeModelAbstractLevel MAL)
{
    U32 width, height;
    U32 i;
    bool end;
    current = this;
    simulationStarted = true;
   
    for(cycle = 0, end = false, dotCount = 0; !end; cycle++) //  Simulation loop.
    {
        CG_WARN("Cycle %ld ----------------------------\n", cycle);
        if (ArchConf.sim.dumpSignalTrace && (cycle >= ArchConf.sim.startDump) && (cycle <= (ArchConf.sim.startDump + ArchConf.sim.dumpCycles)))  //  Dump the signals.
            sigBinder.dumpSignalTrace(cycle);
        cyclesCounter->inc(); //  Update cycle counter statistic.

        for(i = 0; i < GpuCMdl.MduArray.size(); i++) // Clock all the boxes.
            GpuCMdl.MduArray[i]->clock(cycle);

        if (ArchConf.sim.statistics) //  Check if statistics generation is active.
        {
            gpuStatistics::StatisticsManager::instance().clock(cycle); //  Update statistics.
            if (GpuCMdl.CP->endOfBatch() && ArchConf.sim.perBatchStatistics) //  Check if current batch has finished.
                gpuStatistics::StatisticsManager::instance().batch(); //  Update statistics.
           
            if (GpuCMdl.CP->isSwap() && ArchConf.sim.perFrameStatistics) //  Check if statistics and per frame statistics are enabled.
                gpuStatistics::StatisticsManager::instance().frame(frameCounter); //  Update statistics.
        }

        if (GpuCMdl.CP->endOfBatch())  //  Check end of batch event.
        {
            batchCounter++; //  Update rendered batches counter.
            frameBatch++; //  Update rendered batches in the current frame.
        }
       
        if (GpuCMdl.CP->isSwap()) //  Check if color buffer swap has started and fragment map is enabled.
        {
            if  (ArchConf.sim.fragmentMap) //  Check if fragment map dumping is enabled.
            {
                for(i = 0; i < ArchConf.gpu.numStampUnits; i++) //  Get the latency maps from the color write units.
                    latencyMap[i] = GpuCMdl.colorWriteV2[i]->getLatencyMap(width, height);
                dumpLatencyMap(width, height); //  Dump the latency map.
            }
            frameCounter++; //  Update frame counter.
            frameBatch = 0; //  Reset batch counter for the current batch.
            if (ArchConf.sim.simFrames != 0) //  Determine end of simulation.
                end = end || ((frameCounter - ArchConf.sim.startFrame) == ArchConf.sim.simFrames); //  End if the number of requested frames has been rendered.
        }
        
        dotCount++; //  Update progress counter.
        if (dotCount == 10000) //  Display progress dots.
        {
            dotCount = 0;
            putchar('.');
            fflush(stdout);
//DynamicMemoryOpt::usage();
            end = end || GpuCMdl.CP->isEndOfTrace(); //  Check if trace has finished.
        }

        if (ArchConf.sim.simCycles != 0) //  Determine end of simulation.
            end = end || ((cycle + 1) == ArchConf.sim.simCycles);
        
        if (ArchConf.sim.detectStalls && (cycle % 100000) == 99999)  //  Check stalls every 100K cycles.
        {
            bool gpuStalled = false;
            for(i = 0; i < GpuCMdl.MduArray.size() && !gpuStalled; i++)
            {
                bool stallDetectionEnabled;
                bool boxStalled;

                GpuCMdl.MduArray[i]->detectStall(cycle, stallDetectionEnabled, boxStalled);
                gpuStalled = gpuStalled || (stallDetectionEnabled && boxStalled);
                CG_INFO_COND(gpuStalled, "Stall detected on mdu %s at cycle %lld\n", GpuCMdl.MduArray[i]->getName(), cycle);
            }
            
            if (gpuStalled)
            {
                ofstream reportFile;
                reportFile.open("StallReport.txt");                               
                if (!reportFile.is_open())
                    cout << "ERROR: Couldn't create report file." << endl;
                cout << "Reporting stall for all boxes : " << endl;

                if (reportFile.is_open())
                    reportFile <<  "Reporting stall for all boxes : " << endl;

                for(i = 0; i < GpuCMdl.MduArray.size(); i++) //  Dump stall reports for all boxes.
                {
                    string report;
                    GpuCMdl.MduArray[i]->stallReport(cycle, report);
                    cout << report << endl;
                    if (reportFile.is_open())
                        reportFile << report << endl;
                }
                cout << "Stall report written into StallReport.txt file." << endl;    
                reportFile.close();
                createSnapshot();
            } // if (gpuStalled)
            end = end || gpuStalled;  //  End the simulation if a stall was detected.                  
        } // if (ArchConf.sim.detectStalls && ((cycle% 100000) == 99999))  //  Check stalls every 100K cycles.
    } // for(cycle = 0, end = false, dotCount = 0; !end; cycle++) //  Simulation loop.

    GPU_DEBUG( printf("\nEND Cycle %lld ----------------------------\n", cycle); )

    if (ArchConf.sim.dumpSignalTrace)           //  Check signal trace dump enabled. 
        sigBinder.endSignalTrace();     //  End signal tracing.  

    DynamicMemoryOpt::usage();
    gpuStatistics::StatisticsManager::instance().finish();

    //DynamicMemoryOpt::dumpDynamicMemoryState(FALSE, FALSE);
}

void CG1CMDL::simulationLoopMultiClock()
{
    U32 width, height;
    U32 i;
    bool end;

    current = this;

    // Initialize clock, step counters, etc.
    gpuCycle = 0;
    shaderCycle = 0;
    memoryCycle = 0;
    nextGPUClock = gpuClockPeriod;
    nextShaderClock = shaderClockPeriod;
    nextMemoryClock = memoryClockPeriod;
    end = false;
    
    simulationStarted = true;

    while(!end)
    {
        //
        //  WARNING: Signal Trace Dump not supported yet with multi clock domain.
        //
        //  Dump the signals.
        //if (ArchConf.sim.dumpSignalTrace && (cycle >= ArchConf.sim.startDump) && (cycle <= (ArchConf.sim.startDump + ArchConf.sim.dumpCycles)))
        //    sigBinder.dumpSignalTrace(cycle);

        //  Determine the picoseconds to the next clock.
        U32 nextStep = GPU_MIN(GPU_MIN(nextGPUClock, nextShaderClock), nextMemoryClock);
        
        //printf("Clock -> Next GPU Clock = %d | Next Shader Clock = %d | Next Memory Clock = %d | Next Step = %d | GPU Cycle = %lld | Shader Cycle = %lld | MemoryCycle = %lld\n",
        //    nextGPUClock, nextShaderClock, nextMemoryClock, nextStep, gpuCycle, shaderCycle, memoryCycle);
                        
        //  Update step counters for both clock domains.
        nextGPUClock -= nextStep;
        nextShaderClock -= nextStep;
        nextMemoryClock -= nextStep;
        
        //  Check if a gpu domain tick has to be issued.
        if (nextGPUClock == 0)
        {
            GPU_DEBUG(
                printf("GPU Domain. Cycle %lld ----------------------------\n", gpuCycle);
            )

            // Clock all the boxes in the GPU Domain.
            for(i = 0; i < GpuCMdl.GpuDomainMduArray.size(); i++)
                GpuCMdl.GpuDomainMduArray[i]->clock(gpuCycle);
                
            //  Clock boxes with multiple domains.
            for(i = 0; i < GpuCMdl.ShaderDomainMduArray.size(); i++)
                GpuCMdl.ShaderDomainMduArray[i]->clock(GPU_CLOCK_DOMAIN, gpuCycle);

            for(i = 0; i < GpuCMdl.MemoryDomainMduArray.size(); i++)
                GpuCMdl.MemoryDomainMduArray[i]->clock(GPU_CLOCK_DOMAIN, gpuCycle);

            //  Update cycle counter statistic.
            cyclesCounter->inc();
            
            //  Check if statistics generation is active.
            if (ArchConf.sim.statistics)
            {
                //  Update statistics.
                gpuStatistics::StatisticsManager::instance().clock(gpuCycle);

                //  Check if current batch has finished.
                if (GpuCMdl.CP->endOfBatch() && ArchConf.sim.perBatchStatistics)
                {
                    //  Update statistics.
                    gpuStatistics::StatisticsManager::instance().batch();
                }


                //  Check if statistics and per frame statistics are enabled.
                if (GpuCMdl.CP->isSwap() && ArchConf.sim.perFrameStatistics)
                {
                    //  Update statistics.  
                    gpuStatistics::StatisticsManager::instance().frame(frameCounter);
                }
            }
            
            //  Check end of batch event.
            if (GpuCMdl.CP->endOfBatch())
            {
                //  Update rendered batches counter.
                batchCounter++;
                
                //  Update rendered batches in the current frame.
                frameBatch++;
            }

            //  Check if color buffer swap has started and fragment map is enabled.
            if (GpuCMdl.CP->isSwap())
            {
                //  Check if fragment map dumping is enabled.
                if  (ArchConf.sim.fragmentMap)
                {
                    //  Get the latency maps from the color write units.
                    for(i = 0; i < ArchConf.gpu.numStampUnits; i++)
                        latencyMap[i] = GpuCMdl.colorWriteV2[i]->getLatencyMap(width, height);

                    //  Dump the latency map.
                    dumpLatencyMap(width, height);
                }

                //  Update frame counter.
                frameCounter++;
                
                //  Reset batch counter for the current batch.
                frameBatch = 0;

                //  Determine end of simulation.
                if (ArchConf.sim.simFrames != 0)
                {
                    //  End if the number of requested frames has been rendered.
                    end = end || ((frameCounter - ArchConf.sim.startFrame) == ArchConf.sim.simFrames);
                }
            }
            
            //  Update progress counter.
            dotCount++;

            //  Display progress dots.
            if (dotCount == 10000)
            {
                dotCount = 0;
                putchar('.');
                fflush(stdout);

//DynamicMemoryOpt::usage();

                //  Check if trace has finished.
                end = end || GpuCMdl.CP->isEndOfTrace();
            }

            //  Determine end of simulation.
            if (ArchConf.sim.simCycles != 0)
            {
                end = end || ((gpuCycle + 1) == ArchConf.sim.simCycles);
            }
            
            //  Check stalls every 100K cycles.
            if (ArchConf.sim.detectStalls && ((gpuCycle % 100000) == 99999))
            {
                bool gpuStalled = false;
                            
                for(i = 0; i < GpuCMdl.GpuDomainMduArray.size() && !gpuStalled; i++)
                {
                    bool stallDetectionEnabled;
                    bool boxStalled;
                 
                    GpuCMdl.GpuDomainMduArray[i]->detectStall(gpuCycle, stallDetectionEnabled, boxStalled);
                    
                    gpuStalled |= stallDetectionEnabled && boxStalled;
                    
                    if (gpuStalled)
                    {
                        printf("Stall detected on mdu %s at cycle (GPU Domain) %lld\n", GpuCMdl.GpuDomainMduArray[i]->getName(), gpuCycle);
                    }
                }

                for(i = 0; i < GpuCMdl.ShaderDomainMduArray.size() && !gpuStalled; i++)
                {
                    bool stallDetectionEnabled;
                    bool boxStalled;
                 
                    GpuCMdl.ShaderDomainMduArray[i]->detectStall(shaderCycle, stallDetectionEnabled, boxStalled);
                    
                    gpuStalled |= stallDetectionEnabled && boxStalled;
                    
                    if (gpuStalled)
                    {
                        printf("Stall detected on mdu %s at cycle (Shader Domain) %lld\n", GpuCMdl.ShaderDomainMduArray[i]->getName(), shaderCycle);
                    }
                }
                
                if (gpuStalled)
                {
                    ofstream reportFile;
                    
                    reportFile.open("StallReport.txt");                               
                    
                    if (!reportFile.is_open())
                        cout << "ERROR: Couldn't create report file." << endl;
                    
                    cout << "Reporting stall for all boxes : " << endl;
                    
                    if (reportFile.is_open())
                        reportFile <<  "Reporting stall for all boxes : " << endl;
                    
                    //  Dump stall reports for all boxes.
                    for(i = 0; i < GpuCMdl.MduArray.size(); i++)
                    {
                        string report;
                        GpuCMdl.MduArray[i]->stallReport(gpuCycle, report);
                                            
                        //cout << report << endl;
                        
                        if (reportFile.is_open())
                            reportFile << report << endl;
                    }
                
                    cout << "Stall report written into StallReport.txt file." << endl;    
                    
                    reportFile.close();
                }
                
                //  End the simulation if a stall was detected.                  
                end = end || gpuStalled;
            }
            
            //  Update GPU domain clock and step counter.
            gpuCycle++;
            nextGPUClock = gpuClockPeriod;
        }
        
        //  Check if a shader domain tick must be issued.
        if (nextShaderClock == 0)
        {
            if (GpuCMdl.shaderClockDomain)
            {
                GPU_DEBUG(
                    printf("Shader Domain. Cycle %lld ----------------------------\n", shaderCycle);
                )

                //  Clock boxes with multiple domains.
                for(i = 0; i < GpuCMdl.ShaderDomainMduArray.size(); i++)
                    GpuCMdl.ShaderDomainMduArray[i]->clock(SHADER_CLOCK_DOMAIN, shaderCycle);

                //  Update shader domain clock and step counter.
                shaderCycle++;
            }
            
            nextShaderClock = shaderClockPeriod;
        }        
        
        //  Check if a memory domain tick must be issued.
        if (nextMemoryClock == 0)
        {
            if (GpuCMdl.memoryClockDomain)
            {
                GPU_DEBUG(
                    printf("Memory Domain. Cycle %lld ----------------------------\n", memoryCycle);
                )
                
                //  Clock boxes with multiple domains.
                for(i = 0; i < GpuCMdl.MemoryDomainMduArray.size(); i++)
                    GpuCMdl.MemoryDomainMduArray[i]->clock(MEMORY_CLOCK_DOMAIN, memoryCycle);

                //  Update memory domain clock and step counter.
                memoryCycle++;
            }
            
            nextMemoryClock = memoryClockPeriod;            
        }
    }
    
    //GPU_DEBUG(
        printf("\n");
        printf("GPU Domain. END Cycle %lld ----------------------------\n", gpuCycle);
        if (GpuCMdl.shaderClockDomain)
            printf("Shader Domain. END Cycle %lld ----------------------------\n", shaderCycle);
        if (GpuCMdl.memoryClockDomain)
            printf("Memory Domain. END Cycle %lld ----------------------------\n", memoryCycle);
        printf("\n");
    //)

    //
    //  WARNING:  Signal trace dump not supported with multiple clock domains.
    //
    
    //  Check signal trace dump enabled.  */
    //if (ArchConf.sim.dumpSignalTrace)
    //{
        //  End signal tracing.  */
    //    sigBinder.endSignalTrace();
    //}

    DynamicMemoryOpt::usage();
    gpuStatistics::StatisticsManager::instance().finish();

    //DynamicMemoryOpt::dumpDynamicMemoryState(FALSE, FALSE);
}

void CG1CMDL::getCounters(U32 &frame, U32 &batch, U32 &totalBatches)
{
    frame = frameCounter;
    batch = frameBatch;
    totalBatches = batchCounter;
}

void CG1CMDL::getCycles(U64 &_gpuCycle, U64 &_shaderCycle, U64 &_memCycle)
{
    if (GpuCMdl.multiClock)
    {
        _gpuCycle = gpuCycle;
        if (GpuCMdl.shaderClockDomain)
            _shaderCycle = shaderCycle;
        else
            _shaderCycle = 0;
        if (GpuCMdl.memoryClockDomain)
            _memCycle = memoryCycle;
        else
            _memCycle = 0;
    }
    else
    {
        _gpuCycle = cycle;
        _shaderCycle = 0;
        _memCycle = 0;
    }
}

void CG1CMDL::abortSimulation()
{
    abortDebug = true;
}


//  Dumps the latency maps.  
void CG1CMDL::dumpLatencyMap(U32 w, U32 h)
{
    FILE *fout;
    char filename[30];
    S32 x, y;
    U32 stampLatency;
    U32 i;

    //  Create current frame filename.  
    sprintf(filename, "latencyMap%04d.ppm", frameCounter);

    //  Open/Create the file for the current frame.  
    fout = fopen(filename, "wb");

    //  Check if the file was correctly created.  
    CG_ASSERT_COND(!(fout == NULL), "Error creating latency map output file.");
    //  Write file header.  

    //  Write magic number.  
    fprintf(fout, "P6\n");

    //  Write latency map size.  
    fprintf(fout, "%d %d\n", w, h);

    //  Write color component maximum value.  
    fprintf(fout, "255\n");

    // Do this for the whole picture now 
    for (y = h - 1; y >= 0; y--)
    {
        for (x = 0; x < S32(w); x++)
        {
            /*  Calculate the latency for the stamp.  Does a join of the
                latency maps per color write unit (stamp pipe).  A stamp should
                have latency > 0 only for one of the color write units.  */
            for(i = 0, stampLatency = 0; i < ArchConf.gpu.numStampUnits; i++)
            {
                if ((stampLatency != 0) && (latencyMap[i][y * w + x] != 0))
                    printf("stamp written by two units!!! at %d %d => ", x, y);
                stampLatency += latencyMap[i][y * w + x];
            }

            //stampLatency = applyColorKey(stampLatency);

            fputc(char(stampLatency & 0xff), fout);
            fputc(char((stampLatency >> 8) & 0xff), fout);
            fputc(char((stampLatency >> 16) & 0xff), fout);
        }
    }

    fclose(fout);
}

U32 CG1CMDL::applyColorKey(U32 p)
{
    if (p == 0)
        return 0x00000000;
    if ((p > 0) && (p <= 50))
        return 0x00000020;
    if ((p > 50) && (p <= 100))
        return 0x00000040;
    if ((p > 100) && (p <= 150))
        return 0x00000080;
    if ((p > 150) && (p <= 200))
        return 0x000000ff;
    if ((p > 200) && (p <= 300))
        return 0x00002000;
    if ((p > 300) && (p <= 400))
        return 0x00004000;
    if ((p > 400) && (p <= 500))
        return 0x00008000;
    if ((p > 500) && (p <= 600))
        return 0x0000ff00;
    if ((p > 600) && (p <= 800))
        return 0x00200000;
    if ((p > 800) && (p <= 1000))
        return 0x00400000;
    if ((p > 1000) && (p <= 1200))
        return 0x00800000;
    if ((p > 1200) && (p <= 1400))
        return 0x00ff0000;
    if ((p > 1400) && (p <= 1800))
        return 0x00ff0020;
    if ((p > 1800) && (p <= 2200))
        return 0x00ff0040;
    if ((p > 2200) && (p <= 2600))
        return 0x00ff0080;
    if ((p > 2600) && (p <= 3000))
        return 0x00ff00ff;
    if ((p > 3000) && (p <= 3800))
        return 0x00ff2000;
    if ((p > 3800) && (p <= 4600))
        return 0x00ff4000;
    if ((p > 4600) && (p <= 5400))
        return 0x00ff8000;
    if ((p > 5400) && (p <= 6200))
        return 0x00ffff00;
    if ((p > 6200) && (p <= 0xffffffff))
        return 0x00ffffff;

    return 0x00ffffff;
}

}   // namespace cg1gpu

