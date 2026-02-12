#ifndef CDL_DRIVER
	#define CDL_DRIVER

#include "GPUReg.h"

class CDLDriver
{
public:

    /**
     * Shaders attribs
     */
    enum ShAttrib
    {
        VS_VERTEX = 0,
        VS_WEIGHT = 1,
        VS_NORMAL = 2,
        VS_COLOR = 3,
        VS_SEC_COLOR = 4,
        VS_FOG = 5,
        VS_SEC_WEIGHT = 6,
        VS_THIRD_WEIGHT = 7,
        VS_TEXTURE_0 = 8,
        VS_TEXTURE_1 = 9,
        VS_TEXTURE_2 = 10,
        VS_TEXTURE_3 = 11,
        VS_TEXTURE_4 = 12,
        VS_TEXTURE_5 = 13,
        VS_TEXTURE_6 = 14,
        VS_TEXTURE_7 = 15,
        VS_NOOUTPUT = -1 // Defines an attribute which is not passed from VSh to FSh
    };

	enum MemoryRequestPolicy
	{
		GPUMemoryFirst, // searches for free memory first in GPU local memory and if it fails tries in system memory
		SystemMemoryFirst // searches for free memory first in system memory and if it fails tries in gpu local memory
	};

    /*
     * Default attributte input/output mapping
     * This translation table will be private soon
     *
     * From vertex shader to fragment shader: i -> outputAttrib[i]
     */
	static const S32 outputAttrib[arch::MAX_VERTEX_ATTRIBUTES];

    struct VertexAttribute
    {
        U32 stream; // stream identifier, from 0 to MAX_STREAM_BUFFERS 
        ShAttrib attrib; // atributte id 
        bool enabled; // enabled or disabled attribute 
        U32 md; // memory descriptor asociated to this VertexAttribute 
        U32 offset; // offset 
        U32 stride; // stride 
		arch::StreamData componentsType; // type of components 
        U32 components; // number of components per stream element 
    };


    /**
     * Returns the maximum number of vertex attribs
     */
	virtual U32 getMaxVertexAttribs() const = 0;

    /**
     * Returns the maximum number of vertex attribs supported by the GPU�s shader model.
     */
	virtual U32 getMaxAddrRegisters() const = 0;

    /**
     * Sets the GPU Registers that stores the current display resolution
     */
	virtual void setResolution(U32 width, U32 height) = 0;

    /**
     * Gets the resolution specified in the trace
     */
	virtual void getResolution(U32& width, U32& height) const = 0;

    /**
     * Checks if setResolution has been called at least once
     */
	virtual bool isResolutionDefined() const = 0;

    /**
     *
     *  Sets the configuration parameters for the GPU.
     *
     *  This method is called by simulator. This method does not set anything in the simulator.
     *
     *  @param gpuMemSz GPU memory size in KBytes.
     *  @param sysMemSz System memory size in KBytes.
     *  @param blocksz Texture first tile level (block) size (as 2^blocksz x 2^blocksz).
     *  @param sblocksz Texture second tile level (superblock) size (as 2^blocksz x 2^blocksz).
     *  @param scanWidth Rasterizer scan tile height (in fragments).
     *  @param scanHeight Rasterizer scan tile width (in fragments).
     *  @param overScanWidth Rasterizer over scan tile height (in scan tiles).
     *  @param overScanHeight Rasterizer over scan tile widht (in scan tiles).
     *  @param colorWriteBytesPixel Bytes per pixel in the color buffer.
     *  @param zstencilBytesPixel Bytes per pixel in the Z/Stencil buffer.
     *  @param doubleBuffers Uses double buffer (color buffer).
     *  @param fetchRate Instructions fetched per cycle and thread in the shader units.
     *
     */
    virtual void setGPUParameters(U32 gpuMemSz, U32 sysMemSz, U32 blocksz, U32 sblocksz,
        U32 scanWidth, U32 scanHeight, U32 overScanWidth, U32 overScanHeight,
        U32 colorWriteBytesPixel, U32 zstencilBytesPixel, bool doubleBuffer, U32 fetchRate,
		bool memoryControllerV2,
		bool secondInterleavingEnabled
		) = 0;

