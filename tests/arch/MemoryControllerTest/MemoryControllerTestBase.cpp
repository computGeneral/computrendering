/**************************************************************************
 *
 */

#include "MemoryControllerTestBase.h"


using namespace arch;

MemoryControllerTestBase::MemoryControllerTestBase( const cgsArchConfig& arch,
                                                    const char** tuPrefixes,
                                                    const char** suPrefixes,
                                                    const char** slPrefixes,
                                                    const char* name,
                                                    const MemoryControllerProxy& mcProxy,
                                                    cmoMduBase* parentBox ) : cmoMduBase(name, parentBox), ArchConf(arch), currentCycle(0), mcProxy(mcProxy)
{

    clientEmulators.push_back(vector<ClientEmulator*>());
    clientEmulators.back().push_back( new ClientEmulator(
                                              this, COMMANDPROCESSOR, 0, 
                                              newInputSignal("CommProcMemoryRead", 2, 1, 0), 
                                              newOutputSignal("CommProcMemoryWrite", 1, 1, 0) 
                                          ) 
                                     );

    clientEmulators.push_back(vector<ClientEmulator*>());
    clientEmulators.back().push_back( new ClientEmulator(
                                              this, STREAMERFETCH, 0, 
                                              newInputSignal("StreamerFetchMemoryData", 2, 1, 0), 
                                              newOutputSignal("StreamerFetchMemoryRequest", 1, 1, 0) 
                                          ) 
                                     );

    clientEmulators.push_back(vector<ClientEmulator*>());
    for ( U32 i = 0; i < arch.str.streamerLoaderUnits; ++i )
    {
        clientEmulators.back().push_back( new ClientEmulator(
                                                  this, STREAMERLOADER, i, 
                                                  newInputSignal("StreamerLoaderMemoryData", 2, 1, slPrefixes[i]), 
                                                  newOutputSignal("StreamerLoaderMemoryRequest", 1, 1, slPrefixes[i]) 
                                              ) 
                                         );
    }
    
    clientEmulators.push_back(vector<ClientEmulator*>());
    for (U32 i = 0; i < arch.gpu.numStampUnits; i++ )
    {
        clientEmulators.back().push_back( new ClientEmulator(
                                                  this, ZSTENCILTEST, i, 
                                                  newInputSignal("ZStencilTestMemoryData", 2, 1, suPrefixes[i]), 
                                                  newOutputSignal("ZStencilTestMemoryRequest", 1, 1, suPrefixes[i])
                                              )
                                         );
    }

    clientEmulators.push_back(vector<ClientEmulator*>());
    for (U32 i = 0; i < arch.gpu.numStampUnits; i++ )
    {
        clientEmulators.back().push_back( new ClientEmulator(
                                                  this, COLORWRITE, i, 
                                                  newInputSignal("ColorWriteMemoryData", 2, 1, suPrefixes[i]), 
                                                  newOutputSignal("ColorWriteMemoryRequest", 1, 1, suPrefixes[i])
                                              )
                                         );
    }
    
    clientEmulators.push_back(vector<ClientEmulator*>());
    clientEmulators.back().push_back( new ClientEmulator(
                                              this, DISPLAYCONTROLLER, 0, 
                                              newInputSignal("DisplayControllerMemoryData", 2, 1, 0), 
                                              newOutputSignal("DisplayControllerMemoryRequest", 1, 1, 0) 
                                          ) 
                                     );

    const U32 numTexUnits = arch.gpu.numFShaders * arch.ush.textureUnits;
    clientEmulators.push_back(vector<ClientEmulator*>());
    for ( U32 i = 0; i < numTexUnits;i++ )
    {
        clientEmulators.back().push_back( new ClientEmulator(
                                                  this, TEXTUREUNIT, i, 
                                                  newInputSignal("TextureMemoryData", 2, 1, tuPrefixes[i]), 
                                                  newOutputSignal("TextureMemoryRequest", 1, 1, tuPrefixes[i])
                                              )
                                         );
    }


    mcCommSignal = newOutputSignal("MemoryControllerCommand", 1, 1, 0);

    MCT_DEBUG( cout << "MCT_DEBUG: MemoryTestBase ctor() completed!" << endl; )

}

