/**************************************************************************
 *
 * Global Profiler definition and implementation file.
 *  This file defines and implements a Global Profiler class that can be used for
 *  profiling in a whole program.
 *
 */
#include "Profiler.h"

#undef GLOBAL_PROFILER_ENTER_REGION
#undef GLOBAL_PROFILER_EXIT_REGION
#undef GLOBAL_PROFILER_GENERATE_REPORT

//#define ENABLE_GLOBAL_PROFILER
#ifdef ENABLE_GLOBAL_PROFILER
    #define GLOBAL_PROFILER_ENTER_REGION(a, b, c) cg1gpu::GlobalProfiler::instance().enterRegion((a), (b), (c));
    #define GLOBAL_PROFILER_EXIT_REGION()         cg1gpu::GlobalProfiler::instance().exitRegion();
    #define GLOBAL_PROFILER_GENERATE_REPORT(a)    cg1gpu::GlobalProfiler::instance().generateReport((a));
#else
    #define GLOBAL_PROFILER_ENTER_REGION(a, b, c)
    #define GLOBAL_PROFILER_EXIT_REGION()
    #define GLOBAL_PROFILER_GENERATE_REPORT(a)
#endif

#ifndef _GLOBALPROFILER_

#define _GLOBALPROFILER_


namespace cg1gpu
{

class GlobalProfiler : public Profiler
{
private:

    //  Avoid explicit constructor and copies.
    GlobalProfiler();
    GlobalProfiler(const cg1gpu::GlobalProfiler&);
    GlobalProfiler& operator=(const GlobalProfiler&);

public:

    static GlobalProfiler& instance();
};

}   // namespace cg1gpu


#endif

