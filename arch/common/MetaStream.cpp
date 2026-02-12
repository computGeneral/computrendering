/**************************************************************************
 * MetaStream class implementation file.
 */

#include "MetaStream.h"
#include <iostream>
#include <cstring>

using namespace std;
using namespace arch;

// Macro for fast typing in cgoMetaStream::dump() method 
#define CASE_PRINT(value)\
    case  value:\
        os << #value;\
        break;


//  MetaStream constructor.  For memory operations.  
cgoMetaStream::cgoMetaStream(U32 addr, U32 dataSize, U08 *dataBuffer, U32 _md, bool isWrite, bool isLocked):

//  Set object attributes.  
address(addr), size(dataSize), data(dataBuffer), md(_md), locked(isLocked)

{
    if ( dataSize == 0 )
        CG_ASSERT("Trying to create an cgoMetaStream (memory op) with size 0");

    // create a copy of data 
    data = new U08[dataSize]; // "slow new"
    memcpy(data, dataBuffer,dataSize);

    //  Set as a write (system to local) memory operation.  
    if (isWrite)
        metaStreamType = META_STREAM_WRITE;
    else
    //  Or as a read (local to system) memory operation.  
        metaStreamType = META_STREAM_READ;

    //  Calculate number of packets for this transaction.  
    numPackets = 1 + (size >> META_STREAM_PACKET_SHIFT) + GPU_MOD(GPU_MOD(numPackets, META_STREAM_PACKET_SIZE), 1);

    //  Set object color for tracing.  
    setColor(metaStreamType);
    setTag("MetaStreamTr");
    //dump();
}

//  MetaStream constructor.  For preloading memory operations.  
cgoMetaStream::cgoMetaStream(U32 addr, U32 dataSize, U08 *dataBuffer, U32 _md):

//  Set object attributes.  
address(addr), size(dataSize), data(dataBuffer), md(_md), locked(true)

{
    if ( dataSize == 0 )
        CG_ASSERT("Trying to create an cgoMetaStream (memory preload) with size 0");

    // create a copy of data 
    data = new U08[dataSize]; // "slow new"
    memcpy(data, dataBuffer,dataSize);

    //  Set as a preload (system or local) memory operation.  
    metaStreamType = META_STREAM_PRELOAD;

    //  Calculate number of packets for this transaction.  
    numPackets = 1;

    //  Set object color for tracing.  
    setColor(metaStreamType);

    setTag("MetaStreamTr");

    
    //dump();
}

//  MetaStream constructor.  For register write operations.  
cgoMetaStream::cgoMetaStream(GPURegister gpuR, U32 subR, GPURegData rData, U32 _md):

//  Set object attributes.  
metaStreamType(META_STREAM_REG_WRITE), gpuReg(gpuR), subReg(subR), regData(rData), md(_md),
numPackets(1), locked(false)
{
    //  Set object color for tracing.  
    setColor(META_STREAM_REG_WRITE);

    setTag("MetaStreamTr");

}


//  MetaStream constructor.  For register read operations.  
cgoMetaStream::cgoMetaStream(GPURegister gpuR, U32 subR):

//  Set object attributes.  
metaStreamType(META_STREAM_REG_READ),gpuReg(gpuR), subReg(subR), md(0), numPackets(1), locked(false)
{
    //  Set object color for tracing.  
    setColor(META_STREAM_REG_READ);

    setTag("MetaStreamTr");
}


//  MetaStream constructor.  For GPU control commands.  
cgoMetaStream::cgoMetaStream(GPUCommand gpuComm):

//  Set object attributes.  
metaStreamType(META_STREAM_COMMAND), gpuCommand(gpuComm), md(0), numPackets(1), locked(false)
{
    //  Set object color for tracing.  
    setColor(META_STREAM_COMMAND);

    setTag("MetaStreamTr");
}

// MetaStream constructor.  Creates an META_STREAM_INIT_END transaction.
cgoMetaStream::cgoMetaStream() :
    metaStreamType(META_STREAM_INIT_END), md(0), numPackets(1), locked(false)
{
    setTag("MetaStreamTr");
}

//  MetaStream constructor.  Creates an META_STREAM_EVENT transaction.
cgoMetaStream::cgoMetaStream(GPUEvent gpuEvent, string msg) :

