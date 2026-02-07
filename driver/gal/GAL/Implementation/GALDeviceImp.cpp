/**************************************************************************
 *
 */

#include "GALDeviceImp.h"
#include "GALBlendingStageImp.h"
#include "GALRasterizationStageImp.h"
#include "GALZStencilStageImp.h"
#include "GALRenderTargetImp.h"
#include "GALShaderProgramImp.h"
#include "GALStoredStateImp.h"

#include "GALShaderProgramImp.h"
#include "GALBufferImp.h"
#include "GALStreamImp.h"
#include "GALSamplerImp.h"

#include "GALMath.h"
#include "GALMacros.h"
#include "TextureMipmap.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#include "GALTexture2DImp.h"
#include "GALTextureCubeMapImp.h"
#include "TextureAdapter.h"

#include "GlobalProfiler.h"

using namespace std;

using namespace libGAL;

gal_ubyte GALDeviceImp::defaultVSh[] = "mov o0, i0\n"
                                       "mov o1, i1\n";

gal_ubyte GALDeviceImp::defaultFSh[] = "mov o1, i1\n";

GALDeviceImp::GALDeviceImp(HAL* driver) :
    _driver(driver),
    _requiredSync(true),
    _MAX_STREAMS(cg1gpu::MAX_STREAM_BUFFERS - 1),
    _MAX_SAMPLES(16),
    //_MAX_STREAMS(3),
    _indexedMode(false),
    _clearColorBuffer(0),
    _primitive(GAL_POINTS),
    _vaMap(cg1gpu::MAX_VERTEX_ATTRIBUTES, cg1gpu::ST_INACTIVE_ATTRIBUTE),
    _gsh(0), _gpuMemGshTrack(0),
    _vsh(0), _gpuMemVshTrack(0),
    _fsh(0), _gpuMemFshTrack(0),
    _vshOutputs(cg1gpu::MAX_VERTEX_ATTRIBUTES, false),
    _fshInputs(cg1gpu::MAX_FRAGMENT_ATTRIBUTES, false),
    _vshResources(1),
    _fshResources(1),
    _shOptimizer(libGAL_opt::SHADER_ARCH_PARAMS()),
    //_stream(cg1gpu::MAX_STREAM_BUFFERS - 1),
    _stream(_MAX_STREAMS),
    _zClearValue(1.0f),
    _stencilClearValue(0x00ffffff),
    _hzActivated(false),
    _hzBufferValid(false),
    _defaultBackBuffer(0),
    _defaultFrontBuffer(0),
    _defaultZStencilBuffer(0),
    _currentZStencilBuffer(0),
    _defaultRenderBuffers(false),
    _defaultZStencilBufferInUse(false),
    _colorSRGBWrite(false),
    _startFrame(0),
    _currentFrame(0),
    _currentBatch(0),
    _alphaTest(false),
    _streamStart(0),
    _streamCount(0),
    _streamInstances(1),
    _earlyZ(true),
    _currentColor(gal_float(0.0))
{
    _driver->setContext(this);

    _moa = new MemoryObjectAllocator(driver);

    gal_uint tileLevel1Sz;
    gal_uint tileLevel2Sz;
    _driver->getTextureTilingParameters(tileLevel1Sz, tileLevel2Sz);

    // configure the texture tiling used in texture mipmaps
    TextureMipmap::setTextureTiling(tileLevel1Sz, tileLevel2Sz);
    TextureMipmap::setDriver(_driver);

    // Create stream objects
    for ( gal_uint i = 0; i < _MAX_STREAMS; ++i )
        _stream[i] = new GALStreamImp(this, _driver, i);

    // The last available stream in CG1 is reserved to stream indices when indexedMode is enabled
    _indexStream = new GALStreamImp(this, _driver, _MAX_STREAMS);

    // Init register with the stream of indices
    cg1gpu::GPURegData data;
    data.uintVal = _MAX_STREAMS;
    _driver->writeGPURegister(cg1gpu::GPU_INDEX_STREAM, 0, data);

    _sampler = new GALSamplerImp*[cg1gpu::MAX_TEXTURES];
    for ( gal_uint i = 0; i < cg1gpu::MAX_TEXTURES; ++i )
        _sampler[i] = new GALSamplerImp(this, _driver, i);

    // Create rasterization stage object
    _rast = new GALRasterizationStageImp(this, _driver);

    // Create ZStencil stage object
    _zStencil = new GALZStencilStageImp(this, _driver);

    // Create Blending stage object
    _blending = new GALBlendingStageImp(this, _driver);

    _syncRegisters();

    // Start the deferred dump structures
    _nextDumpEvent.valid = false;

    data.uintVal = _MAX_STREAMS;
    _driver->writeGPURegister(cg1gpu::GPU_INDEX_STREAM, 0, data);


    _defaultVshProgram = (GALShaderProgramImp*)(createShaderProgram());
    _defaultFshProgram = (GALShaderProgramImp*)(createShaderProgram());

    _defaultVshProgram->setProgram(defaultVSh);
    _defaultFshProgram->setProgram(defaultFSh);

    for ( gal_uint i = 0; i < _vshOutputs.size(); ++i )
        _defaultVshProgram->setOutputWritten(i, false);

    for ( gal_uint i = 0; i < _fshInputs.size(); ++i )
        _defaultFshProgram->setInputRead(i, false);

    _defaultVshProgram->setOutputWritten(cg1gpu::POSITION_ATTRIBUTE, true);
    _defaultVshProgram->setOutputWritten(3, true);

    _defaultVshProgram->setInputRead(cg1gpu::POSITION_ATTRIBUTE, true);
    _defaultVshProgram->setInputRead(cg1gpu::COLOR_ATTRIBUTE, true);

    _vsh = _defaultVshProgram;
    _fsh = _defaultFshProgram;

    _syncVertexShader();
    _syncFragmentShader();

    zValuePartialClear = 0;

    //  Allocate buffer with the quad positions.
    gal_float vertexPositions[4 * 4] =
    {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

        
    vertexBufferPartialClear = createBuffer(4*16, (gal_ubyte*)vertexPositions);

    for (gal_uint i = 0; i < GAL_MAX_RENDER_TARGETS; i++)
        _currentRenderTarget[i] = NULL;
}

GALDeviceImp::~GALDeviceImp()
{
 
}

void GALDeviceImp::setOptions(const GAL_CONFIG_OPTIONS& configOptions)
{
    CG_ASSERT("Method not implemented yet");
}

void GALDeviceImp::_syncRegisters()
{
    cg1gpu::GPURegData data;

    if ( _indexedMode )
        data.uintVal = 1;
    else
        data.uintVal = 0;
    _driver->writeGPURegister(cg1gpu::GPU_INDEX_MODE, 0, data); // Init INDEX MODE

    _translatePrimitive(_primitive, &data);
    _driver->writeGPURegister(cg1gpu::GPU_PRIMITIVE, data); // Init primitive topology

    for ( gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; ++i ) {
        data.uintVal = _vaMap[i];
        _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_MAP, i, data);
    }

    data.qfVal[0] =  float(_clearColorBuffer         & 0xff) / 255.0f;
    data.qfVal[1] =  float((_clearColorBuffer >> 8)  & 0xff) / 255.0f;
    data.qfVal[2] =  float((_clearColorBuffer >> 16) & 0xff) / 255.0f;
    data.qfVal[3] =  float((_clearColorBuffer >> 24) & 0xff) / 255.0f;
    _driver->writeGPURegister(cg1gpu::GPU_COLOR_BUFFER_CLEAR, data);

    data.uintVal = static_cast<gal_uint>(static_cast<gal_float>(_zClearValue) * static_cast<gal_float>((1 << 24) - 1));
    _driver->writeGPURegister(cg1gpu::GPU_Z_BUFFER_CLEAR, data);

    data.intVal = _stencilClearValue;
    _driver->writeGPURegister(cg1gpu::GPU_STENCIL_BUFFER_CLEAR, data);

    data.booleanVal = _hzActivated;
    //_driver->writeGPURegister(cg1gpu::GPU_HIERARCHICALZ, data);

    cg1gpu::GPURegData bValue;

    for ( gal_uint i = 0; i < _vshOutputs.size(); ++i ) {
        bValue.booleanVal = _vshOutputs[i];
        _driver->writeGPURegister(cg1gpu::GPU_VERTEX_OUTPUT_ATTRIBUTE, i, bValue);
        _vshOutputs[i].restart();
    }

    for ( gal_uint i = 0; i < _fshInputs.size(); ++i ) {
        bValue.booleanVal = _fshInputs[i];
        _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_INPUT_ATTRIBUTES, i, bValue);
        _fshInputs[i].restart();
    }

    /*
    cout << "CDDeviceImp::_syncRegisters() -> "
            "Temporary initializing VERTEX_THREAD_RESOURCES & FRAGMENT_THREAD_RESOURCES statically to 5"  << endl;
    data.uintVal = 5;
    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_THREAD_RESOURCES, data);
    _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_THREAD_RESOURCES, data);
    */

    data.uintVal = _vshResources;
    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_THREAD_RESOURCES, data);

    data.uintVal = _fshResources;
    _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_THREAD_RESOURCES, data);

    // Note: If more registers must be initialized by the device, put the initialization code here
}

MemoryObjectAllocator& GALDeviceImp::allocator() const
{
    return *_moa;
}

HAL& GALDeviceImp::driver() const
{
    return *_driver;
}

void GALDeviceImp::setPrimitive(GAL_PRIMITIVE primitive)
{
    _primitive = primitive;
}

GALBlendingStage& GALDeviceImp::blending()
{
    return *_blending;
}

GALRasterizationStage& GALDeviceImp::rast()
{
    return *_rast;
}

GALZStencilStage& GALDeviceImp::zStencil()
{
    return *_zStencil;
}

GALTexture2D* GALDeviceImp::createTexture2D()
{
    return new GALTexture2DImp();
}

GALTexture3D* GALDeviceImp::createTexture3D()
{
    return new GALTexture3DImp();
}

GALTextureCubeMap* GALDeviceImp::createTextureCubeMap()
{
    return new GALTextureCubeMapImp();
}

GALBuffer* GALDeviceImp::createBuffer(gal_uint size, const gal_ubyte* data)
{
    return new GALBufferImp(size, data);
}

gal_bool GALDeviceImp::destroy(GALResource* resourcePtr)
{
    switch ( resourcePtr->getType() ) {
        case GAL_RESOURCE_BUFFER:
            {
                GALBufferImp* bufImp = static_cast<GALBufferImp*>(resourcePtr);
                _moa->deallocate(bufImp); // Deallocate from GPU memory if required
                //cout << "I'm going to delete " << bufImp << "\n";
                delete bufImp; // Release buffer interface
                return true;
            }
            break;
        default:
            CG_ASSERT("Destroy is only implemented for buffer resources only [sorry for the inconvenience]");
    }
    return false;
}

gal_bool GALDeviceImp::destroy(GALShaderProgram* shProgram)
{
    if ( shProgram == 0 )
        CG_ASSERT("Trying to destroy a NULL shader program interface");

    if ( shProgram == _vsh )
        CG_ASSERT("Destroying a shader program used as the currently bound vertex program");
    else if ( shProgram == _fsh )
        CG_ASSERT("Destroying a shader program used as the currently bound fragment program");
    else if ( shProgram == _gsh )
        CG_ASSERT("Destroying a shader program used as the currently bound geometry program");

    GALShaderProgramImp* shProgramImp = static_cast<GALShaderProgramImp*>(shProgram);

    _moa->deallocate(shProgramImp); // Deallocate from GPU memory if required

    delete shProgramImp; // Destroy the interface

    return true;
}



void GALDeviceImp::setResolution(gal_uint width, gal_uint height)
{
    /*if ( _driver->isResolutionDefined() )
        CG_ASSERT("Resolution is already defined");*/

    _driver->setResolution(width, height);

    gal_uint mdFront, mdBack, mdZStencil;

    _driver->initBuffers(&mdFront, &mdBack, &mdZStencil);

    bool multisampling;
    gal_uint samples;
    bool fp16color;

    _driver->getFrameBufferParameters(multisampling, samples, fp16color);

    GAL_FORMAT format;

    if (fp16color)
        format = GAL_FORMAT_RGBA16F;
    else
        format = GAL_FORMAT_ARGB_8888;

    //  Create the render targets for the default render buffers.
    _defaultFrontBuffer = new GALRenderTargetImp(this, width, height, multisampling, samples, format, true, mdFront);
    _defaultBackBuffer = new GALRenderTargetImp(this, width, height, multisampling, samples, format, true, mdBack);
    _defaultZStencilBuffer = new GALRenderTargetImp(this, width, height, multisampling, samples, GAL_FORMAT_S8D24, true, mdZStencil);

    //  Set the default back buffer and z stencil buffer as the current render targets.
    _currentRenderTarget[0] = _defaultBackBuffer;
    _currentZStencilBuffer = _defaultZStencilBuffer;

    _currentRenderTarget[0].restart();
    _currentZStencilBuffer.restart();

    //  Set as using the default render buffers.
    _defaultRenderBuffers = true;
    _defaultZStencilBufferInUse = true;

    //  Create a buffer for saving the color block state data on render target switch.
    //
    //  NOTE:  Current implementation uses a fixed size, this should change to take into
    //         account the resolution and the GPU parameters.
    //
    _mdColorBufferSavedState = _driver->obtainMemory(512 * 1024);

    //  Set the gpu register that points to the color state buffer save area.
    _driver->writeGPUAddrRegister(cg1gpu::GPU_COLOR_STATE_BUFFER_MEM_ADDR, 0, _mdColorBufferSavedState);

    //  Create a buffer for saving the z stencil block state data on z stencil buffer switch.
    //
    //  NOTE:  Current implementation uses a fixed size, this should change to take into
    //         account the resolution and the GPU parameters.
    //
    _mdZStencilBufferSavedState = _driver->obtainMemory(512 * 1024);

    //  Set the gpu register that points to the z stencil state buffer save area.
    _driver->writeGPUAddrRegister(cg1gpu::GPU_ZSTENCIL_STATE_BUFFER_MEM_ADDR, 0, _mdZStencilBufferSavedState);

    //  Set the z stencil buffer as defined.
    _zStencil->setZStencilBufferDefined(true);
}

gal_bool GALDeviceImp::getResolution(gal_uint& width, gal_uint& height)
{
    _driver->getResolution(width, height);
    return _driver->isResolutionDefined();
}

GALStream& GALDeviceImp::stream(gal_uint streamID)
{
    return *_stream[streamID];
}


void GALDeviceImp::enableVertexAttribute(gal_uint vaIndex, gal_uint streamID)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( vaIndex >= cg1gpu::MAX_VERTEX_ATTRIBUTES )
            CG_ASSERT("Vertex attribute index too high");

        if ( _usedStreams.count(streamID) != 0 ) {
            stringstream ss;
            ss << "Stream " << vaIndex << " is already enabled with another vertex attribute";
            CG_ASSERT(ss.str().c_str());
        }
    )

    _vaMap[vaIndex] = streamID;
    _usedStreams.insert(streamID);
    GLOBAL_PROFILER_EXIT_REGION()    
}

