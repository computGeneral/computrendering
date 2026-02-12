/**************************************************************************
 *
 */

#ifndef MEMORYTRACERECORDER_H
    #define MEMORYTRACERECORDER_H

#include "GPUType.h"
#include "cmMemoryControllerDefs.h"
#include <fstream>
// #include "zfstream.h"


namespace arch 
{
namespace memorycontroller
{

class MemoryTraceRecorder
{
public:

    bool open(const char* path);

    void record( U64 cycle, GPUUnit clientUnit, U32 clientSubUnit, MemTransCom memoryCommand,
                 U32 address, U32 size);

private:

    std::ofstream trace;
    // gzofstream trace;

}; // class MemoryTraceRecorder

} // namespace memorycontroller
} // namespace arch

#endif // MEMORYTRACERECORDER_H
