/**************************************************************************
 *
 */

#include "cmChannelScheduler.h"
#include "cmMemoryRequest.h"
#include "cmGPUMemorySpecs.h"
#include <cmath>

using namespace std;
using namespace cg1gpu;
using namespace cg1gpu::memorycontroller;

#ifdef GPU_DEBUG
    #undef GPU_DEBUG
#endif
//#define GPU_DEBUG(expr) { expr }
#define GPU_DEBUG(expr) {  }

SchedulerState::SchedulerState(SchedulerState::State state) : allBanksShareState(true), sharedSchedState(state), bankStates(0)
{}

SchedulerState::SchedulerState(std::vector<State>* bankStates) : allBanksShareState(false), bankStates(bankStates)
{
    CG_ASSERT_COND(!( bankStates == 0 ), "Bank states vector == NULL!");}

SchedulerState::State SchedulerState::state(U32 bank) const
{
    if ( allBanksShareState )
        return sharedSchedState;

    GPU_ASSERT(
        if ( bank >= bankStates->size() ) {
            stringstream ss;
            ss << "BankID out of bound. BankID=" << bank << "  bankState.size()=" << bankStates->size();
            CG_ASSERT(ss.str().c_str());
        }
    )
    return (*bankStates)[bank];
}

SchedulerState::~SchedulerState()
{
    if ( bankStates ) // delete NULL is not a problem, but... Just in case
        delete bankStates;
}

ChannelScheduler::ChannelScheduler(const char* name,  const char* prefix, cmoMduBase* parent, const CommonConfig& config) :
cmoMduBase(name, parent),

        _debugString(config.debugString),
        pagePolicy(config.pagePolicy),
        //currentState(SchedulerState::AcceptNone), 
        currentState(0),
        modState(config.moduleBanks, config.burstLength, config.burstBytesPerCycle, config.memSpecs), 
        BurstLength(config.burstLength), Banks(config.moduleBanks),
        lastCmdWasRW(config.moduleBanks,false),
        setStateCalled(false),
        moduleRequestLastCycle(0),
        prefixStr(prefix),
        
        readBytesStat(getSM().getNumericStatistic("ReadBytes", U32(0),
                                          "ChannelScheduler", prefix)),
        writeBytesStat(getSM().getNumericStatistic("WriteBytes", U32(0),
                                           "ChannelScheduler", prefix)),
        readCmdStat(getSM().getNumericStatistic("DDRReadCommands", U32(0),
                                        "ChannelScheduler", prefix)),
        writeCmdStat(getSM().getNumericStatistic("DDRWriteCommands", U32(0),
                                         "ChannelScheduler", prefix)),
        activeCmdStat(getSM().getNumericStatistic("DDRActiveCommands", U32(0),
                                          "ChannelScheduler", prefix)),
        prechargeCmdStat(getSM().getNumericStatistic("DDRPrechargeCommands", U32(0),
                                              "ChannelScheduler", prefix)),
        totalRowHitStat(getSM().getNumericStatistic("RowHit", U32(0),
                                       "ChannelScheduler", prefix)),
        readRowHitStat(getSM().getNumericStatistic("ReadRowHit", U32(0),
                                       "ChannelScheduler", prefix)),
        writeRowHitStat(getSM().getNumericStatistic("WriteRowHit", U32(0),
                                       "ChannelScheduler", prefix)),
        totalRowMissStat(getSM().getNumericStatistic("RowMiss", U32(0),
                                        "ChannelScheduler", prefix)),
        readRowMissStat(getSM().getNumericStatistic("ReadRowMiss", U32(0),
                                        "ChannelScheduler", prefix)),
        writeRowMissStat(getSM().getNumericStatistic("WriteRowMiss", U32(0),
                                        "ChannelScheduler", prefix))

