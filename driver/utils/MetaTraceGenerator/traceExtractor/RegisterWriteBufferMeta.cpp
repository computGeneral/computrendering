/**************************************************************************
 *
 * RegisterWriteBufferMeta implementation file
 *  This file contains the implementation of a GPU register write buffer class used by the traceExtractor tool to
 *  store register writes until the start of the region to extract from the original MetaStream trace.
 *
 */

#include "RegisterWriteBufferMeta.h"
#include <iostream>

using namespace std;
using namespace cg1gpu;

RegisterWriteBufferMeta::RegisterWriteBufferMeta()
{}

void RegisterWriteBufferMeta::writeRegister(GPURegister reg, U32 index, const GPURegData& data, U32 md)
{
    RegisterIdentifier ri(reg, index);
    WriteBufferIt it = writeBuffer.find(ri);
    if ( it != writeBuffer.end() ) // register found, update contents
    {
        it->second.data = data;
        it->second.md = md;
    }
    else // register not written yet, create a new entry
    {
        RegisterData rd(data, md);
        writeBuffer.insert(make_pair(ri, rd));
    }
    registerWritesCount++;
}

bool RegisterWriteBufferMeta::readRegister(GPURegister reg, U32 index, GPURegData& data, U32 &md)
{
    RegisterIdentifier ri(reg, index);
    WriteBufferIt it = writeBuffer.find(ri);
    if ( it != writeBuffer.end() ) // register found, update contents
    {
        data = it->second.data;
        md = it->second.md;
        return true;
    }
    else
    {
        // register not written yet, create a new entry
        return false;    
    }
}

bool RegisterWriteBufferMeta::flushNextRegister(cg1gpu::GPURegister &reg, U32 &index, cg1gpu::GPURegData &data, U32 &md)
{
    //cout << "RegisterWriteBuffer::flushNextRegister() -> Doing pendent register writes: " << writeBuffer.size() << " (saved "
    //<< registerWritesCount - writeBuffer.size() << " reg. writes)" << endl;

    //  GPU_INDEX_STREAM must be sent before the configuration for the stream.
    //    But the storing order in the map is the inverse.

    RegisterIdentifier ri(GPU_INDEX_STREAM, 0);
    WriteBufferIt it = writeBuffer.find(ri);

    ///  Check if GPU_INDEX_STREAM was found.
    if ( it != writeBuffer.end() ) // register found, update contents
    {
        //  Get the register data.  */
        reg = GPU_INDEX_STREAM;
        index = 0;
        data = it->second.data;
        md = 0;

        //  Remove the register update from the write buffer.
        writeBuffer.erase(it);
        
        //  Update stored register writes counter.
        registerWritesCount--;

        //  A register was extracted from the buffer.
        return true;
    }
    else
    {
        // Read next CG1 register
        WriteBufferIt it = writeBuffer.begin();
        
        //  Check if all registers have been extracted.
        if(it != writeBuffer.end())
        {
            //  Get the register data.
            reg = it->first.reg;
            index = it->first.index;
            data = it->second.data;
            md = it->second.md;

            //  Remove the register update from the write buffer.
            writeBuffer.erase(it);

            //  Update stored register writes counter.
            registerWritesCount--;

            //  A register was extracted from the buffer.
            return true;
        }
        else
        {
            //  No register writes remaining in the buffer.
            return false;
        }
    }
}

