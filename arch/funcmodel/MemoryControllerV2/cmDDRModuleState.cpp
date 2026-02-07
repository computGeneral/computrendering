/**************************************************************************
 *
 */

#include "cmDDRModuleState.h"
#include <iostream>
#include <sstream>

using namespace std;
using namespace cg1gpu::memorycontroller;

DDRModuleState::DDRModuleState(U32 nBanks, U32 burstLength, 
                               // U32 burstElementsPerCycle, 
                               U32 burstBytesPerCycle,
                               const GPUMemorySpecs& memSpecs) : 
    // DebugTracker("DDRModuleStateTRACKER"),
    nBanks(nBanks), burstLength(burstLength),
    cycle(0), lastCommand(C_Unknown),
    lastActiveBank(0), lastActiveStart(0), lastActiveEnd(0),
    lastReadBank(0), lastReadStart(0), lastReadEnd(0),
    lastWriteBank(0), lastWriteStart(0), lastWriteEnd(0),
    // burstTransmissionTime(burstLength/burstElementsPerCycle)
    burstTransmissionTime((4*burstLength)/burstBytesPerCycle)

{
    setModuleParameters(memSpecs, burstLength);

    bankState = new BankState[nBanks];

    for ( U32 i = 0; i < nBanks; i++ )
    {
        BankState& bs = bankState[i];
        bs.autoprecharge = false;
        bs.state = BS_Idle;
        bs.endCycle = 0;
        bs.lastWriteEnd = 0;
        bs.openRow = NoActiveRow;
    }
}

string DDRModuleState:: getCommandIDStr(CommandID cmdid)
{
    switch ( cmdid ) {
        case C_Active:
            return "Active";
        case C_Precharge:
            return "Precharge";
        case C_Read:
            return "Read";
        case C_Write:
            return "Write";
        default:
            return "Unknown";
    }
}

U32 DDRModuleState::getActiveRow(U32 bank) const
{
    if ( bank >= nBanks )
        CG_ASSERT("Bank identifier too high");
    return bankState[bank].openRow;
}

U32 DDRModuleState::banks() const
{
    return nBanks;
}

DDRModuleState::CommandMask DDRModuleState::acceptedCommands(U32 bank) const
{
    GPU_ASSERT
    (
        if ( bank >= nBanks )
            CG_ASSERT("Bank identifier too high");
    )
    
    CommandMask mask = 0;
    
    if ( canBeIssued(bank, C_Active) )
        mask |= ActiveBit;
    if ( canBeIssued(bank, C_Read) )
        mask |= ReadBit;
    if ( canBeIssued(bank, C_Write) )
        mask |= WriteBit;
    if ( canBeIssued(bank, C_Precharge) )
        mask |= PrechargeBit;
    
    return mask;
}

U64 DDRModuleState::getWaitCycles(U32 bank, CommandID cmd) const
{
    CG_ASSERT("Method not implemented yet");
    return 0;
}

string DDRModuleState::getIssueConstraintStr(IssueConstraint ic)
{
    switch ( ic )
    {
        case CONSTRAINT_NONE:
            return string("CONSTRAINT_NONE");
        case CONSTRAINT_PRE_TO_ACT:
            return string("CONSTRAINT_PRE_TO_ACT");
        case CONSTRAINT_ACT_TO_ACT:
            return string("CONSTRAINT_ACT_TO_ACT");
        case CONSTRAINT_ACT_WITH_OPENROW:
            return string("CONSTRAINT_ACT_WITH_OPENROW");
        case CONSTRAINT_ACT_TO_READ:
            return string("CONSTRAINT_ACT_TO_READ");
        case CONSTRAINT_NOACT_WITH_READ:
            return string("CONSTRAINT_NOACT_WITH_READ");
        case CONSTRAINT_AUTOP_READ:
            return string("CONSTRAINT_AUTOP_READ");
        case CONSTRAINT_WRITE_TO_READ:
            return string("CONSTRAINT_WRITE_TO_READ");
        // case CONSTRAINT_READ_TO_READ:
            return string("CONSTRAINT_READ_TO_READ");
        case CONSTRAINT_ACT_TO_WRITE:
            return string("CONSTRAINT_ACT_TO_WRITE");
        case CONSTRAINT_AUTOP_WRITE:
            return string("CONSTRAINT_AUTOP_WRITE");
        case CONSTRAINT_NOACT_WITH_WRITE:
            return string("CONSTRAINT_NOACT_WITH_WRITE");
        case CONSTRAINT_READ_TO_WRITE:
            return string("CONSTRAINT_READ_TO_WRITE");
        // case CONSTRAINT_WRITE_TO_WRITE:
            return string("CONSTRAINT_WRITE_TO_WRITE");
        case CONSTRAINT_WRITE_TO_PRE:
            return string("CONSTRAINT_WRITE_TO_PRE");
        case CONSTRAINT_ACT_TO_PRE:
            return string("CONSTRAINT_ACT_TO_PRE");
        case CONSTRAINT_READ_TO_PRE:
            return string("CONSTRAINT_READ_TO_PRE");
        case CONSTRAINT_DATA_BUS_CONFLICT:
            return string("CONSTRAINT_DATA_BUS_CONFLICT");
        case CONSTRAINT_UNKNOWN:
            return string("CONSTRAINT_UNKNOWN ");
        default:
            return string("UNKNOWN TOKEN CONSTRAINT");
    }
}

