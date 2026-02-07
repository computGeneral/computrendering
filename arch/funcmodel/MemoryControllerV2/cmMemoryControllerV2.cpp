/**************************************************************************
 *
 */

#include "cmMemoryControllerV2.h"
#include "cmSchedulerSelector.h"
#include "cmMemoryControllerCommand.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "cmMemorySpace.h"
#include "GPUMath.h"
#include "cmMemoryRequest.h"
#include "cmFifoScheduler.h"

#include "cmMCSplitter.h" // uses byte interleaving for selecting channel and bank
#include "cmMCSplitter2.h" // uses bit masks for selecting channel and bank

#include "cmGPUMemorySpecs.h"
#include "cmMemoryTraceRecorder.h"

using namespace std;
using namespace cg1gpu::memorycontroller;

#ifdef GPU_DEBUG
    #undef GPU_DEBUG
#endif
//#define GPU_DEBUG(expr) { expr }
#define GPU_DEBUG(expr) { }


#define DEBUG_CYCLE(expr) { if (cycle > 2105) { expr } }


// Import cg1gpu symbols
using cg1gpu::tools::Queue; // to use our model of Queue (cg1gpu::tools::Queue)
using cg1gpu::GPUUnit;
using cg1gpu::MemState;
using cg1gpu::MemoryTransaction;

MemoryControllerParameters MemoryController::MemoryControllerDefaultParameters =
{
    512*1024*1024, // 512 MB of GDDR3
    128*1024*1024 // 128 MB of system memory

};

