/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERTESTBASE_H
    #define MEMORYCONTROLLERTESTBASE_H

#include "MemoryControllerTest.h"

#include "MduBase.h"
#include <vector>
#include <queue>
#include "MemoryTransaction.h"
#include "MemoryControllerCommand.h"
#include "ConfigLoader.h"
#include "MemoryControllerProxy.h"

namespace cg1gpu {

// From ConfigLoader.h (included in implementation .cpp file)
//struct cgsArchConfig;

class MemoryControllerTestBase : public cmoMduBase
{
private:

    U64 currentCycle;

    /**
     * Emulates the memory client's interface
     */
    class ClientEmulator
    {
    private:

        U64 memoryCycles;
        
        bool memoryWrite;
        bool memoryRead;
        
        std::queue<MemoryTransaction*> pendingMTs;

        GPUUnit unit;
        U32 subunit;

        Signal* outputSignal;
        Signal* inputSignal;

        MemoryTransaction* currentMT;
        MemState memState;
        MemoryControllerTestBase* owner;

    public:

        // Temporary here
        static const U32 MAX_REQUEST_SIZE = 256;
        char requestData[MAX_REQUEST_SIZE];
        U32 nextID;

        ClientEmulator( MemoryControllerTestBase* owner, 
                        GPUUnit unit, U32 subunit,
                        Signal* inputSignal, Signal* outputSignal);

        void enqueue(MemoryTransaction* mt);

        void clock(U64 cycle);

    };

    std::vector<std::vector<ClientEmulator*> > clientEmulators;

    Signal* mcCommSignal; // Command signal from the command processor (offered to tests in case they want to send a MC command)

    std::queue<MemoryControllerCommand*> pendingMCCommands;

    const cgsArchConfig& ArchConf;

    const MemoryControllerProxy& mcProxy;


protected:

    const cgsArchConfig& ArchConfig() const { return ArchConf; }
    
    void sendMCCommand(MemoryControllerCommand* mccom);

    
    // By default deletes the transaction
    virtual void handler_receiveTransaction(U64 cycle, MemoryTransaction* mt);

    virtual void handler_sendTransaction(U64 cycle, MemoryTransaction* mt);

    enum RequestType
    {
        READ_REQUEST,
        WRITE_REQUEST
    };

    void sendRequest(GPUUnit unit, U32 subunit, RequestType type, U32 requestSize, U32 address, U08* userData = 0);

    // void sendRequest(MemoryTransaction* mt);

    static void printTransaction(const MemoryTransaction* mt);

    /**
     * Redefine this function to implement your memory test
     */
    virtual void clock_test(U64 cycle) = 0;

    const MemoryControllerProxy& getMCProxy() const;

public:

    virtual std::string parseParams(const std::vector<std::string>& params)
    {
        return std::string(); // OK
    }

    MemoryControllerTestBase( const cgsArchConfig& arch,
                              const char** tuPrefixes,
                              const char** suPrefixes,
                              const char** slPrefixes,
                              const char* name,
                              const MemoryControllerProxy& memProxy,
                              cmoMduBase* parentBox);

    void clock(U64 cycle);

    virtual bool finished() = 0;

};

} // namespace cg1gpu

#endif // MEMORYCONTROLLERTESTBASE_H
