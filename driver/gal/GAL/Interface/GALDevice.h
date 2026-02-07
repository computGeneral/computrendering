/**************************************************************************
 *
 */

#ifndef GAL_DEVICE
    #define GAL_DEVICE

#include "GALTypes.h"
#include "GALBuffer.h"
#include "GALStream.h"
#include "GALStoredItemID.h"
#include "GALTextureCubeMap.h"
#include "GALTexture3D.h"
#include "GALSampler.h"

#include <list>

namespace libGAL
{

// Forward declaration of interfaces (will be includes)
class GALSampler;
class GALResource;
class GALTexture;
class GALTexture2D;
class GALTextureCubeMap;
class GALRenderTarget;
class GALZStencilTarget;

class GALShaderProgram;

class GALRasterizationStage;
class GALZStencilStage;
class GALBlendingStage;

class GALStoredState;

/**
 * Stored item list definition
 */
typedef std::list<GAL_STORED_ITEM_ID> GALStoredItemIDList;


/**
 * CG1 geometric primitive identifiers
 */
enum GAL_PRIMITIVE
{
    GAL_PRIMITIVE_UNDEFINED,
    GAL_POINTS,
    GAL_LINES,
    GAL_LINE_LOOP,
    GAL_LINE_STRIP,
    GAL_TRIANGLES,
    GAL_TRIANGLE_STRIP,
    GAL_TRIANGLE_FAN,
    GAL_QUADS,
    GAL_QUAD_STRIP
};

enum GAL_SHADER_TYPE
{
    GAL_VERTEX_SHADER,
    GAL_FRAGMENT_SHADER,
    GAL_GEOMETRY_SHADER,
};

struct GAL_SHADER_LIMITS
{
    // Registers available per type
    gal_uint inRegs;
    gal_uint outRegs;
    gal_uint tempRegs;
    gal_uint constRegs;
    gal_uint addrRegs;

    // Read ports available
    gal_uint inReadPorts;
    gal_uint outReadPorts;
    gal_uint tempReadPorts;
    gal_uint constReadPorts;
    gal_uint addrReadPorts;

    // Instructions limits
    gal_uint staticInstrs;
    gal_uint dynInstrs;
};

struct GAL_CONFIG_OPTIONS
{
    gal_bool earlyZ; ///< Enables or disables early Z
    gal_bool hierarchicalZ; ///< Enables or disables hierarchical Z
};


/**
 * This interface represents the high-level abstraction of CG1 GPU
 *
 * It includes access to two interfaces GALResourceManager (for managing resources such as buffers, 
 * textures, etc) and GALStateManager (to manage GPU state that can be saved and restored)
 */
class GALDevice
{
public:

    /**
     * Set configuration options to the GALDevice
     */
    virtual void setOptions(const GAL_CONFIG_OPTIONS& configOptions) = 0;

    /**
     * Sets the primitive to assemble vertex data
     */
    virtual void setPrimitive(GAL_PRIMITIVE primitive) = 0;

    /**
     * Gets an interface to the blending stage
     */
    virtual GALBlendingStage& blending() = 0;

    /**
     * Gets an interface to the rasterization stage
     */
    virtual GALRasterizationStage& rast() = 0;

    /**
     * Gets a configuration interface for the Depth & Stencil stage
     */
    virtual GALZStencilStage& zStencil() = 0;

    /**
     * Creates a Texture2D object
     */
    virtual GALTexture2D* createTexture2D() = 0;

    /**
     * Creates a Texture2D object
     */
    virtual GALTexture3D* createTexture3D() = 0;

    /**
     * Creates a TextureCubeMap object
     */
    virtual GALTextureCubeMap* createTextureCubeMap() = 0;

    /**
     * Creates a GAL Buffer object 
     *
     * @note data can be null to create an uninitilized buffer of size 'size'
     * @note size can be 0 (if data is null) -> resize or pushData should be called before updating the buffer
     */
    virtual GALBuffer* createBuffer(gal_uint size = 0, const gal_ubyte* data = 0) = 0;

    /**
     * Release a GALResource interface
     *
     * @param resourcePtr a pointer to the resource interface
     *
     * @returns true if the method succeds, false otherwise
     */
    virtual gal_bool destroy(GALResource* resourcePtr) = 0;

