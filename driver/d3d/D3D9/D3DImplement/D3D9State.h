/**************************************************************************
 *
 */

#ifndef D3D9STATE_H
#define D3D9STATE_H

#include "GALDevice.h"
#include "GALSampler.h"
#include "GALTextureCubeMap.h"
#include "GALZStencilStage.h"
#include "GALBlendingStage.h"
#include <vector>
#include "ShaderTranslator.h"
#include "FFShaderGenerator.h"
#include "D3DTrace.h"

class AIVertexBufferImp9;
class AIIndexBufferImp9;
class AIVertexDeclarationImp9;
class AIVertexShaderImp9;
class AIPixelShaderImp9;
class AISurfaceImp9;
class AIVolumeImp9;
class AITextureImp9;
class AIBaseTextureImp9;
class AICubeTextureImp9;
class AIVolumeTextureImp9;
class AIDeviceImp9;

//
//  Enables a library hack to reduce the resolution of render targets.
//
//#define LOWER_RESOLUTION_HACK

typedef enum {
    D3D9_NONDEFINED,
    D3D9_TEXTURE2D,
    D3D9_CUBETEXTURE,
    D3D9_RENDERTARGET,
    D3D9_VOLUMETEXTURE
} TextureType;

class D3D9State {

public:
    
    static D3D9State &instance();

    void initialize(AIDeviceImp9 *device, UINT width, UINT height);
    void destroy();
    
    void setD3DTrace(D3DTrace *trace);
    bool isPreloadCall();

    //libGAL::GALDevice* getGALDevice();

    AISurfaceImp9 *getDefaultRenderSurface();
    libGAL::GALTexture2D* createTexture2D();
    libGAL::GALTextureCubeMap* createCubeMap();
    libGAL::GALTexture3D* createVolumeTexture();
    libGAL::GALBuffer* createBuffer(UINT Length);
    libGAL::GALShaderProgram* createShaderProgram();
    libGAL::GALRenderTarget* createRenderTarget(libGAL::GALTexture* resource, const libGAL::GAL_RT_DIMENSION rtdimension, D3DCUBEMAP_FACES face, UINT mipmap);

    //void addVertexShader(AIVertexShaderImp9* vs, CONST DWORD* func); // ok
    //void addPixelShader(AIPixelShaderImp9* ps, CONST DWORD* func); // ok

    //void addVertexBuffer(AIVertexBufferImp9* vb, UINT size);
    //void updateVertexBuffer(AIVertexBufferImp9* vb, UINT offset, UINT size, BYTE* data);
    //void addVertexDeclaration(AIVertexDeclarationImp9* vd, CONST D3DVERTEXELEMENT9* elements);

    //void addIndexBuffer(AIIndexBufferImp9* ib, UINT size); // ok
    //void updateIndexBuffer(AIIndexBufferImp9* ib, UINT offset, UINT size, BYTE* data); // ok
    void addTypeTexture(AIBaseTextureImp9* tex, TextureType type);

    //void addTexture2D(AITextureImp9* tex, DWORD usage);
    //void addCubeTexture(AICubeTextureImp9* tex);
    //void addVolumeTexture(AIVolumeTextureImp9* tex);
    //void addMipLevel(AITextureImp9* tex, AISurfaceImp9* mip, UINT mipLevel, UINT Width, UINT Height, D3DFORMAT format);
    //void addCubeTextureMipLevel(AICubeTextureImp9* tex, AISurfaceImp9* mip, D3DCUBEMAP_FACES face, UINT mipLevel, UINT width, D3DFORMAT format);
    //void addVolumeLevel(AIVolumeTextureImp9* tex, AIVolumeImp9* vol, UINT mipLevel, UINT width, UINT height, UINT depth, D3DFORMAT format);
    //void updateSurface(AISurfaceImp9* surf, CONST RECT* rect, D3DFORMAT format, BYTE* data);
    //void updateVolume(AIVolumeImp9* vol, CONST D3DBOX* pBox, D3DFORMAT format, BYTE* data);
    void copySurface(AISurfaceImp9* srcSurf , CONST RECT* srcRect , AISurfaceImp9* destSurf , CONST RECT* destRect , D3DTEXTUREFILTERTYPE Filter);

    void setVertexShader(AIVertexShaderImp9* vs);
    void setPixelShader(AIPixelShaderImp9* ps);