void MemoryControllerTestBase::sendMCCommand(MemoryControllerCommand* mccom)
{
    pendingMCCommands.push(mccom);
}


MemoryControllerTestBase::ClientEmulator::ClientEmulator(MemoryControllerTestBase* owner, 
                                                         GPUUnit unit, U32 subunit,
                                                         Signal* inputSignal, Signal* outputSignal) :
    owner(owner), unit(unit), subunit(subunit),
    inputSignal(inputSignal), outputSignal(outputSignal), memoryRead(false), memoryWrite(false), memoryCycles(0), nextID(0)
{
    MCT_DEBUG( cout << "MCT_DEBUG: Creating client behaviorModel UNIT=" << MemoryTransaction::getBusName(unit) << " SUBUNIT=" << subunit << endl; )
}

void MemoryControllerTestBase::ClientEmulator::enqueue(MemoryTransaction* mt)
{
    pendingMTs.push(mt);
}

void MemoryControllerTestBase::ClientEmulator::clock(U64 cycle)
{    
    MemoryTransaction* mt = 0;

    // read replies
    while ( inputSignal->read(cycle, (DynamicObject*&)mt) ) {
        // MCT_DEBUG( cout << "MT recived: " << mt->toString() << endl; )
        switch ( mt->getCommand() ) {
            case MT_STATE:
                memState = mt->getState();
                delete mt; // Get rid of state transactions
                break;
            case MT_READ_DATA:
                MCT_DEBUG( cout << "MCT_DEBUG (cycle " << cycle <<  "): MT recived: " << mt->toString() << endl; )
                GPU_ASSERT
                (
                    if ( memoryCycles > 0 ) {
                        CG_ASSERT("Memory bus still busy when receiving a new transaction");
                    }
                )
                memoryCycles = mt->getBusCycles();
                memoryRead = true;
                // send received transaction back to test
                owner->handler_receiveTransaction(cycle, mt);
                break;
            case MT_PRELOAD_DATA:
                cout << "-------------------\n" << mt->toString() << "\n";
                CG_ASSERT("Received an unexpected transaction from MC ( MT_PRELOAD_DATA )");
            case MT_READ_REQ:
                cout << "-------------------\n" << mt->toString() << "\n";
                CG_ASSERT("Received an unexpected transaction from MC ( MT_READ_REQ )");
            case MT_WRITE_DATA:
                cout << "-------------------\n" << mt->toString() << "\n";
                CG_ASSERT("Received an unexpected transaction from MC ( MT_WRITE_REQ )");
            default:
                CG_ASSERT("Received an unexpected transaction from MC ( UNKNOWN )");
        }
    }

    if ( memoryCycles > 0 ) {
        --memoryCycles;
        if ( memoryCycles == 0 ) {
            if ( memoryRead ) {
                memoryRead = false;
            }
            if ( memoryWrite ) {
                memoryWrite = false;
            }
        }
        // else ( bus busy )
    }

    if ( !pendingMTs.empty() && !memoryWrite && !memoryRead ) {
        MemoryTransaction* mt = pendingMTs.front();
        switch ( mt->getCommand() ) {
            case MT_WRITE_DATA:
                if ( (memState & MS_WRITE_ACCEPT) != 0 ) {
                    memoryWrite = true;
                    memoryCycles = mt->getBusCycles();
                    // MCT_DEBUG( cout << "Cycle: " << cycle << ". Sending MT: " << mt->toString() << endl; )
                    owner->handler_sendTransaction(cycle, mt);
                    outputSignal->write(cycle, mt);
                    pendingMTs.pop();
                }
                break;
            case MT_READ_REQ:
                if ( (memState & MS_READ_ACCEPT) != 0 ) {
                    // MCT_DEBUG( cout << "Cycle: " << cycle << ". Sending MT: " << mt->toString() << endl; )
                    owner->handler_sendTransaction(cycle, mt);
                    outputSignal->write(cycle, mt);
                    pendingMTs.pop();
                }
                break;
            default:
                CG_ASSERT("Unsupported memory transaction picked from the client's behaviorModel queue");
        }
    }
}


