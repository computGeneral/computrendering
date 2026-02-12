
#ifndef REGISTERWRITEBUFFER_H
#define REGISTERWRITEBUFFER_H

#include <map>
#include "GPUReg.h"

class HAL;

class RegisterWriteBuffer
{
public:

    enum WritePolicy
    {
        WaitUntilFlush, // Register writes are stored until calling 'flush()' [DEFAULT]
        Inmediate // Do not use the store buffer (forward data instantaneously)  
    };

    RegisterWriteBuffer(HAL* driver, WritePolicy wp = WaitUntilFlush);
    void writeRegister(arch::GPURegister reg, U32 index, const arch::GPURegData& data, U32 md = 0);

    void readRegister(arch::GPURegister reg, U32 index, arch::GPURegData& data);
    
    // Direct write (inmediate write, not buffered)
    void unbufferedWriteRegister(arch::GPURegister reg, U32 index, const arch::GPURegData& data, U32 md = 0);

    void flush();

    void setWritePolicy(WritePolicy wp);
    WritePolicy getWritePolicy() const;

    void initRegisterStatus (arch::GPURegister reg, U32 index, const arch::GPURegData& data, U32 md);

    void initAllRegisterStatus ();

    void dumpRegisterStatus (int frame, int batch);

    void dumpRegisterInfo(std::ofstream& out, arch::GPURegister reg, U32 index, const arch::GPURegData& data, U32 md);


private:

    U32 registerWritesCount; // Statistic

    WritePolicy writePolicy;

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
    enum registerType{
        uintVal,
        intVal,
        qfVal,
        f32Val,
        booleanVal,
        status,
        faceMode,
        culling,
        primitive,
        streamData,
        compare,
        stencilUpdate,
        blendEquation,
        blendFunction,
        logicOp,
        txMode,
        txFormat,
        txCompression,
        txBlocking,
        txClamp,
        txFilter
    };

    struct RegisterData
    {
        arch::GPURegData data;
        U32 md;
        registerType dataType;

        RegisterData(arch::GPURegData data_, U32 md_ ) :
            data(data_), md(md_)
        {}
    };

    HAL* driver;

    // id, value
    typedef std::map<RegisterIdentifier, RegisterData> WriteBuffer;
    typedef WriteBuffer::iterator WriteBufferIt;
    typedef WriteBuffer::const_iterator WriteBufferConstIt;
    
    WriteBuffer writeBuffer;

    WriteBuffer registerStatus;
};

#endif // REGISTERWRITEBUFFER_H