//  Set object attributes.
metaStreamType(META_STREAM_EVENT), gpuEvent(gpuEvent), eventMsg(msg)

{
    //  Set object color for tracing.
    setColor(META_STREAM_EVENT);
    
    setTag("MetaStreamTr");
}

//  MetaStream constructor.  Load from MetaStream trace file.
cgoMetaStream::cgoMetaStream(cgoMetaStream *sourceMetaStreamTrans)
{
    //  Clone the transaction type.
    metaStreamType = sourceMetaStreamTrans->metaStreamType;
    
    //  Clone transaction parameters based on the transaction type.
    switch(sourceMetaStreamTrans->metaStreamType)
    {
        case META_STREAM_WRITE:
        case META_STREAM_PRELOAD:
        
            address = sourceMetaStreamTrans->address;
            md = sourceMetaStreamTrans->md;
            locked = sourceMetaStreamTrans->locked;
            size = sourceMetaStreamTrans->size;
            
            //  Allocate space for the transaction data
            data = new U08[size];
            
            //  Check allocation
            if (data == NULL)
                CG_ASSERT("Error allocating memory for META_STREAM_WRITE/META_STREAM_PRELOAD transaction data buffer.");

            memcpy(data, sourceMetaStreamTrans->data, size);            
        
            break;
            
        case META_STREAM_REG_WRITE:
        
            gpuReg = sourceMetaStreamTrans->gpuReg;
            subReg = sourceMetaStreamTrans->subReg;
            memcpy(&regData, &sourceMetaStreamTrans->regData, sizeof(GPURegData));
            md = sourceMetaStreamTrans->md;
            
            break;
            
        case META_STREAM_COMMAND:
        
            gpuCommand = sourceMetaStreamTrans->gpuCommand;
            
            break;
            
        case META_STREAM_EVENT:
        
            gpuEvent = sourceMetaStreamTrans->gpuEvent;
            eventMsg =  sourceMetaStreamTrans->eventMsg;
                   
            break;
        
        case META_STREAM_INIT_END:
        
            break;           

        case META_STREAM_READ:
        
            CG_ASSERT("META_STREAM_READ transactions not supported.");
            break;

        case META_STREAM_REG_READ:
            CG_ASSERT("META_STREAM_REG_READ transactions not supported.");
            break;
        
        default:
            CG_ASSERT("Unknown MetaStream type.");
            break;
    }
    
    //  Set dynamic object color for signal tracing.
    setColor(metaStreamType);
    
    //  Set dynamic object tag.
    setTag("MetaStreamTr");
}

//  MetaStream constructor.  Load from MetaStream trace file.
cgoMetaStream::cgoMetaStream(gzifstream *ProfilingFile)
{
    U32 stringLength;
    U08 *stringData;
    
    ProfilingFile->read((char *) &metaStreamType, sizeof(metaStreamType));
    
    if (ProfilingFile->eof())
        return;
        
    //  Load transaction parameters based on the transaction type.
    switch(metaStreamType)
    {
        case META_STREAM_WRITE:
        case META_STREAM_PRELOAD:
        
            ProfilingFile->read((char *) &address, sizeof(address));
            ProfilingFile->read((char *) &md, sizeof(md));
            ProfilingFile->read((char *) &locked, sizeof(locked));
            ProfilingFile->read((char *) &size, sizeof(size));
            
            if (ProfilingFile->eof())
                return;
                
            //  Allocate space for the transaction data
            data = new U08[size];
            
            //  Check allocation
            if (data == NULL)
                CG_ASSERT("Error allocating memory for META_STREAM_WRITE/META_STREAM_PRELOAD transaction data buffer.");
            
            ProfilingFile->read((char *) data, size);
        
            break;
            
        case META_STREAM_REG_WRITE:
        
            ProfilingFile->read((char *) &gpuReg, sizeof(gpuReg));
            ProfilingFile->read((char *) &subReg, sizeof(subReg));
            ProfilingFile->read((char *) &regData, sizeof(regData));
            ProfilingFile->read((char *) &md, sizeof(md));
            
            break;
            
        case META_STREAM_COMMAND:
        
            ProfilingFile->read((char *) &gpuCommand, sizeof(gpuCommand));
            
            break;
            
        case META_STREAM_EVENT:
        
            ProfilingFile->read((char *) &gpuEvent, sizeof(gpuEvent));
            ProfilingFile->read((char *) &stringLength, sizeof(stringLength));

            //  Check if there was a message associated with the event.
            if (stringLength > 0)
            {
                //  Allocate the string.
                stringData = new U08[stringLength];
                
                // Read the string.
                ProfilingFile->read((char *) stringData, stringLength);
                
                // Set the event string.
                eventMsg = (char *) stringData;   
            
                //  Deallocate the string.
                delete[] stringData;
            }
            else
            {
                //  Set empty event string.
                eventMsg = "";
            }
                   
            break;
        
        case META_STREAM_INIT_END:
        
            CG_ASSERT("META_STREAM_INIT_END shouldn't be stored in an MetaStream trace.");
            break;           

        case META_STREAM_READ:
        
            CG_ASSERT("META_STREAM_READ transactions not supported.");
            break;

        case META_STREAM_REG_READ:
            CG_ASSERT("META_STREAM_REG_READ transactions not supported.");
            break;
        
        default:
            CG_ASSERT("Unknown MetaStream type.");
            break;
    }
    
    //  Set dynamic object color for signal tracing.
    setColor(metaStreamType);
    
    //  Set dynamic object tag.
    setTag("MetaStreamTr");
}

