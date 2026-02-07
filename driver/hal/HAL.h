
#ifndef GPUDRIVER_H
#define GPUDRIVER_H

#include "GPUReg.h"
#include "MetaStream.h"
#include "cmMemorySpace.h"
#include <vector>
#include "RegisterWriteBuffer.h"
#include "ShaderProgramSched.h"

// HAL Driver for CG1GPU
//#define DUMP_SYNC_REGISTERS_TO_GPU

class HAL {

private:
    friend class RegisterWriteBuffer;
    RegisterWriteBuffer registerWriteBuffer; // Write buffer for registers
    ShaderProgramSched shSched;
    U64 batchCounter;
    // statistics counters
    U32 metaStreamGenerated;
    U32 memoryAllocations;
    U32 memoryDeallocations;
    U32 mdSearches;
    U32 addressSearches;
    U32 memPreloads;
    U32 memWrites;
    U32 memPreloadBytes;
    U32 memWriteBytes;

    bool preloadMemory;

    /**
     * Descriptor for a sequence of GPU memory blocks
     */
    struct _MemoryDescriptor
    {
        // both physical addresses in GPU
        U32 size; // size reclaimed with obtain memory...
        U32 firstAddress;
        U32 lastAddress;
        U32 memId;
        U64 lastBatchWritten; // used to support batch pipelining
        U32 highAddressWritten; // debug
    };

    /**
     * _MemoryDescriptor map with current _MemoryDescriptors managed by the HAL
     */
    std::map<U32, _MemoryDescriptor*> memoryDescriptors;
    


    /// Memory regions marked to be deleted, will be deleted when the next batch will finish
    /// This is done to support batch pipelining (avoids false locks)
    std::vector<U32> pendentReleases;

    /**
     * Finds the _MemoryDescriptor which have a Address in its range address
     *
     * @param physicalAddress the matching address
     *
     * @returns the _MemoryDescriptor that have 'physicalAddress' in its range
     */
    _MemoryDescriptor* _findMDByAddress( U32 physicalAddress );

    /**
     * Finds the _MemoryDescriptor which have memId equals to memId parameter
     *
     * @param memId memId Identifier for the _MemoryDescriptor we want
     *
     * @returns the _MemoryDescriptor searched
     */
    _MemoryDescriptor* _findMD( U32 memId );

    /**
     * Creates a new _MemoryDescriptor given a initial and final physical address
     *
     * The _MemoryDescritor is inserted in the _MemoryDescritor list
     *
     * @param firstAddress start memory address
     * @param lastAddress final memory address
     * @param size real reclaimed size by user of obtainMemory
     *
     * @returns a new _MemoryDescriptor initialized ( with memId assigned )
     */
    _MemoryDescriptor* _createMD( U32 firstAddress, U32 lastAddress, U32 size );

    /**
     * Release the memory pointed by memId descriptor
     *
     * @param memId a memory descriptor
     */
    void _releaseMD( U32 memId );


    /**
     *
     *  Search a memory map for empty blocks.
     *
     *  @param first Reference to a variable where to store the index to the first
     *  consecutive block found.
     *  @param map Pointer to the memory map where to search for blocks.
     *  @param memSize Size of the memory map (in KBytes).
     *  @param memRequired Amount of memory requested (in bytes).
     *
     *  @return The number of consecutive blocks found for the requested memory.
     *  Returns 0 if no consecutive blocks large enough are found.
     *
     */

    U32 searchBlocks(U32 &first, U08 *map, U32 memSize, U32 memRequired);

    /**
     * Encapsulates MetaStream dispatch
     *
     * @note this is the only function that should be modified if cgoMetaStream
     *       dispatches will be changed in the future
     *
     * @param agpt the cgoMetaStream to be sent
     */
    bool _sendcgoMetaStream( cg1gpu::cgoMetaStream* agpt );

    /**
     * Map for occupied memory blocks
     */
    U08* gpuMap;
    U08* systemMap;

    U32 lastBlock;   //  Stores the address of the last block tested for free in the memory allocation algorithm.  

    U32 gpuAllocBlocks;  //  Number of blocks currently allocated in GPU memory.  
    U32 systemAllocBlocks;   //  Number of blocks currently allocated in system memory.  

    /**
     * Used to generate memory descriptors
     */
    U32 nextMemId;

    /**
     * Maximum number of buffered cgoMetaStreams
     * Must be power of 2
     */
    enum { META_STREAM_BUFFER_SIZE = 2048 };

    /**
     * physical card memory ( in Kbytes ) (jnow is configurable)
     */
    //enum { physicalCardMemory = 16384 };

