/**************************************************************************
 *
 * cmoStreamController Loader class definition file.
 *
 */


/**
 *
 *  @file cmStreamerLoader.h
 *
 *  This file contains the definition of the cmoStreamController Loader
 *  mdu.
 *
 */

namespace cg1gpu
{
    class StreamerLoader;
} // namespace cg1gpu

#ifndef _STREAMERLOADER_

#define _STREAMERLOADER_

#include "GPUType.h"
#include "cmMduBase.h"
#include "cmStreamer.h"
#include "cmStreamerCommand.h"
//#include "cmMemoryController.h"
#include "cmMemoryControllerDefs.h"
#include "GPUReg.h"
#include "cmStreamerControlCommand.h"
#include "cmInputCache.h"

namespace cg1gpu
{

/**
 *
 *  This structure defines an Input Request Queue entry.
 *
 */

struct InputRequest
{
    U32 index;                               //  The input index to load.  
    U32 instance;                            //  The instance index corresponding to the index.  
    U32 oFIFOEntry;                          //  Output FIFO entry where the index is stored.  
    StreamerControlCommand *stCCom;             //  The StreamController control command for this index.  
    Vec4FP32 attributes[MAX_VERTEX_ATTRIBUTES];//  Input attributes.  
    U32 nextAttribute;                       //  Next attribute to fetch/read from memory.  
    bool nextSplit;                             //  Fetching/reading the splited (second) part of the attribute.  
    U32 line[MAX_VERTEX_ATTRIBUTES][2];      //  Input cache line where the attribute is stored.  
    U32 address[MAX_VERTEX_ATTRIBUTES][2];   //  Address in memory where the attribute is stored.  
    U32 size[MAX_VERTEX_ATTRIBUTES][2];      //  Size in bytes of the attribute data to read from the cache.  
    bool split[MAX_VERTEX_ATTRIBUTES];          //  Flag for attributes that must be splitted into two memory accesses.  
};

/**
 *
 *  This class implements a cmoStreamController Loader cmoMduBase.
 *
 *  The cmoStreamController Loader cmoMduBase receives new indexes from the cmoStreamController
 *  Output Cache or the cmoStreamController Fetch, loads the attribute data for
 *  the index and issues it to the shaders.
 *
 *  Inherits from the cmoMduBase class that offers simulation support.
 *
 */

class StreamerLoader : public cmoMduBase
{

private:
    
    U32 unitID;		    //  The cmoStreamController Loader Unit identifier.  

    //  cmoStreamController Loader signals.  
    Signal *streamerFetchNewIndex;  //  New index signal from the cmoStreamController Fetch.  
    Signal *streamerNewIndex;       //  New index signal from the cmoStreamController Output Cache.  
    Signal *streamerLoaderDeAlloc;  //  Deallocation signal to the cmoStreamController Fetch.  
    Signal *memoryRequest;          //  Request signal to the Memory Controller.  
    Signal *memoryData;             //  Data signal from the Memory Controller.  
    Signal *streamerCommand;        //  Command signal from the main cmoStreamController mdu.  
    Signal **shCommSignal;          //  Pointer to an array of command signals to the Shaders.  
    Signal **shStateSignal;         //  Pointer to an array of shader state signals.  

    //  cmoStreamController Loader registers.  
    U32 streamAddress[MAX_STREAM_BUFFERS];   //  Stream buffer address in GPU local memory.  
    U32 streamStride[MAX_STREAM_BUFFERS];    //  Stream buffer stride.  
    StreamData streamData[MAX_STREAM_BUFFERS];  //  Stream buffer data type.  
    U32 streamElements[MAX_STREAM_BUFFERS];  //  Number of elements (of any stream data type) per entry in the stream buffer.  
    U32 streamFrequency[MAX_STREAM_BUFFERS]; //  Update/read frequency for the buffer: 0 -> per index/vertex, >0 -> MOD(instance, freq).  
    U32 attributeMap[MAX_VERTEX_ATTRIBUTES]; //  Mapping from vertex attributes to stream buffers..  
    Vec4FP32 attrDefValue[MAX_VERTEX_ATTRIBUTES];  //  Vertex attributes default values.  
    U32 streamStart;             //  Start position in the stream buffers from where start streaming data.  
    bool d3d9ColorStream[MAX_STREAM_BUFFERS];   //  Sets if the color stream has be be read using the component order defined by D3D9.  
    bool attributeLoadBypass;                   /**<  Flag used to disable attribute load, the index is passed to the shader through the index attribute
                                                      to implement attribute load in the shader using the LDA instruction.  */
                                                      