    /**
     * Releases a GALShaderProgram interface
     *
     * @param shProgram a pointer to the shader program interface
     * @return true if the mehod succeds, false otherwise
     */
    virtual gal_bool destroy(GALShaderProgram* shProgram) = 0;

    /**
     * Sets the current display resolution
     *
     * @param width Resolution width in pixels
     * @param height Resolution height in pixels
     */
    virtual void setResolution(gal_uint width, gal_uint height) = 0;

    /**
     * Gets the current display resolution
     *
     * @retval width Resolution width in pixels
     * @retval height Resolution height in pixels
     * @return true if the resolution is defined, false otherwise
     */
    virtual gal_bool getResolution(gal_uint& width, gal_uint& height) = 0;

    /**
     * Selects an CG1 stream to be configured
     *
     * @param streamID stream identifier
     *
     * @returns the CG1 stream identified by 'stream'
     *
     * @code
     *    // Example: Setting a data buffer for the stream 0 (without changing anything else)
     *    GALDevice* a3ddev = getGALDevice();
     *    GALBuffer* vtxPosition = a3ddev->getResourceManager()->createBuffer(bName, bSize, 0);
     *    // ...
     *    a3ddev->stream(0).setBuffer(vtxPosition);
     * @endcode
     */
    virtual GALStream& stream(gal_uint streamID) = 0;

    /**
     * Enables a vertex attribute and associates it to a stream
     *
     * @param vaIndex Vertex attribute index accessible by the vertex shader
     * @param streamID Stream from where to stream vertex data
     */
    virtual void enableVertexAttribute(gal_uint vaIndex, gal_uint streamID) = 0;

    /**
     * Disables a vertex atributes ( remove the association with the attached stream )
     *
     * @param vaIndex Vertex attribute index to be disabled
     */
    virtual void disableVertexAttribute(gal_uint vaIndex) = 0;

    /**
     * Disables all vertex attributes (remove the association among all the attached streams to attributes)
     */
    virtual void disableVertexAttributes() = 0;

    /**
     * Gets the number of available streams
     *
     * @returns the available number of streams
     */
    virtual gal_uint availableStreams() const = 0;

    /**
     * Gets the number of available vertex inputs
     */
    virtual gal_uint availableVertexAttributes() const = 0;
                                                 
    /**
     * Sets the buffer from where to fetch indices
     *
     * @param ib The index buffer representing the index data to fecth
     * @param offset Offset in bytes from the start of the index buffer
     * @param indicesType type of the indices in the index buffer
     *
     */                       
    virtual void setIndexBuffer( GALBuffer* ib,
                                 gal_uint offset,
                                 GAL_STREAM_DATA indicesType) = 0;

    /**
     * Select an CG1 texture sampler to be configured
     *
     * @param samplerID the GAL sampler identifier
     *
     * @returns a reference to the CG1 sampler identified by 'samplerID'
     *
     * @code
     *  
     *    // Example: Configuring the minification filter of the sampler 0
     *    a3ddev->sampler(0).setMinFilter(minFilter);   
     *
     * @endcode
     */
    virtual GALSampler& sampler(gal_uint samplerID) = 0;

    /**
     * Draws a sequence of nonindexed primitives from the current set of CG1 input streams
     *
     * @param start First vertex of the current set of CG1 streams to fetch
     * @param count Number of vertices to be rendered
     * @param instances Number of instances of the passed primitives to draw.
     *
     */
    virtual void draw(gal_uint start, gal_uint count, gal_uint instances = 1) = 0;
        
