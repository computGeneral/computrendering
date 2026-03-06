/**************************************************************************
 *
 */

#ifndef STATISTICSMANAGER_H
#define STATISTICSMANAGER_H

#include "cmStatistic.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "support.h"
#include "GPUType.h"

namespace arch
{

namespace gpuStatistics
{

static const U32 FREQ_CYCLES = 0;
static const U32 FREQ_FRAME = 1;
static const U32 FREQ_BATCH = 2;

class StatisticsManager
{
private:

    // list of current stats 
    std::map<std::string,gpuStatistics::Statistic*> stats;

    // Helper method to find a Statistic with name 'name' 
    Statistic* find(std::string name);

    // dump scheduling 
    U64 startCycle;
    U64 nCycles;
    U64 nextDump;
    U64 lastCycle;
    bool autoReset;

    // current output per cycle stream 
    std::ostream* osCycle;

    //  output streams for per batch and per frame statistics.  
    std::ostream* osFrame;
    std::ostream* osBatch;

    //! Buffer for transposed output: column headers and per-column values.
    struct TransposedData {
        std::vector<std::string> colHeaders;
        std::vector<std::vector<std::string>> colValues; // [colIdx][statIdx]
    };

    TransposedData transCycle;
    TransposedData transFrame;
    TransposedData transBatch;

    //! Buffer one column of values into a TransposedData struct.
    void bufferColumn(TransposedData& buf, const std::string& header, U32 freq);

    //! Write the transposed table to an output stream.
    void flushTransposed(TransposedData& buf, const std::string& rowLabel,
                         std::ostream& os);

    // singleton instance 
    // static StatisticsManager* sm;

    // avoid copying 
    StatisticsManager();
    StatisticsManager(const StatisticsManager&);
    StatisticsManager& operator=(const StatisticsManager&);

public:

    static StatisticsManager& instance();


    template<typename T>
    gpuStatistics::NumericStatistic<T>& getNumericStatistic(const char* name, T initialValue, const char* owner = 0, const char* postfix=0)
    {
        char temp[256];
        if ( postfix != 0 )
        {
            sprintf(temp, "%s_%s", name, postfix);
            name = temp;
        }
        Statistic* st = find(name);
        NumericStatistic<T>* nst;
        if ( st )
        {
            /*
             * Warning (in windows):
             * Use this flag (-GR) in VS6 to achive RTTI in dynamic_cast
             *
             *  -GR    enable standard C++ RTTI (Run Time Type
             *         Identification)
             */
            nst = dynamic_cast<NumericStatistic<T>*>(st);
            CG_ASSERT_COND((nst != 0), "Another Statistic exists with name '%s' but has different type", name);
            return *nst;
        }

        nst = new NumericStatistic<T>(name,initialValue);
        stats.insert(std::make_pair(name, nst));

        if ( owner != 0 )
            nst->setOwner(owner);

        return *nst;
    }

    Statistic* operator[](std::string statName);

    virtual void clock(U64 cycle);

    virtual void frame(U32 frame);

    virtual void batch();

    void setDumpScheduling(U64 startCycle, U64 nCycles, bool autoReset = true);

    void setOutputStream(std::ostream& os);

    void setPerFrameStream(std::ostream& os);

    void setPerBatchStream(std::ostream& os);

    void reset(U32 freq);

    void dump(std::ostream& os = std::cout);

    void dump(std::string boxName, std::ostream& os = std::cout);

    void finish();

};


} // namespace gpuStatistics

} // namespace arch

#endif // STATISTICSMANAGER_H