//  Sets the register data field for a register read transaction.  
void cgoMetaStream::setGPURegData(GPURegData rData)
{
    regData = rData;
}

//  Gets the register data field.  
GPURegData cgoMetaStream::getGPURegData()
{
    return regData;
}

//  Gets the MetaStream type.  
MetaStreamType cgoMetaStream::GetMetaStreamType()
{
    return metaStreamType;
}

//  Gets the MetaStream memory address for memory transactions.  
U32 cgoMetaStream::getAddress()
{
    return address;
}

//  Gets the MetaStream data size for memory transactions.  
U32 cgoMetaStream::getSize()
{
    return size;
}

//  Gets the MetaStream pointer to data for memory transactions.  
U08 *cgoMetaStream::getData()
{
    return data;
}

//  Gets the MetaStream GPU register identifier for register transactions.  
GPURegister cgoMetaStream::getGPURegister()
{
    return gpuReg;
}

//  Gets the MetaStream GPU subregister number for register transactions.  
U32 cgoMetaStream::getGPUSubRegister()
{
    return subReg;
}

//  Gets the GPU control command issued with this MetaStream.  
GPUCommand cgoMetaStream::getGPUCommand()
{
    return gpuCommand;
}

//  Gets the number of MetaStream packets for this MetaStream.  
U32 cgoMetaStream::getNumPackets()
{
    return numPackets;
}

//  Gets if the MetaStream is locked against the current batch and must wait.  
bool cgoMetaStream::getLocked()
{
    return locked;
}

//  Gets the memory descriptor identifier associated with the MetaStream.
U32 cgoMetaStream::getMD()
{
    return md;
}

//  Gets the GPU event.
GPUEvent cgoMetaStream::getGPUEvent()
{
    return gpuEvent;
}

//  Gets the event message string.
string cgoMetaStream::getGPUEventMsg()
{
    return eventMsg;
}

//  Changes a META_STREAM_WRITE transaction into an META_STREAM_PRELOAD transaction.
void cgoMetaStream::forcePreload()
{
    //  Check if the transaction is a write to memory.
    if (metaStreamType == META_STREAM_WRITE)
    {
        //  Change to a preload memory write.
        metaStreamType = META_STREAM_PRELOAD;
    }
}