    /**
     * Minimum assignable size per obtainMemory command ( in Kbytes )
     */
    enum { BLOCK_SIZE = 4 };

    /**
     * Outstanding cgoMetaStreams
     */
    cg1gpu::cgoMetaStream** metaStreamBuffer;

    /**
     * Position available in the MetaStreamBufferQueue to store an cgoMetaStream
     */
    int in;

    /**
     * First item to be returned from the MetaStreamBufferQueue
     */
    int out;
    int         metaStreamCount;    // Count outstanding cgoMetaStreams
    static HAL* driver;     //* Driver pointer

    /**
     * Current resolution in GPU
     */
    U32 hRes, vRes;
    bool setResolutionCalled;           //  GPU resolution was set.  

    //
    //  GPU parameters
    //
    U32 gpuMemory;
    U32 systemMemory;
    U32 blocksz;
    U32 sblocksz;
    U32 scanWidth;
    U32 scanHeight;
    U32 overScanWidth;
    U32 overScanHeight;
    U32 fetchRate;
    bool doubleBuffer;
    bool forceMSAA;
    U32 forcedMSAASamples;
    bool forceFP16ColorBuffer;
    bool secondInterleavingEnabled;
    bool convertShaderProgramToLDA;             //  Stores if the conversion of input attribute reads to LDA instructions in shader programs is enabled.  
    bool convertShaderProgramToSOA;             //  Stores if the conversion of shader programs to Scalar format is enabled.  
    bool enableShaderProgramTransformations;    //  Stores if shader program transformation/translation is enabled.  
    bool microTrianglesAsFragments;             //  Stores if micro triangles are processed directly as fragments, which enables the microtriangle fragment shader transformation.  
    bool setGPUParametersCalled;                //  GPU parameters received from the simulator.  
    int batch;
    int frame;
   
    void* ctx;

    /**
     * Constructor performs driver setup automatically
     */
    HAL();

    static U08 _mortonTable[];
    void MortonTableBuilder();
    U32 _mortonFast(U32 size, U32 i, U32 j);
    U32 _texel2address(U32 width, U32 blockSz, U32 sBlockSz, U32 i, U32 j);
    F64 ceil(F64 x);
    F64 logTwo(F64 x);

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

    // Required information for the GPU Driver when performing the microtriangle transformation.
    struct MicroTriangleRasterSettings
    {
        bool zPerspective;
        bool smoothInterp[cg1gpu::MAX_FRAGMENT_ATTRIBUTES];
        bool perspectCorrectInterp[cg1gpu::MAX_FRAGMENT_ATTRIBUTES];
        bool faceCullEnabled;
        bool CWFaceCull;
        bool CCWFaceCull;
        unsigned int MSAA_samples;
    };

    /*
     * Default attributte input/output mapping
     * This translation table will be private soon
     *
     * From vertex shader to fragment shader: i -> outputAttrib[i]
     */
    static const S32 outputAttrib[cg1gpu::MAX_VERTEX_ATTRIBUTES];


    struct VertexAttribute
    {
        U32 stream; // stream identifier, from 0 to MAX_STREAM_BUFFERS 
        ShAttrib attrib; // atributte id 
        bool enabled; // enabled or disabled attribute 
        U32 md; // memory descriptor asociated to this VertexAttribute 
        U32 offset; // offset 
        U32 stride; // stride 
        cg1gpu::StreamData componentsType; // type of components 
        U32 components; // number of components per stream element 

        void dump() const;
    };


    /**
     * Returns the maximum number of vertex attribs
     */
    U32 getMaxVertexAttribs() const;

    /**
     * Returns the maximum number of vertex attribs supported by the GPU�s shader model.
     */
    U32 getMaxAddrRegisters() const;

    /**
     * Sets the GPU Registers that stores the current display resolution
     */
    void setResolution(U32 width, U32 height);

    /**
     * Gets the resolution specified in the trace
     */
    void getResolution(U32& width, U32& height) const;

    /**
     * Checks if setResolution has been called at least once
     */
    bool isResolutionDefined() const;

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
     *  @param doubleBuffers Uses double buffer (color buffer).
     *  @param forceMSAA Flag that forces multisampling for the whole trace.
     *  @param forcedMSAASamples Number of samples per pixel when multisampling is being forced.
     *  @param forceFP16ColorBuffer Force color buffer format to float16.
     *  @param fetchRate Instructions fetched per cycle and thread in the shader units.
     *  @param memoryControllerV2 Flag that enables the Memory Controller Version 2.
     *  @param secondInterleavingEnabled Flag that enables the two interleaving modes in the
     *  Memory Controller Version 2.
     *  @param convertToLDA Convert input attribute reads to LDA in shader programs.
     *  @param convertToSOA Convert shader programs to Scalar format.
     *  @param enableTransformations Enable shader program transformations/optimizations by the driver.
     *  @param microTrisAsFrags Microtriangles are processed directly as fragments, which enables the microtriangle fragment shader transformation.
     * 
     */

