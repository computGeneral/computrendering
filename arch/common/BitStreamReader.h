/**************************************************************************
 *
 */

#ifndef BITSTREAMREADER_H_
#define BITSTREAMREADER_H_

#include "GPUType.h"
#include "BitStream.h"

namespace cg1gpu
{

class BitStreamReader : public BitStream
{
public:
    BitStreamReader(U32* buffer)
        : buffer(buffer), value(0), validBits(0), readedBits(0) {}
    
    virtual ~BitStreamReader() {}
    
    U32 read(unsigned int bits);
    
    unsigned int getReadedBits() {
        return readedBits;
    }
    
private:
    U32* buffer;
    U32 value;
    unsigned int validBits;
    unsigned int readedBits;
};

}

#endif //BITSTREAMREADER_H_
