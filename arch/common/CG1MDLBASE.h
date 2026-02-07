/**************************************************************************
 * GPU simulator definition file.
 * This file contains definitions and includes for the CG1 GPU simulator.
 *
 */

#ifndef __CG1MDLBASE_H__
#define __CG1MDLBASE_H__
#include <vector>
#include "zfstream.h"
#include "GPUType.h"
#include "GPUReg.h"

namespace cg1gpu
{

    class CG1MDLBASE
    {
    public:
        CG1MDLBASE() {};
        virtual ~CG1MDLBASE() {};
        /**
         *  Fire-and-forget simulation loop for a single clock domain architecture.
         */
        virtual void  simulationLoop(cgeModelAbstractLevel MAL = CG_BEHV_MODEL) = 0;
        /**
         *  Fire-and-forget simulation loop for a multi-clock domain architecture.
         */
        virtual void  simulationLoopMultiClock() = 0;            
        /**
         *  Aborts the simulation.
         */
        virtual void  abortSimulation() = 0;
        /**
         *  Fire-and-forget simulation loop with integrated debugger.
         */
        virtual void  debugLoop(bool validate) = 0;
        /**
         *  Get current frame and draw call counters from the simulator.
         *  @param frameCounter Reference to an integer variable where to store the current frame counter.
         *  @param frameBatch Reference to an integer variable where to store the current frame batch counter.
         *  @param batchCounter Reference to an integer variable where to store the current batch counter.
         */
        virtual void  getCounters(U32& frameCounter, U32& frameBatch, U32& batchCounter) = 0;
        /**
         *  Get the current simulation cycles.
         *  @param gpuCycle Reference to an integer variable where to store the current GPU main clock cycle.
         *  @param shaderCycle Reference to an integer variable where to store the current shader domain clock cycle.
         *  @param memCycle Reference to an integer variable where to store the current memory domain clock cycle.
         */
        //TODO for draft code sharing of CG1GpuSim.cpp 
        virtual void  getCycles(U64 &gpuCycle, U64 &shaderCycle, U64 &memCycle) = 0;
        virtual void  createSnapshot() {};

    };

    class CG1TIMEMDLBASE : public CG1MDLBASE
    {
    private:
        /**
         *  Saves the simulator state to the 'state.snapshot' file.
         *  The simulator state includes current frame and draw call, current simulation cycle(s),
         *  trace information and current trace position.
         */
        virtual void  saveSimState() = 0;
        
        /**
         *  Saves the simulator configuration parameters (cgsArchConfig structure) to the 'config.snapshot' file.
         */
        virtual void  saveSimConfig() = 0;
        
    public:
    
        /**
         *
         *  CG1MDLBASE constructor.
         *  Creates and initializes the objects and state required to simulate a GPU.
         *
         *  The emulation objects and and simulation modules are created.
         *  The statistics and signal dump state and files are initialized and created if the simulation
         *  as required by the simulator parameters.
         *
         *  @param arch Configuration parameters.
         *  @param TraceDriver Pointer to the driver that provides MetaStreams for simulation.
         *  @param unified  Boolean value that defines if the simulated architecture implements unified shaders.
         *  @param d3dTrace Boolean value that defines if the input trace is a D3D9 API trace (PIX).
         *  @param oglTrace Boolean value that defines if the input trace is an OpenGL API trace.
         *  @param MetaTrace Boolean value that defines if the input trace is an MetaStream trace.
         *
         */
        //CG1MDLBASE(cgsArchConfig arch, cgoTraceDriverBase *TraceDriver);
        CG1TIMEMDLBASE() {};
    
        /**
         *  CG1MDLBASE destructor.
         */
        ~CG1TIMEMDLBASE() {};
        
    
        /**
         *  Fire-and-forget simulation loop for a multi-clock domain architecture.
         */
        virtual void  simulationLoopMultiClock() = 0;            
    
    
        /**
         *  Implements the 'debugmode' command of the GPU simulator integrated debugger.
         *  The 'debugmode' command is used to enable or disable verbose output for a specific simulator mdu.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  debugModeCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'listboxes' command of the GPU simulator integrated debugger.
         *  The 'listboxes' command is used to output a list the modules instantiated for the simulated GPU architecture.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  listMdusCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'listcommands' command of the GPU simulator integrated debugger.
         *  The 'listcommands' command is used to output a list of debug commands supported by a given simulator mdu.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  listMduCmdCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'execcommand' command of the GPU simulator integrated debugger.
         *  The 'execcommand' command is used to execute a command in the defined simulator mdu.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  execMduCmdCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'help' command of the GPU simulator integrated debugger.
         *  The 'help' command outputs a list of all the command supported by the integrated simulator debugger.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  helpCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'run' command of the GPU simulator integrated debugger.
         *  The 'run' command simulates the defined number of cycles (main clock).
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  runCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'runframe' command of the GPU simulator integrated debugger.
         *  The 'run' command simulates the defined number of frames.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  runFrameCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'runbatch' command of the GPU simulator integrated debugger.
         *  The 'runbatch' command simulates the defined number of batches (draw calls).
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  runBatchCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'runcom' command of the GPU simulator integrated debugger.
         *  The 'runcom' command simulated the defined number of MetaStream commands.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  runComCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'skipframe' command of the GPU simulator integrated debugger.
         *  The 'skipframe' command skips (only library state is updated) the defined number of frames from the input trace.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  skipFrameCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'skipbatch' command of the GPU simulator integrated debugger.
         *  The 'skipbatch' command skips (only library state is updated) the defined number of batches (draw calls) from the input trace.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  skipBatchCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'state' command of the GPU simulator integrated debugger.
         *  The 'state' command outputs information about the state of the simulator or a specific simulator mdu.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  stateCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'savesnapshot' command of the GPU simulator integrated debugger.
         *  The 'savesnapshot' command saves a snapshot of the current simulator state (memory, caches, registers, trace) into
         *  a newly created snapshot directory.
         */
        virtual void  saveSnapshotCommand() = 0;
    
