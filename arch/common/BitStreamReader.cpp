/**************************************************************************
 *
 */

#include "BitStreamReader.h"

namespace arch
{

U32 BitStreamReader::read(unsigned int bits)
{
    U32 r = 0;
    
    bool enoughBits = bits <= validBits;
    int minBits = enoughBits ? bits : validBits;
    int remBits = bits - minBits;

    r = value & mask[minBits];
    
    if (enoughBits) {    
        value >>= bits;
        validBits -= bits;
    }
    else {
        value = *(buffer++);
        r |= (value & mask[remBits]) << validBits;
        value >>= remBits;
        validBits = maxBits - remBits;
    }
    
    readedBits += bits;
    
    // Alternative without if's
    /*r = value & mask[minBits];
    value = enoughBits ? value >> bits : *buffer;
    buffer += enoughBits ? 0 : 1;
    r |= (value & mask[remBits]) << validBits;
    value >>= remBits;
    validBits = enoughBits ? validBits - bits : maxBits - remBits;*/
    return r;
}

}