void MemoryControllerTestBase::clock(U64 cycle)
{
    currentCycle = cycle;

    // send the next MC command (if any)
    if ( !pendingMCCommands.empty() ) {
        MemoryControllerCommand* mccom = pendingMCCommands.front();
        pendingMCCommands.pop();
        mcCommSignal->write(cycle, mccom);
    }

    vector<vector<ClientEmulator*> >::iterator it = clientEmulators.begin();
    for ( ; it != clientEmulators.end(); ++it ) {
        vector<ClientEmulator*>::iterator it2 = it->begin();
        for ( ;it2 != it->end(); ++it2 )
            (*it2)->clock(cycle);
    }

    clock_test(cycle);
}

void MemoryControllerTestBase::printTransaction(const MemoryTransaction* mt)
{
    mt->dump(false);
}

void MemoryControllerTestBase::handler_receiveTransaction(U64 cycle, MemoryTransaction* mt)
{
    cout << "C: " << cycle << ". Receiving: " << mt->toString(true) << endl;
    delete[] mt->getData();
    delete mt;
}

void MemoryControllerTestBase::handler_sendTransaction(U64 cycle, MemoryTransaction* mt)
{
    cout << "C: " << cycle << ". Sending: " << mt->toString(true) << endl;

}

void MemoryControllerTestBase::sendRequest( GPUUnit unit, 
                                            U32 subunit, 
                                            RequestType type, 
                                            U32 requestSize,
                                            U32 address,
                                            U08* data )
{
    /*
    cout << "DEBUG: sendRequest -> GPUUnit= " << unit << "[" << subunit << "]"
        << " " << (type == READ_REQUEST ? "READ" : "WRITE") 
        << " sz=" << requestSize << " Address (dec)=" << address << " DataPtr=" 
        << reinterpret_cast<U64>(data) << "\n";
    if ( data ) {
        cout << "Data: " << (const char*)data << endl;
    }
    */

    if ( static_cast<U32>(unit) >= clientEmulators.size() ) { 
        CG_ASSERT("Unit token unknown");
    }
    if ( subunit >= clientEmulators[unit].size() ) {
        CG_ASSERT("Subunit ID too high");
    }

    if ( requestSize > ClientEmulator::MAX_REQUEST_SIZE ) {
        stringstream ss;
        ss << "Request size is larger than MAX_REQUEST_SIZE which is defined as " << ClientEmulator::MAX_REQUEST_SIZE << " bytes";
        CG_ASSERT(ss.str().c_str());
    }

    ClientEmulator* ce = clientEmulators[unit][subunit];

    memset(ce->requestData, 0, ClientEmulator::MAX_REQUEST_SIZE);
    if ( data != 0 )
        memcpy(ce->requestData, data, requestSize);

    MemoryTransaction* mt = 0;
    if ( type == WRITE_REQUEST ) {
        mt = new MemoryTransaction(
                    MT_WRITE_DATA,
                    address, 
                    requestSize,
                    (U08*)ce->requestData, 
                    unit,
                    subunit,
                    ce->nextID++  );
    }
    else if ( type == READ_REQUEST )  {
        mt = new MemoryTransaction(
                    MT_READ_REQ,
                    address, 
                    requestSize,
                    (U08*)new char[requestSize],
                    unit,
                    subunit,
                    ce->nextID++  );        
    }
    else {
        CG_ASSERT("Unsupported memory transaction type");
    }

    clientEmulators[unit][subunit]->enqueue(mt);
}


const MemoryControllerProxy& MemoryControllerTestBase::getMCProxy() const
{
    return mcProxy;
}