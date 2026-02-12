/**************************************************************************
 *
 */

#include "cmMCSplitter.h"
#include "cmMemoryRequest.h"
#include "cmChannelTransaction.h"
#include "support.h"

using namespace arch::memorycontroller;
using namespace std;

MCSplitter::MCSplitter(U32 burstLength, U32 channels, U32 channelBanks, 
            U32 bankRows, U32 bankCols,
            U32 channelInterleaving,
            U32 bankInterleaving) : 
   MemoryRequestSplitter(burstLength, channels, channelBanks, bankRows, bankCols)
{
    GPU_ASSERT
    (
        if ( channelInterleaving < 4*burstLength )
            CG_ASSERT("Channel interleaving must be equal to 4*BurstLength bytes or greater");
        if ( bankInterleaving < 4*burstLength )
            CG_ASSERT("Bank interleaving must be equal to 4*BurstLength bytes or greater");
        if ( channelInterleaving % 4*burstLength != 0 )
            CG_ASSERT("Channel interleaving must be multiple of 4*BurstLength");
        if ( bankInterleaving % 4*burstLength != 0 )
            CG_ASSERT("Bank interleaving must be multiple of 4*BurstLength");
    )

    channelMask = createMask(channels);
    channelShift = createShift(channelInterleaving);
    channelInterleavingMask = createMask(channelInterleaving);
    channelInterleavingShift = createShift(channels);

    bankMask = createMask(channelBanks);
    bankShift = createShift(bankInterleaving);
    bankInterleavingMask = createMask(bankInterleaving);
    bankInterleavingShift = createShift(channelBanks);

    rowMask = createMask(bankRows);
    rowShift = createShift(bankCols) + 2; // see below to understand the "+ 2"

    colMask = createMask(bankCols);
    colShift = 2; // a column refers to 4 bytes thus the last two address bits don't care
}

MCSplitter::AddressInfo MCSplitter::extractAddressInfo(U32 address) const
{
    AddressInfo inf;
    U32 temp;

    if ( CHANNELS == 1 )
        inf.channel = 0;
    else { 
        // Extract channel bits selector
        inf.channel = ((address >> channelShift) & channelMask);    
        // remove channel bits selector from address
        temp = ((address >> channelInterleavingShift) & ~channelInterleavingMask);
        address = temp | (address & channelInterleavingMask);
    }

    if (  CHANNEL_BANKS == 1 )
        inf.bank = 0;
    else  { 
        // Extract bank bits selector
        inf.bank = ((address >> bankShift) & bankMask);
        // Remove bank bits selector address
        temp = ((address >> bankInterleavingShift) & ~bankInterleavingMask);
        address = temp | (address & bankInterleavingMask);
    }

    // Extract row and column bits selector
    inf.row = ((address >> rowShift) & rowMask);
    inf.startCol = ((address >> colShift) & colMask);

    return inf;    
}

U32 MCSplitter::createAddress(AddressInfo addrInfo) const
{
    U32 address;
    U32 temp;
    
    address = (addrInfo.row << rowShift) | (addrInfo.startCol << colShift);
    
    address = ((address & ~bankInterleavingMask) << bankInterleavingShift) |
              (addrInfo.bank << bankShift) | (address & bankInterleavingMask);
              
    address = ((address & ~channelInterleavingMask) << channelInterleavingShift) |
              (addrInfo.channel << channelShift) | (address & channelInterleavingMask);
              
    return address;              
}