void GALDeviceImp::disableVertexAttribute(gal_uint vaIndex)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( vaIndex >= cg1gpu::MAX_VERTEX_ATTRIBUTES )
            CG_ASSERT("Vertex attribute index too high");
    )

    gal_uint stream = _vaMap[vaIndex];

    if ( stream != cg1gpu::ST_INACTIVE_ATTRIBUTE )  { // Mark this stream as free
        _usedStreams.erase(stream);
        _vaMap[vaIndex] = cg1gpu::ST_INACTIVE_ATTRIBUTE;
    }
    GLOBAL_PROFILER_EXIT_REGION()    
}

void GALDeviceImp::disableVertexAttributes()
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _usedStreams.clear();
    //_vaMap.assign(cg1gpu::MAX_VERTEX_ATTRIBUTES, cg1gpu::ST_INACTIVE_ATTRIBUTE);
    for(U32 a = 0; a < _vaMap.size(); a++)
        _vaMap[a] = cg1gpu::ST_INACTIVE_ATTRIBUTE;
    GLOBAL_PROFILER_EXIT_REGION()    
}

void GALDeviceImp::_syncStreamerState()
{

#ifdef GAL_DUMP_STREAMS
    gal_ubyte filename[30];
    sprintf((char*)filename, "GALDumpStream.txt");
    remove((char*)filename);
#endif

    // Create a sorted map
    cg1gpu::GPURegData data;
    map<gal_uint,gal_uint> m;
    for ( gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++ ) {
        m.insert(make_pair(i, _vaMap[i]));
    }

    // Use map to sort by stream unit (to match all legacy code)
    map<gal_uint, gal_uint>::iterator it = m.begin();
    while( it != m.end()) {
        gal_uint stream = it->second;
        gal_uint attrib = it->first;
		if ( _vaMap[attrib].changed() ) {
			data.uintVal = _vaMap[attrib];
			_driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_MAP, attrib, data);
			_vaMap[attrib].restart();
		}

		if ( _vaMap[attrib] != cg1gpu::ST_INACTIVE_ATTRIBUTE) // Sync stream if the vertex attribute is enabled
			_stream[_vaMap[attrib]]->sync();
        it++ ;
    }
}

void GALDeviceImp::_syncSamplerState()
{

#ifdef GAL_DUMP_SAMPLERS
    for (gal_uint i = 0; i < cg1gpu::MAX_TEXTURES; i++)
        for (gal_uint j = 0; j < 6; j++)
            for (gal_uint k = 0; k < cg1gpu::MAX_TEXTURE_SIZE; k++)
            {
                    gal_ubyte filename[30];
                    sprintf((char*)filename, "Sampler%d_Face%d_Mipmap%d.ppm", i,j,k);
                    remove((char*)filename);
            }
#endif

    GALShaderProgramImp* fProgramImp = _fsh;

    for ( gal_uint i = 0; i < cg1gpu::MAX_TEXTURES; ++i )
    {
        //if (!fProgramImp->getTextureUnitsUsage(i)) // If the texture is not used in the shader, there is no need to sync it
        //    _sampler[i]->setEnabled(false);

        _sampler[i]->sync();
    }
}


gal_uint GALDeviceImp::availableStreams() const
{
    // the stream buffer (MAX_STREAM_BUFFERS-1) is reserved to stream indexes
    return _MAX_STREAMS;
}

gal_uint GALDeviceImp::availableVertexAttributes() const
{
    return cg1gpu::MAX_VERTEX_ATTRIBUTES;
}

GALSampler& GALDeviceImp::sampler(gal_uint samplerID)
{
    //CG_ASSERT("Samplers not available yet");
    return *_sampler[samplerID];
}

void GALDeviceImp::setIndexBuffer(GALBuffer* ib, gal_uint offset, GAL_STREAM_DATA indicesType)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    // Configure stream of indices
    GAL_STREAM_DESC desc;

    desc.components = 1;
    desc.frequency = 0;
    desc.offset = offset;
    desc.type = indicesType;
    desc.stride = 0;

    _indexStream->set(ib, desc);

    GLOBAL_PROFILER_EXIT_REGION()    
}

void GALDeviceImp::_syncStreamingMode(gal_uint start, gal_uint count, gal_uint instances, gal_uint, gal_uint)
{
    // min & max params ignored currently

    cg1gpu::GPURegData data;

    // Update GPU registers with the start vertex/index and the vertex/index count

    _streamStart = start;
    if(_streamStart.changed())
    {
        data.uintVal = _streamStart;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_START, 0, data);
        _streamStart.restart();
    }

    _streamCount = count;
    if(_streamCount.changed())
    {
        data.uintVal = _streamCount;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_COUNT, 0, data);
        _streamCount.restart();
    }

    _streamInstances = instances;
    if(_streamInstances.changed())
    {
        data.uintVal = _streamInstances;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_INSTANCES, 0, data);
        _streamInstances.restart();
    }

    if ( _indexedMode ) {
        if ( _indexedMode.changed() ) {
            // Changing from sequential to indexed mode (enable GPU indexed mode)
            data.uintVal = 1;
            _driver->writeGPURegister(cg1gpu::GPU_INDEX_MODE, 0, data);
            _indexedMode.restart();
        }
        // Synchronize the stream of indices
        _indexStream->sync();
    }
    else {
        if ( _indexedMode.changed() ) {
            // Changing from indexed to sequential mode (disable GPU indexed mode)
            data.uintVal = 0;
            _driver->writeGPURegister(cg1gpu::GPU_INDEX_MODE, 0, data);
            _indexedMode.restart();
        }
        // else (nothing to do)
    }
}

GALRenderTarget* GALDeviceImp::createRenderTarget( GALTexture* resource, const GAL_RT_DIMENSION rtdimension, GAL_CUBEMAP_FACE face, gal_uint mipmap )
{
    return new GALRenderTargetImp (this, resource, rtdimension, face, mipmap);
}


gal_bool GALDeviceImp::setRenderTargets( gal_uint numRenderTargets,
                                         const GALRenderTarget* const * renderTargets,
                                         const GALZStencilTarget* zStencilTarget )
{
    cout << "GALDeviceImp::setRenderTargets(" << numRenderTargets << ",RTargets**,ZStencilTargets*) -> Not implemented yet" << endl;
    return true;
}

gal_bool GALDeviceImp::setRenderTarget(gal_uint indexRenderTarget, GALRenderTarget *renderTarget)
{
    if (renderTarget == NULL)
    {
        //  Disable drawing.
        _blending->disableColorWrite(indexRenderTarget);
        _currentRenderTarget[indexRenderTarget] = NULL;
        return true;
    }

    //  Check the render target format.
    if ((renderTarget->getFormat() != GAL_FORMAT_ARGB_8888) &&
        (renderTarget->getFormat() != GAL_FORMAT_XRGB_8888) &&
        (renderTarget->getFormat() != GAL_FORMAT_RG16F) && 
        (renderTarget->getFormat() != GAL_FORMAT_R32F) &&
        (renderTarget->getFormat() != GAL_FORMAT_RGBA16F) &&
        (renderTarget->getFormat() != GAL_FORMAT_ABGR_161616) &&
        (renderTarget->getFormat() != GAL_FORMAT_S8D24))
    {
        CG_ASSERT("Format not supported");
        return false;
    }

    GALRenderTargetImp *rt = reinterpret_cast<GALRenderTargetImp *> (renderTarget);

    //  Check if setting the already defined render target.
    if (rt == _currentRenderTarget[indexRenderTarget])
        return true;

    if ((_currentRenderTarget[indexRenderTarget] == NULL) && (rt != NULL))
    {
        // Enable drawing.
        _blending->enableColorWrite(indexRenderTarget);
    }
    
    //  Check if using the default render buffers
    if (_defaultRenderBuffers)
    {
        //  Check if changing to the front buffer (swap).
        if (rt == _defaultFrontBuffer)
        {
            _currentRenderTarget[indexRenderTarget] = rt;
            _defaultFrontBuffer = _defaultBackBuffer;
            _defaultBackBuffer = rt;

            return true;
        }

        // Ensures any pending drawing is done on previous RT.
        _driver->sendCommand(cg1gpu::GPU_FLUSHCOLOR);

        //  Save default render buffer state.
        _driver->sendCommand(cg1gpu::GPU_SAVE_COLOR_STATE);

        _currentRenderTarget[indexRenderTarget] = rt;

        _defaultRenderBuffers = false;
        
        //  Reset viewport.
        _rast->setViewport(0, 0, rt->getWidth(), rt->getHeight());

        //  Reset scissor.
        _rast->enableScissor(false);
        _rast->setScissor(0, 0, rt->getWidth(), rt->getHeight());

        return true;
    }
    else
    {
        // Ensures any pending drawing is done on previous RT.
        _driver->sendCommand(cg1gpu::GPU_FLUSHCOLOR);

        //  Check if changing to the default render buffers.
        if (rt == _defaultFrontBuffer)
        {
            _defaultFrontBuffer = _defaultBackBuffer;
            _defaultBackBuffer = rt;
            _currentRenderTarget[indexRenderTarget] = rt;

            _defaultRenderBuffers = true;

            //  Restore the color block state data from the save area.
            _driver->sendCommand(cg1gpu::GPU_RESTORE_COLOR_STATE);

            //  Reset viewport.
            _rast->setViewport(0, 0, rt->getWidth(), rt->getHeight());

            //  Reset scissor.
            _rast->enableScissor(false);
            _rast->setScissor(0, 0, rt->getWidth(), rt->getHeight());

            return true;
        }

        if (rt == _defaultBackBuffer)
        {
            _currentRenderTarget[indexRenderTarget] = rt;

            _defaultRenderBuffers = true;

            //  Restore the color block state data from the save area.
            _driver->sendCommand(cg1gpu::GPU_RESTORE_COLOR_STATE);

            //  Reset viewport.
            _rast->setViewport(0, 0, rt->getWidth(), rt->getHeight());

            //  Reset scissor.
            _rast->enableScissor(false);
            _rast->setScissor(0, 0, rt->getWidth(), rt->getHeight());

            return true;
        }

        _currentRenderTarget[indexRenderTarget] = rt;

        //  Reset viewport.
        if (indexRenderTarget == 0)
            _rast->setViewport(0, 0, rt->getWidth(), rt->getHeight());

        //  Reset scissor.
        _rast->enableScissor(false);
        _rast->setScissor(0, 0, rt->getWidth(), rt->getHeight());

        return true;
    }
}

bool GALDeviceImp::setZStencilBuffer(GALRenderTarget *zstencilBuffer)
{

//GALRenderTargetImp *currentZSB = _currentZStencilBuffer;
//GALRenderTargetImp *defaultZSB = _defaultZStencilBuffer;
//printf("GALDeviceImp::setZStencilBuffer => >> Setting zStencilBuffer = %p | currentZStencilBuffer = %p | defaultZStencilBuffer = %p | defaultZStencilBufferInUse =%s \n",
//    zstencilBuffer, currentZSB, defaultZSB, _defaultZStencilBufferInUse ? "T" : "F");

    if (zstencilBuffer == NULL)
    {
        // Disable depth stencil operation
        _currentZStencilBuffer = NULL;
        return true;
    }

    //  Check the z stencil buffer format.
    if (zstencilBuffer->getFormat() != GAL_FORMAT_S8D24)
    {
        return false;
    }

    GALRenderTargetImp *zstencil = reinterpret_cast<GALRenderTargetImp *> (zstencilBuffer);

    //  Check if setting the already defined render target.
    if (zstencil == _currentZStencilBuffer)
        return true;
    
    if (_defaultZStencilBufferInUse)
    {   
        //  Check if setting the default z stencil buffer.  This happens with the default -> NULL -> default transition.
        if (zstencil == _defaultZStencilBuffer)
        {
            _currentZStencilBuffer = zstencil;
            return true;
        }
        
        // Ensures any pending drawing is done on previous RT.
        _driver->sendCommand(cg1gpu::GPU_FLUSHZSTENCIL);

        //  Save default render buffer state.
        _driver->sendCommand(cg1gpu::GPU_SAVE_ZSTENCIL_STATE);

        _defaultZStencilBufferInUse = false;
    }
    else
    {
        // Ensures any pending drawing is done on previous RT.
        _driver->sendCommand(cg1gpu::GPU_FLUSHZSTENCIL);

        if (zstencil == _defaultZStencilBuffer)
        {
            //  Restore the color block state data from the save area.
            _driver->sendCommand(cg1gpu::GPU_RESTORE_ZSTENCIL_STATE);

            _defaultZStencilBufferInUse = true;
        }
    }
    
    _currentZStencilBuffer = zstencil;

    return true;
}

GALRenderTarget *GALDeviceImp::getRenderTarget(gal_uint indexRenderTarget)
{
    return _currentRenderTarget[indexRenderTarget];
}

GALRenderTarget *GALDeviceImp::getZStencilBuffer()
{
    return _currentZStencilBuffer;
}

bool GALDeviceImp::_multipleRenderTargetsSet() {

    for (gal_uint i = 1; i < GAL_MAX_RENDER_TARGETS; i++) {

        if (_currentRenderTarget[i] != NULL)
            return true;

    }

    return false;

}

