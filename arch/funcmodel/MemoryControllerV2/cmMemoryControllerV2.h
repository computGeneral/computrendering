/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

#include "cmMultiClockMdu.h"
#include "cmMemoryTransaction.h"
#include "cmtoolsQueue.h"
#include "cmChannelScheduler.h"
#include "cmDDRModule.h"
#include "cmMemoryRequest.h"

//  std includes
#include <string>
#include <vector>

namespace cg1gpu
{
namespace memorycontroller
{
class GPUMemorySpecs;
class MemoryRequestSplitter;
class MemoryTraceRecorder;

//**  Maximum number of memory transaction ids availables.  
const U32 MAX_MEMORY_TICKETS = 256;

//*  Memory transaction transmission cycles (to physical memory).  
const U32 TRANSACTION_CYCLES = 4;

//** Maximum number of buses per GPU unit type.  
// deprecated
//const U32 MAX_GPU_UNIT_BUSES = 16;

/**
 *  Defines the latency to system memory in cycles.
 */
const U32 SYSTEM_MEMORY_READ_LATENCY = 500;
const U32 SYSTEM_MEMORY_WRITE_LATENCY = 500;

/**
 *  Defines the cycles it takes to transmit a memory transaction from/to system memory.
 */
const U32 SYSTEM_TRANSACTION_CYCLES = 16;

/**
 *  Defines the number of buses to system memory.
 *  In current implementation: two, one for read and one for writes.
 */
const U32 SYSTEM_MEMORY_BUSES = 2;

// By now this constant should be used to configure the number of DDR Channels
const U32 GPU_MEMORY_CHANNELS = 8;

struct MemoryControllerParameters
{
    enum SchedulerType
    {
        Fifo = 0,
        RWFifo = 1,
        BankQueueFifo = 2,
        BankRWQueueFifo = 3,
        LookAhead = 4
    };

    enum SchedulerPagePolicy
    {
        ClosePage,
        OpenPage
    };

    /////////////////
    // Buses width //
    /////////////////
    U32 comProcBus;
    U32 streamerFetchBus;
    U32 streamerLoaderBus;
    U32 zStencilTestBus;
    U32 colorWriteBus;
    U32 dacBus;
    U32 textUnitBus;

    U32 gpuMemorySize; ///< GPU local memory size (in bytes) (ignored for now)
    U32 systemMemorySize; // System memory size (in bytes)
    U32 numTexUnits;
    U32 systemMemoryBuses;
    U32 numStampUnits;
    U32 streamerLoaderUnits;
    U32 systemMemoryReadLatency;
    U32 systemMemoryWriteLatency;
    U32 requestQueueSize;
    U32 serviceQueueSize;
    U32 readBuffers;
    U32 writeBuffers;
    U32 systemTransactionCycles;

    bool perBankChannelQueues;
    // 0 means "Round & Robin"
    // != 0 means "Oldest first"
    U32 perBankChannelQueuesSelection;
    bool perBankSchedulerState;

    bool memoryTrace; ///< Enables/disables memory trace generation
    U32 memoryChannels; ///< Number of GPU Memory Channels
    U32 banksPerMemoryChannel;
    U32 channelInterleaving;
    U32 bankInterleaving;
    bool enableSecondInterleaving;
    U32 secondChannelInterleaving;
    U32 secondBankInterleaving;
    U32 splitterType;
    std::string* channelInterleavingMask;
    std::string* bankInterleavingMask;
    std::string* secondChannelInterleavingMask;
    std::string* secondBankInterleavingMask;
    //U32 fifo_maxChannelTransactions;
    //U32 rwfifo_maxReadChannelTransactions;
    //U32 rwfifo_maxWriteChannelTransactions;
    U32 maxChannelTransactions;
    U32 dedicatedChannelReadTransactions;
    U32 rwfifo_maxConsecutiveReads;
    U32 rwfifo_maxConsecutiveWrites;

    U32 switchModePolicy;
    U32 bankfifo_activeManagerMode;
    U32 bankfifo_prechargeManagerMode;

    bool bankfifo_disablePrechargeManager;
    bool bankfifo_disableActiveManager;
    U32 bankfifo_managerSelectionAlgorithm;

    // This structure defines all the parameters relative to the underlaying memory used by CG1
    // Currently only GDDR-like memory is supported
    U32 memoryRowSize;
    //U32 burstElementsPerCycle;
    U32 burstBytesPerCycle;
    U32 burstLength;
    GPUMemorySpecs* memSpecs;

    SchedulerType schedulerType;
    std::string* bankfifo_bankSelectionPolicy;
    SchedulerPagePolicy schedulerPagePolicy;

