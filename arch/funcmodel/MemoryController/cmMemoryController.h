/**************************************************************************
 *
 * Memory Controller class definition file.
 *
 */

/**
 *
 *  @file cmMemoryController.h
 *
 *  Memory Controller definition file.  The Memory Controller class defines the mdu
 *  controlling the access to GPU and system memory and servicing memory requests
 *  from the other GPU units.
 *
 */

#ifndef _MEMORY_CONTROLLER_
    #define _MEMORY_CONTROLLER_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmMemorySpace.h"
#include "cmMemoryControllerDefs.h"

namespace arch
{
    class MemoryTransaction;

/**
 *
 *  Stores the bus width (in bytes) between the Memory Controller and
 *  the GPU units.
 *
 */
//extern U32 busWidth[];

/**
 *
 *  Strings with the names of the GPU units connected to the Memory Controller.
 *
 */
//extern char *busNames[];

//*  Defines a memory request queue entry.  
struct MemoryRequest
{
    MemoryTransaction *memTrans;    //  The memory transaction that carries the request.  
    MemReqState state;              //  Current state of the request.  
    U32 waitCycles;              //  Number of cycles the request has been waiting to start being processed.  
    U32 transCycles;             //  Number of cycles the request has been transmiting data.  
    bool dependency;                //  Flag that stores if the transaction has a dependency.  
    U32 waitForTrans;            //  Pointer/identifier to the transaction that must precede the memory request.  
    bool wakeUpWaitingTrans;        //  Flag that stores if the transaction, when served must wake up dependant transactions.  
};

//**  Maximum number of memory transaction ids availables.  
//static const U32 MAX_MEMORY_TICKETS = 256;

//*  Read transaction latency.  
//static const U32 READ_LATENCY = 10;

//*  Write transaction latency.  
//static const U32 WRITE_LATENCY = 5;

//*  Memory transaction transmission cycles (to physical memory).  
static const U32 TRANSACTION_CYCLES = 4;

//** Maximum number of buses per GPU unit type.  
static const U32 MAX_GPU_UNIT_BUSES = 16;

//** Maximum number of consecutive read requests that can be processed when there are pending write requests.  
//static const U32 MAX_CONSECUTIVE_READS = 8;

//** Maximum number of consecutive write requests that can be processed when there are pending read requests.  
//static const U32 MAX_CONSECUTIVE_WRITES = 8;

/**
 *
 *  Defines the latency to system memory in cycles.
 *
 */
static const U32 MAPPED_MEMORY_LATENCY = 500;

/**
 *
 *  Defines the cycles it takes to transmit a memory transaction from/to system memory.
 *
 */
static const U32 MAPPED_TRANSACTION_CYCLES = 16;

/**
 *
 *  Defines the number of buses to system memory.
 *
 *  In current implementation: two, one for read and one for writes.
 *
 */
static const U32 MAPPED_MEMORY_BUSES = 2;

/**
 *
 *  Implements a Memory Controller cmoMduBase.
 *
 *  Simulates the GPU memory controller.
 *
 */

class MemoryController: public cmoMduBase
{

private:

    U64 _lastCycle;

    // Defined here for compatibility reasons
    // each entry is initialized with (example):
    // busNames[COMMANDPROCESSOR] = MemoryTransaction::getBusName(COMMANDPROCESSOR);
    const char* busNames[LASTGPUBUS];