void GALDeviceImp::_syncRenderBuffers()
{

    cg1gpu::GPURegData data;
    GALRenderTargetImp *zstencil = _currentZStencilBuffer;

    bool resolutionSet = false;
    U32 resWidth;
    U32 resHeight;
    
    //  Check if the z stencil buffer changed and was not set to null.
    if (_currentZStencilBuffer.changed() && (zstencil != NULL))
    {
        //  Get the initial resolution from the Z Stencil buffer dimensions if z or stencil tests are currently enabled.   
        resolutionSet = zStencil().isStencilEnabled() || zStencil().isZEnabled();
        resWidth = zstencil->getWidth();
        resHeight = zstencil->getHeight();
    }
        
    for (gal_uint i = 0; i < GAL_MAX_RENDER_TARGETS; i++) {

        GALRenderTargetImp *rt = _currentRenderTarget[i];

        //  Check that the render target and the z stencil buffer have the same parameters.
        if (_currentRenderTarget[i].changed() || _currentZStencilBuffer.changed())
        {
            //  Check if resolution was already set.
            if (resolutionSet)
            {
                //  Check if the render target is active.
                if (blending().getColorEnable(i))
                {
                    //  Check if the render target dimension matches the resolution set.
                    /*if ((rt->getWidth() != resWidth) || (rt->getHeight() != resHeight))
                    {
                        CG_ASSERT("Render target resolution doesn't match current resolution.");
                    }*/
                }
            }
            else
            {
                //  Check if the render target is set.
                if (rt != NULL)
                {
                    //  Set the resolution from the render target dimensions..
                    resolutionSet = true;
                    resWidth = rt->getWidth();
                    resHeight = rt->getHeight();
                }
            }
        }

        if (_currentRenderTarget[i].changed() && rt != NULL)
        {
            //  Set the resolution of the new render target.
            //_driver->setResolution(rt->getWidth(), rt->getHeight());

            data.booleanVal = true;
            _driver->writeGPURegister(cg1gpu::GPU_RENDER_TARGET_ENABLE, i, data);

            //  Set compression.
            data.booleanVal = rt->allowsCompression() && !_multipleRenderTargetsSet();

            _driver->writeGPURegister(cg1gpu::GPU_COLOR_COMPRESSION, 0, data);

            //  Set render target format.
            switch (rt->getFormat())
            {
                case GAL_FORMAT_XRGB_8888:
                case GAL_FORMAT_ARGB_8888:
                    data.txFormat = cg1gpu::GPU_RGBA8888;
                    break;
                case GAL_FORMAT_RG16F:
                    data.txFormat = cg1gpu::GPU_RG16F;
                    break;
                case GAL_FORMAT_R32F:
                    data.txFormat = cg1gpu::GPU_R32F;
                    break;                
                case GAL_FORMAT_ABGR_161616:
                    data.txFormat = cg1gpu::GPU_RGBA16;
                    break;
                case GAL_FORMAT_RGBA16F:
                    data.txFormat = cg1gpu::GPU_RGBA16F;
                    break;
                case GAL_FORMAT_S8D24:
                    data.txFormat = cg1gpu::GPU_DEPTH_COMPONENT24;
                    break;
                default:
                    CG_ASSERT("Render target format unknown.");
                    break;
            }
            //_driver->writeGPURegister(cg1gpu::GPU_COLOR_BUFFER_FORMAT, 0, data);
            _driver->writeGPURegister(cg1gpu::GPU_RENDER_TARGET_FORMAT, i, data);

            //  Set multisampling parameters
            //  TODO

            //  Set render target address.
            if (_defaultRenderBuffers)
            {
                GALRenderTargetImp *rtTemp = _defaultFrontBuffer;

                if (i == 0)
                    _driver->writeGPUAddrRegister(cg1gpu::GPU_FRONTBUFFER_ADDR, 0, rtTemp->md());

                rtTemp = _defaultBackBuffer;
                //_driver->writeGPUAddrRegister(cg1gpu::GPU_BACKBUFFER_ADDR , 0, rtTemp->md());
                _driver->writeGPUAddrRegister(cg1gpu::GPU_RENDER_TARGET_ADDRESS , i, rtTemp->md());

            }
            else
            {
                // This cleans block state bits in the color caches
                _driver->sendCommand(cg1gpu::GPU_RESET_COLOR_STATE);

                if (i == 0)
                    _driver->writeGPUAddrRegister(cg1gpu::GPU_FRONTBUFFER_ADDR, 0, rt->md());

                //_driver->writeGPUAddrRegister(cg1gpu::GPU_BACKBUFFER_ADDR , 0, rt->md());
                _driver->writeGPUAddrRegister(cg1gpu::GPU_RENDER_TARGET_ADDRESS , i, rt->md());
            }

            _currentRenderTarget[i].restart();
        }
        else if (_currentRenderTarget[i].changed() && rt == NULL && i != 0){

            data.booleanVal = false;
            _driver->writeGPURegister(cg1gpu::GPU_RENDER_TARGET_ENABLE, i, data);

        }

    }

    if (_currentZStencilBuffer.changed())
    {
        //  Check if the depth stencil buffer is not defined.
        if (zstencil == NULL)
        {
            //  Sets the z and stencil buffer as not defined.  Disables z and stencil test.
            _zStencil->setZStencilBufferDefined(false);
        }
        else
        {
            if (!_defaultZStencilBufferInUse)
            {
                // This cleans block state bits in the color caches
                _driver->sendCommand(cg1gpu::GPU_RESET_ZSTENCIL_STATE);
            }

            //  Set the z stencil buffer address.
            _driver->writeGPUAddrRegister(cg1gpu::GPU_ZSTENCILBUFFER_ADDR, 0, zstencil->md());

            //  Sets the z and stencil buffer as defined.
            _zStencil->setZStencilBufferDefined(true);

            //  Set compression.
            data.booleanVal = zstencil->allowsCompression();
            _driver->writeGPURegister(cg1gpu::GPU_ZSTENCIL_COMPRESSION, 0, data);
        }

        _currentZStencilBuffer.restart();
    }

    //  Check if the resolution was set.
    if (resolutionSet)
    {
        //  Set the new resolution.
        _driver->setResolution(resWidth, resHeight);
    }        
    else
    {
        //  No resolution was set.
    }
    
    //  Check if color linear to sRGB conversion on color write flag has changed.
    if (_colorSRGBWrite.changed())
    {
        //  Set convert color from linear to sRGB space on color write.
        data.booleanVal = _colorSRGBWrite;
        _driver->writeGPURegister(cg1gpu::GPU_COLOR_SRGB_WRITE, 0, data);
        _colorSRGBWrite.restart();
    }
}

void GALDeviceImp::draw(gal_uint start, gal_uint count, gal_uint instances)
{
    _indexedMode = false;

    if ((_primitive == GAL_TRIANGLES) || (_primitive == GAL_TRIANGLE_FAN) || (_primitive == GAL_TRIANGLE_STRIP) ||
        (_primitive == GAL_QUADS) || (_primitive == GAL_QUAD_STRIP))
    {
        _draw(start, count, 0, 0, 0, instances);
    }
    else
    {
        printf("GALDeviceImp::draw() => WARNING : Primitive ");
        switch(_primitive)
        {
            case GAL_POINTS : printf("GAL_POINTS"); break;
            case GAL_LINES :  printf("GAL_LINES"); break;
            case GAL_LINE_LOOP : printf("GAL_LINE_LOOP"); break;
            case GAL_LINE_STRIP : printf("GAL_LINE_STRIP"); break;
            default :
                CG_ASSERT("Undefined draw primitive.");
                break;
        }

        printf(" not implemented.  Ignoring draw call.\n");
        
        //CG_ASSERT("Unsupported draw primitive.");
    }
}

void GALDeviceImp::drawIndexed(gal_uint startIndex, gal_uint indexCount, gal_uint minIndex, gal_uint maxIndex,
                               gal_int baseVertexIndex, gal_uint instances)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    if ((_primitive == GAL_TRIANGLES) || (_primitive == GAL_TRIANGLE_FAN) || (_primitive == GAL_TRIANGLE_STRIP) ||
        (_primitive == GAL_QUADS) || (_primitive == GAL_QUAD_STRIP))
    {
        GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
        
        _indexedMode = true;
        _draw(startIndex, indexCount, minIndex, maxIndex, baseVertexIndex, instances);

        GLOBAL_PROFILER_EXIT_REGION()
    }
    else
    {
        printf("GALDeviceImp::drawIndexed() => WARNING : Primitive ");
        switch(_primitive)
        {
            case GAL_POINTS : printf("GAL_POINTS"); break;
            case GAL_LINES :  printf("GAL_LINES"); break;
            case GAL_LINE_LOOP : printf("GAL_LINE_LOOP"); break;
            case GAL_LINE_STRIP : printf("GAL_LINE_STRIP"); break;
            default :
                CG_ASSERT("Undefined draw primitive.");
                break;
        }

        printf(" not implemented.  Ignoring draw call.\n");
        
        //CG_ASSERT("Unsupported draw primitive.");
    }
}

void GALDeviceImp::_translatePrimitive(GAL_PRIMITIVE primitive, cg1gpu::GPURegData* data)
{
    switch ( primitive ) {
        case GAL_TRIANGLES:
            data->primitive = cg1gpu::TRIANGLE;
            break;
        case GAL_TRIANGLE_STRIP:
            data->primitive = cg1gpu::TRIANGLE_STRIP;
            break;
        case GAL_TRIANGLE_FAN:
            data->primitive = cg1gpu::TRIANGLE_FAN;
            break;
        case GAL_QUADS:
            data->primitive = cg1gpu::QUAD;
            break;
        case GAL_QUAD_STRIP:
            data->primitive = cg1gpu::QUAD_STRIP;
            break;
        case GAL_LINES:
            data->primitive = cg1gpu::LINE;
            break;
        case GAL_LINE_STRIP:
            data->primitive = cg1gpu::LINE_STRIP;
            break;
        case GAL_LINE_LOOP:
            data->primitive = cg1gpu::LINE_FAN;
            break;
        case GAL_POINTS:
            data->primitive = cg1gpu::POINT;
            break;
        default:
            CG_ASSERT("GAL Unsupported primitive");
    }
}

void GALDeviceImp::_dump(const gal_char* file, gal_enum flags)
{
    // cout << "GALDeviceImp::DBG_dump(" << file << ", 0x" << hex << flags << dec << ")" << endl;
    ofstream out;

    BoolPrint boolPrint;

    out.open(file);

    if ( !out.is_open() )
        CG_ASSERT("Dump failed (output file could not be opened)");

    out << "####################" << endl;
    out << "## GAL STATE DUMP ##" << endl;
    out << "####################" << endl;
    out << "# FRAME: " << _currentFrame << endl;
    out << "# BATCH: " << _currentBatch << endl << endl;

    // Output internal data

    out << stateItemString(_primitive,"PRIMITIVE", false, &primitivePrint);
    out << stateItemString(_indexedMode,"INDEXED_MODE", false, &boolPrint);

    if (_indexedMode)
        out << "INDEX_STREAM:" << _indexStream->getInternalState();

    std::vector<StateItem<gal_uint> >::const_iterator iter = _vaMap.begin();

    gal_uint i = 0;

    while ( iter != _vaMap.end() )
    {
        out << "VERTEX_ATTR" << i << "_STREAM";

        if ((*iter) != cg1gpu::ST_INACTIVE_ATTRIBUTE)
            out << " = " << (*iter);
        else
            out << " = INACTIVE";

        if ( (*iter).changed() )
        {
            out << " NotSync = ";
            if ((*iter).initial() != cg1gpu::ST_INACTIVE_ATTRIBUTE)
            {
                out << (*iter).initial() << "\n";
            }
            else
                out << "INACTIVE\n";
        }
        else
            out << " Sync\n";

        if ((*iter) != cg1gpu::ST_INACTIVE_ATTRIBUTE && (*iter) < _MAX_STREAMS)
        {
            out << _stream[(*iter)]->getInternalState();
        }
        iter++;
        i++;
    }

    // Output stages states
    out << _rast->getInternalState();
    out << _zStencil->getInternalState();
    out << _blending->getInternalState();

    out << stateItemString(_clearColorBuffer, "CLEAR_COLOR_BUFFER",false);
    out << stateItemString(_zClearValue,"Z_CLEAR_VALUE",false);
    out << stateItemString(_stencilClearValue,"STENCIL_CLEAR_VALUE",false);

    out << stateItemString(_hzActivated,"HZ_ACTIVE",false, &boolPrint);

    out << "GEOMETRY_SHADER_DEFINED = ";
    if (_gsh)
    {
        out << "TRUE" << endl;
        GALShaderProgramImp* gProgramImp = _gsh;
        gProgramImp->printASM(out);
        gProgramImp->printConstants(out);
    }
    else
        out << "FALSE" << endl;

    out << "VERTEX_SHADER_DEFINED = ";
    if (_vsh)
    {
        out << "TRUE" << endl;
        GALShaderProgramImp* vProgramImp = _vsh;
        vProgramImp->printASM(out);
        vProgramImp->printConstants(out);

        const gal_uint vshOutputsCount = _vshOutputs.size();
        for ( gal_uint i = 0; i < vshOutputsCount; ++i )
        {
            out << "VERTEX_SHADER_OUTPUT" << i << "_WRITTEN";
            out << stateItemString(_vshOutputs[i],"",false,&boolPrint);
        }

        out << stateItemString(_vshResources,"VERTEX_SHADER_THREAD_RESOURCES",false);
    }
    else
        out << "FALSE" << endl;


    out << "FRAGMENT_SHADER_DEFINED = ";
    if (_vsh)
    {
        out << "TRUE" << endl;
        GALShaderProgramImp* fProgramImp = _fsh;
        fProgramImp->printASM(out);
        fProgramImp->printConstants(out);

        const gal_uint fshOutputsCount = _fshInputs.size();
        for ( gal_uint i = 0; i < fshOutputsCount; ++i )
        {
            out << "FRAGMENT_SHADER_INPUT" << i << "_WRITTEN";
            out << stateItemString(_fshInputs[i],"",false,&boolPrint);
        }

        out << stateItemString(_fshResources,"FRAGMENT_SHADER_THREAD_RESOURCES",false);
    }
    else
        out << "FALSE" << endl;


    for ( gal_uint i = 0; i < cg1gpu::MAX_TEXTURES; ++i )
        out << _sampler[i]->getInternalState();

    out.close();
}

void GALDeviceImp::_check_deferred_dump()
{
    if (!_nextDumpEvent.valid)
        return;

    if (_currentFrame > _nextDumpEvent.frame)
        CG_ASSERT("A deferred dump was skipped and this shouldnt happen");

    if (_currentFrame == _nextDumpEvent.frame)
    {
        if (_currentBatch > _nextDumpEvent.batch)
            CG_ASSERT("A deferred dump was skipped and this shouldnt happen");

        if (_currentBatch == _nextDumpEvent.batch)
        {
            // There is an event for this time.

            // Dump the state.
            _dump(_nextDumpEvent.fileName.c_str(), _nextDumpEvent.flags);

            // Remove the event from list
            _dumpEventList.remove(_nextDumpEvent);

            // Get the next dump event in time

            // First, get the frame and batch of the closest event.
            gal_uint minFrame, minBatch;
            minFrame = minBatch = gal_uint(-1);

            std::list<DumpEventInfo>::const_iterator iter = _dumpEventList.begin();

            for (; iter != _dumpEventList.end(); iter++)
            {
                if ((*iter).frame <= minFrame)
                {
                    minFrame = (*iter).frame;
                    if ((*iter).batch < minBatch)
                        minBatch = (*iter).batch;
                }
            };

            // Now, find the closest event struct and copy it into the
            // _nextDumpEvent;

            iter = _dumpEventList.begin();

            gal_bool found = false;

            while ( iter != _dumpEventList.end() && !found)
            {
                if ((*iter).frame == minFrame)
                    if ((*iter).batch == minBatch)
                    {
                        _nextDumpEvent = (*iter);
                        found = true;
                    }
                iter++;
            }

            if (!found)
                _nextDumpEvent.valid = false;

        }
    }
};

void GALDeviceImp::_addBaseVertexIndex(gal_uint baseVertexIndex, gal_uint start, gal_uint count)
{

    /*if ( _vaMap[31] != cg1gpu::ST_INACTIVE_ATTRIBUTE)
        CG_ASSERT("Trying to perform a draw indexed call without any stream of indexes setted");
    else
    {*/

        GALBuffer* original = _indexStream->getBuffer(); // Stream 31 is where Indexed Stream is stored by default

        gal_ubyte* originalUbyte = (gal_ubyte* )original->getData();
        gal_ushort* originalUshort = (gal_ushort*) originalUbyte;
        gal_uint* originalUint = (gal_uint*) originalUbyte;
        gal_float* originalFloat = (gal_float*) originalUbyte;

        GALBuffer* copy = createBuffer(0,0);

        gal_uint last = count + start;

        switch(_indexStream->getType())
        {
            case GAL_SD_UINT8:
            case GAL_SD_INV_UINT8:
                for(int i = start; i < last; i++)
                {
                    /*gal_ubyte newValue = originalUbyte[i] + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_ubyte));*/
                    gal_uint newValue = (gal_uint)(originalUbyte[i]) + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_uint));
                }
                _indexStream->setType(GAL_SD_UINT32);
                break;

            case GAL_SD_UINT16:
            case GAL_SD_INV_UINT16:
                for(int i = start; i < last; i++)
                {
                    /*gal_ushort newValue = originalUshort[i] + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_ushort));*/
                    gal_uint newValue = (gal_uint)(originalUshort[i]) + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_uint));
                }
                _indexStream->setType(GAL_SD_UINT32);
                break;

            case GAL_SD_UINT32:
            case GAL_SD_INV_UINT32:
                for(int i = start; i < last; i++)
                {
                    gal_uint newValue = originalUint[i] + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_uint));
                }
                break;

            case GAL_SD_FLOAT32:
            case GAL_SD_INV_FLOAT32:
                for(int i = start; i < last; i++)
                {
                    gal_float newValue = originalFloat[i] + baseVertexIndex;
                    copy->pushData(&newValue, sizeof(gal_float));
                }
                break;

            default:
                CG_ASSERT("Buffer content type not available");
        }

        _indexStream->setBuffer(copy);
    //}

}

