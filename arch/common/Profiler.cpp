/**************************************************************************
 * Profiler implementation file.
 *
 * This file implements the Profiler class used as a profiling tool.
 *
 */

#include "Profiler.h"

#include <cstdio>

#ifdef WIN32
    #define _WIN32_WINNT 0x400
    #include "windows.h"
    #include <intrin.h>
    
    #define FORCE_PROCESSOR
    //#define CHECK_PROCESSOR
    
#endif

#ifdef WIN32
    #pragma intrinsic(__rdtsc)
#endif

namespace cg1gpu
{

Tracer::Tracer()
{
    traceRegions.clear();
    regionStack.clear();
    regionStackSize = 0;
    allTicks = 0;
    allTicksTotal = 0;

#ifdef WIN32

#ifdef FORCE_PROCESSOR
    SetThreadAffinityMask(GetCurrentThread(), 0x01);
#endif

#ifdef CHECK_PROCESSOR    
    lastProcessor = GetCurrentProcessorNumber();
    printf("Tracer => Initialized at processor %d\n", lastProcessor);
#endif
    
#endif    
}

Tracer::~Tracer()
{
    map<string, TraceRegion*>::iterator it;
    it = traceRegions.begin();
    while (it != traceRegions.end())
    {
        delete it->second;
        it++;
    }

    traceRegions.clear();
    regionStack.clear();
}

U64 Tracer::sampleTickCounter()
{
#ifdef WIN32

    //  This may not work in a multiprocessor if the process
    //  moves from one processor to another.
    return __rdtsc();	

#else

    U64 tickSample;
    
    //  Not sure if this code works on Linux or Cygwin.
    
    __asm__ __volatile__ ("rdtsc" : "=&A" (tickSample));
    
    return tickSample;
    
#endif
}

void Tracer::enterRegion(char *regionName, char *className, char *functionName)
{
    //  Update the tick count for the current region.
    U64 ticks = sampleTickCounter() - lastTickSample;

#ifdef WIN32    
#ifdef CHECK_PROCESSOR    
    U32 proc = GetCurrentProcessorNumber();           
    differentProcessors = (lastProcessor != proc);
    if (differentProcessors)
    {
        processorChanges++;
        lastProcessor = proc;
    }
#endif    
#endif
                
    //  Check if inside a region.
    if (regionStackSize != 0)
    {
        regionStack[regionStackSize - 1]->ticks += ticks;
        allTicks += ticks;
    }
    
    //  Search the region in the map of profile regions.
    map<string, TraceRegion*>::iterator it;
    it = traceRegions.find(string(regionName));
    if (it != traceRegions.end())
    {
        //  Found.  Set as current profile region.
        it->second->visits++;
        regionStack.push_back(it->second);
        regionStackSize++;
    }
    else
    {
        //  Not found.  Create new region.
        TraceRegion *region = new TraceRegion;
        region->ticks = 0;
        region->ticksTotal = 0;
        region->visits = 1;
        region->visitsTotal = 0;
        region->regionName = string(regionName);
        region->className = string(className);
        region->functionName = string(functionName);
        
        //  Add region to the map.
        traceRegions[string(regionName)] = region;
        
        //  Set as current profile region.
        regionStack.push_back(region);
        regionStackSize++;
    }
    
    //  Sample tick counter.
    lastTickSample = sampleTickCounter();;
}

void Tracer::exitRegion()
{
    //  Update the tick count for the region.
    U64 ticks = sampleTickCounter() - lastTickSample;

#ifdef WIN32
#ifdef CHECK_PROCESSOR    
    U32 proc = GetCurrentProcessorNumber();           
    differentProcessors = (lastProcessor != proc);
    if (differentProcessors)
    {
        processorChanges++;
        lastProcessor = proc;
    }  
#endif
#endif    

    //  Check if inside a region.
    if (regionStackSize != 0)
    {
        regionStack[regionStackSize - 1]->ticks += ticks;
        allTicks += ticks;
     
        //  Exit this region and move to the previous one.
        regionStack.pop_back();
        regionStackSize--;
    }
    
    //  Sample tick counter.
    lastTickSample = sampleTickCounter();
}

void Tracer::reset()
{
    map<string, TraceRegion*>::iterator it;
    it = traceRegions.begin();
    while (it != traceRegions.end())
    {
        it->second->ticksTotal += it->second->ticks;
        it->second->ticks = 0;
        it->second->visitsTotal = it->second->visits;
        it->second->visits = 0;
        
        it++;
    }
    allTicksTotal += allTicks;
    allTicks = 0;
}

void Tracer::generateReport(char *filename)
{

    FILE *outFile = fopen(filename, "w");
    
    if (outFile == NULL)
        CG_ASSERT("Error opening profiler report file.");
    
    U32 maxLength[3];
    maxLength[0] = 15;
    maxLength[1] = 10;
    maxLength[2] = 15;
    
    //  Order by the number of ticks spend in the region.
    multimap<U64, TraceRegion *> orderedRegions;
    
    map<string, TraceRegion*>::iterator it;
    it = traceRegions.begin();
    while (it != traceRegions.end())
    {
        orderedRegions.insert(pair<U64, TraceRegion*>(it->second->ticks, it->second));
        maxLength[0] = (maxLength[0] < it->second->regionName.size()) ? (U32) it->second->regionName.size() : maxLength[0];
        maxLength[1] = (maxLength[1] < it->second->className.size()) ? (U32) it->second->className.size() : maxLength[1];
        maxLength[2] = (maxLength[2] < it->second->functionName.size()) ? (U32) it->second->functionName.size() : maxLength[2];
        it++;
    }
    
    fprintf(outFile, "Total Ticks Accounted = %lld\n", allTicks);
    fprintf(outFile, "\n");

#ifdef WIN32    
#ifdef CHECK_PROCESSOR
    fprintf(outFile, "Sampled cycle counter from different processors? = %s.  Processor changes = %d\n",
        (differentProcessors ? "yes" : "false"), processorChanges);
    fprintf(outFile, "\n");
#endif CHECK_PROCESSOR    
#endif
    
    U32 paddingSpaces = (maxLength[0] - 11) >> 1;
    for(U32 c = 0; c < paddingSpaces; c++)
        fprintf(outFile, " ");
    fprintf(outFile, "Region Name");
    paddingSpaces = ((maxLength[0] - 11) >> 1) + ((maxLength[0] - 11) & 0x01);
    for(U32 c = 0; c < paddingSpaces; c++)
        fprintf(outFile, " ");
    fprintf(outFile,"   ");
    paddingSpaces = (maxLength[1] + maxLength[2] - 23) >> 1;
    for(U32 c = 0; c < paddingSpaces; c++)
        fprintf(outFile, " ");
    fprintf(outFile, "ClassName::FunctionName");
    paddingSpaces = ((maxLength[1] + maxLength[2] - 23) >> 1) + ((maxLength[1] + maxLength[2] - 23) & 0x01);
    for(U32 c = 0; c < paddingSpaces; c++)
        fprintf(outFile, " ");
    fprintf(outFile, "       Visits                Ticks              Time\n");
    fprintf(outFile, "\n");
    
    //  Print info for all the regions ordered from most ticks to less ticks.
    multimap<U64, TraceRegion*>::reverse_iterator it2;
    it2 = orderedRegions.rbegin();
    while (it2 != orderedRegions.rend())
    {
        U32 paddingSpaces = maxLength[0] - it2->second->regionName.size();
        for(U32 c = 0; c < paddingSpaces; c++)
            fprintf(outFile, " ");
        fprintf(outFile, "%s", it2->second->regionName.c_str());
        fprintf(outFile, "  ");
        paddingSpaces = maxLength[1] + maxLength[2] - it2->second->className.size() - it2->second->functionName.size();        
        for(U32 c = 0; c < paddingSpaces; c++)
            fprintf(outFile, " ");
        fprintf(outFile,"%s::%s",
            it2->second->className.c_str(),
            it2->second->functionName.c_str());
#ifdef WIN32        
        fprintf(outFile, "    %12I64d    %24I64d    %3.2f\n",
#else
        fprintf(outFile, "    %12lld    %24lld    %3.2f\n",
#endif        
            it2->second->visits,
            it2->second->ticks,
            100.0f * F64(it2->second->ticks) / F64(allTicks));
            
        it2++;
    }
    
    fclose(outFile);
}

}   // namespace cg1gpu

//=============================================================================
// Global profiler instance and accessor
//=============================================================================
namespace cg1gpu
{

static Tracer *globalTracer = NULL;

Tracer& getTracer()
{
    if (globalTracer == NULL)
        globalTracer = new Tracer();
    
    return *globalTracer;
}

}   // namespace cg1gpu


