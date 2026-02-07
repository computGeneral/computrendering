/**************************************************************************
 * GPU simulator definition file.
 * This file contains definitions and includes for the CG1 GPU simulator.
 *
 */

#ifndef __CG1_CMDL_H__

#define __CG1_CMDL_H__
#include "CG1MDLBASE.h"

#include "ConfigLoader.h"
#include "TraceDriverBase.h"
#include "GPUReg.h"

#include "cmStatisticsManager.h"
#include "cmSignalBinder.h"
#include "DynamicMemoryOpt.h"

#include "cmGpuTop.h"

#include "zfstream.h"

#include <vector>

namespace cg1gpu
{

/**
 *  GPU Simulator class.
 *  The class defines structures and attributes to implement a GPU simulator.
 *
 */

class CG1CMDL : public CG1TIMEMDLBASE
{
private:
    cgsArchConfig ArchConf;            //  Structure storing the simulator configuration parameters.  */
    cmoGpuTop   GpuCMdl;
    //  Trace reader+driver.
    cgoTraceDriverBase *TraceDriver;  //  Pointer to a cgoTraceDriverBase object that will provide the MetaStreams to simulate.  */
    cmoSignalBinder &sigBinder;      //  Reference to the cmoSignalBinder that tracks all signals between and in the simulator modules.  */

    U64 cycle;              //  Cycle counter for GPU clock domain (unified clock architecture).  */
    U64 gpuCycle;           //  Cycle counter for GPU clock domain.  */
    U64 shaderCycle;        //  Cycle counter for shader clock domain.  */
    U64 memoryCycle;        //  Cycle counter for memory clock domain.  */
    U32 nextGPUClock;       //  Stores the picoseconds (ps) to next gpu clock.  */
    U32 nextShaderClock;    //  Stores the picoseconds (ps) to next shader clock.  */
    U32 nextMemoryClock;    //  Stores the picoseconds (ps) to next memory clock.  */

    U32 gpuClockPeriod;     //  Period of the GPU clock in picoseconds (ps).  */
    U32 shaderClockPeriod;  //  Period of the shader clock in picoseconds (ps).  */
    U32 memoryClockPeriod;  //  Period of the memory clock in picoseconds (ps).  */

    bool simulationStarted;    //  Flag that stores if the simulation was started.  */
    U32 dotCount;           //  Progress dot displayer counter.  */
    U32 frameCounter;       //  Counts the frames (swap commands).  */
    U32 batchCounter;       //  Stores the number of batches rendered.  */
    U32 frameBatch;         //  Stores the batches rendered in the current batch.  */
    U32 **latencyMap;       //  Arrays for the signal maps.  */
    bool abortDebug;           //  Flag that stores if the abort signal has been received.  */
    bool endDebug;             //  Flag that stores if the simulator debugger must end.  */

    gpuStatistics::Statistic *cyclesCounter;   //  Pointer to GPU statistic used to count the number of simulated cycles (main clock domain!).  */

    gzofstream outCycle;            //  Compressed stream output file for statistics.  */
    gzofstream outFrame;       //  Compressed stream output file for per frame statistics.  */
    gzofstream outBatch;       //  Compressed stream output file for per batch statistics.  */
    gzofstream sigTraceFile;   //  Compressed stream output file for signal dump trace.  */


    static CG1CMDL* current;      //  Stores the pointer to the currently executing GPU simulator instance.  */
    
    bool pendingSaveSnapshot;  //  Flag that stores if there is pending snapshot in process. */
    bool autoSnapshotEnable;   //  Flag that stores if auto snapshot is enabled.  */
    U32 snapshotFrequency;  //  Stores the snapshot frequency in minutes.  */
    U64 startTime;          //  Stores the time since the previous snapshot.  */
    
    //  Debug/Validation.
    bool validationMode;       //  Stores if the validation mode is enabled.  */
    bool skipValidation;       //  Used to skip validation when loading a snapshot.  */
    CG1BMDL *GpuBehavMdl;  //  Pointer to the associated GPU behaviorModel for validation purposes.  */
    
    /**
     *  Saves the simulator state to the 'state.snapshot' file.
     *  The simulator state includes current frame and draw call, current simulation cycle(s),
     *  trace information and current trace position.
     */
    void saveSimState();
    
    /**
     *  Saves the simulator configuration parameters (cgsArchConfig structure) to the 'config.snapshot' file.
     */
    void saveSimConfig();
    
public:

    /**
     *
     *  CG1CMDL constructor.
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
    CG1CMDL(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver);

    /**
     *  CG1CMDL destructor.
     */
    ~CG1CMDL();
    
