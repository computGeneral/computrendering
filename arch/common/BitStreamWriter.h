/**************************************************************************
 *
 */

#ifndef BITSTREAMWRITER_H_
#define BITSTREAMWRITER_H_

#include "GPUType.h"
#include "BitStream.h"

namespace arch
{

class BitStreamWriter : public BitStream
{
public:
    BitStreamWriter(U32 *buffer)
        : buffer(buffer), value(0), bitPos(0), writtenBits(0) {}
    
    virtual ~BitStreamWriter() { flush(); }
    
    void write(U32 data, unsigned int bits);
    
    void flush();
    
    unsigned int getWrittenBits() {
        return writtenBits;
    }
    
private:
    U32* buffer;
    U32 value;
    unsigned int bitPos;
    unsigned int writtenBits;
};

}

#endif //BITSTREAMWRITER_H_