    /**
     *
     *  Stores the GPU configuration parameters in the variables passed as references.
     *
     *  @param gpuMemSz GPU memory size in KBytes.
     *  @param sysMemSz System memory size in KBytes.
     *  @param blocksz Texture first tile level (block) size (as 2^blocksz x 2^blocksz).
     *  @param sblocksz Texture second tile level (superblock) size (as 2^blocksz x 2^blocksz).
     *  @param scanWidth Rasterizer scan tile height (in fragments).
     *  @param scanHeight Rasterizer scan tile width (in fragments).
     *  @param overScanWidth Rasterizer over scan tile height (in scan tiles).
     *  @param overScanHeight Rasterizer over scan tile widht (in scan tiles).
     *  @param colorWriteBytesPixel Bytes per pixel in the color buffer.
     *  @param zstencilBytesPixel Bytes per pixel in the Z/Stencil buffer.
     *  @param doubleBuffers Uses double buffer (color buffer).
     *  @param fetchRate Instructions fetched per cycle and thread in the shader units.
     *
     */
    virtual void getGPUParameters(U32& gpuMemSz, U32& sysMemSz, U32& blocksz, U32& sblocksz,
        U32& scanWidth, U32& scanHeight, U32& overScanWidth, U32& overScanHeight,
        U32& colorWriteBytesPixel, U32& zstencilBytesPixel, bool& doubleBuffer, U32& fetchRate) const = 0;


    /**
     * Path for preloading data in GPU memory in 1 cycle
     * Used to implement GPU Hot Start Simulation
     *
     * @param enable true if the driver must preload data into gpu local memory, false means normal behaviour
     */
    virtual void setPreloadMemory(bool enable) = 0;
    
    virtual bool getPreloadMemory() const = 0;

    /**
     *
     *  Returns the fetch instruction rate for the GPU shader units.
     *
     *  @return The instructions per cycle and thread fetched in the GPU shader units.
     */

    virtual U32 getFetchRate() const = 0;

    /**
     *
     *  Returns the parameters for texture tiling.
     *
     *  @param blockSz Reference to a variable where to store the first tile level size in 2^n x 2^n texels.
     *  @param sblockSz Reference to a variable where to stored the second tile level size in 2^m x 2^m texels.
     *
     */

    virtual void getTextureTilingParameters(U32 &blockSz, U32 &sBlockSz) const = 0;

    /**
     * Reserve memory to front/back buffers and zstencil buffer
     * and sets the address registers of this buffers to point to the reserved memory
     */
    virtual void initBuffers() = 0;

    /**
     * Opaque context pointer
     */
    virtual void setContext(void* ctx) = 0;


    virtual void* getContext() = 0;

    virtual bool configureVertexAttribute( const VertexAttribute& va ) = 0;

    virtual bool setVShaderOutputWritten(ShAttrib attrib, bool isWritten) = 0;

    virtual bool setSequentialStreamingMode( U32 count, U32 start ) = 0;

    /**
     * @param stream stream used for indices
     * @param count number of indexes
     * @param mIndices memory descriptor pointing to indices
     * @param offsetBytes offset (in bytes) within mdIndices
     * @param indicesType type of indices
     */
	virtual bool setIndexedStreamingMode( U32 stream, U32 count, U32 start, U32 mdIndices, 
										  U32 offsetBytes, arch::StreamData indicesType ) = 0;


    /**
     * performs initialization for a vertex program
     *
     * initialize the vertex program identified by 'memDesc'
     * Load the program in vertex shader instruction memory, that is, make the program
     * the current vertex program
     *
     * @param memDesc memory descriptor pointing to a vertex program data
     * @param programSize program's size
     * @param startPC first instruction that will be executed for this vertex program
     *
     * @return true if all goes well, false otherwise
     *
     * @code
     *     // ex. Commit a vertex program ( make it current )
     *
     *     // get the singleton HAL
     *     HAL* driver = HAL::getHAL();
     *
     *     // vertex program in binary format
     *     static const U08 vprogram[] = { 0x09, 0x00, 0x02, 0x00, 0x07, ... , 0x00 };
     *
     *     // assuption : program it is not still in local memory, transfer must be performed
     *     U32 mdVP = driver->obtainMemory( sizeof(vprogram) );
     *     driver->writeMemory( mdVP, vprogram, sizeof(vprogram) ); // copy program to local memory
     *
     *     // load vertex program to vertex shader instruction memory and set PCinstr to 0
     *     driver->commitVertexProgram( mdVP, sizeof(vprogram), 0 );
     *
     * @endcode
     */
    virtual bool commitVertexProgram( U32 memDesc, U32 programSize, U32 startPC ) = 0;

    /**
     * Same that commitVertexProgram for fragments
     */
    virtual bool commitFragmentProgram( U32 memDesc, U32 programSize, U32 startPC ) = 0;