    /**
     *  Fire-and-forget simulation loop for a single clock domain architecture.
     */
    void simulationLoop(cgeModelAbstractLevel MAL = CG_FUNC_MODEL);

    /**
     *  Fire-and-forget simulation loop for a multi-clock domain architecture.
     */
    void simulationLoopMultiClock();            

    /**
     *  Fire-and-forget simulation loop with integrated debugger.
     */
    void debugLoop(bool validate);

    /**
     *  Implements the 'debugmode' command of the GPU simulator integrated debugger.
     *  The 'debugmode' command is used to enable or disable verbose output for a specific simulator mdu.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void debugModeCommand(stringstream &streamCom);

    /**
     *  Implements the 'listboxes' command of the GPU simulator integrated debugger.
     *  The 'listboxes' command is used to output a list the modules instantiated for the simulated GPU architecture.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void listMdusCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'listcommands' command of the GPU simulator integrated debugger.
     *  The 'listcommands' command is used to output a list of debug commands supported by a given simulator mdu.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void listMduCmdCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'execcommand' command of the GPU simulator integrated debugger.
     *  The 'execcommand' command is used to execute a command in the defined simulator mdu.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void execMduCmdCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'help' command of the GPU simulator integrated debugger.
     *  The 'help' command outputs a list of all the command supported by the integrated simulator debugger.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void helpCommand(stringstream &streamCom);

    /**
     *  Implements the 'run' command of the GPU simulator integrated debugger.
     *  The 'run' command simulates the defined number of cycles (main clock).
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void runCommand(stringstream &streamCom);

    /**
     *  Implements the 'runframe' command of the GPU simulator integrated debugger.
     *  The 'run' command simulates the defined number of frames.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void runFrameCommand(stringstream &streamCom);

    /**
     *  Implements the 'runbatch' command of the GPU simulator integrated debugger.
     *  The 'runbatch' command simulates the defined number of batches (draw calls).
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void runBatchCommand(stringstream &streamCom);

    /**
     *  Implements the 'runcom' command of the GPU simulator integrated debugger.
     *  The 'runcom' command simulated the defined number of MetaStream commands.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void runComCommand(stringstream &streamCom);

    /**
     *  Implements the 'skipframe' command of the GPU simulator integrated debugger.
     *  The 'skipframe' command skips (only library state is updated) the defined number of frames from the input trace.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void skipFrameCommand(stringstream &streamCom);

    /**
     *  Implements the 'skipbatch' command of the GPU simulator integrated debugger.
     *  The 'skipbatch' command skips (only library state is updated) the defined number of batches (draw calls) from the input trace.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void skipBatchCommand(stringstream &streamCom);

    /**
     *  Implements the 'state' command of the GPU simulator integrated debugger.
     *  The 'state' command outputs information about the state of the simulator or a specific simulator mdu.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void stateCommand(stringstream &streamCom);

    /**
     *  Implements the 'savesnapshot' command of the GPU simulator integrated debugger.
     *  The 'savesnapshot' command saves a snapshot of the current simulator state (memory, caches, registers, trace) into
     *  a newly created snapshot directory.
     */
    void saveSnapshotCommand();

    /**
     *  Implements the 'loadsnapshot' command of the GPU simulator integrated debugger.
     *  The 'loadsnapshot' command loads a snapshot with simulator state (memory, caches, registers, trace) into
     *  from the defined snapshot directory.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void loadSnapshotCommand(stringstream &streamCom);

    /**
     *  Implements the 'autosnapshot' command of the GPU simulator integrated debugger.
     *  The 'autosnapshot' command sets to simulator to automatically save every n minutes a snapshot of the current
     *  simulator state (memory, caches, registers, trace) into a newly created snapshot directory.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void autoSnapshotCommand(stringstream &streamCom);

    //  Debug commands associated with the GPU Driver
    /**
     *  Implements the 'memoryUsage' command of the GPU simulator integrated debugger.
     *  The 'memoryUsage' outputs information about GPU memory utilization from the GPU driver.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void driverMemoryUsageCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'listMDs' command of the GPU simulator integrated debugger.
     *  The 'listMDs' outputs the list of defined memory descriptors from the GPU driver.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void driverListMDsCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'infoMD' command of the GPU simulator integrated debugger.
     *  The 'infoMD' outputs information about a memory descriptor from the GPU driver.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void driverInfoMDCommand(stringstream &streamCom);

    /**
     *  Implements the 'dumpMetaStreamBuffer' command of the GPU simulator integrated debugger.
     *  The 'dumpMetaStreamBuffer' outputs the current content of the MetaStream buffer in the GPU driver.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     *
     */
    void driverDumpMetaStreamBufferCommand(stringstream &streamCom);
    
