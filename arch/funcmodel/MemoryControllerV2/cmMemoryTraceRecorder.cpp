/**************************************************************************
 *
 */

#include "cmMemoryTraceRecorder.h"
#include "cmMemoryTransaction.h"

using namespace std;
using namespace arch;
using namespace arch::memorycontroller;

bool MemoryTraceRecorder::open(const char* path)
{
    trace.open(path, ios::out | ios::binary);
    if ( !trace ) {
        return false;
    }
    return true; // memory trace file properly opened
}

void MemoryTraceRecorder::record( U64 cycle, GPUUnit clientUnit, U32 clientSubUnit, MemTransCom memoryCommand,
                                  U32 address, U32 size)
{

    trace << cycle << ";" << MemoryTransaction::getBusName(clientUnit) << "[" << clientSubUnit << "];";
    switch ( memoryCommand ) {
        case MT_READ_REQ:
            trace << "MT_READ_REQ;";
            break;
        case MT_READ_DATA:
            trace << "MT_READ_DATA;";
            break;
        case MT_WRITE_DATA:
            trace << "MT_WRITE_DATA;";
            break;
        case MT_PRELOAD_DATA:
            trace << "MT_PRELOAD_DATA;";
            break;
        case MT_STATE:
            trace << "MT_STATE;";
            break;
        default:
            trace << static_cast<U32>(memoryCommand) << ";";
    } 
    trace << std::hex << address << std::dec << ";" << size << "\n";
}