{
    memset(perClientRowHit, 0, sizeof(perClientRowHit));

    for ( U32 i = 0; i < LASTGPUBUS; ++i ) {

        if ( i == MEMORYMODULE || i == SYSTEM )
            // no statistics for MEMORYMODULE & SYSTEM definitions
            continue;

        string uName = MemoryTransaction::getBusName(GPUUnit(i));

        perClientRowHit[i].totalHits = 
            & getSM().getNumericStatistic((string("ClientRowHit_") + uName).c_str(),
                                           U32(0), "ChannelScheduler", prefix);
        perClientRowHit[i].readHits = 
            & getSM().getNumericStatistic((string("ClientReadRowHit_") + uName).c_str(), 
                                           U32(0), "ChannelScheduler", prefix);
        perClientRowHit[i].writeHits = 
            & getSM().getNumericStatistic((string("ClientWriteRowHit_") + uName).c_str(),
                                           U32(0), "ChannelScheduler", prefix);
        perClientRowHit[i].totalMisses =
            & getSM().getNumericStatistic((string("ClientRowMiss_") + uName).c_str(), 
                                           U32(0), "ChannelScheduler", prefix);
        perClientRowHit[i].readMisses =
            & getSM().getNumericStatistic((string("ClientReadRowMiss_") + uName).c_str(),
                                           U32(0), "ChannelScheduler", prefix);
        perClientRowHit[i].writeMisses =
            & getSM().getNumericStatistic((string("ClientWriteRowMiss_") + uName).c_str(),
                                          U32(0), "ChannelScheduler", prefix);
    }

    //if ( requestBandwidth == 0 )
    //    CG_ASSERT("Request bandwidth must be at least 1");

    channelRequest = newInputSignal("ChannelRequest", 1/*requestBandwidth*/, 1, prefix);
    moduleReply = newInputSignal("DDRModuleReply", 1, 1, prefix);

    channelReply = newOutputSignal("ChannelReply", 1, 1, prefix);
    schedulerState = newOutputSignal("SchedulerState", 1, 1, prefix);
    moduleRequest = newOutputSignal("DDRModuleRequest", 1, 1, prefix);

    // set initial scheduler state
    DynamicObject* defValue[1];
    defValue[0] = new SchedulerState(SchedulerState::AcceptNone);
    schedulerState->setData(defValue);
}


/*
void ChannelScheduler::setState(SchedulerState::State state)
{
    setStateCalled = true;
    switch ( state )
    {
        case SchedulerState::AcceptBoth:
        case SchedulerState::AcceptRead:
        case SchedulerState::AcceptWrite:
        case SchedulerState::AcceptNone:
            currentState = state;
            break;
        default:
            CG_ASSERT("Unknown state");
    }
}
*/

void ChannelScheduler::setState(SchedulerState* state)
{
    setStateCalled = true;
    CG_ASSERT_COND(!( state == 0 ), "setState parameter cannot be a NULL pointer");    currentState = state;
}




void ChannelScheduler::clock(U64 cycle)
{
    GPU_DEBUG( printf("%s => Clock "U64FMT".\n", getName(), cycle); )

    modState.updateState(cycle); // Update  module state

    ChannelTransaction* request;
    if ( channelRequest->read(cycle, (DynamicObject*&) request) )
    {
        GPU_DEBUG
        (
            cout << getName() << " => Channel Transaction received. "
                 "Type: " << ( request->isRead()?"READ ":"WRITE ")
                 << "Bank: " << request->getBank() << " Row: " << request->getRow()
                 << " Col: " << request->getCol() << " Size: " << request->bytes() 
                 << "\n";
        )
        // update channel transaction timestamp
        // request->setArrivalTimestamp(cycle);
        // inform to the dereived scheduler class that a request has arrived
        receiveRequest(cycle, request);
    }

    DDRBurst* readData;
    if ( moduleReply->read(cycle, (DynamicObject*&) readData) )
    {
        GPU_DEBUG(
            cout << getName() << " => Receiving DDRBurst from GDDR3 module. " 
                << (4 * readData->getSize()) << " bytes.\n";
        )
        // update channel read bytes statistic
        readBytesStat.inc(readData->getSize()*4);
        receiveData(cycle, readData);
    }
    
    schedulerClock(cycle);

    /* 
     * At this point setState() has to have been called by the derived 
     * class (usually inside schedulerClock() 
     */

    if ( setStateCalled )
        setStateCalled = false;
    else
        CG_ASSERT("setState must be called every simulation clock");

    //SchedulerState* schedState = 0;
    switch ( currentState->state(0) )
    {
        case SchedulerState::AcceptBoth:
            //schedState = new SchedulerState(SchedulerState::AcceptBoth);
            //schedState->setColor(SchedulerState::AcceptBoth);
            currentState->setColor(SchedulerState::AcceptBoth);
            break;
        case SchedulerState::AcceptRead:
            //schedState = new SchedulerState(SchedulerState::AcceptRead);
            //schedState->setColor(SchedulerState::AcceptRead);
            currentState->setColor(SchedulerState::AcceptRead);
            break;
        case SchedulerState::AcceptWrite:
            //schedState = new SchedulerState(SchedulerState::AcceptWrite);
            //schedState->setColor(SchedulerState::AcceptWrite);
            currentState->setColor(SchedulerState::AcceptWrite);
            break;
        case SchedulerState::AcceptNone:
            //schedState = new SchedulerState(SchedulerState::AcceptNone);
            //schedState->setColor(SchedulerState::AcceptNone);
            currentState->setColor(SchedulerState::AcceptWrite);
            break;
        default:
            CG_ASSERT("Unknown state");
    }
    schedulerState->write(cycle, currentState); // send current scheduler state
    currentState = 0; // reset current state
}