    //  Memory Controller parameters.  
    U32 gpuMemorySize;       //  Amount of GPU local memory in bytes.  
    U32 baseClockMultiplier; //  Frequency multiplier applied to the GPU frequency to be used as the reference memory frequency.  
    U32 gpuMemoryFrequency;  //  Memory frequency in clock cycles from the reference memory frequency.  
    U32 gpuMemoryBusWidth;   //  Width in bits of each GPU memory bus.  
    U32 gpuMemoryBuses;      //  Number of data/control buses to memory.  
    bool gpuSharedBanks;        //  Flag that stores if the gpu memory buses access shared or distributed memory banks.  
    U32 gpuBankGranurality;  //  Access granurality in bytes for the access to the distributed gpu memory banks.  
    U32 gpuMemoryBurstLength;//  Length of a gpu memory burst data ccess in words (bus width).  
    U32 gpuMemReadLatency;   //  Cycles until data is received from GPU memory after a read command.  
    U32 gpuMemWriteLatency;  //  Cycles until data can be sent to GPU memory after a write command.  
    U32 gpuMemWriteToReadLatency;    //  Cycles to wait after data was written to the GPU memory before issuing a read command.  
    U32 gpuMemPageSize;      //  Size of a GPU memory page.  
    U32 gpuMemOpenPages;     //  Number of pages open in a GPU memory chip.  
    U32 gpuMemPageOpenLat;   //  Latency for opening a page in a GPU memory chip.  
    U32 maxConsecutiveReads; //  Number of consecutive read transactions than can be processed before processing a write transaction.  
    U32 maxConsecutiveWrites;//  Number of consecutive write transactions than can be processed before processing a read transaction.  
    U32 mappedMemorySize;    //  Amount of system memory mapped system into the GPU address space in bytes.  
    U32 readBufferSize;      //  Size of the Memory Controller buffer for read transactions in lines.  
    U32 writeBufferSize;     //  Size of the Memory Controller buffer for write transactions in lines.  
    U32 requestQueueSize;    //  Number of entries of the memory request queue.  
    U32 serviceQueueSize;    //  Number of entries in the memory service queues per GPU unit buses.  
    U32 numStampUnits;       //  Number of stamp pipes (Z Stencil and Color Write) attached to the Memory Controller.  
    U32 numTextUnits;        //  Number of Texture Units attached to the Memory Controller.  
    U32 streamerLoaderUnits; //  Number of cmoStreamController Loader units attached to the Memory Controller.  

    //  Precalculated constants.  
    U32 busShift;            //  Bit shift required to get the bank for a given GPU memory address.  
    U32 busOffsetMask;       //  Mask for the offset inside a gpu memory bank.  
    U32 pageShift;           //  Bit shift required to get the memory page identifier.  
    U32 pageBankShift;       //  Bit shift required to get the memory page bank identifier.  
    U32 pageBankMask;        //  Mask for require to get the memory page bank identifier.  

    //  Memory buffers.  
    U08 *gpuMemory;       //  Pointer to the buffer where the GPU local memory is stored.  
    U08 *mappedMemory;    //  Pointer to the buffer where the mapped system memory is stored.  

    /**
     * Command signal from the Command Processor.
     */
    Signal* mcCommSignal;

    //  Memory Controller signals.  
    Signal *commProcReadSignal;     //  Memory read/status signal to the Command Processor.  
    Signal *commProcWriteSignal;    //  Memory write/request signal from the Command Processor.  
    Signal **streamLoadDataSignal;  //  Memory read/status signal to the cmoStreamController Loader units.  
    Signal **streamLoadReqSignal;   //  Memory request signal to the cmoStreamController Loader units.  
    Signal *streamFetchDataSignal;  //  Memory data signal to the cmoStreamController Fetch unit.  
    Signal *streamFetchReqSignal;   //  Memory request signal from the cmoStreamController Fetch.  
    Signal **zStencilDataSignal;    //  Array of memory data signal to the Z Stencil Test unit.  
    Signal **zStencilReqSignal;     //  Array of memory request signal from Z Stencil Test unit.  
    Signal **colorWriteDataSignal;  //  Array of memory data signal to the Color Write unit.  
    Signal **colorWriteReqSignal;   //  Array of memory request signal from Color Write unit.  
    Signal *dacDataSignal;          //  Memory data signal to the DisplayController unit.  
    Signal *dacReqSignal;           //  Memory request signal from DisplayController unit.  
    Signal **tuDataSignal;          //  Array of memory request signals from the Texture Units.  
    Signal **tuReqSignal;           //  Array of memory data signals to the Texture Units.  
    Signal **memoryModuleRequestSignal; //  Request signal to the memory modules (simulates memory access latency).  
    Signal **memoryModuleDataSignal;    //  Data signal from the memory modules (simulates memory access latency).  
    Signal **mappedMemoryRequestSignal; //  Request signal to the mapped system memory (simulates system memory access latency).  
    Signal **mappedMemoryDataSignal;    //  Data signal from the mapped system memory (simulates system memory access latency).  

    //  Signals and structures to generate bandwidth usage information.  
    Signal *memBusIn[LASTGPUBUS][MAX_GPU_UNIT_BUSES];   //  Signals for displaying bandwidth usage.  
    Signal *memBusOut[LASTGPUBUS][MAX_GPU_UNIT_BUSES];  //  Signals for displaying bandwidth usage.  
    DynamicObject busElement[LASTGPUBUS][MAX_GPU_UNIT_BUSES][2];    //  Stores the cookies for the current transaction in the unit bus.  
    U32 elemSelect[LASTGPUBUS][MAX_GPU_UNIT_BUSES];  //  Which bus element is currently active.  

