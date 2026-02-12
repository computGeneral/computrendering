/**************************************************************************
 *
 * Texture Cache class definition file.
 *
 */

/**
 *
 *  @file cmTextureCacheGen.h
 *
 *  Defines the Texture Cache class.  This class defines the cache used
 *  to access texture data.
 *
 */


#ifndef _TEXTURECACHEGEN_

#define _TEXTURECACHEGEN_

#include "GPUType.h"
#include "cmMemoryTransaction.h"
#include <string>

namespace arch
{

/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to get texture data from memory.
 *  The texture cache is used in the Texture Unit GPU unit.
 *
 *
 */

class TextureCacheGen
{

private:


public:

    /**
     *
     *  Texture Cache constructor.
     *
     *  Creates and initializes a Texture Cache object.
     *
     *  @return A new initialized cache object.
     *
     */

    TextureCacheGen();

    /**
     *
     *  Reserves and fetches (if the line is not already available) the cache line for the requested address.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param address The address in GPU memory of the data to read.
     *  @param way Reference to a variable where to store the way in which the data for the
     *  fetched address will be stored.
     *  @param line Reference to a variable where to store the cache line where the fetched data will be stored.
     *  @param tag Reference to a variable where to store the line tag to wait for the fetched address.
     *  @param ready Reference to a variable where to store if the data for the address is already available.
     *  @param source Pointer to a DynamicObject that is the source of the fetch request.
     *
     *  @return If the line for the address could be reserved and fetched (ex. all line are already reserved).
     *
     */

    virtual bool fetch(U64 address, U32 &way, U32 &line, U64 &tag, bool &ready, DynamicObject *source) = 0;

   /**
     *
     *  Reads texture data data from the texture cache.
     *  The line associated with the requested address must have been previously fetched, if not an error
     *  is produced.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param address Address of the data in the texture cache.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address is stored.
     *  @param size Amount of bytes to read from the texture cache.
     *  @param data Pointer to an array where to store the read color data for the stamp.
     *
     *  @return If the read could be performed (ex. line not yet received from memory).
     *
     */

    virtual bool read(U64 address, U32 way, U32 line, U32 size, U08 *data) = 0;

    /**
     *
     *  Unreserves a cache line.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param way The way of the cache line to unreserve.
     *  @param line The cache line to unreserve.
     *
     */

    virtual void unreserve(U32 way, U32 line) = 0;

    /**
     *
     *  Resets the Input Cache structures.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *
     */

    virtual void reset() = 0;

    /**
     *
     *  Process a received memory transaction from the Memory Controller.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param memTrans Pointer to a memory transaction.
     *
     */

    virtual void processMemoryTransaction(MemoryTransaction *memTrans) = 0;

    /**
     *
     *  Updates the state of the memory request queue.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param cycle Current simulation cycle.
     *  @param memoryState Current state of the Memory Controller.
     *  @param filled Reference to a boolean variable where to store if a cache line
     *  was filled this cycle.
     *  @param tag Reference to a variable where to store the tag for the cache line
     *  filled in the current cycle.
     *
     *  @return A new memory transaction to be issued to the
     *  memory controller.
     *
     */

    virtual MemoryTransaction *update(U64 cycle, MemState memoryState, bool &filled, U64 &tag) = 0;

    /**
     *
     *  Simulates a cycle of the texture cache.
     *
     *  Pure virtual.  The derived class must implement this function.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void clock(U64 cycle) = 0;

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    virtual void stallReport(U64 cycle, std::string &stallReport) = 0;

    /**
     *
     *  Enables or disables debug output for the texture cache.
     *
     *
     *  @param enable Boolean variable used to enable/disable debug output for the TextureCache.
     *
     */

    virtual void setDebug(bool enable) = 0;
};

} // namespace arch

#endif