    void setGPUParameters(U32 gpuMemSz, U32 sysMemSz, U32 blocksz, U32 sblocksz,
        U32 scanWidth, U32 scanHeight, U32 overScanWidth, U32 overScanHeight,
        bool doubleBuffer, bool forceMSAA, U32 forcedMSAASamples, bool forceF16ColorBuffer,
        U32 fetchRate, bool memoryControllerV2, bool secondInterleavingEnabled,
        bool converToLDA, bool convertToSOA, bool enableTransformations, bool microTrisAsFrags
        );

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
     *  @param doubleBuffers Uses double buffer (color buffer).
     *  @param fetchRate Instructions fetched per cycle and thread in the shader units.
     *
     */

    void getGPUParameters(U32& gpuMemSz, U32& sysMemSz, U32& blocksz, U32& sblocksz,
        U32& scanWidth, U32& scanHeight, U32& overScanWidth, U32& overScanHeight,
        bool& doubleBuffer, U32& fetchRate) const;


    /**
     * Path for preloading data in GPU memory in 1 cycle
     * Used to implement GPU Hot Start Simulation
     *
     * @param enable true if the driver must preload data into gpu local memory, false means normal behaviour
     */
    void setPreloadMemory(bool enable);

    bool getPreloadMemory() const;

    /**
     *
     *  Returns the fetch instruction rate for the GPU shader units.
     *
     *  @return The instructions per cycle and thread fetched in the GPU shader units.
     */

    U32 getFetchRate() const;

    /**
     *
     *  Returns the parameters for texture tiling.
     *
     *  @param blockSz Reference to a variable where to store the first tile level size in 2^n x 2^n texels.
     *  @param sblockSz Reference to a variable where to stored the second tile level size in 2^m x 2^m texels.
     *
     */

    void getTextureTilingParameters(U32 &blockSz, U32 &sBlockSz) const;

    /**
     * Reserve memory to front/back buffers and zstencil buffer
     * and sets the address registers of this buffers to point to the reserved memory
     * After the call, the memory descriptors of the buffers are assigned to
     * the parameters, if non zero pointers are provided.
     */
    void initBuffers(U32* mdFront = 0, U32* mdBack = 0, U32* mdZS = 0);

    /**
     *
     *  Gets framebuffer parameters forced by the driver.
     *
     *  @param forceMSAA Reference to a boolean variable that will store if multisampling is forced by the driver.
     *  @param samples Reference to a variable that will store the samples per pixel forced by the driver.
     *  @param forceFP16 Reference to a boolean variable that will store if a FP16 color buffer is forced by the driver. 
     *
     */
     
    void getFrameBufferParameters(bool &forceMSAA, U32 &samples, bool &forceFP16);
    
    /**
     *
     *  Allocates space in GPU memory for a render buffer with the defined characteristics and returns a memory descriptor
     *
     *
     *  @param width Width in pixels of the requested render buffer.
     *  @param height Height in pixels of the requested render buffer.
     *  @param multisampling Defines if the render buffer supports multiple samples.
     *  @param samples Maximum number of samples to support in the render buffer if multisampling is enabled.
     *  @param format Format of the render buffer.
     *
     *  @return Returns the memory descriptor for the memory regions allocated for the created render buffer.
     *
     */
     
    U32 createRenderBuffer(U32 width, U32 height, bool multisampling, U32 samples, cg1gpu::TextureFormat format);

    /**
     *
     *  Converts the input surface data to the tiled format internally used in CG1 for render buffers.
     *  A byte array is allocated by the function to store the tiled data.
     *
     *  The surface and render buffer must have the same size.  The surface and render buffer must have
     *  the same format.
     *
     *  @param sourceData Pointer to a byte array storing the input surface data in row major order.
     *  @param width Width in pixels of the render buffer.
     *  @param height Height in pixels of the render buffer.
     *  @param multisampling Defines if the render buffer supports multiple samples.
     *  @param samples Number of samples to store per pixel in the render buffer.
     *  @param format Format of the render buffer.
     *  @param invertColors Invert colors.
     *  @param destData Reference to a pointer where to store the address of the allocated byte array
     *  with the tiled data for the render buffer.
     *  @param destSize Size of the render buffer data array (after applying tiling).
     *
     */
     
