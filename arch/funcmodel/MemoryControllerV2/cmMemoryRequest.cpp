/**************************************************************************
 *
 */

#include "cmMemoryRequest.h"
#include <iostream>

using namespace cg1gpu::memorycontroller;
using namespace cg1gpu;
using std::vector;


MemoryRequest::MemoryRequest(MemoryTransaction* mt, U64 arrivalCycle) : 
    memTrans(mt),
    occupied(false),
    counter(0),
    state(MRS_READY),
    arrivalTime(arrivalCycle)
{}

void MemoryRequest::setArrivalTime(U64 arrivalCycle)
{
    arrivalTime = arrivalCycle;
}

U64 MemoryRequest::getArrivalTime() const
{
    return arrivalTime;
}

U32 MemoryRequest::getID() const
{
    if ( memTrans == 0 )
        CG_ASSERT("Request without any memory transaction");
    
    return memTrans->getID();
}


bool MemoryRequest::isRead() const
{
    if ( memTrans == 0 )
        CG_ASSERT("Request without any memory transaction");
    if ( memTrans->getCommand() == MT_READ_REQ )
        return true;
    else if ( memTrans->getCommand() == MT_WRITE_DATA )
        return false;
    else if ( memTrans->getCommand() == MT_PRELOAD_DATA )
        return false;
    else
        CG_ASSERT(
              "Only MemoryTransactions being READ_REQ and WRITE_DATA/PRELOAD_DATA can be Memory Requests"); 
    return false;
}


bool MemoryRequest::isMasked() const
{
    return memTrans->isMasked();
}

const U32* MemoryRequest::getMask() const
{
    return memTrans->getMask();
}

MemoryTransaction* MemoryRequest::getTransaction() const
{
    return memTrans;
}

void MemoryRequest::setState(MemReqState state_)
{
    state = state_;
}

MemReqState MemoryRequest::getState() const
{
    return state;
}

void MemoryRequest::setTransaction(MemoryTransaction* mt)
{
    memTrans = mt;
}

void MemoryRequest::setOccupied(bool inUse)
{
    occupied = inUse;
}

bool MemoryRequest::isOccupied() const
{
    return occupied;
}

void MemoryRequest::setCounter(U32 initialValue)
{
    counter =  initialValue;
}

void MemoryRequest::decCounter()
{
    if ( counter == 0 )
        CG_ASSERT("Trying to decrement a counter == 0");
    counter--;
}

U32 MemoryRequest::getCounter() const
{
    return counter;
}

void MemoryRequest::dump() const
{
    using std::cout;

    if ( memTrans == 0 )
        cout << "No MemoryTransaction set yet\n";
    else
    {
        cout << "Attached transaction info:\n";
        cout << " MemoryTransaction ID = " << memTrans->getID() << "\n";
        cout << "Source: ";
        switch ( memTrans->getRequestSource() )
        {
            case COMMANDPROCESSOR:
                cout << "COMMANDPROCESSOR"; break;
            case STREAMERFETCH:
                cout << "STREAMERFETCH"; break;
            case STREAMERLOADER:
                cout << "STREAMERLOADER"; break;
            case ZSTENCILTEST:
                cout << "ZSTENCILTEST"; break;
            case COLORWRITE:
                cout << "COLORWRITE"; break;
            case DISPLAYCONTROLLER:
                cout << "DISPLAYCONTROLLER"; break;
            case TEXTUREUNIT:
                cout << "TEXTUREUNIT"; break;
            case MEMORYMODULE:
                cout << "MEMORYMODULE"; break;
            case SYSTEM:
                cout << "SYSTEM"; break;
            default:
                cout << "UNKNOWN!";
        }
        cout << "\n";
        cout << " Address: " << memTrans->getAddress() << "\n"
                " Type: ";
        switch ( memTrans->getCommand() )
        {
            case MT_READ_REQ:
                cout << "MT_READ_REQ"; break;
            case MT_READ_DATA:
                cout << "MT_READ_DATA"; break;
            case MT_WRITE_DATA:
                cout << "MT_WRITE_DATA"; break;
            case MT_PRELOAD_DATA:
                cout << "MT_PRELOAD_DATA"; break;
            case MT_STATE:
                cout << "MT_STATE"; break;
        }
        cout << "\n";
        cout << "Masked: " << (memTrans->isMasked()?"Yes":"No") << "\n"
                " Size: " << memTrans->getSize() << " bytes\n";
    }

    cout << "Memory request owned info:\n";
    cout << " Counter = " << counter << "\n"
            " Occupied = " << (occupied ? "TRUE\n" : "FALSE\n") <<
            " Request state: ";
    switch ( state )
    {
        case MRS_READY:
            cout << "READY\n"; break;
        case MRS_WAITING:
            cout << "WAITING\n"; break;
        case MRS_TRANSMITING:
            cout << "TRANSMITING\n"; break;
        default:
            cout << "UNKNOWN!\n";
    }
    cout << "Arrival time = " << arrivalTime << "\n";
    cout << "-------------------------------------\n";
}
