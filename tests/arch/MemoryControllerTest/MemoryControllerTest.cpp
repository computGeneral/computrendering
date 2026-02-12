/**************************************************************************
 *
 */

#include "MemoryControllerTest.h"
#include "MemoryControllerSelector.h"
#include "MemoryControllerTestBase.h"
#include "ConfigLoader.h"
#include "DynamicMemoryOpt.h"
#include <iostream>
#include "MCTConsole.h"
// #include <fstream>
#include "zfstream.h"
#include "MemoryControllerProxy.h"
#include "GPUMath.h"


using namespace arch;
using namespace std;

gpuStatistics::Statistic *cyclesCounter;    //  Counts cycles (main clock domain!).

int main(int argc, char **argv)
{   
    // Create statistic to count cycles (GPU clock domain!)
    cyclesCounter = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("CycleCounter", U32(0), "GPU", 0);

    cgsArchConfig arch;

    // Get signal binder
    SignalBinder &sigBinder = SignalBinder::getBinder();

    ConfigLoader cl("CG1GPU.ini");
    cl.getParameters(&arch);
    
    DynamicMemoryOpt::initialize(arch.objectSize0, arch.bucketSize0, arch.objectSize1, arch.bucketSize1,
        arch.objectSize2, arch.bucketSize2);

    
    //  Allocate pointer array for the texture unit prefixes.
    char** tuPrefixes = new char*[arch.gpu.numFShaders * arch.ush.textureUnits];
    //  Create prefixes for the fragment shaders.
    for(U32 i = 0; i < arch.gpu.numFShaders; i++)
    {
        for(U32 j = 0; j < arch.ush.textureUnits; j++)
        {
            //  Allocate prefix.
            tuPrefixes[i * arch.ush.textureUnits + j] = new char[10];

            CG_ASSERT_COND(!(tuPrefixes[i * arch.ush.textureUnits + j] == NULL), "Error allocating texture unit prefix.");
            //  Write prefix.
            sprintf(tuPrefixes[i * arch.ush.textureUnits + j], "FS%dTU%d", i, j);
        }
    }

    //  Allocate pointer array for StreamController loader prefixes.  
    char** slPrefixes = new char*[arch.str.streamerLoaderUnits];

    //  Check allocation.  
    CG_ASSERT_COND(!(slPrefixes == NULL), "Error allocating StreamController loader prefix array.");
    //  Create prefixes for the StreamController loader units.  
    for(U32 i = 0; i < arch.str.streamerLoaderUnits; i++)
    {
        //  Allocate prefix.  
        slPrefixes[i] = new char[6];

        CG_ASSERT_COND(!(slPrefixes[i] == NULL), "Error allocating StreamController loader prefix.");
        //  Write prefix.  
        sprintf(slPrefixes[i], "SL%d", i);
    }

    //  Allocate pointer array for the stamp unit prefixes.
    char** suPrefix = new char*[arch.gpu.numStampUnits];
   
    //  Create prefixes for the stamp units. 
    for(U32 i = 0; i < arch.gpu.numStampUnits; i++)
    {
        //  Allocate prefix.  
        suPrefix[i] = new char[6];

        CG_ASSERT_COND(!(suPrefix[i] == NULL), "Error allocating stamp unit prefix.");
        //  Write prefix. 
        sprintf(suPrefix[i], "SU%d", i);
    }

    U64 cycle = 0;
    
    cmoMduBase* mc = createMemoryController(arch, (const char**)tuPrefixes, (const char**)suPrefix, (const char**)slPrefixes, 
                                     "MemoryController", 0);

    MemoryControllerProxy mcProxy(mc);

    // MemoryControllerTestBase* mctest = new MyMCTest(arch, (const char**)tuPrefixes, (const char**)suPrefix, "MyMCTest", 0);
    MemoryControllerTestBase* mctest = new MCTConsole(arch, (const char**)tuPrefixes, (const char**)suPrefix, 
                                     (const char**)slPrefixes, "MyMCTest", mcProxy, 0);

    vector<string> params;
    if ( argc > 1 ) {
        for ( int i = 1; i < argc; ++i ) {
            params.push_back(argv[i]);
        }
    }

    mctest->parseParams(params);

    gzofstream sigTraceFile;
    // ofstream sigTraceFile;

    gzofstream out;
    // ofstream out;

    if ( !sigBinder.checkSignalBindings() ) {
        sigBinder.dump(true); // true = show only signals not properly bound
        CG_ASSERT("Signal connections undefined");
    }

    if ( arch.dumpSignalTrace ) {
        sigTraceFile.open(arch.signalDumpFile, ios::out | ios::binary);
        if ( !sigTraceFile.is_open() )
            CG_ASSERT("Error opening signal trace file");
        sigBinder.initSignalTrace(&sigTraceFile);
        cout << "MCT info: !!! GENERATING SIGNAL TRACE !!!" << endl;
    }

    if ( arch.statistics ) {
        //  Initialize the statistics manager.  
        out.open(arch.statsFile, ios::out | ios::binary);

        if ( !out.is_open() )
            CG_ASSERT("Error opening per cycle statistics file");

        gpuStatistics::StatisticsManager::instance().setDumpScheduling(0, arch.statsRate);
        gpuStatistics::StatisticsManager::instance().setOutputStream(out);
    }

    bool multiClock = (arch.gpu.gpuClock != arch.gpu.memoryClock);

    if ( !arch.mem.memoryControllerV2 && multiClock )
        CG_ASSERT("Multi clock domain is only supported with Memory Controller V2");

    U64 gpuCycle = 0; //  Cycle counter for GPU clock domain.
    U64 memoryCycle = 0; //  Cycle counter for memory clock domain.
    const U32 gpuClockPeriod = (U32) (1E6 / (F32) arch.gpu.gpuClock);
    const U32 memoryClockPeriod = (U32) (1E6 / (F32) arch.gpu.memoryClock);
    U32 nextGPUClock = gpuClockPeriod; //  Stores the picoseconds (ps) to next gpu clock.
    U32 nextMemoryClock = memoryClockPeriod;//  Stores the picoseconds (ps) to next memory clock.


    U32 WAITING_CYCLES = 500; // Wait for pending replies from MC
    U32 i = 0;
    while ( !mctest->finished() || i < WAITING_CYCLES ) 
    {
        if ( mctest->finished() )
            ++i;        
        
        if ( !multiClock ) // Unified clock domain clocking
        {
            if ( arch.dumpSignalTrace ) // Dump the signals
                sigBinder.dumpSignalTrace(cycle);
            cyclesCounter->inc();
            mc->clock(cycle);
            mctest->clock(cycle);
            if ( arch.statistics ) // Update GPU Statistics
                gpuStatistics::StatisticsManager::instance().clock(cycle);
            cycle ++;

            if ( cycle % 1000 == 0 ) {
                cout << ".";
                cout.flush();
            }
        }
        else // Multi clock domain clocking
        {
            U32 nextStep = GPU_MIN(nextGPUClock, nextMemoryClock);
            nextGPUClock -= nextStep;
            nextMemoryClock -= nextStep;
            if ( nextGPUClock == 0 ) 
            {
                cyclesCounter->inc();
                mctest->clock(gpuCycle);
                static_cast<memorycontroller::MemoryController*>(mc)->clock(GPU_CLOCK_DOMAIN, gpuCycle);
                if ( arch.statistics ) // Update GPU Statistics
                    gpuStatistics::StatisticsManager::instance().clock(gpuCycle);

                if ( gpuCycle % 1000 == 0 ){
                    cout << ".";
                    cout.flush();
                }
                ++gpuCycle;
                nextGPUClock = gpuClockPeriod;
            }
            if ( nextMemoryClock == 0 )
            {
                static_cast<memorycontroller::MemoryController*>(mc)->clock(MEMORY_CLOCK_DOMAIN, memoryCycle);
                ++memoryCycle;
                nextMemoryClock = memoryClockPeriod;
            }
        }
    }

    // Signal trace finished
    if ( arch.dumpSignalTrace ) 
        sigBinder.endSignalTrace();

    //DynamicMemoryOpt::usage();
    //DynamicMemoryOpt::dumpDynamicMemoryState();

    DynamicMemoryOpt::usage();
    gpuStatistics::StatisticsManager::instance().finish();

    return 0;
}
