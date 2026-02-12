
#ifndef GAL_DEVICE_IMP
#define GAL_DEVICE_IMP

#include "GALDevice.h"
#include "StateItem.h"
#include "MemoryObjectAllocator.h"
#include "StateItemUtils.h"
#include "HAL.h"
#include "ShaderOptimizer.h"
#include "GALStoredStateImp.h"
#include "GALStoredState.h"
#include "StoredStateItem.h"
#include "GALStoredItemID.h"
#include "GALTextureCubeMap.h"
#include "GALTexture2DImp.h"
#include "GALSampler.h"


#include <vector>
#include <set>
#include <string>


//#define GAL_DUMP_STREAMS
//#define GAL_DUMP_SAMPLERS

class HAL;

namespace libGAL
{

class GALBlendingStageImp;
class GALRasterizationStageImp;
class GALZStencilStageImp;
class GALStreamImp;
class GALSamplerImp;
class GALShaderProgramImp;
class GALRenderTargetImp;

class GALDeviceImp : public GALDevice
{
public:
    GALDeviceImp(HAL* driver);
    ~GALDeviceImp();
    virtual void setOptions(const GAL_CONFIG_OPTIONS& configOptions);
    virtual void setPrimitive(GAL_PRIMITIVE primitive);
    virtual GALBlendingStage& blending();
    virtual GALRasterizationStage& rast();
    virtual GALZStencilStage& zStencil();
    virtual GALTexture2D* createTexture2D();
    virtual GALTexture3D* createTexture3D();
    virtual GALTextureCubeMap* createTextureCubeMap();
    virtual GALBuffer* createBuffer(gal_uint size, const gal_ubyte* data);
    virtual gal_bool destroy(GALResource* resourcePtr);
    virtual gal_bool destroy(GALShaderProgram* shProgram);
    virtual void setResolution(gal_uint width, gal_uint height);
    virtual gal_bool getResolution(gal_uint& width, gal_uint& height);
    virtual GALStream& stream(gal_uint streamID);
    virtual void enableVertexAttribute(gal_uint vaIndex, gal_uint streamID);
    virtual void disableVertexAttribute(gal_uint vaIndex);
    virtual void disableVertexAttributes();
    virtual gal_uint availableStreams() const;
    virtual gal_uint availableVertexAttributes() const;
    virtual void setIndexBuffer( GALBuffer* ib, gal_uint offset, GAL_STREAM_DATA indicesType);
    virtual GALSampler& sampler(gal_uint samplerID);
    virtual void draw(gal_uint start, gal_uint count, gal_uint instances = 1);
    virtual void drawIndexed( gal_uint startIndex, gal_uint indexCount, gal_uint minIndex, gal_uint maxIndex, gal_int baseVertexIndex = 0, gal_uint instances = 1);
    virtual GALRenderTarget* createRenderTarget( GALTexture* resource, const GAL_RT_DIMENSION rtdimension, GAL_CUBEMAP_FACE face, gal_uint mipmap );
    virtual gal_bool setRenderTargets( gal_uint numRenderTargets, const GALRenderTarget* const * renderTargets, const GALZStencilTarget* zStencilTarget );
    virtual gal_bool setRenderTarget(gal_uint indexRenderTarget, GALRenderTarget *renderTarget);
    virtual gal_bool setZStencilBuffer(GALRenderTarget *renderTarget);
    virtual GALRenderTarget *getRenderTarget(gal_uint indexRenderTarget);
    virtual GALRenderTarget *getZStencilBuffer();
    virtual void updateResource( const GALResource* destResource, gal_uint destSubResource, const GAL_BOX* destRegion, const gal_ubyte* srcData, gal_uint srcRowPitch, gal_uint srcDepthPitch );
    virtual gal_bool swapBuffers();
    virtual void clearColorBuffer(gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha);
    virtual void clearZStencilBuffer( gal_bool clearZ, gal_bool clearStencil, gal_float zValue, gal_int stencilValue ); 
    virtual void clearRenderTarget(GALRenderTarget* rTarget, gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha); 
    virtual void clearZStencilTarget(GALZStencilTarget* zsTarget, gal_bool clearZ, gal_bool clearStencil, gal_float zValue, gal_ubyte stencilValue ); 
    virtual void copySurfaceDataToRenderBuffer(GALTexture2D *sourceTexture, gal_uint mipLevel, GALRenderTarget *destRenderTarget, bool preload);
    virtual void setColorSRGBWrite(gal_bool enable);
    virtual void getShaderLimits(GAL_SHADER_TYPE type, GAL_SHADER_LIMITS* limits);
    virtual GALShaderProgram* createShaderProgram() const;
    virtual void setGeometryShader(GALShaderProgram* program);
    virtual void setVertexShader(GALShaderProgram* program);
    virtual void setFragmentShader(GALShaderProgram* program);
    virtual void setVertexDefaultValue(gal_float currentColor[4]);
    virtual GALStoredState* saveState(GALStoredItemIDList siIds) const;
    virtual GALStoredState* saveState(GAL_STORED_ITEM_ID stateId) const;
    virtual GALStoredState* saveAllState() const;
    virtual void destroyState(GALStoredState* state);
    virtual void restoreState(const GALStoredState* state);
    virtual void setStartFrame(gal_uint startFrame);
    virtual void DBG_dump(const gal_char* file, gal_enum flags);
    virtual void DBG_deferred_dump(const gal_char* file, gal_enum flags, gal_uint frame, gal_uint batch);
    virtual gal_bool DBG_save(const gal_char* file);
    virtual gal_bool DBG_load(const gal_char* file);
    virtual gal_bool DBG_printMemoryUsage();
    virtual void alphaTestEnabled(gal_bool enabled);
    virtual GALRenderTarget* getFrontBufferRT();
    virtual GALRenderTarget* getBackBufferRT();
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
                                libGAL::GAL_TEXTURE_FILTER magFilter);

