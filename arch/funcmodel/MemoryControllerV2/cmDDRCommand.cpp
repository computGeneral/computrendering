/**************************************************************************
 *
 */

#include "cmDDRCommand.h"
#include <iostream>
#include <stdio.h>
#include <sstream>

using namespace std;
using arch::memorycontroller::DDRCommand;
using arch::memorycontroller::DDRBurst;

U32 DDRCommand::instances = 0;

DDRCommand::DDRCommand(DDRCmd cmd, U32 bank, U32 row, U32 column, bool autoprecharge, DDRBurst* data, ProtocolConstraint pc) :
    cmd(cmd), bank(bank), row(row), column(column), autoprecharge(autoprecharge), data(data), protocolConstraint(pc), advanced(false)
{
    ++instances;

    //  Set object color for tracing based on DDR command
    setColor(cmd);
}

DDRCommand::~DDRCommand()
{
    --instances;

    GPU_ASSERT
    (
        if ( cmd != DDRCommand::Write && data != 0 )
            CG_ASSERT("Data defined and command different of a write");
    )
    
    // Burst data is not owned by the DDRCommand
    // Must be deleted explicitly 
    // i.e: delete cmd->getData();
    //delete data;
}


U32 DDRCommand::countInstances()
{
    return instances;
}

DDRCommand::DDRCmd DDRCommand::which() const
{
    return cmd;
}

string DDRCommand::protocolConstraintToString(ProtocolConstraint pc)
{
    switch ( pc ) 
    {
        case PC_none:
            return string("NO_CONSTRAINT");
        case PC_a2a:
            return string("ACT_TO_ACT");
        case PC_a2p:
            return string("ACT_TO_PRE");
        case PC_a2r:
            return string("ACT_TO_READ");
        case PC_a2w:
            return string("ACT_TO_WRITE");
        case PC_r2w:
            return string("READ_TO_WRITE");
        case PC_r2p:
            return string("READ_TO_PRE");
        case PC_w2r:
            return string("WRITE_TO_READ");
        case PC_w2p:
            return string("WRITE_TO_PRE");
        case PC_p2a:
            return string("PRE_TO_ACT");
        default:
            return string("UNKNOWN CONSTRAINT");
    }
}

string DDRCommand::whichStr() const
{
    stringstream ss;
    switch ( cmd )
    {
        case Deselect:
            ss << "Deselect";
            break;
        case CG1_ISA_OPCODE_NOP:
            ss << "CG1_ISA_OPCODE_NOP";
            break;
        case Active:
            ss << "Active bank=" << bank << " row=" << row;
            break;
        case Read:
            ss << "Read bank=" << bank << " column=" << column;
            break;
        case Write:
            ss << "Write bank=" << bank << " column=" << column;
            break;
        case Precharge:
            if ( bank == AllBanks )
                ss << "Precharge ALL\n";
            else
                ss << "Precharge " << bank;
            break;
        case AutoRefresh:
            ss << "Autorefresh";
            break;
        case Dummy:
            ss << "Dummy constraint=\"" << protocolConstraintToString(protocolConstraint) << "\"";
            break;
        default:
            ss << "Unknown command";
    }
    return ss.str();
}

U32 DDRCommand::getBank() const
{
    return bank;
}

U32 DDRCommand::getRow() const
{
    if ( cmd != Active )
        CG_ASSERT("Row is only defined for Active command");
    return row;
}

U32 DDRCommand::getColumn() const
{
    if ( cmd != Read && cmd != Write )
        CG_ASSERT(
              "Column is only defined for Read and Write commands");
    return column;
}

const DDRBurst* DDRCommand::getData() const
{
    // implicit cast to "const DDRBurst" pointer
    if ( cmd != Read && cmd != Write )
        CG_ASSERT(
              "Data is only defined for Read and Write commands");

    return data;
}

bool DDRCommand::autoprechargeEnabled() const
{
    if ( cmd != Read && cmd != Write )
        CG_ASSERT(
              "autoprechargeEnabled is only defined for Read and Write commands");
    return autoprecharge;
}

