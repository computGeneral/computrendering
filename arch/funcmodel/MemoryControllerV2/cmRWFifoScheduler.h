/**************************************************************************
 *
 */

#ifndef RWFIFOSCHEDULER_H
    #define RWFIFOSCHEDULER_H

#include "cmFifoSchedulerBase.h"
#include "cmDependencyQueue.h"
#include "cmSwitchOperationMode.h"

namespace cg1gpu
{
namespace memorycontroller
{

class GPUMemorySpecs;

class RWFifoScheduler : public FifoSchedulerBase
{
public:

    RWFifoScheduler( const char* name,
                     const char* prefix,
                     cmoMduBase* parent,
                     const CommonConfig& config
                     //U32 maxReadTransactions,
                     //U32 maxWriteTransaction,
                     // U32 maxConsecutiveReads,
                     // U32 maxConsecutiveWrites 
                     );


protected:

    void receiveRequest(U64 cycle, ChannelTransaction* request);

    ChannelTransaction* selectNextTransaction(U64 cycle);

    // Use this handler to implement page policy
    void handler_ddrCommandNotSent(const DDRCommand* ddrCmd, U64 cycle);

private:

    gpuStatistics::Statistic& closePageActivationsCount; // Counts how many times the close page algorithm is activated


    // ChannelTransaction* _selectNextTransaction_Counters(U64 cycle);
    // ChannelTransaction* _selectNextTransaction_old(U64 cycle);

    std::vector<U32> pendingBankAccesses;

    // SwitchModePolicy switchMode;
    // SwitchModeCounters* smpCounters;
    SwitchOperationModeBase* som;


    const U32 maxReadTransactions;
    const U32 maxWriteTransactions;
    // const U32 maxConsecutiveReads;
    // const U32 maxConsecutiveWrites;

    U32 consecutiveReads;
    U32 consecutiveWrites;

    bool createInnerSignals;

    DependencyQueue readQ;
    DependencyQueue writeQ;

    void handler_endOfClock(U64 cycle);

    void _coreDump() const;

};

} // namespace memorycontroller
} // namespace cg1gpu

#endif // RWFIFOSCHEDULER_H