void GALDeviceImp::_draw(gal_uint start, gal_uint count, gal_uint min, gal_uint max, gal_uint baseVertexIndex, gal_uint instances)
{
    ///////////////////////////////////////////////////////////////
    /// Synchronize render buffers                              ///
    ///////////////////////////////////////////////////////////////
    _syncRenderBuffers();

    ///////////////////////////////////////////////////////////////
    /// Synchronize stages Rasterization, ZStencil and Blending ///
    ///////////////////////////////////////////////////////////////
    _rast->sync();
    _zStencil->sync();
    _blending->sync();

    //////////////////////////////////
    /// Synchronize primitive mode ///
    //////////////////////////////////
    if ( _primitive.changed() )
    {
        cg1gpu::GPURegData data;
        _translatePrimitive(_primitive, &data);
        _driver->writeGPURegister(cg1gpu::GPU_PRIMITIVE, data);
        _primitive.restart();
    }

    //////////////////////////////////
    /// Synchronize bound textures ///
    //////////////////////////////////
    _syncSamplerState();

    ////////////////////
    /// Sync shaders ///
    ////////////////////
    _syncVertexShader();
    _syncFragmentShader();

    ////////////////////////////////////////////////////////////////////////////////
    /// Enable or disable GPU Hierarchical Z depending on current ZStencil state ///
    ////////////////////////////////////////////////////////////////////////////////
    _syncHZRegister();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// If a baseVertexIndex is setted, baseVertexIndex is added to each Index of the Indexed Stream ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    if (_indexedMode && baseVertexIndex != 0) {
        _addBaseVertexIndex(baseVertexIndex, start, count);
        start = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// Synchronize streams state ((buffers attached automatically synchronized ///
    ///////////////////////////////////////////////////////////////////////////////
    _syncStreamerState();

    //////////////////////////////////
    /// Synchronize streaming mode ///
    //////////////////////////////////
    _syncStreamingMode(start, count, instances, min, max);

    //////////////////////////////////////////////////////////////////////////////
    /// Enable or disable VERTEX_OUTPUT_ATTRIBUTES & FRAGMENT_INPUT_ATTRIBUTES ///
    //////////////////////////////////////////////////////////////////////////////
    cg1gpu::GPURegData bValue;

    const gal_uint vshOutputsCount = _vshOutputs.size();
    for ( gal_uint i = 0; i < vshOutputsCount; ++i ) 
    {
        if ( _vshOutputs[i].changed() ) 
        {
            bValue.booleanVal = _vshOutputs[i];
            _driver->writeGPURegister(cg1gpu::GPU_VERTEX_OUTPUT_ATTRIBUTE, i, bValue);
            _vshOutputs[i].restart();
        }
    }

    gal_bool oneFragmentInput = false;

    const gal_uint fshOutputsCount = _fshInputs.size();
    for ( gal_uint i = 0; i < fshOutputsCount; ++i ) 
    {
        if ( _fshInputs[i].changed() ) 
        {
            bValue.booleanVal = _fshInputs[i];
            _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_INPUT_ATTRIBUTES, i, bValue);
            _fshInputs[i].restart();
        }

        oneFragmentInput = oneFragmentInput || _fshInputs[i];
    }

    if ( !oneFragmentInput )
    {
        _fshInputs[cg1gpu::COLOR_ATTRIBUTE] = true;
        bValue.booleanVal = true;
        _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_INPUT_ATTRIBUTES, cg1gpu::COLOR_ATTRIBUTE, bValue);
        _fshInputs[cg1gpu::COLOR_ATTRIBUTE].restart();
    }

    // EarlyZ can be actived if Z is not modified by the fragment shader
    // If fragment depth is modified by the fragment program disable Early Z
    GALShaderProgramImp* fsh = _fsh;

    if (fsh->getOutputWritten(0) || fsh->getKillInstructions())
    {
        _earlyZ = false;
        
        if (_earlyZ.changed())
        {
            bValue.booleanVal = false;
            _driver->writeGPURegister(cg1gpu::GPU_EARLYZ, bValue);
            _alphaTest.restart();
            _earlyZ.restart();
        }
    }
    else
    {
        _earlyZ = true;
        if (_earlyZ.changed())
        {
            bValue.booleanVal = true;
            _driver->writeGPURegister(cg1gpu::GPU_EARLYZ, bValue);
            _alphaTest.restart();
            _earlyZ.restart();
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    ///                 Synchronize Shader Thread Resources                    ///
    //////////////////////////////////////////////////////////////////////////////

    cg1gpu::GPURegData data;

    if ( _vshResources.changed() )
    {
        data.uintVal = _vshResources;
        _driver->writeGPURegister(cg1gpu::GPU_VERTEX_THREAD_RESOURCES, data);
        _vshResources.restart();
    }

    if ( _fshResources.changed() )
    {
        data.uintVal = _fshResources;
        _driver->writeGPURegister(cg1gpu::GPU_FRAGMENT_THREAD_RESOURCES, data);
        _fshResources.restart();
    }

    // Check if deferred GAL state dump is required
    _check_deferred_dump();

    ////////////
    /// DRAW ///
    ////////////
    _driver->sendCommand( cg1gpu::GPU_DRAW );

    _moa->realeaseLockedMemoryRegions();

    _currentBatch++;
}

void GALDeviceImp::updateResource( const GALResource* destResource,
                                   gal_uint destSubResource,
                                   const GAL_BOX* destRegion,
                                   const gal_ubyte* srcData,
                                   gal_uint srcRowPitch,
                                   gal_uint srcDepthPitch )
{
    cout << "GALDeviceImp::updateResource(DEST_RSC," << destSubResource << ",BOX*,srcData," << srcRowPitch
         << "," << srcDepthPitch << ") -> Not implemented yet" << endl;
}

gal_bool GALDeviceImp::swapBuffers()
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    
    
    cout << "GALDeviceImp::swapBuffers() - OK" << endl;
    _driver->sendCommand(cg1gpu::GPU_SWAPBUFFERS);

    //  If using the default render buffer swap front and back buffers (double buffering).
    /*if (_defaultRenderBuffers)
    {
        _defaultBackBuffer = _defaultFrontBuffer;
        _defaultFrontBuffer = _currentRenderTarget;
    }*/

    _currentFrame++;
    _currentBatch = 0;
    
    GLOBAL_PROFILER_EXIT_REGION()
    
    return true;
}

gal_uint GALDeviceImp::_packRGBA8888(gal_ubyte red, gal_ubyte green, gal_uint blue, gal_uint alpha)
{
    return (static_cast<gal_uint>(alpha) << 24) +
           (static_cast<gal_uint>(blue)  << 16) +
           (static_cast<gal_uint>(green) <<  8) +
            static_cast<gal_uint>(red);
}

void GALDeviceImp::clearColorBuffer(gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")

    ///////////////////////////////////////////////////////////////
    /// Synchronize render buffers                              ///
    ///////////////////////////////////////////////////////////////
    _syncRenderBuffers();

    // Update the clear color buffer
    _clearColorBuffer = _packRGBA8888(red, green, blue, alpha);

    //  Get scissor test state.
    gal_bool scissorEnabled;
    gal_int scissorIniX, scissorIniY;
    gal_uint scissorWidth, scissorHeight;

    gal_uint resWidth, resHeight;
    // Incorrecto!!
    _driver->getResolution(resWidth, resHeight);

    _rast->getScissor(scissorEnabled, scissorIniX, scissorIniY, scissorWidth, scissorHeight);

    //  Check if scissor test is enabled.
    if (_defaultRenderBuffers &&
        ((!scissorEnabled) || ((scissorIniX == 0) && (scissorIniY == 0) && (scissorWidth == resWidth) && (scissorHeight == resHeight)))
       )
    {
        //  WARNING:  The clear color register for fast color clear must only be modified before
        //  issuing a fast color clear command.

        // Check if update GPU is required.
        if ( _clearColorBuffer.changed() )
        {
            cg1gpu::GPURegData data;
            data.qfVal[0] =  float(_clearColorBuffer         & 0xff) / 255.0f;
            data.qfVal[1] =  float((_clearColorBuffer >> 8)  & 0xff) / 255.0f;
            data.qfVal[2] =  float((_clearColorBuffer >> 16) & 0xff) / 255.0f;
            data.qfVal[3] =  float((_clearColorBuffer >> 24) & 0xff) / 255.0f;
            _driver->writeGPURegister(cg1gpu::GPU_COLOR_BUFFER_CLEAR, data);
            _clearColorBuffer.restart();
        }

        // Perform a full (fast) clear of the color buffer
        _driver->sendCommand(cg1gpu::GPU_CLEARCOLORBUFFER);
    }
    else
    {
        //  Implement a partial clear of color buffer.
        _partialClear(true, false, false, red, green, blue, alpha, 0x0000, 0x00);
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALDeviceImp::clearZStencilBuffer( gal_bool clearZ, gal_bool clearStencil,
                                        gal_float zValue, gal_int stencilValue )
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    ///////////////////////////////////////////////////////////////
    /// Synchronize render buffers                              ///
    ///////////////////////////////////////////////////////////////
    _syncRenderBuffers();

    if (clearZ)
        _zClearValue = zValue;

    if (clearStencil)
        _stencilClearValue = stencilValue;

    //  Get scissor test parameters.
    gal_bool scissorEnabled;
    gal_int scissorIniX, scissorIniY;
    gal_uint scissorWidth, scissorHeight;

    gal_uint resWidth, resHeight;
    _driver->getResolution(resWidth, resHeight);

    _rast->getScissor(scissorEnabled, scissorIniX, scissorIniY, scissorWidth, scissorHeight);

    GALRenderTargetImp *zstencil = _currentZStencilBuffer;

    if (clearZ && clearStencil)
    {
        //  Check if scissor test is enabled.
        if (zstencil->allowsCompression() && 
            ((!scissorEnabled) || ((scissorIniX == 0) && (scissorIniY == 0) && (scissorWidth == resWidth) && (scissorHeight == resHeight))))
        {
            //  Perform full (fast) z and stencil clear.

            //  WARNING: The z and stencil values for the fast z and stencil clear must only be updated before
            //  issuing the fast z and stencil clear command.

            if (_zClearValue.changed())
            {
                //  The float point depth value in the 0 .. 1 range must be converted to an integer
                //  value of the selected depth bit precission.
                //
                //  WARNING: This function should depend on the current depth bit precission.  Should be
                //  configurable using the wgl functions that initialize the framebuffer.  By default
                //  should be 24 (stencil 8 depth 24).  Currently the simulator only supports 24.
                //
                cg1gpu::GPURegData data;
                data.uintVal = static_cast<gal_uint>(static_cast<gal_float>(_zClearValue) * static_cast<gal_float>((1 << 24) - 1));
                _driver->writeGPURegister(cg1gpu::GPU_Z_BUFFER_CLEAR, data);
                _zClearValue.restart();
            }

            if (_stencilClearValue.changed())
            {
                cg1gpu::GPURegData data;
                data.intVal = _stencilClearValue;
                _driver->writeGPURegister(cg1gpu::GPU_STENCIL_BUFFER_CLEAR, data);
                _stencilClearValue.restart();
            }

            _driver->sendCommand( cg1gpu::GPU_CLEARZSTENCILBUFFER );
            _hzBufferValid = true; // HZ buffer contents are valid
        }
        else
        {
            //  Implement a partial clear of color buffer.
            _partialClear(false, true, true, 0, 0, 0, 0, zValue, stencilValue);
        }
    }
    else if (clearZ)
    {
        //  Check if scissor test is enabled.
        if (zstencil->allowsCompression() && 
            ((!scissorEnabled) || ((scissorIniX == 0) && (scissorIniY == 0) && (scissorWidth == resWidth) && (scissorHeight == resHeight))))
        {
            //  Perform full (fast) z and stencil clear.

            //  WARNING: The z and stencil values for the fast z and stencil clear must only be updated before
            //  issuing the fast z and stencil clear command.

            if (_zClearValue.changed())
            {
                //  The float point depth value in the 0 .. 1 range must be converted to an integer
                //  value of the selected depth bit precission.
                //
                //  WARNING: This function should depend on the current depth bit precission.  Should be
                //  configurable using the wgl functions that initialize the framebuffer.  By default
                //  should be 24 (stencil 8 depth 24).  Currently the simulator only supports 24.
                //
                cg1gpu::GPURegData data;
                data.uintVal = static_cast<gal_uint>(static_cast<gal_float>(_zClearValue) * static_cast<gal_float>((1 << 24) - 1));
                _driver->writeGPURegister(cg1gpu::GPU_Z_BUFFER_CLEAR, data);
                _zClearValue.restart();
            }

            if (_stencilClearValue.changed())
            {
                cg1gpu::GPURegData data;
                data.intVal = _stencilClearValue;
                _driver->writeGPURegister(cg1gpu::GPU_STENCIL_BUFFER_CLEAR, data);
                _stencilClearValue.restart();
            }

            cout << "Warning: Clear Z implemented as Clear Z and Stencil (optimization)" << endl;
            _driver->sendCommand( cg1gpu::GPU_CLEARZSTENCILBUFFER );
            _hzBufferValid = true; // HZ buffer contents are valid
        }
        else
        {
            //  Implement a partial clear of color buffer.
            _partialClear(false, true, false, 0, 0, 0, 0, zValue, stencilValue);
        }
    }
    else if (clearStencil)
    {
        //  Implement a partial clear of color buffer.
        _partialClear(false, false, true, 0, 0, 0, 0, zValue, stencilValue);
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALDeviceImp::_partialClear(gal_bool clearColor, gal_bool clearZ, gal_bool clearStencil,
                                 gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha,
                                 gal_float zValue, gal_int stencilValue)
{
    cg1gpu::GPURegData data;
    vector<cg1gpu::GPURegData> savedRegState;
    vector<const StoredStateItem*> storedState;

    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_MASK_R));
    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_MASK_G));
    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_MASK_B));
    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_MASK_A));
    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR));
    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_ENABLED));

    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_FRONT_STENCIL_COMPARE_MASK));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_Z_ENABLED));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_Z_FUNC));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_Z_MASK));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_RANGE_NEAR));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_RANGE_FAR));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_FRONT_COMPARE_FUNC));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_FRONT_STENCIL_REF_VALUE));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_FRONT_STENCIL_COMPARE_MASK));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_FRONT_STENCIL_OPS));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_STENCIL_ENABLED));
    storedState.push_back(_zStencil->createStoredStateItem(GAL_ZST_STENCIL_BUFFER_DEF));

    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_CULLMODE));

    storedState.push_back(createStoredStateItem(GAL_DEV_PRIMITIVE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VERTEX_THREAD_RESOURCES));
    storedState.push_back(createStoredStateItem(GAL_DEV_FRAGMENT_THREAD_RESOURCES));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_ATTRIBUTE_MAP + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES + i)));

    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_STRIDE));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_ELEMENTS));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_FREQUENCY));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_DATA_TYPE));

    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_COUNT));
    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_START));
    storedState.push_back(createStoredStateItem(GAL_DEV_INDEX_MODE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VSH));
    storedState.push_back(createStoredStateItem(GAL_DEV_FSH));

    _driver->readGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, data);
    savedRegState.push_back(data);

    //  Check if the color buffer must be cleared.
    if (clearColor)
    {
        // Enable writing on all color channels.
        _blending->setColorMask(0, true, true, true, true);
        
        //  Disable color blending.
        _blending->setEnable(0, false);
    }
    else 
    {
        // Disable writing on all color channels
        _blending->setColorMask(0, false, false, false, false);
    }

    //  Check if the z buffer must be cleared
    if (clearZ)
    {
        // Enable writing the depth value in the buffer.
        _zStencil->setZMask(true);

        //  Enable depth test.
        _zStencil->setZEnabled(true);
        _zStencil->setZStencilBufferDefined(true);

        //  Set depth function to always pass/write.
        _zStencil->setZFunc(GAL_COMPARE_FUNCTION_ALWAYS);

        //  Set depth near and far planes.
        _zStencil->setDepthRange(0.0, 1.0);
    }
    else
    {
        // Disable writing the deth value in the buffer.
        _zStencil->setZMask(false);

        // Disable depth test.
        _zStencil->setZEnabled(false);
    }

    //  Check if the stencil buffer must be cleared
    if (clearStencil)
    {
        _zStencil->setStencilEnabled(true);

        //  Set the stencil test function to never fail.
        //  Set the stencil reference value to the clear value.
        //  Enable all bits in the stencil mask.
        _zStencil->setStencilFunc(GAL_FACE_FRONT, GAL_COMPARE_FUNCTION_ALWAYS, stencilValue, 255);

        //  Set stencil pass update function to replace with reference value.
        //  Set depth pass and fail update functions to keep stencil value.
        _zStencil->setStencilOp(GAL_FACE_FRONT, GAL_STENCIL_OP_KEEP, GAL_STENCIL_OP_REPLACE, GAL_STENCIL_OP_REPLACE);

        //  Enable stencil test.
        _zStencil->setStencilEnabled(true);
        _zStencil->setZStencilBufferDefined(true);
    }
    else
    {
        // Disable stencil test.
        _zStencil->setStencilEnabled(false);
    }

    // Disable face culling.
    _rast->setCullMode(GAL_CULL_NONE);

    //  Set rendering primitive to QUAD.
    _primitive = GAL_QUADS;

    //  Set vertex and fragment shaders to the default shaders.
    _vsh = _defaultVshProgram;
    _fsh = _defaultFshProgram;

    //  Set vertex and fragment shader resource usage.
    _vshResources = 1;
    _fshResources = 1;

    //  Set all vertex attributes but position to disabled.
    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
       _vaMap[i] = cg1gpu::ST_INACTIVE_ATTRIBUTE;
    
    _vaMap[cg1gpu::POSITION_ATTRIBUTE] = 0;


    GALFloatVector4 color;
    color[0] = float((red & 0xff) / 255.0f);
    color[1] = float((green & 0xff) / 255.0f);
    color[2] = float((blue & 0xff) / 255.0f);
    color[3] = float((alpha & 0xff) / 255.0f);

    _currentColor = color;

    data.qfVal[0] = color[0];
    data.qfVal[1] = color[1];
    data.qfVal[2] = color[2];
    data.qfVal[3] = color[3];

    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, data);

    zValuePartialClear = zValue;

    if (zValuePartialClear.changed())
    {

        //  Allocate buffer with the quad positions.
        gal_float vertexPositions[4 * 4] =
        {
            -1.0f, -1.0f, zValue, 1.0f,
             1.0f, -1.0f, zValue, 1.0f,
             1.0f,  1.0f, zValue, 1.0f,
            -1.0f,  1.0f, zValue, 1.0f
        };

        destroy(vertexBufferPartialClear);
        vertexBufferPartialClear = createBuffer(4*16, (gal_ubyte*)vertexPositions);

    }

    GAL_STREAM_DESC descPartialClear;
    descPartialClear.components = 4;
    descPartialClear.frequency = 0;
    descPartialClear.offset = 0;
    descPartialClear.stride = 16;
    descPartialClear.type = GAL_SD_FLOAT32;

    //  Set stream 0 for reading the quad vertex positions.
    _stream[0]->set(vertexBufferPartialClear, descPartialClear);

    zValuePartialClear.restart();


    //  Disable indexed mode.
    _indexedMode = false;

    _draw(0, 4, 0, 0);


    gal_int nextReg = 0;

    _blending->restoreStoredStateItem(storedState[nextReg++]);
    _blending->restoreStoredStateItem(storedState[nextReg++]);
    _blending->restoreStoredStateItem(storedState[nextReg++]);
    _blending->restoreStoredStateItem(storedState[nextReg++]);
    _blending->restoreStoredStateItem(storedState[nextReg++]);
    _blending->restoreStoredStateItem(storedState[nextReg++]);

    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);
    _zStencil->restoreStoredStateItem(storedState[nextReg++]);


    _rast->restoreStoredStateItem(storedState[nextReg++]);

    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);

    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);

    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, savedRegState[0]);
}

