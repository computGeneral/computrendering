/**************************************************************************
 *
 */

#include "cmFifoScheduler.h"

// temporary include
#include "cmMemoryRequest.h"

#include <sstream>

using namespace std;
using namespace cg1gpu::memorycontroller;

void FifoScheduler::processStateQueueSignals(U64 cycle)
{
    DynamicObject* dynObj;

    // read and delete previous inner dynamic objects
    while ( queueSignal->read(cycle, dynObj) )
        delete dynObj;

    U32 color = 0;
    
    ChannelTransaction* currentTrans = getCurrentTransaction();

    // Use signals for monitoring queue state
    if ( currentTrans != 0 )
    {
        // Queue colors based on the queue size
        // 0 -> 100 % queue full
        // 1 -> up to 25 %
        // 2 -> up to 50 %
        // 3 -> up to 75 %
        // 4 -> up to 100 % (full not included)
        if ( queueTrack.size() != maxSizeQ ) 
        {
            F32 col = (1.0f + (F32)queueTrack.size()) / ((F32)maxSizeQ + 1.0f);
            U32 col2 = (U32)(col * 100);
            if ( col2 <= 25 )
                color = 1;
            else if ( col2 <= 50 )
                color = 2;
            else if ( col2 <= 75 )
                color = 3;
            else // (col2 < 100)
                color = 4;
        }
        // else color = 0

        dynObj = new DynamicObject;
        dynObj->setColor(color); // selected transaction (current)
        dynObj->copyParentCookies(*currentTrans);
        stringstream ss;
        ss << currentTrans->toString() << " (queue size = " << queueTrack.size() + 1 << ")";
        strcpy((char *)dynObj->getInfo(), ss.str().c_str());
        queueSignal->write(cycle, dynObj);
    }

    // code not included within the previous IF to find possible programming errors
    for ( list<ChannelTransaction*>::iterator it = queueTrack.begin();
          it != queueTrack.end(); it++ )
    {
        CG_ASSERT_COND(!( currentTrans == 0 ), "Queue inconsistency");        dynObj = new DynamicObject;
        dynObj->setColor(color); // ready transaction in the queue not yet selected
        dynObj->copyParentCookies(**it);
        strcpy((char*)dynObj->getInfo(), (*it)->toString().c_str());
        queueSignal->write(cycle, dynObj);
    }
}


FifoScheduler::FifoScheduler(const char* name, const char* prefix, cmoMduBase* parent, const CommonConfig& config) : // , U32 maxTransactions) :
FifoSchedulerBase(name, prefix, parent, config),
maxSizeQ(config.maxChannelTransactions),
useInnerSignals(config.createInnerSignals),
pendingBankAccesses(config.moduleBanks, 0),
closePageActivationsCount(getSM().getNumericStatistic("ClosePageActivationCount", U32(0), "FifoSchedulerBase", prefix))
{
    if ( config.createInnerSignals )
    {
        // "currentTrans + maxTransactions" signals
        queueSignal = newInputSignal("FifoEntry", maxSizeQ + 1, 1, prefix);
        queueSignal = newOutputSignal("FifoEntry", maxSizeQ + 1, 1, prefix);
    }
}

void FifoScheduler::handler_ddrCommandNotSent(const DDRCommand* notSentCommand, U64 cycle)
{
    // Apply page policy
    if ( getPagePolicy() == ClosePage ) {
        const U32 Banks = moduleState().banks();
        U32 bank = ( notSentCommand ? notSentCommand->getBank() : Banks );
        for ( U32 i = 0; i < Banks; ++i ) {
            if ( bank == i )
                continue; // skip -> bank used by current transaction
            if ( pendingBankAccesses[i] == 0 && moduleState().getActiveRow(i) != DDRModuleState::NoActiveRow ) 
            {
                // close page as soon as no requests pending to this row/page are available
                DDRCommand* preCommand = createPrecharge(i);
                if ( notSentCommand )
                    preCommand->setProtocolConstraint(notSentCommand->getProtocolConstraint());
                if ( !sendDDRCommand(cycle, preCommand, 0) ) 
                    delete preCommand; // cannot be issued
                else {
                    closePageActivationsCount.inc();
                    break;
                }
            }
        }
    }
    // nothing to do with OpenPage
}


ChannelTransaction* FifoScheduler::selectNextTransaction(U64 cycle)
{
	if ( transQ.empty() )
        return 0;

    ChannelTransaction* ct = transQ.front();
    transQ.pop();

    // start STV feedback code
    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        queueTrack.pop_front();
    // end STV feedback code

    GPU_ASSERT(
        if ( pendingBankAccesses[ct->getBank()] == 0 ) {
            CG_ASSERT("Internal error: pendingBankAccesses counter failed!!!");
        }
    )
    --pendingBankAccesses[ct->getBank()];        

    return ct;
}

void FifoScheduler::receiveRequest(U64 cycle, ChannelTransaction* request)
{
    if ( transQ.size() == maxSizeQ )
        CG_ASSERT("Fifo scheduler queue is full");

    transQ.push(request);

    const U32 bank = request->getBank();
    if ( bank >= moduleState().banks() ) {
        CG_ASSERT("Bank ID too high");
    }
    pendingBankAccesses[bank]++;
    
    // start STV feedback code
    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        queueTrack.push_back(request);
    // end STV feedback code
}

void FifoScheduler::handler_endOfClock(U64 cycle)
{
    if ( transQ.size() < maxSizeQ - 1 /*getRequestBandwidth()*/ )
        setState(new SchedulerState(SchedulerState::AcceptBoth));
    else
        setState(new SchedulerState(SchedulerState::AcceptNone));

    if ( useInnerSignals && isSignalTracingRequired(cycle) )
        processStateQueueSignals(cycle);
}