    //  Memory Controller state.  
    U32 freeReadBuffers;     //  Number of free read buffers (lines).  
    U32 freeWriteBuffers;    //  Number of free write buffers (lines).  
    U32 memoryCycles;        //  Stores the sum of the cycles until the end of all the current gpu memory bus data transmissions.  
    U32 systemCycles;        //  Stores the sum of the cycles until the end of all the current system memory bus data transmissions.  
    U32 busCycles[LASTGPUBUS][MAX_GPU_UNIT_BUSES];   //  Number of cycles until the end of the current bus data transmission.  
    bool reserveBus[LASTGPUBUS][MAX_GPU_UNIT_BUSES];    //  Used by the service queues to reserve the unit bus.  
    U32 currentTrans[LASTGPUBUS][MAX_GPU_UNIT_BUSES];//  Stores a pointer to the current memory transaction in the GPU unit bus.  
    bool mappedTrans[LASTGPUBUS][MAX_GPU_UNIT_BUSES];   //  Stores if the current memory transaction in the GPU unit bus is to mapped system memory.  
    U32 **currentPages;      //  Stores the identifiers of the memory pages currently open per bus.  

    //  GPU memory request queues and buffers.  
    MemoryRequest *requestBuffer;   //  GPU memory request buffer.  
    U32 numRequests;             //  Number requests for all the GPU memory buses.  
    U32 *freeRequestQ;           //  Queue storing free entries in the request buffer.  
    U32 numFreeRequests;         //  Number of free entries in the request buffer.  

    //  Per GPU bus read request queues, pointers and counters.  
    U32 *lastReadReq;            //  Pointers to the last read request in the read request queues.  
    U32 *firstReadReq;           //  Pointers to the first read request in the read request queues.  
    U32 **readRequestBusQ;       //  Per GPU memory bus read request queues.  
    U32 *numReadBusRequests;     //  Number of read requests per GPU memory bus.  
    U32 *consecutiveReadReqs;    //  Number of consecutive read requests selected per GPU memory bus.  

    //  Per GPU bus read request queues, pointers and counters.  
    U32 *lastWriteReq;           //  Pointers to the last write request in the write request queues.  
    U32 *firstWriteReq;          //  Pointers to the first write request in the write request queues.  
    U32 **writeRequestBusQ;      //  Per GPU memory bus write request queues.  
    U32 *numWriteBusRequests;    //  Number of write requests per GPU memory bus.  
    U32 *consecutiveWriteReqs;   //  Number of consecutive write requests selected per GPU memory bus.  

    U32 *numBusRequests;         //  Number of requests per GPU memory bus.  

    //  Per memory bus state for the current memory transference.  
    MemTransCom *lastCommand;       //  Array storing the command/type of the last transaction issued per memory bus.  
    U64 *lastCycle;              //  Array storing the cycle when the last transaction was issued per memory bus.  
    MemoryTransaction **moduleTrans;//  Array with the transaction being transmitted from/to the memory modules.  
    U32 *currentBusRequest;      //  Pointer to the current request being served in each GPU memory bus.  
    U32 *activeBusRequests;      //  Stores the number of memory requests active in the pipeline (signal latency).  

    //  Mapped memory request queue variables.  
    MemoryRequest *mappedQueue;     //  Mapped system memory request queue.  
    U32 lastMapped;              //  Pointer to the last request.  
    U32 firstMapped;             //  Pointer to the first request.  
    U32 *readyMapped;            //  Ready request FIFO.  
    U32 numReadyMapped;          //  Number of ready requests.  
    U32 *freeMapped;             //  Free request entries list.  
    U32 numFreeMapped;           //  Number of free request entries.  
    MemoryTransaction **systemTrans;//  Array with the transaction beings transmitted from/to the memory modules.  
    U64 systemBus[MAPPED_MEMORY_BUSES];  //  Stores the cycle of the last transaction to a system memory bus.  

    //  Service queues.  
    MemoryRequest *serviceQueue;    //  Stores the memory read requests to be served to the GPU units.  
    U32 nextService;             //  Pointer to the next request to serve.  
    U32 freeServices;            //  Number of free service entries in the service queue.  
    U32 pendingServices;         //  Number of pending service entries in the service queue.  
    U32 nextFreeService;         //  Pointer to the next free service entry in the service queue.  
    bool service[LASTGPUBUS][MAX_GPU_UNIT_BUSES];   //  If a service is being served in a bus (not a WRITE!!!).  

