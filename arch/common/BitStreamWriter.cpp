/**************************************************************************
 *
 */

#include "BitStreamWriter.h"

namespace cg1gpu
{

void BitStreamWriter::write(U32 data, unsigned int bits)
{
    value |= (data & mask[bits]) << bitPos;
    bitPos += bits;
    if (bitPos >= maxBits) {
        bitPos &= (1 << maxBitsLog2) - 1;
        
        *(buffer++) = value;
        
        value = (data >> (bits - bitPos)) & mask[bitPos];
    }
    writtenBits += bits;
}
    
void BitStreamWriter::flush()
{
    if (bitPos > 0)
        *(buffer++) = value;
    value = 0;
    bitPos = 0;
    writtenBits = 0;
}

}