//  Save MetaStream into a file.
void cgoMetaStream::save(gzofstream *outFile)
{
    U32 stringLength;
    
    outFile->write((char *) &metaStreamType, sizeof(metaStreamType));
 
    //  Write the MetaStream parameters based on the MetaStream type.   
    switch(metaStreamType)
    {
        case META_STREAM_WRITE:
        case META_STREAM_PRELOAD:
        
            outFile->write((char *) &address, sizeof(address));
            outFile->write((char *) &md, sizeof(md));
            outFile->write((char *) &locked, sizeof(locked));
            outFile->write((char *) &size, sizeof(size));
            outFile->write((char *) data, size);
        
            break;
            
        case META_STREAM_REG_WRITE:
        
            outFile->write((char *) &gpuReg, sizeof(gpuReg));
            outFile->write((char *) &subReg, sizeof(subReg));
            outFile->write((char *) &regData, sizeof(regData));
            outFile->write((char *) &md, sizeof(md));
        
            break;
            
        case META_STREAM_COMMAND:
        
            outFile->write((char *) &gpuCommand, sizeof(gpuCommand));
            
            break;
        
        case META_STREAM_EVENT:
        
            outFile->write((char *) &gpuEvent, sizeof(gpuEvent));
            
            //  Get the event message string lenght.
            stringLength = eventMsg.length();
            
            outFile->write((char *) &stringLength, sizeof(stringLength));
            
            //  Check if the event message is not empty.
            if (stringLength > 0)
                outFile->write((char *) eventMsg.c_str(), stringLength);
           
            break;
            
        case META_STREAM_INIT_END:
            
            CG_ASSERT("META_STREAM_INIT_END transactions can't be saved to an MetaStream trace file.");
            break;

        case META_STREAM_READ:
        
            CG_ASSERT("META_STREAM_READ transactions not supported.");
            break;

        case META_STREAM_REG_READ:
            CG_ASSERT("META_STREAM_REG_READ transactions not supported.");
            break;
        
        default:
            CG_ASSERT("Unknown MetaStream type.");
            break;
    }
    
}

