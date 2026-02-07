/**************************************************************************
 *
 */

#ifndef MCSPLITTER_H
    #define MCSPLITTER_H

#include <vector>
#include "GPUType.h"
#include "cmMemoryRequestSplitter.h"

namespace cg1gpu
{
namespace memorycontroller
{

/**
 * Object used to split memory request into channel transactions
 *
 * This object has to take into account the address bits
 */
class MCSplitter : public MemoryRequestSplitter
{
public:


    MCSplitter(U32 burstLength, U32 channels, U32 channelBanks,
               U32 bankRows, U32 bankCols,
               U32 channelInterleaving, U32 bankInterleaving );

    virtual U32 createAddress(AddressInfo addrInfo) const;

    virtual AddressInfo extractAddressInfo(U32 address) const;

protected:

private:
    
    U32 channelMask;// = 0x7;
    U32 bankMask;// = 0x7;
    U32 rowMask; // = 0xFFF;
    U32 colMask;// = 0x1FF;  

    U32 channelShift;
    U32 bankShift;
    U32 rowShift;
    U32 colShift;

    // Auxiliar masks and shifts to remove address bits
    U32 channelInterleavingMask;
    U32 channelInterleavingShift;
    U32 bankInterleavingMask;
    U32 bankInterleavingShift;

};

} // namespace memorycontroller
} // namespace cg1gpu

#endif // MCSPLITTER_H
