/**************************************************************************
 *
 */

#ifndef GAL_STREAM
    #define GAL_STREAM

#include "GALTypes.h"
#include "GALBuffer.h"
#include <string>

namespace libGAL
{

/**
 * Supported components data types in CG1 (other type could be supported in future versions)
 */
enum GAL_STREAM_DATA
{
    GAL_SD_UNORM8,
    GAL_SD_SNORM8,
    GAL_SD_UNORM16,
    GAL_SD_SNORM16,
    GAL_SD_UNORM32,
    GAL_SD_SNORM32,
    GAL_SD_FLOAT16,
    GAL_SD_FLOAT32,
    GAL_SD_UINT8,
    GAL_SD_SINT8,
    GAL_SD_UINT16,
    GAL_SD_SINT16,
    GAL_SD_UINT32,
    GAL_SD_SINT32,
    
    GAL_SD_INV_UNORM8,
    GAL_SD_INV_SNORM8,
    GAL_SD_INV_UNORM16,
    GAL_SD_INV_SNORM16,
    GAL_SD_INV_UNORM32,
    GAL_SD_INV_SNORM32,
    GAL_SD_INV_FLOAT16,
    GAL_SD_INV_FLOAT32,
    GAL_SD_INV_UINT8,
    GAL_SD_INV_SINT8,
    GAL_SD_INV_UINT16,
    GAL_SD_INV_SINT16,
    GAL_SD_INV_UINT32,
    GAL_SD_INV_SINT32
};

struct GAL_STREAM_DESC
{
    gal_uint offset;
    gal_uint components;
    GAL_STREAM_DATA type;
    gal_uint stride;
    gal_uint frequency;
};

/**
 * This interface represents an CG1 data stream processed by the CG1 cmoStreamController unit
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 01/22/2007
 */
class GALStream
{
public:

    /**
     * Sets all the parameters of the stream
     *
     * Sets all the parametersn of the stream
     *
     * @param buffer The data buffer used to feed this stream
     * @param desc Descriptor with 
     */
    virtual void set( GALBuffer* buffer, const GAL_STREAM_DESC& desc ) = 0;

    /**
     * Gets all the parameters of the stream
     *
     * @retval buffer the buffer used to feed this stream
     * @retval offset Offset from the beginning og the data buffer (in bytes)
     * @retval components The number of components of each item in the stream (in the buffer)
     * @retval componentsType Type of the components
     * @retval stride Stride in bytes between two consecutive items in the data buffer
     * @retval frequency Frenquency of the stream: 0-> per vertex/index, >0 -> per n instances.
     */
    virtual void get( GALBuffer*& buffer,
                      gal_uint& offset,
                      gal_uint& components,
                      GAL_STREAM_DATA& componentsType,
                      gal_uint& stride,
                      gal_uint &frequency) const = 0;

    /**
     * Sets the buffer used to feed this stream
     *
     * @param buffer the buffer attached to this stream
     */
    virtual void setBuffer(GALBuffer* buffer) = 0;

    /**
     * Sets an offset in the data buffer
     *
     * @param offset Offset from the beginning of the data buffer (in bytes)
     */
    virtual void setOffset(gal_uint offset) = 0;

    /**
     * Sets the number of components per item
     *
     * @param components The number of components of each item in the stream (in the buffer)
     */
    virtual void setComponents(gal_uint components) = 0;


    virtual void setType(GAL_STREAM_DATA componentsType) = 0;   
    virtual void setStride(gal_uint stride) = 0;
    
    virtual void setFrequency(gal_uint frequency) = 0;

    virtual GALBuffer* getBuffer() const = 0;
    virtual gal_uint getOffset() const = 0;
    virtual gal_uint getComponents() const = 0;
    virtual GAL_STREAM_DATA getType() const = 0;
    virtual gal_uint getStride() const = 0;
    virtual gal_uint getFrequency() const = 0;

    virtual std::string DBG_getState() const = 0;

};

}

#endif // GAL_STREAM
