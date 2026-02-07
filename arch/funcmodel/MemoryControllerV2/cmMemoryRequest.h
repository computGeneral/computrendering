/**************************************************************************
 *
 */

#ifndef MEMORYREQUEST_H
    #define MEMORYREQUEST_H

#include "GPUType.h"
#include "cmMemoryTransaction.h"
#include <vector>


namespace cg1gpu
{
namespace memorycontroller
{

class ChannelTransaction; // forward declaration

class MemoryRequest
{
public:

    MemoryRequest(MemoryTransaction* mt = 0, U64 arrivalTime = 0);

    void setArrivalTime(U64 arrivalTime);
    U64 getArrivalTime() const;

    U32 getID() const;
    bool isRead() const; // false if it is a write
    bool isMasked() const;
    const U32* getMask() const;
    MemoryTransaction* getTransaction() const;

    void setState(MemReqState state);
    MemReqState getState() const;
    void setTransaction(MemoryTransaction* mt);
    
    void setOccupied(bool inUse);
    bool isOccupied() const;

    void setCounter(U32 initialValue);
    void decCounter();
    U32 getCounter() const;

    // to compute average time to serve a transaction
    void setReceivedCycle(U64 cycle);
    U64 getReceivedCycle() const;

    void dump() const;

private:

    MemoryTransaction* memTrans;
    U32 counter;
    bool occupied;
    MemReqState state;
    U64 arrivalTime;

};

} // namespace memorycontroller
} // namespace cg1gpu


#endif // MEMORYREQUEST_H
