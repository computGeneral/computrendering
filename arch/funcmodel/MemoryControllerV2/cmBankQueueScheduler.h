/**************************************************************************
 *
 */

#ifndef BANKQUEUESCHEDULER_H
    #define BANKQUEUESCHEDULER_H

#include "cmFifoSchedulerBase.h"
#include "cmBankSelectionPolicy.h"
#include <list>
#include <vector>
#include "cmSwitchOperationMode.h"

namespace cg1gpu
{
namespace memorycontroller
{

class GPUMemorySpecs;

// Helper class implementing a queue with some "special" added features
class TQueue
{
public:

    TQueue();

    U32 getConsecutiveAccesses(bool writes) const;

    bool empty() const;

    U32 size() const;

    U64 getTimestamp() const;

    void enqueue(ChannelTransaction* ct, U64 timestamp);

    ChannelTransaction* front() const;

    void pop();

    void setName(const std::string& name);

    const std::string& getName() const;

private:

    struct QueueEntry
    {
        ChannelTransaction* ct;
        U64 timestamp;
        QueueEntry(ChannelTransaction* ct, U64 timestamp) : ct(ct), timestamp(timestamp)
        {}
        QueueEntry() : ct(0), timestamp(0) {}
    };

    typedef std::list<QueueEntry> Queue;

    Queue q;
    U32 qSize; // to increase efficiency in implementations with STL list.size() O(n)
    std::string qName;

};

class BankQueueScheduler : public FifoSchedulerBase
{
public:

    BankQueueScheduler( const char* name,
                        const char* prefix,
                        cmoMduBase* parent,
                        const CommonConfig& config
                        // U32 maxConsecutiveReads,
                        // U32 maxConsecutiveWrites 
                        );

protected:

    void receiveRequest(U64 cycle, ChannelTransaction* request);
    
    // Reimplement diferent policies
    ChannelTransaction* selectNextTransaction(U64 cycle);

    // use this handler to precharge or active a row when a DDRCommand
    // can not be issued
    void handler_ddrCommandNotSent(const DDRCommand* ddrCmd, U64 cycle);

    // sets state
    void handler_endOfClock(U64 cycle);

private:

    gpuStatistics::Statistic& closePageActivationsCount; // Counts how many times the close page algorithm is activated

    bool banksShareState;

    // SwitchModeCounters* smpCounters;
    SwitchOperationModeBase* som;

    enum AdvancedActiveMode
    {
        AAM_Conservative = 0,
        AAM_Agressive = 1
    };

    AdvancedActiveMode aam;

    // import types
    typedef BankSelectionPolicy::BankInfo BankInfo;
    typedef BankSelectionPolicy::BankInfoArray BankInfoArray;  

    std::vector<BankInfo> queueBankInfos;
    BankInfoArray queueBankPointers;

    BankSelectionPolicy* bankSelector;

    bool disableActiveManager;
    bool disablePrechargeManager;
    U32 managerSelectionAlgorithm;

    bool useInnerSignals;
    const U32 bankQueueSz;

    std::vector<TQueue> bankQ; // one queue per bank    

    U32 lastSelected;

    void _coreDump() const;

    struct Candidates
    {
        U32 read;
        U32 write;
        bool readIsHit;
        bool writeIsHit;
    };

    bool _countHits(U32 bankIgnored, U32& readHits, U32& writeHits) const;

    Candidates _findCandidateTransactions(BankInfoArray& queueBankPointers);

    std::vector<U32> getBankPriority(BankInfoArray& bia) const;

    bool _tryAdvancedPrecharge(const DDRCommand* notSentCommand, U64 cycle);

    bool _tryAdvancedActive(const DDRCommand* notSentCommand, U64 cycle);

    // Generic
    bool _tryAdvancedActive(const DDRCommand* notSentCommand, U64 cycle, bool tryRead, const std::vector<U32>& bankOrder);


    // DEPRECATED METHODS
    // ChannelTransaction* _selectNextTransaction_Counters(U64 cycle);
    // bool _tryAdvancedPrecharge_Counters(const DDRCommand* notSentCommand, U64 cycle);
    // bool _tryAdvancedActive_Counters(const DDRCommand* notSentCommand, U64 cycle);



};

} // namespace memorycontroller
} // namespace cg1gpu

#endif // BANKQUEUESCHEDULER_H
