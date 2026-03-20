/**************************************************************************
 *
 */

#include "StatisticsManager.h"
#include "support.h"
#include <algorithm>

using namespace arch;
using namespace arch::gpuStatistics;
using namespace std;

// StatisticsManager* StatisticsManager::sm = 0;

StatisticsManager::StatisticsManager():
startCycle(0), nCycles(1000), nextDump(999), lastCycle(-1), autoReset(true),
osCycle(NULL), osFrame(NULL), osBatch(NULL)
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
    lastCycle = cycle;

    if ( cycle >= startCycle )
        Statistic::enable();
    else
        Statistic::disable();

    if ( cycle >= nextDump )
    {
        if (osCycle != NULL)
        {
            //  Buffer column for transposed output.
            ostringstream hdr;
            //hdr << startCycle << ".." << lastCycle;
            hdr << startCycle / nCycles;
            bufferColumn(transCycle, hdr.str(), FREQ_CYCLES);
        }

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
    //  Check if output stream for per frame statistics is defined.
    if (osFrame != NULL)
    {
        ostringstream hdr;
        hdr << frame;
        bufferColumn(transFrame, hdr.str(), FREQ_FRAME);

        reset(FREQ_FRAME);
    }
}

void StatisticsManager::batch()
{
    static U32 batch = 0;

    //  Check if the output stream for per batch statistics is defined
    if (osBatch != NULL)
    {
        ostringstream hdr;
        hdr << batch;
        bufferColumn(transBatch, hdr.str(), FREQ_BATCH);

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
            ostringstream hdr;
            //hdr << startCycle << ".." << lastCycle;
            hdr << startCycle / nCycles;
            bufferColumn(transCycle, hdr.str(), FREQ_CYCLES);
        }    
    }

    //  Flush transposed buffers.
    if (osCycle && !transCycle.colHeaders.empty())
        flushTransposed(transCycle, "Sample", *osCycle);
    if (osFrame && !transFrame.colHeaders.empty())
        flushTransposed(transFrame, "Frame", *osFrame);
    if (osBatch && !transBatch.colHeaders.empty())
        flushTransposed(transBatch, "Batch", *osBatch);
}

void StatisticsManager::bufferColumn(TransposedData& buf, const string& header,
                                     U32 freq)
{
    buf.colHeaders.push_back(header);

    vector<string> col;
    col.reserve(stats.size());

    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(freq);
        ostringstream ss;
        ss << *(it->second);
        col.push_back(ss.str());
    }
    buf.colValues.push_back(col);
}

void StatisticsManager::flushTransposed(TransposedData& buf,
                                        const string& rowLabel,
                                        ostream& os)
{
    //  Header row: rowLabel, col1, col2, ...
    os << rowLabel;
    for (size_t c = 0; c < buf.colHeaders.size(); c++)
        os << "," << buf.colHeaders[c];
    os << endl;

    //  One row per statistic.
    size_t statIdx = 0;
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++, statIdx++ )
    {
        os << it->first;
        for (size_t c = 0; c < buf.colValues.size(); c++)
        {
            os << ",";
            if (statIdx < buf.colValues[c].size())
                os << buf.colValues[c][statIdx];
        }
        os << endl;
    }
}