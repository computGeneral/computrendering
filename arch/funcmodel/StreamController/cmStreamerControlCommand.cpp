/**************************************************************************
 *
 * cmoStreamController Control Command class implementation file.
 *
 */

/**
 *
 *  @file StreamerControlCommand.cpp
 *
 *  Implements the cmoStreamController Control Command class.
 *
 *
 */

#include "cmStreamerControlCommand.h"
#include "support.h"
#include <stdio.h>

using namespace cg1gpu;

//  Constructor for STRC_DEALLOC_X.  
StreamerControlCommand::StreamerControlCommand(StreamControl comm, U32 entry)
{
    //  Check command.  
    GPU_ASSERT(
        if ((comm != STRC_DEALLOC_IRQ) && (comm != STRC_DEALLOC_OFIFO)
            && (comm != STRC_DEALLOC_OM) && (comm != STRC_DEALLOC_OM_CONFIRM)
            && (comm != STRC_OM_ALREADY_ALLOC))
            CG_ASSERT("No deallocation command.");
    )

    //  Set command.  
    command = comm;

    //  Set entry to deallocate.  
    switch(command)
    {
        case STRC_DEALLOC_IRQ:
            IRQEntry = entry;
            setUnitID(0);
            sprintf((char *) getInfo(), "DEALLOC_IRQ entry = %d", getIRQEntry());
            break;

        case STRC_DEALLOC_OFIFO:
            OFIFOEntry = entry;
            sprintf((char *) getInfo(), "DEALLOC_OFIFO entry = %d", getOFIFOEntry());
            break;

        case STRC_DEALLOC_OM:
            OMLine = entry;
            sprintf((char *) getInfo(), "DEALLOC_OM entry = %d", getOMLine());
            break;

        case STRC_DEALLOC_OM_CONFIRM:
            OMLine = entry;
            sprintf((char *) getInfo(), "DEALLOC_OM_CONFIRM entry = %d", getOMLine());
            break;

        case STRC_OM_ALREADY_ALLOC:
            OMLine = entry;
            sprintf((char *) getInfo(), "OM_ALREADY_ALLOC entry = %d", getOMLine());
            break;

        default:
            CG_ASSERT("Wrong deallocation command.");
    }

    //  Set color for signal tracing.  
    setColor(command);

    setTag("stCCom");
}

//  Constructor for STRC_NEW_INDEX and STRC_UPDATE.  
StreamerControlCommand::StreamerControlCommand(StreamControl com, U32 idx, U32 instanceIdx, U32 entry)
{
    //  Set the command.  
    command = com;

    //  Set the index.  
    index = idx;

    //  Set the instance index.
    instanceIndex = instanceIdx;
    
    switch(com)
    {
        case STRC_NEW_INDEX:

            //  Set Output FIFO entry.  
            OFIFOEntry = entry;
            sprintf((char *) getInfo(), "NEW_INDEX idx = %d inst = %d OFIFOentry = %d", index, instanceIndex, OFIFOEntry);
            break;

        case STRC_UPDATE_OC:

            //  Set Output Memory line.  
            OMLine = entry;
            sprintf((char *) getInfo(), "UPDATE_OC idx = %d inst = %d OMLine = %d", index, instanceIndex, OMLine);
            break;

        default:
            CG_ASSERT("Wrong control command.");
            break;
    }

    //  Set color for signal tracing.  
    setColor(command);

    setTag("stCCom");
}

//  Returns the StreamerControlCommand command.  
StreamControl StreamerControlCommand::getCommand()
{
    return command;
}

//  Return the input index.  
U32 StreamerControlCommand::getIndex()
{
    return index;
}

//  Return the instance index.
U32 StreamerControlCommand::getInstanceIndex()
{
    return instanceIndex;
}

//  Return the IRQ entry.  
U32 StreamerControlCommand::getIRQEntry()
{
    return IRQEntry;
}

//  Return the output FIFO entry.  
U32 StreamerControlCommand::getOFIFOEntry()
{
    return OFIFOEntry;
}

//  Sets the output memory line found in the output cache.  
void StreamerControlCommand::setOMLine(U32 omLine)
{
    OMLine = omLine;
}

//  Return the output memory line.  
U32 StreamerControlCommand::getOMLine()
{
    return OMLine;
}

//  Sets the output cache hit attribute.  
void StreamerControlCommand::setHit(bool hitOrMiss)
{
    hit = hitOrMiss;
}

//  Return if it was a hit in the output cache.  
bool StreamerControlCommand::isAHit()
{
    return hit;
}

//  Set last index in batch flag.  
void StreamerControlCommand::setLast(bool lastInBatch)
{
    last = lastInBatch;
}

//  Return if the index is the last in the batch.  
bool StreamerControlCommand::isLast()
{
    return last;
}

//  Sets the StreamController loader unit id (for STRC_DEALLOC_IRQ StreamController commands).  
void StreamerControlCommand::setUnitID(U32 unit)
{
    unitID = unit;
}

//  Gets the StreamController loader unit id (for STRC_DEALLOC_IRQ StreamController commands).  
U32 StreamerControlCommand::getUnitID()
{
    return unitID;
}