    virtual void performBlitOperation2D(gal_uint sampler, gal_uint xoffset, gal_uint yoffset,
                                        gal_uint x, gal_uint y, gal_uint width, gal_uint height,
                                        gal_uint textureWidth, GAL_FORMAT internalFormat,
                                        GALTexture2D* texture, gal_uint level);

    virtual gal_ubyte* compressTexture(GAL_FORMAT originalFormat, GAL_FORMAT compressFormat, gal_uint width, gal_uint height, gal_ubyte* originalData, gal_uint selectionMode);

    // Only available with GALDeviceImp
    virtual void HACK_setPreloadMemory(gal_bool enablePreload);

    // arch::cgoMetaStream* nxtMetaStream();

    MemoryObjectAllocator& allocator() const;
    HAL& driver() const;

    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    GALStoredState* saveAllDeviceState() const;

    void restoreAllDeviceState(const GALStoredState* ssi);

    void copyTexture2RenderTarget(GALTexture2DImp* texture, GALRenderTargetImp* renderTarget);

    virtual gal_uint getCurrentFrame();

    virtual gal_uint getCurrentBatch();

private:

    static gal_uint _packRGBA8888(gal_ubyte red, gal_ubyte green, gal_uint blue, gal_uint alpha);
    
    static gal_ubyte defaultVSh[];
    static gal_ubyte defaultFSh[];

    static gal_uint compressionDifference(gal_uint texel, gal_uint compare, gal_uint rowSize, gal_ubyte* data, GAL_FORMAT originalFormat);
    
    GALShaderProgramImp* _defaultVshProgram;
    GALShaderProgramImp* _defaultFshProgram;

    void _syncRegisters();

    const gal_uint _MAX_STREAMS;

    const gal_uint _MAX_SAMPLES;

    HAL* _driver;
    MemoryObjectAllocator* _moa;

    StateItem<gal_float> zValuePartialClear;
    GALBuffer* vertexBufferPartialClear;

    StateItem<GAL_PRIMITIVE> _primitive;

    GALBlendingStageImp* _blending;
    GALRasterizationStageImp* _rast;
    GALZStencilStageImp* _zStencil;

    std::vector<GALStreamImp*> _stream;
    GALStreamImp* _indexStream;
    StateItem<gal_bool> _indexedMode;

    StateItem<gal_uint> _clearColorBuffer;
    StateItem<gal_float> _zClearValue;
    StateItem<gal_int> _stencilClearValue;

    StateItem<gal_bool> _hzActivated; // HZ enabled (true) or disabled (false)
    gal_bool _hzBufferValid; // Tracks if the current HZ buffer contents are valid or invalid

    StateItem<gal_uint> _streamStart;
    StateItem<gal_uint> _streamCount;
    StateItem<gal_uint> _streamInstances;