    /**
     * Reserves an amount of data of at least 'sizeBytes' bytes from local GPU memory
     *
     * @param sizeBytes amount of data required
     * @param memorySource source from where to get free memory
     * @return memory descriptor refering the memory just reserved
     *
     * @note now you can write in this memory calling HAL::writeMemory() methods.
     *       Use the descriptor returned as first parameter in HAL::writeMemory()
     */
    virtual U32 obtainMemory( U32 sizeBytes, MemoryRequestPolicy memRequestPolicy = GPUMemoryFirst) = 0;
    

    /**
     * Deletes memory pointed by 'md' descriptor
     * mark memory associated with memory descriptor as free
     *
     * @param md memory descriptor pointing the memory we want to release
     */
    virtual void releaseMemory( U32 md ) = 0;

    /**
     * Writes data in an specific portion of memory described by a memory descriptor
     *
     * @param md memory descriptor representing a portion of memory previously reserved
     * @param data data buffer we want to write in local memory
     * @param dataSize data size in bytes
     *
     * @return true if the writting was succesfull, false otherwise
     */
    virtual bool writeMemory( U32 md, const U08* data, U32 dataSize, bool isLocked = false) = 0;

    /**
     * Writes data in an specific portion of memory described by a memory descriptor
     *
     * @param md memory descriptor representing a portion of memory previously reserved
     * @param offset logical offset added to the initial address of this memory space
     *        before writting
     * @param data data buffer we want to write in local memory
     * @param dataSize data size in bytes
     *
     * @return true if the writting was succesfull, false otherwise
     *
     * @code
     *     // using writeMemory() offset version to avoid multiple obtainMemory calls
     *
     *     HAL driver = HAL::getHAL();
     *
     *     // ...
     *
     *     U08 vertexs[] = { ... }; // 4 componets per vertex
     *     U08 colors[]  = { ... }; // 4 components per color
     *     U08 normals[] = { ... }; // 3 components per normal
     *
     *     U32 oVertex = 0; // offset for stream of vertex
     *     U32 oColors = 4*sizeof(vertexs); // offset for stream of colors
     *     U32 oNormals = 4*sizeof(colors); // offset for stream of normals
     *     U32 totalSize =  oColors + oNormals + 3*sizeof(normals);
     *
     *     // only one obtain call
     *     U32 md = driver->obtainMemory( totalSize );
     *
     *     driver->writeMemory( md, oVertex, vertexs, sizeof(vertexs) );
     *     driver->writeMemory( md, oColors, colors, sizeof(colors) );
     *     driver->writeMemory( md, oNormals, normals, sizeof(normals) );
     */
    virtual bool writeMemory( U32 md, U32 offset, const U08* data, 
							  U32 dataSize, bool isLocked = false ) = 0;

    /**
     * Writes a GPU simulator's register
     *
     * @param regId register's identifier constant
     * @param value new register value
     * @param md Memory descriptor associated with this register write.
     *
     */
	void writeGPURegister( arch::GPURegister regId, arch::GPURegData data, U32 md = 0 );

    /**
     * Writes a GPU simulator's register
     *
     * @param regId register's identifier constant
     * @param index subRegister indetifier
     * @param data new register value
     * @param md Memory descriptor associated with this register write.
     *
     */
	void writeGPURegister( arch::GPURegister regId, U32 index, arch::GPURegData data, U32 md = 0);

    /**
     * Writes a GPU simulator's register with the start address contained in the internal
     * MemoryDescriptor aliased to 'md' handler + the offset (expressed in bytes)
     */
	void writeGPUAddrRegister( arch::GPURegister regId, U32 index, U32 md, U32 offset = 0);

    /**
     * Sends a command to the GPU Simulator
     *
     * @param com command to be sent
     */
	void sendCommand( arch::GPUCommand com );

    /**
     * Returns the number of mipmaps supported for a texture
     */
    U32 getMaxMipmaps() const;

    /**
     * Returns the number of texture units available
     */
    U32 getTextureUnits() const;

	/**
	 * Invoked by Command Processor
	 * Implements elemental comunication between Driver and MetaStream Port
	 */	
	arch::cgoMetaStream* nextcgoMetaStream();


    /////////////////////
    // Debug functions //
    /////////////////////
    void printMemoryUsage();
    void dumpMemoryAllocation( bool contents = true  );
    void dumpMD( U32 md );
    void dumpMDs();
    void dumpMetaStreamBuffer();
    void dumpStatistics();

};


#endif // CDL_DRIVER