    //  Statistics.  

    //  Per GPU Unit type statistics.  
    gpuStatistics::Statistic *transactions[LASTGPUBUS]; //  Transactions received per GPU unit.  
    gpuStatistics::Statistic *writeTrans[LASTGPUBUS];   //  Write transactions received per GPU unit.  
    gpuStatistics::Statistic *readTrans[LASTGPUBUS];    //  Read transactions received per GPU Unit.  
    gpuStatistics::Statistic *writeBytes[LASTGPUBUS];   //  Bytes written per GPU unit.  
    gpuStatistics::Statistic *readBytes[LASTGPUBUS];    //  Bytes read per GPU unit.  

    //  Memory controller statistics.  
    gpuStatistics::Statistic *unusedCycles;             //  Cycles in which the memory module is unused.  
    gpuStatistics::Statistic *preloadTrans;             //  Preload transactions received.  

    //  GPU memory statistics.  
    gpuStatistics::Statistic **gpuBusReadBytes;         //  Bytes read from a GPU memory bus.  
    gpuStatistics::Statistic **gpuBusWriteBytes;        //  Bytes written to a GPU memory bus.  
    gpuStatistics::Statistic **dataCycles;              //  Cycles in which the GPU memory bus was transmitting data.  
    gpuStatistics::Statistic **rtowCycles;              //  Cycles in which the GPU memory bus was penalized for read to write transaction change.  
    gpuStatistics::Statistic **wtorCycles;              //  Cycles in which the GPU memory bus was penalized for write to read transaction change.  
    gpuStatistics::Statistic **openPageCycles;          //  Cycles in which the GPU memory bus was penalized for opening a new memory page.  
    gpuStatistics::Statistic **openPages;               //  Number of new pages open per GPU memory bus.  

    //  System memory statistics.  
    gpuStatistics::Statistic *sysBusReadBytes;          //  Bytes read from system/mapped memory bus.  
    gpuStatistics::Statistic *sysBusWriteBytes;         //  Bytes written to system/mapped memory bus.  
    gpuStatistics::Statistic *sysDataCycles[MAPPED_MEMORY_BUSES];   //  Cycles in which the system memory bus was transmitting data.  


    void processCommand(U64 cycle);

    //  Private functions.  

    /**
     *
     *  Resets the memory controller.
     *
     */

    void reset();

    /**
     *
     *  Adds a new memory transaction to the gpu memory request queue.
     *
     *  @param memTrans A pointer to the Memory Transaction to add to the queue.
     *
     *  @return A pointer to the request queue entry where the transaction has been stored.
     *
     */

    U32 addRequest(MemoryTransaction *memTrans);

    /*
     *  Wake up read transactions for a memory bus when a write transaction source of dependencies is processed.
     *
     *  @param bus The read request bus queue where read transactions must be waken up.
     *  @param writeTrans The pointer/identifier of the write transactions that was finished.
     *
     */

    void wakeUpReadRequests(U32 bus, U32 wrTrans);

    /**
     *
     *  Wake up write transactions for a memory bus when a read transaction source of dependencies is processed.
     *
     *  @param bus The write request bus queue where write transactions must be waken up.
     *  @param readTrans The pointer/identifier of the read transactions that was finished.
     *
     */

    void wakeUpWriteRequests(U32 bus, U32 rdTrans);

    /**
     *
     *  Removes a memory request from the gpu memory queue.
     *
     *  @param req The position inside the memory request queue of the request to remove.
     *
     */

    void removeRequest(U32 req);

    /**
     *
     *  Adds a new memory transaction to the mapped system memory request queue.
     *
     *  @param memTrans A pointer to the Memory Transaction to add to the queue.
     *
     *  @return A pointer to the request queue entry where the transaction has been stored.
     *
     */

    U32 addMappedRequest(MemoryTransaction *memTrans);

    /**
     *
     *  Removes a memory request from the mapped system memory queue.
     *
     *  @param req The position inside the memory request queue of the request to remove.
     *
     */

    void removeMappedRequest(U32 req);


    /**
     *
     *  Selects the next memory request to process for a GPU memory bus.
     *
     *  @param bus GPU memory bus for which to select the memory request.
     *
     *  @return Pointer to the request buffer entry storing the request to process.
     *
     */

    U32 selectNextRequest(U32 bus);