void cgoMetaStream::dump(std::ostream& os) const
{
    os << "( MetaStreamt: ";
    switch ( metaStreamType )
    {
        CASE_PRINT( META_STREAM_WRITE )
        CASE_PRINT( META_STREAM_READ )
        CASE_PRINT( META_STREAM_PRELOAD )
        CASE_PRINT( META_STREAM_REG_WRITE )
        CASE_PRINT( META_STREAM_REG_READ )
        CASE_PRINT( META_STREAM_COMMAND )
        CASE_PRINT( META_STREAM_EVENT )
        CASE_PRINT( META_STREAM_INIT_END )
        default:
            os << metaStreamType << "(unkown agp transaction)" << endl;
    }

    bool ignoreU32bit = false;

    if ( metaStreamType == META_STREAM_READ || metaStreamType == META_STREAM_WRITE || metaStreamType == META_STREAM_PRELOAD )
    {
        os << ", Address: " << hex << address << dec << ", Size: " << size <<
            ", Data HC: ";
        U32 sum = 0;
        for ( U32 i = 0; i < size; i++ )
        {
            sum += (((U32)data[i]) * i) % 255;
        }
        os << sum << ", ";
    }
    else if ( metaStreamType == META_STREAM_REG_READ || metaStreamType == META_STREAM_REG_WRITE ) {
        os << ", REG: ";
        switch ( gpuReg ) {
            CASE_PRINT( GPU_STATUS );
            CASE_PRINT( GPU_DISPLAY_X_RES )
            CASE_PRINT( GPU_DISPLAY_Y_RES )
            //  GPU memory registers.  
            CASE_PRINT( GPU_FRONTBUFFER_ADDR )
            CASE_PRINT( GPU_BACKBUFFER_ADDR )
            CASE_PRINT( GPU_ZSTENCILBUFFER_ADDR )
            CASE_PRINT( GPU_TEXTURE_MEM_ADDR )
            CASE_PRINT( GPU_PROGRAM_MEM_ADDR )
            //  GPU vertex shader.  
            CASE_PRINT( GPU_VERTEX_PROGRAM )
            CASE_PRINT( GPU_VERTEX_PROGRAM_PC )
            CASE_PRINT( GPU_VERTEX_PROGRAM_SIZE )
            // CASE_PRINT( GPU_VERTEX_CONSTANT )
            case GPU_VERTEX_CONSTANT:
                ignoreU32bit = true;
                os << "GPU_VERTEX_CONSTANT";
                os << ", SubREG: " << subReg << ", Data(4-F32): {" << regData.qfVal[0] << ","
                   << regData.qfVal[1] << ","<< regData.qfVal[2] << ","<< regData.qfVal[3] << "}";
                break;
            CASE_PRINT( GPU_VERTEX_THREAD_RESOURCES )
            CASE_PRINT( GPU_VERTEX_OUTPUT_ATTRIBUTE )
            //  GPU vertex stream buffer registers.  
            CASE_PRINT( GPU_VERTEX_ATTRIBUTE_MAP )
            CASE_PRINT( GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE )
            CASE_PRINT( GPU_STREAM_ADDRESS )
            CASE_PRINT( GPU_STREAM_STRIDE )
            CASE_PRINT( GPU_STREAM_DATA )
            CASE_PRINT( GPU_STREAM_ELEMENTS )
            CASE_PRINT( GPU_STREAM_FREQUENCY )
            CASE_PRINT( GPU_STREAM_START )
            CASE_PRINT( GPU_STREAM_COUNT )
            CASE_PRINT( GPU_STREAM_INSTANCES )
            CASE_PRINT( GPU_INDEX_MODE )
            CASE_PRINT( GPU_INDEX_STREAM )
            CASE_PRINT( GPU_D3D9_COLOR_STREAM )
            //  GPU primitive assembly registers.  
            CASE_PRINT( GPU_PRIMITIVE )
            //  GPU clipping and culling registers.  
            CASE_PRINT( GPU_FRUSTUM_CLIPPING )
            CASE_PRINT( GPU_USER_CLIP )
            CASE_PRINT( GPU_USER_CLIP_PLANE )
            //  End of geometry related GPU registers.
            CASE_PRINT( GPU_LAST_GEOMETRY_REGISTER )
            //  GPU rasterization registers.  
            CASE_PRINT( GPU_FACEMODE )
            CASE_PRINT( GPU_CULLING )
            CASE_PRINT( GPU_HIERARCHICALZ )
            CASE_PRINT( GPU_EARLYZ )
            CASE_PRINT( GPU_D3D9_PIXEL_COORDINATES )
            CASE_PRINT( GPU_VIEWPORT_INI_X )
            CASE_PRINT( GPU_VIEWPORT_INI_Y )
            CASE_PRINT( GPU_VIEWPORT_WIDTH )
            CASE_PRINT( GPU_VIEWPORT_HEIGHT )
            CASE_PRINT( GPU_SCISSOR_TEST )
            CASE_PRINT( GPU_SCISSOR_INI_X )
            CASE_PRINT( GPU_SCISSOR_INI_Y )
            CASE_PRINT( GPU_SCISSOR_WIDTH )
            CASE_PRINT( GPU_SCISSOR_HEIGHT )
            CASE_PRINT( GPU_DEPTH_RANGE_NEAR )
            CASE_PRINT( GPU_DEPTH_RANGE_FAR )
            CASE_PRINT( GPU_DEPTH_SLOPE_FACTOR )
            CASE_PRINT( GPU_DEPTH_UNIT_OFFSET )
            CASE_PRINT( GPU_Z_BUFFER_BIT_PRECISSION )
            CASE_PRINT( GPU_D3D9_DEPTH_RANGE )
            CASE_PRINT( GPU_D3D9_RASTERIZATION_RULES )
            CASE_PRINT( GPU_TWOSIDED_LIGHTING )
            CASE_PRINT( GPU_MULTISAMPLING )
            CASE_PRINT( GPU_MSAA_SAMPLES )
            CASE_PRINT( GPU_INTERPOLATION )
            CASE_PRINT( GPU_FRAGMENT_INPUT_ATTRIBUTES )
            //  GPU fragment registers.  
            CASE_PRINT( GPU_FRAGMENT_PROGRAM )
            CASE_PRINT( GPU_FRAGMENT_PROGRAM_PC )
            CASE_PRINT( GPU_FRAGMENT_PROGRAM_SIZE )
            CASE_PRINT( GPU_FRAGMENT_CONSTANT )
            CASE_PRINT( GPU_FRAGMENT_THREAD_RESOURCES )
            CASE_PRINT( GPU_MODIFY_FRAGMENT_DEPTH )
            // Shader program registers.
            CASE_PRINT( GPU_SHADER_PROGRAM_ADDRESS )
            CASE_PRINT( GPU_SHADER_PROGRAM_SIZE )
            CASE_PRINT( GPU_SHADER_PROGRAM_LOAD_PC )
            CASE_PRINT( GPU_SHADER_PROGRAM_PC )
            CASE_PRINT( GPU_SHADER_THREAD_RESOURCES )
            // Texture Unit registers
            CASE_PRINT( GPU_TEXTURE_ENABLE )
            CASE_PRINT( GPU_TEXTURE_MODE )
            CASE_PRINT( GPU_TEXTURE_ADDRESS )
            CASE_PRINT( GPU_TEXTURE_WIDTH )
            CASE_PRINT( GPU_TEXTURE_HEIGHT )
            CASE_PRINT( GPU_TEXTURE_DEPTH )
            CASE_PRINT( GPU_TEXTURE_WIDTH2 )
            CASE_PRINT( GPU_TEXTURE_HEIGHT2 )
            CASE_PRINT( GPU_TEXTURE_DEPTH2 )
            CASE_PRINT( GPU_TEXTURE_BORDER )
            CASE_PRINT( GPU_TEXTURE_FORMAT )
            CASE_PRINT( GPU_TEXTURE_REVERSE )
            CASE_PRINT( GPU_TEXTURE_D3D9_COLOR_CONV )
            CASE_PRINT( GPU_TEXTURE_D3D9_V_INV )
            CASE_PRINT( GPU_TEXTURE_COMPRESSION )
            CASE_PRINT( GPU_TEXTURE_BLOCKING )
            CASE_PRINT( GPU_TEXTURE_BORDER_COLOR )
            CASE_PRINT( GPU_TEXTURE_WRAP_S )
            CASE_PRINT( GPU_TEXTURE_WRAP_T )
            CASE_PRINT( GPU_TEXTURE_WRAP_R )
            CASE_PRINT( GPU_TEXTURE_MIN_FILTER )
            CASE_PRINT( GPU_TEXTURE_MAG_FILTER )
            CASE_PRINT( GPU_TEXTURE_ENABLE_COMPARISON )
            CASE_PRINT( GPU_TEXTURE_COMPARISON_FUNCTION )
            CASE_PRINT( GPU_TEXTURE_MIN_LOD )
            CASE_PRINT( GPU_TEXTURE_MAX_LOD )
            CASE_PRINT( GPU_TEXTURE_LOD_BIAS )
            CASE_PRINT( GPU_TEXTURE_MIN_LEVEL )
            CASE_PRINT( GPU_TEXTURE_MAX_LEVEL )
            CASE_PRINT( GPU_TEXT_UNIT_LOD_BIAS )
            CASE_PRINT( GPU_TEXTURE_MAX_ANISOTROPY )
            // Render Target and Color Buffer Registers
            CASE_PRINT( GPU_COLOR_BUFFER_FORMAT )
            CASE_PRINT( GPU_COLOR_COMPRESSION )
            CASE_PRINT( GPU_COLOR_SRGB_WRITE )
            CASE_PRINT( GPU_RENDER_TARGET_ENABLE )
            CASE_PRINT( GPU_RENDER_TARGET_FORMAT )
            CASE_PRINT( GPU_RENDER_TARGET_ADDRESS )            
            //  GPU depth and stencil clear values.
            CASE_PRINT( GPU_Z_BUFFER_CLEAR )
            CASE_PRINT( GPU_STENCIL_BUFFER_CLEAR )
            CASE_PRINT( GPU_ZSTENCIL_STATE_BUFFER_MEM_ADDR )
            //  GPU Stencil test registers.
            CASE_PRINT( GPU_STENCIL_TEST )
            CASE_PRINT( GPU_STENCIL_FUNCTION )
            CASE_PRINT( GPU_STENCIL_REFERENCE )
            CASE_PRINT( GPU_STENCIL_COMPARE_MASK )
            CASE_PRINT( GPU_STENCIL_UPDATE_MASK )
            CASE_PRINT( GPU_STENCIL_FAIL_UPDATE )
            CASE_PRINT( GPU_DEPTH_FAIL_UPDATE )
            CASE_PRINT( GPU_DEPTH_PASS_UPDATE )
            //  GPU Depth test registers.
            CASE_PRINT( GPU_DEPTH_TEST )
            CASE_PRINT( GPU_DEPTH_FUNCTION )
            CASE_PRINT( GPU_DEPTH_MASK )
            CASE_PRINT( GPU_ZSTENCIL_COMPRESSION )
            //  GPU color clear value.
            CASE_PRINT( GPU_COLOR_BUFFER_CLEAR )
            CASE_PRINT( GPU_COLOR_STATE_BUFFER_MEM_ADDR )
            //  GPU Color Blend registers.
            CASE_PRINT( GPU_COLOR_BLEND )
            CASE_PRINT( GPU_BLEND_EQUATION )
            CASE_PRINT( GPU_BLEND_SRC_RGB )
            CASE_PRINT( GPU_BLEND_DST_RGB )
            CASE_PRINT( GPU_BLEND_SRC_ALPHA )
            CASE_PRINT( GPU_BLEND_DST_ALPHA )
            CASE_PRINT( GPU_BLEND_COLOR )
            CASE_PRINT( GPU_COLOR_MASK_R )
            CASE_PRINT( GPU_COLOR_MASK_G )
            CASE_PRINT( GPU_COLOR_MASK_B )
            CASE_PRINT( GPU_COLOR_MASK_A )
            //  GPU Color Logical Operation registers. 
            CASE_PRINT( GPU_LOGICAL_OPERATION )
            CASE_PRINT( GPU_LOGICOP_FUNCTION )
            // Register indicating the start address of the second interleaving. 
            //   Must be set to 0 to use a single (CHANNEL/BANK) memory interleaving 
            CASE_PRINT( GPU_MCV2_2ND_INTERLEAVING_START_ADDR )
            //  Blitter registers.
            CASE_PRINT( GPU_BLIT_INI_X )
            CASE_PRINT( GPU_BLIT_INI_Y )
            CASE_PRINT( GPU_BLIT_X_OFFSET )
            CASE_PRINT( GPU_BLIT_Y_OFFSET )
            CASE_PRINT( GPU_BLIT_WIDTH )
            CASE_PRINT( GPU_BLIT_HEIGHT )
            CASE_PRINT( GPU_BLIT_DST_ADDRESS )
            CASE_PRINT( GPU_BLIT_DST_TX_WIDTH2 )
            CASE_PRINT( GPU_BLIT_DST_TX_FORMAT )
            CASE_PRINT( GPU_BLIT_DST_TX_BLOCK )
            
            //  Last GPU register name mark.
            CASE_PRINT( GPU_LAST_REGISTER )
            
            default:
                os << gpuReg << "(unknown constant)";
        }

        if ( !ignoreU32bit )
            os <<", SubREG: " << subReg << ", Data(U32): " << regData.uintVal;
    }
    else if (metaStreamType == META_STREAM_EVENT)
    {
        os << ", GPUEvent: ";
        switch( gpuEvent)
        {
            CASE_PRINT(GPU_UNNAMED_EVENT)
            CASE_PRINT(GPU_END_OF_FRAME_EVENT)
            default:
                os << gpuEvent << "(unknown event)";
        }
    }
    else {
        os << ", GPUComm: ";
        switch ( gpuCommand ) {
            CASE_PRINT( GPU_RESET )
            CASE_PRINT( GPU_DRAW )
            CASE_PRINT( GPU_SWAPBUFFERS )
            CASE_PRINT( GPU_DUMPCOLOR )
            CASE_PRINT( GPU_DUMPDEPTH )
            CASE_PRINT( GPU_DUMPSTENCIL )
            CASE_PRINT( GPU_BLIT )
            CASE_PRINT( GPU_CLEARBUFFERS )
            CASE_PRINT( GPU_CLEARZBUFFER )
            CASE_PRINT( GPU_CLEARZSTENCILBUFFER )
            CASE_PRINT( GPU_CLEARCOLORBUFFER )
            CASE_PRINT( GPU_LOAD_VERTEX_PROGRAM )
            CASE_PRINT( GPU_LOAD_FRAGMENT_PROGRAM )
            CASE_PRINT( GPU_LOAD_SHADER_PROGRAM )
            CASE_PRINT( GPU_FLUSHZSTENCIL )
            CASE_PRINT( GPU_FLUSHCOLOR )
            CASE_PRINT( GPU_SAVE_COLOR_STATE )
            CASE_PRINT( GPU_RESTORE_COLOR_STATE )
            CASE_PRINT( GPU_SAVE_ZSTENCIL_STATE )
            CASE_PRINT( GPU_RESTORE_ZSTENCIL_STATE )
            CASE_PRINT( GPU_RESET_COLOR_STATE )
            CASE_PRINT( GPU_RESET_ZSTENCIL_STATE )

            default:
                os << gpuCommand << "(unknown command)";
        }
    }

    os << ", NumPackets: " << numPackets << " )" << endl;

}


cgoMetaStream::~cgoMetaStream()
{
    if ( metaStreamType == META_STREAM_READ || metaStreamType == META_STREAM_WRITE || metaStreamType == META_STREAM_PRELOAD)
    {
        delete[] data;
    }
}