    //  cmoStreamController Loader parameters.  
    U32 indicesCycle;            //  Number of indices received from cmoStreamController Output Cache per cycle.  
    U32 IRQSize;                 //  Size of the input request queue in entries.  
    U32 attributesCycle;         //  Number of vertex attributes filled per cycle.  
    U32 inputCacheLines;         //  Number of lines in the input cache.  
    U32 inputCacheLineSize;      //  Size in bytes of the input cache lines.  
    U32 inputCachePortWidth;     //  Width in bytes of the input cache ports.  
    U32 inputCacheReqQSize;      //  Number of entries in the cache request queue.  
    U32 inputCacheFillQSize;     //  Number of entries in the input/fill queue in the input cache.  
    U32 numShaders;              //  Number of shaders connected to the cmoStreamController Loader.  
    bool forceAttrLoadBypass;       //  Forces attribute load bypass (attribute load from the shader).  

    //  cmoStreamController Loader state.  
    StreamerState state;            //  State of the cmoStreamController Loader.  
    StreamerCommand *lastStreamCom; //  Pointer to the last cmoStreamController Command received.  Kept for signal tracing.  

    //  Shader variables.  
    ShaderState *shaderState;       //  Array storing the current state of the Shaders.  
    U32 nextShader;              //  Next shader unit where to send an input.  

    //  Input cache.  
    InputCache *cache;              //  Pointer to the StreamController loader input cache.  
    U08 buffer[32];               //  Buffer for the attribute being read from the input cache.  
    
    //  Cache for instance attributes.
    U08 instanceAttribData[MAX_VERTEX_ATTRIBUTES][32];    //  Stores per-instance attribute data.  
    bool instanceAttribValid[MAX_VERTEX_ATTRIBUTES];        //  Stores if the per-instance attribute data is valid.  
    U32 instanceAttribTag[MAX_VERTEX_ATTRIBUTES];        //  Stores the instance index for the per-instance attribute stored.  

    //  Input request queue variables.  
    InputRequest *inputRequestQ;    //  Input request queue.  
    U32 nextFreeInput;           //  Next free entry in the input request queue.  
    U32 nextFetchInput;          //  Next input to fetch from memory.  
    U32 nextReadInput;           //  Next input to read from memory.  
    U32 nextLoadInput;           //  Next input to be sent to the shaders.  
    U32 freeInputs;              //  Number of free entries in the input request queue.  
    U32 fetchInputs;             //  Number of inputs to fetch from memory.  
    U32 readInputs;              //  Number of inputs to be read from memory.  
    U32 loadInputs;              //  Number of inputs already loaded and ready to be sent to the shaders.  

    //  Memory variables.  
    MemState memoryState;       //  Current state of the memory controller.  

    //  Statistics.  
    gpuStatistics::Statistic *indices;             //  Number of indices (only counts indices that must be loaded) received.  
    gpuStatistics::Statistic *transactions;        //  Number of transactions to Memory Controller.  
    gpuStatistics::Statistic *bytesRead;           //  Bytes read from memory.  
    gpuStatistics::Statistic *fetches;             //  Number of succesful fetch operations.  
    gpuStatistics::Statistic *noFetches;           //  Number of non succesful fetch operations.  
    gpuStatistics::Statistic *reads;               //  Number of succesful read operations.  
    gpuStatistics::Statistic *noReads;             //  Number of non succesful read operations.  
    gpuStatistics::Statistic *inputs;              //  Inputs sent to shaders.  
    gpuStatistics::Statistic *splitted;            //  Number of splitted attributes.  
    gpuStatistics::Statistic *mapAttributes;       //  Number of attributes mapped to a stream per vertex.  
    gpuStatistics::Statistic *avgVtxSentThisCycle; //  Average vertices sent per cycle to the Shader units.   
    gpuStatistics::Statistic *stallsShaderBusy;    //  Times the cmoStreamController Loader unit cannot send a ready vertex because Shader Units are busy.   

