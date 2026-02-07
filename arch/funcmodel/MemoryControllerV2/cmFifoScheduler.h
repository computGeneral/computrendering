/**************************************************************************
 *
 */

#ifndef FIFOSCHEDULER_H
    #define FIFOSCHEDULER_H

#include <queue>
#include <utility>
#include "cmFifoSchedulerBase.h"

namespace cg1gpu
{
namespace memorycontroller
{

class GPUMemorySpecs;

/**
 * Basic fifo-scheduler with a single-unified read/write queue
 */
class FifoScheduler : public FifoSchedulerBase
{

public:

    FifoScheduler(const char* name,
                  const char* prefix,
                  cmoMduBase* parent,
                  const CommonConfig& config );
                  // U32 maxTransactions );

private:

    gpuStatistics::Statistic& closePageActivationsCount; // Counts how many times the close page algorithm is activated

    std::vector<U32> pendingBankAccesses;

    bool useInnerSignals;

    std::queue<ChannelTransaction*> transQ; ///< Read and write transaction queue
    U32 maxSizeQ; ///< Maximum number of in-flight channel transactions

    // transQ + currentTrans
    Signal* queueSignal; // bw = transQ + 1
    std::list<ChannelTransaction*> queueTrack;

    void processStateQueueSignals(U64 cycle);

protected:
    
    void receiveRequest(U64 cycle, ChannelTransaction* request);
    ChannelTransaction* selectNextTransaction(U64 cycle);

    void handler_ddrCommandNotSent(const DDRCommand* ddrCmd, U64 cycle);

    // calls directly processStateQueueSignals
    void handler_endOfClock(U64 cycle);
    
};
} // namespace memorycontroller
} // namespace cg1gpu

#endif // FIFOSCHEDULER_H

