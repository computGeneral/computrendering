/**************************************************************************
 *
 */

#include "cmMemoryRequestSplitter.h"
#include "cmMemoryRequest.h"
#include "cmChannelTransaction.h"
#include <utility> // to include make_pair
#include <iomanip> // to use std::setw manipulator
#include <iostream>
#include <stdio.h>
#include <bitset>

using namespace cg1gpu;
using namespace cg1gpu::memorycontroller;
using std::vector;
using std::make_pair;

MemoryRequestSplitter::MemoryRequestSplitter(U32 burstLength, U32 channels, 
                                             U32 channelBanks, U32 bankRows, 
                                             U32 bankCols) :
    BURST_BYTES(burstLength*4),
    CHANNELS(channels),
    CHANNEL_BANKS(channelBanks),
    BANK_ROWS(bankRows),
    BANK_COLS(bankCols)
{}

vector<ChannelTransaction*> MemoryRequestSplitter::split(MemoryRequest* mr)
{
    MemoryTransaction* memTrans = mr->getTransaction();
    
    GPU_ASSERT
    (
        if ( memTrans == 0 )
            CG_ASSERT("the memory transaction of the request is NULL");
        if ( memTrans->getSize() == 0 )
            CG_ASSERT("Request size can not be 0");
    )

    U32 nextAddress = memTrans->getAddress();

    GPU_ASSERT(
        if ((memTrans->getAddress() % BURST_BYTES) != 0)
        {
            printf("MemoryRequestSplitter (dumping transaction) Memory Transaction : \n");
            memTrans->dump(false);
            printf("\n");
            CG_ASSERT("Memory transaction is not aligned to burst length.");
        }
    )
    
    vector<std::pair<U32,U32> > cts;
    
    U32 totalBytes = memTrans->getSize(); // remaining bytes to be added
    
    // bytes to add in one step
    U32 bytes = ( totalBytes < BURST_BYTES ? totalBytes : BURST_BYTES );
    totalBytes -= bytes;

    cts.push_back(make_pair(nextAddress, bytes));   

    AddressInfo prevInfo = extractAddressInfo(nextAddress);

    while ( totalBytes != 0 )
    {
        // Compute next address
        nextAddress += bytes;
        
        // check if the next address pertains to the previous channel transaction
        AddressInfo info = extractAddressInfo(nextAddress);

        bytes = ( totalBytes < BURST_BYTES ? totalBytes : BURST_BYTES );
        totalBytes -= bytes;

        if ( info.channel == prevInfo.channel 
             && info.bank == prevInfo.bank // this condition is temporary
             && info.row == prevInfo.row )
        {
            // The second condition enforces that a transaction pertains to a unique
            // bank (this is temporary)
            cts.back().second += bytes; // Accum access in the previous CT
        }
        else
        {
            cts.push_back(make_pair(nextAddress,bytes));
            prevInfo = info;
        }
    }

    vector<ChannelTransaction*> vtrans;

    // Get data pointer
    U08* data = memTrans->getData();
    U32 offset = 0;
    if ( mr->isRead() )
    {
        for ( U32 i = 0; i < cts.size(); i++ )
        {
            AddressInfo info = extractAddressInfo(cts[i].first);
            vtrans.push_back( ChannelTransaction::createRead(
                                    mr, // The parent memory request
                                    info.channel, // The channel identifier
                                    info.bank, // The bank indentifier
                                    info.row, // The target row 
                                    info.startCol, // The starting column
                                    cts[i].second, // The size in bytes of the Channel T.
                                    data + offset // Pointer to the portion of data of this CT
                            ));
            assertTransaction(vtrans.back());

            offset += cts[i].second; // Accumulated offset into the pointer to data
        }
    }
    else // Write or Preload
    {
        const U32* mask = ( mr->isMasked() ? mr->getMask() : static_cast<const U32*>(0));
        for ( U32 i = 0; i < cts.size(); i++ )
        {
            AddressInfo info = extractAddressInfo(cts[i].first);
            vtrans.push_back( ChannelTransaction::createWrite(
                                    mr, // The parent memory request
                                    info.channel, // The channel identifier
                                    info.bank, // The bank indentifier
                                    info.row, // The target row 
                                    info.startCol, // The starting column
                                    cts[i].second, // The size in bytes of the Channel T.
                                    data + offset // Pointer to the portion of data of this CT
                            ));

            assertTransaction(vtrans.back());
            
            if ( mask != 0 )
                vtrans.back()->setMask(mask+offset/4);

            offset += cts[i].second; // Accumulated offset into the pointer to data
        }
    }

    return vtrans;
}

void MemoryRequestSplitter::assertTransaction(const ChannelTransaction* ct) const
{
  if ( ct->getChannel() >= CHANNELS )
  {
      ct->dump();
        CG_ASSERT("Channel out of bounds");
 }

 if ( ct->getBank() >= CHANNEL_BANKS )
  {
      ct->dump();
        CG_ASSERT("Bank out of bounds");
    }

 if ( ct->getRow() >= BANK_ROWS )
   {
      ct->dump();
        CG_ASSERT("Row out of bounds");
 }

 if ( ct->getCol() >= BANK_COLS )
   {
      ct->dump();
        CG_ASSERT("Column out of bounds");
  }
}

void MemoryRequestSplitter::printAddress(U32 address) const
{
    using namespace std;

    AddressInfo inf = extractAddressInfo(address);
    cout << "Address (as integer) : 0x" << std::hex << address << std::dec << "\n";
    cout << "As raw bytes (HEX): ";
    const char* ptr = reinterpret_cast<const char*>(&address);

    char prevFill = cout.fill('0'); 

    cout << std::hex << setw(2) << U32((U08)ptr[0]) << "." << setw(2) 
        << U32((U08)ptr[1]) << "." << setw(2) << U32((U08)ptr[2]) 
        << "." << setw(2) << U32((U08)ptr[3]) << std::dec << "\n";

    cout.fill(prevFill);

    std::bitset<sizeof(U32)> bset(address);
    cout << "As bitset: " << bset;

    cout << "\nChannel = " << inf.channel
         << "\nBank = " << inf.bank
         << "\nRow = " << inf.row
         << "\nColumn = " << inf.startCol;
    cout << "\n------------------" << endl;
}

MemoryRequestSplitter::~MemoryRequestSplitter()
{}


U32 MemoryRequestSplitter::createMask(U32 n)
{
    U32 l;
    U32 m;

    //  Value 0 has no mask.  
    if (n == 0)
        return 0;
    //  Value 1 translates to mask 0x01.  
    else if (n == 1)
        return 0x01;
    else
    {
        //  Calculate the shift for the value.  
        l = createShift(n);

        //  Build the start mask.  
        m = (unsigned int) -1;

        //  Adjust the mask.  
        m = m >> (32 - l);
    }

    return m;
}

U32 MemoryRequestSplitter::createShift(U32 n)
{
    U32 l;
    U32 m;

    //  Value 0 has no shift.  
    if (n == 0)
        return 0;
    //  Value 1 translates to 1 bit shift.  
    else if (n == 1)
        return 1;
    else
    {
        //  Calculate shift length for the value.  
        for (l = 0, m = 1; (l < 32) && (n > m); l++, m = m << 1);

        return l;
    }
}
