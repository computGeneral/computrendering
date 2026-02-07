/**************************************************************************
 *
 */

#include "cmStatisticsManager.h"
#include "support.h"
#include <algorithm>

using namespace cg1gpu;
using namespace cg1gpu::gpuStatistics;
using namespace std;

// StatisticsManager* StatisticsManager::sm = 0;

StatisticsManager::StatisticsManager():
startCycle(0), nCycles(1000), nextDump(999), lastCycle(-1), autoReset(true),
osCycle(NULL), osFrame(NULL), osBatch(NULL), cyclesFlagNamesDumped(false)
{
}

StatisticsManager& StatisticsManager::instance()
{
    static StatisticsManager* sm = new StatisticsManager();
    return *sm;
}

Statistic* StatisticsManager::find(std::string name)
{
    map<string,Statistic*>::iterator it = stats.find(name);
    if ( it == stats.end() )
        return 0;
    return it->second;
}

void StatisticsManager::clock(U64 cycle)
{
    // static bool flag = false;
    lastCycle = cycle;

    if ( cycle >= startCycle )
        Statistic::enable();
    else
        Statistic::disable();

    if ( cycle >= nextDump )
    {
        if ( !cyclesFlagNamesDumped )
        {
            cyclesFlagNamesDumped = true;
            
            if (osCycle != NULL)
                dumpNames(*osCycle);
        }

        if (osCycle != NULL)
            dumpValues(*osCycle);

        if ( autoReset )
        {
            startCycle = cycle+1;
            reset(FREQ_CYCLES);
        }
        nextDump = cycle + nCycles;
    }
}

void StatisticsManager::frame(U32 frame)
{
    static bool namesOut = false;

    //  Check if output stream for per frame statistics is defined.
    if (osFrame != NULL)
    {
        if (namesOut == false)
        {
            namesOut = true;
            dumpNames("Frame", *osFrame);
        }

        dumpValues(frame, FREQ_FRAME, *osFrame);

        reset(FREQ_FRAME);
    }
}

void StatisticsManager::batch()
{
    static bool namesOut = false;
    static U32 batch = 0;

    //  Check if the output stream for per batch statistics is defined
    if (osBatch != NULL)
    {
        if (namesOut == false)
        {
            namesOut = true;
            dumpNames("Batch", *osBatch);
        }

        dumpValues(batch, FREQ_BATCH, *osBatch);

        batch++;

        reset(FREQ_BATCH);
    }
}

Statistic* StatisticsManager::operator[](std::string statName)
{
    return find(statName);
}


void StatisticsManager::setDumpScheduling(U64 startCycle, U64 nCycles, bool autoReset)
{
    this->startCycle = startCycle;
    this->nextDump= startCycle+nCycles-1;
    this->nCycles = nCycles; // dump every 'nCycles' cycles 
    this->autoReset = autoReset;
}

void StatisticsManager::setOutputStream(ostream& os)
{
    osCycle = &os;
}

void StatisticsManager::setPerFrameStream(ostream& os)
{
    osFrame = &os;
}

void StatisticsManager::setPerBatchStream(ostream& os)
{
    osBatch = &os;
}

void StatisticsManager::reset(U32 freq)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
        (it->second)->clear(freq);
}

void StatisticsManager::dumpValues(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << startCycle << ".." << lastCycle;
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(FREQ_CYCLES);
        os << "," << *(it->second);
    }
    os << endl;
}

void StatisticsManager::dumpValues(U32 n, U32 freq, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << n;
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(freq);
        os << "," << *(it->second);
    }

    os << endl;
}


void StatisticsManager::dumpNames(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << "Cycles";
    for ( ; it != stats.end(); it++ )
        os << "," << it->first;
    os << endl;
}

void StatisticsManager::dumpNames(char *str, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << str;
    for ( ; it != stats.end(); it++ )
        os << "," << it->first;
    os << endl;
}

void StatisticsManager::dump(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        os << it->second->getName() << " = " << *(it->second) << endl;
    }
}


void StatisticsManager::dump(string boxName, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        if ( it->second->getOwner() == boxName )
            os << it->second->getName() << " = " << *(it->second) << endl;
    }

}

void StatisticsManager::finish()
{
    U64 prevDump = nextDump + 1 - nCycles;
    if ( lastCycle > prevDump ) {
        if ( osCycle ) {
            if ( !cyclesFlagNamesDumped )
                dumpNames(*osCycle);
            dumpValues(*osCycle);
        }    
    }
}