    void tileRenderBufferData(U08 *sourceData, U32 width, U32 height, bool multisampling, U32 samples,
                              cg1gpu::TextureFormat format, bool invertColors, U08* &destData, U32 &size);
     
    /**
     *
     *  Converts the input surface data to the mortonized tiled format required for CG1.
     *  The function allocates a byte array to store the mortonized tiled surface data and
     *  returns the pointer to that array.
     *
     *  @param originalData Pointer to a byte array with the surface data in a row-major order.
     *  @param width Width in texels of the input surface.
     *  @param height Height in texels of the input surface.
     *  @param depth Depth in texels of the input surface.
     *  @param format Format of the surface.
     *  @param texelSize Number of bytes per texel.
     *  @param mortonDataSize Reference to an integer variable where to return the size of 
     *  surface data after applying morton and tiling.
     *
     *  @return Pointer to a byte array with the surface data tiled morton order.
     *
     */
     
    U08* getDataInMortonOrder(U08* originalData, U32 width, U32 height, U32 depth,
                                cg1gpu::TextureCompression format, U32 texelSize, U32& mortonDataSize);

    /**
     * Returns the amount of local memory available in the simulator (in KBytes)
     */
    //U32 getGPUMemorySize() const;

    /**
     * Returns block and super block sizes
     */
   // U32 getGPUBlockSize() const;
    //U32 getGPUSuperBlockSize() const;

    /**
     * Opaque context pointer
     */
    void setContext(void* ctx) { this->ctx = ctx; }
    void* getContext() { return ctx; }


    bool configureVertexAttribute( const VertexAttribute& va );

    bool setVShaderOutputWritten(ShAttrib attrib, bool isWritten);

    bool setSequentialStreamingMode( U32 count, U32 start );

    /**
     * @param stream stream used for indices
     * @param count number of indexes
     * @param mIndices memory descriptor pointing to indices
     * @param offsetBytes offset (in bytes) within mdIndices
     * @param indicesType type of indices
     */
    bool setIndexedStreamingMode( U32 stream, U32 count, U32 start, U32 mdIndices, U32 offsetBytes, cg1gpu::StreamData indicesType );

    /**
     * Obtains the Singleton class which implements the GPU driver
     * @returns the unique instance of HAL
     */
    static HAL* getHAL();

    /**
     *
     *  Translates, optimizes or transforms the input shader program to the defined GPU architecture.
     *
     *  @param inCode Pointer to a buffer with the input shader program in binary format.
     *  @param inSize Size of the input shader program in binary format.
     *  @param outCode Pointer to a buffer where to store the transformed shader program in binary format.
     *  @param outSize Reference to a variable storing the size of the buffer where to store the
     *  transformed shader program.  The variable will be overwritten with the actual size of the transformed
     *  shader program.
     *  @param isVertexProgram Tells if the input shader program is a vertex shader program.  Used to activate
     *  the conversion of input attribute reads into load instructions.
     *  @param maxLiveTemRegs Reference to a variable where to store the maximum number of temporal registers
     *  alive at any point of the translated/optimized/transformed shader program.
     *  @param settings required API information to perform the microtriangle transformation.
     *
     */
     
    void translateShaderProgram(U08 *inCode, U32 inSize, U08 *outCoce, U32 &outSize, bool isVertexProgram,
                                U32 &maxLiveTempRegs, MicroTriangleRasterSettings settings);     

    /**
     *
     *  Assembles a shader program written in CG1 Shader Assembly.
     *
     *  @param program Pointer to a byte array with the shader program written in CG1 Shader Assembly.
     *  @param code Pointer to a byte array where to store the assembled program.
     *  @param size Size of the byte array where to store the assembled program.
     *
     *  @return Size in bytes of the assembled program.
     *
     */
          
    static U32 assembleShaderProgram(U08 *program, U08 *code, U32 size);

    /**
     *
     *  Disassembles a shader program to CG1 Shader Assembly.
     *
     *  @param code Pointer to a byte array with shader program byte code.
     *  @param codeSize Size of the encoded shader program in bytes.
     *  @param program Pointer to a byte array where to store the shader program in CG1 Shader Assembly.
     *  @param size Size of the byte array where to store the disassembled program.
     *  @param disableEarlyZ Reference to a boolean variable where to store if the shader program requires
     *  early z to be disabled.
     *
     *  @return Size in bytes of the disassembled program.
     *
     */
          
    static U32 disassembleShaderProgram(U08 *code, U32 codeSize, U08 *program, U32 size, bool &disableEarlyZ);
     
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
    bool commitVertexProgram( U32 memDesc, U32 programSize, U32 startPC );