DDRCommand* DDRCommand::createDummy(ProtocolConstraint pc)
{
    DDRCommand* dummy = new DDRCommand(DDRCommand::Dummy, 0, 0, 0, false, 0, pc);
    sprintf((char*)dummy->getInfo(), DDRCommand::protocolConstraintToString(pc).c_str());
    return dummy;
}

DDRCommand* DDRCommand::createActive(U32 bank, U32 row, ProtocolConstraint pc)
{
    return new DDRCommand(DDRCommand::Active, bank, row, 0, false, 0, pc);
}

DDRCommand* DDRCommand::createRead(U32 bank, U32 column, 
                                          bool autoprecharge, ProtocolConstraint pc)
{
    return new DDRCommand(DDRCommand::Read, bank, 0, column, autoprecharge, 0, pc);
    
}

DDRCommand* DDRCommand::createWrite(U32 bank, U32 column, DDRBurst* data,
                                           bool autoprecharge, ProtocolConstraint pc)
{
    return new DDRCommand(DDRCommand::Write, bank, 0, column, autoprecharge, data, pc);
}

DDRCommand* DDRCommand::createPrecharge(U32 bank, ProtocolConstraint pc)
{
    return new DDRCommand(DDRCommand::Precharge, bank, 0, 0, false, 0, pc);
}



string DDRCommand::toString() const
{
    stringstream ss;
    switch ( cmd )
    {
        case DDRCommand::Dummy:
            ss << "DUMMY_COMMAND constraint=\"" << protocolConstraintToString(protocolConstraint) << "\"";
            break;
        case DDRCommand::Active:
            ss << "ACTIVE bank=" << bank << " row=" << row;
            break;
        case DDRCommand::Read:
            ss << "READ bank=" << bank << " col=" << column << " autopre=" 
                << (autoprecharge?"yes":"no");
            break;
        case DDRCommand::Write:
            ss << "WRITE bank=" << bank << " col=" << column << " autopre=" 
                << (autoprecharge?"yes":"no");
            break;
        case DDRCommand::Precharge:
            ss << "PRECHARGE bank=";
            if ( bank == AllBanks )
                ss << "ALL";
            else
                ss << bank;
            break;
        default:
            ss << whichStr();
    }

    if ( advanced )
        return ss.str() + " (ADVANCED)";

    return ss.str();
}


void DDRCommand::dump() const
{
    
    #define CASEUNSUPPORTED(cmd_) case cmd_: cout << #cmd_ " cannot be dumped" << endl; break;
    
    switch ( cmd )
    {
        case DDRCommand::Dummy:
            cout << "DUMMY_COMMAND" << endl;
        case DDRCommand::Active:
            cout << "ACTIVE bank=" << bank << " row = " << row << (advanced ? " (ADVANCED)" : "") << endl;
            break;
        case DDRCommand::Read:
            cout << "READ bank=" << bank << " column=" << column << " autoprecharge=" 
                 << (autoprecharge?"yes":"no") << endl;
            break;
        case DDRCommand::Write:
            cout << "WRITE bank=" << bank << " column=" << column << " autoprecharge=" 
                 << (autoprecharge?"yes":"no") << endl;
            break;
        case DDRCommand::Precharge:
            cout << "PRECHARGE bank=";
            if ( bank == AllBanks )
                cout << "All" << endl;
            else
                cout << bank << (advanced ? " (ADVANCED)" : "") << endl;
            break;
            
        // Predefined cases for unsupported commands
        CASEUNSUPPORTED(Deselect)
        CASEUNSUPPORTED(CG1_ISA_OPCODE_NOP)
        CASEUNSUPPORTED(AutoRefresh)
        CASEUNSUPPORTED(Unknown)
        default:
            CG_ASSERT("command unknown. Inconsistent state");
    }
    
    #undef CASEUNSUPPORTED
    
}

