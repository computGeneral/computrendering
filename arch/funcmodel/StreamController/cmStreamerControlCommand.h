/**************************************************************************
 *
 * cmoStreamController Control Command class definition file.
 *
 */

/**
 *  @file cmStreamerControlCommand.h
 *
 *  This file defines the cmoStreamController Control Command class.
 *
 */

#ifndef _STREAMERCONTROLCOMMAND_

#define _STREAMERCONTROLCOMMAND_

#include "GPUType.h"
#include "DynamicObject.h"

namespace arch
{

//  cmoStreamController Control Command.  
enum StreamControl
{
    STRC_NEW_INDEX,
    STRC_DEALLOC_IRQ,
    STRC_DEALLOC_OFIFO,
    STRC_DEALLOC_OM,
    STRC_DEALLOC_OM_CONFIRM,
    STRC_OM_ALREADY_ALLOC,
    STRC_UPDATE_OC
};


/**
 *
 *  This class defines objects that carry control information
 *  between the different cmoStreamController simulation boxes.
 *  The class inherits from the DynamicObject class that
 *  offers dynamic memory management and signal tracing
 * support.
 *
 */

class StreamerControlCommand : public DynamicObject
{

private:

    StreamControl command;  //  The StreamController control command.  
    U32 index;           //  A new vertex/input index to search/add.  
    U32 instanceIndex;   //  Instance index.  
    U32 unitID;          //  The StreamController loader unit processing the index.  
    U32 IRQEntry;        //  A vertex request queue entry identifier.  
    U32 OFIFOEntry;      //  An output FIFO entry identifier.  
    U32 OMLine ;         //  An output memory line identifier.  
    bool hit;               //  The index was found in the output cache.  
    bool last;              //  Flag marking the a vertex index as the last for the batch.  

public:

    /**
     *
     *  cmoStreamController control command constructor (for STRC_DEALLOC_X).
     *
     *  Creates a new cmoStreamController Control Command object to deallocate
     *  cmoStreamController resources.
     *
     *  @param command The StreamController control command that is
     *  going to be created.
     *  @param An entry in one of the cmoStreamController structures
     *  that must be deallocated.
     *
     *  @return A new initialized StreamerControlCommand.
     *
     */

    StreamerControlCommand(StreamControl command, U32 entry);

    /**
     *
     *  cmoStreamController control command constructor (for STRC_NEW_INDEX
     *  and STRC_UPDATE_OC).
     *
     *  Creates a new cmoStreamController Control Command object to search
     *  a new index in the output cache and to add the new index
     *  in the VRQ and output FIFO.
     *  Or creates a new cmoStreamController Control Command object to ask
     *  for an update of the output cache.
     *
     *  @param command The StreamController control command to create.
     *  @param index The new input index to add/update/search/ for.
     *  @param instanceIndex The instance index corresponding to the current index.
     *  @param entry Output FIFO for the new index or the output memory line for the output cache update.
     *
     *  @return A new initialized StreamerControlCommand object.
     *
     */

    StreamerControlCommand(StreamControl command, U32 index, U32 instanceIndex, U32 entry);

    /**
     *
     *  Gets the cmoStreamController Control Command.
     *
     *  @return The StreamController control command for this object.
     *
     */

    StreamControl getCommand();

    /**
     *
     *  Gets the input index.
     *
     *  @return The input index.
     *
     */

    U32 getIndex();

    /**
     *
     *  Gets the instance index associated with the current index.
     *
     *  @return The instance index for the current index.
     *
     */
     
    U32 getInstanceIndex();
    
    /**
     *
     *  Gets the IRQ entry.
     *
     *  @return The IRQ entry.
     *
     */

    U32 getIRQEntry();

    /**
     *
     *  Gets the output FIFO entry.
     *
     *  @return The output FIFO entry.
     *
     */

    U32 getOFIFOEntry();

    /**
     *
     *  Sets the output memory line for the StreamController control command.
     *
     *  @param omEntry The output memory line.
     *
     */

    void setOMLine(U32 omLine);

    /**
     *
     *  Gets the Output Memory line.
     *
     *  @return The output memory line.
     *
     */

    U32 getOMLine();

    /**
     *
     *  Sets output cache hit attribute.
     *
     *  @param hit If there was a hit in the output cache
     *  for the input index.
     *
     */

    void setHit(bool hit);

    /**
     *
     *  Gets if there was a hit in the output cache for the input index.
     *
     *  @return If there was a hit in the output cache.
     *
     */

    bool isAHit();

    /**
     *
     *  Sets the index as the last one in the batch.
     *
     *  @param last Boolean storing if the index is the last in the batch.
     *
     */

    void setLast(bool last);

    /**
     *
     *  Gets the last in batch mark for the index.
     *
     *  @return A boolean value representing if the index as the last in the batch.
     *
     */

    bool isLast();

    /**
     *
     *  Sets the StreamController loader unit id (for STRC_DEALLOC_IRQ StreamController commands).
     *
     *  @param unit The StreamController loader unit id.
     *
     */

    void setUnitID(U32 unit);

    /**
     *
     *  Gets the StreamController loader unit id (for STRC_DEALLOC_IRQ StreamController commands).
     *
     *  @return The StreamController loader unit id.
     *
     */

    U32 getUnitID();

};

} // namespace arch

#endif