    std::string* debugString;
    bool useClassicSchedulerStates;

    bool useSplitRequestBufferPerROP;
};

class MemoryController : public cmoMduMultiClk
{
private:

    U64 _lastCycle;
    U64 _lastCycleMem;

    MemoryTraceRecorder* memoryTrace;

    bool useInnerSignals; // execute code of inner signals

    /********************************
     * Memory controller statistics *
     ********************************/

    // Global counter stats
    gpuStatistics::Statistic* totalTransStat;
    gpuStatistics::Statistic* totalWriteTransStat;
    gpuStatistics::Statistic* totalReadTransStat;
    gpuStatistics::Statistic* totalWriteBytesStat;
    gpuStatistics::Statistic* totalReadBytesStat;

    // Accounts for cycles causing a read or write stall in channelRequests (scheduler cannot accept reads or writes, backpressure)
    gpuStatistics::Statistic* channelReq_ReadStallCyclesStat;
    gpuStatistics::Statistic* channelReq_WriteStallCyclesStat;

    // Per unit-subunit stats struct
    struct UnitStatsGroup
    {
        gpuStatistics::Statistic* trans;
        gpuStatistics::Statistic* writeTrans;
        gpuStatistics::Statistic* readTrans;
        gpuStatistics::Statistic* writeBytes;
        gpuStatistics::Statistic* readBytes;
        gpuStatistics::Statistic* acceptReadCycles;
        gpuStatistics::Statistic* acceptWriteCycles;
        gpuStatistics::Statistic* readServiceAccumTime;
        gpuStatistics::Statistic* completedReadTrans; // different from readTrans -> readTrans is computed when a request arrive to the MC
        gpuStatistics::Statistic* readServiceTimeAvg; // equivalent to readServiceAccumTime/completedReadTrans 

        void initUnitStatGroup(const std::string& prefix);
    };

    // Per-subunit/channel stats struct
    struct UnitChannelStatsGroup
    {
        gpuStatistics::Statistic* readBytes;
        gpuStatistics::Statistic* writeBytes;
        gpuStatistics::Statistic* readChannelTransAccumTime;
        gpuStatistics::Statistic* completedReadChannelTrans;

        void initUnitChannelStatsGroup(const std::string& prefix, const std::string& postfix);
    };


    // Example of use
    // unitStat[CMDPROC][unit].transStat->inc();
    // unitStat[CMDPROC].back().transStat->inc(); // inc stat per unit type (total Accum)
    std::vector<UnitStatsGroup> unitStats[LASTGPUBUS];

    // unitChannelStats[COLOR][unit][channel].readBytes->inc(amountOfBytes);
    std::vector< std::vector<UnitChannelStatsGroup> > unitChannelStats[LASTGPUBUS];

    // preload transaction counter
    gpuStatistics::Statistic* preloadStat;

    // System memory statistics
    gpuStatistics::Statistic* sysBusReadBytesStat;
    gpuStatistics::Statistic* sysBusWriteBytesStat;

    // Updates readServiceTimeStat & completedReadTransStat
    void updateCompletedReadStats(MemoryRequest& mr, U64 cycle);

    /**
     * @brief Sum of the request buffer size each cycle
     */
    gpuStatistics::Statistic* stat_requestBufferAccumSizeStat;

    /**
     * @brief Statistic keeping the request buffer's average allocated entries
     */
    gpuStatistics::Statistic* stat_avgRequestBufferAllocated;


    gpuStatistics::Statistic* stat_serviceQueueAccumSizeStat;
    /**
     * @brief Statistic keeping the service queue's average requests
     */
    gpuStatistics::Statistic* stat_avgServiceQueueItems;

    // Object used to split memory request objects
    // A second splitter can be created if a second interleaving is provided
    //std::vector<MCSplitter*> splitterArray;
    std::vector<MemoryRequestSplitter*> splitterArray;

    // Register with the start address of the second interleaving
    // (0 means ignore second interleaving)
    U32 MCV2_2ND_INTERLEAVING_START_ADDR;

    ChannelScheduler** channelScheds;
    DDRModule** ddrModules;
    
    Signal** channelRequest; // Output signal matching "ChannelRequest" from ChannelScheduler
    Signal** channelReply;   // Input signal matching "ChannelReply" from ChannelScheduler
    Signal** schedulerState; // Input signal matching  "SchedulerState" from ChannelScheduler
    Signal* mcCommSignal; // Command signal from the command processor

    // Input/output signals
    std::vector<Signal*> requestSignals; // example: commProcWriteSignal
    std::vector<std::vector<Signal*> > dataSignals; // example:commProcReadSignal