void ChannelScheduler::sendReply(U64 cycle, ChannelTransaction* reply)
{
    GPU_DEBUG( cout << getName() << " => Sending Channel Transaction completion reply.\n"; )
    strcat((char *)reply->getInfo(), " [COMPLETED]");
    channelReply->write(cycle, reply);
}

bool ChannelScheduler::sendDDRCommand(U64 cycle, DDRCommand* ddrCmd, const ChannelTransaction* ct)
{
    // cout << "Sent DDRCommand: " << ddrCmd->toString() << "  Trans: " << ct->toString() << endl;

    bool issueCmd;

    U32 bank = ddrCmd->getBank();


    switch ( ddrCmd->which() ) {
        case DDRCommand::Active:
            issueCmd = modState.canBeIssued(bank, DDRModuleState::C_Active);
            if ( issueCmd ) {
                activeCmdStat.inc();
                lastCmdWasRW[bank] = false;
                modState.postActive(bank, ddrCmd->getRow());
                GPU_DEBUG( 
                    cout << getName() << " => Sending ACTIVE bank=" 
                         << ddrCmd->getBank() << " row=" << ddrCmd->getRow() << ".\n";
                )
            }
            break;
        case DDRCommand::Read:
            if ( ct == 0 ) {
                CG_ASSERT("Channel transaction provided for "
                                                            "a DDR Read command cannot be NULL");
            }
            issueCmd = modState.canBeIssued(bank, DDRModuleState::C_Read);
            if ( issueCmd ) {
                U32 id = ct->getRequestSource();
                if ( id == MEMORYMODULE || id == SYSTEM || id > LASTGPUBUS ) {
                    CG_ASSERT("Invalid unit identifier");
                } 
                readCmdStat.inc();
                if ( lastCmdWasRW[bank] ) {
                    totalRowHitStat.inc();
                    perClientRowHit[id].totalHits->inc();
                    perClientRowHit[id].readHits->inc();
                }
                else {
                    totalRowMissStat.inc();
                    perClientRowHit[id].totalMisses->inc();
                    perClientRowHit[id].readMisses->inc();
                }
                lastCmdWasRW[bank] = true;
                modState.postRead(bank);
                GPU_DEBUG( 
                    cout << getName() << " => Sending READ bank=" 
                    << ddrCmd->getBank() << " col=" << ddrCmd->getColumn() << ".\n";
                )
            }
            break;
        case DDRCommand::Write:
            if ( ct == 0 ) {
                CG_ASSERT("Channel transaction provided for "
                                                            "a DDR Write command cannot be NULL");
            }
            issueCmd = modState.canBeIssued(bank, DDRModuleState::C_Write);
            if ( issueCmd ) {
                U32 id = ct->getRequestSource();
                if ( id == MEMORYMODULE || id == SYSTEM || id > LASTGPUBUS ) {
                    CG_ASSERT("Invalid unit identifier");
                }
                writeCmdStat.inc();
                if ( lastCmdWasRW[bank] ) {
                    totalRowHitStat.inc();
                    perClientRowHit[id].totalHits->inc();
                    perClientRowHit[id].writeHits->inc();
                }
                else {
                    totalRowMissStat.inc();
                    perClientRowHit[id].totalMisses->inc();
                    perClientRowHit[id].writeMisses->inc();
                }
                lastCmdWasRW[bank] = true;
                writeBytesStat.inc(ddrCmd->getData()->getSize() * 4);
                modState.postWrite(bank);
                GPU_DEBUG( 
                    cout << getName() << " => Sending WRITE bank=" 
                    << ddrCmd->getBank() << " col=" << ddrCmd->getColumn() << ".\n";
                )
            }
            break;
        case DDRCommand::Precharge:
            issueCmd = modState.canBeIssued(bank, DDRModuleState::C_Precharge);
            if ( issueCmd ) {
                prechargeCmdStat.inc();
                lastCmdWasRW[bank] = false;
                modState.postPrecharge(bank);
                GPU_DEBUG( 
                    cout << getName() << " => Sending PRECHARGE bank=";
                    if ( ddrCmd->getBank() == DDRCommand::AllBanks )
                        cout << "ALL.\n";
                    else
                        cout << ddrCmd->getBank() << ".\n";
                )
            }
            break;
            case DDRCommand::Dummy:
                // works if no command is sent at cycle 0 (expected case)
                issueCmd = cycle > moduleRequestLastCycle;
                break;
        default:
            issueCmd = false;
            CG_ASSERT("Unexpected DDR command");
    }

    if ( issueCmd ) {


        // cout << "Cycle=" << cycle << ".Prefix=" << prefixStr << ". ChannelScheduler::sendDDRCommand() -> " << ddrCmd->toString() << endl;
        if ( ct ) {
            ddrCmd->copyParentCookies(*ct);
            ddrCmd->addCookie();            
        }
        sprintf((char*)ddrCmd->getInfo(), ddrCmd->toString().c_str());
        moduleRequest->write(cycle, ddrCmd);
        moduleRequestLastCycle = cycle;
    }

    return issueCmd;
}
DDRCommand* ChannelScheduler::createActive(U32 bank, U32 row)
{
    return DDRCommand::createActive(bank, row);
}

