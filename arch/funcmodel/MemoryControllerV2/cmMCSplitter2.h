/**************************************************************************
 *
 */

#ifndef MCSPLITTER2_H
    #define MCSPLITTER2_H

#include <list>
#include <set>
#include "GPUType.h"
#include "cmMemoryRequestSplitter.h"

namespace cg1gpu
{
namespace memorycontroller
{

/**
 * Object used to split memory request into channel transactions
 * This version is enhanced to support arbitrary selection of bits address
 *
 * This object has to take into account the address bits
 */
class MCSplitter2 : public MemoryRequestSplitter
{
public:


    /**
     * ctor
     *
     * @param channelMask string representing the bits that will be selected to compose
     *        the channel identifier
     * @param bankMask string representing the bits that will be selected to compose
     *        the bank identifier
     */
    MCSplitter2(U32 burstLength, U32 channels, U32 channelBanks,
                    U32 bankRows, U32 bankCols,
                    std::string channelMask, std::string bankMask );

    virtual U32 createAddress(AddressInfo addrInfo) const;

    virtual AddressInfo extractAddressInfo(U32 address) const;

protected:

private:

    std::list<U32> channelBits;
    std::list<U32> bankBits;
    std::list<U32> bitsToRemove;
    U32 rowMask;
    U32 rowShift;
    U32 colMask;
    U32 colShift;

    static std::list<U32> getBits(std::string bitMask, std::set<U32>& selected, 
                             U32 selections, std::string bitsTarget);

    static U32 removeBits(U32 addr, const std::list<U32> &bitsToRemove);

    static U32 getValue(const std::list<U32>& bits, U32 address);

}; // class MCSplitter2

}
}

#endif // MCSPLITTER2_H