    // Enables or disables Hierarchical Z test depending on current GALDevice state
    void _syncHZRegister();

    StateItem<GALRenderTargetImp *> _defaultBackBuffer;
    StateItem<GALRenderTargetImp *> _defaultFrontBuffer;
    StateItem<GALRenderTargetImp *> _defaultZStencilBuffer;
    
    StateItem<GALRenderTargetImp *> _currentRenderTarget[GAL_MAX_RENDER_TARGETS];
    StateItem<GALRenderTargetImp *> _currentZStencilBuffer;
    
    gal_bool _defaultRenderBuffers;
    gal_bool _defaultZStencilBufferInUse;
    gal_uint _mdColorBufferSavedState;
    gal_uint _mdZStencilBufferSavedState;

    StateItem<gal_bool> _colorSRGBWrite;
    
    GALSamplerImp** _sampler;

    // vertex attribute -> stream
    std::vector<StateItem<gal_uint> > _vaMap;
    std::set<gal_uint> _usedStreams;

    std::vector<StateItem<gal_bool> > _vshOutputs;
    std::vector<StateItem<gal_bool> > _fshInputs;

    StateItem<gal_uint> _vshResources;
    StateItem<gal_uint> _fshResources;

    StateItem<GALShaderProgramImp*> _gsh;
    gal_uint _gpuMemGshTrack;
    
    StateItem<GALShaderProgramImp*> _vsh;
    gal_uint _gpuMemVshTrack;

    StateItem<GALShaderProgramImp*> _fsh;
    gal_uint _gpuMemFshTrack;

    // The shader optimizer module
    libGAL_opt::ShaderOptimizer _shOptimizer;

    // Frame and batch tracking.
    gal_uint _startFrame;
    gal_uint _currentFrame;
    gal_uint _currentBatch;
    StateItem<GALFloatVector4> _currentColor;

    StateItem<gal_bool> _alphaTest;
    StateItem<gal_bool> _earlyZ;

    // Deferred dump structures
    struct DumpEventInfo
    {
        std::string fileName;
        gal_enum flags;
        gal_uint frame;
        gal_uint batch;
        gal_bool valid;
        
        DumpEventInfo()
            : fileName(""), flags(0), frame(0), batch(0), valid(false) {;}

        DumpEventInfo(const gal_char* file, gal_enum flags, gal_uint frame, gal_uint batch)
            : fileName(file), flags(flags), frame(frame), batch(batch), valid(true) {;}

        bool operator == (const DumpEventInfo& val) const   {    return ((frame == val.frame) && (batch == val.batch));    }
    };

    std::list<DumpEventInfo> _dumpEventList;
    
    DumpEventInfo _nextDumpEvent;

    const char* primitiveString(GAL_PRIMITIVE primitive);

    // called just before issue a draw command
    void _syncStreamerState();

    gal_uint _requiredSync;

    void _syncVertexShader();
    void _syncFragmentShader();

    void _optimizeShader(GALShaderProgramImp* shProgramImp, GAL_SHADER_TYPE shType);

    void _syncStreamingMode(gal_uint start, gal_uint count, gal_uint instances, gal_uint min, gal_uint max);

    void _partialClear(gal_bool clearColor, gal_bool clearZ, gal_bool clearStencil, 
                       gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha,
                       gal_float zValue, gal_int stencilValue);
    
    // Generic draw command
    void _draw(gal_uint start, gal_uint count, gal_uint min, gal_uint max,
               gal_uint baseVertexIndex = 0,
               gal_uint instances = 1);

    void _addBaseVertexIndex(gal_uint baseVertexIndex, gal_uint start, gal_uint count);

    void _syncSamplerState();

    void _translatePrimitive(GAL_PRIMITIVE primitive, arch::GPURegData* data);

    void _syncRenderBuffers();

    bool _multipleRenderTargetsSet();
    
    // Dump state
    void _dump(const gal_char* file, gal_enum flags);

    void _check_deferred_dump();

    // Printer help classes

    class PrimitivePrint : public PrintFunc<GAL_PRIMITIVE>
    {
    public:

        virtual const char* print(const GAL_PRIMITIVE& var) const;
    }
    primitivePrint;


}; // class GALDeviceImp

} // namespace libGAL


#endif // GAL_DEVICE_IMP
