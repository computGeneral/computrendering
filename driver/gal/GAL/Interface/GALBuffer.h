/**************************************************************************
 *
 */

#ifndef GAL_BUFFER
    #define GAL_BUFFER

#include "GALTypes.h"
#include "GALResource.h"

namespace libGAL
{

/**
 * GALBuffer interface is the common interface to declare API independent data buffers.
 * API-dependant vertex and index buffers are mapped onto this interface
 *
 * This objects are created from ResourceManager factory
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 01/19/2007
 */
class GALBuffer : public GALResource
{
public:

    /**
     * Retrieves a pointer to the internal data of the GALBuffer object
     *
     * @returns the pointer to the internal data (read-only) of the GALBufferData object
     */
    virtual const gal_ubyte* getData() const = 0;

    /**
     * Gets the size in bytes of the internal vertex buffer data
     *
     * @returns The vertex buffer size in bytes
     */
    virtual gal_uint getSize() const = 0;

    /**
     * Sets a new data buffer size
     *
     * @param size New data buffer size in bytes
     * @param discard If true, discards previous data. If false, the buffer is resized maintaing previous data
     */
    virtual void resize(gal_uint size, bool discard) = 0;

    /**
     * Discard previous buffer contents
     */
    virtual void clear() = 0;

    /**
     * Adds data to the end of the current accesible data (the buffer will grow accordingly)
     */
    virtual void pushData(const gal_void* data, gal_uint size) = 0;

    /**
     * Updates the buffer or a specified range of the data buffer
     *
     * @param offset offset in the buffer (where to start updating)
     * @param size amount of data in the supplied pointer
     * @param data pointer to the data to update the buffer
     * @param preload boolean flag that defines if the data is to be preloaded into GPU memory (no cycles
     * spent on simulation).
     *
     */
    virtual void updateData( gal_uint offset, gal_uint size, const gal_ubyte* data, gal_bool preload = false ) = 0;

};

} // namespace libGAL

#endif // GAL_BUFFER