    /**
     * Draws the specified primitives based on indexing
     * 
     * @param startIndex The number of the first index used to draw primitives, numbered from the start
     *                   of the index buffer.
     * @param indexCount Number of index used to drawn
     * @param minIndex The lowest-numbered vertex used by the call. baseVertexIndex is added before this
     *                 is used. So the first actual vertex used will be (minIndex + baseVertexIndex)
     * @param maxIndex The highest-numbered vertex used by the call. baseVertexIndex is added before this
     *                 is used. So the last actual vertex used will be (maxIndex + baseVertexIndex)
     * @param baseVertexIndex This is added to all indices before being used to look up a vertex buffer
     * @param instances Number of instances of the passed primitives to draw.
     *
     * @returns True if the method succeds, false otherwise
     *
     * @note set minIndex = 0 and maxIndex = 0 to ignore this parameters
     *
     *
     * @code
     *    // This call
     *
     *    CG1Device->drawIndexed(
     *                    GAL_TRIANGLELIST, // GALPrimitive
     *                     6, // startIndex
     *                     9, // indexCount,
     *                     3, // minIndex
     *                     7, // maxIndex
     *                    20 // baseVertexIndex
     *    );
     * 
     *    // with this buffer: {1,2,3, 3,2,4, 3,4,5, 4,5,6, 6,5,7, 5,7,8}
     *
     *    // It will draw 3 triangles (indexCount = 9), starting with index number 6 (startIndex). So
     *    // it ignores the first two and the last one triangles in the index buffer and only uses these:
     *    // {3,4,5, 4,5,6, 6,5,7} (you have told that you are using only indeces in the range [3,7] OK)
     *
     *    // Then the baseVertexIndex is added to the indices before the vertices are fetched from the
     *    // vertex buffer. So the vertex actually used will be: {23,24,25, 24,25,26, 26,25,27}
     * 
     * @endcode
     */
    virtual void drawIndexed( gal_uint startIndex, 
                                  gal_uint indexCount,
                                  gal_uint minIndex,
                                  gal_uint maxIndex, 
                                  gal_int baseVertexIndex = 0,
                                  gal_uint instances = 1) = 0;

    /**
     * Creates a RenderTarget of a resource to access resource data
     * 
     * @param resource Pointer to the resource that contains the data
     * @param rtDescription Pointer to a render target description. Set this parameter to NULL
     *                      to create a render target that subresource 0 of the entire
     *                      resource (using the format the resource was created with)
     *
     * @code
     *
     * // Create a buffer
      * GALBuffer* buffer = a3ddev->getResourceManager()->createBuffer(bName, bSize, bUsage );
     * // Define a render target description (48 = skip two elements, start from element at pos 2)
     * GAL_RT_DESC rtDesc = { GAL_FORMAT_UNKNOWN, GAL_RT_DIMENSION_BUFFER, { 48, 24 } };
     * // Create a render target using buffer and the render target description
     * GALRenderTarget* rt = a3ddev->createRenderTarget(buffer, rtDesc);
     *
     * @endcode
     */
    //virtual GALRenderTarget* createRenderTarget( GALResource* resource, const GAL_RT_DESC * rtDescription ) = 0;
    virtual GALRenderTarget* createRenderTarget( GALTexture* resource, const GAL_RT_DIMENSION rtdimension, GAL_CUBEMAP_FACE face, gal_uint mipmap ) = 0;

    /**
     * Bind one or more render targets and the depth-stencil buffer to the ROP stage
     *
     * @param numRenderTargets Number of render targets to bind
     * @param renderTargets Pointer to an array of render targets to bind to the device
     * @param zStencilTarget Pointer to a depth-stencil target to bind to the device.
     *                           If NULL, the depth-stencil state is not bound
     *
     * @see createRenderTarget() 
     * @see createZStencilTarget()
     *
     * GALTexture2D* tex2D = ...
     *
     * // Define a render target description & create the render target object
     * GAL_RT_DESC rtDesc = { GAL_FORMAT_R8G8B8A8_UINT, GAL_RT_DIMENSION_TEXTURE_2D, {0} };
     * GALRenderTarget* rtTex2D = a3ddev->createRenderTarget(text2D, rtDesc);
     *
     * // Set the render target
     * a3ddev->setRenderTargets( 1, rtTex2D, 0); // do not use a stencil buffer
     */
    virtual gal_bool setRenderTargets( gal_uint numRenderTargets,
                                   const GALRenderTarget* const * renderTargets,
                                   const GALZStencilTarget* zStencilTarget ) = 0;


    /**
     *
     *  Binds the specified render target as the current render target (back/front buffer).
     *
     *  @param renderTarget Pointer to a render target.
     *
     *  @return If the render target was attached correctly.
     *
     */
     
    virtual gal_bool setRenderTarget(gal_uint indexRenderTarget, GALRenderTarget *renderTarget) = 0;
    
    
    /**
     *
     *  Binds the specified render target as the current depth stencil buffer.
     *
     *  @param renderTarget Pointer to a render target.
     *
     *  @return If the render target was correctly attached.
     *
     */
     
    virtual gal_bool setZStencilBuffer(GALRenderTarget *renderTarget) = 0;
    
    /**
     *
     *  Get a pointer to the current render target.
     *
     *  @return A pointer to the current render target.
     *
     */
     
