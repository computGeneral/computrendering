/**************************************************************************
 *
 */

#ifndef CHANNELTRANSACTION_H
    #define CHANNELTRANSACTION_H

#include <string>
#include "DynamicObject.h"
#include "cmMemoryControllerDefs.h"


namespace cg1gpu
{
namespace memorycontroller
{

// forward declaration
class MemoryRequest;

class ChannelTransaction : public DynamicObject
{
public:

    /**
     * Virtual constructor to create a Read Channel Transaction
     *
     * @param memReq The parent memory request
     * @param bank Destination bank
     * @param row The row destination of this channel transaction
     * @param col the starting column
     * @param bytes bytes to be read or read if the transaction has already been performed
     * @param dataBuffer buffer where the read data will be stored
     */
    static ChannelTransaction* createRead(MemoryRequest* memReq, 
                        U32 channel, U32 bank, U32 row, 
                        U32 col, U32 bytes, U08* dataBuffer);

    /**
     * Virtual constructor to create a Write Channel Transaction
     *
     * @param memReq The parent memory request
     * @param bank Destination bank
     * @param row The row destination of this channel transaction
     * @param col The starting column of the transaction
     * @param bytes bytes to be read or read if the transaction has already been performed
     * @param dataBuffer data to be written
     */
    static ChannelTransaction* createWrite(MemoryRequest* memReq, 
                            U32 channel, U32 bank, U32 row, 
                            U32 col, U32  bytes, U08* dataBuffer);

    MemReqState getState() const;

    // equivalent to "getState() == MRS_READY"
    bool ready() const;

    /**
     * Get the unit requester identifier
     */
    GPUUnit getRequestSource() const;

    /**
     * Gets the sub-unit identifier
     */
    U32 getUnitID() const;

    /**
     * Gets the request ID of the associated request to this channel transaction
     *
     * @note equivalent to: channelTrans->getRequest()->getID();
     *
     * @return the associated request ID
     */
    U32 getRequestID() const;

    /**
     * Tells if it is a read transaction
     *
     * @return true if the transaction is a read, false otherwise (it is a write)
     */
    bool isRead() const;

    /**
     * Gets the destination channel
     */
    U32 getChannel() const;

    /**
     * Gets the destination bank
     *
     * This information is temporary (a transaction can be directed to more than one bank)
     * @see MCSPlitter::split method to known more about...
     */
    U32 getBank() const;

    /**
     * Gets the row of this channel transaction
     *
     * @return the row of this channel transaction
     */
    U32 getRow() const;

    /**
     * Gets the start column of this channel transaction
     *
     * @return the first column of this transaction
     */
    U32 getCol() const;
    
    /**
     * Return the amount of bytes in this transaction
     */
    U32 bytes() const;

    /** 
     * Gets the data of this channel transaction
     *
     * @param offset used to get the data beginning from that offset
     * @return the data associated to this transaction
     *
     * @warning if the transaction is a read the data is not available until the read transaction is performed
     */
    const U08* getData(U32 offset = 0) const;

    /**
     * Sets new data to this transaction
     *
     * @param data new data
     * @param how many bytes are available in the data buffer
     * @param offset offset bytes used to update partially the contents of the channel transaction
     */
    void setData(const U08* data, U32 bytes, U32 offset);

    /**
     * Gets the parent memory request
     */
    MemoryRequest* getRequest() const;

    bool overlapsWith(const ChannelTransaction& ct) const;

    /**
     * Tells is the transaction is a masked write
     *
     * @note Read transaction always return false
     */
    bool isMasked() const;
    void setMask(const U32* mask);

    // @warning offset is expressed in 32bit items
    const U32* getMask(U32 offset = 0) const;

    std::string toString() const;

    void dump(bool showData = false) const;

    static U32 countInstances();

    ~ChannelTransaction();

private:

    static U32 instances;

    // set when the channel transactions arrives to the channel queue
    mutable U64 arrivalTimestamp;
    // set when the channel transaction initiates a pre/act 
    mutable U64 startPageSetupTimestamp;
    // set when the channel transaction initiates an act, the value set is cycle + a2r or a2w
    mutable U64 pageReadyTimestamp;
    // set when the channel transaction is selected by the channel scheduler
    mutable U64 channelTransactionSelectedTimestamp;
    // set when the first read/write ddr command is issued to the DDR modules
   // mutable U64 startPageAccessTimestamp;

    /**
     * Forbids creating, copying and assigning ChannelTransaction objects directly
     */
    ChannelTransaction();
    ChannelTransaction(const ChannelTransaction&);
    ChannelTransaction& operator=(const ChannelTransaction&);

    enum { CHANNEL_TRANSACTION_MAX_BYTES = 128 };

    //U08 data[CHANNEL_TRANSACTION_MAX_BYTES];
    U08* dataBuffer; // pointer to the input/output buffer
    const U32* mask; // if null, it is not masked
    U32 size;
    bool readBit;
    U32 channel;
    U32 bank;
    U32 row;
    U32 col;
    MemoryRequest* req;

}; // class ChannelTransaction


} // namespace memorycontroller

} // namespace cg1gpu

#endif // CHANNELTRANSACTION_H