void GALDeviceImp::clearRenderTarget( GALRenderTarget* rTarget,
                                      gal_ubyte red,
                                      gal_ubyte green,
                                      gal_ubyte blue,
                                      gal_ubyte alpha)
{
    cout << "GALDeviceImp::clearRenderTarget(RTarget," << (gal_uint)red << "," << (gal_uint)green << ","
         << (gal_uint)blue << "," << (gal_uint)alpha << ") -> Not implemented yet" << endl;
}

void GALDeviceImp::clearZStencilTarget( GALZStencilTarget* zsTarget,
                                        gal_bool clearZ,
                                        gal_bool clearStencil,
                                        gal_float zValue,
                                        gal_ubyte stencilValue )
{
    cout << "GALDeviceImp::clearZStencilTarget(ZSTarget," << (clearZ ? "TRUE," : "FALSE,")
        << (clearStencil ? "TRUE," : "FALSE,") << (gal_uint)stencilValue << ") -> Not implemented yet" << endl;
}

void GALDeviceImp::copySurfaceDataToRenderBuffer(GALTexture2D *sourceTexture, gal_uint mipLevel, GALRenderTarget *destRenderTarget, bool preload)
{
    //  Get a pointer to the source mipmap data.
    U08 *sourceData;
    U32 sourceDataPitch;
    sourceTexture->map(mipLevel, GAL_MAP_READ, sourceData, sourceDataPitch);
    
    //  Get source mipmap information.
    U32 sourceWidth = sourceTexture->getWidth(mipLevel);
    U32 sourceHeight = sourceTexture->getHeight(mipLevel);
    U32 width = destRenderTarget->getWidth();
    U32 height = destRenderTarget->getHeight();
    cg1gpu::TextureFormat format;
    bool invertColors;
    switch(sourceTexture->getFormat(mipLevel))
    {
        case GAL_FORMAT_XRGB_8888:
        case GAL_FORMAT_ARGB_8888:
            format = cg1gpu::GPU_RGBA8888;
            invertColors = true;
            break;
        case GAL_FORMAT_RG16F:
            format = cg1gpu::GPU_RG16F;
            invertColors = false;
            break;
        case GAL_FORMAT_R32F:
            format = cg1gpu::GPU_R32F;
            invertColors = false;
            break;                
        case GAL_FORMAT_RGBA16F:
            format = cg1gpu::GPU_RGBA16F;
            invertColors = false;
            break;
        case GAL_FORMAT_ABGR_161616:
            format = cg1gpu::GPU_RGBA16;
            invertColors = false;
            break;

        default:
            CG_ASSERT("Unsupported render buffer format.");
            break;
    }
    
    //  Tile the source data to the render buffer format.
    U08 *destData;    
    U32 destDataSize;    
    _driver->tileRenderBufferData(sourceData, width, height, false, 1, format, invertColors, destData, destDataSize);
    
    sourceTexture->unmap(mipLevel);
    
    TextureAdapter surface(destRenderTarget->getTexture());
    
    //  Get the memory descriptor to the render buffer.
    gal_uint destRenderTargetMD = _moa->md(static_cast<GALTexture2DImp*>(surface.getTexture()), mipLevel);

    //  Check if data is to be preloaded into GPU memory.
    if (preload)
    {
        //  Update the render buffer.
        _driver->writeMemoryPreload(destRenderTargetMD, 0, destData, destDataSize);
    }
    else
    {
        //  Update the render buffer.
        _driver->writeMemory(destRenderTargetMD, destData, destDataSize, true);
    }    
    
    //  Delete the converted data.    
    delete[] destData;
}


void GALDeviceImp::getShaderLimits(GAL_SHADER_TYPE type, GAL_SHADER_LIMITS* limits)
{
    cout << "GALDeviceImp::getShaderLimits() -> Not implemented yet" << endl;
}

GALShaderProgram* GALDeviceImp::createShaderProgram() const
{
    return new GALShaderProgramImp();
}

void GALDeviceImp::setGeometryShader(GALShaderProgram* program)
{
    GALShaderProgramImp* gsh = static_cast<GALShaderProgramImp*>(program);
    _gsh = gsh;
}

void GALDeviceImp::setVertexShader(GALShaderProgram* program)
{
    GALShaderProgramImp* vsh = static_cast<GALShaderProgramImp*>(program);
    _vsh = vsh;
}

void GALDeviceImp::setFragmentShader(GALShaderProgram* program)
{
    GALShaderProgramImp* fsh = static_cast<GALShaderProgramImp*>(program);
    _fsh = fsh;
}

void GALDeviceImp::setVertexDefaultValue(gal_float currentColor[4])
{
    GALFloatVector4 color;
    color[0] = currentColor[0]; 
    color[1] = currentColor[1]; 
    color[2] = currentColor[2];
    color[3] = currentColor[3];

    _currentColor = color;
}

void GALDeviceImp::_syncVertexShader()
{
    GALShaderProgramImp* vsh = _vsh;

    GAL_ASSERT(
        if ( vsh == 0 )
            CG_ASSERT("Vertex program not selected");
    )

    if( _currentColor.changed())
    {
        cg1gpu::GPURegData data;
        const GALFloatVector4& _currentAux = _currentColor;
        data.qfVal[0] = _currentAux[0];
        data.qfVal[1] = _currentAux[1];
        data.qfVal[2] = _currentAux[2];
        data.qfVal[3] = _currentAux[3];

        _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, (U32)HAL::VS_COLOR, data);
            
        _currentColor.restart();

    }

    // Optimize the shader
    _optimizeShader(vsh, GAL_VERTEX_SHADER);

    // Synchronize the current shader written outputs
    const gal_uint vshOutputsCount = _vshOutputs.size();

    for ( gal_uint i = 0; i < vshOutputsCount; ++i )
    {
        _vshOutputs[i] = vsh->getOutputWritten(i);
    }

    // Synchronize the current shader temp resources
    _vshResources = (vsh->getMaxAliveTemps() > 0)? vsh->getMaxAliveTemps():1;

    // Synchronize GPU memory (if required)
    _moa->syncGPU(vsh);

    // Update constants
    vsh->updateConstants(_driver, cg1gpu::GPU_VERTEX_CONSTANT);

    // Get program size (bytes occupied in GPU)
    gal_uint programSize;
    vsh->memoryData(0, programSize);

    GAL_ASSERT(
        if ( programSize == 0 )
            CG_ASSERT("Vertex program code not defined");
    )

    // Get the memory descriptor of this program
    gal_uint md = _moa->md(vsh);

    // Selects the vertex shader as current
    _driver->commitVertexProgram(md, programSize, 0);
}


void GALDeviceImp::_syncFragmentShader()
{
    GALShaderProgramImp* fsh = _fsh;

    GAL_ASSERT(
        if ( fsh == 0 )
            CG_ASSERT("Fragment program not selected");
    )

    // Optimize the shader
    _optimizeShader(fsh, GAL_FRAGMENT_SHADER);

    // Synchronize the current shader written outputs
    const gal_uint fshInputsCount = _fshInputs.size();

    for ( gal_uint i = 0; i < fshInputsCount; ++i )
    {
        _fshInputs[i] = fsh->getInputRead(i);
    }

    // Synchronize the current shader temp resources
    _fshResources = (fsh->getMaxAliveTemps() > 0)? fsh->getMaxAliveTemps():1;

    // Synchronize GPU memory (if required)
    _moa->syncGPU(fsh);

    // Update constants
    fsh->updateConstants(_driver, cg1gpu::GPU_FRAGMENT_CONSTANT);

    // Get program size (bytes occupied in GPU)
    gal_uint programSize;
    fsh->memoryData(0, programSize);

    GAL_ASSERT(
        if ( programSize == 0 )
            CG_ASSERT("Fragment program code not defined");
    )

    // Get the memory descriptor of this program
    gal_uint md = _moa->md(fsh);

    // Selects the vertex shader as current
    _driver->commitFragmentProgram(md, programSize, 0);

}

