/**************************************************************************
 *
 * cmoStreamController Commit mdu definition file.
 *
 */

/**
 *
 *  @file cmStreamerCommit.h
 *
 *  This file defines the cmoStreamController Commit mdu.
 *
 */


#ifndef _STREAMERCOMMIT_

#define _STREAMERCOMMIT_

namespace cg1gpu
{
    class StreamerCommit;
} // namespace cg1gpu


#include "GPUType.h"
#include "cmMduBase.h"
#include "cmStreamer.h"
#include "cmStreamerCommand.h"
#include "cmStreamerControlCommand.h"

namespace cg1gpu
{

//*  Latency signal of the cmoStreamController - Primitive Assembly bus.  
//#define PRIMITIVE_ASSEMBLY_BASE_LATENCY 2

//*  Transmission latency for sending a single vertex attribute to primitive assembly.  
//#define ATTRIBUTE_LATENCY 0.5

/**
 *
 *  This struct defines an output FIFO entry.
 *
 */

struct OFIFO
{
    U32 index;       //  The index stored in this output FIFO entry.  
    U32 instance;    //  The instance for the stored index.  
    U32 omLine;      //  Output Memory line where the output for the index is stored.  
    StreamerControlCommand *stCCom; //  Pointer to the new index StreamController control command that carried the index.  
    bool isShaded;                  //  If the index (output) has been shaded.  
    U64 startCycle;              //  Simulation cycle that entry was added to the Output FIFO.  
    U64 shadingLatency;          //  Shading latency of the output FIFO entry.  
};

/**
 *
 *  This class implements the simulator mdu for the cmoStreamController Commit unit.
 *
 *  The cmoStreamController Commit mdu receives new index allocations from
 *  the cmoStreamController Output Cache and outputs from the Shader.  Then
 *  sends the outputs to the Rasterizer in the order in which
 *  the indexes arrived.  It also manages when an output memory
 *  line can be deallocated.
 *
 *  This class inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class StreamerCommit : public cmoMduBase
{

private:

    //  cmoStreamController Commit signals.  
    Signal **shOutputSignal;            //  Pointer to an array of shader output signals.  
    Signal **shConsumerSignal;          //  Pointer to an array of consumer state (StreamController) signals to the Shaders.  
    Signal *streamerCommitCommand;      //  Command signal from the cmoStreamController main mdu.  
    Signal *streamerCommitState;        //  State signal to the cmoStreamController main mdu.  
    Signal *streamerCommitNewIndex;     //  New index signal from the cmoStreamController Output cache.  
    Signal *streamerCommitDeAlloc;      //  Deallocation signal to the cmoStreamController Fetch.  
    Signal *streamerCommitDeAllocOC;    //  Deallocation signal to the cmoStreamController Output cache.  
    Signal *assemblyOutputSignal;       //  Output signal to the Primitive Assembly unit.  
    Signal *assemblyRequestSignal;      //  Request signal from the Primitive Assembly unit.  

    //  cmoStreamController Commit parameters.  
    U32 indicesCycle;        //  Number of indices from cmoStreamController Output Cache per cycle.  
    U32 outputFIFOSize;      //  Size (in entries) of the output FIFO.  
    U32 outputMemorySize;    //  Size (lines/outputs) of the output memory size.  
    U32 numShaders;          //  Number of shaders that attached to the cmoStreamController.  
    U32 shMaxOutLat;         //  Maximum latency of the output signal from the Shader.  
    U32 verticesCycle;       //  Number of vertices per cycle to send to Primitive Assembly.  
    U32 attributesCycle;     //  Vertex attributes that can be send to Primitive Assembly per cycle.  
    U32 outputBusLat;        //  Latency of the vertex bus to Primitive Assembly.  

    //  cmoStreamController Commit state variables.  
    bool outputAttribute[MAX_VERTEX_ATTRIBUTES];//  Stores if a vertex output attribute is active/written.  
    U32 activeOutputs;                       //  Number of vertex output attributes active.  
    U32 streamCount;                         //  Number of index/input/outputs for the current batch.  
    U32 streamInstances;                     //  Number of instances of the current stream to process.  
    StreamerState state;                        //  The current cmoStreamController state.  
    StreamerCommand *lastStreamCom;             //  Pointer to the last StreamController command received from the cmoStreamController main mdu.  
    bool lastOutputSent;                        //  Whether the last index/vertex in the batch was sent to primitive assembly this cycle. 
    bool firstOutput;                           //  Whether the first index of a new batch has been sent by StreamerOutputCache this cycle.  

    //  Output Memory Variables.  
    Vec4FP32 **outputMemory;   //  Output memory.  
    U32 *lastUseTable;       //  Last use table for the output memory lines.  

    //  Output FIFO variables.  
    OFIFO *outputFIFO;          //  The cmoStreamController Output FIFO.  
    U32 freeOFIFOEntries;    //  Number of free output FIFO entries.  
    U32 nextOutput;          //  Pointer to the next output FIFO to send to the rasterizer.  

    //  Deallocated Output Memory Lines structures.  
    U32 **outputMemoryDeAlloc;                     //  Array storing those deallocated indices in the last cycle that also deallocated its memory line.  
    StreamerControlCommand** outputMemoryDeAllocCCom; //  Array storing the StreamController control commands of the above deallocated indices.  
    U32 outputMemoryDeAllocs;                      //  Number of above deallocated indices the last cycle.  
    bool *outputMemoryDeAllocHit;                     //  Stores if above deallocated indices match with any new index of the next cycle.   

    //  cmoStreamController primitive assembly state.  
    U32 sentOutputs;         //  Number of outputs already sent to the Primitive Assembly unit.  
    U32 requestVertex;       //  Number of vertices requested by the Primitive Assembly unit.  

    //  Data transmission variables.  
    U32 *transCycles;        //  Array of counter storing the cycles remaining for the transmission of the current vertices to Primitive Assembly.  

    //  Statistics.  
    gpuStatistics::Statistic *indices;        //  Indices received from cmoStreamController Output Cache.  
    gpuStatistics::Statistic *shOutputs;      //  Number of outputs received from the Shaders.  
    gpuStatistics::Statistic *paOutputs;      //  Number of ouputs sent to Primitive Assembly.  
    gpuStatistics::Statistic *paRequests;     //  Number of outputs requested by Primitive Assembly.  
    gpuStatistics::Statistic *outAttributes;  //  Number of active output attributes per vertex.  
    gpuStatistics::Statistic *avgVtxShadeLat; //  Average shading latency of vertices in the Output FIFO.  

    //  Debug/validation.
    bool validationMode;                //  Flag that stores if the validation mode is enabled.  
    U32 nextReadLog;                 //  Pointer to the next shaded vertex log to read from.  
    U32 nextWriteLog;                //  Pointer to the next shaded vertex log to write to.  
    ShadedVertexMap shadedVertexLog[4]; //  Map, indexed by vertex index, of the shaded vertices in the current draw call.  

    //  Private functions.  

    /**
     *
     *  Processes a cmoStreamController Command.
     *
     *  @param streamCom The cmoStreamController Command to process.
     *
     */

