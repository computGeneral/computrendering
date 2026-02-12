/**************************************************************************
 *
 */

#ifndef MEMORYTRANSACTION_H
    #define MEMORYTRANSACTION_H

#include "cmMemoryControllerDefs.h"
#include "DynamicObject.h"
#include <string>

namespace arch {

/**
 *  This class defines a memory transaction from the Command
 *  Processor or other GPU units to the GPU memory controller.
 *
 *  This class inherits from the DynamicObject class
 *  for efficient dynamic creation and destruction of transactions,
 *  and support for tracing.
 *
 */
class MemoryTransaction : public DynamicObject
{
public:

    static const U32 MAX_TRANSACION_SIZE = 128;
    static const U32 MAX_WRITE_SIZE = 16;
        
    /**
     *  Creates an initializated Memory Transaction (read, write, preload)
     *
     *  @param memCom Memory command of the Memory Transaction
     *  @param address GPU memory address where to perform the operation
     *  @param size Amount of data affected by this Memory Transaction
     *  @param data Pointer to the buffer where the data for this Memory Transaction
     *              will be found
     *  @param source GPU source unit for the memory transaction
     *  @param id Transaction identifier
     *
     *  @return A new Memory Transaction object.
     */
    MemoryTransaction(MemTransCom memCom, U32 address, U32 size, 
                      U08 *data, GPUUnit source, U32 id);

    /**
     *  Creates an initializated Memory Transaction (masked write)
     *
     *  @param address GPU memory address where to perform the operation
     *  @param size Amount of data affected by this Memory Transaction
     *  @param data Pointer to the buffer where the data for this Memory Transaction
     *              will be found
     *  @param mask Write mask (per byte)
     *  @param source GPU source unit for the memory transaction
     *  @param id Transaction identifier
     *
     *  @return A new Memory Transaction object.
     */
    MemoryTransaction(U32 address, U32 size, U08 *data, U32 *mask,
        GPUUnit source, U32 id);

    /**
     *  Creates an initializated Memory Transaction (read and write).  For units that
     *  support unit replication (Texture Units, Shaders, ...)
     *
     *  @param memCom Memory command of the Memory Transaction
     *  @param address GPU memory address where to perform the operation
     *  @param size Amount of data affected by this Memory Transaction
     *  @param data Pointer to the buffer where the data for this Memory 
     *              Transaction will be found
     *  @param source GPU source unit for the memory transaction
     *  @param sourceID GPU source unit identifier for the memory transaction
     *  @param id Transaction identifier
     *
     *  @return A new Memory Transaction object.
     */
    MemoryTransaction(MemTransCom memCom, U32 address, U32 size, U08 *data,
        GPUUnit source, U32 sourceID, U32 id);

    /**
     *  Creates an initializated Memory Transaction (masked write).  For units that
     *  support unit replication (Texture Units, Shaders, ...)
     *
     *  @param address GPU memory address where to perform the operation
     *  @param size Amount of data affected by this Memory Transaction
     *  @param data Pointer to the buffer where the data for this Memory 
     *              Transaction will be found
     *  @param mask Write mask (per byte)
     *  @param source GPU source unit for the memory transaction
     *  @param sourceID GPU source unit identifier for the memory transaction
     *  @param id Transaction identifier
     *
     *  @return A new Memory Transaction object
     */
    MemoryTransaction(U32 address, U32 size, U08 *data, U32 *mask,
        GPUUnit source, U32 sourceID, U32 id);

    /**
     *  Memory Transaction construtor (copying a read request to a read data
     *  transaction)
     *
     *  @param requestTrans Request transaction for which to create a data
     *  Memory Transaction
     *
     *  @return A new read data MemoryTransaction object
     */
    MemoryTransaction(MemoryTransaction *requestTrans);

    /**
     *  Creates an initialized controller state Memory Transaction
     *  (for MT_STATE)
     *
     *  @return state State of the memory controller
     */
    MemoryTransaction(MemState state);

    ////////////////////////////////////////////////////////////////
    /// Virtual constructors for the different transaction types ///
    ////////////////////////////////////////////////////////////////
    // For compatibility reasons regular ctors are still available
    /**
     * Creates a memory transaction READ_DATA from a previous request transaction REQ_DATA
     */
    static MemoryTransaction* createReadData(MemoryTransaction* reqDataTran);

    // Creates a new state memory transaction
    static MemoryTransaction* createStateTransaction(MemState state);

    /**
     * Returns the Memory Transaction command
     */
    MemTransCom getCommand() const;

    U32 getAddress() const;
    U32 getSize() const;
    U08* getData() const;
    GPUUnit getRequestSource() const;
    U32 getUnitID() const;
    U32 getID() const;   
    U32 getBusCycles() const;
    MemState getState() const;
    bool isMasked() const;
    U32* getMask() const;
    U32 getRequestID() const;
    void setRequestID(U32 id);

    std::string getRequestSourceStr(bool compactName = false) const;

    bool isToSystemMemory() const;

    std::string toString(bool compact = true) const;

    void dump(bool dumpData = true) const;

    // Creates a string based on the internal representation of the transaction
    // Two exact (field by field) transaction has the same string, two different
    // one has a different identification string
    std::string getIdentificationStr(bool concatData = true) const;


    U32 getHashCode(bool useData = true) const;

    ~MemoryTransaction();

    static const char* getBusName(GPUUnit unit);
    static U32 getBusWidth(GPUUnit unit);
    static void setBusWidth(GPUUnit unit, U32 newBW);

    static U32 countInstances();


private:

    static U32 instances;

    // moved here, in previous version these arrays were extern
    static const char* busNames[LASTGPUBUS];
    static U32 busWidth[LASTGPUBUS];

    MemTransCom command; ///< The operation of this memory transaction
    MemState state; ///< State of the memory controller
    U32 address; ///< Address for this memory transaction
    U32 size; ///< Size of the memory transaction in bytes
    U08 *readData; ///<  Pointer to the buffer where to store data from a MT_READ_DATA transaction (to avoid a copy)
    U08 writeData[MAX_TRANSACTION_SIZE]; ///< Buffer where to store write data
    U08 *preloadData; ///<  Buffer with the preloaded data
    bool masked; ///< It is a mask write
    U32 mask[WRITE_MASK_SIZE]; ///< Write mask per byte
    U32 cycles; ///< Number of bus cycles (MC to GPU unit) that the transaction consumes
    GPUUnit sourceUnit; ///<  Memory transaction source unit
    U32 unitID; ///< Identifies between units of the same type
    U32 ID; ///< Transaction identifier
    U32 requestID; ///< Request pointer/identifier for the memory transaction

};

} // namespace arch



#endif // MEMORYTRANSACTION_H