DDRModuleState::IssueConstraint DDRModuleState::getIssueConstraint(U32 bank, CommandID cmd) const
{
    GPU_ASSERT
    (
        if ( bank >= nBanks )
            CG_ASSERT("Bank identifier too high");
    )

    BankState& bstate = bankState[bank];
    BankStateID state = bstate.state;

    switch ( cmd )
    {
        case C_Active:
            if ( state == BS_Precharging )
                return CONSTRAINT_PRE_TO_ACT;            
            if ( lastActiveEnd != 0 && lastActiveStart + tRRD > cycle )
                return CONSTRAINT_ACT_TO_ACT;
            if ( state != BS_Idle )
                return CONSTRAINT_ACT_WITH_OPENROW;
            return CONSTRAINT_NONE;
        case C_Read:
            // Always check first DATA_BUS_CONFLICT to be able to measure real BW usage and protocol overhead
            if ( lastReadEnd != 0 && lastReadEnd > cycle + CASLatency )
                // return CONSTRAINT_READ_TO_READ;
                return CONSTRAINT_DATA_BUS_CONFLICT; // Data bus is already being used by a previous READ (wait please :-))
            if ( isAnyBank(BS_Writing) )
                return CONSTRAINT_DATA_BUS_CONFLICT; // Data bus is being used by a previoys WRITE
            if ( state == BS_Activating )
                return CONSTRAINT_ACT_TO_READ;
            if ( state == BS_Idle || state == BS_Precharging ) 
                 return CONSTRAINT_NOACT_WITH_READ;
            // check if a previous read/write had autoprecharge enabled
            if ( bstate.autoprecharge )
                return CONSTRAINT_AUTOP_READ;
            // check write to read delay
            if ( lastWriteEnd != 0 && lastWriteEnd + tWTR > cycle )
                return CONSTRAINT_WRITE_TO_READ;
            return CONSTRAINT_NONE;
        case C_Write:
            // Always check first DATA_BUS_CONFLICT to be able to measure real BW usage and protocol overhead
            if ( lastWriteEnd != 0 && lastWriteEnd > cycle + WriteLatency )
                // return CONSTRAINT_WRITE_TO_WRITE;
                return CONSTRAINT_DATA_BUS_CONFLICT; // Data bus is already being used by a previos WRITE (wait please :-))
            //if ( isAnyBank(BS_Reading) )
            if ( lastReadEnd != 0 && cycle + WriteLatency < lastReadEnd )
                return CONSTRAINT_DATA_BUS_CONFLICT;
            // check read to write delay
            if ( lastReadEnd != 0 && cycle + WriteLatency < lastReadEnd + tRTW )
                return CONSTRAINT_READ_TO_WRITE;
            // if ( state == BS_Reading )
            //     return CONSTRAINT_DATA_BUS_CONFLICT; // Data bus is being used by a previous READ
            if ( state == BS_Activating )
                return CONSTRAINT_ACT_TO_WRITE;
            if ( state == BS_Idle || state == BS_Precharging )
                return CONSTRAINT_NOACT_WITH_WRITE;
            // check if a previous read/write had autoprecharge enabled
            if ( bstate.autoprecharge )
                return CONSTRAINT_AUTOP_WRITE;

            return CONSTRAINT_NONE;
        case C_Precharge:
            // Check write to precharge delay constraint (also covers state == BS_Writing)
            // This check must be performed before state == BS_Idle || state == BS_Precharging
            if ( bstate.lastWriteEnd != 0 && bstate.lastWriteEnd + tWR > cycle )
                return CONSTRAINT_WRITE_TO_PRE;
            // check if the bank is being activating
            if ( state == BS_Activating )
                return CONSTRAINT_ACT_TO_PRE;
            if ( state == BS_Reading && bstate.endCycle > cycle + tRP )
                return CONSTRAINT_READ_TO_PRE;
            if ( state != BS_Active && state != BS_Reading )
                return CONSTRAINT_UNKNOWN;
            return CONSTRAINT_NONE;
        default:
            CG_ASSERT("Unexpected command");
    }

    return CONSTRAINT_NONE;
}

