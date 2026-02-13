/**************************************************************************
 *
 */
#ifndef _MetaStreamTRACEDRIVER_
#define _MetaStreamTRACEDRIVER_

#include "GPUType.h"
#include "MetaStream.h"
#include "TraceDriverBase.h"
//#include "GLExec.h"
#include "zfstream.h"
#include "RegisterWriteBufferMeta.h"
#include <cstring>

/**
 *
 *  MetaStream Trace Driver class.
 *
 *  Generates MetaStreams to the GPU from an MetaStream trace file.
 *
 */
 
class TraceDriverMeta : public cgoTraceDriverBase
{

private:

    int initValue; ///< 0 if creation was ok, < 0 value if not
    U32 startFrame;
    U32 traceFirstFrame;
    U32 currentFrame;
    gzifstream *MetaTraceFile;
    
    enum TracePhase
    {
        TP_PREINIT,             //  Preinitialization phase.  Skip draw, blit and swap commands.
        TP_LOAD_REGS,           //  Register load phase.  Load the cached GPU register writes into the GPU.
        TP_LOAD_SHADERS,        //  Shader program load phase.  Load the cached shader programs into the GPU.
        TP_CLEARZSTBUFFER,      //  Clear z and stencil buffer phase.
        TP_CLEARCOLORBUFFER,    //  Clear color buffer phase.
        TP_END_INIT,            //  End of initialization phase.
        TP_SIM_START_EVENT,     //  Send reset event phase.
        TP_SIMULATION           //  Simulation phase.
    };
    
    TracePhase currentPhase;        ///<  Stores the current phase in the processing of the MetaStream trace.
    
    U32 shaderProgramLoadPhase;
    
    RegisterWriteBufferMeta registerCache;

    U32 startTransaction;
    U32 agpTransCount;       ///<  Counter for the number of processed agp transactions.
    
    static const U32 SHADERINSTRUCTIONSIZE = 16;     ///<  Size in bytes of a shader instruction.
    static const U32 MAXSHADERINSTRUCTIONS = 512;    ///<  Number of shaders instructions that can be stored in shader program memory.
    
    U08 fragProgramCache[SHADERINSTRUCTIONSIZE * MAXSHADERINSTRUCTIONS];  ///<  Caches the fragment shader program data for skipped frames.
    U08 vertProgramCache[SHADERINSTRUCTIONSIZE * MAXSHADERINSTRUCTIONS];  ///<  Caches the vertex shader program data for skipped frames.
    
    /**
     *
     *  This structure stores data uploaded to GPU memory that may contain shader program data.
     *
     */
     
    struct ProgramUpload
    {
        U32 address;
        U32 size;
        U08 *data;
        U32 md;
        U32 agpTransID;
        
        ///<  Creates a program upload object.
        ProgramUpload(U32 addr, U32 sz, U08 *data_, U32 md_, U32 agpID) :
            address(addr), size(sz), md(md_), agpTransID(agpID)
        {
            if ((data_ != NULL) && (size > 0))
            {
                data = new U08[size];
                memcpy(data, data_, size);
            }
            else
                data = NULL;
        }
        
        ///<  Updates the data associated with a program upload.
        void updateData(U08 *newData, U32 newSize, U32 agpID)
        {
            delete[] data;
            
            agpTransID = agpID;
            
            if ((newData != NULL) && (size > 0))
            {
                size = newSize;
                data = new U08[size];
                memcpy(data, newData, size);
            }
            else
            {
                size = 0;
                data = NULL;
            }            
        }
        
        //  Destroys the program upload.
        ~ProgramUpload()
        {
            delete[] data;
        }
    };
    
    typedef std::map<U32, ProgramUpload*>::iterator ProgramUploadsIterator;
    
    std::map<U32, ProgramUpload*> programUploads;    ///<  Stores all the shader program uploads. 
    
    ProgramUpload *lastProgramUpload;   ///<  Stores the last data upload that may contain shader program data.
        
    U32 fragmentProgramPC;           ///<  Stores the fragment program PC GPU register.
    U32 fragmentProgramAddress;      ///<  Stores the fragment program address GPU register.
    U32 fragmentProgramSize;         ///<  Stores the fragment program size GPU register.

    U32 vertexProgramPC;             ///<  Stores the vertex program PC GPU register.
    U32 vertexProgramAddress;        ///<  Stores the vertex program address GPU register.
    U32 vertexProgramSize;           ///<  Stores the vertex program size GPU register.
    
    
public:

    /**
     *
     *  MetaStream Trace Driver Constructor
     *
     *  Creates an MetaStream Trace Driver object and initializes it.  Trace Driver for MetaStream trace file.
     *
     *  @param ProfilingFile Pointer to a compressed input stream for the MetaStream file.
     *  @param startFrame Start simulation frame.  The Trace Driver won't send DRAW or SWAP commands until
     *  the start frame is reached. 
     *  @param traceFirstFrame First frame in the MetaStream tracefile.
     *
     *  @return  An initialized MetaStream Trace Driver object.
     *
     */
     
    TraceDriverMeta(gzifstream *ProfilingFile, U32 startFrame, U32 traceFirstFrame);
     
    
    /**
     * Starts the trace driver.
     *
     * Verifies if a MetaStream TraceDriver object is correctly created and available for use
     *
     * @return  0 if all is ok.
     *
     */
     
    int startTrace();

    /** Generates the next MetaStream from the API trace file.
     *  @return A pointer to the new MetaStream, NULL if there are no more MetaStreams. */
    arch::cgoMetaStream* nxtMetaStream();

    /**
     *
     *  Returns the current position (agp transaction count) inside the trace file.
     *
     *  @return The current position (agp transaction count) inside the trace file.
     *
     */    
     
    U32 getTracePosition();

    /**
     *
     *  Saves in a file the current trace position.
     *
     *  @param f Pointer to a file stream object where to save the current trace position.
     *
     */
     
    void saveTracePosition(std::fstream *f);

    /**
     *
     *  Loads from a file a start trace position.
     *
     *  @param f Pointer to a file stream object from where to load a start trace position.
     *
     */
     
    void loadStartTracePosition(std::fstream *f);
};


#endif