    void setVertexShaderConstant(UINT sr, CONST float* data, UINT vectorCount);
    void setVertexShaderConstant(UINT sr, CONST int* data, UINT vectorCount);
    void setVertexShaderConstantBool(UINT sr, CONST BOOL *data, UINT vectorCount);
    void setPixelShaderConstant(UINT sr, CONST float* data, UINT vectorCount);
    void setPixelShaderConstant(UINT sr, CONST int* data, UINT vectorCount);
    void setPixelShaderConstantBool(UINT sr, CONST BOOL *data, UINT vectorCount);

    void setVertexBuffer(AIVertexBufferImp9* vb, UINT str, UINT offset, UINT stride);
    AIVertexBufferImp9* getVertexBuffer(UINT str, UINT& offset, UINT& stride);
    void setVertexDeclaration(AIVertexDeclarationImp9* vd);
    AIVertexDeclarationImp9* getVertexDeclaration();
    void setFVF(DWORD FVF);

    void setIndexBuffer(AIIndexBufferImp9* ib);
    AIIndexBufferImp9* getIndexBuffer();

    void setTexture(AIBaseTextureImp9* tex, UINT samp);
    void setLOD(AIBaseTextureImp9* tex, DWORD lod);
    void setSamplerState(UINT samp, D3DSAMPLERSTATETYPE type, DWORD value);

    void setFixedFunctionState(D3DRENDERSTATETYPE state, DWORD value);    
    void setBlendingState(D3DRENDERSTATETYPE state , DWORD value);
    void setFogState(D3DRENDERSTATETYPE state , DWORD value);
    void setAlphaTestState(D3DRENDERSTATETYPE state , DWORD value);
    void setZState(D3DRENDERSTATETYPE state , DWORD value);
    void setStencilState(D3DRENDERSTATETYPE state , DWORD value);
    void setColorWriteEnable(DWORD RenderTargetIndex, DWORD value);
    void setColorSRGBWrite(DWORD value);
    void setCullMode(DWORD value);
    void enableScissorTest(DWORD value);
    void scissorRect(UINT x1, UINT y1, UINT x2, UINT y2);