MemoryController::MemoryController(
    const MemoryControllerParameters& params, // params
    const char** texUnitsPrefixArray,
    const char** stampUnitsPrefixArray,
    const char** streamerLoaderPrefixArray,
    const char* name, // mdu name
    bool createInnerSignals, // create and use inner signals (STV feedback)
    cmoMduBase* parent )  // parent mdu
        :
    cmoMduMultiClk(name, parent),
    useInnerSignals(createInnerSignals),
    memoryTrace(0),
    useIndependentQueuesPerBank(params.perBankChannelQueues),
    perBankChannelQueuesSelection(params.perBankChannelQueuesSelection),
    banksPerMemoryChannel(params.banksPerMemoryChannel),
    texUnitsPrefixArray(texUnitsPrefixArray),
    stampUnitsPrefixArray(stampUnitsPrefixArray),
    streamerLoaderPrefixArray(streamerLoaderPrefixArray),
    MCV2_2ND_INTERLEAVING_START_ADDR(0), // by default ignore a second interleaving
    useRopCounters(params.useSplitRequestBufferPerROP),
    _lastCycle(0)
{
    if ( params.memoryTrace ) {
        memoryTrace = new MemoryTraceRecorder();
        memoryTrace->open("memorytrace.txt.gz");
    }

    // memorySize = #channels * #banks * #rows * rowSize
    // rowSize = 4 * #cols
    // #rows = memorySize / #channels * #banks * rowSize
    if ( params.splitterType == 0 )
    {
        splitterArray.push_back( new MCSplitter(
                params.burstLength, params.memoryChannels, params.banksPerMemoryChannel,
                params.gpuMemorySize / (params.memoryChannels * params.banksPerMemoryChannel *
                params.memoryRowSize),
                params.memoryRowSize / 4,
                params.channelInterleaving,
                params.bankInterleaving
            ));
        if ( params.enableSecondInterleaving )
        {
            splitterArray.push_back( new MCSplitter(
                params.burstLength, params.memoryChannels, params.banksPerMemoryChannel,
                params.gpuMemorySize / (params.memoryChannels * params.banksPerMemoryChannel *
                                    params.memoryRowSize),
                params.memoryRowSize / 4,
                params.secondChannelInterleaving,
                params.secondBankInterleaving
                ));
        }
    }
    else if ( params.splitterType == 1 )
    {
        splitterArray.push_back( new MCSplitter2(
                params.burstLength, params.memoryChannels, params.banksPerMemoryChannel,
                params.gpuMemorySize / (params.memoryChannels * params.banksPerMemoryChannel *
                params.memoryRowSize),
                params.memoryRowSize / 4,
                *params.channelInterleavingMask,
                *params.bankInterleavingMask
            ));
        if ( params.enableSecondInterleaving )
        {
            splitterArray.push_back( new MCSplitter2(
                params.burstLength, params.memoryChannels, params.banksPerMemoryChannel,
                params.gpuMemorySize / (params.memoryChannels * params.banksPerMemoryChannel *
                                    params.memoryRowSize),
                params.memoryRowSize / 4,
                *params.secondChannelInterleavingMask,
                *params.secondBankInterleavingMask
                ));
        }
    }
    else {
        stringstream ss;
        ss << "Splitter type " << params.splitterType << " unsupported";
        CG_ASSERT(ss.str().c_str());
    }


    U32 bankRows =  params.gpuMemorySize /
                            (params.memoryChannels * params.banksPerMemoryChannel *
                             params.memoryRowSize);

    U32 bankCols = params.memoryRowSize / 4;

    ///////////////////////////
    // Configure buses width //
    ///////////////////////////
    MemoryTransaction::setBusWidth(COMMANDPROCESSOR,params.comProcBus);
    MemoryTransaction::setBusWidth(STREAMERFETCH,params.streamerFetchBus);
    MemoryTransaction::setBusWidth(STREAMERLOADER,params.streamerLoaderBus);
    MemoryTransaction::setBusWidth(ZSTENCILTEST,params.zStencilTestBus);
    MemoryTransaction::setBusWidth(COLORWRITE,params.colorWriteBus);
    MemoryTransaction::setBusWidth(DISPLAYCONTROLLER,params.dacBus);
    MemoryTransaction::setBusWidth(TEXTUREUNIT,params.textUnitBus);

    ///////////////////////////////////////
    // Read memory controller parameters //
    ///////////////////////////////////////
    systemMemorySize = params.systemMemorySize;
    numTexUnits = params.numTexUnits;
    systemMemoryBuses = params.systemMemoryBuses;
    numStampUnits = params.numStampUnits;
    numStrLoaderUnits = params.streamerLoaderUnits;
    systemMemoryReadLatency = params.systemMemoryReadLatency;
    systemMemoryWriteLatency = params.systemMemoryWriteLatency;
    freeReadBuffers = params.readBuffers;
    freeWriteBuffers = params.writeBuffers;
    requestQueueSize = params.requestQueueSize;
    gpuMemoryChannels = params.memoryChannels;
    gpuMemorySize = params.gpuMemorySize;
    gpuBurstLength = params.burstLength;

    /////////////////////////////////////////
    // Check Memory Controller consistency //
    /////////////////////////////////////////
    GPU_ASSERT(

        if ( gpuMemoryChannels != 1 && gpuMemoryChannels != 2 &&
             gpuMemoryChannels != 4 && gpuMemoryChannels != 8 && gpuMemoryChannels != 16 ) {
             CG_ASSERT(
                   "Number of memory channels must be 1, 2, 4 or 8 for the current implementation");
        }

        if ( params.banksPerMemoryChannel != 1 && params.banksPerMemoryChannel != 2 &&
             params.banksPerMemoryChannel != 4 && params.banksPerMemoryChannel != 8 ) {
             CG_ASSERT(
                   "Number of banks per channel must be 1, 2, 4 or 8 for the current implementation");
        }

        if ( gpuMemorySize == 0 )
             CG_ASSERT("The amount of memory defined must be greater than 0");

        const U32 pagesBanksChannelsSz = gpuMemoryChannels * params.banksPerMemoryChannel * params.memoryRowSize;

        if ( gpuMemorySize % pagesBanksChannelsSz != 0 ) {
             char aux[256];
             sprintf(aux, "gpuMemorySize (%u bytes) must be a multiple of channels*banks*rowSize (%u bytes)",
                          gpuMemorySize, pagesBanksChannelsSz);
             CG_ASSERT(aux);
        }

        // max memory supported by the model
        const U32 maxMemorySz = (0xFFFFFFFF / pagesBanksChannelsSz) * pagesBanksChannelsSz;

        if ( gpuMemorySize > maxMemorySz ) {
             char aux[256];
             sprintf(aux, "Maximum memory size supported (%u bytes) exceed. Trying to define: %u bytes", maxMemorySz, gpuMemorySize);
             CG_ASSERT(aux);
        }

        if (params.requestQueueSize < ((numTexUnits + numStampUnits + numStrLoaderUnits + 3)*2))
            CG_ASSERT("Minimum number of request queue entries is: "
               "(numTextUnits + numStampsUnits + numStrLoaderUnits + 3) * 2");
        if ( params.serviceQueueSize == 0 )
            CG_ASSERT("Service queue defined with size 0");
        if (freeReadBuffers == 0)
            CG_ASSERT(
                 "Minimum number of read buffer lines required is 1");
        if (freeWriteBuffers == 0)
            CG_ASSERT(
                  "Minimum number of write buffer lines required is 1");
        if ( numStampUnits == 0 )
            CG_ASSERT("There should be at least a stamp pipe");
        if ( stampUnitsPrefixArray == 0 )
            CG_ASSERT("The stamp unit signal name prefix array can not be null");
        for ( U32 i = 0; i < numStampUnits; i++ )
        {
            if ( stampUnitsPrefixArray[i] == 0 )
                CG_ASSERT("Stamp unit signal name prefix is NULL");
        }

        if ( numStrLoaderUnits == 0 )
            CG_ASSERT("There should be at least a cmoStreamController Loader Unit");

        if ( streamerLoaderPrefixArray == 0 )
            CG_ASSERT("The StreamController loader unit signal name prefix array can not be null");

        for ( U32 i = 0; i < numStrLoaderUnits; i++ )
        {
            if ( streamerLoaderPrefixArray[i] == 0 )
                CG_ASSERT("cmoStreamController Loader unit signal name prefix is NULL");
        }

        if ( numTexUnits == 0 )
            CG_ASSERT("There should be at least a Texture Unit");
        if ( texUnitsPrefixArray == 0 )
            CG_ASSERT(
                  "The Texture Unit signal name prefix array can not be NULL");
        for( U32 i = 0; i < numTexUnits; i++ )
        {
            if (texUnitsPrefixArray[i] == NULL)
                CG_ASSERT("Texture Unit signal name prefix is NULL");
        }
    ) // end of assertion


    GPU_ASSERT
    (
        if ( systemMemorySize % 4 != 0 )
            CG_ASSERT("Memory must be a multiple of 4 bytes");
    )

    /// Create Memory Controller Statistics ///
    createStatistics();

    ////////////////////////////////////////////////
    // start creation of system memory structures //
    ////////////////////////////////////////////////

    systemMemory = new U08[systemMemorySize];

    // initialize system memory
    for ( U32 i = 0; i < systemMemorySize; i += 4 )
        *(reinterpret_cast<U32*>(&systemMemory[i])) = 0XDEADCAFE;

    // Create system memory buses
    for ( U32 i = 0; i < systemMemoryBuses; i++ )
    {
        stringstream ss;
        string str;
        ss << "SystemMemory" << setfill('0') << setw(2) << i;
        string signalName = ss.str();
        systemMemoryRequestSignal.push_back(
            newOutputSignal(signalName.c_str(), 1, systemMemoryReadLatency, 0));
        systemMemoryDataSignal.push_back(
            newInputSignal(signalName.c_str(), 1, systemMemoryWriteLatency, 0));
    }
    systemFreeRequestQueue.resize(requestQueueSize);
    systemFreeRequestQueue.setName("systemFreeRequestQueue");

    systemRequestQueue.resize(requestQueueSize);
    systemRequestQueue.setName("systemRequestQueue");

    systemRequestBuffer = new MemoryRequest[requestQueueSize];
    for ( U32 i = 0; i < requestQueueSize; i++ )
        systemFreeRequestQueue.add(i);

    systemTrans = new MemoryTransaction*[systemMemoryBuses];
    for ( U32 i = 0; i < SYSTEM_MEMORY_BUSES; i++ )
        systemBus[i] = 0;

    ///////////////////////////////////////////////////////////////////
    // Create DDR Modules, Channel Schedulers and signal connections //
    ///////////////////////////////////////////////////////////////////

    GPU_ASSERT(
        if ( params.memSpecs == 0 ) {
            CG_ASSERT("Memory Specs struct is NULL!");
        }
    )

    channelScheds = new ChannelScheduler*[gpuMemoryChannels];
    ddrModules = new DDRModule*[gpuMemoryChannels];

    // Create command signal from the command processor
    mcCommSignal = newInputSignal("MemoryControllerCommand", 1, 1, 0);

    // Create array of signals to interconnect the schedulers with the memory controller
    channelRequest = new Signal*[gpuMemoryChannels];
    channelReply = new Signal*[gpuMemoryChannels];
    schedulerState = new Signal*[gpuMemoryChannels];

    ChannelScheduler::PagePolicy pagePolicy;
    if ( params.schedulerPagePolicy == MemoryControllerParameters::ClosePage )
        pagePolicy = FifoScheduler::ClosePage;
    else
        pagePolicy = FifoScheduler::OpenPage;

    // Create internal boxes (Schedulers & ddr chips) and connected
    // them with memory controller
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {

        // Create prefix
        stringstream ss;
        ss << "Sched" << i;
        string prefix = ss.str();
        ss.str("");
        if ( params.schedulerType == MemoryControllerParameters::Fifo )
            ss << "FifoSched" << i;
        else if ( params.schedulerType == MemoryControllerParameters::RWFifo )
            ss << "RWFifoSched" << i;
        else if ( params.schedulerType == MemoryControllerParameters::BankQueueFifo )
            ss << "BankFifoSched" << i;
        else if ( params.schedulerType == MemoryControllerParameters::BankRWQueueFifo )
            ss << "BankRWFifoSched" << i;
        else if ( params.schedulerType == MemoryControllerParameters::LookAhead )
            ss << "LookAheadSched" << i;
        else
            ss << "Unknown scheduler" << i;

        // Create schedulers and ddr chips and connect them
        channelScheds[i] = createChannelScheduler(ss.str().c_str(),
                                                   prefix.c_str(),
                                                  params,
                                                  this);
        ss.str("");
        ss << "DDRModule" << i;

        ddrModules[i] = new DDRModule(ss.str().c_str(), prefix.c_str(),
                                      params.burstLength,
                                      params.banksPerMemoryChannel,
                                      bankRows,
                                      bankCols,
                                      // params.burstElementsPerCycle,
                                      params.burstBytesPerCycle,
                                      *params.memSpecs,
                                      // params.perfectMemory,
                                      this);

        // Create connections from memory controller to channel schedulers
        channelRequest[i]= newOutputSignal("ChannelRequest", 1, 1, prefix.c_str());
        channelReply[i] = newInputSignal("ChannelReply", 1, 1, prefix.c_str());
        schedulerState[i] = newInputSignal("SchedulerState", 1, 1, prefix.c_str());
    }

    //////////////////////////////////////////////////////////////////////////
    // Create bandwidth usage signals (to visualize bandwidth usage in STV) //
    //////////////////////////////////////////////////////////////////////////
    if ( createInnerSignals )
        createUsageSignals(params);

    //////////////////////////////////////////////////////////////////////////////////////
    // Create signals from/to memory controller from/to GPU units (I/O data/ctrl buses) //
    //////////////////////////////////////////////////////////////////////////////////////
    createIOSignals();

    // Create Memory Request entries
    freeRequestQueue.resize(requestQueueSize);
    freeRequestQueue.setName("freeRequestQueue");
    requestBuffer = new MemoryRequest[requestQueueSize];
    for ( U32 i = 0; i < requestQueueSize; i++ )
        freeRequestQueue.add(i);

    serviceQueue.resize(params.serviceQueueSize);
    serviceQueue.setName("serviceQueue");
    // freeServices = params.serviceQueueSize;

    // Used to maintain the arrival time of the in-flight sytem transactions (read)
    systemTransactionArrivalTime.resize(params.requestQueueSize);
    systemTransactionArrivalTime.setName("systemTransactionArrivalTime");
    systemTransactionArrivalTimeCheckID.resize(params.requestQueueSize);
    systemTransactionArrivalTimeCheckID.setName("systemTransactionArrivalTimeCheckID");

    // Allocate the per GPU memory channel queues
    channelQueue = new Queue<std::pair<ChannelTransaction*,U64> >*[gpuMemoryChannels];

    U32 bankQueuesCount = useIndependentQueuesPerBank ? params.banksPerMemoryChannel : 1;
    if ( bankQueuesCount > 1 ) {
        nextBankRR = new U32[bankQueuesCount];
        memset(nextBankRR, 0, sizeof(U32) * bankQueuesCount);
    }
    else
        nextBankRR = 0; // For security/debugging reasons we set the pointer to 0 if it won't be used

    for ( U32 channel = 0; channel < gpuMemoryChannels; ++channel )
        channelQueue[channel] = new Queue<std::pair<ChannelTransaction*,U64> >[bankQueuesCount];

    stringstream auxStream;
    for ( U32 i = 0; i < gpuMemoryChannels; ++i )
    {
        for ( U32 j = 0; j < bankQueuesCount; ++j ) {
            auxStream.str("");
            auxStream << "channelQueue[" << i << "]";
            channelQueue[i][j].resize(requestQueueSize);
            channelQueue[i][j].setName(auxStream.str());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // set size of (busCycles,reserveBus,service,currentTrans,isSystemTrans) vectors ///
    ////////////////////////////////////////////////////////////////////////////////////
    for ( U32 i = 0; i < LASTGPUBUS; i++ )
    {
        U32 count = 0;
        switch ( i )
        {
            case COMMANDPROCESSOR:
            case STREAMERFETCH:
            case DISPLAYCONTROLLER:
                count = 1;
                break;
            case STREAMERLOADER:
                count = numStrLoaderUnits;
                break;
            case ZSTENCILTEST:
            case COLORWRITE:
                count = numStampUnits;
                break;
            case TEXTUREUNIT:
                count = numTexUnits;
                break;
            case SYSTEM:
                count = systemMemoryBuses;
                break;
            case MEMORYMODULE:
                continue; // ignore, not considered in Version 2
        }
        /*
        busCycles[i].resize(count, 0);
        reserveBus[i].resize(count, false);
        service[i].resize(count,false);
        currentTrans[i].resize(count,0);
        isSystemTrans[i].resize(count,false);
        */
        _busState[i].resize(count);
        for ( U32 j = 0; j < _busState[i].size(); ++j ) {
            BusState& busState = _busState[i][j];
            busState.busCycles = 0;
            busState.mt = 0;
            busState.rbEntry = 0;
            busState.reserveBus = false;
            busState.service = false;
            busState.isSystemTrans = false;
        }

    }

    ropCounters.assign(params.numStampUnits, 0);


    // Reset the memory controller state
    reset();
}

void MemoryController::createStatistics()
{
    totalTransStat = &getSM().getNumericStatistic("TotalTransactions", U32(0),
                                                    "MemoryController", "MC");
    totalReadTransStat = &getSM().getNumericStatistic("TotalReadTransactions", U32(0),
                                                    "MemoryController", "MC");
    totalWriteTransStat = &getSM().getNumericStatistic("TotalWriteTransactions", U32(0),
                                                    "MemoryController", "MC");
    totalReadBytesStat = &getSM().getNumericStatistic("TotalReadBytes", U32(0),
                                                    "MemoryController", "MC");
    totalWriteBytesStat = &getSM().getNumericStatistic("TotalWriteBytes", U32(0),
                                                    "MemoryController", "MC");

    channelReq_ReadStallCyclesStat = &getSM().getNumericStatistic("ChannelReq_ReadStallCycles", U32(0),
                                                    "MemoryController", "MC");

    channelReq_WriteStallCyclesStat = &getSM().getNumericStatistic("ChannelReq_WriteStallCycles", U32(0),
                                                    "MemoryController", "MC");

    for ( U32 i = 0; i < LASTGPUBUS; i++ )
    {
        U32 count = 0;
        stringstream ss;
        switch ( i )
        {
            case COMMANDPROCESSOR:
            case STREAMERFETCH:
            case DISPLAYCONTROLLER:
                count = 1;
                break;
            case STREAMERLOADER:
                count = numStrLoaderUnits;
                break;
            case ZSTENCILTEST:
            case COLORWRITE:
                count = numStampUnits;
                break;
            case TEXTUREUNIT:
                count = numTexUnits;
                break;
            default:
               break;
             // ignore (MEMORYMODULE & SYSTEM)
        }

        // Create per unit-subunit bus statistics
        string busName(MemoryTransaction::getBusName(GPUUnit(i)));


        for ( U32 j = 0; j < count; j++ )
        {
            // push vector for unit instances with unit identifier 'i'
            unitChannelStats[i].push_back(vector<UnitChannelStatsGroup>());

            UnitStatsGroup usg;
            stringstream ss;
            ss << busName;
            if ( count > 1 ) ss << j;
            string busName2 = ss.str() + "_";
            // Create all statistics for unit type 'i'
            usg.initUnitStatGroup(busName2);
            unitStats[i].push_back(usg);

            // create per unit/channel stats
            for ( U32 k = 0; k < gpuMemoryChannels; ++k ) {
                UnitChannelStatsGroup usg;
                stringstream auxStr;
                auxStr << k;
                usg.initUnitChannelStatsGroup(busName2, "_Sched" + auxStr.str());
                unitChannelStats[i].back().push_back(usg);
            }
        }

        if ( count > 1 )
        {
            // Add an extra statistic to compute accumutated statistic
            // (sum of all subunits. for example all ColorWrite unit)
            UnitStatsGroup usg;
            usg.initUnitStatGroup(busName + "_");
            unitStats[i].push_back(usg);
        }
    }

    // create preload transaction counter
    preloadStat = &getSM().getNumericStatistic("PreloadTransactions", U32(0), "MemoryController", "MC");

    // Create system memory statistics
    sysBusReadBytesStat = &getSM().getNumericStatistic("ReadBytesSystemBus", U32(0),
                                                       "MemoryController", "MC");
    sysBusWriteBytesStat = &getSM().getNumericStatistic("WriteBytesSystemBus", U32(0),
                                                        "MemoryController", "MC");


    stat_requestBufferAccumSizeStat = &getSM().getNumericStatistic("Request_Buffer_ACCUM_Allocated_Entries", U32(0),
                                                                   "MemoryController", "MC");

    stat_avgRequestBufferAllocated = &getSM().getNumericStatistic("Request_Buffer_AVG_Allocated_Entries", U32(0),
                                                                   "MemoryController", "MC");

    stat_serviceQueueAccumSizeStat = &getSM().getNumericStatistic("Service_Queue_ACCUM_Items", U32(0),
                                                                   "MemoryController", "MC");

    stat_avgServiceQueueItems = &getSM().getNumericStatistic("Service_Queue_AVG_Items", U32(0),
                                                                   "MemoryController", "MC");

}


void MemoryController::UnitStatsGroup::initUnitStatGroup( const std::string& prefix )
{
    // prefix           STAT      postfix
    // ZStencilTest0_Transactions_Sched0
    trans = &getSM().getNumericStatistic((prefix + "Transactions").c_str(), U32(0), "MemoryController", "MC");
    readTrans = &getSM().getNumericStatistic((prefix + "ReadTransactions").c_str(), U32(0), "MemoryController", "MC");
    writeTrans = &getSM().getNumericStatistic((prefix + "WriteTransactions").c_str(), U32(0), "MemoryController", "MC");
    readBytes = &getSM().getNumericStatistic((prefix + "ReadBytes").c_str(), U32(0), "MemoryController", "MC");
    writeBytes = &getSM().getNumericStatistic((prefix + "WriteBytes").c_str(), U32(0), "MemoryController", "MC");
    acceptReadCycles = &getSM().getNumericStatistic((prefix + "AcceptReadCycles").c_str(), U32(0), "MemoryController", "MC");
    acceptWriteCycles = &getSM().getNumericStatistic((prefix + "AcceptWriteCycles").c_str(), U32(0), "MemoryController", "MC");
    readServiceAccumTime = &getSM().getNumericStatistic((prefix + "serviceAccumTime").c_str(), U32(0), "MemoryController", "MC");
    completedReadTrans = &getSM().getNumericStatistic((prefix + "completedReadTrans").c_str(), U32(0), "MemoryController", "MC");
    readServiceTimeAvg = &getSM().getNumericStatistic((prefix + "serviceTimeAvg").c_str(), U32(0), "MemoryController", "MC");
}

void MemoryController::UnitChannelStatsGroup::initUnitChannelStatsGroup( const string& prefix, const string& postfix )
{
    readBytes  = &getSM().getNumericStatistic((prefix + "ReadBytes" + postfix).c_str(), U32(0), "MemoryController", "MC");
    writeBytes = &getSM().getNumericStatistic((prefix + "WriteBytes" + postfix).c_str(), U32(0), "MemoryController", "MC");
    readChannelTransAccumTime = &getSM().getNumericStatistic((prefix + "ReadChannelTransAccumTime" + postfix).c_str(), U32(0), "MemoryController", "MC");
    completedReadChannelTrans = &getSM().getNumericStatistic((prefix + "CompletedReadChannelTrans" + postfix).c_str(), U32(0), "MemoryController", "MC");
}

void MemoryController::createUsageSignals(const MemoryControllerParameters& params)
{
    stringstream ss;
    string busName;
    for ( U32 i = 0; i < LASTGPUBUS; i++ )
    {
        ss.str("");
        switch ( i )
        {
            case COMMANDPROCESSOR:
            case STREAMERFETCH:
            case DISPLAYCONTROLLER:
                ss << MemoryTransaction::getBusName((GPUUnit)i) << "Bus";
                busName = ss.str();
                memBusIn[i].push_back(newInputSignal(busName.c_str(), 1, 1, 0));
                memBusOut[i].push_back(newOutputSignal(busName.c_str(), 1, 1, 0));
                busElement[i].push_back(vector<DynamicObject*>(2));
                busElement[i].front()[0] = new DynamicObject();
                busElement[i].front()[1] = new DynamicObject();
                elemSelect[i].resize(1, 0);
                break;
            case STREAMERLOADER:
            case TEXTUREUNIT:
            case ZSTENCILTEST:
            case COLORWRITE:
            case SYSTEM:
                {
                    U32 count;
                    if ( i == TEXTUREUNIT )
                        count = numTexUnits;
                    else if ( i == STREAMERLOADER )
                       count = numStrLoaderUnits;
                    else if ( i == SYSTEM )
                        count = systemMemoryBuses;
                    else // i == ZSTENCILTEST || i == COLORWRITE
                        count = numStampUnits;

                    busElement[i].resize(count, vector<DynamicObject*>());
                    elemSelect[i].resize(count, 0);

                    for ( U32 j = 0; j < count; j++ )
                    {
                        ss.str("");
                        ss << MemoryTransaction::getBusName((GPUUnit)i) << "Bus" << setfill('0') << setw(2) << j;
                        busName = ss.str();
                        memBusIn[i].push_back(newInputSignal(busName.c_str(), 1, 1, 0));
                        memBusOut[i].push_back(newOutputSignal(busName.c_str(), 1, 1, 0));
                        busElement[i][j].push_back(new DynamicObject());
                        busElement[i][j].push_back(new DynamicObject());
                    }
                }
                break;
            case MEMORYMODULE:
                break; // Ignored in MC version 2
            default:
                CG_ASSERT("Unknown bus name");
                break;
        }
    }
}

void MemoryController::reset()
{
    // empty for now
}

void MemoryController::createIOSignals()
{
    DynamicObject* defValue[2];

    // Create signals between CommandProcessor and MemoryController
    dataSignals.push_back(vector<Signal*>()); // Group identifier: COMMANDPROCESSOR
    dataSignals.back().push_back(newOutputSignal("CommProcMemoryRead", 2, 1, 0));
    requestSignals.push_back(newInputSignal("CommProcMemoryWrite", 1, 1, 0));

    // Create signals beetween StreamerFetch and MemoryController
    dataSignals.push_back(vector<Signal*>());  // Group identifier: STREAMERFETCH
    dataSignals.back().push_back(newOutputSignal("StreamerFetchMemoryData", 2, 1, 0));
    requestSignals.push_back(newInputSignal("StreamerFetchMemoryRequest", 1, 1, 0));

    //  Create signals beetween StreamerLoader units and MemoryController
    dataSignals.push_back(vector<Signal*>()); // Group identifier: STREAMERLOADER
    for (U32 i = 0; i < numStrLoaderUnits; i++)
    {
        dataSignals.back().push_back(newOutputSignal("StreamerLoaderMemoryData", 2, 1, streamerLoaderPrefixArray[i]));
        requestSignals.push_back(newInputSignal("StreamerLoaderMemoryRequest", 1, 1, streamerLoaderPrefixArray[i]));
    }

    dataSignals.push_back(vector<Signal*>()); // Group identifier: ZSTENCILTEST
    for (U32 i = 0; i < numStampUnits; i++ )
    {
        dataSignals.back().push_back(
            newOutputSignal("ZStencilTestMemoryData", 2, 1, stampUnitsPrefixArray[i]));
        requestSignals.push_back(
            newInputSignal("ZStencilTestMemoryRequest", 1, 1, stampUnitsPrefixArray[i]));
    }

    dataSignals.push_back(vector<Signal*>()); // Group identifier: COLORWRITE
    for (U32 i = 0; i < numStampUnits; i++ )
    {
        dataSignals.back().push_back(
            newOutputSignal("ColorWriteMemoryData", 2, 1, stampUnitsPrefixArray[i]));
        requestSignals.push_back(
            newInputSignal("ColorWriteMemoryRequest", 1, 1, stampUnitsPrefixArray[i]));
    }

    // Create signals between DisplayController unit and MemoryController
    dataSignals.push_back(vector<Signal*>()); // Group identifier: DISPLAYCONTROLLER
    dataSignals.back().push_back(newOutputSignal("DisplayControllerMemoryData", 2, 1, 0));
    requestSignals.push_back(newInputSignal("DisplayControllerMemoryRequest", 1, 1, 0));

    // Create signals between TextureUnits and MemoryController
    dataSignals.push_back(vector<Signal*>()); // Group identifier: TEXTUREUNIT
    for ( U32 i = 0; i < numTexUnits;i++ )
    {
        dataSignals.back().push_back(
            newOutputSignal("TextureMemoryData", 2, 1, texUnitsPrefixArray[i]));
        requestSignals.push_back(
            newInputSignal("TextureMemoryRequest", 1, 1, texUnitsPrefixArray[i]));
    }

    // Init with default values all Data Signals
    vector<vector<Signal*> >::iterator it = dataSignals.begin();
    for ( ; it != dataSignals.end(); it++ )
    {
        vector<Signal*>::iterator it2 = it->begin();
        for ( ; it2 != it->end(); it2++ )
        {
            defValue[0] = MemoryTransaction::createStateTransaction(MS_BOTH);
            defValue[1] = MemoryTransaction::createStateTransaction(MS_BOTH);
            (*it2)->setData(defValue); // initial value
        }
    }

}

void MemoryController::readUsageBuses(U64 cycle)
{
    DynamicObject* busElemAux;
    for ( U32 i = 0; i < LASTGPUBUS; i++ )
    {
        vector<Signal*>::const_iterator it = memBusOut[i].begin();
        for ( ; it != memBusOut[i].end(); it++ )
            (*it)->read(cycle, busElemAux);
    }
}


void MemoryController::stage_readRequests(U64 cycle)
{
    MemoryTransaction* memTrans;
    vector<Signal*>::iterator it = requestSignals.begin();
    for ( ; it != requestSignals.end(); it++ )
    {
        if ( (*it)->read(cycle, (DynamicObject* &) memTrans) ) // transaction received
        {
            GPU_DEBUG
            (
                cout << "MemoryControllerV2 => Request from the ";
                cout << memTrans->getRequestSourceStr();
                cout << " Unit: " << memTrans->getUnitID << "\n";
            )
            processMemoryTransaction(cycle, memTrans);
        }
    }
}

MemoryRequestSplitter& MemoryController::selectSplitter(U32 address, U32 size) const
{
    if ( splitterArray.size() == 1 || MCV2_2ND_INTERLEAVING_START_ADDR == 0 )
        return *splitterArray.front();

    // if ( mt->getAddress() < MCV2_2ND_INTERLEAVING_START_ADDR )
    if ( address < MCV2_2ND_INTERLEAVING_START_ADDR )
    {
        GPU_ASSERT(
            // if ( mt->getAddress() + mt->getSize() >= MCV2_2ND_INTERLEAVING_START_ADDR )
            if ( address + size >= MCV2_2ND_INTERLEAVING_START_ADDR )
                CG_ASSERT(
                "Transaction crosses from first interleaving to second interleaving");
        )
        return *splitterArray.front();
    }
    else
        return *splitterArray.back();
}

void MemoryController::preloadGPUMemory(MemoryTransaction* mt)
{
    // MemoryRequestSplitter& splitter = selectSplitter(mt);
    MemoryRequestSplitter& splitter = selectSplitter(mt->getAddress(), mt->getSize());

    MemoryRequest mr(mt);
    MCSplitter::CTV preloadTrans = splitter.split(&mr);
    for ( MCSplitter::CTVCIt it = preloadTrans.begin(); it != preloadTrans.end(); it++ )
    {
        ChannelTransaction& ct = *(*it);
        U32 channel = ct.getChannel();
        if ( channel >= gpuMemoryChannels )
            CG_ASSERT("Channel ID too high");

        ddrModules[channel]->preload(ct.getBank(), ct.getRow(), ct.getCol(),
                                     ct.getData(), ct.bytes(),
                                    (ct.isMasked() ? ct.getMask() : 0));

        delete *it; // consume the channel transaction (delete it)
    }
}

void MemoryController::updateCompletedReadStats(MemoryRequest& mr, U64 cycle)
{

    const MemoryTransaction* mt = mr.getTransaction();
    GPUUnit unit = mt->getRequestSource();
    U32 unitID = mt->getUnitID();
    GPU_ASSERT(
        if ( mr.getArrivalTime() >= cycle )
            CG_ASSERT(
                  "Arrival time greater than current time!");
    )

    UnitStatsGroup& usg = unitStats[unit][unitID];

    usg.readServiceAccumTime->inc(cycle - mr.getArrivalTime());
    usg.completedReadTrans->inc();
    usg.readServiceTimeAvg->incavg(cycle - mr.getArrivalTime());

    // update globals per unit type if the unit is a replicated unit (ex: color write)
    if ( unitStats[unit].size() > 1 )
    {
        UnitStatsGroup usgGlobal = unitStats[unit].back();
        usgGlobal.readServiceAccumTime->inc(cycle - mr.getArrivalTime());
        usgGlobal.completedReadTrans->inc();
        usgGlobal.readServiceTimeAvg->incavg(cycle - mr.getArrivalTime());
    }

}


void MemoryController::processMemoryTransaction(U64 cycle, MemoryTransaction* memTrans)
{
    GPUUnit unit = memTrans->getRequestSource();
    U32 unitID = memTrans->getUnitID();
    U32 address = memTrans->getAddress();
    U32 size = memTrans->getSize();
    MemTransCom command = memTrans->getCommand();
    const U08* data;

    if ( memoryTrace ) {
        memoryTrace->record(cycle, unit, unitID, command, address, size);
    }

    bool isSystemMemory;
    if ( ( address & ADDRESS_SPACE_MASK) == GPU_ADDRESS_SPACE )
        isSystemMemory = false;
    else if ( ( address & ADDRESS_SPACE_MASK ) == SYSTEM_ADDRESS_SPACE )
        isSystemMemory = true;
    else
    {
        isSystemMemory = false;
        CG_ASSERT("Unsupported address space");
    }

    // cout << "MemoryControllerV2 -> Processing transaction from space memory: " << (isSystemMemory ? "SYSTEM" : "GPU") << endl;

    // update transaction counting statistics
    totalTransStat->inc();
    unitStats[unit][unitID].trans->inc();
    if ( unitStats[unit].size() > 1 ) // increase accumulated stat for a replicated unit
        unitStats[unit].back().trans->inc();

    switch ( command )
    {
        case MT_PRELOAD_DATA:
            // update preload transaction counting statistic
            preloadStat->inc();

            data = memTrans->getData();
            GPU_ASSERT
            (
                if ( data == 0 )
                    CG_ASSERT("NULL data buffer");
            )
            GPU_DEBUG
            (
                cout << "MC => Processed MT_PRELOAD_DATA (" << hex << address
                     << "," << dec << size << ")";
                cout << " [to " << (isSystemMemory?"System":"GPU") << " Memory]." << endl;
            )
            if ( isSystemMemory )
            {
                 // check access correctness
                    CG_ASSERT_COND(!(((address & SPACE_ADDRESS_MASK) >= systemMemorySize) ||
                                    (((address & SPACE_ADDRESS_MASK) + size) > systemMemorySize)), 
                                  "System Memory operation out of range.");
                // preload system memory
                memcpy(&systemMemory[address & SPACE_ADDRESS_MASK], data, size);
            }
            else // gpu memory
            {
                GPU_ASSERT
                (
                    if (((address & SPACE_ADDRESS_MASK) >= gpuMemorySize) ||
                        (((address & SPACE_ADDRESS_MASK) + size) > gpuMemorySize))
                        CG_ASSERT("GPU memory operation out of range.");
                )
                // preload DDR modules
                preloadGPUMemory(memTrans);
            }

            delete memTrans; // The transaction is not any more needed
            return ;
        case MT_READ_REQ:
            // show MT_READ_REQ debug info
            GPU_DEBUG
            (
                cout << "MC => Received READ transaction address: " << std::hex
                << address << std::dec << " size: " << size
                << " unit: " << MemoryTransaction::getBusName(memTrans->getRequestSource())
                << " idUnit: " << memTrans->getUnitID();
                cout << " [to " << (isSystemMemory?"System":"GPU") << " Memory]." << endl;
            )
            totalReadTransStat->inc();
            unitStats[unit][unitID].readTrans->inc();
            if ( unitStats[unit].size() > 1 )
                unitStats[unit].back().readTrans->inc();

            break;
        case MT_WRITE_DATA:
            // show MT_WRITE_DATA debug info
            GPU_DEBUG
            (
                cout << "MC => Received WRITE transaction address: " << std::hex
                     << address << std::dec << " size: " << size
                     << " unit: " <<
                MemoryTransaction::getBusName(memTrans->getRequestSource())
                     << " idUnit: " << memTrans->getUnitID();
                cout << " [to " << (isSystemMemory?"System":"GPU") << " Memory]." << endl;
            )
            totalWriteTransStat->inc();
            unitStats[unit][unitID].writeTrans->inc();
            if ( unitStats[unit].size() > 1 )
                unitStats[unit].back().writeTrans->inc();
            break;
        default:
            CG_ASSERT("Unexpected transaction received");
    }

    U32 requestID;

    // Add request to the corresponding address space
    if ( isSystemMemory )
        requestID = addSystemMemoryRequest(memTrans, cycle);
    else // gpu memory
        requestID = addRequest(memTrans, cycle);

    // { Pre: command == MT_READ_REQ || command == MT_WRITE_DATA }

    if ( command == MT_READ_REQ )
    {
         // update read transaction statistics...
    }
    else // command == MT_WRITE_DATA
    {
        // Alias the current bus' state
        BusState& busState = _busState[unit][unitID];

        GPU_ASSERT
        (
            // Check bus availability
            //if (busCycles[unit][unitID] != 0)
            if ( busState.busCycles != 0 )
            {
                stringstream ss;

                string dinfo;
                getDebugInfo(dinfo);
                ss << dinfo << "\n";
                ss << "Cycle: " << _lastCycle << ". Write not allowed. GPU unit bus " << MemoryTransaction::getBusName(unit) << "[" << unitID
                   << "] is busy for " << busState.busCycles << " cycles\n"
                   << "Previous trans: " << busState.mt->toString() << "\n";
                   // << "Busy with: " + requestBuffer[busState.rbEntry].getTransaction()->toString() << "\n";
                //ss << "Current transaction in bus: " << sysTe
                CG_ASSERT( ss.str().c_str());
            }
            // Check if a write buffer is available
                if (freeWriteBuffers == 0)
                    CG_ASSERT("No free write buffer available");
        )


        // bus cycles required to transmit the data payload of the write transaction
        //busCycles[unit][unitID] = memTrans->getBusCycles();
        busState.busCycles = memTrans->getBusCycles();
        busState.mt = memTrans;

        freeWriteBuffers--; // obtain a write buffer

        if ( isSystemMemory )
            systemRequestBuffer[requestID].setState(MRS_TRANSMITING);
        else // gpu memory
            requestBuffer[requestID].setState(MRS_TRANSMITING);

        //isSystemTrans[unit][unitID] = isSystemMemory;
        //currentTrans[unit][unitID] = requestID;
        //service[unit][unitID] = false; // It is a write (service == true means READ)
        busState.isSystemTrans = isSystemMemory;
        busState.rbEntry = requestID;
        busState.service = false;

        if ( useInnerSignals && isSignalTracingRequired(cycle) )
        {
            // Update dynamic object to use for the unit for bus bandwidth signals
            elemSelect[unit][unitID] = elemSelect[unit][unitID] & 0x1; // perform mod 2
            // Copy cookies from the current transaction to the bus usage object.  */
            busElement[unit][unitID][elemSelect[unit][unitID]]->copyParentCookies(*memTrans);
        }
    }
}

U32 MemoryController::addRequest(MemoryTransaction* memTrans, U64 cycle)
{
    GPU_ASSERT
    (
        if ( freeRequestQueue.empty() )
            CG_ASSERT("Request buffer is full");

        if ( memTrans->getCommand() != MT_READ_REQ
          && memTrans->getCommand() != MT_WRITE_DATA )
            CG_ASSERT(
                  "Unexpected Memory Transaction command received");
    )

    GPU_DEBUG( cout << "MemoryController => Added new request.\n"; )

    U32 requestID = freeRequestQueue.pop(); // Get a free memory request slot
    MemoryRequest& request = requestBuffer[requestID]; // Get the request information slot
    request.setArrivalTime(cycle);
    request.setOccupied(true); // mark this entry as in use
    request.setTransaction(memTrans);
    request.setState(MRS_READY);


    // Check if the request is from a ROP
    switch ( memTrans->getRequestSource() ) {
        case cg1gpu::COLORWRITE:
        case cg1gpu::ZSTENCILTEST:
            {
                // update counters
                U32 subid = memTrans->getUnitID();
                GPU_ASSERT( if ( subid >= ropCounters.size() ) CG_ASSERT("ROP counter out of bounds"); )
                ++ropCounters[subid]; // add new request
            }
        default:
            ; // nothing to do
    }

    // Split the request into Channel Transactions
    MCSplitter::CTV ctv = selectSplitter(memTrans->getAddress(), memTrans->getSize()).split(&request);

    // register how many channel transactions are required to complete this request
    // Required to correct assembly of the memory transaction
    request.setCounter(ctv.size());

    vector<bool> check(gpuMemoryChannels,false);
    for (
        MCSplitter::CTVCIt it = ctv.begin(); it != ctv.end(); it++ )
    {
        ChannelTransaction* ctrans = *it;
        U32 channel = ctrans->getChannel();
        U32 bank = (useIndependentQueuesPerBank ? ctrans->getBank() : 0);
        GPU_ASSERT
        (
            // check that two transactions does not split to the same channel
            if ( channel >= gpuMemoryChannels )
            {
                cout << "Channel ID = " << channel << "\n";
                CG_ASSERT(
                "Memory channel idenfier too high");
            }
            if ( check[channel] )
                CG_ASSERT(
                "Memory transaction chunks split to the same channel");
            if ( channelQueue[channel][bank].full() )
                CG_ASSERT(
                "Channel Queue is full (should not happed!)");
            check[channel] = true; // This channel transaction goes to 'channel'
        )


        // Copy parent cookies from memory transaction into each generated ch. transaction
        ctrans->copyParentCookies(*request.getTransaction());
        ctrans->addCookie();
        // set STV info
        sprintf((char*)ctrans->getInfo(), ctrans->toString().c_str());

        // drive the channel transaction to the corresponding channel
        channelQueue[channel][bank].add(make_pair(ctrans, cycle));
    }

    return requestID;
}


U32 MemoryController::addSystemMemoryRequest(MemoryTransaction* mt, U64 cycle)
{
    GPU_ASSERT
    (
        if ( systemFreeRequestQueue.empty() )
            CG_ASSERT(
                  "System memory request full");
    )

    GPU_DEBUG( cout << "MemoryController => Added a new system memory request.\n"; )

    // Get a free system buffer request entry
    U32 requestID = systemFreeRequestQueue.pop();
    MemoryRequest& request = systemRequestBuffer[requestID]; // create alias
    systemRequestQueue.add(requestID); // add into the FIFO system request queue
    request.setArrivalTime(cycle);
    request.setTransaction(mt);
    request.setOccupied(true); // mark this entry as in use
    request.setState(MRS_READY);
    return requestID;
}

void MemoryController::issueSystemTransaction(U64 cycle)
{
    // Same than the previous memory controller implementation by V�ctor.
    /*  NOTE:  Two unidirectional buses, one for writes and one for reads.
        In current implementation bus 0 is used for reads and bus 1 for writes.  */

    if ( systemRequestQueue.empty() )
        return ;

    GPU_DEBUG( cout << "MemoryController => Issue a new system request.\n"; )

    // select the first ready system transaction
    U32 requestID = systemRequestQueue.head();

    MemoryTransaction* memTrans = systemRequestBuffer[requestID].getTransaction();

    switch ( memTrans->getCommand() )
    {
        case MT_READ_REQ:
            // Read request command
            if ( cycle - systemBus[0] >= SYSTEM_TRANSACTION_CYCLES &&
                 freeReadBuffers > 0 /* && freeServices > 0 */)
            {
                U32 address = memTrans->getAddress();
                U32 size = memTrans->getSize();
                U08* data = memTrans->getData();

                GPU_DEBUG
                (
                    cout << "MemoryController => Issuing MT_READ_REQ (" << std::hex
                    << address << std::dec << ", " << size
                    << ") to system memory bus " << 0 << ".\n";
                )

                // check memory operation access
                GPU_ASSERT
                (
                    if (((address & SPACE_ADDRESS_MASK) >= systemMemorySize ) ||
                         (((address & SPACE_ADDRESS_MASK) + size) >= systemMemorySize))
                    {

                        std::printf("MC %lld => checking error\n", cycle);

                        cout << "MT_READ_REQ address: " << hex << address << dec
                            << " size: " << size << " unit: "
                            << MemoryTransaction::getBusName(memTrans->getRequestSource())
                            << " unit ID: " << memTrans->getUnitID() << ".\n";
                        CG_ASSERT(
                            "System memory operation out of range.");
                    }
                )

                // create the corresponding READ_DATA transaction produced by READ_REQ
                MemoryTransaction* readData =
                    MemoryTransaction::createReadData(memTrans);

                memcpy(data, &systemMemory[address & SPACE_ADDRESS_MASK], size);

                // store arrival time of the current read in the bus
                // Consumed in updateSystemBuses()
                systemTransactionArrivalTime.add(
                    systemRequestBuffer[requestID].getArrivalTime()); // Copy arrival time
                systemTransactionArrivalTimeCheckID.add(
                    systemRequestBuffer[requestID].getTransaction()->getID());

                // Simulate system memory latency writing through a signal
                systemMemoryRequestSignal[0]->write(cycle, readData, systemMemoryReadLatency);
                systemBus[0] = cycle; // set bus as occupied
                // systemBusID[0] = requestID; // store the ID of the ongoing read request
                systemBusID[0].push(requestID);

                // Reserve a read buffer
                freeReadBuffers--;

                // Reserve a service queue entry
                // freeServices--;

                //Remove it from the system ready queue
                systemRequestQueue.pop();
            }
            break;
        case MT_WRITE_DATA:
            if (((cycle - systemBus[1]) >= SYSTEM_TRANSACTION_CYCLES)
                  && (systemRequestBuffer[requestID].getState() == MRS_READY))
            {
                U32 address = memTrans->getAddress();
                U32 size = memTrans->getSize();
                U08* data = memTrans->getData();
                GPU_ASSERT(
                    if (((address & SPACE_ADDRESS_MASK) >= systemMemorySize) ||
                        (((address & SPACE_ADDRESS_MASK) + size) > systemMemorySize))
                    {
                        printf("MC %lld => checking error\n", cycle);
                        printf("MT_WRITE_DATA address %x size %d unit %s unit ID %d\n",
                            address, size,
                            MemoryTransaction::getBusName(memTrans->getRequestSource()),
                            memTrans->getUnitID());

                        CG_ASSERT(
                            "Mapped system memory operation out of range.");
                    }
                )
                //  Check data buffer pointer not null
                GPU_ASSERT(
                    if (data == NULL)
                        CG_ASSERT(
                        "Undefined data buffer.");
                )

                GPU_DEBUG(
                    printf("MemoryController => Issuing MT_WRITE (%x, %d) to system memory bus %d.\n", address, size, 0);
                )
                //  Check if it is a masked write
                if (memTrans->isMasked())
                {
                    GPU_DEBUG(
                        printf("MemoryController => Masked write.\n");
                    )

                    U32* mask = memTrans->getMask(); // Get write mask

                    U32 source, dest;
                    // Write only unmasked bytes
                    for(U32 i = 0; i < size; i = i + 4)
                    {
                        // Get source data
                        source = *((U32 *) &systemMemory[(address & SPACE_ADDRESS_MASK) + i]);
                        source = source & ~mask[i >> 2]; // Delete unmasked bytes
                        dest = *((U32 *) &data[i]); // Get destination data
                        dest = dest & mask[i >> 2]; // Delete masked bytes
                        // Write to memory
                        *((U32 *) &systemMemory[(address & SPACE_ADDRESS_MASK) + i]) = source | dest;
                    }
                }
                else
                {
                    // Copy data to memory
                    memcpy(&systemMemory[address & SPACE_ADDRESS_MASK], data, size);
                }
                // Simulate system memory access latency
                systemMemoryRequestSignal[1]->write(cycle, memTrans, systemMemoryWriteLatency);
                // cout << "cycle: " << cycle << ". MCV2 -> Sending system requests thru Signal: " << memTrans->toString() << endl;

                systemBus[1] = cycle; // Set system bus as not available
                // Remove it from the system ready queue

                systemRequestQueue.pop();
                systemFreeRequestQueue.add(requestID); // Add the entry to the free list
                systemRequestBuffer[requestID].setOccupied(false); // Mark as not occupied
            }
            break;
        default:
            CG_ASSERT("Unsupported memory transaction");
    } // end switch
}

void MemoryController::updateSystemBuses(U64 cycle)
{
    GPUUnit currentUnit;

    for ( U32 bus = 0; bus < SYSTEM_MEMORY_BUSES; bus++ )
    {
        BusState& busState = _busState[SYSTEM][bus];

        // if ( busCycles[SYSTEM][bus] > 0 )
        if ( busState.busCycles > 0 )
        {
            // busCycles[SYSTEM][bus]--;
            --busState.busCycles;

            GPU_DEBUG
            (
                printf("MemoryController => Transmiting system data bus %d"
                    ".  Remaining cycles %d.\n", bus, busCycles[SYSTEM][bus]);
            )
            currentUnit = systemTrans[bus]->getRequestSource();
            U32 unitID = systemTrans[bus]->getUnitID();


            GPU_DEBUG(
                cout << "MC => Transmiting system data bus " << bus
                     << ". Remaining cycles " << busCycles[SYSTEM][bus] << ".\n";
            )

            // if ( busCycles[SYSTEM][bus] == 0 ) // check end of transmission
            if ( busState.busCycles == 0 ) // check end of transmission
            {
                GPU_DEBUG( cout << "MemoryController => End of transmission.\n"; )

                // cout << "Cycle: " << cycle << ". MCV2 -> System bus transmission completed: " << systemTrans[bus]->toString() << endl;
                // terminate current transaction
                switch ( systemTrans[bus]->getCommand() )
                {
                    case MT_READ_DATA:
                        {
                            // Check if there is a free service entry
                            /*
                            GPU_ASSERT(
                                if (freeServices < 0)
                                  CG_ASSERT(
                                  "No free service entry available.");
                            )
                            */

                            MemoryRequest mr;
                            mr.setTransaction(systemTrans[bus]);
                            mr.setState(MRS_MEMORY);
                            // Patch arrival service time for system transactions
                            GPU_ASSERT(
                                if ( systemTransactionArrivalTimeCheckID.head() != mr.getID() )
                                     panic("MemoryController","updateSystemBuses",
                                           "Arrival time check fail");
                            )
                            systemTransactionArrivalTimeCheckID.remove();
                            mr.setArrivalTime(systemTransactionArrivalTime.pop());
                            GPU_ASSERT(
                                if ( serviceQueue.full() ) {
                                    panic("MemoryController","updateSystemBuses",
                                        "Service queue full! System transaction could not be allocated (workaround: increase serviceQueue size)");
                                }
                            )
                            serviceQueue.add(mr);
                            // update read bytes statistic
                            U32 tsize = systemTrans[bus]->getSize();
                            unitStats[currentUnit][unitID].readBytes->inc(tsize);
                            if ( unitStats[currentUnit].size() > 1 )
                                unitStats[currentUnit].back().readBytes->inc(tsize);
                            // update read bytes from system bus statistic
                            sysBusReadBytesStat->inc(systemTrans[bus]->getSize());

                            // This code was previsouly located in issueSystemTransaction() method
                            // Remove the transaction from the request queue (add to the free list)

                            U32 requestID = systemBusID[bus].front();
                            systemBusID[bus].pop();
                            // delete completed READ_REQ transaction
                            delete systemRequestBuffer[requestID].getTransaction();
                            // liberate system request buffer entry
                            systemFreeRequestQueue.add(requestID);
                            systemRequestBuffer[requestID].setOccupied(false);
                        }
                        break;
                    case MT_WRITE_DATA:
                        {
                            freeWriteBuffers++; // liberate write buffer

                            // update write bytes statistic
                            U32 tsize = systemTrans[bus]->getSize();
                            unitStats[currentUnit][unitID].writeBytes->inc(tsize);
                            if ( unitStats[currentUnit].size() > 1 )
                                unitStats[currentUnit].back().writeBytes->inc(tsize);

                            // update written bytes from system bus statistic
                            sysBusWriteBytesStat->inc(tsize);
                            delete systemTrans[bus]; // the transaction can be deleted
                        }
                        break;
                    default:
                        printf(" MC %lld >> transaction %p command %d data %d\n",
                            cycle, systemTrans[bus], systemTrans[bus]->getCommand(),
                               *((U32 *) systemTrans[bus]));
                        CG_ASSERT(
                            "Unsupported memory transaction.");
                      break;
                } // end switch
            } // end if
        } // end if
    } // end for
}


void MemoryController::processSystemMemoryReplies(U64 cycle)
{
    for ( U32 bus = 0; bus < SYSTEM_MEMORY_BUSES; bus++ )
    {
        BusState& busState = _busState[SYSTEM][bus];

        MemoryTransaction* memTrans;
        if ( systemMemoryDataSignal[bus]->read(cycle, (DynamicObject*&)memTrans) )
        {
            // cout << "cycle: " << cycle << ". MCV2 -> MemBus=" << bus << ". Received system requests from Signal: " << memTrans->toString() << endl;
            //GPUUnit currentUnit;
            GPU_ASSERT(
                if (busState.busCycles > 0)
                    CG_ASSERT(
                 "System memory bus busy.");
            )
            GPU_DEBUG(
                printf("MemoryController %lld => Transaction %p from system memory bus %d.\n",
                       cycle, memTrans, bus);
            )

            //  Set system memory bus reserved cycles
            busState.busCycles = SYSTEM_TRANSACTION_CYCLES;
            busState.mt = memTrans;

            //currentUnit = memTrans->getRequestSource();

            // Store the current memory transaction in the memory bus
            systemTrans[bus] = memTrans;
            switch ( memTrans->getCommand() )
            {
                case MT_READ_DATA:
                case MT_WRITE_DATA:
                    // nothing to do: OK
                    break;
                default:
                   CG_ASSERT(
                       "Unsupported memory transaction.");
            } // end switch
        } // end if
    } // end for
}


void MemoryController::reserveGPUBus(U64 cycle )
{
    if ( !serviceQueue.empty() ) // check if there are pending services
    {
        // Get the next transaction to be served
        MemoryTransaction* memTrans = serviceQueue.head().getTransaction();
        U32 unit = memTrans->getRequestSource();
        U32 unitID = memTrans->getUnitID();
        BusState& busState = _busState[unit][unitID];
        if ( busState.busCycles <= 1 ) // check bus availability at the next cycle
            busState.reserveBus = true; // bus will be available, reserve it
    }
}

void MemoryController::stage_sendToSchedulers(U64 cycle)
{
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {
        SchedulerState* schedState;
        if ( !schedulerState[i]->read(cycle, (DynamicObject*&)schedState) )
        {
            stringstream ss;
            ss << "State information (schedulerStat) from scheduler " << i << " was not received!";
            CG_ASSERT(ss.str().c_str());
        }

        if ( !useIndependentQueuesPerBank ) {
            // Check if it exists a Channel Transaction and if it is ready
            if ( !channelQueue[i][0].empty() && channelQueue[i][0].head().first->ready() )
            {
                ChannelTransaction* ct = channelQueue[i][0].head().first;

                SchedulerState::State state = schedState->state(ct->getBank());

                if ( state == SchedulerState::AcceptBoth ||
                   ( state == SchedulerState::AcceptRead  &&  ct->isRead() ) ||
                   ( state == SchedulerState::AcceptWrite && !ct->isRead() ) )
                {
                    GPU_DEBUG
                    (
                        cout << "MemoryController => Sending ChannelTransaction ("
                            << (ct->isRead()?"READ,":"WRITE,") << ct->bytes()
                            << ") reqID = " << ct->getRequest()->getID()
                            << " to ChannelScheduler " << i << "\n";
                    )
                    channelRequest[i]->write(cycle, ct); // send next channel transaction
                    channelQueue[i][0].pop();

                    MemoryRequest* mreq = ct->getRequest();
                    if ( mreq->getState() == MRS_READY ) {
                        mreq->setState(MRS_MEMORY);
                        if ( ct->isRead() )
                            --freeReadBuffers; // reserve buffer for this request
                    }
                    else if ( mreq->getState() != MRS_MEMORY )
                        CG_ASSERT("Unexpected memory request state");
                    // else state == MRS_MEMORY (another channel transaction owned by this mem request is accessing memory)
                }
                else {
                    // Compute statistics of stall
                    if ( ct->isRead() )
                        channelReq_ReadStallCyclesStat->inc();
                    else
                        channelReq_WriteStallCyclesStat->inc();
                }
            }
        }
        else {
            // generate vector of indirections
            vector<U32> ind;

            if ( perBankChannelQueuesSelection == 0 ) {
                // Generate an indirection vector based on Round&Robin
                for ( U32 j = 0; j < banksPerMemoryChannel; ++j ) {
                    U32 bank = (nextBankRR[i] + j) % banksPerMemoryChannel;
                    if ( !channelQueue[i][bank].empty() && channelQueue[i][bank].head().first->ready() )
                        ind.push_back( bank );
                }
            }
            else {
                // Generate an indirection vector based on Oldest first
                vector<pair<U64, U32> > banksTS;
                for ( U32 j = 0; j < banksPerMemoryChannel; ++j ) {
                    if ( !channelQueue[i][j].empty() && channelQueue[i][j].head().first->ready() )
                        banksTS.push_back(make_pair(channelQueue[i][j].head().second, j));
                }
                std::sort(banksTS.begin(), banksTS.end());
                // Copy bank ids
                vector<pair<U64,U32> >::const_iterator it = banksTS.begin();
                for ( ; it != banksTS.end(); ++it )
                    ind.push_back( it->second );
            }

            ChannelTransaction* firstBankConsideredFound = 0;

            vector<U32>::const_iterator it = ind.begin();
            for ( ; it != ind.end(); ++it ) {
            // for ( ; bankPos < banksPerMemoryChannel; ++bankPos ) {

                U32 bank = *it; // Get the next bank to be considered

                // if ( !channelQueue[i][bank].empty() && channelQueue[i][bank].head().first->ready() ) {

                    ChannelTransaction* ct = channelQueue[i][bank].head().first;

                    SchedulerState::State state = schedState->state(bank);

                    if ( !firstBankConsideredFound )
                        firstBankConsideredFound = ct;

                    if ( state == SchedulerState::AcceptBoth ||
                       ( state == SchedulerState::AcceptRead  &&  ct->isRead() ) ||
                       ( state == SchedulerState::AcceptWrite && !ct->isRead() ) ) {
                        GPU_DEBUG
                        (
                            cout << "MemoryController => Sending ChannelTransaction ("
                                << (ct->isRead()?"READ,":"WRITE,") << ct->bytes()
                                << ") reqID = " << ct->getRequest()->getID()
                                << " to ChannelScheduler " << i << "\n";
                        )
                        channelRequest[i]->write(cycle, ct); // send next channel transaction
                        channelQueue[i][bank].pop();
                        nextBankRR[i] = (bank + 1) % banksPerMemoryChannel;

                        MemoryRequest* mreq = ct->getRequest();
                        if ( mreq->getState() == MRS_READY ) {
                            mreq->setState(MRS_MEMORY);
                            if ( ct->isRead() )
                                --freeReadBuffers; // reserve buffer for this request
                        }
                        else if ( mreq->getState() != MRS_MEMORY )
                            CG_ASSERT("Unexpected memory request state");
                        // else state == MRS_MEMORY (another channel transaction owned by this mem request is accessing memory)

                        break;
                    }
                // }
            }

            if ( it == ind.end() && firstBankConsideredFound ) { // no transaction issued
                // Compute statistics of stall
                if ( firstBankConsideredFound->isRead() )
                    channelReq_ReadStallCyclesStat->inc();
                else
                    channelReq_WriteStallCyclesStat->inc();
            }
        }

        delete schedState; // Consume state transaction from channel scheduler i


    } // end for
}

void MemoryController::stage_receiveFromSchedulers(U64 cycle)
{
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {
        ChannelTransaction* ct;
        if ( channelReply[i]->read(cycle, (DynamicObject*&)ct) )
        {
            // Channel Transaction (completed) received from ChannelScheduler i
            MemoryRequest* req = ct->getRequest();
            req->decCounter(); // notify that one chunk is completed

            GPU_ASSERT
            (
                if ( req->getState() != MRS_MEMORY ) {
                    CG_ASSERT(
                          "Received a channel transaction from memory and the request state is different of MRS_MEMORY");
                }
            )

            if ( req->getCounter() == 0 ) // Access to GDDR memory completed
                req->setState(MRS_READY);

            GPU_DEBUG(
                cout << getName() << " => Channel transaction from request "
                << req->getID() << " completed.";
                if ( req->getCounter() > 0 )
                    cout << " " << req->getCounter() << " CT's required to complete request.\n";
                else
                    cout << " All CT's completed -> request completed\n";
            )
            // Update read/write statistics per unit bus
            MemoryTransaction* memTrans = req->getTransaction();
            GPUUnit unit = memTrans->getRequestSource();
            U32 unitID = memTrans->getUnitID();
            if ( ct->isRead() )
            {
                totalReadBytesStat->inc();
                unitStats[unit][unitID].readBytes->inc(ct->bytes());
                if (unitStats[unit].size() > 1 )
                    unitStats[unit].back().readBytes->inc(ct->bytes());

                // Update per unit/channel stats
                UnitChannelStatsGroup& ucsg = unitChannelStats[unit][unitID][i]; // alias
                ucsg.readBytes->inc(ct->bytes());
                ucsg.completedReadChannelTrans->inc();
                // It works assuming that a request is splitted/distributed as soon as arrives
                ucsg.readChannelTransAccumTime->inc(cycle - req->getArrivalTime());
            }
            else // it is a write
            {
                totalWriteBytesStat->inc();

                unitStats[unit][unitID].writeBytes->inc(ct->bytes());

                if ( unitStats[unit].size() > 1 )
                    unitStats[unit].back().writeBytes->inc(ct->bytes());

                // Update per unit/channel stats
                unitChannelStats[unit][unitID][i].writeBytes->inc(ct->bytes());
            }

            // Channel transaction completed, delete it.
            delete ct;
        }
    }
}


void MemoryController::stage_updateCompletedRequests(U64 cycle)
{
    for ( U32 i = 0; i < requestQueueSize; i++ )
    {
        MemoryRequest& request = requestBuffer[i];
        if ( request.isOccupied() && request.getCounter() == 0 )
        {
            MemoryTransaction* requestTransaction = request.getTransaction();

            if ( request.isRead() ) // add the completed request to the service queue (if read)
            {
                if ( serviceQueue.full() )
                     continue; // no space available in the reply queue (try it later)

                MemoryTransaction* readData = MemoryTransaction::createReadData(requestTransaction);

                request.setTransaction(readData); // replace with read_data transaction
                serviceQueue.add(request);
            }
            else // it is a write
                freeWriteBuffers++; // liberate write buffer

            /// update per rop counters ///
            // Check if the request is from a ROP
            switch ( requestTransaction->getRequestSource() ) {
                case cg1gpu::COLORWRITE:
                case cg1gpu::ZSTENCILTEST:
                    {
                        // update counters
                        U32 subid = requestTransaction->getUnitID();
                        GPU_ASSERT( if ( subid >= ropCounters.size() ) CG_ASSERT("ROP counter out of bounds"); )
                        GPU_ASSERT( if ( ropCounters[subid] == 0 ) CG_ASSERT("ropCounter == 0 before releasing a previous request"); )
                        --ropCounters[subid]; // add new request
                    }
                default:
                    ; // nothing to do
            }


            delete requestTransaction; // clear previous request transaction
            freeRequestQueue.add(i); // Add to the free list
            request.setOccupied(false); // release request buffer entry
        }
    }
}


void MemoryController::stage_serveRequest(U64 cycle)
{
    // Check if there are pending services in the queue
    // DEBUG: cycle % 10 used to saturate artificially the serviceQueue
    if ( !serviceQueue.empty() /* && ((cycle % 10) == 0)*/ )
    {
        MemoryTransaction* memTrans = serviceQueue.head().getTransaction(); // get the next service
        U32 unit = memTrans->getRequestSource();
        U32 unitID = memTrans->getUnitID();
        BusState& busState = _busState[unit][unitID]; // create alias
        if ( busState.busCycles == 0 ) // check if the unit bus is free
        {
            if ( busState.reserveBus ) // check if the bus was previously reserved
            {
                dataSignals[unit][unitID]->write(cycle, memTrans, 1);
                GPU_DEBUG
                (
                    printf("MemoryController %lld => Response %d to the GPU Unit %s[%d]."
                          " Bus cycles %d.\n",
                           cycle, memTrans->getID(),
                         MemoryTransaction::getBusName((GPUUnit)unit),
                          unitID, memTrans->getBusCycles());
                )
                busState.busCycles = memTrans->getBusCycles();
                busState.mt = memTrans;

                // Update dynamic object to use for the unit for bus bandwidth signals
                if ( useInnerSignals && isSignalTracingRequired(cycle) )
                {
                    elemSelect[unit][unitID] = (elemSelect[unit][unitID] + 1) & 0x01;

                    // Copy cookies from the memory transaction to the bus usage object
                    busElement[unit][unitID][elemSelect[unit][unitID]]->copyParentCookies(
                                                                               *memTrans);
                }
                busState.service = true;
                busState.reserveBus = false; // unreserve bus for next request

                CG_ASSERT_COND(!( serviceQueue.head().getArrivalTime() >= cycle ), "Service time 0 or less!");
                // Update statistic relative to read completion
                updateCompletedReadStats(serviceQueue.head(), cycle);

                serviceQueue.pop(); // Remove transaction from pending requests
            }
        }
    }
}

void MemoryController::updateBusCounters(U64 cycle)
{
    for ( U32 i = 0; i < LASTGPUBUS; i++ )
    {
        if ( i == MEMORYMODULE || i == SYSTEM )
        {
            // SYSTEM updated separately and MEMORYMODULE
            // is ignored (not considered in this implementation)
            // (MEMORY MODULES are implemented using signals and boxes)
            continue;
        }

        for ( U32 j = 0; j < _busState[i].size(); j++ )
        {
            BusState& busState = _busState[i][j];
            if ( busState.busCycles > 0 )
            {
                GPU_DEBUG
                (
                    cout << "MemoryController => Bus "
                        << MemoryTransaction::getBusName(GPUUnit(i)) <<
                          "[" << j << "] remaining cycles " << busCycles[i][j] << "\n";
                )

                // Send bus usage signal
                if ( useInnerSignals && isSignalTracingRequired(cycle) )
                    memBusIn[i][j]->write(cycle, busElement[i][j][elemSelect[i][j]]);

                --busState.busCycles;
                if ( busState.busCycles == 0 )
                {
                    // check if the transmission was a read (service)
                    if ( busState.service )
                    {
                        GPU_DEBUG( cout << "MC => End of transmission (service)\n"; )
                        busState.service = false;
                        freeReadBuffers++; // liberate read buffer
                        // freeServices++; // liberate service queue entry
                    }
                    else // current transmission was a request
                    {
                        // set transmission as finished
                        if ( busState.isSystemTrans )
                        {
                            systemRequestBuffer[busState.rbEntry].setState(MRS_READY);
                            GPU_DEBUG( cout << "MC => End of transmission (request) - [system memory]\n"; )
                        }
                        else
                        {
                            GPU_DEBUG( cout << "MC => End of transmission (request) - [gpu unit]\n"; )
                            requestBuffer[busState.rbEntry].setState(MRS_READY);
                        }
                    }
                }
            }
        }
    }
}

void MemoryController::sendBusStateToClients(U64 cycle)
{
    // This method to inform is maintained from previous MC implementation by Victor

    /*  NOTE:  The number of free entries that must be available for the
      Memory Controller to accept transactions is the (latency of the request signal + 1)
       for each GPU unit that doesn't support replication and
        (the latency + 1) x (number of replicated units) for the
        GPU units that support replication.  In the current implementation
       only the Texture Unit supports replication.
      This may change and the formula below will have to be changed.  */

    static const U32 requiredEntries = 2 + // CommandProcessor
                                          2 + // StreamerFetch
                                          2 * numStrLoaderUnits + // StreamerLoader
                                          4 * numStampUnits + // ZStencil & ColorWrite
                                          2 + // DisplayController
                                          2 * numTexUnits; // Texture Units
    MemState state;


    // Tests global constraints
    if ( freeRequestQueue.items() >= requiredEntries && systemFreeRequestQueue.items() >= requiredEntries )
        state = MS_READ_ACCEPT;
    else
        state = MS_NONE;

    // Check individual bus constraints
    sendBusState(cycle, COMMANDPROCESSOR, state);
    sendBusState(cycle, STREAMERLOADER, state);
    sendBusState(cycle, STREAMERFETCH, state);
    sendBusState(cycle, ZSTENCILTEST, state);
    sendBusState(cycle, COLORWRITE, state);
    sendBusState(cycle, DISPLAYCONTROLLER, state);
    sendBusState(cycle, TEXTUREUNIT, state);
}

void MemoryController::sendBusState(U64 cycle, GPUUnit unit, MemState state)
{
    // Same implementation that in the previous version of V�ctor
    /*  NOTE:  We consider only the units that can write to memory when
       calculating if there are enough free buffers.  Currently
      Command Processor, Z Stencil Test and Color Write
        are the only units that are allowed to write.
      This may change in future implementations
        and my require a change in the implementation of this function.  */
    bool enoughWriteBuffers = freeWriteBuffers >=
                                2 + // CommandProcessor
                                4 * numStampUnits; // ZStencil & Color write

    vector<Signal*>& signals = dataSignals[unit]; // create alias
    for ( U32 i = 0; i < signals.size(); i++ )
    {
        MemState unitState = state;
        BusState& busState = _busState[unit][i];
        // if ( enoughWriteBuffers && busCycles[unit][i] < 2
        if ( enoughWriteBuffers && busState.busCycles < 2
             && !busState.reserveBus && state == MS_READ_ACCEPT )
             unitState = MS_BOTH; // Writes also accepted

        if ( useRopCounters ) {
            switch ( unit ) {
                case cg1gpu::COLORWRITE:
                case cg1gpu::ZSTENCILTEST:
                    {
                        GPU_ASSERT( if ( i >= ropCounters.size() ) CG_ASSERT("ROP counter out of bounds"); )
                        if ( ropCounters[i] >= (requestQueueSize / ropCounters.size()) )
                            unitState = MS_NONE;
                    }
                default:
                    ; // nothing to do
            }
        }

        MemoryTransaction* memTrans = new MemoryTransaction(unitState);
        signals[i]->write(cycle, memTrans); // send the state transaction

        //  Update statistics.
        if ((unitState & MS_READ_ACCEPT) != 0)
            unitStats[unit][i].acceptReadCycles->inc();

        if ((unitState & MS_WRITE_ACCEPT) != 0)
            unitStats[unit][i].acceptWriteCycles->inc();
    }
}

void MemoryController::processCommand(U64 cycle)
{
    DynamicObject* dynObj;
    if ( mcCommSignal->read(cycle, dynObj) )
    {
        MemoryControllerCommand* mcComm = (MemoryControllerCommand *)dynObj;
        switch ( mcComm->getCommand() ) {
            case MCCOM_REG_WRITE:
                {
                    switch ( mcComm->getRegister() )
                    {
                        case GPU_MCV2_2ND_INTERLEAVING_START_ADDR:
                            MCV2_2ND_INTERLEAVING_START_ADDR = mcComm->getRegisterData().uintVal;
                            CG_INFO("MCV2_2ND_INTERLEAVING_START_ADDR updated with value = %d",  mcComm->getRegisterData().uintVal );
                            break;
                        default:
                            CG_ASSERT("Unknown register in Memory Controller");
                    }
                    delete dynObj;
                }
                break;
            case MCCOM_LOAD_MEMORY:
                loadMemory();
                delete dynObj;
                break;
            case MCCOM_SAVE_MEMORY:
                saveMemory();
                delete dynObj;
                break;
            default:
                CG_ASSERT("Only write register command is supported");
        }
    }
}


void MemoryController::clock(U64 cycle)
{
    GPU_DEBUG( printf("MemoryController => Clock %lld\n", cycle); )

    _lastCycle = cycle; _lastCycleMem = cycle;

    // Process memory controller commands coming from CP
    processCommand(cycle);

    // Read and dump bus usage signals (STV feedback)
    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        readUsageBuses(cycle);

    // Read new memory requests from input signals (buses)
    stage_readRequests(cycle);

    stat_avgRequestBufferAllocated->incavg(requestQueueSize - freeRequestQueue.items());
    stat_requestBufferAccumSizeStat->inc(requestQueueSize - freeRequestQueue.items());

    // Update the state of each unit/memory controller bus
    updateBusCounters(cycle);

    // Serve a pending request from the serviceQueue
    stage_serveRequest(cycle);

    stat_avgServiceQueueItems->incavg(serviceQueue.items());
    stat_serviceQueueAccumSizeStat->inc(serviceQueue.items());

    // reserve the bus for next request
    reserveGPUBus(cycle);

    // Clock children boxes: channel schedulers and their attached ddr modules
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {
        channelScheds[i]->clock(cycle);
        ddrModules[i]->clock(cycle);
    }

    // Release request buffer entries of completed requests and
    // move completed read requests to the serviceQueue
    stage_updateCompletedRequests(cycle);


    // receive reply channel transactions from schedulers
    stage_receiveFromSchedulers(cycle);

    // send channel transactions to the channel schedulers
    stage_sendToSchedulers(cycle);

    // Update system buses counters
    updateSystemBuses(cycle);

    // Issue a new memory transaction
    issueSystemTransaction(cycle);

    // Check for completed requests
    processSystemMemoryReplies(cycle);

    // Send state feedback to clients
    sendBusStateToClients(cycle);
}



/*
void MemoryController::clock(U64 cycle)
{
    GPU_DEBUG( printf("MemoryController => Clock %lld\n", cycle); )

    _lastCycle = cycle; _lastCycleMem = cycle;

    // Process memory controller commands coming from CP
    processCommand(cycle);

    // Read and dump bus usage signals (STV feedback)
    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        readUsageBuses(cycle);

    // Update the state of each unit/memory controller bus
    updateBusCounters(cycle);

    // Read new memory requests from input signals (buses)
    // MANDATORY to execute it before stage_serveRequests()
    stage_readRequests(cycle);

    // Serve a pending request from the serviceQueue
    stage_serveRequest(cycle);

    stat_avgRequestBufferAllocated->incavg(requestQueueSize - freeRequestQueue.items());
    stat_requestBufferAccumSizeStat->inc(requestQueueSize - freeRequestQueue.items());

    stat_avgServiceQueueItems->incavg(serviceQueue.items());
    stat_serviceQueueAccumSizeStat->inc(serviceQueue.items());

    // reserve the bus for next request
    reserveGPUBus(cycle);

    // Clock children boxes: channel schedulers and their attached ddr modules
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {
        channelScheds[i]->clock(cycle);
        ddrModules[i]->clock(cycle);
    }

    // Release request buffer entries of completed requests and
    // move completed read requests to the serviceQueue
    stage_updateCompletedRequests(cycle);

    // receive reply channel transactions from schedulers
    stage_receiveFromSchedulers(cycle);

    // send channel transactions to the channel schedulers
    stage_sendToSchedulers(cycle);

    // Update system buses counters
    updateSystemBuses(cycle);

    // Issue a new memory transaction
    issueSystemTransaction(cycle);

    // Check for completed requests
    processSystemMemoryReplies(cycle);

    // Send state feedback to clients
    sendBusStateToClients(cycle);
}
*/

void MemoryController::clock(U32 domain, U64 cycle)
{
    switch(domain)
    {
        case GPU_CLOCK_DOMAIN:
            updateGPUDomain(cycle);
            break;
        case MEMORY_CLOCK_DOMAIN:
            updateMemoryDomain(cycle);
            break;
        default:
            CG_ASSERT("Clock domain not supported.");
            break;
    }
}

void MemoryController::updateGPUDomain(U64 cycle)
{
    GPU_DEBUG(
        printf("MemoryController => GPU Domain. Clock %lld\n", cycle);
    )

    _lastCycle = cycle;

    // Update the state of each unit/memory controller bus
    updateBusCounters(cycle);

    // Read new memory requests from input signals (buses)
    // MANDATORY to execute it before stage_serveRequests()
    stage_readRequests(cycle);

    // Serve a pending request from the serviceQueue
    stage_serveRequest(cycle);

    // Process memory controller commands coming from CP
    processCommand(cycle);

    // Read and dump bus usage signals (STV feedback)
    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        readUsageBuses(cycle);

    stat_avgRequestBufferAllocated->incavg(requestQueueSize - freeRequestQueue.items());
    stat_requestBufferAccumSizeStat->inc(requestQueueSize - freeRequestQueue.items());

    stat_avgServiceQueueItems->incavg(serviceQueue.items());
    stat_serviceQueueAccumSizeStat->inc(serviceQueue.items());

    // reserve the bus for next request
    reserveGPUBus(cycle);

    // Release request buffer entries of completed requests and
    // move completed read requests to the serviceQueue
    stage_updateCompletedRequests(cycle);

    // Update system buses counters
    updateSystemBuses(cycle);

    // Issue a new memory transaction
    issueSystemTransaction(cycle);

    // Check for completed requests
    processSystemMemoryReplies(cycle);

    // Send state feedback to clients
    sendBusStateToClients(cycle);
}

void MemoryController::updateMemoryDomain(U64 cycle)
{
    CG_WARN("MemoryController => Memory Domain. Clock %lld\n", cycle);

    _lastCycleMem = cycle;

    // Clock children boxes: channel schedulers and their attached ddr modules
    for ( U32 i = 0; i < gpuMemoryChannels; i++ )
    {
        channelScheds[i]->clock(cycle);
        ddrModules[i]->clock(cycle);
    }

    // receive reply channel transactions from schedulers
    stage_receiveFromSchedulers(cycle);

    // send channel transactions to the channel schedulers
    stage_sendToSchedulers(cycle);
}


void MemoryController::saveMemory() const
{
    ofstream out;

    //  Create snapshot file for the gpu memory.
    out.open("mcv2.gpumem.snapshot", ios::binary);

    //  Check if file was open/created correctly.
    if ( !out.is_open() )
        CG_ASSERT("Error creating gpu memory snapshot file.");

    const U32 BurstBytes = gpuBurstLength * 4;

    for ( U32 addr = 0; addr < gpuMemorySize; addr += BurstBytes ) {
        // Get the proper splitter
        const MemoryRequestSplitter& splitter = selectSplitter(addr, BurstBytes);
        // Convert a linear address into a (channel,bank,row,col) tuple
        const MemoryRequestSplitter::AddressInfo addrInfo = splitter.extractAddressInfo(addr);
        // Read the next burst data and write it into the output stream
        ddrModules[addrInfo.channel]->readData(addrInfo.bank, addrInfo.row, addrInfo.startCol, BurstBytes, out);
    }

    out.close();

    out.open("mcv2.sysmem.snapshot", ios::binary);

    //  Check if file was open/created correctly.
    if (!out.is_open())
        CG_ASSERT("Error creating system memory snapshot file.");

    //  Dump the system content into the file.
    out.write((char *) systemMemory, systemMemorySize);

    //  Close the file.
    out.close();
}

void MemoryController::loadMemory()
{
    ifstream in;

    //  Open snapshot file for the gpu memory.
    in.open("mcv2.gpumem.snapshot", ios::binary);


    if (in.is_open() ) {
        const U32 BurstBytes = gpuBurstLength * 4;

        for ( U32 addr = 0; addr < gpuMemorySize; addr += BurstBytes ) {
            // Get the proper splitter
            const MemoryRequestSplitter& splitter = selectSplitter(addr, BurstBytes);
            // Convert a linear address into a (channel,bank,row,col) tuple
            const MemoryRequestSplitter::AddressInfo addrInfo = splitter.extractAddressInfo(addr);
            // Read the next burst data and write it into the output stream
            ddrModules[addrInfo.channel]->writeData(addrInfo.bank, addrInfo.row, addrInfo.startCol, BurstBytes, in);
        }

        //  Close the file.
        in.close();
    }
    else {
        //  Check if file was opened correctly.
        //CG_ASSERT("Error opening gpu memory snapshot file.");
        std::cerr << "Error loading GPU memory. File ' " << "mcv2.gpumem.snapshot not found (loading GPU memory ignored)" << endl;
    }

    //  Open snapshot file for the system memory.
    in.open("mcv2.sysmem.snapshot", ios::binary);

    if ( in.is_open() ) {
        //  Load the system content from the file.
        in.read((char *) systemMemory, systemMemorySize);

        //  Close the file.
        in.close();
    }
    else {
        //  Check if file was opened correctly.
        // CG_ASSERT("Error opening system memory snapshot file.");
        std::cerr << "Error loading System memory. File ' " << "mcv2.sysmem.snapshot not found (loading system memory ignored)" << endl;
    }
}


string MemoryController::getRangeList(const vector<U32>& listOfIndices)
{
    if ( listOfIndices.empty() )
        return string("[]");

    stringstream ss;
    map<U32,U32> indices;

    // Sort and count indices
    for ( vector<U32>::const_iterator it = listOfIndices.begin()
          ; it != listOfIndices.end(); ++it )
    {
        map<U32,U32>::iterator it2 = indices.find(*it);
        if ( it2 != indices.end() )
            it2->second++; // already present, inc counter
        else
            indices.insert(make_pair(*it,1));
    }

    if ( indices.size() == 1 )
    {
        ss << "{" << indices.begin()->first;
        if ( listOfIndices.size() > 1 )
            ss << "(" << indices.begin()->second << ")";

        ss << "}";
        return ss.str();
    }

    // Generate string
    ss << "{ ";
    map<U32,U32>::const_iterator itStart = indices.begin();
    map<U32,U32>::const_iterator itPrev = indices.begin();
    map<U32,U32>::const_iterator it = indices.begin();
    ++it;
    for ( ; it != indices.end(); ++it )
    {
        // cout << "\nitStart (first,second): (" << itStart->first << "," << itStart->second << ")\n";
        // cout << "itPrev (first,second): (" << itPrev->first << "," << itPrev->second << ")\n";
        // cout << "it (first,second): (" << it->first << "," << it->second << ")\n";
        if ( itPrev->second > 1 )
        {
            ss << itPrev->first << "(" << itPrev->second << "), ";
            itStart = it;
        }
        else if ( it->second > 1 || itPrev->first + 1 != it->first )
        {
            if ( itStart != itPrev )
                ss << "[" << itStart->first << ".." << itPrev->first << "], ";
            else
                ss << itStart->first << ", ";
            itStart = it;
        }
        itPrev = it;
    }
    if ( itPrev->second > 1 )
        ss << itPrev->first << "(" << itPrev->second << ")";
    else if ( itPrev == itStart )
        ss << itPrev->first;
    else {
        ss << "[" << itStart->first << ".." << itPrev->first << "]";
    }

    ss << " }";

    return ss.str();
}

void MemoryController::getDebugInfo(string &debugInfo) const
{
    vector<U32> rbEntriesInUse[2];
    for ( U32 i = 0; i < requestQueueSize; ++i )
    {
        if ( requestBuffer[i].isOccupied() )
            rbEntriesInUse[0].push_back(i);
        if ( systemRequestBuffer[i].isOccupied() )
            rbEntriesInUse[1].push_back(i);
    }

    stringstream ss;
    ss << "MEMORY CONTROLLER V2 (debug info) \n"
        "   (GPU domain cycle, MEM domain cycle): (" << _lastCycle << "," << _lastCycleMem << ")\n"
        "   Request buffer entries in use (max: " << requestQueueSize << "): " << (requestQueueSize - freeRequestQueue.items()) << "\n"
        "    -> " << getRangeList(rbEntriesInUse[0]) << "\n"
        "   System request buffer entries in use (max: " << requestQueueSize << ") : " << (requestQueueSize - systemFreeRequestQueue.items()) << "\n"
        "    -> " << getRangeList(rbEntriesInUse[1]) << "\n"
        "   Service queue entries in use (max: " << serviceQueue.capacity() << "): " << serviceQueue.items() << "\n"
        // "   Free services: " << freeServices << "\n"
        "   Free read buffers: " << freeReadBuffers << "\n"
        "   Free write buffers: " << freeWriteBuffers << "\n"
        "";


    ss << "   DATA IN IO BUSES (ReqType.MemSpace.CyclesInBus.ReqBufEntry):\n";
    for ( U32 i = 0; i < LASTGPUBUS; ++i )
    {
        if ( _busState[i].empty() ) // ignore buses not used (ex: MEMORYMODULE)
            continue;

        // for ( U32 j = 0; j < currentTrans[i].size(); ++j )
        bool busInUse = false;
        for ( U32 j = 0; j < _busState[i].size(); ++j )
        {
            const BusState& busState = _busState[i][j];
            if ( busState.busCycles > 0 )
            {
                if ( !busInUse ) {
                    ss << "     " << MemoryTransaction::getBusName(static_cast<GPUUnit>(i)) << "\n";
                    busInUse = true;
                }
                ss << "     " << "  [" << j << "]: ";
                ss << (busState.service?"R":"W") << "." << (busState.isSystemTrans?"s":"g")
                   << "." << busState.busCycles << "." << busState.rbEntry
                   << " -> " << busState.mt->toString() << "\n";
            }
            else {
                // ss << "Idle";
            }
        }
        /*
        if ( ! _busState[i].empty() && busInUse )
            ss << "\n";
        */
    }

    ss << "   BUS RESERVATION STATE:\n";
    for ( U32 i = 0; i < LASTGPUBUS; ++i )
    {
        if ( _busState[i].empty() || i == MEMORYMODULE )
            continue;

        bool busReserved = false;
        for ( U32 j = 0; j < _busState[i].size(); ++j )
        {
            const BusState& busState = _busState[i][j];
            if ( busState.reserveBus ) {
                if ( !busReserved ) {
                    ss << "    " << MemoryTransaction::getBusName(static_cast<GPUUnit>(i)) << "\n";
                    busReserved = true;
                }
                ss << "    " << "  [" << j << "] = RESERVED\n";
            }
        }
    }

    debugInfo = ss.str();
}

//  List the debug commands supported by the Command Processor
void MemoryController::getCommandList(std::string &commandList)
{
    commandList.append("savememory  - Saves GPU and system memory to snapshot files.\n");
    commandList.append("loadmemory  - Loads GPU and system memmory from snapshot files.\n");
    commandList.append("_savememory - Saves GPU and system memory to snapshot files (silent).\n");
    commandList.append("_loadmemory - Loads GPU and system memmory from snapshot files (silent).\n");
}

//  Execute a debug command
void MemoryController::execCommand(stringstream &commandStream)
{
    string command;

    commandStream >> ws;
    commandStream >> command;

    if (!command.compare("savememory"))
    {
        if (!commandStream.eof())
        {
            cout << "Usage: " << endl;
            cout << "savememory" << endl;
        }
        else
        {
            cout << " Saving memory to file.\n";
            saveMemory();
        }
    }
    else if (!command.compare("loadmemory"))
    {
        if (!commandStream.eof())
        {
            cout << "Usage: " << endl;
            cout << "loadmemory" << endl;
        }
        else
        {
            cout << " Loading memory from file.\n";
            loadMemory();
        }
    }
    else if (!command.compare("_savememory"))
    {
        saveMemory();
    }
    else if (!command.compare("_loadmemory"))
    {
        loadMemory();
    }
}