void GALDeviceImp::_optimizeShader(GALShaderProgramImp* shProgramImp, GAL_SHADER_TYPE shType)
{
    if (!shProgramImp->isOptimized())
    {
        const gal_ubyte* unOptCode = shProgramImp->getCode();
        gal_uint unOptCodeSize = shProgramImp->getSize();

//cout << "GALDeviceImp:_optimizeShader => Shader Before Optimization: " << endl;
//shProgramImp->printASM(cout);
//cout << endl;

        vector<GALVector<gal_float,4> > constantBank;

        for (gal_uint i=0; i < 96; i++)
        {
            gal_float constant[4];
            shProgramImp->getConstant(i, constant);
            constantBank.push_back(GALVector<gal_float,4>(constant));
        }

        _shOptimizer.setCode(unOptCode, unOptCodeSize);
        _shOptimizer.setConstants(constantBank);

        _shOptimizer.optimize();


        gal_uint optCodeSize;
        const gal_ubyte* optCode = _shOptimizer.getCode(optCodeSize);

//shProgramImp->setOptimizedCode(optCode, optCodeSize);
//cout << "GALDeviceImp:_optimizeShader => Shader After GAL Optimization: " << endl;
//shProgramImp->printASM(cout);
//cout << endl;

        libGAL_opt::OPTIMIZATION_OUTPUT_INFO optOutInfo;
        _shOptimizer.getOptOutputInfo(optOutInfo);

        gal_uint maxAliveTemps = optOutInfo.maxAliveTemps;

        //  Perform optimization/translation/transformation of the shader using the GPU Driver
        //  functions.

        //  Allocate buffer for the driver optimized code.
        gal_uint driverOptCodeSize = optCodeSize * 10;
        U08 *driverOptCode = new gal_ubyte[driverOptCodeSize];

        ////////////////////////////////////////////////////////////////////////////////////////////
        //  Prepare the rendering state information required for the microtriangle transformation.
        ////////////////////////////////////////////////////////////////////////////////////////////
        HAL::MicroTriangleRasterSettings settings;
        
        bool frontIsCCW = (_rast->_faceMode == GAL_FACE_CCW);
        bool frontCulled = ((_rast->_cullMode == GAL_CULL_FRONT) || (_rast->_cullMode == GAL_CULL_FRONT_AND_BACK));
        bool backCulled = ((_rast->_cullMode == GAL_CULL_BACK) || (_rast->_cullMode == GAL_CULL_FRONT_AND_BACK));

        settings.faceCullEnabled = _rast->_cullMode == GAL_CULL_NONE;

        if (frontIsCCW)
        {
            settings.CCWFaceCull = frontCulled;
            settings.CWFaceCull = backCulled;
        }
        else // backface is CCW
        {
            settings.CCWFaceCull = backCulled;
            settings.CWFaceCull = frontCulled;
        }

        //  For now GAL doesn�t known anything about perspective transformation. A perspective/orthogonal
        //  projection is done according to the constants used by the vertex shader. Since the vertex shader
        //  code and constants are user-specified we cannot determine the type of perspective.
        //  The future solution will be to request the GAL user to specify the type of vertex projection.
        settings.zPerspective = true;

        //  Set attribute interpolation.
        for (unsigned int attr = 0; attr < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; attr++)
        {
            switch(_rast->_interpolation[attr])
            {
                case GAL_INTERPOLATION_NONE:
                    settings.smoothInterp[attr] = false;
                    settings.perspectCorrectInterp[attr] = false;
                    break;
                case GAL_INTERPOLATION_LINEAR:
                    settings.smoothInterp[attr] = true;
                    settings.perspectCorrectInterp[attr] = true;
                    break;
                case GAL_INTERPOLATION_NOPERSPECTIVE:
                    settings.smoothInterp[attr] = true;
                    settings.perspectCorrectInterp[attr] = false;
                    break;
                default:
                    CG_ASSERT("Unknown/unsupported interpolation mode");
            }
        }

        ////////////////////////////////////////////////////////////////////////////

        // Use the GPU driver shader program capabilities.
        _driver->translateShaderProgram((U08 *) optCode, optCodeSize, driverOptCode, driverOptCodeSize,
                                       (shType == GAL_VERTEX_SHADER), maxAliveTemps, settings);

        shProgramImp->setOptimizedCode(driverOptCode, driverOptCodeSize);

//cout << "GALDeviceImp:_optimizeShader => Shader After Driver Optimization: " << endl;
//shProgramImp->printASM(cout);
//cout << endl;

        //  Delete buffers.
        delete[] driverOptCode;

        constantBank = _shOptimizer.getConstants();

        for (gal_uint i=0; i < 96; i++)
        {
            shProgramImp->setConstant(i, constantBank[i].c_array());
        }

        for (gal_uint i=0; i < GALShaderProgramImp::MAX_SHADER_ATTRIBUTES; i++)
        {
            shProgramImp->setInputRead(i, optOutInfo.inputsRead[i]);
            shProgramImp->setOutputWritten(i, optOutInfo.outputsWritten[i]);
        }

        shProgramImp->setMaxAliveTemps( maxAliveTemps );
    }
}

GALStoredState* GALDeviceImp::saveState(GALStoredItemIDList siIds) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    
    GALStoredStateImp* ret = new GALStoredStateImp();
    GALStoredItemIDList::const_iterator it = siIds.begin();

    for ( ; it != siIds.end(); ++it )
    {
        if ((*it) < GAL_ZST_FIRST_ID)
        {
            // Its a rasterization stage
            ret->addStoredStateItem(_rast->createStoredStateItem((*it)));
        }
        else if ((*it) < GAL_BLENDING_FIRST_ID)
        {
            // Its a z stencil stage
            ret->addStoredStateItem(_zStencil->createStoredStateItem((*it)));
        }
        else if ((*it) < GAL_LAST_STATE)
        {
            // Its a blending stage
            ret->addStoredStateItem(_blending->createStoredStateItem((*it)));
        }
        else
            CG_ASSERT("Unexpected Stored Item Id");
    }

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

GALStoredState* GALDeviceImp::saveState(GAL_STORED_ITEM_ID stateId) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    GALStoredStateImp* ret = new GALStoredStateImp();

    if (stateId < GAL_ZST_FIRST_ID)
    {
        // Its a rasterization stage
        ret->addStoredStateItem(_rast->createStoredStateItem(stateId));
    }
    else if (stateId < GAL_BLENDING_FIRST_ID)
    {
        // Its a z stencil stage
        ret->addStoredStateItem(_zStencil->createStoredStateItem(stateId));
    }
    else if (stateId < GAL_LAST_STATE)
    {
        // Its a blending stage
        ret->addStoredStateItem(_blending->createStoredStateItem(stateId));
    }
    else
        CG_ASSERT("Unexpected Stored Item Id");

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

GALStoredState* GALDeviceImp::saveAllState() const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i=0; i < GAL_LAST_STATE; i++)
    {
        if (i < GAL_ZST_FIRST_ID)
        {
            // Its a rasterization stage
            ret->addStoredStateItem(_rast->createStoredStateItem(GAL_STORED_ITEM_ID(i)));
        }
        else if (i < GAL_BLENDING_FIRST_ID)
        {
            // Its a z stencil stage
            ret->addStoredStateItem(_zStencil->createStoredStateItem(GAL_STORED_ITEM_ID(i)));
        }
        else if (i < GAL_LAST_STATE)
        {
            // Its a blending stage
            ret->addStoredStateItem(_blending->createStoredStateItem(GAL_STORED_ITEM_ID(i)));
        }
        else
            CG_ASSERT("Unexpected Stored Item Id");
    }

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

void GALDeviceImp::restoreState(const GALStoredState* state)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    
    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = ssi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();

    while ( iter != ssiList.end() )
    {
        const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(*iter);

        if (galssi->getItemId() < GAL_ZST_FIRST_ID)
        {
            // Its a rasterization stage
            _rast->restoreStoredStateItem(galssi);
        }
        else if (galssi->getItemId() < GAL_BLENDING_FIRST_ID)
        {
            // Its a z stencil stage
            _zStencil->restoreStoredStateItem(galssi);

        }
        else if (galssi->getItemId() < GAL_LAST_STATE)
        {
            // Its a blending stage
            _blending->restoreStoredStateItem(galssi);
        }
        else
            CG_ASSERT("Unexpected Stored Item Id");

        iter++;
    }

    GLOBAL_PROFILER_EXIT_REGION()
}

void GALDeviceImp::setStartFrame(gal_uint startFrame)
{
    _startFrame = startFrame;
    _currentFrame = 0;
    _currentBatch = 0;
}

void GALDeviceImp::destroyState(GALStoredState* state)
{
    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> stateList = ssi->getSSIList();

    std::list<const StoredStateItem*>::iterator iter = stateList.begin();

    while ( iter != stateList.end() )
    {
        delete (*iter);
        iter++;
    }

    delete ssi;
}

void GALDeviceImp::DBG_dump(const gal_char* file, gal_enum flags)
{
    _dump(file, flags);
}

void GALDeviceImp::DBG_deferred_dump(const gal_char* file, gal_enum flags, gal_uint frame, gal_uint batch)
{
    // Avoid deferring dumps for already past events.

    if (frame < _currentFrame)
        return;

    if (frame == _currentFrame)
    {
        if (batch < _currentBatch)
            return;
    }

    // The frame and batch event is in the future (not in current frame and batch)

    DumpEventInfo newEvent(file, flags, frame, batch);
    _dumpEventList.push_back(newEvent);

    // Update the _nextDumpEvent attributes for faster searches.

    if (_nextDumpEvent.valid)
    {
        if (frame < _nextDumpEvent.frame)
        {
            _nextDumpEvent = newEvent;
        }
        else if (frame == _nextDumpEvent.frame && batch < _nextDumpEvent.batch)
        {
            _nextDumpEvent = newEvent;
        }
    }
    else
        _nextDumpEvent = newEvent;
}

gal_bool GALDeviceImp::DBG_save(const gal_char* file)
{
    return true;
}

gal_bool GALDeviceImp::DBG_load(const gal_char* file)
{
    return true;
}

void GALDeviceImp::HACK_setPreloadMemory(gal_bool enablePreload)
{
    _driver->setPreloadMemory(enablePreload);
}

const char* GALDeviceImp::PrimitivePrint::print(const GAL_PRIMITIVE& primitive) const
{
    switch ( primitive )
    {
        case GAL_POINTS:
            return "GAL_POINTS";
        case GAL_LINE_LOOP:
            return "GAL_LINE_LOOP";
        case GAL_LINE_STRIP:
            return "GAL_LINE_STRIP";
        case GAL_TRIANGLES:
            return "GAL_TRIANGLES";
        case GAL_TRIANGLE_STRIP:
            return "GAL_TRIANGLE_STRIP";
        case GAL_TRIANGLE_FAN:
            return "GAL_TRIANGLE_FAN";
        case GAL_QUADS:
            return "GAL_QUADS";
        case GAL_QUAD_STRIP:
            return "GAL_QUAD_STRIP";
        case GAL_PRIMITIVE_UNDEFINED:
        default:
            return "GAL_PRIMITIVE_UNDEFINED";
    }
}

gal_bool GALDeviceImp::DBG_printMemoryUsage()
{
    _driver->printMemoryUsage();
    return true;
}

void GALDeviceImp::_syncHZRegister()
{
    //////////////////////////////////////////////////////////////////////////////////////////
    // Code based on previous version in Legacy OGL14, AuxfuncLib.cpp, function setHZTest() //
    //////////////////////////////////////////////////////////////////////////////////////////
    gal_bool activateHZ = false;

    // Check if z test is enabled
    if ( _zStencil->isZEnabled() ) {

        // Get the current Z compare function
        GAL_COMPARE_FUNCTION depthFunc = _zStencil->getZFunc();

        // Determine if HZ must be activated.
        activateHZ = (_hzBufferValid && ( depthFunc == GAL_COMPARE_FUNCTION_LESS
                      || depthFunc == GAL_COMPARE_FUNCTION_LESS_EQUAL || depthFunc == GAL_COMPARE_FUNCTION_EQUAL ));

        // Check if stencil test is enabled
        if ( _zStencil->isStencilEnabled() ) {

            // Get the stencil operations for the different ZStencil events
            GAL_STENCIL_OP stFail, dpFail, dpPass;

            _zStencil->getStencilOp(GAL_FACE_FRONT, stFail, dpFail, dpPass); // or back or what ?

            // Check consistency, front & back are faces with the same values (this check maybe is not required)
            GAL_STENCIL_OP dummyStFail, dummyDpFail, dummyDpPass;

            _zStencil->getStencilOp(GAL_FACE_BACK, dummyStFail, dummyDpFail, dummyDpPass);

            //if ( stFail != dummyStFail || dpFail != dummyDpFail || dpPass != dummyDpPass)
            //    CG_ASSERT("FRONT and BACK stencil operation differs");

            // Determine if HZ must be activated
            activateHZ = (activateHZ && stFail == GAL_STENCIL_OP_KEEP && dpFail == GAL_STENCIL_OP_KEEP);
        }

        // Check if HZ buffer content is invalid
        if ( _zStencil->getZMask() && (depthFunc == GAL_COMPARE_FUNCTION_ALWAYS
             || depthFunc == GAL_COMPARE_FUNCTION_GREATER || depthFunc == GAL_COMPARE_FUNCTION_EQUAL
             || depthFunc == GAL_COMPARE_FUNCTION_NOT_EQUAL) )
        {
            // Set HZ buffer content as not valid.
            _hzBufferValid = false;
        }
    }

    _hzActivated = activateHZ;

    if ( _hzActivated.changed() ) {
        cg1gpu::GPURegData data;
        data.booleanVal = _hzActivated;
        _driver->writeGPURegister(cg1gpu::GPU_HIERARCHICALZ, data);
        _hzActivated.restart();
    }
}

void GALDeviceImp::alphaTestEnabled(gal_bool enabled)
{
    _alphaTest = enabled;
}

