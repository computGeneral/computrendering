/**************************************************************************
 *
 * cmoStreamController mdu definition file.
 *
 */

#ifndef _STREAMER_

#define _STREAMER_

#include "support.h"
#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmStreamerCommand.h"
#include "cmShaderInput.h"
#include "cmShaderFetch.h"
#include "ValidationInfo.h"

#include <map>

namespace arch
{

//**  Unassigned output cache tag identifier.  
static const U32 UNASSIGNED_OUTPUT_TAG = 0xffffffff;

//**  This describes the StreamController state/mode.  
enum StreamerState
{
    ST_RESET,           //  cmoStreamController is in reset mode.  
    ST_READY,           //  cmoStreamController can receive new commands and state changes.  
    ST_STREAMING,       //  cmoStreamController is streaming data to the vertex shaders.  
    ST_WAITING,         //  cmoStreamController is waiting for memory.  
    ST_FINISHED         //  cmStreamer.has finished streaming data to the vertex shaders.  
};


//**  Table with the size in bytes of each Stream data type.  
extern U32 streamDataSize[];

} // namespace arch

//  Just after all the definitions these files need.  
#include "cmStreamerFetch.h"
#include "cmStreamerOutputCache.h"
#include "cmStreamerLoader.h"
#include "cmStreamerCommit.h"

namespace arch
{

/**
 *
 *  This class implements the simulation of the cmoStreamController cmoMduBase.
 *
 *  The cmoStreamController cmoMduBase request data to the memory controller
 *  from the buffers with vertex attribute data and sends
 *  vertices to the vertex shader.
 *
 */

class cmoStreamController: public cmoMduBase
{

private:

    //  cmoStreamController main mdu signals.  
    Signal *streamCtrlSignal;       //  Control signal from the Command Processor.  
    Signal *streamStateSignal;      //  State signal to the Command Processor.  
    Signal *streamerFetchCom;       //  Command Signal to the StreamController Fetch.  
    Signal *streamerOCCom;          //  Command Signal to the StreamController Output Cache.  
    Signal **streamerLoaderCom;     //  Command Signal to the StreamController Loader Units.  
    Signal *streamerCommitCom;      //  Command Signal to the StreamController Commit.  
    Signal *streamerFetchState;     //  State signal from the StreamController Fetch.  
    Signal *streamerCommitState;    //  State signal from the StreamController Commit.  

    //  cmoStreamController sub boxes.  
    StreamerFetch *streamerFetch;               //  The StreamController Fetch subunit.  
    StreamerOutputCache *streamerOutputCache;   //  The StreamController Output Cache subunit.  
    StreamerLoader **streamerLoader;            //  The StreamController Loader subunits.  
    StreamerCommit *streamerCommit;             //  The StreamController Commit subunit.  

    //  cmoStreamController parameters.  
    U32 indicesCycle;            //  Indices per cycle processed in the cmoStreamController.  
    U32 indexBufferSize;         //  The index data buffer size (bytes).  
    U32 outputFIFOSize;          //  The output FIFO size (entries).  
    U32 outputMemorySize;        //  The size in lines of the output Memory.  
    U32 numShaders;              //  Number of Shaders that must be feed from the StreamController.  
    U32 maxShOutLat;             //  Maximum latency of the shader output signal.  
    U32 verticesCycle;           //  Vertices commited and sent to Primitive Assembly by the cmoStreamController per cycle.  
    U32 paAttributes;            //  Vertex attributes that can be sent per cycle to Primitive Assembly.  
    U32 outputBusLat;            //  Latency of the vertex bus with Primitive Assembly.  
    U32 streamerLoaderUnits;     //  Number of cmoStreamController Loader Units.  
    U32 slIndicesCycle;          //  Indices per cycle processed per cmoStreamController Loader unit.  
    U32 slInputRequestQueueSize; //  The input request queue size (entries) (cmoStreamController Loader unit).  
    U32 slFillAttributes;        //  Vertex attributes filled with data per cycle (cmoStreamController Loader unit).  
    U32 slInputCacheLines;       //  Number of lines (per set) in the input cache (cmoStreamController Loader unit).  
    U32 slInputCacheLineSize;    //  Input cache line size (cmoStreamController Loader unit).  
    U32 slInputCachePortWidth;   //  Input cache port width (cmoStreamController Loader unit).  
    U32 slInputCacheReqQSz;      //  Input cache request queue size (cmoStreamController Loader unit).  
    U32 slInputCacheInQSz;       //  Input cache read request queue size (cmoStreamController Loader unit).  
    bool slForceAttrLoadBypass;     //  Forces attribute load bypass (cmoStreamController Loader unit).  