    /**
     * System memory signals required to simulate system memory access
     */
    std::vector<Signal*> systemMemoryRequestSignal;
    std::vector<Signal*> systemMemoryDataSignal;
    // used to maintain the arrival time of the in-flight sytem transactions (read)
    tools::Queue<U64> systemTransactionArrivalTime;
    // Check that the arrival time corresponds to the transaction received from system memory
    tools::Queue<U32> systemTransactionArrivalTimeCheckID;

    /**
     *  Signals and structures to generate bandwidth usage information
     */
    std::vector<Signal*> memBusIn[LASTGPUBUS]; ///<  Signals for displaying bandwidth usage
    std::vector<Signal*> memBusOut[LASTGPUBUS]; ///<  Signals for displaying bandwidth usage
    std::vector<std::vector<DynamicObject*> > busElement[LASTGPUBUS]; ///<  Stores the cookies for the current transaction in the unit bus
    std::vector<U32> elemSelect[LASTGPUBUS];  ///<  Which bus element is currently active

    /***************************
     * Memory Controller state *
     ***************************/
    U32 freeReadBuffers; ///< Number of free read buffers (lines)
    U32 freeWriteBuffers; ///< Number of free write buffers (lines)

    /*****************************************
     * GPU Memory request queues and buffers *
     *****************************************/
    MemoryRequest* requestBuffer; ///< GPU Memory request buffer entries
    tools::Queue<U32> freeRequestQueue; ///< Queue storing free entries in the request buffer

    //// Counters to implement a pseudo split Request buffer ///
    std::vector<U32> ropCounters;
    bool useRopCounters;

    /************************************************
     * Per GPU bus read request queues and counters *
     ************************************************/
    // Channel transaction + timestamp
    //tools::Queue<std::pair<ChannelTransaction*, U64> >* channelQueue; ///< Per GPU memory bus read request queues
    
    bool useIndependentQueuesPerBank;
    U32 perBankChannelQueuesSelection; // ALgorithm used to select the target bank if independent queues per bank are used
    tools::Queue<std::pair<ChannelTransaction*, U64> >** channelQueue; ///< Per GPU memory bus read request queues
    // One Round Robin indicator per channel group of queues
    U32* nextBankRR;
    U32 banksPerMemoryChannel;
    
    ///////////////////////////////
    /// System memory variables ///
    ///////////////////////////////
    
    ///<  Mapped system memory request queue
    ///< Array with the transaction beings transmitted from/to system memory
    MemoryRequest* systemRequestBuffer; 
    
    tools::Queue<U32> systemRequestQueue; ///< FIFO with ready requests
    ///< Free list of system memory request entries
    tools::Queue<U32> systemFreeRequestQueue;
    ///< Stores the cycle of the last transaction to a system memory bus
    U64 systemBus[SYSTEM_MEMORY_BUSES];
    ///< Stores the request ID in the systemRequestBuffer of the current memory transaction being processed
    // U32 systemBusID[SYSTEM_MEMORY_BUSES];
    std::queue<U32> systemBusID[SYSTEM_MEMORY_BUSES];

    ///< Array with the transaction beings transmitted from/to system memory
    MemoryTransaction** systemTrans;

    // receive transactions from system memory
    void processSystemMemoryReplies(U64 cycle);

    //std::vector<bool> service[LASTGPUBUS]; ///< If a service is being served in a bus
    //std::vector<U32> currentTrans[LASTGPUBUS]; ///<  Stores a pointer to the current memory transaction in the GPU unit bus
    //std::vector<bool> isSystemTrans[LASTGPUBUS]; ///<  Stores if the current memory transaction in the GPU unit bus is to mapped system memory
    //std::vector<U32> busCycles[LASTGPUBUS]; ///<  Number of cycles until the end of the current bus data transmission
    //std::vector<bool> reserveBus[LASTGPUBUS]; ///< Used by the service queues to reserve the unit bus

    /**
     * Structure keeping the state of one Memory Controller's I/O bus
     */
    struct BusState
    {
        bool service;
        bool isSystemTrans;
        U32 rbEntry; // previously known as 'currentTrans'
        U32 busCycles;
        bool reserveBus;
        // to debug
        const MemoryTransaction* mt;
    };

    std::vector<BusState> _busState[LASTGPUBUS]; ///< State of all Memory Controller's I/O buses

    tools::Queue<MemoryRequest> serviceQueue; ///< Stores the memory read requests to be served to the GPU units

    U08* systemMemory; ///< Pointer to the buffer where the mapped system memory is stored
    U32 systemMemorySize; ///< Amount of system memory (bytes)

