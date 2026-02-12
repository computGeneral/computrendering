/**************************************************************************
 *
 */

#ifndef FIFOSCHEDULERBASE_H
    #define FIFOSCHEDULERBASE_H

#include "cmChannelScheduler.h"
#include <queue>
#include <list>
#include "cmtoolsQueue.h"

namespace arch
{
namespace memorycontroller
{

class GPUMemorySpecs;

/**
 * Parent class of all schedulers using a Fifo-like strategy
 */
class FifoSchedulerBase : public ChannelScheduler
{
protected:


    FifoSchedulerBase(const char* name, const char* prefix, cmoMduBase* parent, const ChannelScheduler::CommonConfig& config);

    // Implemented for all fifo-like schedulers
    void schedulerClock(U64 cycle);

    /**
     * Gets the current in progress transaction
     */
    ChannelTransaction* getCurrentTransaction();

    /**
     * This method implements the transaction allocation in the fifo structure
     * implemented in the scheduler
     */ 
    virtual void receiveRequest(U64 cycle, ChannelTransaction* request)=0;
    
    /**
     * Must be implemented in each subclass
     * This method must implement the queue strategy selection
     */
    virtual ChannelTransaction* selectNextTransaction(U64 cycle) = 0;

    /**
     * Called when a DDRCommand in the command buffer cannot be sent
     * due to any memory constraint
     *
     * This method is provided to allow sending another command if the command
     * in the command buffer cannot be sent
     * (for example a precharge or active to another bank)
     *
     * @param ddrCmd The command not sent
     */
    virtual void handler_ddrCommandNotSent(const DDRCommand* /*ddrCmd*/, U64 /*cycle*/) 
    { /* empty by default*/  }
    virtual void handler_ddrCommandSent(const DDRCommand* /*ddrCmd*/, U64 /*cycle*/) 
    { /* empty by default*/  }

    /**
     * This method is called at the end of the current cycle simulation
     *
     * Usually used to include specific code to be performed each cycle
     *
     * (for example: managing STV feedback signals)
     */
    virtual void handler_endOfClock(U64 /*cycle*/) { /* empty by default*/  }

    /**
     * Called each time a channel transaction has been resolved (completed)
     */
    virtual void handler_transactionCompleted(const ChannelTransaction* /*ct*/, U64 /*cycle*/)
    { /* empty by default */ }

    // Implemented here for all Fifo-like schedulers
    // (ie. automatically called by all fifo schedulers)
    void receiveData(U64 cycle, DDRBurst* data);


private:

    enum CShedState
    {
        CSS_Idle,
        CSS_WaitPrevCmd,
        CSS_WaitPrevPageClose,
        CSS_WaitOpeningPage,
        CSS_AccessingData
    };

    CShedState cSchedState;

    struct CTAccessInfo
    {
        U64 startCycle; ///< When a transaction starts putting/getting data to/from datapins
        U32 remainingTransferCycles; ///< Cycles remaing for complete the transaction
        bool isRead; /// type of transaction
    };

    U32 readDelay;
    U32 writeDelay;
    const U32 BytesPerCycle;
    const U32 CyclesPerBurst;

    /**
     * Used to keep track of when data is in the datapins and be able to detect when data is being read/written
     *
     * This queue should have data only when cSchedState is equal to CSS_AccessingData
     */
    arch::tools::Queue<CTAccessInfo> ongoingAccessesQueue; 

    gpuStatistics::Statistic& cSched_IdleCycles;
    gpuStatistics::Statistic& cSched_PrevCmdWaitCycles;
    gpuStatistics::Statistic& cSched_PrevPageCloseCycles;
    gpuStatistics::Statistic& cSched_OpeningPageCycles;
    gpuStatistics::Statistic& cSched_AccessingReadDelayCycles; // CAS or WL latency cycles
    gpuStatistics::Statistic& cSched_AccessingWriteDelayCycles;
    gpuStatistics::Statistic& cSched_AccessingReadDataCycles;
    gpuStatistics::Statistic& cSched_AccessingWriteDataCycles;

    // This signal is written with each time a new transaction is selected
    // This signal is read every clock
    Signal* selectedTransactionSignal;

    // Cycles in which no DDR commands are available (idle control)
    gpuStatistics::Statistic& ctrlIdleCyclesStat;

    // Cycles sending a command in the control bus
    gpuStatistics::Statistic& ctrlUsedCyclesStat;

    // Time constraint stats
    gpuStatistics::Statistic& atorStat; // Active to read penalty cycles
    gpuStatistics::Statistic& atowStat; // Active to write cycles penalty cycles
    gpuStatistics::Statistic& atoaStat; // Active to Active penalty cycles
    gpuStatistics::Statistic& ptoaStat; // Precharge to active penalty cycles
    gpuStatistics::Statistic& wtorStat; // Write to read penalty cycles
    gpuStatistics::Statistic& rtowStat; // Read to write penalty cycles
    gpuStatistics::Statistic& wtopStat; // Write to precharge penalty cycles
    gpuStatistics::Statistic& atopStat; // Active to precharge penalty cycles
    gpuStatistics::Statistic& rtopStat; // Read to precharge penalty cycles
    gpuStatistics::Statistic& rtorStat; // Read to read "penalty" cycles
    gpuStatistics::Statistic& wtowStat; // Write to write "penalty" cycles

    bool lastTransactionWasRead;
    gpuStatistics::Statistic& switchModeCount; // Statistic counting how much switches from r2w and w2r happen

    bool waitingForFirstTransactionAccess;
    U64 cycleLastTransactionSelected;
    gpuStatistics::Statistic& sumPreAct2AccessCycles; // sum of cycles lost between 


    //gpuStatistics::Statistic& otherLostCyclesStat;

    void updateUnusedCyclesStats(DDRModuleState::IssueConstraint ic);

    /**
     * Generate all the required DDRCommands to satisfy the channeltransaction
     *
     * Return the number of burst (reads or writes) required to satisfy the transaction
     */
    U32 fillCommandBuffer(const ChannelTransaction* ct); // , PagePolicy policy);
    
    void processNextDDRCommand(U64 cycle);
    void processReadReply(U64 cycle);


    U32 BurstBytes; /// constant equivalent to BurstSize * 4

    // Pending DDRCommand to solve the last channel transaction processed
    std::list<DDRCommand*> commandBuffer;
    
    // Transaction that has generated the current commands in the command buffer
    ChannelTransaction* currentTrans;

    U32 pendingWriteBursts; // Counter to known how many write burst remains
    // This counter is only available if the currentTrans is a write

    std::list<std::pair<ChannelTransaction*,U32> > inProgressReads;
    std::list<U32> inProgressReadBursts;

    std::queue<ChannelTransaction*> replyQ;
};

} // namespace memorycontroller
} // namespace arch

#endif
