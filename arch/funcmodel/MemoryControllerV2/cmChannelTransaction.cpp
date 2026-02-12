/**************************************************************************
 *
 */

#include "cmChannelTransaction.h"
#include "cmMemoryRequest.h"
#include <iostream>
#include <sstream>
#include <cstring>

using namespace arch::memorycontroller;
using arch::MemReqState;

U32 ChannelTransaction::instances = 0;

ChannelTransaction::ChannelTransaction()
{
    ++instances;
}

ChannelTransaction::~ChannelTransaction()
{
    --instances;
}

U32 ChannelTransaction::countInstances()
{
    return instances;
}


MemReqState ChannelTransaction::getState() const
{
    return req->getState();
}

bool ChannelTransaction::ready() const
{
    MemReqState state = req->getState();
    return ( state == MRS_READY || state == MRS_MEMORY );
}


ChannelTransaction* ChannelTransaction::createRead(MemoryRequest* memReq, 
                            U32 channel, U32 bank, U32 row, U32 col, 
                            U32 bytes, U08* dataBuffer)
{
    GPU_ASSERT
    (
        if ( memReq == 0 )
            CG_ASSERT("Memory Request is NULL");
        if ( dataBuffer == 0 )
            CG_ASSERT("Output data buffer is NULL");
    )

    ChannelTransaction* ct = new ChannelTransaction;
    ct->readBit = true; // it is a read
    ct->req = memReq; // memReq must be a read request
    ct->channel = channel;
    ct->bank = bank;
    ct->row = row;
    ct->col = col;
    ct->size = bytes;
    ct->dataBuffer = dataBuffer;
    ct->mask = 0;
    
    ct->setColor(0); // Use a 0 to identify reads
    
    return ct;
}

ChannelTransaction* ChannelTransaction::createWrite(MemoryRequest* memReq,
                    U32 channel, U32 bank, U32 row, U32 col, 
                    U32  bytes, U08* dataBuffer)
{
    GPU_ASSERT
    (
        if ( memReq == 0 )
            CG_ASSERT("Memory Request is NULL");
        if ( dataBuffer == 0 )
            CG_ASSERT("Input data buffer is NULL");
    )

    ChannelTransaction* ct = new ChannelTransaction;
    ct->readBit = false; // it is a write
    ct->req = memReq;
    ct->channel = channel;
    ct->bank = bank;
    ct->row = row;
    ct->col = col;
    ct->size = bytes;
    ct->dataBuffer = dataBuffer;
    ct->mask = 0;

    ct->setColor(1); // Use a 1 to identify writes

    return ct;
}


U32 ChannelTransaction::getUnitID() const
{
    return req->getTransaction()->getUnitID();
}


arch::GPUUnit ChannelTransaction::getRequestSource() const
{
    return req->getTransaction()->getRequestSource();
}


U32 ChannelTransaction::getRequestID() const
{
    return req->getID();
}


bool ChannelTransaction::isRead() const
{
    return readBit;
}

U32 ChannelTransaction::getChannel() const
{
    return channel;
}

U32 ChannelTransaction::getBank() const
{
    return bank;
}

U32 ChannelTransaction::getRow() const
{
    return row;
}

U32 ChannelTransaction::getCol() const
{
    return col;
}

const U08* ChannelTransaction::getData(U32 offset) const
{
    return (dataBuffer + offset);
}

void ChannelTransaction::setData(const U08* data_, U32 bytes, U32 offset)
{
    if ( bytes + offset > size )
        CG_ASSERT("Channel Transaction overflow");

    std::memcpy(dataBuffer+offset, data_, bytes);
}

MemoryRequest* ChannelTransaction::getRequest() const
{
    return req;
}

U32 ChannelTransaction::bytes() const
{
    return size;
}

bool ChannelTransaction::isMasked() const
{
    return mask != 0;
}

void ChannelTransaction::setMask(const U32* mask)
{
    this->mask = mask;
}

const U32* ChannelTransaction::getMask(U32 offset) const
{
    if ( offset >= size/4 )
        CG_ASSERT(
              "Offset cannot be greater that ChannelTransaction_size / 4");
    return mask + offset;
}

std::string ChannelTransaction::toString() const
{
    std::stringstream ss;
    ss << (readBit?"READ":"WRITE") << " ID=" << req->getID() << " bytes=" << size
       << " (C,B,R,Col)=(" << channel << "," << bank << "," << row << "," << col << ")";
    return ss.str();
}

void ChannelTransaction::dump(bool showData) const
{
    using namespace std;    
    cout << "Memory tran. ID = " << req->getID()
         << " Size: " << size
         << " Type: " << (readBit?"READ":"WRITE")
         << " (C,B,R,Col)=(" << channel << "," << bank << "," << row << "," << col << ")";
    
    if ( showData )
    {
        cout << " Data: ";
        for ( U32 i = 0; i < size; i++ )
        {
            cout << hex << (U32)dataBuffer[i] << dec << ".";
        }
    }
}


bool ChannelTransaction::overlapsWith(const ChannelTransaction& ct) const
{
    if ( bank == ct.bank && row == ct.row )
    {
        U32 minStart = col << 2;
        U32 maxStart = ct.col << 2;
        U32 minEnd;
        if ( minStart > maxStart )
        {
            // Swap mins
            U32 temp = minStart;
            minStart = maxStart;
            maxStart = temp;
            minEnd = minStart + ct.size;
        }
        else
            minEnd = minStart + size;

        return minEnd >= maxStart;
    }
    return false;
}