bool DDRModuleState::canBeIssued(U32 bank, DDRModuleState::CommandID cmd) const
{
    return getIssueConstraint(bank, cmd) == CONSTRAINT_NONE;
}


void DDRModuleState::setModuleParameters(const GPUMemorySpecs& memSpecs, U32 burstLength_)
{
    switch ( memSpecs.memtype() ) {
        case GPUMemorySpecs::GDDR3:
            {
                const GDDR3Specs& params = static_cast<const GDDR3Specs&>(memSpecs);
                tRRD = params.tRRD;
                tRCD = params.tRCD;
                tWTR = params.tWTR;
                tRTW = params.tRTW;
                tWR = params.tWR;
                tRP = params.tRP;
                CASLatency = params.CASLatency;
                WriteLatency = params.WriteLatency;
                burstLength = burstLength_;
            }
            break;
        case GPUMemorySpecs::GDDR4:
            CG_ASSERT("GDDR4 memory not yet supported");
        case GPUMemorySpecs::GDDR5:
            CG_ASSERT("GDDR5 memory not yet supported");
        default:
            CG_ASSERT("Unknown memory");

    }
}


void DDRModuleState::updateState(U64 cycle)
{
    this->cycle = cycle; // update current cycle

    for ( U32 i = 0; i < nBanks; i++ )
        updateBankState(cycle, i);
    
}

void DDRModuleState::updateBankState(U64 cycle, U32 bankId)
{
    BankState& bstate = bankState[bankId];

    if ( bstate.state == BS_Idle || bstate.state == BS_Active )
        return ; // no passive state change can happen

    if ( cycle < bstate.endCycle )
        return ; // no state updating required until bstate.endCycle

    if ( cycle > bstate.endCycle ) {
        CG_ASSERT("cycle is bigger than bstate.endCycle (updatedBankState() not synchronized properly)");
    }

    // These two vars are used to generate endCycle for "autoprecharge"
    U64 extraDelay = 0;
    bool updateEndCycle = false;

    // { cycle >= bstate.newStateCycle }
    // State transition completed, change bank state
    switch ( bstate.state )
    {
        case BS_Activating:
            /*
            GPU_DEBUG
            (
                cout << "Cycle: " << cycle << 
                " -> Bank state change: Activating -> Active";
            )
            */
            bstate.state = BS_Active;
            break;
        case BS_Reading:
            /*
            GPU_DEBUG
            (
                cout << "Cycle: " << cycle 
                     << " -> Bank state change: Reading -> ";
            )
            */
            if ( bstate.autoprecharge )
            {
                //////////////////////////////////////////////////////////////
                /// This code has not been tested and probably has bugs!!! ///
                //////////////////////////////////////////////////////////////
                if ( tRP + 1 <= CASLatency + burstTransmissionTime )
                {
                    // autoprecharge is fully overlapped with the read command
                    // GPU_DEBUG( cout << "Idle"; )
                    bstate.state = BS_Idle;
                }
                else 
                {
                    // { P: tRP + 1 > CASLatency + burstLength/2 }
                    extraDelay = tRP + 1 - CASLatency - burstTransmissionTime;
                    // there is additional delay due to autoprecharge
                    if ( cycle >= bstate.endCycle + extraDelay )
                    {
                        // GPU_DEBUG( cout << "Idle"; )
                        bstate.state = BS_Idle;
                    }
                    else
                    {
                        // GPU_DEBUG( cout << "Precharging"; )
                        bstate.state = BS_Precharging;
                        updateEndCycle = true;
                    }
                }
                bstate.autoprecharge = false; // reset autoprecharge bit
            }
            else
            {
                // GPU_DEBUG( cout << "Active"; )
                bstate.state = BS_Active;
            }
            break;
        case BS_Writing:
            /*
            GPU_DEBUG
            (
                cout << "Cycle: " << cycle 
                     << " -> Bank state change: Writing -> ";
            )
            */
            if ( bstate.autoprecharge )
            {
                extraDelay = tWR + tRP;
                if ( cycle >= bstate.endCycle + extraDelay )
                {
                    // GPU_DEBUG( cout << "Idle"; )
                    bstate.state = BS_Idle;
                }
                else
                {
                    // GPU_DEBUG( cout << "Precharging"; )
                    bstate.state = BS_Precharging;
                    updateEndCycle = true;
                }
                bstate.autoprecharge = false; // reset autoprecharge bit
            }
            else
            {
                // GPU_DEBUG( cout << "Active"; )
                bstate.state = BS_Active;
            }
            break;
        case BS_Precharging:
            /*
            GPU_DEBUG
            (
                cout << "Cycle: " << cycle 
                     << " -> Bank state change: Precharging -> Idle";
            )
            */
            bstate.state = BS_Idle;
            //banks[bankId].deactivate();
            //bstate.openRow = NoActiveRow; // moved to postPrecharge
            break;
        default:
            CG_ASSERT("Unexpected state");
    }

    /*
    if ( cycle != bstate.endCycle )
        cout << " (changed in cycle: " << bstate.endCycle << ")";
        */

    if ( updateEndCycle )
        bstate.endCycle = cycle + extraDelay;

    //GPU_DEBUG( cout << endl; )
}