const StoredStateItem* GALDeviceImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")

    GALStoredStateItem* ret;
    gal_uint aux;

    if (stateId >= GAL_DEVICE_FIRST_ID && stateId < GAL_DEV_LAST)
    {  
        if ((stateId >= GAL_DEV_VERTEX_ATTRIBUTE_MAP) && (stateId < GAL_DEV_VERTEX_ATTRIBUTE_MAP + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_VERTEX_ATTRIBUTE_MAP;
            ret = new GALSingleUintStoredStateItem(_vaMap[aux]);
        }
        else if ((stateId >= GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE) && (stateId < GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE;
            ret = new GALSingleBoolStoredStateItem(_vshOutputs[aux]);
        }
            else if ((stateId >= GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES) && (stateId < GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES;
            ret = new GALSingleBoolStoredStateItem(_fshInputs[aux]);
        }
        else 
        {
            switch(stateId)
            {
                case GAL_DEV_FRONT_BUFFER:              ret = new GALSingleVoidStoredStateItem((void*)(_defaultFrontBuffer));   break;
                case GAL_DEV_BACK_BUFFER:				ret = new GALSingleVoidStoredStateItem((void*)(_defaultBackBuffer));    break;
                case GAL_DEV_PRIMITIVE:                 ret = new GALSingleEnumStoredStateItem((gal_enum)(_primitive));	        break;
                case GAL_DEV_CURRENT_COLOR:             ret = new GALFloatVector4StoredStateItem(_currentColor);                break;
                case GAL_DEV_HIERARCHICALZ:             ret = new GALSingleBoolStoredStateItem((gal_enum)(_hzActivated));       break;
                case GAL_DEV_STREAM_START:              ret = new GALSingleUintStoredStateItem((gal_enum)(_streamStart));       break;
                case GAL_DEV_STREAM_COUNT:              ret = new GALSingleUintStoredStateItem((gal_enum)(_streamCount));       break;
                case GAL_DEV_STREAM_INSTANCES:          ret = new GALSingleUintStoredStateItem((gal_enum)(_streamInstances));   break;
                case GAL_DEV_INDEX_MODE:                ret = new GALSingleBoolStoredStateItem((gal_enum)(_indexedMode));       break;
                case GAL_DEV_EARLYZ:                    ret = new GALSingleBoolStoredStateItem((gal_enum)(_earlyZ));            break;
                case GAL_DEV_VERTEX_THREAD_RESOURCES:   ret = new GALSingleUintStoredStateItem((gal_enum)(_vshResources));      break;
                case GAL_DEV_FRAGMENT_THREAD_RESOURCES: ret = new GALSingleUintStoredStateItem((gal_enum)(_fshResources));      break;
                case GAL_DEV_VSH:                       ret = new GALSingleVoidStoredStateItem((void*)(_vsh));                  break;
                case GAL_DEV_FSH:                       ret = new GALSingleVoidStoredStateItem((void*)(_fsh));                  break;


                // case GAL_RASTER_... <- add here future additional rasterization states.
                default:
                    cout << stateId << endl;
                    CG_ASSERT("Unknown device state");
                    ret = 0;
            }
        }
    }
    else
        CG_ASSERT("Unexpected device state id");

    ret->setItemId(stateId);

    GLOBAL_PROFILER_EXIT_REGION()
    return ret;
}


#define CAST_TO_UINT(X)     *(static_cast<const GALSingleUintStoredStateItem*>(X))
#define CAST_TO_BOOL(X)     *(static_cast<const GALSingleBoolStoredStateItem*>(X))
#define CAST_TO_VECT4(X)    *(static_cast<const GALFloatVector4StoredStateItem*>(X))

void GALDeviceImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")

    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);
    gal_uint aux;

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    if (stateId >= GAL_DEVICE_FIRST_ID && stateId < GAL_DEV_LAST)
    {  
        if ((stateId >= GAL_DEV_VERTEX_ATTRIBUTE_MAP) && (stateId < GAL_DEV_VERTEX_ATTRIBUTE_MAP + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_VERTEX_ATTRIBUTE_MAP;
            _vaMap[aux] = *(static_cast<const GALSingleUintStoredStateItem*>(galssi));
        }
        else if ((stateId >= GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE) && (stateId < GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE;
            _vshOutputs[aux] = *(static_cast<const GALSingleBoolStoredStateItem*>(galssi));
        }
        else if ((stateId >= GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES) && (stateId < GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            aux = stateId - GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES;
            _fshInputs[aux] = *(static_cast<const GALSingleBoolStoredStateItem*>(galssi));
        }
        else
        {
            switch(stateId)
            {
                case GAL_DEV_PRIMITIVE:                 
                    {
                        gal_enum param = *(static_cast<const GALSingleEnumStoredStateItem*>(galssi));
                        _primitive = GAL_PRIMITIVE(param);
                        break;
                    }

                case GAL_DEV_CURRENT_COLOR:             _currentColor = CAST_TO_VECT4(galssi);  break;
                case GAL_DEV_HIERARCHICALZ:             _hzActivated = CAST_TO_BOOL(galssi);    break;
                case GAL_DEV_STREAM_START:              _streamStart = CAST_TO_UINT(galssi);    break;
                case GAL_DEV_STREAM_COUNT:              _streamCount = CAST_TO_UINT(galssi);    break;
                case GAL_DEV_STREAM_INSTANCES:          _streamInstances = CAST_TO_UINT(galssi);    break;
                case GAL_DEV_INDEX_MODE:                _indexedMode = CAST_TO_BOOL(galssi);    break;
                case GAL_DEV_EARLYZ:                    _earlyZ = CAST_TO_BOOL(galssi);         break;
                case GAL_DEV_VERTEX_THREAD_RESOURCES:   _vshResources = CAST_TO_UINT(galssi);   break;
                case GAL_DEV_FRAGMENT_THREAD_RESOURCES: _fshResources = CAST_TO_UINT(galssi);   break;
                case GAL_DEV_VSH:                       
                    {
                        void* param = *(static_cast<const GALSingleVoidStoredStateItem*>(galssi));
                        _vsh = (GALShaderProgramImp*)(param);            
                        break;
                    }
                case GAL_DEV_FSH: 
                    {
                        void* param = *(static_cast<const GALSingleVoidStoredStateItem*>(galssi));
                        _fsh = (GALShaderProgramImp*)(param);   
                        break;
                    }
				case GAL_DEV_FRONT_BUFFER:
					{
						void* param = *(static_cast<const GALSingleVoidStoredStateItem*>(galssi));
						_defaultFrontBuffer = (GALRenderTargetImp*)(param);
						break;
					}
                case GAL_DEV_BACK_BUFFER:
					{
						void* param = *(static_cast<const GALSingleVoidStoredStateItem*>(galssi));
						_defaultBackBuffer = (GALRenderTargetImp*)(param);
						break;
					}


                // case GAL_RASTER_... <- add here future additional rasterization states.
                default:
                    CG_ASSERT("Unknown rasterization state");
            }
        }
    }
    else
        CG_ASSERT("Unexpected raster state id");

    GLOBAL_PROFILER_EXIT_REGION()
}

#undef CAST_TO_UINT
#undef CAST_TO_BOOL
#undef CAST_TO_VECT4

GALStoredState* GALDeviceImp::saveAllDeviceState() const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i = 0; i < GAL_DEV_LAST; i++)
        ret->addStoredStateItem(createStoredStateItem(GAL_STORED_ITEM_ID(i)));

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

void GALDeviceImp::restoreAllDeviceState(const GALStoredState* state)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = ssi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();

    while ( iter != ssiList.end() )
    {
        const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(*iter);
        restoreStoredStateItem(galssi);
        iter++;
    }

    GLOBAL_PROFILER_EXIT_REGION()
}

void GALDeviceImp::copyMipmap (GALTexture* inTexture, libGAL::GAL_CUBEMAP_FACE inFace, gal_uint inMipmap, gal_uint inX, gal_uint inY, gal_uint inWidth, gal_uint inHeight, 
                               GALTexture* outTexture, libGAL::GAL_CUBEMAP_FACE outFace, gal_uint outMipmap, gal_uint outX, gal_uint outY, gal_uint outWidth, gal_uint outHeight, 
							   GAL_TEXTURE_FILTER minFilter, GAL_TEXTURE_FILTER magFilter)
{

    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

	TextureAdapter adaptedIn(inTexture);
	TextureAdapter adaptedOut(outTexture);
	GAL_FORMAT texFormat;

	if(adaptedIn.getFormat(inFace, inMipmap) != adaptedOut.getFormat(outFace, outMipmap))
		CG_ASSERT("Different format beetween input and output textures"); // Maybe it is possible to perform a conversion in case textures have different formats
    else
        texFormat = adaptedIn.getFormat(inFace, inMipmap);

	if(inX < 0 || inY < 0)
		CG_ASSERT("Start point of the portion to copy is outside the mipmap range of the input texture");
			
	if(outX < 0 || outY < 0)
		CG_ASSERT("Start point of the portion to copy is outside the mipmap range of the output texture");
	
	if(adaptedIn.getWidth(inFace, inMipmap) > inX + inWidth || adaptedIn.getHeight(inFace, inMipmap) > inY + inHeight)
		CG_ASSERT("The region to copy of the input texture is partially or totally out of the texture mipmap");
		
	if(adaptedOut.getWidth(outFace, outMipmap) > outX + outWidth || adaptedOut.getHeight(outFace, outMipmap) > outY + outHeight)
		CG_ASSERT("The region to copy of the output texture is partially or totally out of the texture mipmap");	
		
    GALRenderTargetImp* outRT = new GALRenderTargetImp(this, outTexture, GAL_RT_DIMENSION_TEXTURE2D, inFace, inMipmap);
    
    cg1gpu::GPURegData data;
    vector<cg1gpu::GPURegData> savedRegState;
    vector<const StoredStateItem*> storedState;


    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_VIEWPORT));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_TEST));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_X));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_Y));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_WIDTH));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_HEIGHT));

    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_WRITE_ENABLED));

    storedState.push_back(createStoredStateItem(GAL_DEV_FRONT_BUFFER));
    storedState.push_back(createStoredStateItem(GAL_DEV_BACK_BUFFER));
    storedState.push_back(createStoredStateItem(GAL_DEV_PRIMITIVE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VERTEX_THREAD_RESOURCES));
    storedState.push_back(createStoredStateItem(GAL_DEV_FRAGMENT_THREAD_RESOURCES));
    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_COUNT));
    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_START));
    storedState.push_back(createStoredStateItem(GAL_DEV_INDEX_MODE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VSH));
    storedState.push_back(createStoredStateItem(GAL_DEV_FSH));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_ATTRIBUTE_MAP + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES + i)));

    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_STRIDE));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_ELEMENTS));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_DATA_TYPE));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_BUFFER));

    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_ENABLED));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_S));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_T));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_R));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MIN_FILTER));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MAG_FILTER));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MIN_LOD));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MAX_LOD));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_LOD_BIAS));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_UNIT_LOD_BIAS));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_TEXTURE));

    _driver->readGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, data);
    savedRegState.push_back(data);

    gal_uint inWidthTexture = adaptedIn.getWidth(inFace, inMipmap);
    gal_uint inHeightTexture = adaptedIn.getHeight(inFace, inMipmap);

    gal_uint outWidthTexture = adaptedOut.getWidth(inFace, inMipmap);
    gal_uint outHeightTexture = adaptedOut.getHeight(inFace, inMipmap);


    libGAL::gal_float dataBuffer[] = {  ((outX*2)/outWidthTexture)-1, ((outY)*2)/outHeightTexture-1, 0.f, inX/inWidthTexture, (inY+inHeight)/inHeightTexture,
                                        (((outX+outWidth)*2)/outWidthTexture)-1, ((outY)*2)/outHeightTexture-1, 0.f, (inX+inWidth)/inWidthTexture, (inY+inHeight)/inHeightTexture,
                                        (((outX+outWidth)*2)/outWidthTexture)-1, (((outY+outHeight)*2)/outHeightTexture)+1, 0.f, (inX+inWidth)/inWidthTexture, inY/inHeightTexture,
                                        ((outX*2)/outWidthTexture)-1, (((outY+outHeight)*2)/outHeightTexture+1), 0.f, inX/inWidthTexture, inY/inHeightTexture};

    libGAL::gal_uint indexBuffer[] = {0, 1, 2, 2, 3, 0};

    libGAL::gal_ubyte vertexShader[] = "mov o0, i0\n"
                                       "move o5, i1\n";
    
    libGAL::gal_ubyte fragmentShader[] = "tex r0, i5, t0\n"
                                         "mov o1, r0\n";
    
    GALBuffer* dataGALBuffer = createBuffer(sizeof(libGAL::gal_float) * 5 * 4, (libGAL::gal_ubyte *)&dataBuffer);


    GAL_STREAM_DESC streamDesc;
    streamDesc.offset = 0;
    streamDesc.stride = sizeof(libGAL::gal_float) * 5;
    streamDesc.components = 3;
    streamDesc.frequency = 0;
    streamDesc.type = GAL_SD_FLOAT32;

    // Set the vertex buffer to GAL
    stream(0).set(dataGALBuffer, streamDesc);
    enableVertexAttribute(0, 0);


    streamDesc.offset = sizeof(libGAL::gal_float) * 3;
    streamDesc.stride = sizeof(libGAL::gal_float) * 5;
    streamDesc.components = 2;
    streamDesc.frequency = 0;
    streamDesc.type = libGAL::GAL_SD_FLOAT32;

    // Set the vertex buffer to GAL
    stream(1).set(dataGALBuffer, streamDesc);
    enableVertexAttribute(1, 1);


    GALBuffer* indexGALBuffer = createBuffer((sizeof (libGAL::gal_uint)) * 6, (libGAL::gal_ubyte *)&indexBuffer);
    setIndexBuffer(indexGALBuffer, 0, libGAL::GAL_SD_UINT32);


    // Set source surface as a Texture
    sampler(0).setEnabled(true);
    sampler(0).setTexture(inTexture);
    sampler(0).setMagFilter(magFilter);
    sampler(0).setMinFilter(minFilter);
    sampler(0).setMaxAnisotropy(1);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_S_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_T_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_R_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);

    GALShaderProgram* vertexProgram = createShaderProgram();
    GALShaderProgram* fragmentProgram = createShaderProgram();

    vertexProgram->setProgram(vertexShader);
    fragmentProgram->setProgram(fragmentShader);

    setVertexShader(vertexProgram);
    setFragmentShader(fragmentProgram);

    setRenderTarget(0, outRT);
    setZStencilBuffer(NULL);

    rast().setCullMode(libGAL::GAL_CULL_NONE);
    setPrimitive(libGAL::GAL_TRIANGLES);


    drawIndexed(0, 6, 0, 0, 0);

    gal_int nextReg = 0;

    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);

    _blending->restoreStoredStateItem(storedState[nextReg++]);

    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);

    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);

    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, savedRegState[0]);



}

void GALDeviceImp::copyTexture2RenderTarget(GALTexture2DImp* texture, GALRenderTargetImp* renderTarget)
{
    cg1gpu::GPURegData data;
    vector<cg1gpu::GPURegData> savedRegState;
    vector<const StoredStateItem*> storedState;


    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_VIEWPORT));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_TEST));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_X));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_Y));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_WIDTH));
    storedState.push_back(_rast->createStoredStateItem(GAL_RASTER_SCISSOR_HEIGHT));

    storedState.push_back(_blending->createStoredStateItem(GAL_BLENDING_COLOR_WRITE_ENABLED));

    storedState.push_back(createStoredStateItem(GAL_DEV_FRONT_BUFFER));
    storedState.push_back(createStoredStateItem(GAL_DEV_BACK_BUFFER));
    storedState.push_back(createStoredStateItem(GAL_DEV_PRIMITIVE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VERTEX_THREAD_RESOURCES));
    storedState.push_back(createStoredStateItem(GAL_DEV_FRAGMENT_THREAD_RESOURCES));
    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_COUNT));
    storedState.push_back(createStoredStateItem(GAL_DEV_STREAM_START));
    storedState.push_back(createStoredStateItem(GAL_DEV_INDEX_MODE));
    storedState.push_back(createStoredStateItem(GAL_DEV_VSH));
    storedState.push_back(createStoredStateItem(GAL_DEV_FSH));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_ATTRIBUTE_MAP + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_VERTEX_OUTPUT_ATTRIBUTE + i)));

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        storedState.push_back(createStoredStateItem(GAL_STORED_ITEM_ID(GAL_DEV_FRAGMENT_INPUT_ATTRIBUTES + i)));

    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_STRIDE));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_ELEMENTS));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_DATA_TYPE));
    storedState.push_back(_stream[0]->createStoredStateItem(GAL_STREAM_BUFFER));

    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_ENABLED));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_S));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_T));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_CLAMP_R));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MIN_FILTER));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MAG_FILTER));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MIN_LOD));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_MAX_LOD));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_LOD_BIAS));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_UNIT_LOD_BIAS));
    storedState.push_back(_sampler[0]->createStoredStateItem(GAL_SAMPLER_TEXTURE));

    _driver->readGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, data);
    savedRegState.push_back(data);


    libGAL::gal_float dataBuffer[] = {  -1.f, -1.f, 0.f, 0.f, 1.f,
                                        1.f, -1.f, 0.f, 1.f, 1.f,
                                        1.f, 1.f, 0.f, 1.f, 0.f,
                                        -1.f, 1.f, 0.f, 0.f, 0.f};

    libGAL::gal_uint indexBuffer[] = {0, 1, 2, 2, 3, 0};

    libGAL::gal_ubyte vertexShader[] = "mov o0, i0\n"
                                       "mov o5, i1\n";
                                       
    libGAL::gal_ubyte fragmentShader[] = "tex r0, i5, t0\n"
                                         "mov o1, r0\n";

    GALBuffer* dataGALBuffer = createBuffer(sizeof(libGAL::gal_float) * 5 * 4, (libGAL::gal_ubyte *)&dataBuffer);


    GAL_STREAM_DESC streamDesc;
    streamDesc.offset = 0;
    streamDesc.stride = sizeof(libGAL::gal_float) * 5;
    streamDesc.components = 3;
    streamDesc.frequency = 0;
    streamDesc.type = GAL_SD_FLOAT32;

    // Set the vertex buffer to GAL
    stream(0).set(dataGALBuffer, streamDesc);
    enableVertexAttribute(0, 0);


    streamDesc.offset = sizeof(libGAL::gal_float) * 3;
    streamDesc.stride = sizeof(libGAL::gal_float) * 5;
    streamDesc.components = 2;
    streamDesc.frequency = 0;
    streamDesc.type = libGAL::GAL_SD_FLOAT32;

    // Set the vertex buffer to GAL
    stream(1).set(dataGALBuffer, streamDesc);
    enableVertexAttribute(1, 1);


    GALBuffer* indexGALBuffer = createBuffer((sizeof (libGAL::gal_uint)) * 6, (libGAL::gal_ubyte *)&indexBuffer);
    setIndexBuffer(indexGALBuffer, 0, libGAL::GAL_SD_UINT32);


    // Set source surface as a Texture
    sampler(0).setEnabled(true);
    sampler(0).setTexture(texture);
    sampler(0).setMagFilter(GAL_TEXTURE_FILTER_NEAREST);
    sampler(0).setMinFilter(GAL_TEXTURE_FILTER_NEAREST);
    sampler(0).setMaxAnisotropy(1);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_S_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_T_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    sampler(0).setTextureAddressMode(GAL_TEXTURE_R_COORD, GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);

    GALShaderProgram* vertexProgram = createShaderProgram();
    GALShaderProgram* fragmentProgram = createShaderProgram();

    vertexProgram->setProgram(vertexShader);
    fragmentProgram->setProgram(fragmentShader);

    setVertexShader(vertexProgram);
    setFragmentShader(fragmentProgram);

    setRenderTarget(0, renderTarget);
    setZStencilBuffer(NULL);

    rast().setCullMode(libGAL::GAL_CULL_NONE);
    setPrimitive(libGAL::GAL_TRIANGLES);


    drawIndexed(0, 6, 0, 0, 0);

    gal_int nextReg = 0;

    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);
    _rast->restoreStoredStateItem(storedState[nextReg++]);

    _blending->restoreStoredStateItem(storedState[nextReg++]);

    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);
    restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_VERTEX_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    for (gal_uint i = 0; i < cg1gpu::MAX_FRAGMENT_ATTRIBUTES; i++)
        restoreStoredStateItem(storedState[nextReg++]);

    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);
    _stream[0]->restoreStoredStateItem(storedState[nextReg++]);

    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);
    _sampler[0]->restoreStoredStateItem(storedState[nextReg++]);

    _driver->writeGPURegister(cg1gpu::GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, cg1gpu::COLOR_ATTRIBUTE, savedRegState[0]);
}