    /**
     * Same that commitVertexProgram for fragments
     */
    bool commitFragmentProgram(U32 memDesc, U32 programSize, U32 startPC );

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
    U32 obtainMemory( U32 sizeBytes, MemoryRequestPolicy memRequestPolicy = GPUMemoryFirst);


    /**
     * Deletes memory pointed by 'md' descriptor
     * mark memory associated with memory descriptor as free
     *
     * @param md memory descriptor pointing the memory we want to release
     */
    void releaseMemory( U32 md );

    /**
     * Writes data in an specific portion of memory described by a memory descriptor
     *
     * @param md memory descriptor representing a portion of memory previously reserved
     * @param data data buffer we want to write in local memory
     * @param dataSize data size in bytes
     *
     * @return true if the writting was succesfull, false otherwise
     */
    bool writeMemory( U32 md, const U08* data, U32 dataSize, bool isLocked = false);

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
    bool writeMemory( U32 md, U32 offset, const U08* data, U32 dataSize, bool isLocked = false );

    //bool writeMemoryDebug(U32 md, U32 offset, const U08* data, U32 dataSize, const std::string& debugInfo);

    /**
     *
     * Preloads data in an specific portion of memory described by a memory descriptor
     *
     * @param md memory descriptor representing a portion of memory previously reserved
     * @param offset logical offset added to the initial address of this memory space
     *        before writting
     * @param data data buffer we want to preload in local memory
     * @param dataSize data size in bytes
     *
     * @return true if the preload was succesful, false otherwise
     *
     */
    bool writeMemoryPreload(U32 md, U32 offset, const U08* data, U32 dataSize);

    /**
     * Writes a GPU simulator's register
     *
     * @param regId register's identifier constant
     * @param value new register value
     * @param md Memory descriptor associated with this register write.
     *
     */
    void writeGPURegister( cg1gpu::GPURegister regId, cg1gpu::GPURegData data, U32 md = 0 );

    /**
     * Writes a GPU simulator's register
     *
     * @param regId register's identifier constant
     * @param index subRegister indetifier
     * @param data new register value
     * @param md Memory descriptor associated with this register write.
     *
     */
    void writeGPURegister( cg1gpu::GPURegister regId, U32 index, cg1gpu::GPURegData data, U32 md = 0);

    /**
     * Writes a GPU simulator's register with the start address contained in the internal
     * MemoryDescriptor aliased to 'md' handler + the offset (expressed in bytes)
     */
    void writeGPUAddrRegister( cg1gpu::GPURegister regId, U32 index, U32 md, U32 offset = 0);

    /**
     *
     *  Reads the GPU simulator register.
     *
     *  @param regID GPU register identifier.
     *  @param data Reference to a variable where to store value read from the register.
     *
     */
     
    void readGPURegister(cg1gpu::GPURegister regID, cg1gpu::GPURegData &data);

    /**
     *
     *  Reads the GPU simulator register.
     *
     *  @param regID GPU register identifier.
     *  @param index GPU register subregister identifier.
     *  @param data Reference to a variable where to store value read from the register.
     *
     */
     
    void readGPURegister(cg1gpu::GPURegister regID, U32 index, cg1gpu::GPURegData &data);
        
    /**
     * Sends a command to the GPU Simulator
     *
     * @param com command to be sent
     */
    void sendCommand( cg1gpu::GPUCommand com );

    /**
     * Sends command list to the GPU Simulator
     *
     * @param listOfCommands commands to be sent
     * @param numberOfCommands quantity of commands in the list
     */
     
    void sendCommand( cg1gpu::GPUCommand* listOfCommands, int numberOfCommands );

    /**
     *
     *  Signals an event to the GPU simulator.
     *
     *  @param gpuEvent The GPU event to signal to the simulator.
     *  @param eventMsg A message string for the event signaled.
     *
     */
     
    void signalEvent(cg1gpu::GPUEvent gpuEvent, std::string eventMsg);
         
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
     * Implements elemental comunication between Driver and CGD Port
     */
    cg1gpu::cgoMetaStream* nextMetaStream();

    /**
     * Destructor
     */
    ~HAL();

    /////////////////////
    // Debug functions //
    /////////////////////
    void printMemoryUsage();
    void dumpMemoryAllocation( bool contents = true  );
    void dumpMD( U32 md );
    void dumpMDs();
    void dumpMetaStreamBuffer();
    void dumpStatistics();
    void dumpRegisterStatus (int frame, int batch);

};

#endif