    virtual GALRenderTarget *getRenderTarget(gal_uint indexRenderTarget) = 0;
    
    /**
     *
     *  Get a pointer to the current z stencil buffer.
     *
     *  @return A pointer to the current z stencil buffer.
     *
     */
     
     virtual GALRenderTarget *getZStencilBuffer() = 0;
    
    /**
     * Copies data from user space memory to a resource
     *
     * This method copies data from user space to a subresource of a resource
     *
     * @param destResource A pointer to the destination resource
     * @param destSubResource A zero-based index identifying the subresource to update
     * @param destRegion The portion of the destination subresource to copy the data into.
     *                   Coordinates are always in bytes. If NULL, the data is written 
     *                   to the destination resource
     *
     * @param srcData pointer to the user space buffer containing the data
     * @param srcRowPitch size in bytes of a row (not used in 1D textures)
     * @param srcDepthPitch size in bytes of a slice (only for 3D textures)
     *
     * @code
     *
     *    GALBuffer* b = a3ddev->getResourceManager()->createBuffer(...);
     *    ...
     *
     * @endcode
     */
    virtual void updateResource( const GALResource* destResource,
                                 gal_uint destSubResource, 
                                 const GAL_BOX* destRegion,
                                 const gal_ubyte* srcData,
                                 gal_uint srcRowPitch,
                                 gal_uint srcDepthPitch ) = 0;

    /**
     * Swaps back buffer into front buffer (displays the frame just rendered)
     */
    virtual gal_bool swapBuffers() = 0;

    /**
     * Clears the color buffer using the RGBA color passed as parameter
     */
    virtual void clearColorBuffer(gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha) = 0;

    /**
     * Clears the Z-buffer and the Stencil buffer using
     */
    virtual void clearZStencilBuffer( gal_bool clearZ,
                                      gal_bool clearStencil,
                                      gal_float zValue,
                                      gal_int stencilValue ) = 0;

    /**
     * Sets all the elements in a render target to one value
     *
     * @param rTarget The render target to clear
     * @param red Red intensity component, ranging from 0 to 255
     * @param green Green intensity component, ranging from 0 to 255
     * @param blue Blue intensity component, ranging from 0 to 255
     * @param alpha Alpha components, ranging from 0 to 255
     */
    virtual void clearRenderTarget(GALRenderTarget* rTarget, 
                                   gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha) = 0;

    /**
     * Clears the Z and Stencil target
     *
     * @param zsTarget The Z & Stencil target to clear
     * @param clearZ Specify to clear the Z part of the Z&Stencil buffer
     * @param clearStencil Specify to clear the Stencil part of the Z&Stencil buffer
     * @param zValue The value used to clear the Z part of the Z&Stencil buffer
     * @param stencilValue The value used to clear the stencil part od the Z&Stencil buffer
     */
    virtual void clearZStencilTarget(GALZStencilTarget* zsTarget,
                                     gal_bool clearZ,
                                     gal_bool clearStencil,
                                     gal_float zValue,
                                     gal_ubyte stencilValue ) = 0;



    /**
     *
     *  Copies data from a 2D texture mipmap to a render buffer.  The dimensions and format of the source
     *  mipmap and the destination render buffer must be the same.
     *
     *  @param sourceTexture Pointer to the source 2D texture.
     *  @param mipLevel Mip level in the source 2D texture to copy.
     *  @param destRenterTarget Pointer to the destination Render Buffer.
     *  @param preload Boolean flag that defines if the data is to be preloaded into GPU memory without
     *  consuming simulation cycles.
     *
     */
    virtual void copySurfaceDataToRenderBuffer(GALTexture2D *sourceTexture, gal_uint mipLevel, GALRenderTarget *destRenderTarget, bool preload) = 0;

    /**
     *
     *  Sets if color is converted from linear to sRGB space on color write.
     *
     */
     
    virtual void setColorSRGBWrite(gal_bool enable) = 0;
    
    /**
     * Gets the shader registers limitations
     */
    virtual void getShaderLimits(GAL_SHADER_TYPE type, GAL_SHADER_LIMITS* limits) = 0;

    /**
     * Creates a generic shader program that can be used as vertex/fragment/geometry shader program
     */
    virtual GALShaderProgram* createShaderProgram() const = 0;

    /**
     * Set the current geometry shader
     */
    virtual void setGeometryShader(GALShaderProgram* program) = 0;

