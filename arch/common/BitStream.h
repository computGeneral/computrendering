/**************************************************************************
 *
 */

#ifndef BITSTREAM_H_
#define BITSTREAM_H_

#include "GPUType.h"

namespace arch
{

template <size_t N, size_t base=2> 
struct const_log {
    enum { value = 1 + const_log<N/base, base>::value }; 
};

template <size_t base>
struct const_log<1, base> {
    enum { value = 0 };
};

template <size_t base>
struct const_log<0, base> {
    enum { value = 0 };
};

/* Rudimentary bit stream support.
 * See also BitStreamReader and BitStreamWriter. */

class BitStream
{
public:
    BitStream() {}
    virtual ~BitStream() {}
    
protected:
    static const int maxBits = sizeof(U32) * 8;
    static const int maxBitsLog2 = const_log<maxBits, 2>::value;
        
    static const U32 mask[maxBits + 1];
};

}

#endif //BITSTREAM_H_