    /**
     *  Implements the 'memoryAlloc' command of the GPU simulator integrated debugger.
     *  The 'memoryAlloc' outputs information about the GPU memory allocation from the GPU driver.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void driverMemAllocCommand(stringstream &streamCom);

    /**
     *  Implements the 'glcontext' command of the GPU simulator integrated debugger.
     *  The 'glcontext' outputs information from the legacy OpenGL library (deprecated).
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    //void libraryGLContextCommand(stringstream &lineStream);
    
    /**
     *  Implements the 'dumpStencil' command of the GPU simulator integrated debugger.
     *  The 'dumpStencil' forces the legacy OpenGL library to dump the content of the stencil buffer (deprecated).
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    //void libraryDumpStencilCommand(stringstream &lineStream);

    /**
     *  In the integrated simulator debugger runs the simulation until the execution of a command forced from
     *  the debugger into the simulated GPU finishes.
     *  @return If the simulation of the input trace has finished.
     */
    bool simForcedCommand();

    /**
     *  Implements the 'emulatortrace' command of the GPU simulator integrated debugger.
     *  The 'emulatortrace' command is used to enable/disable the trace log from the GPU behaviorModel
     *  instantiated for the validation mode.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void emulatorTraceCommand(stringstream &lineStream);

    /**
     *  Implements the 'tracevertex' command of the GPU simulator integrated debugger.
     *  The 'tracevertex' command is used to enable/disable the trace of the execution of a defined
     *  vertex in the Shader Processors.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void traceVertexCommand(stringstream &lineStream);

    /**
     *  Implements the 'tracefragment' command of the GPU simulator integrated debugger.
     *  The 'tracefragment' command is used to enable/disable the trace of the execution of a defined
     *  fragment in the Shader Processors.
     *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
     */
    void traceFragmentCommand(stringstream &lineStream);

    /**
     *  Simulates at least one clock of the main GPU clock before returning.
     *  Used by the integrated GPU simulator debugger run commands.
     *  @param endOfBatch Reference to a boolean variable where to store if a draw call finished.
     *  @param endOfFrame Reference to a boolean variable where to store if a frame finished.
     *  @param endOfTrace Reference to a boolean variable where to store if the trace finished.
     *  @param gpuStalled Reference to a boolean variable where to store if a stall was detected.
     *  @param validationError Reference to a boolean variable where to store if a validation error was detected.
     */
    void advanceTime(bool &endOfBatch, bool &endOfFrame, bool &endOfTrace, bool &gpuStalled, bool &validationError);

    /**
     *  Creates a snapshot of the simulated state.
     *  Creates a new directory for the snapshot.  Saves simulator state and configuration to files.
     *  Saves GPU registers to a file.  Saves the Hierarchical Z buffer to a file.  Saves the z/stencil and
     *  color cache block state buffer to files.  Saves GPU and system memory to files.
     */
    static void createSnapshot();
    
    /**
     *  Get current frame and draw call counters from the simulator.
     *  @param frameCounter Reference to an integer variable where to store the current frame counter.
     *  @param frameBatch Reference to an integer variable where to store the current frame batch counter.
     *  @param batchCounter Reference to an integer variable where to store the current batch counter.
     */
    void getCounters(U32 &frameCounter, U32 &frameBatch, U32 &batchCounter);
    
    /**
     *  Get the current simulation cycles.
     *  @param gpuCycle Reference to an integer variable where to store the current GPU main clock cycle.
     *  @param shaderCycle Reference to an integer variable where to store the current shader domain clock cycle.
     *  @param memCycle Reference to an integer variable where to store the current memory domain clock cycle.
     */
     
    void getCycles(U64 &gpuCycle, U64 &shaderCycle, U64 &memCycle);

    /**
     *  Aborts the simulation.
     */
    void abortSimulation();
    
    /**
     *  Dumps the content of the quad/latency map obtained from the ColorWrite modules as a PPM image file.
     *  @param width Width of the quad/latency map in pixels (1 pixel -> 2x2 pixels in the framebuffer).
     *  @param width Height of the quad/latency map in pixels (1 pixel -> 2x2 pixels in the framebuffer).
     */
    void dumpLatencyMap(U32 width, U32 height);
    
    /**
     *  Converts a value to a color.
     *  @param p Value to convert.
     *  @return A 32-bit RGBA color.
     */
    U32 applyColorKey(U32 p);
};  // class CG1CMDL
};  //  namespace cg1gpu

#endif
