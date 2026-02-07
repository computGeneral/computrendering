/**************************************************************************
 *
 */

#include "MemoryControllerSelector.h"
#include "cmMemoryController.h"
#include "cmMemoryControllerV2.h"
#include <iostream>
#include "cmGPUMemorySpecs.h"


using namespace cg1gpu;

cmoMduBase* cg1gpu::createMemoryController(cgsArchConfig& arch, 
                            const char** tuPrefixes, 
                            const char** suPrefix,
                            const char** slPrefixes,
                            const char* memoryControllerName,
                            cmoMduBase* parentBox)
{
    using std::cout;
    using std::endl;
    
    U32 gpuMemSize;
    U32 mappedMemSize;
    
    GPU_ASSERT(
        if (arch.mem.memSize > 2048)
            CG_ASSERT("Maximum GPU memory size allowed is 2 GBs.");
        if (arch.mem.mappedMemSize > 2048)
            CG_ASSERT("Maximum mapped (system) memory size allowed is 2 GBs.");
    )
    
    //  The memory size coming from the configuration files is in MBs but the Memory Controller expects it in
    //  bytes (for now).
    gpuMemSize = arch.mem.memSize * 1024 * 1024;
    mappedMemSize = arch.mem.mappedMemSize * 1024 * 1024;
    
    cmoMduBase* MC;
    if ( !arch.mem.memoryControllerV2 )
    {
        // Select legacy memory controller
        CG_INFO("Using legacy Memory Controller...");
        MC = new MemoryController(
            gpuMemSize,                 //  GPU Memory Size.  
            arch.mem.clockMultiplier,   //  Frequency multiplier applied to the GPU clock to be used as the reference memory frequency.  
            arch.mem.memoryFrequency,   //  Memory frequency in cycles from the reference memory frequency.  
            arch.mem.busWidth,          //  Bus width in bits.  
            arch.mem.memBuses,          //  Number of buses to the memory modules.  
            arch.mem.sharedBanks,       //  Flag that enables shared access to all memory banks from all gpu buses.  
            arch.mem.bankGranurality,   //  Access granurality to gpu memory banks.  
            arch.mem.burstLength,       //  Number of words (bus width) transfered per memory access in a gpu bus.  
            arch.mem.readLatency,       //  Cycles from read command to data from GPU memory.  
            arch.mem.writeLatency,      //  Cycles from write command to data towards GPU memory.  
            arch.mem.writeToReadLat,    //  Cycles from last written data to GPU memory to next read command.  
            arch.mem.memPageSize,       //  Size in bytes of a GPU memory page.  
            arch.mem.openPages,         //  Number of pages open per memory bus.  
            arch.mem.pageOpenLat,       //  Latency of opening a new page.  
            arch.mem.maxConsecutiveReads,   //  Number of consecutive read transactions before the next write transaction for a bus.  
            arch.mem.maxConsecutiveWrites,  //  Number of consecutive write transactions before the next read transaction for a bus.  
            arch.mem.comProcBus,        //  Bus with the Command Processor (bytes).  
            arch.mem.strFetchBus,       //  Bus with the cmoStreamController Fetch (bytes).   
            arch.mem.strLoaderBus,      //  Bus with the cmoStreamController Loader (bytes).  
            arch.mem.zStencilBus,       //  Bus with Z Stencil Test (bytes).  
            arch.mem.cWriteBus,         //  Bus with Color Write (bytes).  
            arch.mem.dacBus,            //  Bus with DisplayController (bytes).  
            arch.mem.textUnitBus,       //  Texture Unit bus (bytes).  
            mappedMemSize,              //  Amount of system memory mapped into the GPU address space.  
            arch.mem.readBufferLines,   //  Memory buffer lines for read transactions.  
            arch.mem.writeBufferLines,  //  Memory buffer lines for write transactions.  
            arch.mem.reqQueueSz,        //  Request queue size.  
            arch.mem.servQueueSz,       //  Service queue size.  
            arch.gpu.numStampUnits,     //  Number of Stamp Units attached.  
            suPrefix,                   //  Array of prefixes for the Stamp Units.  
            arch.gpu.numFShaders * arch.ush.textureUnits,   //  Number of Texture Units attached.  
            tuPrefixes,                 //  Array of prefixes for the Texture Units.  
            arch.str.streamerLoaderUnits,//  Number of cmoStreamController Loader units.  
            slPrefixes,                 //  Array of prefixes for the cmoStreamController Loader units.  
            "MemoryController",
            NULL);
    }
    else
    {
        CG_INFO("Using Memory Controller Version 2...");

        using memorycontroller::MemoryControllerParameters;

        MemoryControllerParameters::SchedulerType schedType;
        MemoryControllerParameters::SchedulerPagePolicy schedPagePolicy;

        schedType = (MemoryControllerParameters::SchedulerType)arch.mem.v2ChannelScheduler;

        if ( arch.mem.v2PagePolicy == 0 )
            schedPagePolicy = MemoryControllerParameters::ClosePage;
        else if ( arch.mem.v2PagePolicy == 1 )
            schedPagePolicy = MemoryControllerParameters::OpenPage;

        memorycontroller::MemoryControllerParameters params;

        // default to 0
        memset(&params, '\0', sizeof(memorycontroller::MemoryControllerParameters));

        string memtype;
        if ( arch.mem.v2MemoryType == 0 ) {
            cout << "MCv2 -> No V2MemoryType defined, defaulting to GDDR3" << endl;
            memtype = "gddr3";            
        }
        else {
            memtype = arch.mem.v2MemoryType;
        }        
        
        if ( memtype == "gddr3" ) {  
            if ( arch.mem.v2GDDR_Profile == 0 ) {
                cout << "MCv2 -> No V2GDDR_Profile found for GDDR3 memory, defaulting to V2GDDR_Profile=\"600\"" << endl;
                params.memSpecs = new memorycontroller::GDDR3Specs600MHz();
            }
            else {
                string profile(arch.mem.v2GDDR_Profile);
                if ( profile == "perfect" ) {
                    cout << "MCv2 -> Using 'PERFECT MEMORY' profile (NO DELAY)\n";
                    params.memSpecs = new memorycontroller::GDDR3SpecsZeroDelay();
                }
                else if ( profile == "custom" ) {
                    params.memSpecs = new memorycontroller::GDDR3SpecsCustom( arch.mem.v2GDDR_tRRD,
                                                                              arch.mem.v2GDDR_tRCD,
                                                                              arch.mem.v2GDDR_tWTR,
                                                                              arch.mem.v2GDDR_tRTW,
                                                                              arch.mem.v2GDDR_tWR,
                                                                              arch.mem.v2GDDR_tRP,
                                                                              arch.mem.v2GDDR_CAS,
                                                                              arch.mem.v2GDDR_WL );
                }
                else if ( profile == "600" ) {
                    params.memSpecs = new memorycontroller::GDDR3Specs600MHz();
                }
                else {
                    stringstream ss;
                    ss << "Memory profile \"" << profile << "\" not supported";
                    CG_ASSERT(ss.str().c_str());
                }
            }
        }
        else {
            stringstream ss;
            ss << "Memory type \"" << memtype << "\" not supported";
            CG_ASSERT(ss.str().c_str());
        }

        params.burstLength = arch.mem.burstLength;
        params.comProcBus = arch.mem.comProcBus;
        params.streamerFetchBus = arch.mem.strFetchBus;
        params.streamerLoaderBus = arch.mem.strLoaderBus;
        params.zStencilTestBus = arch.mem.zStencilBus;
        params.colorWriteBus = arch.mem.cWriteBus;
        params.dacBus = arch.mem.dacBus;
        params.textUnitBus = arch.mem.textUnitBus;
        params.gpuMemorySize = gpuMemSize;
        params.systemMemorySize = mappedMemSize;
        params.systemMemoryBuses = memorycontroller::SYSTEM_MEMORY_BUSES;
        params.systemMemoryReadLatency = memorycontroller::SYSTEM_MEMORY_READ_LATENCY;
        params.systemMemoryWriteLatency = memorycontroller::SYSTEM_MEMORY_WRITE_LATENCY;
        params.readBuffers = arch.mem.readBufferLines;
        params.writeBuffers = arch.mem.writeBufferLines;
        params.requestQueueSize = arch.mem.reqQueueSz;
        params.serviceQueueSize = arch.mem.servQueueSz;
        params.numStampUnits = arch.gpu.numStampUnits;
        params.numTexUnits = arch.gpu.numFShaders * arch.ush.textureUnits;
        params.streamerLoaderUnits = arch.str.streamerLoaderUnits;

        params.perBankChannelQueues = arch.mem.v2UseChannelRequestFIFOPerBank;
        params.perBankChannelQueuesSelection = arch.mem.v2ChannelRequestFIFOPerBankSelection;

        params.memoryChannels = arch.mem.v2MemoryChannels;
        params.banksPerMemoryChannel = arch.mem.v2BanksPerMemoryChannel;
        params.memoryRowSize = arch.mem.v2MemoryRowSize;
        params.burstBytesPerCycle = arch.mem.v2BurstBytesPerCycle;
        params.channelInterleaving = arch.mem.v2ChannelInterleaving;
        params.bankInterleaving = arch.mem.v2BankInterleaving;

        // second channel interleaving configuration
        params.enableSecondInterleaving = arch.mem.v2SecondInterleaving;
        params.secondChannelInterleaving = arch.mem.v2SecondChannelInterleaving;
        params.secondBankInterleaving = arch.mem.v2SecondBankInterleaving;

        params.splitterType = arch.mem.v2SplitterType;

        params.channelInterleavingMask = new string(
            (arch.mem.v2ChannelInterleavingMask ? arch.mem.v2ChannelInterleavingMask : "") );

        params.bankInterleavingMask =  new string(
            (arch.mem.v2BankInterleavingMask ? arch.mem.v2BankInterleavingMask : "") );

        params.secondChannelInterleavingMask = new string(
            (arch.mem.v2SecondChannelInterleavingMask ? arch.mem.v2SecondChannelInterleavingMask : "") );

        params.secondBankInterleavingMask = new string(
            (arch.mem.v2SecondBankInterleavingMask ? arch.mem.v2SecondBankInterleavingMask : "") );

        // params.fifo_maxChannelTransactions = arch.mem.v2MaxChannelTransactions;
        // params.rwfifo_maxReadChannelTransactions = arch.mem.v2MaxChannelTransactions;
        // params.rwfifo_maxWriteChannelTransactions = arch.mem.v2MaxChannelTransactions;
        params.maxChannelTransactions = arch.mem.v2MaxChannelTransactions;
        params.dedicatedChannelReadTransactions = arch.mem.v2DedicatedChannelReadTransactions;

        params.rwfifo_maxConsecutiveReads = arch.mem.maxConsecutiveReads;
        params.rwfifo_maxConsecutiveWrites = arch.mem.maxConsecutiveWrites;

        params.schedulerType = schedType;

        if ( arch.mem.v2BankSelectionPolicy )
            params.bankfifo_bankSelectionPolicy = new string(arch.mem.v2BankSelectionPolicy);
        else 
            params.bankfifo_bankSelectionPolicy = new string("OLDEST_FIRST RANDOM"); // default

        params.switchModePolicy = arch.mem.v2SwitchModePolicy;
        params.bankfifo_activeManagerMode = arch.mem.v2ActiveManagerMode;
        params.bankfifo_prechargeManagerMode = arch.mem.v2PrechargeManagerMode;

        params.bankfifo_disableActiveManager = arch.mem.v2DisableActiveManager;
        params.bankfifo_disablePrechargeManager = arch.mem.v2DisablePrechargeManager;
        params.bankfifo_managerSelectionAlgorithm = arch.mem.v2ManagerSelectionAlgorithm;

        params.schedulerPagePolicy = schedPagePolicy;

        // Enable/disable memory trace dump file
        params.memoryTrace = arch.mem.v2MemoryTrace;

        params.debugString = ( arch.mem.v2DebugString ? new string(arch.mem.v2DebugString) : new string("") );

        params.useClassicSchedulerStates = arch.mem.v2UseClassicSchedulerStates;

        params.useSplitRequestBufferPerROP = arch.mem.v2UseSplitRequestBufferPerROP;

        MC = 
            new memorycontroller::MemoryController( params,
                                                    (const char**)tuPrefixes, 
                                                    (const char**)suPrefix,
                                                    (const char**)slPrefixes,
                                                    "MemoryControllerV2", 
                                                    true, // create inner signals
                                                    0);

        delete params.memSpecs;
    }

    return MC;

}