void DDRModuleState::postActive(U32 bank, U32 row)
{
    // GPU_DEBUG( cout << "Cycle: " << cycle <<" -> Active command received" << endl; )

    GPU_ASSERT
    (
        if ( bank == AllBanks )
            CG_ASSERT("'bank = AllBanks' not accepted with Active command");
        if ( bank >= nBanks )
            CG_ASSERT("bank identifier too high");
    )
    if ( bankState[bank].state != BS_Idle )
    {
        stringstream ss;
        ss << "The bank " << bank << " is not IDLE, cannot be activated";
        CG_ASSERT(ss.str().c_str());
    }

    if ( lastActiveEnd != 0 && lastActiveStart + tRRD > cycle )
        CG_ASSERT(
              "tRRD violated between two ACTIVE commands");

    // update global state
    lastActiveBank = bank;
    lastActiveStart = cycle;
    lastActiveEnd = cycle + tRCD;

    // update local bank state
//  banks[bankId].activate(activeCommand->getRow());
    bankState[bank].openRow = row;
    bankState[bank].state = BS_Activating;
    bankState[bank].endCycle = lastActiveEnd;
}

void DDRModuleState::postRead(U32 bank, bool autoprecharge)
{
    //GPU_DEBUG( cout << "Cycle: " << cycle <<" -> Read command received" << endl; )
    GPU_ASSERT
    (
        if ( bank == AllBanks )
            CG_ASSERT("'bank = AllBanks' not accepted with Read command");
        if ( bank >= nBanks )
            CG_ASSERT("bank identifier too high");
        if ( bankState[bank].state != BS_Active && bankState[bank].state != BS_Reading )
            CG_ASSERT("Only in ACTIVE or READING state the bank can be read");
    )

    // check if a previous read/write had autoprecharge enabled
    if ( bankState[bank].autoprecharge )
        CG_ASSERT("Performing a previous read/write with autoprecharge. Read not accepted");
    
    // check conflict with previous writes (check write to read delay restriction)
    if ( lastWriteEnd != 0 && lastWriteEnd + tWTR > cycle )
        CG_ASSERT("write to read delay violated");

    if ( lastReadEnd != 0 && lastReadEnd > cycle + CASLatency )
        CG_ASSERT("data readout collision between tow reads");

    // update global state
    lastReadBank = bank;
    lastReadStart = cycle;
    lastReadEnd = cycle + CASLatency + burstTransmissionTime;

    // update local bank state
    BankState& bstate = bankState[bank];
    bstate.state = BS_Reading;
    bstate.endCycle = lastReadEnd;
    bstate.autoprecharge = autoprecharge;
}