        /**
         *  Implements the 'loadsnapshot' command of the GPU simulator integrated debugger.
         *  The 'loadsnapshot' command loads a snapshot with simulator state (memory, caches, registers, trace) into
         *  from the defined snapshot directory.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  loadSnapshotCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'autosnapshot' command of the GPU simulator integrated debugger.
         *  The 'autosnapshot' command sets to simulator to automatically save every n minutes a snapshot of the current
         *  simulator state (memory, caches, registers, trace) into a newly created snapshot directory.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  autoSnapshotCommand(stringstream &streamCom) = 0;
    
        //  Debug commands associated with the GPU Driver
        /**
         *  Implements the 'memoryUsage' command of the GPU simulator integrated debugger.
         *  The 'memoryUsage' outputs information about GPU memory utilization from the GPU driver.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  driverMemoryUsageCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'listMDs' command of the GPU simulator integrated debugger.
         *  The 'listMDs' outputs the list of defined memory descriptors from the GPU driver.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  driverListMDsCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'infoMD' command of the GPU simulator integrated debugger.
         *  The 'infoMD' outputs information about a memory descriptor from the GPU driver.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  driverInfoMDCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'dumpMetaStreamBuffer' command of the GPU simulator integrated debugger.
         *  The 'dumpMetaStreamBuffer' outputs the current content of the MetaStream buffer in the GPU driver.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         *
         */
        virtual void  driverDumpMetaStreamBufferCommand(stringstream &streamCom) = 0;
        
        /**
         *  Implements the 'memoryAlloc' command of the GPU simulator integrated debugger.
         *  The 'memoryAlloc' outputs information about the GPU memory allocation from the GPU driver.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  driverMemAllocCommand(stringstream &streamCom) = 0;
    
        /**
         *  Implements the 'glcontext' command of the GPU simulator integrated debugger.
         *  The 'glcontext' outputs information from the legacy OpenGL library (deprecated).
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        //virtual void  libraryGLContextCommand(stringstream &lineStream) = 0;
        
        /**
         *  Implements the 'dumpStencil' command of the GPU simulator integrated debugger.
         *  The 'dumpStencil' forces the legacy OpenGL library to dump the content of the stencil buffer (deprecated).
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        //virtual void  libraryDumpStencilCommand(stringstream &lineStream) = 0;
    
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
        virtual void  emulatorTraceCommand(stringstream &lineStream) = 0;
    
        /**
         *  Implements the 'tracevertex' command of the GPU simulator integrated debugger.
         *  The 'tracevertex' command is used to enable/disable the trace of the execution of a defined
         *  vertex in the Shader Processors.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  traceVertexCommand(stringstream &lineStream) = 0;
    
        /**
         *  Implements the 'tracefragment' command of the GPU simulator integrated debugger.
         *  The 'tracefragment' command is used to enable/disable the trace of the execution of a defined
         *  fragment in the Shader Processors.
         *  @param streamCom A reference to a stringstream object storing the line with the debug command and parameters.
         */
        virtual void  traceFragmentCommand(stringstream &lineStream) = 0;
    
        /**
         *  Simulates at least one clock of the main GPU clock before returning.
         *  Used by the integrated GPU simulator debugger run commands.
         *  @param endOfBatch Reference to a boolean variable where to store if a draw call finished.
         *  @param endOfFrame Reference to a boolean variable where to store if a frame finished.
         *  @param endOfTrace Reference to a boolean variable where to store if the trace finished.
         *  @param gpuStalled Reference to a boolean variable where to store if a stall was detected.
         *  @param validationError Reference to a boolean variable where to store if a validation error was detected.
         */
        virtual void  advanceTime(bool &endOfBatch, bool &endOfFrame, bool &endOfTrace, bool &gpuStalled, bool &validationError) = 0;
    
        /**
         *  Creates a snapshot of the simulated state.
         *  Creates a new directory for the snapshot.  Saves simulator state and configuration to files.
         *  Saves GPU registers to a file.  Saves the Hierarchical Z buffer to a file.  Saves the z/stencil and
         *  color cache block state buffer to files.  Saves GPU and system memory to files.
         */
        //static void  createSnapshot() {};
        
        
    
        /**
         *  Dumps the content of the quad/latency map obtained from the ColorWrite modules as a PPM image file.
         *  @param width Width of the quad/latency map in pixels (1 pixel -> 2x2 pixels in the framebuffer).
         *  @param width Height of the quad/latency map in pixels (1 pixel -> 2x2 pixels in the framebuffer).
         */
        virtual void  dumpLatencyMap(U32 width, U32 height) = 0;
        
        /**
         *  Converts a value to a color.
         *  @param p Value to convert.
         *  @return A 32-bit RGBA color.
         */
        virtual U32 applyColorKey(U32 p) = 0;
    };  // class CG1MDLBASE

};  //  namespace cg1gpu

#endif