    //  Debug/validation.
    bool validationMode;                //  Flag that stores if the validation mode is enabled.  
    U32 nextReadLog;                 //  Pointer to the next read vertex log to read from.  
    U32 nextWriteLog;                //  Pointer to the next read vertex log to write to.  
    VertexInputMap vertexInputLog[4];   //  Map, indexed by vertex index, of the vertices read in the current draw call.  
    
    //  Private functions.  

    /**
     *
     *  Processes a Memory Transaction from the Memory Controller.
     *
     *  Checks the Memory Controller state and accepts requested
     *  data from the Memory Controller.
     *
     *  @param memTrans The Memory Transaction to process.
     *
     */

    void processMemoryTransaction(MemoryTransaction *memTrans);

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

    /**
     *
     *  Converts from a stream buffer data format to a
     *  32 bit float point value.
     *
     *  @param format stream buffer data format.
     *  @param data pointer to the data to convert.
     *
     *  @return The converted 32 bit float.
     *
     */

    F32 attributeDataConvert(StreamData format, U08 *data);

    /**
     *
     *  Configures the addresses and sizes for the actives attributes
     *  of a stream input to be read from memory.
     *
     *  @param input Pointer to a input request queue entry.
     *
     */

    void configureAttributes(U64 cycle, InputRequest *input);

    /**
     *
     *  Fills an input attribute with the read data using the format
     *  configuration currently active.
     *
     *  @param input Pointer to an input request queue entry.
     *  @param attr The input attribute to fill.
     *  @param data Pointer to a buffer with the attribute data read.
     *
     */

    void loadAttribute(InputRequest *input, U32 attr, U08 *data);


public:

    /**
     *
     *  cmoStreamController Loader constructor.
     *
     *  Creates and initializes a new cmoStreamController Loader object.
     *
     *  @param unitId The StreamController loader unit identifier
     *  @param idxCycle Number of indices received from cmoStreamController Output Cache per cycle.
     *  @param irqSize Size of the input request queue.
     *  @param attrCycle Number of vertex attributes filled per cycle.
     *  @param lines Number of lines in the input cache.
     *  @param linesize Size in bytes of the input cache lines.
     *  @param portWidth Width in bytes of the input cache ports.
     *  @param requestQSize Number of entries in the cache request queue.
     *  @param fillQSize Number of entries in the input/fill queue in the input cache.
     *  @param numShaders Number of Shaders fed by the cmoStreamController Loader.
     *  @param forceAttrLoadBypass Forces attribute load bypass (attribute load is performed in the shader).
     *  @param shPrefixArray Array of prefixes for the shader signals.
     *  @param name The mdu name.
     *  @param prefix The cmoStreamController Loader unit prefix
     *  @param parent The mdu parent mdu.
     *
     *  @return A new initialized cmoStreamController Loader mdu.
     *
     */

    StreamerLoader(U32 unitId, U32 idxCycle, U32 irqSize, U32 attrCycle, U32 lines, U32 linesize,
        U32 portWidth, U32 requestQSize, U32 fillQSize, bool forceAttrLoadBypass,
        U32 numShaders, char **shPrefixArray, char *name, char *prefix, cmoMduBase *parent);

    /**
     *
     *  cmoStreamController Loader simulation rutine.
     *
     *  Simulates a cycle of the cmoStreamController Loader.
     *
     *  @param cycle The cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /** 
     *
     *  Set cmoStreamController Loader validation mode.
     *
     *  @param enable Boolean value that defines if the validation mode is enabled.
     *
     */
     
    void setValidationMode(bool enable);
    
    /**
     *
     *  Get a map indexed per vertex index with the information about the vertices read in the current batch.
     *
     *  @return A reference to a map indexed per vertex index with the information about the vertices
     *  read in the current batch.
     *
     */
     
    VertexInputMap &getVertexInputInfo();
};

} // namespace cg1gpu

#endif