    /**
     * Set the current vertex shader
     */
    virtual void setVertexShader(GALShaderProgram* program) = 0;

    /**
     * Set the current fragment shader
     */
    virtual void setFragmentShader(GALShaderProgram* program) = 0;

    /**
     * Set the vertex shader default value
     */
    virtual void setVertexDefaultValue(gal_float currentColor[4]) = 0;

    /**
     * Save a group of CG1 states
     *
     * @param siIds List of state identifiers to save
     * @returns The group of states saved
     */
    virtual GALStoredState* saveState(GALStoredItemIDList siIds) const = 0;

    /**
     * Save the whole set of CG1 states
     *
     * @returns The whole states group saved
     */
    virtual GALStoredState* saveAllState() const = 0;

    /**
     * Restores the value of a group of states
     *
     * @param state a previously saved state group to be restored
     */
    virtual void restoreState(const GALStoredState* state) = 0;

    /**
     * Releases a GALStoredState object
     *
     * @param state a previously saved state group to release
     */
    virtual void destroyState(GALStoredState* state) = 0;

    /**
     * Sets the starting frame from which to track current frame and batch.
     *
     * @param startFrame The starting frame.
     */
    virtual void setStartFrame(gal_uint startFrame) = 0;

    /**
     * Dumps the current device state (CG1 Library state)
     */
    virtual void DBG_dump(const gal_char* file, gal_enum flags) = 0;

    /**
     * Dumps the future device state when the specified frame and batch event 
     * occurs, and just before drawing the batch.
     */
    virtual void DBG_deferred_dump(const gal_char* file, gal_enum flags, gal_uint frame, gal_uint batch) = 0;

    /**
     * Saves a file with an image of the current CG1 library state
     */
    virtual gal_bool DBG_save(const gal_char* file) = 0;

    /**
     * Restores the CG1 state from a image file previously saved
     */
    virtual gal_bool DBG_load(const gal_char* file) = 0;

    /**
     * Set the alphaTest. ONLY for activing earlyZ, is compulsory to active alphaTest in the GALx!!!
     */
    virtual void alphaTestEnabled(gal_bool enabled) = 0;


    virtual GALRenderTarget* getFrontBufferRT() = 0;

    virtual GALRenderTarget* getBackBufferRT() = 0;

    virtual void copyMipmap (   GALTexture* inTexture, 
                            libGAL::GAL_CUBEMAP_FACE inFace, 
                            gal_uint inMipmap, 
                            gal_uint inX, 
                            gal_uint inY, 
                            gal_uint inWidth, 
                            gal_uint inHeight, 
                            GALTexture* outTexture, 
                            libGAL::GAL_CUBEMAP_FACE outFace, 
                            gal_uint outMipmap, 
                            gal_uint outX, 
                            gal_uint outY,
                            gal_uint outWidth,
                            gal_uint outHeight,
                            libGAL::GAL_TEXTURE_FILTER minFilter,
							libGAL::GAL_TEXTURE_FILTER magFilter) = 0;

    virtual void performBlitOperation2D(gal_uint sampler, gal_uint xoffset, gal_uint yoffset,
                                        gal_uint x, gal_uint y, gal_uint width, gal_uint height,
                                        gal_uint textureWidth, GAL_FORMAT internalFormat,
                                        GALTexture2D* texture, gal_uint level) = 0;

    /**
     * Compress a given texture into a given format
     *
     * @param originalFormat Format from the given texture
     * @param compressFormat Destiny format for the conversion
     * @param width Texture width
     * @param height Texture height
     * @param originalData Texture data pointer
     *
     * @return Compressed texture data into compressFormat
     */
    virtual gal_ubyte* compressTexture(GAL_FORMAT originalFormat, GAL_FORMAT compressFormat, gal_uint width, gal_uint height, gal_ubyte* originalData, gal_uint selectionMode) = 0;

    /**
     *  Obtain which frame is being executed by the GAL 
     *
     * @return Frame which is being executed by the GAL
     * 
     */
    virtual gal_uint getCurrentFrame() = 0;

    /**
     *  Obtain which batch is being executed by the GAL 
     *
     * @return Batch which is being executed by the GAL
     * 
     */
    virtual gal_uint getCurrentBatch() = 0;
         
};

} // namepace libGAL

#endif // GAL_DEVICE