    /**
     *
     *  Updates the read request queue pointers and counters for a GPU memory bus after the selected
     *  read request has been processed.
     *
     *  @param bus GPU memory bus for which to select the memory request.
     *
     */

    void updateReadRequests(U32 bus);

    /**
     *
     *  Updates the read request queue pointers and counters for a GPU memory bus after the selected
     *  read request has been processed.
     *
     *  @param bus GPU memory bus for which to select the memory request.
     *
     */

    void updateWriteRequests(U32 bus);

    /**
     *
     *  Receives a memory transaction from a GPU bus.
     *
     *  @param memTrans The received memory transaction.
     *
     */

    void receiveTransaction(MemoryTransaction *memTrans);

    /**
     *
     *  Tries to issue a memory transaction to a memory module.
     *
     *  @param cycle Current simulation cycle.
     *  @param bus Bus for which to send a transaction.
     *
     */

    void issueTransaction(U64 cycle, U32 bus);

    /**
     *
     *  Tries to issue a memory transaction to system memory.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void issueSystemTransaction(U64 cycle);

    /**
     *
     *  Tries to serve a memory request to the GPU unit that generated it.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void serveRequest(U64 cycle);

    /**
     *
     *  Reserves the GPU unit bus the next request to serve in the service queue.
     *
     */

    void reserveGPUBus();

    /**
     *
     *  Sends a transaction with the state of the data bus with the Memory Controller
     *  to the GPU unit.
     *
     *  @param cycle The current simulation cycle.
     *  @param unit The unit to which send the state update.
     *  @param mcState State of the memory controller (if there are enough request entries
     *  to allow transactions from the units).
     *  @param dataSignal Pointer to the data/state signal with the GPU unit.
     *
     */

    void sendBusState(U64 cycle, GPUUnit unit, MemState mcState, Signal *dataSignal);

    /**
     *
     *  Sends a transaction with the state of the data bus with the Memory Controller
     *  to the GPU unit.  Version for units that support replication.
     *
     *  @param cycle The current simulation cycle.
     *  @param unit The unit to which send the state update.
     *  @param mcState State of the memory controller (if there are enough request entries
     *  to allow transactions from the units).
     *  @param dataSignal Pointer to the data/state signal with the GPU unit.
     *  @param units Number of replicated units.
     *
     */

    void sendBusState(U64 cycle, GPUUnit unit, MemState mcState, Signal **dataSignal, U32 units);

    /**
     *
     *  Processes a transaction received back from the memory module.
     *
     *  @param cycle Current simulation cycle.
     *  @param bus Memory module bus from which to receive transactions.
     *
     */

    void moduleTransaction(U64 cycle, U32 bus);

    /**
     *
     *  Updates the state of a gpu memory bus.
     *
     *  @param cycle Current simulation cycle.
     *  @param bus Memory module bus to update.
     *
     */

    void updateMemoryBus(U64 cycle, U32 bus);

    /**
     *
     *  Processes a transaction received back from the system memory bus.
     *
     *  @param cycle Current simulation cycle.
     *  @param bus Memory module bus from which to receive transactions.
     *
     */

    void mappedTransaction(U64 cycle, U32 bus);

    /**
     *
     *  Updates the state of a system memory bus.
     *
     *  @param cycle Current simulation cycle.
     *  @param bus Memory module bus to update.
     *
     */

    void updateSystemBus(U64 cycle, U32 bus);

    /**
     *
     *  Calculates the gpu memory bank being accessed by a gpu memory address
     *  based on the access bank granurality.
     *
     *  @param address GPU memory address for which to determine the bank
     *  accessed.
     *
     *  @return The non shared GPU memory bank associated with a gpu memory bus
     *  that is being accessed by the given address.
     *
     */

    U32 address2Bus(U32 address);

    /**
     *
     *  Search the open page table for bus.
     *
     *  @param address Address being accessed
     *  @param bus Which bus is being accessed.
     *
     *  @return If the page for the address to be accessed is in the current open page
     *  directory for the bus.
     *
     */

    bool searchPage(U32 address, U32 bus);

    /**
     *
     *  Update per bus open page table with a new page.
     *
     *  @param address Address being accessed
     *  @param bus Which bus is being accessed.
     *
     */

    void openPage(U32 address, U32 bus);


public:

    /**
     *
     *  Memory Controller cmoMduBase constructor.
     *
     *  This function creates and initializes a Memory Controller cmoMduBase.
     *
     *  @param memSize Amount of GPU memory available for the memory controller.
     *  @param clockMult Frequency multiplier applied to the GPU frequency to be used as the memory reference frequency.
     *  @param memFreq Memory frequency as a number of clock cycles from the memory reference frequency.
     *  @param busWidth Width in bits of each independant GPU memory bus.
     *  @param memBuses Number of independant GPU memory buses.
     *  @param sharedBanks Enables access from the gpu memory buses to a shared memory array.
     *  @param bankGranurality Access granurality when shared access through the gpu memory bus is disabled.
     *  @param burstLength Numbers of words (bus width) transfered with each memory access.
     *  @param readLatency Cycles from read command to data from gpu memory.
     *  @param writeLatency Cycles from write command to data towards gpu memory.
     *  @param writeToReadLatency Cycles after last written data until next read command.
     *  @param pageSize Size of GPU memory page in bytes.
     *  @param openPages Number of open pages in a GPU memory chip.
     *  @param pageOpenLat Latency for opening a page in a GPU memory chip.
     *  @param consecutiveReads Number of consecutive read transactions that can be processed before processing the next write transaction.
     *  @param consecutiveWrites Number of consecutive read transactions that can be processed before processing the next write transaction.
     *  @param comProcBus Command Processor data bus width.
     *  @param streamerFetchBus cmoStreamController Fetch data bus width.
     *  @param streamerLoaderBus cmoStreamController Loader data bus width.
     *  @param zStencilTestBus Z Stencil Tes data bus widht.
     *  @param colorWriteBus Color Write data bus width.
     *  @param dacBus DisplayController data bus width.
     *  @param systemMem Amount of system memory mapped into the GPU address space.
     *  @param readBufferSize Size of the memory controller write transaction buffer (lines).
     *  @param writeBufferSize Size of the memory controller read transaction buffer (lines).
     *  @param reqQueueSize Number of entries in the memory request queue.
     *  @param serviceQueueSize Number of entries in the service queues for the GPU buses.
     *  @param numStampUnits Number of stamp pipes (Z Stencil and Color Write) attached to the Memory Controller.
     *  @param suPrefixArray Array of the signal name prefixes for the attached stamp units.
     *  @param numTxUnits Number of Texture Units attached to the Memory Controller.
     *  @param tuPrefixArray Array of signal name prefixes for the attached Texture Units.
     *  @param streamerLoaderUnits Number of cmoStreamController Loader Units.
     *  @param slPrefixArray Array of signal name prefixes for the attached cmoStreamController Loader units.
     *  @param name cmoMduBase name.
     *  @param parent Parent cmoMduBase.
     *
     *  @return An initialized Memory Controller cmoMduBase.
     *
     */

    MemoryController(U32 memSize, U32 clockMult, U32 memFreq, U32 busWidth, U32 memBuses,
        bool sharedBanks, U32 bankGranurality, U32 burstLength, U32 readLatency, U32 writeLatency,
        U32 writeToReadLatency, U32 pageSize, U32 openPages, U32 pageOpenLat,
        U32 consecutiveReads, U32 consecutiveWrites,
        U32 comProcBus, U32 streamerFetchBus, U32 streamerLoaderBus, U32 zStencilTestBus,
        U32 colorWriteBus, U32 dacBus, U32 textUnitBus,
        U32 systemMem, U32 readBufferSize, U32 writeBufferSize,
        U32 reqQueueSize, U32 serviceQueueSize,
        U32 numStampUnits, const char **suPrefixArray, U32 numTxUnits, const char **tuPrefixArray,
        U32 streamerLoaderUnits, const char **slPrefixArray, char *name, cmoMduBase* parent = 0);

    /**
     *
     *  The clock function implements the cycle accurated simulation
     *  of the Memory Controller mdu.
     *
     *  @param cycle The cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /** 
     *
     *  Returns a list of the debug commands supported by the Memory Controller.
     *
     *  @param commandList Reference to a string where to store the list of debug commands supported by 
     *  the Memory Controller.
     *
     */

    void getCommandList(std::string &commandList);

    /** 
     *
     *  Executes a debug command.
     *
     *  @param commandStream A string stream with the command and parameters to execute.
     *
     */

    void execCommand(std::stringstream &commandStream);

    /**
     *  
     *  Saves the content of the GPU and system memory to a file.
     *
     */
     
    void saveMemory();

    /**
     *
     *  Loads the content of the GPU and system memory from a file.
     *
     */    
     
    void loadMemory();

    
    void getDebugInfo(std::string &debugInfo) const;
    
    
};

} // namespace arch

#endif