    U32 gpuMemorySize;
    U32 gpuBurstLength;
    //U32 gpuMemPageSize;
    //U32 gpuMemOpenPages;
    U32 numTexUnits;
    const char** texUnitsPrefixArray;
    U32 gpuMemoryChannels;
    U32 systemMemoryBuses;
    U32 numStampUnits;
    const char** stampUnitsPrefixArray;
    U32 numStrLoaderUnits;
    const char** streamerLoaderPrefixArray;
    U32 memReadLatency;
    U32 systemMemoryReadLatency;
    U32 systemMemoryWriteLatency;
    U32 requestQueueSize;
    U32 serviceQueueSize;
    // U32 freeServices;

    U32 maxConsecutiveReads;
    U32 maxConsecutiveWrites;
    U32 readBufferSize;
    U32 writeBufferSize;

    // MemoryRequestSplitter& selectSplitter(const MemoryTransaction* mt);
    MemoryRequestSplitter& selectSplitter(U32 address, U32 size) const;

    void processCommand(U64 cycle);

    /**
     * Reset contents of internal structures
     */
    void reset();

    ///////////////////////////////////////
    // Memory controller pipeline stages //
    ///////////////////////////////////////
    void stage_readRequests(U64 cycle);
    void stage_sendToSchedulers(U64 cycle);
    void stage_receiveFromSchedulers(U64 cycle);
    void stage_updateCompletedRequests(U64 cycle);
    void stage_serveRequest(U64 cycle);

    /// Implements the logic required to keep track of bus occupation
     // and transmission duration
    void updateBusCounters(U64 cycle);
    /// Implements the logic managing the buses arbitration 
    void reserveGPUBus(U64 cycle);
    
    // Called inside stage_readRequests for each incoming request
    void processMemoryTransaction(U64 cycle, MemoryTransaction* memTrans);
    
    // Called by processMemoryTransaction: 
    //    Adds a new memory transaction to the GPU memory request queue 
    U32 addRequest(MemoryTransaction* memTrans, U64 cycle);
    // Called by processMemoryTransaction:
    //    Adds a new memory transaction to the GPU memory request queue
    U32 addSystemMemoryRequest(MemoryTransaction* mt, U64 cycle);

    // Create signals to keep track of bus bw (feedback to STV)
    void createUsageSignals(const MemoryControllerParameters& params);
    // Create I/O signals (input/output buses)
    void createIOSignals(); 

    void createStatistics();

    // Read all the usage 
    void readUsageBuses(U64 cycle);

    void writeUsageBuses(U64 cycle);

    // Implements write the data of a memory transaction into 
    // memory modules directly (without timing overhead)
    void preloadGPUMemory(MemoryTransaction* mt);

    // System memory methods
    // Tries to issue a transaction to system memory
    void issueSystemTransaction(U64 cycle);
    void updateSystemBuses(U64 cycle);

    void sendBusStateToClients(U64 cycle);
    // called over all GPU units by sendBusStateToClients
    void sendBusState(U64 cycle, GPUUnit unit, MemState state);

    //  Clock/update the GPU clock domain of the Memory Controller.
    void updateGPUDomain(U64 cycle);
    
    //  Clock/update the Memory clock domain of the Memory Controller.
    void updateMemoryDomain(U64 cycle);

    static std::string getRangeList(const std::vector<U32>& listOfIndices);

public:

    // To initialize the memory controller with a default set of parameters
    static MemoryControllerParameters MemoryControllerDefaultParameters;

    MemoryController(const MemoryControllerParameters& params, 
                     const char** texUnitsPrefixArray,
                     const char** stampUnitsPrefixArray,
                     const char** streamerLoaderPrefixArray,
                     const char* name,
                     bool createInnerSignals = true,
                     cmoMduBase* parent = 0);

    void clock(U64 cycle);
    //  Clock update function for multiple clock domain support.
    void clock(U32 domain, U64 cycle);

    /** 
     *
     *  Returns a list of the debug commands supported by the Memory Controller.
     *
     *  @param commandList Reference to a string where to store the list of debug commands supported by 
     *  the Memory Controller.
     *
     */

    void getCommandList(string &commandList);

    /** 
     *
     *  Executes a debug command.
     *
     *  @param commandStream A string stream with the command and parameters to execute.
     *
     */

    void execCommand(stringstream &commandStream);

    void saveMemory() const;

    void loadMemory();

    void getDebugInfo(std::string &debugInfo) const;

};


} // namespace memorycontroller
} // namespace cg1gpu

#endif // MEMORYCONTROLLER_H