DDRCommand* ChannelScheduler::createPrecharge(U32 bank)
{
    return DDRCommand::createPrecharge(bank);
}

list<DDRCommand*> ChannelScheduler::transactionToCommands(const ChannelTransaction* channelTrans)
{
    list<DDRCommand*> translation;
    U32 bank = channelTrans->getBank();
    U32 col = channelTrans->getCol();

    // Compute BurstByte constant
    const U32 BurstBytes = 4 * BurstLength;
    
    // Compute the number of bursts required to satisfy the request
    U32 nBurst = channelTrans->bytes() / BurstBytes;
    bool lastMasked = ((channelTrans->bytes() % BurstBytes) != 0);
    if ( lastMasked )
        nBurst++; // Add a burst that will be partially needed (wastes bandwidth)

    if ( channelTrans->isRead() )
    {
        for ( U32 i = 0; i < nBurst; i++ )
        {
            translation.push_back( DDRCommand::createRead(bank, col, false) );
            col +=  BurstLength;
        }
    }
    else
    {
        const U08* data;
        for ( U32 i = 0; i < nBurst; i++ )
        {
            data = channelTrans->getData(i * BurstBytes);
            DDRBurst* writeBurst = new DDRBurst(BurstLength);

            if ( i == nBurst - 1 && lastMasked ) // The last one is a partial write
            {
                writeBurst->setData(data, channelTrans->bytes() % BurstBytes );
                if ( channelTrans->isMasked() )
                    writeBurst->setMask(channelTrans->getMask(i*BurstLength),
                                        (channelTrans->bytes() % BurstBytes)/4);
            }
            else
            {
                writeBurst->setData(data, BurstBytes);
                if ( channelTrans->isMasked() )
                    writeBurst->setMask(channelTrans->getMask(i*BurstLength));
            }
            translation.push_back( DDRCommand::createWrite(bank, col, writeBurst, false) );
            col += BurstLength;
        }
    }
    return translation;
}

const DDRModuleState& ChannelScheduler::moduleState() const
{
    return modState;
}

const string& ChannelScheduler::getDebugString() const
{
    return _debugString;
}

