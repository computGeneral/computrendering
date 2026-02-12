/**************************************************************************
 *
 */

#include "cmBankQueueScheduler.h"
#include <iostream>
#include <bitset>

using std::cout;

using namespace arch::memorycontroller;

///////// Methods for TQueue /////////////

TQueue::TQueue() : qSize(0), qName(string("NO_NAME"))
{}

U32 TQueue::getConsecutiveAccesses(bool writes) const
{
    if ( q.empty() )
        return 0;

    U32 ccount = 0;
    Queue::const_iterator it = q.begin();
    
    U32 row = it->ct->getRow();
    bool isWrite = !it->ct->isRead();

    for ( ; it != q.end(); ++it ) {
        if ( row == it->ct->getRow() && isWrite == writes )
            ++ccount;
    }
    return ccount;
}

bool TQueue::empty() const
{
    return q.empty();
}

U32 TQueue::size() const
{
    return qSize;
}

U64 TQueue::getTimestamp() const
{
    if ( q.empty() )
    {
        stringstream ss;
        ss << "Queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }
    return q.front().timestamp;
}

void TQueue::enqueue(ChannelTransaction* ct, U64 timestamp)
{
    ++qSize;
    q.push_back(QueueEntry(ct, timestamp));
}

ChannelTransaction* TQueue::front() const
{
    if ( empty() )
    {
        stringstream ss;
        ss << "Queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }

    return q.front().ct;
}

void TQueue::pop()
{
    if ( empty() )
    {
        stringstream ss;
        ss << "Queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }
    --qSize;
    q.pop_front();
}

void TQueue::setName(const string& name)
{
    qName = name;
}

const string& TQueue::getName() const
{
    return qName;
}

//// Methods for BankQueueScheduler ////


BankQueueScheduler::BankQueueScheduler( 
                  const char* name,
                  const char* prefix,
                  cmoMduBase* parent,
                  const CommonConfig& config
                  // U32 maxTransactions,
                  // U32 maxConsecutiveReads,
                  // U32 maxConsecutiveWrites
                  ) :
FifoSchedulerBase(name, prefix, parent, config),
bankQueueSz((config.maxChannelTransactions / config.moduleBanks) + 1), // +1 => extra buffer slot to support backpressure
useInnerSignals(config.createInnerSignals),
bankQ(config.moduleBanks), lastSelected(config.moduleBanks-1),
queueBankInfos(config.moduleBanks),
aam(static_cast<AdvancedActiveMode>(config.activeManagerMode)),
disableActiveManager(config.disableActiveManager),
disablePrechargeManager(config.disablePrechargeManager),
managerSelectionAlgorithm(config.managerSelectionAlgorithm),
banksShareState(config.useClassicSchedulerStates),
closePageActivationsCount(getSM().getNumericStatistic("ClosePageActivationCount", U32(0), "FifoSchedulerBase", prefix))
{
    GPU_ASSERT(
        if ( config.maxChannelTransactions % config.moduleBanks != 0 ) {
            stringstream ss;
            ss << "MaxChannelTransactions (" << config.maxChannelTransactions << ") must be a multiple of the number of banks per channel ("
               << config.moduleBanks << ")";
            CG_ASSERT(ss.str().c_str());
        }
        if ( aam != AAM_Conservative && aam != AAM_Agressive )
            CG_ASSERT("Advance ACT mode error. Only AAM_Conservative(=0) and AAM_Agressive(=1) modes are supported");
        if ( managerSelectionAlgorithm > 1 )
            CG_ASSERT("ManagerSelectionAlgorithms supported are 0=active-then-precharge or 1=precharge-then-active");
    )

    som = SwitchOperationModeSelector::create(config);

    for ( U32 i = 0; i < config.moduleBanks; ++i ) {
        queueBankInfos[i].bankID = i;
        queueBankInfos[i].age = 0;
        queueBankInfos[i].consecHits = 0;
        queueBankInfos[i].queueSize = 0;
        queueBankPointers.push_back( & queueBankInfos[i] );
    }

    bankSelector = BankSelectionPolicyFactory::create(config.bankSelectionPolicy.c_str(), config.moduleBanks);
}



void BankQueueScheduler::receiveRequest(U64 cycle, ChannelTransaction* request)
{
    GPU_ASSERT
    (
        if ( request->getBank() >= bankQ.size() )
        {
            _coreDump();
            CG_ASSERT("Bank identifier too high");
        }
        if ( bankQ[request->getBank()].size() == bankQueueSz )
        {
            _coreDump();
            CG_ASSERT("Bank queue full");
        }
    )
    bankQ[request->getBank()].enqueue(request, cycle);
}

vector<U32> BankQueueScheduler::getBankPriority(BankInfoArray& bia) const
{
    /// Update bank info pointers ///
    for ( BankInfoArray::iterator it = bia.begin(); it != bia.end(); ++it ) {
        BankInfo* bi = *it;
        const TQueue& q = bankQ[bi->bankID];
        bi->age = q.empty() ? 0 : q.getTimestamp();
        bi->consecHits = q.empty() ? 0 : q.getConsecutiveAccesses(!q.front()->isRead());
        bi->queueSize = q.size();
    } 

    const U32 Banks = static_cast<U32>(bia.size());

    vector<U32> prio(Banks);

    /// Apply bank sorting policy ///
	bankSelector->sortBanks(bia);

    /// Copy bank identifiers based on the result sorted ///
	for ( U32 i = 0; i < Banks; ++i ) {
		prio[i] = bia[i]->bankID;
	}

	return prio;
}


BankQueueScheduler::Candidates BankQueueScheduler::_findCandidateTransactions(BankInfoArray& queueBankPointers)
{

    const U32 Banks = bankQ.size();

    Candidates candidates;
    candidates.read = Banks;
    candidates.write = Banks;
    candidates.readIsHit = false;
    candidates.writeIsHit = false;

    /// Get bank prioritization based on 'queue bank pointers' ///
    const vector<U32>& ind = getBankPriority(queueBankPointers);

    GPU_ASSERT(
        if ( ind.size() != bankQ.size() )
        {
            _coreDump();
            panic("BankQueueScheduler","_findCandidateTransactions", "Code inconsistency");
        }
    )

    bool readHit = false;
    bool writeHit = false;
    U32 posRead = Banks;
    U32 posWrite = Banks;

    for ( U32 bank = 0; bank < ind.size() && !(readHit && writeHit); ++bank )
    {
        const U32 i = ind[bank];
        if ( !bankQ[i].empty() )
        {
            ChannelTransaction* req = bankQ[i].front();
            if ( req->isRead() && !readHit ) { // If we have a read doing hit we don't want another candidate for reads
                readHit = (req->getRow() == moduleState().getActiveRow(i));
                if ( candidates.read == Banks || readHit ) {
                    candidates.read = i;
                    candidates.readIsHit = readHit;
                    posRead = bank;
                }
            }
            else if ( !req->isRead() && !writeHit ) { 
                writeHit = (req->getRow() == moduleState().getActiveRow(i));
                if ( candidates.write == Banks || writeHit )
                    candidates.write = i;
                    candidates.writeIsHit = writeHit;
                    posWrite = bank;
            }
        }
    }

    return candidates;
}


/*
ChannelTransaction* BankQueueScheduler::_selectNextTransaction_Counters(U64 cycle)
{

    Candidates candidates = _findCandidateTransactions(queueBankPointers);
    const U32 Banks = bankQ.size();

    bool readExists = candidates.read < Banks;
    bool writeExists = candidates.write < Banks;

    if ( !readExists && !writeExists )
        return 0;

    ChannelTransaction* selTrans;
    

    if ( smpCounters->reading() ) {
        if ( !readExists || (smpCounters->moreConsecutiveOpsAllowed() == 0 && writeExists) )
            smpCounters->switchMode();
    }
    else {
        if ( !writeExists || (smpCounters->moreConsecutiveOpsAllowed() == 0 && readExists) )
            smpCounters->switchMode();
    }

    if ( smpCounters->reading() ) {
        selTrans = bankQ[candidates.read].front();
        bankQ[candidates.read].pop();
    }
    else {
        selTrans = bankQ[candidates.write].front();
        bankQ[candidates.write].pop();
    }
    smpCounters->newOp();
    
    return selTrans;
}
*/

ChannelTransaction* BankQueueScheduler::selectNextTransaction(U64 cycle)
{
    Candidates candidates = _findCandidateTransactions(queueBankPointers);
    const U32 Banks = bankQ.size();

    bool readExists = candidates.read < Banks;
    bool writeExists = candidates.write < Banks;
    bool readIsHit = candidates.readIsHit;
    bool writeIsHit = candidates.writeIsHit;

    if ( !readExists && !writeExists )
        return 0;

    ChannelTransaction* selTrans;
    
    som->update(readExists, writeExists, readIsHit, writeIsHit);

    if ( som->reading() ) {
        GPU_ASSERT(
            if ( candidates.read >= Banks ) {
                stringstream ss;
                ss << "readCandidate: " << candidates.read << " writeCandidate: " << candidates.write << " readHit: " << 
                       readIsHit << "  writeHit: " << writeIsHit;
                cout << "findCandidate outputs: " << ss.str().c_str() << endl;
                CG_ASSERT("Inconsistency: Trying to select a read but not candidate was found in any bank");
            }
            if ( bankQ[candidates.read].empty() ) {
                stringstream ss;
                ss << "readCandidate: " << candidates.read << " writeCandidate: " << candidates.write << " readHit: " << 
                      readIsHit << "  writeHit: " << writeIsHit;
                cout << "findCandidate outputs: " << ss.str().c_str() << endl;
                CG_ASSERT("Inconsistency: No available read and cmSwitchOperationMode.has decided to set READ mode");
            }
        )
        selTrans = bankQ[candidates.read].front();
        bankQ[candidates.read].pop();
    }
    else {
        GPU_ASSERT( 
            if ( candidates.write >= Banks ) {
                stringstream ss;
                ss << "readCandidate: " << candidates.read << " writeCandidate: " << candidates.write << " readHit: " << 
                       readIsHit << "  writeHit: " << writeIsHit;
                cout << "findCandidate outputs: " << ss.str().c_str() << endl;
                CG_ASSERT("Inconsistency: Trying to select a write but not candidate was found in any bank");
            }

            if ( bankQ[candidates.write].empty() ) {
                stringstream ss;
                ss << "readCandidate: " << candidates.read << " writeCandidate: " << candidates.write << " readHit: " << 
                      readIsHit << "  writeHit: " << writeIsHit;
                cout << "findCandidate outputs: " << ss.str().c_str() << endl;
                CG_ASSERT("Inconsistency: No available write and cmSwitchOperationMode.has decided to set WRITE mode");
            }
        )
        selTrans = bankQ[candidates.write].front();
        bankQ[candidates.write].pop();
    }
    return selTrans;
}

void BankQueueScheduler::handler_ddrCommandNotSent(const DDRCommand* notSentCommand, U64 cycle)
{
    // First param (DDRCommand not sent) not required

    //////////////////////////////////////////
    // oportunity to precharge/active a row //
    //////////////////////////////////////////

    // Apply page policy
    const U32 Banks = moduleState().banks();

    // Apply page policy
    if ( getPagePolicy() == ClosePage ) {
        U32 bank = ( notSentCommand ? notSentCommand->getBank() : Banks );
        for ( U32 i = 0; i < Banks; ++i ) {
            if ( bank == i )
                continue; // skip bank used by current transaction
            if ( bankQ[i].empty() && moduleState().getActiveRow(i) != DDRModuleState::NoActiveRow )
            {
                // close page as soon as no requests pending to this row/page are available
                DDRCommand* preCommand = createPrecharge(i);
                if ( notSentCommand )
                    preCommand->setProtocolConstraint(notSentCommand->getProtocolConstraint()); // Copy protocol constraint
                if ( !sendDDRCommand(cycle, preCommand, 0) ) 
                    delete preCommand; // cannot be issued
                else {
                    closePageActivationsCount.inc();
                    return ; // no more commands can be issued this cycle
                }
            }
        }
    }


    if ( managerSelectionAlgorithm == 1 ) // First try precharge
    {
        if ( _tryAdvancedPrecharge(notSentCommand, cycle) )
            return ;
        _tryAdvancedActive(notSentCommand, cycle);
    }
    else if ( managerSelectionAlgorithm == 0 ) // First try active
    {
        if ( _tryAdvancedActive(notSentCommand, cycle) )
            return ;    
        _tryAdvancedPrecharge(notSentCommand, cycle);
    }
}

bool BankQueueScheduler::_tryAdvancedPrecharge(const DDRCommand* notSentCommand, 
                                                U64 cycle)
{
    if ( disablePrechargeManager )
        return false; // Ignore precharge manager

    const DDRModuleState& info = moduleState();
    DDRCommand* cmd = 0;
    const U32 Banks = static_cast<U32>(bankQ.size());

    /// Get bank prioritization based on 'queue bank pointers' ///
    const vector<U32>& ind = getBankPriority(queueBankPointers);

    for ( U32 bank = 0; bank < ind.size(); ++bank )
    {
        const U32 i = ind[bank];
        if ( bankQ[i].empty() || // this case is handled by page policy
             notSentCommand == 0 || notSentCommand->getBank() == i || info.getActiveRow(i) == DDRModuleState::NoActiveRow || 
             info.getState(i) != info.BS_Active )
            continue; // no precharge chance

        if ( info.getActiveRow(i) != bankQ[i].front()->getRow() ) {
            cmd = DDRCommand::createPrecharge(i);
            cmd->setProtocolConstraint(notSentCommand->getProtocolConstraint());
            if ( sendDDRCommand(cycle, cmd, bankQ[i].front()) )
                return true;
            else
                delete cmd; // destroy this command and try another
        }
    }

    return false;
}

bool BankQueueScheduler::_countHits(U32 bankIgnored, U32& readHits, U32& writeHits) const
{
    readHits = 0;
    writeHits = 0;

    bool availableReadsOrWrites = false;
    const DDRModuleState& info = moduleState();
    const U32 Banks = bankQ.size();

    for ( U32 i = 0; i < Banks; ++i ) {
        if ( i != bankIgnored ) {
            if ( !bankQ[i].empty() ) {
                const ChannelTransaction* ct = bankQ[i].front();
                if ( ct->isRead() ) {
                    availableReadsOrWrites = true;
                    if ( info.getActiveRow(i) == ct->getRow() )
                        readHits += bankQ[i].getConsecutiveAccesses(false);
                }
                else {
                    availableReadsOrWrites = true;
                    if ( info.getActiveRow(i) == ct->getRow() )
                        writeHits += bankQ[i].getConsecutiveAccesses(true);
                }
            }
         }
    }
    return availableReadsOrWrites;
}


/*
bool BankQueueScheduler::_tryAdvancedActive_Counters(const DDRCommand* notSentCommand, U64 cycle)
{   
    U32 readHits, writeHits;
    U32 ignoreBank = notSentCommand ? notSentCommand->getBank() : bankQ.size();
    bool transactionsExist = _countHits(ignoreBank, readHits, writeHits);
    
    if ( !transactionsExist )
        return false;

    /// Get bank prioritization based on 'queue bank pointers' ///
    const vector<U32>& ind = getBankPriority(queueBankPointers);

    bool activeSent = false;

    if ( smpCounters->reading() && readHits < smpCounters->moreConsecutiveOpsAllowed() )
        activeSent = _tryAdvancedActive(notSentCommand, cycle, true, ind);
    else if ( smpCounters->writing() && writeHits < smpCounters->moreConsecutiveOpsAllowed() )
        activeSent = _tryAdvancedActive(notSentCommand, cycle, false, ind);

    if ( aam == AAM_Agressive && !activeSent ) {
        if ( smpCounters->reading() && readHits >= smpCounters->moreConsecutiveOpsAllowed() && writeHits < smpCounters->MaxConsecutiveWrites() )
            activeSent = _tryAdvancedActive(notSentCommand, cycle, false, ind);
        else if ( smpCounters->writing() && writeHits >= smpCounters->moreConsecutiveOpsAllowed() && readHits < smpCounters->MaxConsecutiveReads() )
            activeSent = _tryAdvancedActive(notSentCommand, cycle, true, ind);
    }

    return activeSent;

}
*/

bool BankQueueScheduler::_tryAdvancedActive(const DDRCommand* notSentCommand, U64 cycle, bool tryRead, const vector<U32>& bankOrder)
{
    const DDRModuleState& info = moduleState();
    const U32 Banks = bankQ.size();

    for ( U32 bank = 0; bank < bankOrder.size(); ++bank ) {
        const U32 i = bankOrder[bank];
        if ( info.getActiveRow(i) == DDRModuleState::NoActiveRow && !bankQ[i].empty() && bankQ[i].front()->isRead() == tryRead ) {
            DDRCommand* cmd = DDRCommand::createActive(i, bankQ[i].front()->getRow());
            if ( notSentCommand )
                cmd->setProtocolConstraint(notSentCommand->getProtocolConstraint());
            cmd->setAsAdvancedCommmand();
            if ( sendDDRCommand(cycle, cmd, bankQ[i].front()) )
                return true;
            else
                delete cmd;
        }
    }
    return false; 
}


bool BankQueueScheduler::_tryAdvancedActive(const DDRCommand* notSentCommand, U64 cycle)
{
    if ( disableActiveManager )
        return false; // Ignore Active Manager

    U32 readHits, writeHits;
    U32 ignoreBank = notSentCommand ? notSentCommand->getBank() : bankQ.size();
    bool transactionsExist = _countHits(ignoreBank, readHits, writeHits);
    
    if ( !transactionsExist )
        return false;

    /// Get bank prioritization based on 'queue bank pointers' ///
    const vector<U32>& ind = getBankPriority(queueBankPointers);

    bool activeSent = false;

    if ( som->reading() && readHits < som->moreConsecutiveOpsAllowed() )
        activeSent = _tryAdvancedActive(notSentCommand, cycle, true, ind);
    else if ( som->writing() && writeHits < som->moreConsecutiveOpsAllowed() )
        activeSent = _tryAdvancedActive(notSentCommand, cycle, false, ind);

    if ( aam == AAM_Agressive && !activeSent ) {
        if ( som->reading() && readHits >= som->moreConsecutiveOpsAllowed() && writeHits < som->MaxConsecutiveWrites() )
            activeSent = _tryAdvancedActive(notSentCommand, cycle, false, ind);
        else if ( som->writing() && writeHits >= som->moreConsecutiveOpsAllowed() && readHits < som->MaxConsecutiveReads() )
            activeSent = _tryAdvancedActive(notSentCommand, cycle, true, ind);
    }

    return activeSent;

    // return _tryAdvancedActive_Counters(notSentCommand, cycle);
}


void BankQueueScheduler::handler_endOfClock(U64 cycle)
{
    if ( banksShareState ) {    
        // Classic implementation (if one bank is full stall is generated)
        U32 i;
        for ( i = 0; i < bankQ.size(); i++ )
        {
            if ( bankQ[i].size() >= bankQueueSz - 1 )
                break;
        }        

        if ( i == bankQ.size() )
            setState(new SchedulerState(SchedulerState::AcceptBoth));
        else
            setState(new SchedulerState(SchedulerState::AcceptNone));
    }
    else { // new implementation (each bank has a individual stall state)
        std::vector<SchedulerState::State>* states = new std::vector<SchedulerState::State>();

        for ( U32 i = 0; i < bankQ.size(); ++i )
            states->push_back( bankQ[i].size() >= bankQueueSz - 1 ? SchedulerState::AcceptNone : SchedulerState::AcceptBoth );

        setState(new SchedulerState(states));
    }
}

void BankQueueScheduler::_coreDump() const
{
    using std::cout;
    cout << "\n-----------------------------------\n";
    cout << "Max Queue sizes: " << bankQueueSz << "\n";
    for ( U32 i = 0; i < bankQ.size(); i++ )
        cout << "Queue bank=" << i << ". Size = " << bankQ[i].size() << "\n";
}
