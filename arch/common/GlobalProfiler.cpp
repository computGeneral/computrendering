/**************************************************************************
 *
 * Global Profiler implementation file.
 *  This file implements a Global Profiler class that can be used for
 *  profiling in a whole program.
 *
 */
#include "GlobalProfiler.h"

namespace cg1gpu
{

GlobalProfiler::GlobalProfiler() : Profiler()
{
}

GlobalProfiler& GlobalProfiler::instance()
{
    static GlobalProfiler *gp = NULL;
    
    if (gp == NULL)
        gp = new GlobalProfiler;
    
    return *gp;
}

}   // Namespace cg1gpu

