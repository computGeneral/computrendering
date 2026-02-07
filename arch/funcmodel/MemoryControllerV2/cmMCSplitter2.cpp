/**************************************************************************
 *
 */

#include "cmMCSplitter2.h"
#include "support.h"
#include <cmath>
#include <set>
#include <sstream>
#include <bitset>
#include <iostream>

using namespace cg1gpu;
using namespace cg1gpu::memorycontroller;
using namespace std;


MCSplitter2::MCSplitter2(U32 burstLength, U32 channels, U32 channelBanks,
                    U32 bankRows, U32 bankCols,
                    string channelMask, string bankMask ) :
    MemoryRequestSplitter(burstLength, channels, channelBanks, bankRows, bankCols)
{
    bitset<8*sizeof(U32)> chBitset(channels);
    bitset<8*sizeof(U32)> bBitset(channelBanks);
    if ( channels != 1 && chBitset.count() != 1 )
        CG_ASSERT("Only a number of channels power of 2 supported");
    if ( channelBanks != 1 && bBitset.count() != 1 )
        CG_ASSERT("Only a number of banks power of 2 supported");

    std::set<U32> selectedBits;
    channelBits = getBits(channelMask, selectedBits, channels, "Channel");
    bankBits = getBits(bankMask, selectedBits, channelBanks, "Bank");

    // channelBits.sort();
    // bankBits.sort();

    list<U32> channelBitsToRemove, toRemove;
    channelBitsToRemove = channelBits; // Copy channel bit list
    bitsToRemove = bankBits;           // Copy bank bit list
    bitsToRemove.splice(bitsToRemove.begin(), channelBitsToRemove); // merge the two lists
    bitsToRemove.sort(std::greater<U32>());
    
    colMask = createMask(bankCols);
    colShift = 2;
    rowMask = createMask(bankRows);
    rowShift = createShift(bankCols) + colShift;
}

list<U32> MCSplitter2::getBits(string bitMask, set<U32>& selectedBits, 
                                  U32 selections, string bitsTarget)
{
    list<U32> bitPositions;
    std::stringstream ss(bitMask);
    U32 bit = 32;
    while ( ss >> bit )
    {
        if ( bit > 31 )
        {    
            stringstream msg;
            msg << bitsTarget << " bitmask selecting bit (" << bit << ") greater than 31 or unknown value";
            CG_ASSERT(msg.str().c_str());
        }
        if ( selectedBits.count(bit) )
        {
            stringstream msg;
            msg << bitsTarget << " bitmask selecting a bit (" << bit << ") already used";
            CG_ASSERT(msg.str().c_str());
        }
        selectedBits.insert(bit);
        bitPositions.push_front(bit);        
    }

    U32 expectedBits = (U32)ceil(log((F64)selections)/log(2.0));
    if ( bitPositions.size() != expectedBits )
    {
        cout << "Bits = " << bitPositions.size() << "  Expected bits = " 
             << expectedBits << "\n";
        stringstream ss;
        ss << bitsTarget << " bitmask with not the exact bits to select the " << bitsTarget;
        CG_ASSERT(ss.str().c_str());
    }

    return bitPositions;
}


U32 MCSplitter2::getValue(const list<U32>& bits, U32 address)
{
    std::bitset<sizeof(U32)*8> addressbits(address);
    U32 expvalue = 1;
    U32 value = 0;
    list<U32>::const_iterator it = bits.begin();
    for ( ; it != bits.end(); it++ ) {
        if ( addressbits.test(*it) )
            value += expvalue;
        expvalue <<= 1;
    }
    return value;
}

U32 MCSplitter2::removeBits(U32 addr, const list<U32> &bitsToRemove)
{
    list<U32>::const_iterator it = bitsToRemove.begin();
    for ( ; it != bitsToRemove.end(); it++ )
    {
        U32 mask = (1 << *it) - 1;
        U32 low = addr & mask;
        addr = ((addr >> 1) & ~mask) | low;
    }
    return addr;
}
    
MemoryRequestSplitter::AddressInfo MCSplitter2::extractAddressInfo(U32 address) const
{
    AddressInfo inf;
    inf.channel = getValue(channelBits, address);
    inf.bank = getValue(bankBits, address);
    address = removeBits(address, bitsToRemove); // Remove channel & bank bits
    
    // Extract row and column
    // Last two bits ignored (columns have size multiple of 4)
    // Expected format: @ = R...RRCC..CCxx (bits 1 and 0 ignored)
    inf.row = ((address >> rowShift) & rowMask);
    inf.startCol = ((address >> colShift) & colMask);
    return inf;
}

U32 MCSplitter2::createAddress(AddressInfo addrInfo) const
{
    U32 address = 0;

    U32 bank = addrInfo.bank;
    U32 channel = addrInfo.channel;

    U32 tempAddress = (addrInfo.row << rowShift) | (addrInfo.startCol << colShift);

    list<U32>::const_iterator itChannel = channelBits.begin();
    list<U32>::const_iterator itBank = bankBits.begin();
    
    U32 nextChannelBit;
    U32 nextBankBit;
    
    if (itChannel != channelBits.end())    
        nextChannelBit = *itChannel;
    else
        nextChannelBit = 32;
    
    if (itBank != bankBits.end())    
        nextBankBit = *itBank;
    else
        nextBankBit = 32;        
    
    for(U32 bit = 0; bit < 32; bit++)
    {
        if (bit == nextChannelBit)
        {
            address = address | ((channel & 0x01) << bit);
            channel = channel >> 1;
            
            itChannel++;
            
            if (itChannel != channelBits.end())
                nextChannelBit = *itChannel;
            else
                nextChannelBit = 32;                
        }
        else if (bit == nextBankBit)
        {
            address = address | ((bank & 0x01) << bit);
            bank = bank >> 1;
            
            itBank++;
            
            if (itBank != bankBits.end())
                nextBankBit = *itBank;
            else
                nextBankBit = 32;                
        }
        else if ((bit != nextChannelBit) && (bit != nextBankBit))
        {        
            address = address | ((tempAddress & 0x01) << bit);
            tempAddress = tempAddress >> 1;
        }
        else
        {
            CG_ASSERT("Same bit used for channel and bank.");
        }
    }
    
    return address;   
}
