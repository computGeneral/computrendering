/**************************************************************************
 *
 * RegisterWriteBufferMeta definition file
 *
 *  This file contains the definitions of a GPU register write buffer class used by the traceExtractor tool to
 *  store register writes until the start of the region to extract from the original MetaStream trace.
 */
 
#ifndef _REGISTERWRITEBUFFERMETA_STREAM_
    #define _REGISTERWRITEBUFFERMETA_STREAM_

#include <map>
#include "GPUReg.h"

class RegisterWriteBufferMeta
{
public:

    RegisterWriteBufferMeta();

    void writeRegister(arch::GPURegister reg, U32 index, const arch::GPURegData& data, U32 md = 0);

    bool readRegister(arch::GPURegister reg, U32 index, arch::GPURegData &data, U32 &md);
    
    bool flushNextRegister(arch::GPURegister &reg, U32 &index, arch::GPURegData &data, U32 &md);

private:

    U32 registerWritesCount; // Statistic

    struct RegisterIdentifier
    {
        arch::GPURegister reg;
        U32 index;

        RegisterIdentifier(arch::GPURegister reg_, U32 index_) :
            reg(reg_), index(index_)
        {}
        
        friend bool operator<(const RegisterIdentifier& lv,
                              const RegisterIdentifier& rv)
        {
            if ( lv.reg == rv.reg )
                return lv.index < rv.index;
                
            return lv.reg < rv.reg;
        }

        friend bool operator==(const RegisterIdentifier& lv, 
                               const RegisterIdentifier& rv)
        {
            return (lv.reg == rv.reg && lv.index == rv.index );
        }
    };
    
    struct RegisterData
    {
        arch::GPURegData data;
        U32 md;

        RegisterData(arch::GPURegData data_, U32 md_) :
            data(data_), md(md_)
        {}
        
    };

    // id, value
    typedef std::map<RegisterIdentifier, RegisterData> WriteBuffer;
    typedef WriteBuffer::iterator WriteBufferIt;
    typedef WriteBuffer::const_iterator WriteBufferConstIt;
    
    WriteBuffer writeBuffer;
};

#endif // REGISTERWRITEBUFFER_H
