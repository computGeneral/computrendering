/**************************************************************************
 * Parser definition file.
 * This file defines the Profiler class used as a profiling tool.
 *
 */

#include "GPUType.h"

#include <map>
#include <vector>

using namespace std;

#ifndef _PROFILER_
#define _PROFILER_

// Macros for use within code so it gets compiled out

// Function prototypes
namespace cg1gpu
{

class Profiler
{

private:

    struct ProfileRegion
    {
        U64 ticks;
        U64 visits;
        U64 ticksTotal;
        U64 visitsTotal;
        string regionName;
        string className;
        string functionName;
    };

    map<string, ProfileRegion *> profileRegions;
    vector<ProfileRegion *> regionStack;
    U32 regionStackSize;
    U64 lastTickSample;
    U64 allTicks;
    U64 allTicksTotal;
    ProfileRegion *currentProfileRegion;
    static U64 sampleTickCounter();
    U32 lastProcessor;
    bool differentProcessors;
    U32 processorChanges;
    
public:
    Profiler();
    ~Profiler();
    void reset();
    void enterRegion(char *regionName, char *className, char *functionName);
    void exitRegion();
    void generateReport(char *fileName);
};

}   // namespace cg1gpu


#endif    // _PROFILER_
