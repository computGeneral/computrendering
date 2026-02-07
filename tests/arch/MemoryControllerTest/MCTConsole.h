/**************************************************************************
 *
 */

#ifndef MCTCONSOLE_H
    #define MCTCONSOLE_H

#include "MemoryControllerTestBase.h"
#include "MemoryRequestSplitter.h"
#include <vector>
#include <list>

namespace cg1gpu
{


class MCTConsole : public MemoryControllerTestBase
{
private:

    struct Stats
    {
        U32 gpuReadRequestsSent;
        U32 gpuReadRequestsReceived;
        U32 systemReadRequestsSent;
        U32 systemReadRequestsReceived;
        U32 gpuWriteRequestsSent;
        U32 systemWriteRequestsSent;
    };

    Stats stats;

    static const U32 MC_INVALID_INTEGER = static_cast<U32>(-1);

    U64 currentCycle;
    U64 startCycle;
    bool quitConsole;

    std::list<std::string> pendingCommands;

    bool processSendCommand(const std::vector<std::string>& params);
    bool processRunCommand(const std::vector<std::string>& params);
    bool processQuitCommand(const std::vector<std::string>& params);
    bool processHelpCommand(const std::vector<std::string>& params);
    bool processScriptCommand(const std::vector<std::string>& params);
    bool processSaveMemoryCommand(const std::vector<std::string>& params);
    bool processLoadMemoryCommand(const std::vector<std::string>& params);
    bool processArchCommand(const std::vector<std::string>& params);
    bool processStatsCommand(const std::vector<std::string>& params);
    bool processMCDebugCommand(const std::vector<std::string>& params);
    bool processPrintCommand(const std::vector<std::string>& params);
    bool processReadCommand(const std::vector<std::string>& params);
    bool processWriteCommand(const std::vector<std::string>& params);


    static std::string parseUnitName(string unitName, U32& unit, U32& subunit);
    U32 parseAddress(string addressStr);

    std::string getPrompt() const;

    // virtual void handler_receiveTransaction(U64 cycle, MemoryTransaction* mt);
    // virtual void handler_sendTransaction(U64 cycle, MemoryTransaction* mt);

    // std::vector<memorycontroller::MemoryRequestSplitter*> splitterArray;
    memorycontroller::MemoryRequestSplitter* splitter;

    // Last data received from each client
    std::vector<std::string> lastData[LASTGPUBUS];
    

protected:

    void clock_test(U64 cycle);

    void handler_receiveTransaction(U64 cycle, MemoryTransaction* mt);
    void handler_sendTransaction(U64 cycle, MemoryTransaction* mt);


public:

    std::string parseParams(const std::vector<std::string>& params);

    bool finished();

    MCTConsole( const cgsArchConfig& arch,
              const char** tuPrefixes,
              const char** suPrefixes,
              const char** slPrefixes,
              const char* name, 
              const MemoryControllerProxy& mcProxy,
              cmoMduBase* parentBox);

};

} // namespace cg1gpu

#endif // MCTCONSOLE_H