    //  cmoStreamController registers.  
    U32 bufferAddress[MAX_STREAM_BUFFERS];       //  Stream buffer address in GPU local memory.  
    U32 bufferStride[MAX_STREAM_BUFFERS];        //  Stream buffer stride.  
    StreamData bufferData[MAX_STREAM_BUFFERS];      //  Stream buffer data type.  
    U32 bufferElements[MAX_STREAM_BUFFERS];      //  Number of elements (of any stream data type) per entry in the stream buffer.  
    U32 bufferFrequency[MAX_STREAM_BUFFERS];     //  Update/read frequency for the buffer: 0 -> per index/vertex, >0 -> MOD(instance, freq).  
    U32 attributeMap[MAX_VERTEX_ATTRIBUTES];     //  Mapping from vertex attributes to stream buffers..  
    Vec4FP32 attrDefValue[MAX_VERTEX_ATTRIBUTES];  //  Vertex attributes default values.  
    U32 start;                                   //  Start position in the stream buffers from where start streaming data.  
    U32 count;                                   //  Count of data elements to stream to the Vertex Shaders.  
    U32 instances;                               //  Number of instances of the current stream to process.  
    bool indexedMode;                               //  Use an index buffer to read vertex data.  
    U32 indexBuffer;                             //  Which stream buffer is used index buffer.  
    bool outputAttribute[MAX_VERTEX_ATTRIBUTES];    //  Stores if the vertex output attribute is active/written.  
    bool d3d9ColorStream[MAX_STREAM_BUFFERS];       //  Defines if the color stream must be read using the D3D9 color component order.  
    bool attributeLoadBypass;                       //  Flag that is used to disable attribute load (StreamerLoader) when implementing attribute load in the shader.  

    //  cmoStreamController state.  
    StreamerState state;        //  Current state of the cmoStreamController.  
    StreamerCommand *lastStreamCom;     //  Keeps the last StreamController command.  

    //  Private functions.  

    /**
     *
     *  Processes a cmoStreamController Command.
     *
     *  @param cycle Current simulation cycle.
     *  @param streamCom The cmoStreamController Command to process.
     *
     */

    void processStreamerCommand(U64 cycle, StreamerCommand *streamCom);

    /**
     *
     *  Processes a cmoStreamController register write.
     *
     *  @param gpuReg cmoStreamController register to write to.
     *  @param gpuSubReg cmoStreamController register subregister to write to.
     *  @param gpuData Data to write to the cmoStreamController register.
     *  @param cycle The current simulation cycle.
     *
     */

    void processGPURegisterWrite(GPURegister gpuReg, U32 gpuSubReg,
        GPURegData gpuData, U64 cycle);


public:

    /**
     *
     *  cmoStreamController mdu constructor.
     *
     *  Creates and initializes a new cmoStreamController mdu.
     *
     *  @param idxCycle Indices processed per cycle by the cmoStreamController.
     *  @param indexBufferSize The size in bytes of the index data buffer.
     *  @param outputFIFOSize The size in entries of the output FIFO.
     *  @param outputMemorySize The size in lines of the output Memory.
     *  @param numShaders Number of Shaders fed by the cmoStreamController.
     *  @param maxShOutLat Maximum latency of the shader output signal.
     *  @param vertCycle Number of vertices commited and sent to Primitive Assembly per cycle.
     *  @param paAttrCycle Vertex attributes that can be sent to Primitive Assembly per cycle.
     *  @param outLat Latency of the vertex bus with Primitive Assembly.
     *  @param sLoaderUnits Number of cmoStreamController Loader units.
     *  @param slIdxCycle Indices per cycle processed per cmoStreamController Loader unit.
     *  @param slInputReqSize The size in entries of the input request queue (cmoStreamController Loader unit).
     *  @param slFillAttrCycle Vertex attributes filled per cycle by the cmoStreamController (cmoStreamController Loader unit).
     *  @param slInCacheLines Number of lines (per set) in the input cache (cmoStreamController Loader unit).
     *  @param slInCacheLinesize Input cache line size (cmoStreamController Loader unit).
     *  @param slInCachePortWidth Input cache port width (cmoStreamController Loader unit).
     *  @param slInCacheReqQSz Input cache request queue size (cmoStreamController Loader unit).
     *  @param slInCacheInQSz Input cache read request queue size (cmoStreamController Loader unit).
     *  @param slForceAttrLoadBypass Forces attribute load bypass (cmoStreamController Loader unit).
     *  @param slPrefixArray Array of prefixes for the StreamController loader signals.     
     *  @param shPrefixArray Array of prefixes for the shader signals.
     *  @param name Name of the StreamController mdu.
     *  @param parent Pointer to the parent mdu.
     *
     *  @return A new cmoStreamController object.
     *
     */

    cmoStreamController(U32 idxCycle, U32 indexBufferSize, U32 outputFIFOSize, U32 outputMemorySize,
        U32 numShaders, U32 maxShOutLat, U32 vertCycle, U32 paAttrCycle, U32 outLat,
        U32 sLoaderUnits, U32 slIdxCycle, U32 slInputReqQSize, U32 slFillAttrCycle, U32 slInCacheLines,
        U32 slInCacheLinesize, U32 slInCachePortWidth, U32 slInCacheReqQSz, U32 slInCacheInQSz,
        bool slForceAttrLoadBypass,
        char **slPrefixArray, char **shPrefixArray, char *name, cmoMduBase *parent);

    /**
     *
     *  cmoStreamController mdu simulation rutine.
     *
     *  Carries the cycle accurate simulation for the cmoStreamController mdu.
     *  Receives commands and parameters from the Command Processor,
     *  requests memory transfers from the Memory Controller and
     *  sends vertex input data to the Vertex Shader.
     *
     *  @param cycle Cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /** 
     *
     *  Set cmoStreamController validation mode.
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
    
    /**
     *
     *  Get a map indexed per vertex index with the information about the vertices read from a cmoStreamController Loader unit
     *  in the current batch.
     *
     *  @param unit The cmoStreamController Loader unit for which to obtain the information about read vertices.
     *    
     *  @return A reference to a map indexed per vertex index with the information about the vertices read in the
     *  current batch.
     *
     */
  
    VertexInputMap &getVertexInputInfo(U32 unit);
    
};

} // namespace arch

#endif

