/**************************************************************************
 *
 */

#ifndef MEMORYREQUESTSPLITTER_H
    #define MEMORYREQUESTSPLITTER_H

#include <vector>
#include "GPUType.h" // to include U32 type

namespace arch 
{
namespace memorycontroller
{

class MemoryRequest;
class ChannelTransaction;

// Interface for Splitters splitting requests
class MemoryRequestSplitter
{
public:

    // Public types
    typedef std::vector<ChannelTransaction*> CTV;
    typedef CTV::iterator CTVIt;
    typedef CTV::const_iterator CTVCIt;

    MemoryRequestSplitter(U32 burstLength, U32 channels, U32 channelBanks,
                          U32 bankRows, U32 bankCols);
                          //U32 channelInterleaving, U32 bankInterleaving);

    // Called to split a mr in several channel transactions
    CTV split(MemoryRequest* mr);

    // Prints a representation of the address in the standard output
    virtual void printAddress(U32 address) const;
    
    struct AddressInfo
    {
        U32 channel;
        U32 bank;
        U32 row;
        U32 startCol;
    };

    // Creates and address given a channel, bank, row and column
    virtual U32 createAddress(AddressInfo addrInfo) const = 0;

    // Must be implemented by each subclass (is called by split method)
    virtual AddressInfo extractAddressInfo(U32 address) const = 0;

    virtual ~MemoryRequestSplitter() = 0;

protected:

    // Constants initialized at Splitter creation
    const U32 BURST_BYTES;
    const U32 CHANNELS;
    const U32 CHANNEL_BANKS;
    const U32 BANK_ROWS;
    const U32 BANK_COLS;

    // Helper static methods
    static U32 createMask(U32 value);
    static U32 createShift(U32 value);



private:

    void assertTransaction(const ChannelTransaction* ct) const;

};

} // namespace memorycontroller
} // namespace arch

#endif
