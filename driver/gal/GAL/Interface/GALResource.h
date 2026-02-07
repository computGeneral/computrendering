/**************************************************************************
 *
 */

#ifndef GAL_RESOURCE
    #define GAL_RESOURCE

#include "GALTypes.h"

namespace libGAL
{

/**
 * Resource types
 */
enum GAL_RESOURCE_TYPE
{
    GAL_RESOURCE_UNKNOWN,
    GAL_RESOURCE_BUFFER,
    GAL_RESOURCE_TEXTURE1D,
    GAL_RESOURCE_TEXTURE2D,
    GAL_RESOURCE_TEXTURE3D,
    GAL_RESOURCE_TEXTURECUBEMAP,
};

/**
 * Equivalent to D3D10_USAGE enum (D3D10_USAGE_INMUTABLE has no equivalent currently)
 *
 * @note See D3D10_USAGE enum specification to a more comprensive explanation
 */
enum GAL_USAGE
{
    /**
     * Equivalent to D3D10_USAGE_DEFAULT (default if usage is not specified)
     * Not mappable, GPU in/out, 
     */
    GAL_USAGE_STATIC, ///< 
    GAL_USAGE_DYNAMIC, ///< Equivalent to D3D10_USAGE_DYNAMIC
    GAL_USAGE_STAGING ///< Equivalent to D3D10_USAGE_STAGING
};


/**
 *
 *  Defines how the resource is stored in memory.
 *
 */
enum GAL_MEMORY_LAYOUT
{
    GAL_LAYOUT_TEXTURE,
    GAL_LAYOUT_RENDERBUFFER
};


/**
 * Base interface for all resources managed by GALResourceManager
 *
 * @note Synchronization of resources with local GPU memory is done transparently
 *       by the libGAL. Implementors on top of libGAL (OGL and D3D) can assume 
 *       instantaneuos resource data synchronization.
 *
 * @note To optimize the scheduling of resources in GPU memory the setPriority()
 *       method can be used.
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 01/23/2007
 */
class GALResource
{
public:

    /**
     * Sets the resource usage
     *
     * This method must be called before set any resource contents and can not be called
     * again after setting contents
     *
     * @param usage Resource usage
     */
    virtual void setUsage(GAL_USAGE usage) = 0;

    /**
     * Gets the resource usage
     *
     * @returns Resource usage
     */
    virtual GAL_USAGE getUsage() const = 0;

    /**
     *
     *  Sets the memory layout for the resource.
     *
     *  @param layout The memory layout defined for the resource.
     *
     */
    virtual void setMemoryLayout(GAL_MEMORY_LAYOUT layout) = 0;

    /**
     *
     *  Gets the memory layout for the resource.
     *
     *  @return The memory layout defined for the resource.
     *
     */
    virtual GAL_MEMORY_LAYOUT getMemoryLayout() const = 0;

    /**
     * Gets the resource type identifier
     *
     * @returns The resource type identifier
     */
    virtual GAL_RESOURCE_TYPE getType() const = 0;

    /**
     * Sets the priority of this resource
     *
     * @param prio the = modifiedBytes;
     *
     * @note lower number indicates greater priority (0 means maximum prority)
     */
    virtual void setPriority(gal_uint prio) = 0;

    /**
     * Gets the priority of the resource
     *
     * @returns the resource priority
     */
    virtual gal_uint getPriority() const = 0;

    /**
     * Gets the number of subresources that compound this resource
     *
     * @returns the number of subresources that compound the resource
     */
    virtual gal_uint getSubresources() const = 0;

    /**
     * Checks if a resource is well defined and can be bound to the pipeline
     *
     * @return true if the resource is well defined, false otherwise
     *
     * @note i.e: Textures are well defined when the mipmap chain is coherently defined
     */
    virtual gal_bool wellDefined() const = 0;
                         
};


}

#endif // GAL_RESOURCE