    void processStreamerCommand(StreamerCommand *streamCom);

    /**
     *
     *  Processes a cmoStreamController register write.
     *
     *  @param gpuReg cmoStreamController register to write to.
     *  @param gpuSubReg cmoStreamController register subregister to write to.
     *  @param gpuData Data to write to the cmoStreamController register.
     *
     */

    void processGPURegisterWrite(GPURegister gpuReg, U32 gpuSubReg,
        GPURegData gpuData);



public:

    /**
     *
     *  cmoStreamController Commit contructor.
     *
     *  Creates and initializes a new cmoStreamController Commit mdu.
     *
     *  @param idxCycle Indices received from cmoStreamController Output Cache per cycle.
     *  @param oFIFOSize Output FIFO Size (entries).
     *  @param omSize Size of the output memory in lines (outputs).
     *  @param numShaders Number of Shaders attached to the cmoStreamController.
     *  @param maxOutLat Maximum latency of the shader output signal.
     *  @param vertCycle Vertices to send to Primite Assembly per cycle.
     *  @param attrCycle Number of attributes that can be sent per cycle to Primitive Assembly.
     *  @param outLat Latency of the vertex bus with Primitive Assembly.
     *  @param shPrefixArray Array of prefixes for the shader signals.
     *  @param name  The mdu name.
     *  @param parent The mdu parent mdu.
     *
     *  @return An initialized cmoStreamController Commit mdu.
     *
     */

    StreamerCommit(U32 idxCycle, U32 oFIFOSize, U32 omSize, U32 numShaders, U32 maxOutLat,
        U32 vertCycle, U32 attrCycle, U32 outLat, char **shPrefixArray, char *name, cmoMduBase *parent);

    /**
     *
     *  cmoStreamController Commit simulation rutine.
     *
     *  Simulates a cycle of the cmoStreamController Commit unit.
     *
     *  @param cycle The cycle to simulate of the cmoStreamController
     *  Commit unit.
     *
     */

    void clock(U64 cycle);

    /** 
     *
     *  Set cmoStreamController Commit validation mode.
     *
     *  @param enable Boolean value that defines if the validation mode is enabled.
     *
     */
     
    void setValidationMode(bool enable);
    
    /**
     *
     *  Get a map indexed per vertex index with the information about the vertices shaded in the current batch.
     *
     *  @return A reference to a map indexed per vertex index with the information about the vertices
     *  shaded in the current batch.
     *
     */
     
    ShadedVertexMap &getShadedVertexInfo();


};

} // namespace cg1gpu

#endif