void DDRModuleState::postWrite(U32 bank, bool autoprecharge)
{
    //GPU_DEBUG( cout << "Cycle: " << cycle <<" -> Write command received" << endl; )
    GPU_ASSERT
    (
        if ( bank == AllBanks )
            CG_ASSERT("'bank = AllBanks' not accepted with Write command");
        if ( bank >= nBanks )
            CG_ASSERT("bank identifier too high");
    )

    if ( bankState[bank].autoprecharge )
        CG_ASSERT("Performing a previous read/write with autoprecharge. Read not accepted");

    // check read to write delay
    if ( lastReadEnd != 0 && cycle + WriteLatency < lastReadEnd + tRTW )
    {
        stringstream ss;
        ss << "In a read DQ must be unused for at least " << tRTW << " cycles using DQ pins for writing";
        CG_ASSERT(ss.str().c_str());
    }

    if ( lastWriteEnd != 0 && lastWriteEnd > cycle + WriteLatency )
        CG_ASSERT("data readin collision between two writes");

    // update global state
    lastWriteBank = bank;
    lastWriteStart = cycle;
    lastWriteEnd = cycle + WriteLatency + burstTransmissionTime;

    // update local bank state
    BankState& bstate = bankState[bank];
    bstate.state = BS_Writing;
    bstate.endCycle = lastWriteEnd;
    bstate.lastWriteEnd = lastWriteEnd;
    bstate.autoprecharge = autoprecharge;
}

void DDRModuleState::postPrecharge(U32 bank)
{
    // GPU_DEBUG( cout << "Cycle: " << cycle <<" -> Precharge command received" << endl; )

    GPU_ASSERT
    (
        if (bank == AllBanks )
        {
            // precharge all
            CG_ASSERT("Precharge ALL not implemented yet");
        }
        if ( bank >= nBanks )
            CG_ASSERT("bank identifier too high");
    )

    if ( bankState[bank].autoprecharge )
    {
        // ignore precharge, previous autoprecharge will be performed
        return;
    }

    BankStateID state = bankState[bank].state;

    // Check write to precharge delay constraint (also covers state == BS_Writing)
    // This check must be performed before state == BS_Idle || state == BS_Precharging
    if ( bankState[bank].lastWriteEnd != 0 && bankState[bank].lastWriteEnd + tWR > cycle )
        CG_ASSERT("write to precharge delay within the same bank violated");     

    if ( state == BS_Idle || state == BS_Precharging )
        return; // interpreted as a CG1_ISA_OPCODE_NOP
    else if ( state == BS_Activating )
        CG_ASSERT("Bank being activated. Can not be precharged");
    else if ( state == BS_Reading )
    {
        if ( bankState[bank].endCycle > cycle + tRP )
            CG_ASSERT("Read to precharge delay violated");
    }
    // else: writing state check is covered by "write to precharge" delay test

    GPU_ASSERT
    (
        if ( state != BS_Active && state != BS_Reading )
            CG_ASSERT("Bank Active/Reading states expected");
    )


    // { P: state == BS_Active || state == BS_Reading }
    bankState[bank].state = BS_Precharging;
    bankState[bank].endCycle = cycle + tRP; 
    bankState[bank].openRow = NoActiveRow;
}


U32 DDRModuleState::writeBurstRequiredCycles() const
{
    return WriteLatency + burstTransmissionTime;
}

U32 DDRModuleState::readBurstRequiredCycles() const
{
    return CASLatency + burstTransmissionTime;
}

U32 DDRModuleState::getBurstLength() const
{
    return burstLength;
}

DDRModuleState::BankStateID DDRModuleState::getState(U32 bank) const
{
    if ( bank >= banks() )
        CG_ASSERT("Bank identifier too high");

    return bankState[bank].state;
}

string DDRModuleState::getStateStr(U32 bank) const
{
    BankStateID state = getState(bank);
    switch ( state )
    {
        case BS_Idle:
            return string("BS_Idle");
        case BS_Activating:
            return string("BS_Activating");
        case BS_Active:
            return string("BS_Active");
        case BS_Reading:
            return string("BS_Reading");
        case BS_Writing:
            return string("BS_Writing");
        case BS_Precharging:
            return string("BS_Precharging");
        default:
            return string("DDRModuleState::getStateStr() -> state unknown");
    }
}

bool DDRModuleState::isAnyBank(BankStateID state) const
{
    for ( U32 i = 0; i < banks(); ++i ) {
        if ( bankState[i].state == state )
            return true;
    }
    return false;
}


U32 DDRModuleState::getRemainingCyclesToChangeState(U32 bank) const
{
    if ( bank >= banks() ) {
        CG_ASSERT("Bank identifier too high");
    }

    U64 endCycle = bankState[bank].endCycle;

    if ( cycle >= endCycle )
        return 0;

    GPU_ASSERT(
        if ( endCycle >= cycle + 1000 ) {
            // Panic to avoid the simulator to hang itself
            CG_ASSERT("Difference between bstate[bank].endCycle and last cycle above 1000 cycles (re-check implementation)");
        }
    )

    return static_cast<U32>(endCycle - cycle);

}
