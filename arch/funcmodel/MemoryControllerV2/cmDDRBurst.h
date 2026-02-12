/**************************************************************************
 *
 */

#ifndef DDRBURST_H
#define DDRBURST_H

#include <vector>
#include "DynamicObject.h"


namespace arch
{
namespace memorycontroller
{

/**
 * Class implementing a DDR Read/Write burst
 *
 * A DDRBurst is a sequence of 32bit values, each 32bit item can be masked at byte level.
 *
 * @warning: Each U32 value is interpreted as a secuence of 4 bytes so the U32 value
 * 0xAABBCCDD is interpreted as its layaout in memory (that in x86 is little-endian), then
 * is viewed as DD.CC.BB.AA
 */
class DDRBurst : public DynamicObject
{
private:

    enum { MAX_BURST_SIZE = 32 };

    static U32 instances;

    U32 values[MAX_BURST_SIZE];
    U08 masks[MAX_BURST_SIZE];    
    U32 size; ///< can be 4 or 8
    

public:

    /**
     * Convenient enum for selecting bytes
     */
    enum BytePos
    {
        First,
        Second,
        Third,
        Fourth
    };

    /**
     * Sets the number of 32bit elements (size) that DDRBurst
     * objects created with default constructor will have
     *
     * @param size number of 32bit items in within the burst
     */
    //static void setDefaultSize(U32 size);

    /**
     * Returns the current default size used to create DDRBurst objects
     * with the default constructor
     *
     * @return burst size
     */
    //static U32 getDefaultSize();

    /**
     * Creates a burst object of a default size
     */
    //DDRBurst();

    /**
     * Creates a burst object of a given size
     *
     * @param size number of 32bit elements of the burst
     */
    DDRBurst(U32 size);

    ~DDRBurst();

    static U32 countInstances();

    /**
     * Return A pointer to the burst raw bytes
     *
     * @return The pointer to the burst raw bytes
     *
     * @note The number of bytes accessible through the pointer is
     *       equals to getSize()*4
     */
    const U08* getBytes() const;

    /**
     * Sets raw data into the burst
     *
     * @param dataBytes raw bytes
     * @param size number of bytes in dataBytes
     *
     * @note If the specified size is less than burst size the bytes not set are masked
     */
    void setData(const U08* dataBytes, U32 size);

    /**
     * Each U32 mask/unmask 4 data bytes
     *
     * @note maskSize = 0 means: use current burst lenght as mask size
     */
    void setMask(const U32* writeMask, U32 maskSize = 0);

    /**
     * Set one 32bit item of the burst
     *
     * @param position 32bit item position
     * @param datum value of the 32bit item
     */
    void setData(U32 position, U32 datum);

    /**
     * Set one 32bit item of the burst using a per-byte mask
     *
     * @param position 32bit item position
     * @param datum value of the 32bit item
     * @param mask per-byte mask
     * @note masks have this format 0..0XXXX being X = {0,1}
     *
     * @code
     *    // Note that x86 uses Little-endian and the 32-bit item is considered
     *    // as a 4-byte entity without any numerical meaning
     *    // The memory layout (raw bytes) must be considered, not the value
     *    // represented by the integer
     *    Burst* burst = new Burst;
     *    burst->setData(0, 0xAABBCCDD, 0x1);
     *    // in little-endian memory 0xAABBCCDD the layaout in bytes is
     *    // DD.CC.BB.AA, so the mask is selecting only the less significant
     *    // byte (AA). When write method is called only the less significant
     *    // byte (AA) will update the 32-bit destination target
     * @endcode
     *
     * @see write() method
     */
    void setData(U32 position, U32 datum, U08 mask);

    /**
     * Writes a 32bit value on a destination taking into account the
     * associated mask for that value
     *
     * @param position value position within the burst
     * @retval destination write target
     *
     * @code
     *     // Example of use
     *     U32 destination = 0x112233344; // layout in memory 44.33.22.11
     *     Burst* b = new Burst; // Creates a new burst (of default size)
     *     b->setData(1, 0xDDCCBBAA, 0x1);
     *     b->write(1, destination); // perform a masked write
     *     // destination == 0xDD223344
     *     // The layout in memory (little-endian) is 44.33.22.DD
     * @endcode
     */
    void write(U32 position, U32& destination) const;
    
    /**
     * Return the burst size. The size is defined as the number of 32bit datums that compound
     * the burst
     *
     * @return burst size
     */ 
    U32 getSize() const;
    
    /**
     * Dump a readable string representing the burst
     */
    void dump() const;

};

} // namespace memorycontroller

} // namespace arch

#endif // DDRBURST_H