GALRenderTarget* GALDeviceImp::getFrontBufferRT()
{
    return _defaultFrontBuffer;
}

GALRenderTarget* GALDeviceImp::getBackBufferRT()
{
    return _defaultBackBuffer;
}

gal_uint GALDeviceImp::getCurrentFrame()
{
    return _currentFrame;
}

gal_uint GALDeviceImp::getCurrentBatch()
{
    return _currentBatch;
}

void GALDeviceImp::performBlitOperation2D(gal_uint samplerID, gal_uint xoffset, gal_uint yoffset,
                                          gal_uint x, gal_uint y, gal_uint width, gal_uint height,
                                          gal_uint textureWidth, GAL_FORMAT internalFormat,
                                          GALTexture2D* texture, gal_uint level)
{
    //  Syncronize render buffer state.
    _syncRenderBuffers();
    
    //  Perform the blit operation.
    sampler(samplerID).performBlitOperation2D(xoffset, yoffset, x, y, width, height, textureWidth, internalFormat, texture, level);
}

void GALDeviceImp::setColorSRGBWrite(libGAL::gal_bool enable)
{
    //  Set the color conversion from linear to sRGB on color write flag.
    _colorSRGBWrite = enable;
}

gal_ubyte* GALDeviceImp::compressTexture(GAL_FORMAT originalFormat, GAL_FORMAT compressFormat, gal_uint width, gal_uint height, gal_ubyte* originalData, gal_uint selectionMode)
{
    gal_ubyte* data;
    gal_ubyte* compressedData = 0;
    gal_uint maxWidthBlocks = std::floor(gal_double(width/4));
    gal_uint maxHeightBlocks = std::floor(gal_double(height/4));
    gal_uint bestDifference = -1;
    gal_uint difference;
    gal_uint pixelsRowSize;
    gal_uint code = 0;
    gal_uint k;
    gal_ubyte color0_R, color0_G, color0_B;
    gal_ubyte color1_R, color1_G, color1_B;


    switch (compressFormat)
    {

        case GAL_COMPRESSED_S3TC_DXT1_RGB:
            {
            if (originalFormat != GAL_FORMAT_RGBA_8888 && originalFormat != GAL_FORMAT_RGB_888)
                CG_ASSERT("By the time you can only compress GAL_FORMAT_RGBA_8888 textures");

            compressedData = new gal_ubyte[maxWidthBlocks*maxHeightBlocks*8];

            k = 0;
            code = 0;
            
            pixelsRowSize = width * TextureMipmap::getTexelSize(originalFormat);

            for (gal_uint j = 0; j < maxHeightBlocks; j++) 
                for (gal_uint i = 0; i < maxWidthBlocks; i++)
                {

                    data = originalData + (gal_uint)(TextureMipmap::getTexelSize(originalFormat) * (width * j * 4 + i * 4));

                    switch (selectionMode)
                    {
                        case 0:
                            {
                                // In this mode color are selected statically. upper left corner and lower right corner

                                gal_uint texel = 0;
                                gal_uint compare = 15;

                                // Upper left corner
                                color0_R = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat))]);
                                color0_G = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
                                color0_B = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);

                                // Lower right corner
                                color1_R = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat))]);
                                color1_G = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
                                color1_B = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);
                            }
                            break;
                        case 1:
                        default:
                            {
                                for(gal_uint texel = 0; texel < 16; texel++)
                                    for(gal_uint compare = 0; compare < 16; compare++)
                                    {
                                        
                                        if (texel == compare)
                                            continue;

                                        difference = compressionDifference(texel, compare, pixelsRowSize, data, originalFormat);

                                        if (difference < bestDifference || bestDifference == -1)
                                        {
                                            bestDifference = difference;

                                            //getColor0
                                            color0_R = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat))]);
                                            color0_G = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
                                            color0_B = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);

                                            //getColor1
                                            color1_R = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat))]);
                                            color1_G = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
                                            color1_B = (((gal_ubyte* )(data+(pixelsRowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);

                                        }
                                    }
                            }
                    } 

                gal_ushort color0_R5G6B5, color1_R5G6B5;
                gal_ubyte texel_R, texel_G, texel_B;
                gal_uint tryR, tryG, tryB;
                gal_uint diffA, diffB, diffC, diffD;
                gal_ushort texel_R5G6B5;

                color0_R5G6B5 = (((gal_ushort)(color0_R & 0xF8)) << 8) | (((gal_ushort)(color0_G & 0xFC)) << 3) | (((gal_ushort)(color0_B & 0xF8)) >> 3);
                color1_R5G6B5 = (((gal_ushort)(color1_R & 0xF8)) << 8) | (((gal_ushort)(color1_G & 0xFC)) << 3) | (((gal_ushort)(color1_B & 0xF8)) >> 3);

                for (gal_uint texel = 0; texel < 16; texel++)
                {
                    texel_R = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat))]);
                    texel_G = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
                    texel_B = (((gal_ubyte* )(data+(pixelsRowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);

                    texel_R5G6B5 = (((gal_ushort)(texel_R & 0xF8)) << 8) | (((gal_ushort)(texel_G & 0xFC)) << 3) | (((gal_ushort)(texel_B & 0xF8)) >> 3);

                    if (color0_R5G6B5 < color1_R5G6B5)
                    {
                        diffA = abs((int)(color0_R-texel_R)) + abs((int)(color0_G-texel_G)) + abs((int)(color0_B-texel_B));
                        diffB = abs((int)(color1_R-texel_R)) + abs((int)(color1_G-texel_G)) + abs((int)(color1_B-texel_B));

                        tryR = 2/3*color0_R + color1_R/3;
                        tryG = 2/3*color0_G + color1_G/3;
                        tryB = 2/3*color0_B + color1_B/3;

                        diffC = abs((int)(tryR-texel_R)) + abs((int)(tryG-texel_G)) + abs((int)(tryB-texel_B));

                        tryR = color0_R/3 + 2/3*color1_R;
                        tryG = color0_G/3 + 2/3*color1_G;
                        tryB = color0_B/3 + 2/3*color1_B;

                        diffD = abs((int)(tryR-texel_R)) + abs((int)(tryG-texel_G)) + abs((int)(tryB-texel_B));

                        if (diffA <= diffB && diffA <= diffC && diffA <= diffD )
                            code = code | (0x00 << 2*texel);
                        else if (diffB <= diffA && diffB <= diffC && diffB <= diffD)
                            code = code | (0x01 << 2*texel);
                        else if (diffC <= diffA && diffC <= diffB && diffC <= diffD)
                            code = code | (0x02 << 2*texel);
                        else
                            code = code | (0x03 << 2*texel);
                    }
                    else
                    {
                        diffA = abs((int)(color0_R-texel_R)) + abs((int)(color0_G-texel_G)) + abs((int)(color0_B-texel_B));
                        diffB = abs((int)(color1_R-texel_R)) + abs((int)(color1_G-texel_G)) + abs((int)(color1_B-texel_B));

                        tryR = (color0_R + color1_R)/2;
                        tryG = (color0_G + color1_G)/2;
                        tryB = (color0_B + color1_B)/2;

                        diffC = abs((int)(tryR-texel_R)) + abs((int)(tryG-texel_G)) + abs((int)(tryB-texel_B));

                        diffD = texel_R + texel_G + texel_B;

                        if (diffA <= diffB && diffA <= diffC && diffA <= diffD )
                            code = code | (0x00 << 2*texel);
                        else if (diffB <= diffA && diffB <= diffC && diffB <= diffD)
                            code = code | (0x01 << 2*texel);
                        else if (diffC <= diffA && diffC <= diffB && diffC <= diffD)
                            code = code | (0x02 << 2*texel);
                        else
                            code = code | (0x03 << 2*texel);
                    }
                }

                /*cout << (gal_uint)(color0_R5G6B5) << endl;
                cout << (gal_uint)(color1_R5G6B5) << endl;
                cout << code << endl;*/
                compressedData[k] = color0_R5G6B5 & 0xFF;
                compressedData[k+1] = (color0_R5G6B5 & 0xFF00) >> 8;
                compressedData[k+2] = color1_R5G6B5 & 0xFF;
                compressedData[k+3] = (color1_R5G6B5 & 0xFF00) >> 8;
                compressedData[k+4] = code & 0xFF;
                compressedData[k+5] = (code & 0xFF00) >> 8;
                compressedData[k+6] = (code & 0xFF0000) >> 16;
                compressedData[k+7] = (code & 0xFF000000) >> 24;

                code = 0;
                bestDifference = -1;
                k = k + 8;

            }


        }
            break;

        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
             CG_ASSERT("111By the time you can only compress to GAL_COMPRESSED_S3TC_DXT1_RGB textures");
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
            CG_ASSERT("222By the time you can only compress to GAL_COMPRESSED_S3TC_DXT1_RGB textures");
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
        default:
            CG_ASSERT("333By the time you can only compress to GAL_COMPRESSED_S3TC_DXT1_RGB textures");
            break;

    }

    return (gal_ubyte*)compressedData;
}

gal_uint GALDeviceImp::compressionDifference(gal_uint texel, gal_uint compare, gal_uint rowSize, gal_ubyte* data, GAL_FORMAT originalFormat)
{

    gal_uint i = 0;

    gal_uint diffA, diffB, diffC, diffD;
    gal_uint totalDiff = 0;
    gal_ushort tryR, tryG, tryB;
    gal_ushort rc, gc, bc;
    gal_ushort rt, gt, bt;
    gal_ushort r,g,b;
    gal_ushort texelR5G6B5, compareR5G6B5;

    // Obtain each color component to obtain R5G6B5 format
    rt = (((gal_ubyte* )(data+(rowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat))]);
    gt = (((gal_ubyte* )(data+(rowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
    bt = (((gal_ubyte* )(data+(rowSize*(texel/4))))[(gal_uint)((texel%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);
    texelR5G6B5 = (((rt & 0xF8) << 8 ) | ((gt & 0xFC) << 3) | ((bt & 0xF8) >> 3)); 

    // Obtain each color component to obtain R5G6B5 format
    rc = (((gal_ubyte* )(data+(rowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat))]);
    gc = (((gal_ubyte* )(data+(rowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+1)]);
    bc = (((gal_ubyte* )(data+(rowSize*(compare/4))))[(gal_uint)((compare%4)*TextureMipmap::getTexelSize(originalFormat)+2)]);
    compareR5G6B5 = (((rc & 0xF8) << 8) | ((gc & 0xFC) << 3) | ((bc & 0xF8) >> 3)); 

    if (texelR5G6B5 < compareR5G6B5)
    {
        for (gal_uint height = 0; height < 4; height++)
        {
            for (gal_uint width = 0; width < 4; width++)
            {
                r = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat))];
                g = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat)+1)];
                b = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat)+2)];

                diffA = abs((int)(rt-r)) + abs((int)(gt-g)) + abs((int)(bt-b));
                diffB = abs((int)(rc-r)) + abs((int)(gc-g)) + abs((int)(bc-b));

                tryR = 2/3*rt + rc/3;
                tryG = 2/3*gt + gc/3;
                tryB = 2/3*bt + bc/3;

                diffC = abs((int)(tryR-r)) + abs((int)(tryG-g)) + abs((int)(tryB-b));

                tryR = rt/3 + 2/3*rc;
                tryG = gt/3 + 2/3*gc;
                tryB = bt/3 + 2/3*bc;

                diffD = abs((int)(tryR-r)) + abs((int)(tryG-g)) + abs((int)(tryB-b));

                if (diffA <= diffB && diffA <= diffC && diffA <= diffD )
                    totalDiff += diffA;
                else if (diffB <= diffA && diffB <= diffC && diffB <= diffD)
                    totalDiff += diffB;
                else if (diffC <= diffA && diffC <= diffB && diffC <= diffD)
                    totalDiff += diffC;
                else
                    totalDiff += diffD;

            }

            data += rowSize;
        }
    }
    else
    {
        for (gal_uint height = 0; height < 4; height++)
        {
            for (gal_uint width = 0; width < 4; width++)
            {
                r = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat))];
                g = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat)+1)];
                b = data[(gal_uint)(width*TextureMipmap::getTexelSize(originalFormat)+2)];

                tryR = (rt + r)/2;
                tryG = (bt + g)/2;
                tryB = (gt + b)/2;

                diffA = abs((int)(rt-r)) + abs((int)(gt-g)) + abs((int)(bt-b));
                diffB = abs((int)(rc-r)) + abs((int)(gc-g)) + abs((int)(bc-b));
                diffC = abs((int)(tryR-r)) + abs((int)(tryG-g)) + abs((int)(tryB-b));
                diffD = r + g + b;

                if (diffA <= diffB && diffA <= diffC && diffA <= diffD )
                    totalDiff += diffA;
                else if (diffB <= diffA && diffB <= diffC && diffB <= diffD)
                    totalDiff += diffB;
                else if (diffC <= diffA && diffC <= diffB && diffC <= diffD)
                    totalDiff += diffC;
                else
                    totalDiff += diffD;

            }

            data += rowSize;
        }

    }

    return totalDiff;

}