    void setLight(DWORD Index, CONST D3DLIGHT9* pLight);
    void enableLight(DWORD Index, BOOL bEnable);
    void setMaterial(CONST D3DMATERIAL9* pMaterial);
    void setTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX * pMatrix);
    void setTextureStage(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
        
    void setFreqType(UINT stream, UINT type);
    void setFreqValue(UINT stream, UINT value);

    void clear(DWORD count, CONST D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil);

    void setViewport(DWORD x, DWORD y, DWORD width, DWORD height, float minZ, float maxZ);

    void setRenderTarget(DWORD RenderTargetIndex, AISurfaceImp9 *surface);
    void setZStencilBuffer(AISurfaceImp9 *surface);
    AISurfaceImp9 *getRenderTarget(DWORD RenderTargetIndex);
    AISurfaceImp9 *getZStencilBuffer();
    void addRenderTarget(AISurfaceImp9* rt, UINT width , UINT height , D3DFORMAT format);
    
    void draw(D3DPRIMITIVETYPE type, UINT start, UINT count);
    void drawIndexed(D3DPRIMITIVETYPE type, INT baseVertexIndex, UINT minVertexIndex, UINT numVertices, UINT start, UINT count);
int getBatchCounter ();
    void swapBuffers();

    libGAL::GAL_FORMAT getGALFormat(D3DFORMAT Format);
    libGAL::GAL_CUBEMAP_FACE getGALCubeMapFace(D3DCUBEMAP_FACES face);

private:

    static const libGAL::gal_uint MAX_VERTEX_SHADER_CONSTANTS = 256;
    static const libGAL::gal_uint MAX_PIXEL_SHADER_CONSTANTS = 256;
    static const libGAL::gal_uint MAX_RENDER_TARGETS = 4;

    UINT batchCount;
    
    D3D9State();
    
    D3DTrace *d3dTrace;

    libGAL::gal_uint getElementLength(D3DDECLTYPE Type);
    void getTypeAndComponents(D3DDECLTYPE Type, libGAL::gal_uint* components, libGAL::GAL_STREAM_DATA* GalType);
    libGAL::GAL_TEXTURE_FILTER getGALTextureMinFilter(D3DTEXTUREFILTERTYPE Filter, D3DTEXTUREFILTERTYPE FilterMip);
    libGAL::GAL_TEXTURE_FILTER getGALTextureMagFilter(D3DTEXTUREFILTERTYPE Filter);
    libGAL::GAL_BLEND_OPTION getGALBlendOption(D3DBLEND mode);
    libGAL::GAL_BLEND_FUNCTION getGALBlendOperation(D3DBLENDOP operation);
    libGAL::GAL_PRIMITIVE getGALPrimitiveType(D3DPRIMITIVETYPE primitive);
    libGAL::GAL_TEXTURE_ADDR_MODE getGALTextureAddrMode(D3DTEXTUREADDRESS address);
    libGAL::GAL_STENCIL_OP getGALStencilOperation(D3DSTENCILOP operation);
    libGAL::GAL_COMPARE_FUNCTION getGALCompareFunction(D3DCMPFUNC func);
    void createFVFVertexDeclaration();

    void redefineGALBlend();
    void redefineGALStencil();

    bool setGALShaders(NativeShader* &nativeVertexShader);
    void setGALStreams(NativeShader* nativeVertexShader, UINT count, UINT maxVertex);
    void setGALTextures();
    void setGALIndices();

    // GAL main device
    libGAL::GALDevice* GalDev;

    // Vertex shaders created
    //std::map<AIVertexShaderImp9*, DWORD*> vertexShaders;
    // Pixel shaders created
    //std::map<AIPixelShaderImp9*, DWORD*> pixelShaders;

    // Vertex buffers created
    //std::map<AIVertexBufferImp9*, libGAL::GALBuffer*> vertexBuffers;

    // Vertex declarations created
    //std::map<AIVertexDeclarationImp9*, std::vector<D3DVERTEXELEMENT9> > vertexDeclarations;

    // Index buffers created
    //std::map<AIIndexBufferImp9*, libGAL::GALBuffer*> indexBuffers;

    // Textures 2D created
    //std::map<AITextureImp9*, libGAL::GALTexture2D*> textures2D;
    // Textures Cube Map created
    //std::map<AICubeTextureImp9*, libGAL::GALTextureCubeMap*> cubeTextures;
    // Volume Textures created
    //std::map<AIVolumeTextureImp9*, libGAL::GALTexture3D*> volumeTextures;
    // Texture 2D mip levels created
    //std::map<AISurfaceImp9*, AITextureImp9*> mipLevelsTextures2D;
    // Texture Cube Map mip levels created
    //std::map<AISurfaceImp9*, AICubeTextureImp9*> mipLevelsCubeTextures;
    // Mip level number of each mip level
    //std::map<AISurfaceImp9*, UINT> mipLevelsLevels;
    // Face of each mip level
    //std::map<AISurfaceImp9*, libGAL::GAL_CUBEMAP_FACE> mipLevelsFaces;
    
    //std::map<AIVolumeImp9*, AIVolumeTextureImp9*> mipVolumesVolumeTextures;

    //std::map<AIVolumeImp9*, UINT> mipVolumesLevels;

    //std::map<AISurfaceImp9*, libGAL::GALTexture2D*> renderTargetSurface;


    //std::map<AISurfaceImp9*, TextureType> mipLevelsTextureType;
    std::map<AIBaseTextureImp9*, TextureType> textureTextureType;

    AISurfaceImp9 *defaultRenderSurface;
    AISurfaceImp9 *defaultZStencilSurface;
    
    AISurfaceImp9 *currentRenderSurface[MAX_RENDER_TARGETS];
    AISurfaceImp9 *currentZStencilSurface;
    
    //std::map<AISurfaceImp9 *, libGAL::GALRenderTarget *> renderTargets;
    
    AIVertexShaderImp9* settedVertexShader;
    AIPixelShaderImp9* settedPixelShader;
    
    libGAL::gal_float *settedVertexShaderConstants[MAX_VERTEX_SHADER_CONSTANTS];
    libGAL::gal_bool settedVertexShaderConstantsTouched[MAX_VERTEX_SHADER_CONSTANTS];

    libGAL::gal_int *settedVertexShaderConstantsInt[16];
    libGAL::gal_bool settedVertexShaderConstantsIntTouched[16];

    libGAL::gal_bool settedVertexShaderConstantsBool[16];
    libGAL::gal_bool settedVertexShaderConstantsBoolTouched[16];

    libGAL::gal_float *settedPixelShaderConstants[MAX_PIXEL_SHADER_CONSTANTS];
    libGAL::gal_bool settedPixelShaderConstantsTouched[MAX_VERTEX_SHADER_CONSTANTS];

    libGAL::gal_int *settedPixelShaderConstantsInt[16];
    libGAL::gal_bool settedPixelShaderConstantsIntTouched[16];

    libGAL::gal_bool settedPixelShaderConstantsBool[16];
    libGAL::gal_bool settedPixelShaderConstantsBoolTouched[16];

    static const U32 BOOLEAN_CONSTANT_START = 256;
    static const U32 INTEGER_CONSTANT_START = 272;

    typedef struct stDesc{

        AIVertexBufferImp9* streamData;
        UINT offset;
        UINT stride;
        UINT freqType;
        UINT freqValue;

        stDesc():
            streamData(NULL),
            offset(0),
            stride(0),
            freqType(0),
            freqValue(1) {}

    } StreamDescription;

    libGAL::gal_uint maxStreams;

    //std::map<UINT, StreamDescription> settedVertexBuffers;
    std::vector<StreamDescription> settedVertexBuffers;

    // Instancing mode
    bool instancingMode;
    UINT instancesLeft;
    UINT instancesDone;
    UINT instancingStride;
    libGAL::GALBuffer* transfBuffer;

    AIVertexDeclarationImp9* settedVertexDeclaration;

    DWORD settedFVF;
    std::vector<D3DVERTEXELEMENT9> fvfVertexDeclaration;

    AIIndexBufferImp9* settedIndexBuffer;

    typedef struct saDesc{

        AIBaseTextureImp9* samplerData;
        DWORD minFilter;
        DWORD mipFilter;
        DWORD magFilter;
        DWORD maxAniso;
        DWORD maxLevel; //  Base (Min) level in the simulator!!!
        DWORD addressU;
        DWORD addressV;
        DWORD addressW;
        DWORD sRGBTexture;

        saDesc():
            samplerData(NULL),
            minFilter(D3DTEXF_POINT),
            mipFilter(D3DTEXF_NONE),
            magFilter(D3DTEXF_POINT),
            maxAniso(1),
            maxLevel(0),
            addressU(D3DTADDRESS_WRAP),
            addressV(D3DTADDRESS_WRAP),
            addressW(D3DTADDRESS_WRAP),
            sRGBTexture(0) {}

    } SamplerDescription;

    static const libGAL::gal_uint maxSamplers = 16;
    static const libGAL::gal_uint maxVertexSamplers = 4;

    //std::map<UINT, SamplerDescription> settedTextures;
    std::vector<SamplerDescription> settedTextures;
    std::vector<SamplerDescription> settedVertexTextures;

    bool fogEnable;
    DWORD fogColor;
    
    bool alphaTest;
    BYTE alphaRef;
    D3DCMPFUNC alphaFunc;

    bool alphaBlend;
    bool separateAlphaBlend;
    libGAL::GAL_BLEND_OPTION srcBlend;
    libGAL::GAL_BLEND_OPTION srcBlendAlpha;
    libGAL::GAL_BLEND_OPTION dstBlend;
    libGAL::GAL_BLEND_OPTION dstBlendAlpha;
    libGAL::GAL_BLEND_FUNCTION blendOp;
    libGAL::GAL_BLEND_FUNCTION blendOpAlpha;

    libGAL::gal_bool stencilEnabled;
    libGAL::GAL_STENCIL_OP stencilFail;
    libGAL::GAL_STENCIL_OP stencilZFail;
    libGAL::GAL_STENCIL_OP stencilPass;
    libGAL::GAL_COMPARE_FUNCTION stencilFunc;
    libGAL::gal_uint stencilRef;
    libGAL::gal_uint stencilMask;
    libGAL::gal_uint stencilWriteMask;
    
    libGAL::gal_bool twoSidedStencilEnabled;
    libGAL::GAL_STENCIL_OP ccwStencilFail;
    libGAL::GAL_STENCIL_OP ccwStencilZFail;
    libGAL::GAL_STENCIL_OP ccwStencilPass;
    libGAL::GAL_COMPARE_FUNCTION ccwStencilFunc;

    libGAL::gal_bool colorwriteRed[MAX_RENDER_TARGETS];
    libGAL::gal_bool colorwriteGreen[MAX_RENDER_TARGETS];
    libGAL::gal_bool colorwriteBlue[MAX_RENDER_TARGETS];
    libGAL::gal_bool colorwriteAlpha[MAX_RENDER_TARGETS];

    
    libGAL::GALTexture2D *blitTex2D;
    
    //  Stores the fixed function state for the fixed function generator.
    FFState fixedFunctionState;
    libGAL::GALShaderProgram *fixedFunctionVertexShader;
    libGAL::GALShaderProgram *fixedFunctionPixelShader;
    NativeShader *nativeFFVSh;
};

#endif
