/**************************************************************************
 *
 */

#include "Common.h"

#include "GAL.h"
#include "GALRasterizationStage.h"
#include "GALZStencilStage.h"
#include "GALBlendingStage.h"
#include "GALShaderProgram.h"
#include "GALBuffer.h"
#include "GALTexture2D.h"
#include "GALTextureCubeMap.h"
#include "GALSampler.h"
#include "GALRenderTarget.h"

#include "D3D9State.h"

#include "AISurfaceImp_9.h"
#include "AIVolumeImp_9.h"
#include "AIIndexBufferImp_9.h"
#include "AIVertexBufferImp_9.h"
#include "AIVertexDeclarationImp_9.h"
#include "AIVertexShaderImp_9.h"
#include "AIPixelShaderImp_9.h"
#include "AITextureImp_9.h"
#include "AICubeTextureImp_9.h"
#include "AIVolumeTextureImp_9.h"

#include "ShaderTranslator.h"
#include "Utils.h"

#include <cstring>
#include <stdio.h>

D3D9State& D3D9State::instance()
{
	static D3D9State inst;

	return inst;
}

D3D9State::D3D9State()
{
    GalDev = NULL;

    batchCount = 0;

    settedVertexDeclaration = NULL;
    blitTex2D = NULL;

    fogEnable = false;
    fogColor = 0;
    
    alphaTest = false;
    alphaRef = 0;
    alphaFunc = D3DCMP_ALWAYS;

    alphaBlend = false;
    separateAlphaBlend = false;
    srcBlend = libGAL::GAL_BLEND_ONE;
    srcBlendAlpha = libGAL::GAL_BLEND_ONE;
    dstBlend = libGAL::GAL_BLEND_ZERO;
    dstBlendAlpha = libGAL::GAL_BLEND_ZERO;
    blendOp = libGAL::GAL_BLEND_ADD;
    blendOpAlpha = libGAL::GAL_BLEND_ADD;

    stencilEnabled = false;
    stencilFail = libGAL::GAL_STENCIL_OP_KEEP;
    stencilZFail = libGAL::GAL_STENCIL_OP_KEEP;
    stencilPass = libGAL::GAL_STENCIL_OP_KEEP;
    stencilFunc = libGAL::GAL_COMPARE_FUNCTION_ALWAYS;
    stencilRef = 0;
    stencilMask = 0xFFFFFFFF;
    stencilWriteMask = 0xFFFFFFFF;

    twoSidedStencilEnabled = false;
    ccwStencilFail = libGAL::GAL_STENCIL_OP_KEEP;
    ccwStencilZFail = libGAL::GAL_STENCIL_OP_KEEP;
    ccwStencilPass = libGAL::GAL_STENCIL_OP_KEEP;
    ccwStencilFunc = libGAL::GAL_COMPARE_FUNCTION_ALWAYS;

    for(U32 c = 0; c < MAX_VERTEX_SHADER_CONSTANTS; c++)
    {
        settedVertexShaderConstants[c] = new libGAL::gal_float[4];
        settedVertexShaderConstants[c][0] = 0.0f;
        settedVertexShaderConstants[c][1] = 0.0f;
        settedVertexShaderConstants[c][2] = 0.0f;
        settedVertexShaderConstants[c][3] = 0.0f;
        settedVertexShaderConstantsTouched[c] = false;
    }
    
    for(U32 c = 0; c < 16; c++)
    {
        settedVertexShaderConstantsInt[c] = new libGAL::gal_int[4];
        settedVertexShaderConstantsInt[c][0] = 0;
        settedVertexShaderConstantsInt[c][1] = 0;
        settedVertexShaderConstantsInt[c][2] = 0;
        settedVertexShaderConstantsInt[c][3] = 0;
        settedVertexShaderConstantsIntTouched[c] = false;
        
        settedVertexShaderConstantsBool[c] = false;
        settedVertexShaderConstantsBoolTouched[c] = false;
    }

    for(U32 c = 0; c < MAX_PIXEL_SHADER_CONSTANTS; c++)
    {
        settedPixelShaderConstants[c] = new libGAL::gal_float[4];
        settedPixelShaderConstants[c][0] = 0.0f;
        settedPixelShaderConstants[c][1] = 0.0f;
        settedPixelShaderConstants[c][2] = 0.0f;
        settedPixelShaderConstants[c][3] = 0.0f;
        settedPixelShaderConstantsTouched[c] = false;
    }

    for(U32 c = 0; c < 16; c++)
    {
        settedPixelShaderConstantsInt[c] = new libGAL::gal_int[4];
        settedPixelShaderConstantsInt[c][0] = 0;
        settedPixelShaderConstantsInt[c][1] = 0;
        settedPixelShaderConstantsInt[c][2] = 0;
        settedPixelShaderConstantsInt[c][3] = 0;
        settedPixelShaderConstantsIntTouched[c] = false;
        
        settedPixelShaderConstantsBool[c] = false;
        settedPixelShaderConstantsBoolTouched[c] = false;
    }
    
    for (U32 i = 0; i < MAX_RENDER_TARGETS; i++){
        colorwriteRed[i] = true;
        colorwriteGreen[i] = true;
        colorwriteBlue[i] = true;
        colorwriteAlpha[i] = true;
    }

    instancingMode = false;
    instancesLeft = 0;
    instancesDone = 0;
    instancingStride = 1;
    transfBuffer = NULL;

    for (U32 i = 0; i < MAX_RENDER_TARGETS; i++)
        currentRenderSurface[i] = NULL;

    nativeFFVSh = NULL;
}

void D3D9State::initialize(AIDeviceImp9 *device, UINT width, UINT height)
{
    // Create a new device
    if (GalDev == NULL)
        GalDev = libGAL::createDevice(0);

    maxStreams = GalDev->availableStreams();
    settedVertexBuffers.resize(maxStreams);

    //  Allocate the sate for the 16 pixel shader samplers.
    settedTextures.resize(maxSamplers);
    
    //  Allocate the state for the 4 vertex shader samplers.
    settedVertexTextures.resize(maxVertexSamplers);

    // Set viewport resolution
    GalDev->setResolution(width, height);
    GalDev->rast().setViewport(0, 0, width, height);
    GalDev->zStencil().setDepthRange(0.f, 1.f);

    // Set default values
    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_LESS_EQUAL);
    GalDev->rast().setFaceMode(libGAL::GAL_FACE_CW);
    GalDev->rast().setCullMode(libGAL::GAL_CULL_BACK);
    //GalDev->alphaTestEnabled(alphaTest);
    GalDev->alphaTestEnabled(true);
    GalDev->rast().useD3D9RasterizationRules(true);
    GalDev->rast().useD3D9PixelCoordConvention(true);
    GalDev->zStencil().setD3D9DepthRangeMode(true);

    for (U32 i = 0; i < MAX_RENDER_TARGETS; i++)
        GalDev->blending().setColorMask(i, colorwriteRed[i], colorwriteGreen[i], colorwriteBlue[i], colorwriteAlpha[i]);

    for (U32 i = 0; i < MAX_RENDER_TARGETS; i++)
        GalDev->blending().setBlendColor(i, 1.0f, 1.0f, 1.0f, 1.0f);

    redefineGALBlend();
    redefineGALStencil();

    //  Get the current/default render buffers.
    libGAL::GALRenderTarget *currentRT = GalDev->getRenderTarget(0);
    libGAL::GALRenderTarget *currentZStencil = GalDev->getZStencilBuffer();

    //  Create default surfaces for the default render buffers.
    defaultRenderSurface = new AISurfaceImp9(device, width, height, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT);
    defaultZStencilSurface = new AISurfaceImp9(device, width, height, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24S8, D3DPOOL_DEFAULT);

    defaultRenderSurface->setAsRenderTarget(currentRT);
    defaultZStencilSurface->setAsRenderTarget(currentZStencil);
    
    defaultRenderSurface->AddRef();
    defaultZStencilSurface->AddRef();

    currentRenderSurface[0] = defaultRenderSurface;
    currentZStencilSurface = defaultZStencilSurface;

    //  Associate the default render surfaces with the default render targets.
    //renderTargets[defaultRenderSurface] = currentRT;
    //mipLevelsTextureType[defaultRenderSurface] = D3D9_RENDERTARGET;
    //renderTargets[defaultZStencilSurface] = currentZStencil;
    //mipLevelsTextureType[defaultZStencilSurface] = D3D9_RENDERTARGET;

    fixedFunctionVertexShader = GalDev->createShaderProgram();
    fixedFunctionPixelShader = GalDev->createShaderProgram();
}

void D3D9State::destroy() {

    // Destroy the current device
    libGAL::destroyDevice(GalDev);
    GalDev = NULL;

}

void D3D9State::setD3DTrace(D3DTrace *trace)
{
    d3dTrace = trace;
}

bool D3D9State::isPreloadCall()
{
    return d3dTrace->isPreloadCall();
}

/*libGAL::GALDevice* D3D9State::getGALDevice() {

    return GalDev;

}*/

AISurfaceImp9* D3D9State::getDefaultRenderSurface()
{
    return defaultRenderSurface;
}



libGAL::GALTexture2D* D3D9State::createTexture2D() {

    return GalDev->createTexture2D();

}

libGAL::GALTextureCubeMap* D3D9State::createCubeMap() {

    return GalDev->createTextureCubeMap();

}

libGAL::GALTexture3D* D3D9State::createVolumeTexture() {

    return GalDev->createTexture3D();

}

libGAL::GALBuffer* D3D9State::createBuffer(UINT Length) {

    return GalDev->createBuffer(Length);

}

libGAL::GALShaderProgram* D3D9State::createShaderProgram() {

    return GalDev->createShaderProgram();

}

libGAL::GALRenderTarget* D3D9State::createRenderTarget(libGAL::GALTexture* resource, const libGAL::GAL_RT_DIMENSION rtdimension, D3DCUBEMAP_FACES face, UINT mipmap) {

    return GalDev->createRenderTarget(resource, rtdimension, getGALCubeMapFace(face), mipmap);

}

/*void D3D9State::addVertexShader(AIVertexShaderImp9* vs, CONST DWORD* func) {

    // Get shader lenght
    int i = 0;
    while ((func[i] & D3DSI_OPCODE_MASK) != D3DSIO_END)
        i++;

    UINT token_count = i + 1;
    UINT size = sizeof(DWORD) * token_count;

    DWORD* program = new DWORD[token_count];

    // Copy the content of the program
    memcpy(program, func, size);

    // Add the vertex shader to the known vertex shaders
    vertexShaders[vs] = program;

}*/

/*void D3D9State::addPixelShader(AIPixelShaderImp9* ps, CONST DWORD* func) {

    // Get shader lenght
    int i = 0;
    while ((func[i] & D3DSI_OPCODE_MASK) != D3DSIO_END)
        i++;

    UINT token_count = i + 1;
    UINT size = sizeof(DWORD) * token_count;

    DWORD* program = new DWORD[token_count];

    // Copy the content of the program
    memcpy(program, func, size);

    // Add the pixel shader to the known pixel shaders
    pixelShaders[ps] = program;

}*/

/*void D3D9State::addVertexBuffer(AIVertexBufferImp9* vb, UINT size) {

    // Create a new GAL Buffer
    libGAL::GALBuffer* galStreamBuffer = GalDev->createBuffer(size);

    // Add the vertex buffer to the known vertex buffers
    vertexBuffers[vb] = galStreamBuffer;

}*/

/*void D3D9State::updateVertexBuffer(AIVertexBufferImp9* vb, UINT offset, UINT size, BYTE* data) {

    // Update vertex buffer data
    vertexBuffers[vb]->updateData(offset, size, (libGAL::gal_ubyte*)data);

}*/

/*void D3D9State::addVertexDeclaration(AIVertexDeclarationImp9* vd, CONST D3DVERTEXELEMENT9* elements) {

    std::vector< D3DVERTEXELEMENT9 > vElements;
    D3DVERTEXELEMENT9 end = D3DDECL_END();
    UINT numElements = 0;

    // Copy the vertex declaration to a more easy to use structure
    while(elements[numElements].Stream != end.Stream) {

        D3DVERTEXELEMENT9 tmpElem;

        tmpElem.Stream = elements[numElements].Stream;
        tmpElem.Offset = elements[numElements].Offset;
        tmpElem.Type = elements[numElements].Type;
        tmpElem.Method = elements[numElements].Method;
        tmpElem.Usage = elements[numElements].Usage;
        tmpElem.UsageIndex = elements[numElements].UsageIndex;

        vElements.push_back(tmpElem);
        numElements++;

    }

    // Add the vertex declaration to the known vertex declarations
    vertexDeclarations[vd] = vElements;

}*/

/*void D3D9State::addIndexBuffer(AIIndexBufferImp9* ib, UINT size) {

    // Create a new GAL Buffer
    libGAL::GALBuffer* galIndexBuffer = GalDev->createBuffer(size);

    // Add the index buffer to the known index buffers
    indexBuffers[ib] = galIndexBuffer;

}*/

/*void D3D9State::updateIndexBuffer(AIIndexBufferImp9* ib, UINT offset, UINT size, BYTE* data) {

    // Update vertex buffer data
    indexBuffers[ib]->updateData(offset, size, (libGAL::gal_ubyte*)data);

}*/

/*void D3D9State::addTexture2D(AITextureImp9* tex, DWORD usage) {

    // Create a new GAL Texture 2D
    libGAL::GALTexture2D* galTexture = GalDev->createTexture2D();

    // Add the texture 2D to the known textures 2D
    textures2D[tex] = galTexture;

    //  Set layout to render buffer.
    if ((usage & D3DUSAGE_DEPTHSTENCIL) || (usage & D3DUSAGE_RENDERTARGET))
    {
        galTexture->setMemoryLayout(libGAL::GAL_LAYOUT_RENDERBUFFER);
    }

    textureTextureType[(AIBaseTextureImp9*)tex] = D3D9_TEXTURE2D;

}*/

/*void D3D9State::addCubeTexture(AICubeTextureImp9* tex) {

    // Create a new GAL Texture Cube Map
    libGAL::GALTextureCubeMap* galTexture = GalDev->createTextureCubeMap();

    // Add the cube texture to the known cube textures
    cubeTextures[tex] = galTexture;

    textureTextureType[(AIBaseTextureImp9*)tex] = D3D9_CUBETEXTURE;

}*/

/*void D3D9State::addVolumeTexture(AIVolumeTextureImp9* tex) {

    // Create a new GAL Volume Texture
    libGAL::GALTexture3D* galTexture = GalDev->createTexture3D();

    // Add the volume texture to the known cube textures
    volumeTextures[tex] = galTexture;

    textureTextureType[(AIBaseTextureImp9*)tex] = D3D9_VOLUMETEXTURE;

}*/

/*void D3D9State::addMipLevel(AITextureImp9* tex, AISurfaceImp9* mip, UINT mipLevel, UINT width, UINT height, D3DFORMAT format) {

    // Reference to the mip level texture
    mipLevelsTextures2D[mip] = tex;

    // Reference to the mip level number
    mipLevelsLevels[mip] = mipLevel;

    mipLevelsTextureType[mip] = D3D9_TEXTURE2D;

    // Get mip level size
    UINT size = getSurfaceSize(width, height, format);

    //BYTE* data = new BYTE[size];

    // If isn't a compressed texture, the size passed to GAL must be 0
    if (!is_compressed(format))
        size = 0;

    // Set a void buffer to create the new mipmap in GAL
    //textures2D[tex]->setData(mipLevel, width, height, getGALFormat(format), 0, (libGAL::gal_ubyte*)data, size);
    textures2D[tex]->setData(mipLevel, width, height, getGALFormat(format), 0, NULL, size);

    //delete[] data;
}*/

/*void D3D9State::addCubeTextureMipLevel(AICubeTextureImp9* tex, AISurfaceImp9* mip, D3DCUBEMAP_FACES face, UINT mipLevel, UINT width, D3DFORMAT format) {

    // Reference to the mip level texture
    mipLevelsCubeTextures[mip] = tex;

    // Reference to the mip level number
    mipLevelsLevels[mip] = mipLevel;

    // Reference to the face
    mipLevelsFaces[mip] = getGALCubeMapFace(face);

    mipLevelsTextureType[mip] = D3D9_CUBETEXTURE;

    // Get mip level size
    UINT size = getSurfaceSize(width, width, format);

    //BYTE* data = new BYTE[size];

    // If isn't a compressed texture, the size passed to GAL must be 0
    if (!is_compressed(format)) size = 0;

    // Set a void buffer to create the new mipmap in GAL
    //cubeTextures[tex]->setData(getGALCubeMapFace(face), mipLevel, width, width, getGALFormat(format), 0, (libGAL::gal_ubyte*)data, size);
    cubeTextures[tex]->setData(getGALCubeMapFace(face), mipLevel, width, width, getGALFormat(format), 0, NULL, size);

    //delete[] data;
}*/

/*void D3D9State::addVolumeLevel(AIVolumeTextureImp9* tex, AIVolumeImp9* vol, UINT mipLevel, UINT width, UINT height, UINT depth, D3DFORMAT format) {

    mipVolumesVolumeTextures[vol] = tex;

    mipVolumesLevels[vol] = mipLevel;

    UINT size = getVolumeSize(width, height, depth, format);

    BYTE* data = new BYTE[size];

    // If isn't a compressed texture, the size passed to GAL must be 0
    if (!is_compressed(format)) size = 0;

    volumeTextures[tex]->setData(mipLevel, width, height, depth, getGALFormat(format), 0, (libGAL::gal_ubyte*)data, size);

    delete[] data;

}*/

/*void D3D9State::updateSurface(AISurfaceImp9* surf, CONST RECT* rect, D3DFORMAT format, BYTE* data) {

    switch (mipLevelsTextureType[surf])
    {

        case D3D9_TEXTURE2D:
            // Update the mipmap that corresponts to the passed surface with the passed data
            textures2D[mipLevelsTextures2D[surf]]->updateData(mipLevelsLevels[surf], rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top), getGALFormat(format), getSurfacePitchGAL((rect->right - rect->left), format), (libGAL::gal_ubyte*)data);
            break;

        case D3D9_CUBETEXTURE:
            cubeTextures[mipLevelsCubeTextures[surf]]->updateData(mipLevelsFaces[surf], mipLevelsLevels[surf], rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top), getGALFormat(format), getSurfacePitchGAL((rect->right - rect->left), format), (libGAL::gal_ubyte*)data);
            break;

        case D3D9_RENDERTARGET:
            //renderTargetSurface[surf]->updateData(0, rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top), getGALFormat(format), getSurfacePitchGAL((rect->right - rect->left), format), (libGAL::gal_ubyte*)data);
            break;

        default:
            CG_ASSERT("Unsupported texture type");
            break;
    }

}*/

/*void D3D9State::updateVolume(AIVolumeImp9* vol, CONST D3DBOX* pBox, D3DFORMAT format, BYTE* data) {

    volumeTextures[mipVolumesVolumeTextures[vol]]->updateData(mipVolumesLevels[vol], pBox->Left, pBox->Top, pBox->Front, (pBox->Right - pBox->Left), (pBox->Bottom - pBox->Top), (pBox->Back - pBox->Front), getGALFormat(format), getSurfacePitchGAL((pBox->Right - pBox->Left), format), (libGAL::gal_ubyte*)data);

}*/

/*const libGAL::gal_ubyte* D3D9State::getDataSurface(AISurfaceImp9* surf) {

    switch (mipLevelsTextureType[surf])
    {

        case D3D9_TEXTURE2D:
            //return textures2D[mipLevelsTextures2D[surf]]->getData(mipLevelsLevels[surf]);
            break;

        case D3D9_CUBETEXTURE:
            //return cubeTextures[mipLevelsCubeTextures[surf]]->getData(mipLevelsFaces[surf], mipLevelsLevels[surf]);
            break;

        case D3D9_RENDERTARGET:
            //return renderTargetSurface[surf]->getData(0);
            break;

        default:
            CG_ASSERT("Unsupported texture type");
            break;
    }
return NULL;
}*/

/*void D3D9State::copySurface(AISurfaceImp9* srcSurf , CONST RECT* srcRect , AISurfaceImp9* destSurf , CONST POINT* destPoint) {

    libGAL::gal_uint inX;
    libGAL::gal_uint inY;
    libGAL::gal_uint inWidth;
    libGAL::gal_uint inHeight;
    libGAL::gal_uint outX;
    libGAL::gal_uint outY;

    if (mipLevelsTextureType[destSurf] == D3D9_RENDERTARGET) return;

    if (destPoint) {

        outX = destPoint->x;
        outY = destPoint->y;

    }
    else {

        outX = 0;
        outY = 0;

    }

    if (srcRect) {

        inX = srcRect->left;
        inY = srcRect->top;

        inWidth = srcRect->right - srcRect->left;
        inHeight = srcRect->bottom - srcRect->top;

    }
    else {

        inX = 0;
        inY = 0;

        D3DSURFACE_DESC srcDesc;
        srcSurf->GetDesc(&srcDesc);

        inWidth = srcDesc.Width;
        inHeight = srcDesc.Height;

    }

    switch (mipLevelsTextureType[srcSurf])
    {
        case D3D9_TEXTURE2D:
            GalDev->copyMipmap(textures2D[mipLevelsTextures2D[srcSurf]],
                                (libGAL::GAL_CUBEMAP_FACE)0,
                                mipLevelsLevels[srcSurf],
                                inX,
                                inY,
                                inWidth,
                                inHeight,
                                textures2D[mipLevelsTextures2D[destSurf]],
                                (libGAL::GAL_CUBEMAP_FACE)0,
                                mipLevelsLevels[destSurf],
                                outX,
                                outY);

            break;

        case D3D9_CUBETEXTURE:
            GalDev->copyMipmap(cubeTextures[mipLevelsCubeTextures[srcSurf]],
                                mipLevelsFaces[srcSurf],
                                mipLevelsLevels[srcSurf],
                                inX,
                                inY,
                                inWidth,
                                inHeight,
                                cubeTextures[mipLevelsCubeTextures[destSurf]],
                                mipLevelsFaces[srcSurf],
                                mipLevelsLevels[destSurf],
                                outX,
                                outY);
            break;

        default:
            CG_ASSERT("Unsupported texture type");
            break;
    }

}*/

void D3D9State::addTypeTexture(AIBaseTextureImp9* tex, TextureType type) {

    textureTextureType[tex] = type;

}

void D3D9State::copySurface(AISurfaceImp9* srcSurf , CONST RECT* srcRect , AISurfaceImp9* destSurf , CONST RECT* destRect , D3DTEXTUREFILTERTYPE Filter) {

    // Temporal implementation. Ignoring rects an updating full surface...


    AISurfaceImp9* ppRenderTarget;
    AISurfaceImp9* ppZStencilSurface;

    D3DSURFACE_DESC srcDesc;
    D3DSURFACE_DESC destDesc;

    srcSurf->GetDesc(&srcDesc);
    destSurf->GetDesc(&destDesc);

    if ((srcDesc.Format == D3DFMT_NULL) || (destDesc.Format == D3DFMT_NULL))
        return;

    if (((is_compressed(srcDesc.Format)) && (srcSurf->getD3D9Type() == D3D9_TEXTURE2D)) || 
         (is_compressed((destDesc.Format)) && (destSurf->getD3D9Type() == D3D9_TEXTURE2D)))
    {
        //  Get the pointer to the GAL Texture associated with the source surface.
        libGAL::GALTexture2D *destTex2D = destSurf->getGALTexture2D();
        libGAL::GALTexture2D *srcTex2D = srcSurf->getGALTexture2D();

        srcTex2D->copyData(srcSurf->getMipMapLevel(), destSurf->getMipMapLevel(), destTex2D);

        return;
    }
    
    /*if (!((srcRect->top == 0) && (srcRect->bottom == srcDesc.Height) && (srcRect->left == 0) && (srcRect->right == srcDesc.Width) &&
        (destRect->top == 0) && (destRect->bottom == destDesc.Height) && (destRect->left == 0) && (destRect->right == destDesc.Width))) {
            CG_ASSERT("Partial copy not supported.");
    }*/

    //  Check easy case copying from texture surface to render target surface.
    if ((srcSurf->getD3D9Type() == D3D9_TEXTURE2D) && (destSurf->getD3D9Type() == D3D9_RENDERTARGET) &&
        (srcRect->top == 0) && (srcRect->bottom == srcDesc.Height) && (srcRect->left == 0) && (srcRect->right == srcDesc.Width) &&
        (destRect->top == 0) && (destRect->bottom == destDesc.Height) && (destRect->left == 0) && (destRect->right == destDesc.Width))
    {
        //  Get the pointer to the GAL Texture associated with the source surface.
        libGAL::GALTexture2D *sourceTex2D = srcSurf->getGALTexture2D();

        //  Get the mip level associated with the source surface.
        U32 sourceMipLevel = srcSurf->getMipMapLevel();

        //  Get the pointer to the GAL Render Target associated with the destination surface.
        libGAL::GALRenderTarget *destinationRT = destSurf->getGALRenderTarget();


        //  Copy the surface data to the render buffer (same dimensions and format).
        GalDev->copySurfaceDataToRenderBuffer(sourceTex2D, sourceMipLevel, destinationRT, isPreloadCall());

        return;
    }
    else if ((srcSurf->getD3D9Type() == D3D9_TEXTURE2D) && (destSurf->getD3D9Type() == D3D9_TEXTURE2D) &&
        (srcRect->top == 0) && (srcRect->bottom == srcDesc.Height) && (srcRect->left == 0) && (srcRect->right == srcDesc.Width) &&
        (destRect->top == 0) && (destRect->bottom == destDesc.Height) && (destRect->left == 0) && (destRect->right == destDesc.Width))
    {
        //  Get the pointer to the GAL Texture associated with the destination surface.
        libGAL::GALTexture2D *destTex2D = destSurf->getGALTexture2D();

        //  Check if the destination surface uses the render buffer layout.
        if (destTex2D->getMemoryLayout() == libGAL::GAL_LAYOUT_RENDERBUFFER)
        {
            //  Get the mip level associated with the destination surface.
            //U32 destMipLevel = mipLevelsLevels[destSurf];

            //  Get the pointer to the GAL Texture associated with the source surface.
            libGAL::GALTexture2D *sourceTex2D = srcSurf->getGALTexture2D();

            //  Get the mip level associated with the source surface.
            U32 sourceMipLevel = srcSurf->getMipMapLevel();

            /*//  Create a render target for the destination surface.
            libGAL::GAL_RT_DESC rtDesc;
            rtDesc.format = destTex2D->getFormat(destMipLevel);
            rtDesc.dimension = libGAL::GAL_RT_DIMENSION_TEXTURE2D;
            rtDesc.texture2D.mipmap = destMipLevel;
            libGAL::GALRenderTarget *destinationRT = GalDev->createRenderTarget(destTex2D, libGAL::GAL_RT_DIMENSION_TEXTURE2D,
                                                                                 static_cast<libGAL::GAL_CUBEMAP_FACE>(0), destMipLevel);

            //  Add the new render target.
            renderTargets[destSurf] = destinationRT;*/

            libGAL::GALRenderTarget *destinationRT = destSurf->getGALRenderTarget();
			
            //  Copy the surface data to the render buffer (same dimensions and format).
            GalDev->copySurfaceDataToRenderBuffer(sourceTex2D, sourceMipLevel, destinationRT, isPreloadCall());

            //  Copy the surface data to the render buffer (same dimensions and format).
            //GalDev->copySurfaceDataToRenderSurface(sourceTex2D, sourceMipLevel, destTex2D, destMipLevel);

            return;
        }

    }

    //  Evaluate if the source texture is a render target.
    bool sourceIsRenderTarget = (srcSurf->getD3D9Type() == D3D9_RENDERTARGET);
    
    //  Evaluate if the source texture is a compressed render target.
    bool sourceIsCompressedRenderTarget = false;
    if (sourceIsRenderTarget)
        sourceIsCompressedRenderTarget = (srcSurf->getGALRenderTarget()->allowsCompression());
    
    //  Check for compressed render targets.  They need to be blitted and uncompressed to a normal texture.
    if (sourceIsCompressedRenderTarget)
    {    
        //cout << " -- render target." << endl;
        //cout << "      -- render target address: " << hex << renderTargets[srcSurf] << endl;
        //cout << "      -- render target texture address: " << hex << renderTargets[srcSurf]->getTexture() << dec << endl;

        //GalDev->sampler(0).setTexture(renderTargetSurface[srcSurf]);
        //GalDev->sampler(0).setTexture(renderTargets[srcSurf]->getTexture());

        /***************************
        Blit
        ***************************/
        
        // Set source surface as a render target
        // Get the current render target
        ppRenderTarget = getRenderTarget(0);
        // Get the current depth stencil
        ppZStencilSurface = getZStencilBuffer();

        setRenderTarget(0, srcSurf);
        setZStencilBuffer(NULL);

        /*if (blitTex2D != NULL)
            GalDev->destroy(blitTex2D);*/

        blitTex2D = GalDev->createTexture2D();
        //blitTex2D->setData(0,destDesc.Width, destDesc.Height, getGALFormat(destDesc.Format), 0, NULL, 0);
    
        libGAL::GAL_FORMAT destFormat = getGALFormat(destDesc.Format);

        //  Conversion needed because for render buffers the inverted formats are not
        //  implemented and are just aliased with the non inverted formats.
        switch(destFormat)
        {
            case libGAL::GAL_FORMAT_XRGB_8888:
            case libGAL::GAL_FORMAT_ARGB_8888:
                destFormat = libGAL::GAL_FORMAT_RGBA_8888;
                break;

            default:
                //  Keep the format.
                break;
        }
        
        blitTex2D->setData(0,srcDesc.Width, srcDesc.Height, destFormat, 0, NULL, 0);

        //GalDev->sampler(0).setTexture(textures2D[mipLevelsTextures2D[destSurf]]);
        GalDev->sampler(0).setTexture(/*destSurf->getGALTexture2D()*/ blitTex2D);
        //GalDev->sampler(0).performBlitOperation2D(0, 0, 0, 0, srcDesc.Width, srcDesc.Height, destDesc.Width, getGALFormat(srcDesc.Format), /*textures2D[mipLevelsTextures2D[destSurf]]*//*destSurf->getGALTexture2D()*/blitTex2D, 0);
        GalDev->performBlitOperation2D(0, 0, 0, 0, 0, srcDesc.Width, srcDesc.Height, srcDesc.Width, getGALFormat(srcDesc.Format), /*textures2D[mipLevelsTextures2D[destSurf]]*//*destSurf->getGALTexture2D()*/blitTex2D, 0);
        
        setRenderTarget(0, ppRenderTarget);
        setZStencilBuffer(ppZStencilSurface);

    }
    //else if (//mipLevelsTextureType[srcSurf]srcSurf->getD3D9Type() == D3D9_TEXTURE2D) {

    libGAL::gal_float xoffset = (1.f / (libGAL::gal_float)srcDesc.Width) / 4.f;
    libGAL::gal_float yoffset = (1.f / (libGAL::gal_float)srcDesc.Height) / 4.f;

    libGAL::gal_float srcX1 = (libGAL::gal_float)srcRect->left / (libGAL::gal_float)srcDesc.Width;
    libGAL::gal_float srcY1 = (libGAL::gal_float)srcRect->top / (libGAL::gal_float)srcDesc.Height;
    libGAL::gal_float srcX2 = (libGAL::gal_float)srcRect->right / (libGAL::gal_float)srcDesc.Width;
    libGAL::gal_float srcY2 = (libGAL::gal_float)srcRect->bottom / (libGAL::gal_float)srcDesc.Height;

    libGAL::gal_float dstX1 = ((libGAL::gal_float)destRect->left / (libGAL::gal_float)destDesc.Width) * 2.f - 1.f;
    libGAL::gal_float dstY1 = -((libGAL::gal_float)destRect->top / (libGAL::gal_float)destDesc.Height) * 2.f + 1.f;
    libGAL::gal_float dstX2 = ((libGAL::gal_float)destRect->right / (libGAL::gal_float)destDesc.Width) * 2.f - 1.f;
    libGAL::gal_float dstY2 = -((libGAL::gal_float)destRect->bottom / (libGAL::gal_float)destDesc.Height) * 2.f + 1.f;

    /*libGAL::gal_float dataBuffer[] = {-1.f, -1.f, 0.f, 0.f + xoffset, 1.f + yoffset,
                                    1.f, -1.f, 0.f, 1.f + xoffset, 1.f + yoffset,
                                    1.f, 1.f, 0.f, 1.f + xoffset, 0.f + yoffset,
                                    -1.f, 1.f, 0.f, 0.f + xoffset, 0.f + yoffset};*/
    
    libGAL::gal_float dataBuffer[] = {dstX1, dstY2, 0.f, srcX1 + xoffset, srcY2 + yoffset,
                                    dstX2, dstY2, 0.f, srcX2 + xoffset, srcY2 + yoffset,
                                    dstX2, dstY1, 0.f, srcX2 + xoffset, srcY1 + yoffset,
                                    dstX1, dstY1, 0.f, srcX1 + xoffset, srcY1 + yoffset};

    libGAL::gal_uint indexBuffer[] = {0, 1, 2, 2, 3, 0};
    //libGAL::gal_uint indexBuffer[] = {0, 2, 1, 2, 0, 3};

    libGAL::gal_ubyte vertexShader[] = "mov o0, i0\n"
                                       "mov o5, i1\n";
                                       
    libGAL::gal_ubyte fragmentShader[] = "tex r0, i5, t0\n"
                                         "mov o1, r0\n";
                                         
    libGAL::GALBuffer* dataGALBuffer = GalDev->createBuffer((sizeof (libGAL::gal_float)) * 5 * 4, (libGAL::gal_ubyte *)&dataBuffer);

    //cout << " -- data buffer created: " << hex << dataGALBuffer << endl;

    libGAL::GAL_STREAM_DESC streamDescription;
    streamDescription.offset = 0;
    streamDescription.stride = (sizeof (libGAL::gal_float)) * 5;
    streamDescription.components = 3;
    streamDescription.frequency = 0;
    streamDescription.type = libGAL::GAL_SD_FLOAT32;

    //cout << " -- streamDescription 0 setted: " << endl;
    //cout << "     -- offset: " << dec << streamDescription.offset << endl;
    //cout << "     -- stride: " << dec << streamDescription.stride << endl;
    //cout << "     -- components: " << dec << streamDescription.components << endl;
    //cout << "     -- frequency: " << dec << streamDescription.frequency << endl;
    //cout << "     -- type: " << dec << streamDescription.type << endl;

    // Set the vertex buffer to GAL
    GalDev->stream(0).set(dataGALBuffer, streamDescription);
    GalDev->enableVertexAttribute(0, 0);

    //cout << " -- data buffer 0 setted and enabled." << endl;

    streamDescription.offset = (sizeof (libGAL::gal_float)) * 3;
    streamDescription.stride = (sizeof (libGAL::gal_float)) * 5;
    streamDescription.components = 2;
    streamDescription.frequency = 0;
    streamDescription.type = libGAL::GAL_SD_FLOAT32;

    //cout << " -- streamDescription 1 setted: " << endl;
    //cout << "     -- offset: " << dec << streamDescription.offset << endl;
    //cout << "     -- stride: " << dec << streamDescription.stride << endl;
    //cout << "     -- components: " << dec << streamDescription.components << endl;
    //cout << "     -- frequency: " << dec << streamDescription.frequency << endl;
    //cout << "     -- type: " << dec << streamDescription.type << endl;

    // Set the vertex buffer to GAL
    GalDev->stream(1).set(dataGALBuffer, streamDescription);
    GalDev->enableVertexAttribute(1, 1);

    //cout << " -- data buffer 1 setted and enabled." << endl;

    libGAL::GALBuffer* indexGALBuffer = GalDev->createBuffer((sizeof (libGAL::gal_uint)) * 6, (libGAL::gal_ubyte *)&indexBuffer);

    //cout << " -- index buffer created: " << hex << indexGALBuffer << endl;

    GalDev->setIndexBuffer(indexGALBuffer, 0, libGAL::GAL_SD_UINT32);

    //cout << " -- index buffer setted." << endl;

    // Set source surface as a Texture
    GalDev->sampler(0).setEnabled(true);

    //cout << " -- sampler 0 enabled." << endl;
        //cout << " -- texture 2d." << dec << endl;

    if (sourceIsCompressedRenderTarget)
        GalDev->sampler(0).setTexture(blitTex2D);
    else if (sourceIsRenderTarget)
        GalDev->sampler(0).setTexture(srcSurf->getGALRenderTarget()->getTexture());
    else
        GalDev->sampler(0).setTexture(/*textures2D[mipLevelsTextures2D[srcSurf]]*/ srcSurf->getGALTexture2D());

                        //cout << " - Format: " << textures2D[mipLevelsTextures2D[srcSurf]]->getFormat(0) << endl;
                        //cout << " - Width: " << textures2D[mipLevelsTextures2D[srcSurf]]->getWidth(0) << endl;
                        //cout << " - Height: " << textures2D[mipLevelsTextures2D[srcSurf]]->getHeight(0) << endl;
                        //cout << " - MemoryLayout: " << textures2D[mipLevelsTextures2D[srcSurf]]->getMemoryLayout() << endl;
                        //cout << " - SettedMipmaps: " << textures2D[mipLevelsTextures2D[srcSurf]]->getSettedMipmaps() << endl;



    GalDev->sampler(0).setMagFilter(getGALTextureMagFilter(Filter));
    GalDev->sampler(0).setMinFilter(libGAL::GAL_TEXTURE_FILTER_NEAREST);
    GalDev->sampler(0).setMaxAnisotropy(1);

    GalDev->sampler(0).setTextureAddressMode(libGAL::GAL_TEXTURE_S_COORD, libGAL::GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    GalDev->sampler(0).setTextureAddressMode(libGAL::GAL_TEXTURE_T_COORD, libGAL::GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);
    GalDev->sampler(0).setTextureAddressMode(libGAL::GAL_TEXTURE_R_COORD, libGAL::GAL_TEXTURE_ADDR_CLAMP_TO_EDGE);

    libGAL::GALShaderProgram* GALVertexShader = GalDev->createShaderProgram();
    libGAL::GALShaderProgram* GALFragmentShader = GalDev->createShaderProgram();

    GALVertexShader->setProgram(vertexShader);
    GALFragmentShader->setProgram(fragmentShader);

    GalDev->setVertexShader(GALVertexShader);
    GalDev->setFragmentShader(GALFragmentShader);

    // Set dest surface as a render target
    // Get the current render target
    ppRenderTarget = getRenderTarget(0);
    // Get the current depth stencil
    ppZStencilSurface = getZStencilBuffer();

    setRenderTarget(0, destSurf);
    setZStencilBuffer(NULL);

    //GalDev->clearColorBuffer(0, 0, 0, 0);
    GalDev->rast().setCullMode(libGAL::GAL_CULL_NONE);
    GalDev->zStencil().setStencilEnabled(false);
    GalDev->blending().setColorMask(0, true, true, true, true);
    GalDev->blending().setEnable(0, false);


    // Draw
    GalDev->setPrimitive(libGAL::GAL_TRIANGLES);

    GalDev->drawIndexed(0, 6, 0, 0, 0);

    GalDev->disableVertexAttributes();

    // Swap buffers
    //GalDev->swapBuffers();

    //GalDev->destroy(dataGALBuffer);
    //GalDev->destroy(indexGALBuffer);

    //GalDev->destroy(GALVertexShader);
    //GalDev->destroy(GALFragmentShader);

    setRenderTarget(0, ppRenderTarget);
    setZStencilBuffer(ppZStencilSurface);

    // TODO: restore cull mode...
    redefineGALStencil();    
    GalDev->blending().setColorMask(0, colorwriteRed[0], colorwriteGreen[0], colorwriteBlue[0], colorwriteAlpha[0]);
    GalDev->blending().setEnable(0, alphaBlend);

    batchCount++;

    /*}
    else {
        CG_ASSERT("Not a texture or a render target.");
    }*/

}

/*void D3D9State::copySurface(AISurfaceImp9* srcSurf , CONST RECT* srcRect , AISurfaceImp9* destSurf , CONST RECT* destRect , D3DTEXTUREFILTERTYPE Filter) {

    if (mipLevelsTextureType[srcSurf] == D3D9_RENDERTARGET) {

        if (mipLevelsTextureType[destSurf] == D3D9_RENDERTARGET) {

            GalDev->copyMipmap(renderTargets[srcSurf]->getTexture(), (libGAL::GAL_CUBEMAP_FACE)0, 0,
                srcRect->left, srcRect->top, srcRect->right - srcRect->left, srcRect->bottom - srcRect->top,
                renderTargets[destSurf]->getTexture(), (libGAL::GAL_CUBEMAP_FACE)0, 0,
                destRect->left, destRect->top, destRect->right - destRect->left, destRect->bottom - destRect->top,
                getGALTextureMagFilter(Filter));

        }
        else if (mipLevelsTextureType[destSurf] == D3D9_TEXTURE2D) {

            GalDev->copyMipmap(renderTargets[srcSurf]->getTexture(), (libGAL::GAL_CUBEMAP_FACE)0, 0,
                srcRect->left, srcRect->top, srcRect->right - srcRect->left, srcRect->bottom - srcRect->top,
                textures2D[mipLevelsTextures2D[destSurf]], (libGAL::GAL_CUBEMAP_FACE)0, mipLevelsLevels[destSurf],
                destRect->left, destRect->top, destRect->right - destRect->left, destRect->bottom - destRect->top,
                getGALTextureMagFilter(Filter));

        }

    }
    else if (mipLevelsTextureType[srcSurf] == D3D9_TEXTURE2D) {

        if (mipLevelsTextureType[destSurf] == D3D9_RENDERTARGET) {

            GalDev->copyMipmap(textures2D[mipLevelsTextures2D[srcSurf]], (libGAL::GAL_CUBEMAP_FACE)0, mipLevelsLevels[srcSurf],
                srcRect->left, srcRect->top, srcRect->right - srcRect->left, srcRect->bottom - srcRect->top,
                renderTargets[destSurf]->getTexture(), (libGAL::GAL_CUBEMAP_FACE)0, 0,
                destRect->left, destRect->top, destRect->right - destRect->left, destRect->bottom - destRect->top,
                getGALTextureMagFilter(Filter));

        }
        else if (mipLevelsTextureType[destSurf] == D3D9_TEXTURE2D) {

            GalDev->copyMipmap(textures2D[mipLevelsTextures2D[srcSurf]], (libGAL::GAL_CUBEMAP_FACE)0, mipLevelsLevels[srcSurf],
                srcRect->left, srcRect->top, srcRect->right - srcRect->left, srcRect->bottom - srcRect->top,
                textures2D[mipLevelsTextures2D[destSurf]], (libGAL::GAL_CUBEMAP_FACE)0, mipLevelsLevels[destSurf],
                destRect->left, destRect->top, destRect->right - destRect->left, destRect->bottom - destRect->top,
                getGALTextureMagFilter(Filter));

        }

    }

}*/

void D3D9State::setVertexShader(AIVertexShaderImp9* vs) {

    // Set the vertex shader to use
    settedVertexShader = vs;

}

void D3D9State::setPixelShader(AIPixelShaderImp9* ps) {

    // Set the pixel shader to use
    settedPixelShader = ps;

}

void D3D9State::setVertexShaderConstant(UINT first, CONST float* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        if ((first + c) >= MAX_VERTEX_SHADER_CONSTANTS)
        {
            printf("WARNING!!! D3D9State::setVertexShaderConstants => Vertex shader constant out of range.  First = %d Count = %d\n",
                first, vectorCount);
            break;
            //CG_ASSERT("Vertex shader constant index out of range.");
        }

        // Add the new constant setted to the setted constants map
        settedVertexShaderConstants[first + c][0] = data[c * 4 + 0];
        settedVertexShaderConstants[first + c][1] = data[c * 4 + 1];
        settedVertexShaderConstants[first + c][2] = data[c * 4 + 2];
        settedVertexShaderConstants[first + c][3] = data[c * 4 + 3];

        settedVertexShaderConstantsTouched[first + c] = true;
    }
}

void D3D9State::setVertexShaderConstant(UINT first, CONST int* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        // Add the new constant setted to the setted constants map
        settedVertexShaderConstantsInt[first + c][0] = data[c * 4 + 0];
        settedVertexShaderConstantsInt[first + c][1] = data[c * 4 + 1];
        settedVertexShaderConstantsInt[first + c][2] = data[c * 4 + 2];
        settedVertexShaderConstantsInt[first + c][3] = data[c * 4 + 3];

        settedVertexShaderConstantsIntTouched[first + c] = true;
    }
}

void D3D9State::setVertexShaderConstantBool(UINT first, CONST BOOL* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        // Add the new constant setted to the setted constants map
        settedVertexShaderConstantsBool[first + c] = data[c];
        settedVertexShaderConstantsBoolTouched[first + c] = true;
    }
}


void D3D9State::setPixelShaderConstant(UINT first, CONST float* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        if ((first + c) >= MAX_PIXEL_SHADER_CONSTANTS)
        {
            printf("WARNING!!! D3D9State::setPixelShaderConstants => Pixel shader constant out of range.  First = %d Count = %d\n",
                first, vectorCount);
            break;
            //CG_ASSERT("Pixel shader constant index out of range.");
        }

        // Add the new constant setted to the setted constants map
        settedPixelShaderConstants[first + c][0] = data[c * 4 + 0];
        settedPixelShaderConstants[first + c][1] = data[c * 4 + 1];
        settedPixelShaderConstants[first + c][2] = data[c * 4 + 2];
        settedPixelShaderConstants[first + c][3] = data[c * 4 + 3];

        settedPixelShaderConstantsTouched[first + c] = true;
    }
}

void D3D9State::setPixelShaderConstant(UINT first, CONST int* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        // Add the new constant setted to the setted constants map
        settedPixelShaderConstantsInt[first + c][0] = data[c * 4 + 0];
        settedPixelShaderConstantsInt[first + c][1] = data[c * 4 + 1];
        settedPixelShaderConstantsInt[first + c][2] = data[c * 4 + 2];
        settedPixelShaderConstantsInt[first + c][3] = data[c * 4 + 3];

        settedPixelShaderConstantsIntTouched[first + c] = true;
    }
}

void D3D9State::setPixelShaderConstantBool(UINT first, CONST BOOL* data, UINT vectorCount)
{
    // For each constant
    for(UINT c = 0; c < vectorCount; c++)
    {
        // Add the new constant setted to the setted constants map
        settedPixelShaderConstantsBool[first + c] = data[c];
        settedPixelShaderConstantsBoolTouched[first + c] = true;
    }
}


void D3D9State::setVertexBuffer(AIVertexBufferImp9* vb, UINT str, UINT offset, UINT stride) 
{

    // Set the vertex buffer to the indicated stream
    if (str < maxStreams) 
    {
        settedVertexBuffers[str].streamData = vb;
        settedVertexBuffers[str].offset = offset;
        settedVertexBuffers[str].stride = stride;
    }
    else
        CG_ASSERT("Selected stream is bigger than the maximum number of streams");

}

AIVertexBufferImp9* D3D9State::getVertexBuffer(UINT str, UINT& offset, UINT& stride) 
{

    // Set the vertex buffer to the indicated stream
    if (str < maxStreams) 
    {
        return settedVertexBuffers[str].streamData;
    }
    else
        CG_ASSERT("Selected stream is bigger than the maximum number of streams");

    return 0;

}

void D3D9State::setVertexDeclaration(AIVertexDeclarationImp9* vd) {

    // Set the vertex declaration to use
    settedVertexDeclaration = vd;
        
    //  Set the vertex declaration for the fixed function generator.    
    fixedFunctionState.vertexDeclaration.clear();
    for(U32 d = 0; d < vd->getVertexElements().size(); d++)
        fixedFunctionState.vertexDeclaration.push_back(vd->getVertexElements()[d]);
}

AIVertexDeclarationImp9* D3D9State::getVertexDeclaration()
{
    return settedVertexDeclaration;
}

void D3D9State::setFVF(DWORD FVF)
{
    // Set the fixed function vertex declaration
    settedFVF = FVF;

    //  Set the fixed function vertex declaration for the fixed function generator.
    fixedFunctionState.fvf = FVF;
}

void D3D9State::setIndexBuffer(AIIndexBufferImp9* ib) {

    // Set the index buffer to use
    settedIndexBuffer = ib;

}

AIIndexBufferImp9* D3D9State::getIndexBuffer ()
{
    return settedIndexBuffer;
}

void D3D9State::setTexture(AIBaseTextureImp9* tex, UINT samp)
{
    // Set the texture to the indicated sampler
    if (samp < maxSamplers)
    {
        settedTextures[samp].samplerData = tex;

        //  Update fixed function state.  Only the first 8 samplers affect the fixed function state.
        if (samp < 8)
        {
            //  Set if a texture is defined for the sampler.
            fixedFunctionState.settedTexture[samp] = (tex != NULL);
            
            if (tex != NULL)
            {
                //  Set the texture type.
                switch(textureTextureType[settedTextures[samp].samplerData])
                {
                    case D3D9_TEXTURE2D:
                    case D3D9_RENDERTARGET:
                        fixedFunctionState.textureType[samp] = D3DSTT_2D;
                        break;
                    case D3D9_CUBETEXTURE:
                        fixedFunctionState.textureType[samp] = D3DSTT_CUBE;
                        break;
                    case D3D9_VOLUMETEXTURE:
                        fixedFunctionState.textureType[samp] = D3DSTT_VOLUME;
                        break;
                    default:
                        CG_ASSERT("Undefined texture type.");
                        break;                    
                }
            }
        }
        
                /*if (batchCount == 619) {
                    if (textureTextureType[tex] == D3D9_TEXTURE2D) {
                        cout << "---- setTexture ----" << endl;
                        cout << "Sampler: " << samp << endl;
                        cout << "Format: " << textures2D[(AITextureImp9*)tex]->getFormat(0) << endl;
                        cout << "Width: " << textures2D[(AITextureImp9*)tex]->getWidth(0) << endl;
                        cout << "Height: " << textures2D[(AITextureImp9*)tex]->getHeight(0) << endl;
                        cout << "MemoryLayout: " << textures2D[(AITextureImp9*)tex]->getMemoryLayout() << endl;
                        cout << "SettedMipmaps: " << textures2D[(AITextureImp9*)tex]->getSettedMipmaps() << endl;
                    }
                }*/
    }
    else if (samp == D3DDMAPSAMPLER)
    {
        printf("WARNING!!! D3D9State::setTexture => Map sampler not implemented (tesselator).\n");
    }
    else if ((samp >= D3DVERTEXTEXTURESAMPLER0) && (samp <= D3DVERTEXTEXTURESAMPLER3))
    {
        settedVertexTextures[samp - D3DVERTEXTEXTURESAMPLER0].samplerData = tex;
    }
    else
    {
        CG_ASSERT("Sampler identifier out of range.");   
    }

}

void D3D9State::setLOD(AIBaseTextureImp9* tex, DWORD lod)
{
    //  NOTE:  SetLOD in Direct3D9 just affects how mipmaps are loaded in video memory
    //  not the lod computation or mipmap selection.
   
    //switch (textureTextureType[tex])
    //{
    //    case D3D9_TEXTURE2D:
    //        //textures2D[(AITextureImp9*)tex]((AITextureImp9*)tex)->getGALTexture2D()->setBaseLevel(lod);
    //        break;
    //
    //    case D3D9_CUBETEXTURE:
    //        //cubeTextures[(AICubeTextureImp9*)tex]((AICubeTextureImp9*)tex)->getGALCubeTexture()->setBaseLevel(lod);
    //        break;
    //
    //    default:
    //        CG_ASSERT("Unsupported texture type");
    //        break;
    //}
}

void D3D9State::setSamplerState(UINT samp, D3DSAMPLERSTATETYPE type, DWORD value) {

    if (samp < maxSamplers)
    {
        switch(type)
        {
            case D3DSAMP_MINFILTER:
                settedTextures[samp].minFilter = value;
                break;

            case D3DSAMP_MIPFILTER:
                settedTextures[samp].mipFilter = value;
                break;

            case D3DSAMP_MAGFILTER:
                settedTextures[samp].magFilter = value;
                break;

            case D3DSAMP_MAXANISOTROPY:
                settedTextures[samp].maxAniso = value;
                break;

            case D3DSAMP_ADDRESSU:
                settedTextures[samp].addressU = value;
                break;

            case D3DSAMP_ADDRESSV:
                settedTextures[samp].addressV = value;
                break;

            case D3DSAMP_ADDRESSW:
                settedTextures[samp].addressW = value;
                break;

            case D3DSAMP_MAXMIPLEVEL:
                settedTextures[samp].maxLevel = value;
                break;

            case D3DSAMP_BORDERCOLOR:
            case D3DSAMP_MIPMAPLODBIAS:

                // NOT IMPLEMENTED!!!!
                break;

            case D3DSAMP_SRGBTEXTURE:
                settedTextures[samp].sRGBTexture = value;
                break;
                
            case D3DSAMP_ELEMENTINDEX:
            case D3DSAMP_DMAPOFFSET:
                // NOT SUPPORTED BY THE SIMULATOR!!!

                break;

            default:
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State: WARNING: Sampler state " << samplerstate2string(type) << ", value " << (int)value << " not supported." << endl; ) )
                {
                    char message[50];
                    sprintf(message, "Unsupported sampler state identifier with value = %d", type);
                    CG_ASSERT(message);
                }
                break;
        }
    }
    else if (samp == D3DDMAPSAMPLER)
    {
        printf("WARNING!!! D3D9State::setTexture => Map sampler not implemented (tesselator).\n");
    }
    else if ((samp >= D3DVERTEXTEXTURESAMPLER0) && (samp <= D3DVERTEXTEXTURESAMPLER3))
    {
        samp = samp - D3DVERTEXTEXTURESAMPLER0;
        
        switch(type)
        {
            case D3DSAMP_MINFILTER:
                settedVertexTextures[samp].minFilter = value;
                break;

            case D3DSAMP_MIPFILTER:
                settedVertexTextures[samp].mipFilter = value;
                break;

            case D3DSAMP_MAGFILTER:
                settedVertexTextures[samp].magFilter = value;
                break;

            case D3DSAMP_MAXANISOTROPY:
                settedVertexTextures[samp].maxAniso = value;
                break;

            case D3DSAMP_ADDRESSU:
                settedVertexTextures[samp].addressU = value;
                break;

            case D3DSAMP_ADDRESSV:
                settedVertexTextures[samp].addressV = value;
                break;

            case D3DSAMP_ADDRESSW:
                settedVertexTextures[samp].addressW = value;
                break;

            case D3DSAMP_MAXMIPLEVEL:
                settedVertexTextures[samp].maxLevel = value;
                break;

            case D3DSAMP_BORDERCOLOR:
            case D3DSAMP_MIPMAPLODBIAS:

                // NOT IMPLEMENTED!!!!
                break;

            case D3DSAMP_SRGBTEXTURE:
                settedVertexTextures[samp].sRGBTexture = value;
                break;
                
            case D3DSAMP_ELEMENTINDEX:
            case D3DSAMP_DMAPOFFSET:
                // NOT SUPPORTED BY THE SIMULATOR!!!

                break;

            default:
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State: WARNING: Sampler state " << samplerstate2string(type) << ", value " << (int)value << " not supported." << endl; ) )
                {
                    char message[50];
                    sprintf(message, "Unsupported sampler state identifier with value = %d", type);
                    CG_ASSERT(message);
                }
                break;
        }
    }
    else
    {
        CG_ASSERT("Sampler identifier out of range.");   
    }
}

void D3D9State::setFixedFunctionState(D3DRENDERSTATETYPE state , DWORD value)
{
    switch(state)
    {
        case D3DRS_FOGENABLE:         
        
            fixedFunctionState.fogEnable = true;
            
            setFogState(state, value);
            break;
        
        case D3DRS_FOGCOLOR:
        
            fixedFunctionState.fogColor.r = F32((value >> 16) && 0xff) / 255.0f;
            fixedFunctionState.fogColor.g = F32((value >> 8) && 0xff) / 255.0f;
            fixedFunctionState.fogColor.b = F32(value & 0x0ff) / 255.0f;
            fixedFunctionState.fogColor.a = F32((value >> 24) && 0xff) / 255.0f;
            
            setFogState(state, value);           
            break;
            
        case D3DRS_FOGTABLEMODE:
        
            fixedFunctionState.fogPixelMode = static_cast<D3DFOGMODE>(value);
            break;
            
        case D3DRS_FOGVERTEXMODE:
        
            fixedFunctionState.fogVertexMode = static_cast<D3DFOGMODE>(value);
            break;

        case D3DRS_FOGSTART:
        
            fixedFunctionState.fogStart = *((F32 *) &value);
            break;
            
        case D3DRS_FOGEND:

            fixedFunctionState.fogEnd = *((F32 *) &value);
            break;
            
        case D3DRS_FOGDENSITY:
        
            fixedFunctionState.fogDensity = *((F32 *) &value);
            break;
        
        case D3DRS_RANGEFOGENABLE:
        
            fixedFunctionState.fogRange = (value != FALSE);
            break;
            
        case D3DRS_WRAP0:
        case D3DRS_WRAP1:
        case D3DRS_WRAP2:
        case D3DRS_WRAP3:
        case D3DRS_WRAP4:
        case D3DRS_WRAP5:
        case D3DRS_WRAP6:
        case D3DRS_WRAP7:
        case D3DRS_WRAP8:
        case D3DRS_WRAP9:
        case D3DRS_WRAP10:
        case D3DRS_WRAP11:
        case D3DRS_WRAP12:
        case D3DRS_WRAP13:
        case D3DRS_WRAP14:
        case D3DRS_WRAP15:
        
            printf("WARNING!!! D3D9State::setFixedFunctionState => WRAP state not implemented.\n");
            break;
        
        case D3DRS_SPECULARENABLE:
        
            fixedFunctionState.specularEnable = (value != FALSE);            
            break;
            
        case D3DRS_LIGHTING:
        
            fixedFunctionState.lightingEnabled = (value != FALSE);
            break;
            
        case D3DRS_AMBIENT:

            fixedFunctionState.ambient.r = F32((value >> 16) && 0xff) / 255.0f;
            fixedFunctionState.ambient.g = F32((value >> 8) && 0xff) / 255.0f;
            fixedFunctionState.ambient.b = F32(value & 0x0ff) / 255.0f;
            fixedFunctionState.ambient.a = F32((value >> 24) && 0xff) / 255.0f;
            break;
            
        case D3DRS_COLORVERTEX:
        
            fixedFunctionState.vertexColor = (value != FALSE);
            break;
            
        case D3DRS_LOCALVIEWER:
        
            fixedFunctionState.localViewer = (value != FALSE);
            break;
            
        case D3DRS_NORMALIZENORMALS:
        
            fixedFunctionState.normalizeNormals = (value != FALSE);
            break;
            
        case D3DRS_DIFFUSEMATERIALSOURCE:
        
            fixedFunctionState.diffuseMaterialSource = static_cast<D3DMATERIALCOLORSOURCE>(value);
            break;
            
        case D3DRS_SPECULARMATERIALSOURCE:

            fixedFunctionState.specularMaterialSource = static_cast<D3DMATERIALCOLORSOURCE>(value);
            break;

        case D3DRS_AMBIENTMATERIALSOURCE:

            fixedFunctionState.ambientMaterialSource = static_cast<D3DMATERIALCOLORSOURCE>(value);
            break;

        case D3DRS_EMISSIVEMATERIALSOURCE:

            fixedFunctionState.emissiveMaterialSource = static_cast<D3DMATERIALCOLORSOURCE>(value);
            break;

        case D3DRS_VERTEXBLEND:
            
            fixedFunctionState.vertexBlend = static_cast<D3DVERTEXBLENDFLAGS>(value);
            break;
            
        case D3DRS_INDEXEDVERTEXBLENDENABLE:
        
            fixedFunctionState.indexedVertexBlend = (value != FALSE);
            break;
            
        case D3DRS_TWEENFACTOR:       
               
            fixedFunctionState.tweenFactor = *((F32 *) &value);
            break;
        
        case D3DRS_TEXTUREFACTOR:
            
            fixedFunctionState.textureFactor.r = F32((value >> 16) && 0xff) / 255.0f;
            fixedFunctionState.textureFactor.g = F32((value >> 8) && 0xff) / 255.0f;
            fixedFunctionState.textureFactor.b = F32(value & 0x0ff) / 255.0f;
            fixedFunctionState.textureFactor.a = F32((value >> 24) && 0xff) / 255.0f;
            break;
            
        default:
            CG_ASSERT("Undefined fixed function state.");
            break;
    }    
}

void D3D9State::setLight(DWORD index , CONST D3DLIGHT9* pLight)
{
    //  Check light index.
    if (index < 8)
        memcpy(&fixedFunctionState.lights[index], pLight, sizeof(D3DLIGHT9));
    else
        CG_ASSERT("Light index out of range");
}

void D3D9State::enableLight(DWORD index, BOOL enable)
{
    //  Check light index.
    if (index < 8)
        fixedFunctionState.lightsEnabled[index] = (enable != FALSE);
    else
        CG_ASSERT("Light index out of range");
}

void D3D9State::setMaterial(CONST D3DMATERIAL9* pMaterial)
{
    memcpy(&fixedFunctionState.material, pMaterial, sizeof(D3DMATERIAL9));
}

void D3D9State::setTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX * pMatrix)
{
    if (State < 256)
    {
        switch(State)
        {
            case D3DTS_VIEW:

                memcpy(&fixedFunctionState.view, pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_PROJECTION:

                memcpy(&fixedFunctionState.view, pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE0:

                memcpy(&fixedFunctionState.texture[0], pMatrix, sizeof(D3DMATRIX));
                break;
            
            case D3DTS_TEXTURE1:

                memcpy(&fixedFunctionState.texture[1], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE2:

                memcpy(&fixedFunctionState.texture[2], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE3:

                memcpy(&fixedFunctionState.texture[3], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE4:

                memcpy(&fixedFunctionState.texture[4], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE5:

                memcpy(&fixedFunctionState.texture[5], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE6:

                memcpy(&fixedFunctionState.texture[6], pMatrix, sizeof(D3DMATRIX));
                break;

            case D3DTS_TEXTURE7:

                memcpy(&fixedFunctionState.texture[7], pMatrix, sizeof(D3DMATRIX));
                break;

            default:
                CG_ASSERT("Undefined matrix.");
                break;
        }
    }
    else
    {
        //  Check for world matrix 0.
        if (State == 256)
            memcpy(&fixedFunctionState.world, pMatrix, sizeof(D3DMATRIX));
        /*else
            printf("WARNING!!! D3D9State::setTransform => Only one world matrix supported.\n");*/
    }
}

void D3D9State::setTextureStage(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    if (Stage > 7)
        CG_ASSERT("Texture stage identifier out of range");
        
    switch(Type)
    {
        case D3DTSS_COLOROP:

            fixedFunctionState.textureStage[Stage].colorOp = static_cast<D3DTEXTUREOP>(Value);
            break;
            
        case D3DTSS_COLORARG1:
        
            fixedFunctionState.textureStage[Stage].colorArg1 = Value;
            break;
            
        case D3DTSS_COLORARG2:

            fixedFunctionState.textureStage[Stage].colorArg2 = Value;
            break;

        case D3DTSS_ALPHAOP:
        
            fixedFunctionState.textureStage[Stage].alphaOp = static_cast<D3DTEXTUREOP>(Value);
            break;
            
        case D3DTSS_ALPHAARG1:
        
            fixedFunctionState.textureStage[Stage].alphaArg1 = Value;
            break;
            
        case D3DTSS_ALPHAARG2:
        
            fixedFunctionState.textureStage[Stage].alphaArg2 = Value;
            break;

        case D3DTSS_BUMPENVMAT00:

            fixedFunctionState.textureStage[Stage].bumpEnvMatrix[0][0] = *((F32 *) &Value);
            break;
            
        case D3DTSS_BUMPENVMAT01:

            fixedFunctionState.textureStage[Stage].bumpEnvMatrix[0][1] = *((F32 *) &Value);
            break;

        case D3DTSS_BUMPENVMAT10:

            fixedFunctionState.textureStage[Stage].bumpEnvMatrix[1][0] = *((F32 *) &Value);
            break;

        case D3DTSS_BUMPENVMAT11:

            fixedFunctionState.textureStage[Stage].bumpEnvMatrix[1][1] = *((F32 *) &Value);
            break;

        case D3DTSS_TEXCOORDINDEX:
            
            fixedFunctionState.textureStage[Stage].index = Value;
            break;
            
        case D3DTSS_BUMPENVLSCALE:
        
            fixedFunctionState.textureStage[Stage].bumpEnvLScale = *((F32 *) &Value);
            break;
            
        case D3DTSS_BUMPENVLOFFSET:
        
            fixedFunctionState.textureStage[Stage].bumpEnvLOffset = *((F32 *) &Value);
            break;
            
        case D3DTSS_TEXTURETRANSFORMFLAGS:
            
            fixedFunctionState.textureStage[Stage].transformFlags = static_cast<D3DTEXTURETRANSFORMFLAGS>(Value);
            break;
            
        case D3DTSS_COLORARG0:

            fixedFunctionState.textureStage[Stage].colorArg0 = Value;
            break;
            
        case D3DTSS_ALPHAARG0:

            fixedFunctionState.textureStage[Stage].alphaArg0 = Value;
            break;

        case D3DTSS_RESULTARG:
            
            fixedFunctionState.textureStage[Stage].resultArg = Value;
            break;
            
        case D3DTSS_CONSTANT:

            fixedFunctionState.textureStage[Stage].constant.r = F32((Value >> 16) && 0xff) / 255.0f;
            fixedFunctionState.textureStage[Stage].constant.g = F32((Value >> 8) && 0xff) / 255.0f;
            fixedFunctionState.textureStage[Stage].constant.b = F32(Value && 0xff) / 255.0f;
            fixedFunctionState.textureStage[Stage].constant.a = F32((Value >> 24) && 0xff) / 255.0f;
            break;
            
        default:
            CG_ASSERT("Undefined texture stage state.");
            break;
    }
}

void D3D9State::setBlendingState(D3DRENDERSTATETYPE state , DWORD value) {

    switch (state)
    {
        case D3DRS_ALPHABLENDENABLE:
            alphaBlend = (value != FALSE);
            break;

        case D3DRS_SEPARATEALPHABLENDENABLE:
            separateAlphaBlend = (value != FALSE);
            break;

        case D3DRS_SRCBLEND:
            srcBlend = getGALBlendOption((D3DBLEND)value);
            break;

        case D3DRS_SRCBLENDALPHA:
            srcBlendAlpha = getGALBlendOption((D3DBLEND)value);
            break;

        case D3DRS_DESTBLEND:
            dstBlend = getGALBlendOption((D3DBLEND)value);
            break;

        case D3DRS_DESTBLENDALPHA:
            dstBlendAlpha = getGALBlendOption((D3DBLEND)value);
            break;

        case D3DRS_BLENDOP:
            if (value != 0)
                blendOp = getGALBlendOperation((D3DBLENDOP)value);
            break;

        case D3DRS_BLENDOPALPHA:
            blendOpAlpha = getGALBlendOperation((D3DBLENDOP)value);
            break;

        case D3DRS_BLENDFACTOR:
            GalDev->blending().setBlendColor(0, libGAL::gal_float((value >> 16) & 0xff) / 255.0f,
                                                libGAL::gal_float((value >> 8) & 0xff) / 255.0f,
                                                libGAL::gal_float((value) & 0xff) / 255.0f,
                                                libGAL::gal_float((value >> 24) & 0xff) / 255.0f);
            break;

        default:
            CG_ASSERT("Unsupported blending state.");
            break;

    }
    redefineGALBlend();

}

void D3D9State::setFogState(D3DRENDERSTATETYPE state , DWORD value)
{
    switch (state)
    {
        case D3DRS_FOGENABLE:
            fogEnable = (value != FALSE);
            break;

        case D3DRS_FOGCOLOR:
            fogColor = value;
            break;

        default:
            CG_ASSERT("Unsupported alpha test state.");
            break;
    }
}

void D3D9State::setAlphaTestState(D3DRENDERSTATETYPE state , DWORD value)
{
    switch (state)
    {
        case D3DRS_ALPHATESTENABLE:
            alphaTest = (value != FALSE);
            break;

        case D3DRS_ALPHAREF:
            alphaRef = value;
            break;

        case D3DRS_ALPHAFUNC:
            alphaFunc = (D3DCMPFUNC)value;
            break;

        default:
            CG_ASSERT("Unsupported alpha test state.");
            break;
    }
}

void D3D9State::setZState(D3DRENDERSTATETYPE state , DWORD value)
{
    switch (state)
    {
        case D3DRS_ZENABLE:

            switch(value)
            {
                case D3DZB_FALSE:
                    GalDev->zStencil().setZEnabled(false);
                    break;

                case D3DZB_TRUE:
                    GalDev->zStencil().setZEnabled(true);
                    break;

                case D3DZB_USEW:
                    cout << "D3D9State: WARNING: D3DZB_USEW is not supported." << endl;
                    break;

                default:
                    CG_ASSERT("Undefined z enable parameter value.");
                    break;
            }

            break;

        case D3DRS_ZFUNC:
            switch(value)
            {

                case D3DCMP_LESS:
                    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_LESS);
                    //cout << "D3D9State: ZFUNC = LESS" << endl;
                    break;

                case D3DCMP_LESSEQUAL:
                    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_LESS_EQUAL);
                    //cout << "D3D9State: ZFUNC = LESS_EQUAL" << endl;
                    break;

                case D3DCMP_GREATER:
                    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_GREATER);
                    break;

                case D3DCMP_EQUAL:
                    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_EQUAL);
                    break;

                case D3DCMP_ALWAYS:
                    GalDev->zStencil().setZFunc(libGAL::GAL_COMPARE_FUNCTION_ALWAYS);
                    break;
                
                default:
                    CG_ASSERT("Unsupported z function parameter value.");
                    break;

            }
            break;


        case D3DRS_ZWRITEENABLE:
            if (value != FALSE)
                GalDev->zStencil().setZMask(true);
            else
                GalDev->zStencil().setZMask(false);

            break;

        default:
            CG_ASSERT("Unsupported z test parameter.");
            break;
    }
}

void D3D9State::setStencilState(D3DRENDERSTATETYPE state , DWORD value)
{
    switch (state)
    {
        case D3DRS_STENCILENABLE:
            switch(value)
            {
                case D3DZB_FALSE:
                    stencilEnabled = false;
                    break;

                case D3DZB_TRUE:
                    stencilEnabled = true;
                    break;

                default:
                    CG_ASSERT("Undefined stencil enable parameter value.");
                    break;
            }
            break;

        case D3DRS_STENCILFAIL:
            stencilFail = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_STENCILZFAIL:
            stencilZFail = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_STENCILPASS:
            stencilPass = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_STENCILFUNC:
            stencilFunc = getGALCompareFunction((D3DCMPFUNC)value);
            break;

        case D3DRS_STENCILREF:
            stencilRef = value;
            break;

        case D3DRS_STENCILMASK:
            stencilMask = value;
            break;

        case D3DRS_STENCILWRITEMASK:
            stencilWriteMask = value;
            break;

        case D3DRS_TWOSIDEDSTENCILMODE:
            switch(value)
            {
                case D3DZB_FALSE:
                    twoSidedStencilEnabled = false;
                    break;

                case D3DZB_TRUE:
                    twoSidedStencilEnabled = true;
                    break;

                default:
                    CG_ASSERT("Undefined two sided stencil mode parameter value.");
                    break;
            }
            break;

        case D3DRS_CCW_STENCILFAIL:
            ccwStencilFail = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_CCW_STENCILZFAIL:
            ccwStencilZFail = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_CCW_STENCILPASS:
            ccwStencilPass = getGALStencilOperation((D3DSTENCILOP)value);
            break;

        case D3DRS_CCW_STENCILFUNC:
            ccwStencilFunc = getGALCompareFunction((D3DCMPFUNC)value);
            break;

        default:
            CG_ASSERT("Unsupported stencil paramter.");
            break;
    }

    redefineGALStencil();

}

void D3D9State::setColorWriteEnable(DWORD RenderTargetIndex, DWORD value)
{
    colorwriteRed[RenderTargetIndex]   = ((value & D3DCOLORWRITEENABLE_RED)   != 0);
    colorwriteGreen[RenderTargetIndex] = ((value & D3DCOLORWRITEENABLE_GREEN) != 0);
    colorwriteBlue[RenderTargetIndex]  = ((value & D3DCOLORWRITEENABLE_BLUE)  != 0);
    colorwriteAlpha[RenderTargetIndex] = ((value & D3DCOLORWRITEENABLE_ALPHA) != 0);

    GalDev->blending().setColorMask(RenderTargetIndex, colorwriteRed[RenderTargetIndex], colorwriteGreen[RenderTargetIndex], colorwriteBlue[RenderTargetIndex], colorwriteAlpha[RenderTargetIndex]);
}

void D3D9State::setColorSRGBWrite(DWORD value)
{
    GalDev->setColorSRGBWrite(value != FALSE);
}

void D3D9State::setCullMode(DWORD value) {

    switch (value)
    {
        case D3DCULL_NONE:
            GalDev->rast().setCullMode(libGAL::GAL_CULL_NONE);
            break;

        case D3DCULL_CW:
            GalDev->rast().setCullMode(libGAL::GAL_CULL_FRONT);
            //GalDev->rast().setCullMode(libGAL::GAL_CULL_BACK);
            //GalDev->rast().setFaceMode(libGAL::GAL_FACE_CCW);
            break;

        case D3DCULL_CCW:
            GalDev->rast().setCullMode(libGAL::GAL_CULL_BACK);
            //GalDev->rast().setFaceMode(libGAL::GAL_FACE_CW);
            break;

        default:
            CG_ASSERT("Undefined cull mode value.");
            break;
    }

}

void D3D9State::enableScissorTest(DWORD value)
{
    if (value != FALSE)
        GalDev->rast().enableScissor(true);
    else
        GalDev->rast().enableScissor(false);

}

void D3D9State::scissorRect(UINT x1, UINT y1, UINT x2, UINT y2) {

    GalDev->rast().setScissor(x1, y1, x2 - x1, y2 - y1);

}


void D3D9State::setFreqType(UINT stream, UINT type) {

    if (stream < maxStreams)
        settedVertexBuffers[stream].freqType = type;

}

void D3D9State::setFreqValue(UINT stream, UINT value) {

    if (stream < maxStreams)
        settedVertexBuffers[stream].freqValue = value;

}

void D3D9State::clear(DWORD count, CONST D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {

    // Color mask doesn't afect clear
    //GalDev->blending().setColorMask(true, true, true, true);

    //  Check if clear on the color buffer must be disabled.
    
    D3DSURFACE_DESC sDesc;
    bool disableColorClear;
    
    //  Check if z stencil surface is set to NULL;
    bool disableZStencilClear = (currentZStencilSurface == NULL);
    
    //  Check if the current render surface is set to NULL.
    if (currentRenderSurface[0] == NULL)
    {
        //  Render surface is disabled.
        disableColorClear = true;
    }
    else
    {
        //  Get information about the current render surface.
        currentRenderSurface[0]->GetDesc(&sDesc);

        //  Check if the render surface is of NULL FOURCC type.
        if (sDesc.Format == D3DFMT_NULL)
        {
            //  Render surface is disabled.
            disableColorClear = true;
        }
        else
        {
            //  Check if the z stencil surface is defined.
            if (!disableZStencilClear)
            {
                libGAL::GALRenderTarget *rt = currentRenderSurface[0]->getGALRenderTarget();
                libGAL::GALRenderTarget *zst = currentZStencilSurface->getGALRenderTarget();
                disableColorClear = (rt != NULL) && (zst != NULL) &&
                                    !(colorwriteRed[0] || colorwriteBlue[0] || colorwriteGreen[0] || colorwriteAlpha[0]) &&
                                    ((rt->getWidth() != zst->getWidth()) || (rt->getHeight() != zst->getHeight()));
            }
            else
                disableColorClear = false;
        }
    }
        
    if (rects == NULL)
    {
        if ((flags & D3DCLEAR_TARGET) && !disableColorClear)
        {
                // Clear color buffer with the setted color
                GalDev->clearColorBuffer( (color >> 16) & 0xff, (color >> 8) & 0xff, (color) & 0xff, (color >> 24) & 0xff );
        }

        if ((flags & D3DCLEAR_ZBUFFER) && (flags & D3DCLEAR_STENCIL) && !disableZStencilClear)
        {
            // If flags to clear depth buffer and stencil buffer are active, clear at the same time
            GalDev->clearZStencilBuffer(true, true, z, stencil);
        }
        else if ((flags & D3DCLEAR_STENCIL) && !disableZStencilClear)
        {
            // else clear stencil buffer
            GalDev->clearZStencilBuffer(false, true, z, stencil);
        }
        else if ((flags & D3DCLEAR_ZBUFFER) && !disableZStencilClear)
        {
            // or clear depth buffer
            GalDev->clearZStencilBuffer(true, false, z, stencil);
        }

    }
    else {

        libGAL::gal_bool senabled;
        libGAL::gal_int sx, sy;
        libGAL::gal_uint swidth, sheight;

        GalDev->rast().getScissor(senabled, sx, sy, swidth, sheight);

        GalDev->rast().enableScissor(true);

        for (libGAL::gal_uint i = 0; i < count; i++) {

            GalDev->rast().setScissor(rects[i].x1, rects[i].y1, rects[i].x2 - rects[i].x1, rects[i].y2 - rects[i].y1);

            if ((flags & D3DCLEAR_TARGET) && !disableColorClear)
            {
                // Clear color buffer with the setted color
                GalDev->clearColorBuffer( (color >> 16) & 0xff, (color >> 8) & 0xff, (color) & 0xff, (color >> 24) & 0xff );
            }

            if ((flags & D3DCLEAR_ZBUFFER) && (flags & D3DCLEAR_STENCIL) && !disableZStencilClear)
            {
                // If flags to clear depth buffer and stencil buffer are active, clear at the same time
                GalDev->clearZStencilBuffer(true, true, z, stencil);
            }
            else if ((flags & D3DCLEAR_STENCIL) && !disableZStencilClear)
            {
                // else clear stencil buffer
                GalDev->clearZStencilBuffer(false, true, z, stencil);
            }
            else if ((flags & D3DCLEAR_ZBUFFER) && !disableZStencilClear)
            {
                // or clear depth buffer
                GalDev->clearZStencilBuffer(true, false, z, stencil);
            }
        }

        GalDev->rast().enableScissor(senabled);
        GalDev->rast().setScissor(sx, sy, swidth, sheight);

    }

    //GalDev->blending().setColorMask(colorwriteRed, colorwriteGreen, colorwriteBlue, colorwriteAlpha);

    batchCount++;
}

void D3D9State::setViewport(DWORD x, DWORD y, DWORD width, DWORD height, float minZ, float maxZ)
{
    // Set a new viewport position and size
    GalDev->rast().setViewport(x, y, width, height);

    GalDev->zStencil().setDepthRange(minZ, maxZ);
}

void D3D9State::draw(D3DPRIMITIVETYPE type, UINT start, UINT count)
{

    //  Check if all rendering surfaces are set to NULL.
    if ((currentRenderSurface == NULL) && (currentZStencilSurface == NULL))
        return;
  
    //  Check if the primitive count is 0.
    if (count == 0)
        return;
        
    NativeShader* nativeVertexShader;

    // Set setted shaders to GAL.  Check for problems.
    if (!setGALShaders(nativeVertexShader))
        return;

    //  Reset instancing information.
    instancingMode = false;
    instancesLeft = 1;
    instancesDone = 0;
    
    // Set setted vertex buffers in streams to GAL
    setGALStreams(nativeVertexShader, get_vertex_count(type, count), 0);

    // Set setted textures in samplers to GAL
    setGALTextures();

    // Draw
    GalDev->setPrimitive(getGALPrimitiveType(type));

#ifdef SOFTWARE_INSTANCING
    if (instancingMode) {

        GalDev->draw(start, get_vertex_count(type, count));

        batchCount++;
                /*cout << "Batch: " << batchCount << endl;
                cout << "Instancing: " << instancesLeft << ", " << instancesDone << endl;*/

        GalDev->DBG_dump("GAL_Dump.txt",0);

        GalDev->disableVertexAttributes();

        instancesLeft--; instancesDone++;

        for (; instancesLeft != 0; --instancesLeft) {

            // Set setted vertex buffers in streams to GAL
            setGALStreams(nativeVertexShader, get_vertex_count(type, count), 0);

            GalDev->draw(start, get_vertex_count(type, count));

            batchCount++;
                /*cout << "Batch: " << batchCount << endl;
                cout << "Instancing: " << instancesLeft << ", " << instancesDone << endl;*/

            GalDev->DBG_dump("GAL_Dump.txt",0);

            GalDev->disableVertexAttributes();

            instancesDone++;

        }

    }
    else {

        GalDev->draw(start, get_vertex_count(type, count));

        batchCount++;
                //cout << "Batch: " << batchCount << endl;

        GalDev->DBG_dump("GAL_Dump.txt",0);

        GalDev->disableVertexAttributes();

    }
#else       //  !SOFTWARE_INSTANCING

    GalDev->draw(start, get_vertex_count(type, count), instancesLeft);

    batchCount++;
    //cout << "Batch: " << batchCount << endl;

    //GalDev->DBG_dump("GAL_Dump.txt",0);

    GalDev->disableVertexAttributes();

#endif      //  SOFTWARE_INSTANCIG

    //GalDev->swapBuffers();

    instancingMode = false;

}

int D3D9State::getBatchCounter ()
{
    return batchCount;
}

void D3D9State::drawIndexed(D3DPRIMITIVETYPE type, INT baseVertexIndex, UINT minVertexIndex, UINT numVertices, UINT start, UINT count) 
{
    //  Check if all rendering surfaces are set to NULL.
    if ((currentRenderSurface == NULL) && (currentZStencilSurface == NULL))
        return;

    //  Check if the count is 0.
    if (count == 0)
        return;
    
    //  Reset instancing information.
    instancingMode = false;
    instancesLeft = 1;
    instancesDone = 0;

    NativeShader* nativeVertexShader;
    
    // Set setted shaders to GAL.  Check for problems.
    if (!setGALShaders(nativeVertexShader))
        return;

    // Set setted vertex buffers in streams to GALdu
    setGALStreams(nativeVertexShader, get_vertex_count(type, count), minVertexIndex + numVertices);

    // Set setted textures in samplers to GAL
    setGALTextures();

    // Set setted indices buffer to GAL
    setGALIndices();

    // Draw
    GalDev->setPrimitive(getGALPrimitiveType(type));

#ifdef SOFTWARE_INSTANCING
    if (instancingMode) {

        GalDev->drawIndexed(start, get_vertex_count(type, count), minVertexIndex, 0,baseVertexIndex);

        batchCount++;
                /*cout << "Batch: " << batchCount << endl;
                cout << "Instancing: " << instancesLeft << ", " << instancesDone << endl;*/

        GalDev->DBG_dump("GAL_Dump.txt",0);

        GalDev->disableVertexAttributes();

        instancesLeft--; instancesDone++;

        for (; instancesLeft != 0; --instancesLeft) {

            // Set setted vertex buffers in streams to GAL
            setGALStreams(nativeVertexShader, get_vertex_count(type, count), minVertexIndex + numVertices);

            // Set setted indices buffer to GAL
            //setGALIndices();

            GalDev->drawIndexed(start, get_vertex_count(type, count), minVertexIndex, 0,baseVertexIndex);

            batchCount++;
                /*cout << "Batch: " << batchCount << endl;
                cout << "Instancing: " << instancesLeft << ", " << instancesDone << endl;*/

            GalDev->DBG_dump("GAL_Dump.txt",0);

            GalDev->disableVertexAttributes();

            instancesDone++;

        }

    }
    else {

        GalDev->drawIndexed(start, get_vertex_count(type, count), minVertexIndex, 0,baseVertexIndex);

        batchCount++;
                //cout << "Batch: " << batchCount << endl;

        GalDev->DBG_dump("GAL_Dump.txt",0);

        GalDev->disableVertexAttributes();

    }
#else       //  !SOFTWARE_INSTANCING

    GalDev->drawIndexed(start, get_vertex_count(type, count), minVertexIndex, 0, baseVertexIndex, instancesLeft);

    //char dump[128];
    //sprintf(dump, "GALDevice_batch_%d", batchCount);
    //GalDev->DBG_dump(dump,0);

    batchCount++;
    //cout << "Batch: " << batchCount << endl;

    GalDev->disableVertexAttributes();

#endif      //  SOFTWARE_INSTANCING


    //GalDev->swapBuffers();

    instancingMode = false;

}

void D3D9State::swapBuffers() {

    // Swap buffers
    GalDev->swapBuffers();

}



bool D3D9State::setGALShaders(NativeShader* &nativeVertexShader)
{
    FFShaderGenerator ffShGen;

    //  Check for POSITIONT in the vertex declaration.
    bool disableVertexProcessing = false;    
    
    //  Check if there is a vertex declaration defined.
    if (settedVertexDeclaration != NULL)
    {
        //  Get the vertex declaration.
        vector<D3DVERTEXELEMENT9> vElements = settedVertexDeclaration->getVertexElements();
        
        //  Search for POSITIONT usage which disables vertex processing.
        for(U32 d = 0; d < vElements.size() && !disableVertexProcessing; d++)
            disableVertexProcessing = (vElements[d].Usage == D3DDECLUSAGE_POSITIONT);
    }

    //if (disableVertexProcessing)
    //    printf("D3D9State::setGALShaders => Disabling vertex processing due to POSITIONT in vertex declaration.\n");

    // Set vertex shader
    if ((settedVertexShader != NULL) && !disableVertexProcessing)
    {
        nativeVertexShader = settedVertexShader->getNativeVertexShader();
        
        // If the setted shaders have a jump instruction the draw is skipped.
        if (nativeVertexShader->untranslatedControlFlow)
        {
            cout << " * WARNING: This batch is using shaders with untranslated jump instructionc, skiping it.\n";
            return false;
        }
        
        libGAL::GALShaderProgram* galVertexShader = settedVertexShader->getAcdVertexShader();      

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader lenght: " << nativeVertexShader->lenght << endl; ) )
        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader assembler: \n" << nativeVertexShader->debug_assembler << endl; ) )
        //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader binary: \n" << nativeVertexShader->debug_binary << endl; ) )
        //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader ir: \n" << nativeVertexShader->debug_ir << endl; ) )

        for(libGAL::gal_uint c = 0; c < MAX_VERTEX_SHADER_CONSTANTS; c++)
        {
            if (settedVertexShaderConstantsTouched[c])
                galVertexShader->setConstant(c, settedVertexShaderConstants[c]);
        }

        for (libGAL::gal_uint c = 0; c < 16; c++)
        {
            if (settedVertexShaderConstantsIntTouched[c])
                galVertexShader->setConstant(INTEGER_CONSTANT_START + c, (libGAL::gal_float *) settedVertexShaderConstantsInt[c]);
           
            if (settedVertexShaderConstantsBoolTouched[c])
            {
                libGAL::gal_float psConstant[4];
                *((BOOL *) &psConstant[0]) = settedVertexShaderConstantsBool[c];
                psConstant[1] = 0.0f;
                psConstant[2] = 0.0f;
                psConstant[3] = 0.0f;
                galVertexShader->setConstant(BOOLEAN_CONSTANT_START + c, psConstant);
            }
        }

        std::list<ConstRegisterDeclaration>::iterator itCRD;
        for (itCRD = nativeVertexShader->declaration.constant_registers.begin(); itCRD != nativeVertexShader->declaration.constant_registers.end(); itCRD++) {
            if (itCRD->defined) {
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State vertex shader constants defined: \n  * constNum: " << itCRD->native_register << "\n  * constant: " << itCRD->value.x << ", " << itCRD->value.y << ", " << itCRD->value.z << ", " << itCRD->value.w << endl; ) )
                libGAL::gal_float vsConstant[4];
                vsConstant[0] = itCRD->value.value.floatValue.x;
                vsConstant[1] = itCRD->value.value.floatValue.y;
                vsConstant[2] = itCRD->value.value.floatValue.z;
                vsConstant[3] = itCRD->value.value.floatValue.w;

                galVertexShader->setConstant(itCRD->native_register, vsConstant);
            }
        }

        GalDev->setVertexShader(galVertexShader);
    }
    else
    {
        //cout << " * WARNING: This batch is using vertex fixed function.\n";

        //  Delete previous native vertex shader if required.
        if (nativeFFVSh != NULL)
            delete nativeFFVSh;
        
        //  Generate a D3D9 pixel shader for the current fixed function state.        
        FFGeneratedShader *ffVSh;
        ffVSh = ffShGen.generate_vertex_shader(fixedFunctionState);

        //  Build the intermediate representation for the generated D3D9 vertex shader.
        IR *ffVShIR;
        ShaderTranslator::get_instance().buildIR(ffVSh->code, ffVShIR);

        //printf("Translating fixed function vertex shader.\n");

        //  Translate the generated D3D9 vertex shader to CG1 instructions.
        nativeVertexShader = nativeFFVSh = ShaderTranslator::get_instance().translate(ffVShIR);

        //printf("-------- End of FF VSH translation --------\n");
        
        //  Set the generated code in the GAL fixed function vertex shader.
        fixedFunctionVertexShader->setCode(nativeFFVSh->bytecode, nativeFFVSh->lenght);

        //  Update the constants defined with value in the generated D3D9 vertex shader.
        std::list<ConstRegisterDeclaration>::iterator itCRD;
        for (itCRD = nativeFFVSh->declaration.constant_registers.begin();
             itCRD != nativeFFVSh->declaration.constant_registers.end();
             itCRD++)
        {
            //  Check if the constant was defined with a value.
            if (itCRD->defined)
            {
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State vertex shader constants defined: \n  * constNum: " << itCRD->native_register << "\n  * constant: " << itCRD->value.x << ", " << itCRD->value.y << ", " << itCRD->value.z << ", " << itCRD->value.w << endl; ) )
                libGAL::gal_float vsConstant[4];
                vsConstant[0] = itCRD->value.value.floatValue.x;
                vsConstant[1] = itCRD->value.value.floatValue.y;
                vsConstant[2] = itCRD->value.value.floatValue.z;
                vsConstant[3] = itCRD->value.value.floatValue.w;

                fixedFunctionVertexShader->setConstant(itCRD->native_register, vsConstant);
            }
        }

        //  Update the fixed function constants used by the generated D3D9 vertex shader.
        std::list<FFConstRegisterDeclaration>::iterator itFFCRD;
        for(itFFCRD = ffVSh->const_declaration.begin(); itFFCRD != ffVSh->const_declaration.end(); itFFCRD++)
        {
            libGAL::gal_float vsConstant[4];
            
            //  Translate the D3D9 constant identifier to the CG1 constant identifier.
            U32 constantID;
            bool found = false;
            std::list<ConstRegisterDeclaration>::iterator itCRD;
            for (itCRD = nativeFFVSh->declaration.constant_registers.begin();
                 itCRD != nativeFFVSh->declaration.constant_registers.end() && !found;
                 itCRD++)
            {
                //  Check if the register matches.
                if (itCRD->d3d_register == itFFCRD->constant.num)
                {
                    constantID = itCRD->native_register;
                    found = true;
                }
            }
            
            //  Check the constant usage type.
            switch(itFFCRD->usage.usage)
            {
	            case FF_NONE:
	                
	                // I think this one shouldn't be allowedd.
	                break;
	                
	            case FF_VIEWPORT:
	            
	                //  Check the usage index.
	                if (itFFCRD->usage.index == 0)
	                {
	                    U32 width;
	                    U32 height;
	                    
	                    //  Use the current RT dimensions.
	                    if (currentRenderSurface[0] != NULL)
	                    {
	                        width = currentRenderSurface[0]->getGALRenderTarget()->getWidth();
	                        height = currentRenderSurface[0]->getGALRenderTarget()->getHeight();
                        }
                        else
                        {
                            if (currentZStencilSurface != NULL)
                            {
                                width = currentZStencilSurface->getGALRenderTarget()->getWidth();
                                height = currentZStencilSurface->getGALRenderTarget()->getHeight();
                            }
                            else
                            {
                                width = 1;
                                height = 1;
                            }
                        }

#ifdef LOWER_RESOLUTION_HACK
                        width = (width > 1) ? (width << 1) : 1;
                        height = (height > 1) ? (height << 1) : 1;
#endif
	                    
	                    //  viewport0 = {2/width, -2/height, 1, 1}
	                    vsConstant[0] = 2.0f / F32(width);
	                    vsConstant[1] = -2.0f / F32(height);
	                    vsConstant[2] =
	                    vsConstant[3] = 1.0f;
                                               
                        //  Set the constant in the fixed function vertex shader.
                        fixedFunctionVertexShader->setConstant(constantID, vsConstant);
	                }
	                
	                break;
	                
	            case FF_WORLDVIEWPROJ:
	            case FF_WORLDVIEW:
	            case FF_WORLDVIEW_IT:
	            case FF_VIEW_IT:
	            case FF_WORLD:
	            case FF_MATERIAL_EMISSIVE:
	            case FF_MATERIAL_SPECULAR:
	            case FF_MATERIAL_DIFFUSE:
	            case FF_MATERIAL_AMBIENT:
	            case FF_MATERIAL_POWER:
	            case FF_AMBIENT:
	            case FF_LIGHT_POSITION:
	            case FF_LIGHT_DIRECTION:
	            case FF_LIGHT_AMBIENT:
	            case FF_LIGHT_DIFFUSE:
	            case FF_LIGHT_SPECULAR:
	            case FF_LIGHT_RANGE:
	            case FF_LIGHT_ATTENUATION:
	            case FF_LIGHT_SPOT:
	                
	                //  Not implemented.
	                break;
	                
	            default:
	                CG_ASSERT("Undefined fixed function constant usage.");
	                break;
            }
        }
        
        //  Set the fixed function vertex shader as the current GAL vertex shader.
        GalDev->setVertexShader(fixedFunctionVertexShader);
    }

    // Set pixel shader
    if (settedPixelShader != NULL)
    {
        NativeShader* nativePixelShader;

        if (alphaTest)
            nativePixelShader = settedPixelShader->getNativePixelShader(alphaFunc, fogEnable);
        else
            nativePixelShader = settedPixelShader->getNativePixelShader(D3DCMP_ALWAYS, fogEnable);

        // If the setted shaders have a jump instruction the draw is skipped.
        if (nativePixelShader->untranslatedControlFlow)
        {
            cout << " * WARNING: This batch is using shaders with untranslated jump instructionc, skiping it.\n";
            return false;
        }
    
        libGAL::GALShaderProgram* galPixelShader;

        if (alphaTest)
            galPixelShader = settedPixelShader->getAcdPixelShader(alphaFunc, fogEnable);
        else
            galPixelShader = settedPixelShader->getAcdPixelShader(D3DCMP_ALWAYS, fogEnable);

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader lenght: " << nativePixelShader->lenght << endl; ) )
        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader assembler: \n" << nativePixelShader->debug_assembler << endl; ) )
        //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader binary: \n" << nativePixelShader->debug_binary << endl; ) )
        //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State shader ir: \n" << nativePixelShader->debug_ir << endl; ) )

        for(libGAL::gal_uint c = 0; c < MAX_PIXEL_SHADER_CONSTANTS; c++)
        {
            if (settedPixelShaderConstantsTouched[c])
                galPixelShader->setConstant(c, settedPixelShaderConstants[c]);
        }
        
        for (libGAL::gal_uint c = 0; c < 16; c++)
        {
            if (settedPixelShaderConstantsIntTouched[c])
                galPixelShader->setConstant(INTEGER_CONSTANT_START + c, (libGAL::gal_float *) settedPixelShaderConstantsInt[c]);
           
            if (settedPixelShaderConstantsBoolTouched[c])
            {
                libGAL::gal_float psConstant[4];
                *((BOOL *) &psConstant[0]) = settedPixelShaderConstantsBool[c];
                psConstant[1] = 0.0f;
                psConstant[2] = 0.0f;
                psConstant[3] = 0.0f;
                galPixelShader->setConstant(BOOLEAN_CONSTANT_START + c, psConstant);
            }
        }

        std::list<ConstRegisterDeclaration>::iterator itCRD;
        for (itCRD = nativePixelShader->declaration.constant_registers.begin(); itCRD != nativePixelShader->declaration.constant_registers.end(); itCRD++)
        {
            if (itCRD->defined) {
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State pixel shader constants defined: \n  * constNum: " << itCRD->native_register << "\n  * constant: " << itCRD->value.x << ", " << itCRD->value.y << ", " << itCRD->value.z << ", " << itCRD->value.w << endl; ) )
                libGAL::gal_float psConstant[4];
                psConstant[0] = itCRD->value.value.floatValue.x;
                psConstant[1] = itCRD->value.value.floatValue.y;
                psConstant[2] = itCRD->value.value.floatValue.z;
                psConstant[3] = itCRD->value.value.floatValue.w;

                galPixelShader->setConstant(itCRD->native_register, psConstant);
            }
        }

        if (alphaTest)
        {
            libGAL::gal_float alphaR[4];
            alphaR[0] = (float)alphaRef / 255.f;
            alphaR[1] = (float)alphaRef / 255.f;
            alphaR[2] = (float)alphaRef / 255.f;
            alphaR[3] = (float)alphaRef / 255.f;
            galPixelShader->setConstant(nativePixelShader->declaration.alpha_test.alpha_const_ref, alphaR);

            libGAL::gal_float alphaM1[4];
            alphaM1[0] = -1.f;
            alphaM1[1] = -1.f;
            alphaM1[2] = -1.f;
            alphaM1[3] = -1.f;
            galPixelShader->setConstant(nativePixelShader->declaration.alpha_test.alpha_const_minus_one, alphaM1);
        }
        
        if (fogEnable)
        {
            libGAL::gal_float fogColorConst[4];
            fogColorConst[0] = float((fogColor >> 16) & 0xFF) / 255.0f;
            fogColorConst[1] = float((fogColor >>  8) & 0xFF) / 255.0f;
            fogColorConst[2] = float( fogColor        & 0xFF) / 255.0f;
            fogColorConst[3] = 1.0f;
            galPixelShader->setConstant(nativePixelShader->declaration.fog.fog_const_color, fogColorConst);
        }

        GalDev->setFragmentShader(galPixelShader);
    }
    else
    {
        //cout << " * WARNING: This batch is using pixel fixed function, skiping it.\n";

        FFGeneratedShader *ffPSh;
        ffPSh = ffShGen.generate_pixel_shader(fixedFunctionState);

        IR *ffPShIR;
        ShaderTranslator::get_instance().buildIR(ffPSh->code, ffPShIR);
        
        //printf("Translating fixed function pixel shader.\n");            
        NativeShader *nativeFFPSh = ShaderTranslator::get_instance().translate(ffPShIR);
        //printf("-------- End of FF PSH translation --------\n");

        //  Set the generated code in the GAL fixed function pixel shader.
        fixedFunctionPixelShader->setCode(nativeFFPSh->bytecode, nativeFFPSh->lenght);

        //  Update the constants defined with value in the generated D3D9 pixel shader.
        std::list<ConstRegisterDeclaration>::iterator itCRD;
        for (itCRD = nativeFFPSh->declaration.constant_registers.begin();
             itCRD != nativeFFPSh->declaration.constant_registers.end();
             itCRD++)
        {
            //  Check if the constant was defined with a value.
            if (itCRD->defined)
            {
                //D3D_DEBUG( D3D_DEBUG( cout << "D3D9State pixel shader constants defined: \n  * constNum: " << itCRD->native_register << "\n  * constant: " << itCRD->value.x << ", " << itCRD->value.y << ", " << itCRD->value.z << ", " << itCRD->value.w << endl; ) )
                libGAL::gal_float psConstant[4];
                psConstant[0] = itCRD->value.value.floatValue.x;
                psConstant[1] = itCRD->value.value.floatValue.y;
                psConstant[2] = itCRD->value.value.floatValue.z;
                psConstant[3] = itCRD->value.value.floatValue.w;

                fixedFunctionPixelShader->setConstant(itCRD->native_register, psConstant);
            }
        }

        //  Set the fixed function pixel shader as the current GAL pixel shader.
        GalDev->setFragmentShader(fixedFunctionPixelShader);
    }

    return true;
}

void D3D9State::setGALStreams(NativeShader* nativeVertexShader, UINT count, UINT maxVertex) 
{

    // Set stream buffers
    std::vector<StreamDescription>::iterator itSS;
    libGAL::gal_uint galStream = 0;

    std::vector<D3DVERTEXELEMENT9> vElements;

    if (settedVertexDeclaration != NULL)
    {
        // If exist the setted vertex declaration, use it
        vElements = settedVertexDeclaration->getVertexElements();
    }
    else 
    {
        // If not exist the setted vertex declaration or is NULL, create one using FVF
        createFVFVertexDeclaration();
        vElements = fvfVertexDeclaration;
    }

    // For each vertex buffer setted
    for (libGAL::gal_uint i = 0; i < maxStreams; i++) 
    {
        if (settedVertexBuffers[i].streamData != NULL) 
        {
            libGAL::GALBuffer* currentBuffer;
            UINT currentOffset;
            UINT currentStride;

            
#ifdef SOFTWARE_INSTANCING
            if (settedVertexBuffers[i].freqType == D3DSTREAMSOURCE_INDEXEDDATA) {

                if (instancingMode == false)
                {
                    instancingMode = true;
                    instancesLeft = settedVertexBuffers[i].freqValue;
                    instancesDone = 0;
                }

                currentBuffer = settedVertexBuffers[i].streamData->getAcdVertexBuffer();
                currentOffset = settedVertexBuffers[i].offset;
                currentStride = settedVertexBuffers[i].stride;
            }
            else if (settedVertexBuffers[i].freqType == D3DSTREAMSOURCE_INSTANCEDATA)
            {
                instancingStride = settedVertexBuffers[i].freqValue;

                // Build a new buffer
                libGAL::gal_ubyte* transfData = new libGAL::gal_ubyte[(count + maxVertex) * settedVertexBuffers[i].stride];

                const libGAL::gal_ubyte* origData = settedVertexBuffers[i].streamData->getAcdVertexBuffer()->getData();

                for (libGAL::gal_uint j = 0; j < count + maxVertex; j++) {
                    memcpy(&transfData[j * settedVertexBuffers[i].stride], &origData[settedVertexBuffers[i].offset + (instancesDone * instancingStride) * settedVertexBuffers[i].stride], settedVertexBuffers[i].stride);
                }

                if (transfBuffer != NULL)
                    GalDev->destroy(transfBuffer);

                transfBuffer = GalDev->createBuffer((count + maxVertex) * settedVertexBuffers[i].stride, transfData);

                //cout << "Batch: " << batchCount << endl;

                /*if (instancingMode == true) {
                    cout << "origData: " << "(" << *((float *)&(origData[0])) << ", " << *((float *)&(origData[4])) << ", " << *((float *)&(origData[8])) << ", " << *((float *)&(origData[12])) << ")" << endl;
                    cout << "origData: " << "(" << *((float *)&(origData[0 + 16])) << ", " << *((float *)&(origData[4 + 16])) << ", " << *((float *)&(origData[8 + 16])) << ", " << *((float *)&(origData[12 + 16])) << ")" << endl;
                    cout << "origData: " << "(" << *((float *)&(origData[0 + 32])) << ", " << *((float *)&(origData[4 + 32])) << ", " << *((float *)&(origData[8 + 32])) << ", " << *((float *)&(origData[12 + 32])) << ")" << endl;

                    cout << "transfData: " << "(" << *((float *)&(transfData[0])) << ", " << *((float *)&(transfData[4])) << ", " << *((float *)&(transfData[8])) << ", " << *((float *)&(transfData[12])) << ")" << endl;
                    cout << "transfData: " << "(" << *((float *)&(transfData[0 + 16])) << ", " << *((float *)&(transfData[4 + 16])) << ", " << *((float *)&(transfData[8 + 16])) << ", " << *((float *)&(transfData[12 + 16])) << ")" << endl;
                    cout << "transfData: " << "(" << *((float *)&(transfData[0 + 32])) << ", " << *((float *)&(transfData[4 + 32])) << ", " << *((float *)&(transfData[8 + 32])) << ", " << *((float *)&(transfData[12 + 32])) << ")" << endl;

                    ofstream out;

                    out.open("GALorigData.txt");

                    if ( !out.is_open() )
                        CG_ASSERT("Dump failed (output file could not be opened)");


                    //out << " Dumping Stream " << _STREAM_ID << endl;
                    out << "\t Stride: " << settedVertexBuffers[i].stride << endl;
                    //out << "\t Data type: " << _componentsType << endl;
                    out << "\t Size: " << settedVertexBuffers[i].streamData->getAcdVertexBuffer()->getSize() << endl;
                    //out << "\t Offset: " << _offset << endl;
                    out << "\t Dumping stream content: " << endl;

                    //((GALBufferImp*)_buffer)->dumpBuffer(out, _componentsType, _components, _stride, _offset);

                    for (int j = 0; j < ((settedVertexBuffers[i].streamData->getAcdVertexBuffer()->getSize()) / settedVertexBuffers[i].stride); j++) {
                                    cout << "(" << *((float *)&(origData[0 + 48 * j])) << ", " << *((float *)&(origData[4 + 48 * j])) << ", " << *((float *)&(origData[8 + 48 * j])) << ", " << *((float *)&(origData[12 + 48 * j])) << ") ";
                                    cout << "(" << *((float *)&(origData[0 + 16 + 48 * j])) << ", " << *((float *)&(origData[4 + 16 + 48 * j])) << ", " << *((float *)&(origData[8 + 16 + 48 * j])) << ", " << *((float *)&(origData[12 + 16 + 48 * j])) << ") ";
                                    cout << "(" << *((float *)&(origData[0 + 32 + 48 * j])) << ", " << *((float *)&(origData[4 + 32 + 48 * j])) << ", " << *((float *)&(origData[8 + 32 + 48 * j])) << ", " << *((float *)&(origData[12 + 32 + 48 * j])) << ")" << endl;
                    }

                    out.close();

                    }*/

                delete[] transfData;

                currentBuffer = transfBuffer;
                currentOffset = 0;
                currentStride = settedVertexBuffers[i].stride;
            }
            else
            {
                currentBuffer = settedVertexBuffers[i].streamData->getAcdVertexBuffer();
                currentOffset = settedVertexBuffers[i].offset;
                currentStride = settedVertexBuffers[i].stride;
            }

#else   //  !SOFTWARE_INSTANCING


            //  Check if instancing is enabled in the current stream.
            if (settedVertexBuffers[i].freqType == D3DSTREAMSOURCE_INDEXEDDATA)
            {
                //  Check if instancing was already enabled.
                if (!instancingMode)
                {
                    //  Enable instancing.
                    instancingMode = true;
                    instancesLeft = settedVertexBuffers[i].freqValue;
                    instancesDone = 0;
                }
            }
            
            currentBuffer = settedVertexBuffers[i].streamData->getAcdVertexBuffer();
            currentOffset = settedVertexBuffers[i].offset;
            currentStride = settedVertexBuffers[i].stride;

#endif  //  SOFTWARE_INSTANCING

            std::vector<D3DVERTEXELEMENT9>::iterator itVE;

            // For each element declared
            for (itVE = vElements.begin(); itVE != vElements.end(); itVE++)
            {
                // If is the vertex buffer of the element
                if (itVE->Stream == i)
                {
                    libGAL::GAL_STREAM_DESC streamDescription;
                    streamDescription.offset = itVE->Offset + currentOffset;
                    streamDescription.stride = currentStride;
                    getTypeAndComponents((D3DDECLTYPE)itVE->Type, &(streamDescription.components), &(streamDescription.type));
                    

                    //  Check per index/vertex frequency or per instance frequency.
                    if (settedVertexBuffers[i].freqType == D3DSTREAMSOURCE_INSTANCEDATA)
                        streamDescription.frequency = 1;
                    else if (settedVertexBuffers[i].freqType == D3DSTREAMSOURCE_INDEXEDDATA)
                        streamDescription.frequency = 0;
                    else
                        streamDescription.frequency = 0;

                    // Set the vertex buffer to GAL
                    GalDev->stream(galStream).set(currentBuffer, streamDescription);

                    D3D_DEBUG( D3D_DEBUG( cout << "IDEVICE9: Stream " << dec << galStream << ":\n  * Offset: " << streamDescription.offset << "\n  * Stride: " << streamDescription.stride << "\n  * Components: " << streamDescription.components << "\n  * Type: " << streamDescription.type << endl; ) )

                    std::list<InputRegisterDeclaration>::iterator itIRD;

                    bool equivalentInput = false;

                    // Search for an input attribute in the shader corresponding with the vertex declaration.
                    for (itIRD = nativeVertexShader->declaration.input_registers.begin();
                         itIRD != nativeVertexShader->declaration.input_registers.end() && !equivalentInput; itIRD++)
                    {
                        //  There is a special case for the vertex position attribute.  It can be defined with usage
                        //  D3DDECLUSAGE_POSITION (0) or D3DDECLUSAGE_POSITIONT (9).  The difference is that
                        //  D3DDECLUSAGE_POSITIONT represents transformed vertices, in viewport space, coordinates
                        //  from [0,0] to [viewport width, viewport height], and disables vertex processing (however
                        //  I'm not sure if it actually disables vertex shading or only applies to the fixed function).
                        //
                        //  CHECK: More special cases may be possible.
                        equivalentInput = ((itIRD->usage == itVE->Usage) && (itIRD->usage_index == itVE->UsageIndex)) ||
                                          ((itIRD->usage == D3DDECLUSAGE_POSITION) && (itVE->Usage == D3DDECLUSAGE_POSITIONT) &&
                                           (itIRD->usage_index == itVE->UsageIndex));

                        // If the vertex buffer have the same usage as the register
                        if (equivalentInput)
                        {
                            // Enable the vertex attribute with this stream
                            D3D_DEBUG( D3D_DEBUG( cout << "IDEVICE9: Stream usage: " << (unsigned int)itIRD->usage << "  usage index: " << (unsigned int)itIRD->usage_index << endl; ) )
                            GalDev->enableVertexAttribute(itIRD->native_register ,galStream);
                            D3D_DEBUG( D3D_DEBUG( cout << "IDEVICE9: Stream " << dec << galStream << " setted as vertex attribute number " << itIRD->native_register << endl; ) )
                        }
                    } //End FOR
                    galStream++;
                } // end IF
            } // end FOR
        }
    }// End FOR each vertexBuffer

}

void D3D9State::setGALTextures()
{
    //std::vector<SamplerDescription>::iterator itTS;

    bool usedSamplers[maxSamplers];

    for (libGAL::gal_uint i = 0; i < maxSamplers; i++)
    {
    //for (itTS = settedTextures.begin(); itTS != settedTextures.end(); itTS++) {

        //if (itTS->first < 16) {
            if (settedTextures[i].samplerData == NULL)
            {
                //  Set this sampler as not used by a pixel shader sampler.
                usedSamplers[i] = false;

                GalDev->sampler(i).setEnabled(false);
            }
            else
            {
                //  Set this sampler as already used by a pixel shader sampler.
                usedSamplers[i] = true;
                
                GalDev->sampler(i).setEnabled(true);

                /*if (batchCount == 619) {
                    if (textureTextureType[settedTextures[i].samplerData] == D3D9_TEXTURE2D) {
                        cout << "Sampler: " << i << endl;
                        cout << " - Format: " << textures2D[(AITextureImp9*)settedTextures[i].samplerData]->getFormat(0) << endl;
                        cout << " - Width: " << textures2D[(AITextureImp9*)settedTextures[i].samplerData]->getWidth(0) << endl;
                        cout << " - Height: " << textures2D[(AITextureImp9*)settedTextures[i].samplerData]->getHeight(0) << endl;
                        cout << " - MemoryLayout: " << textures2D[(AITextureImp9*)settedTextures[i].samplerData]->getMemoryLayout() << endl;
                        cout << " - SettedMipmaps: " << textures2D[(AITextureImp9*)settedTextures[i].samplerData]->getSettedMipmaps() << endl;
                    }
                }*/

                if (textureTextureType[settedTextures[i].samplerData] == D3D9_TEXTURE2D) {
                    GalDev->sampler(i).setTexture(/*textures2D[(AITextureImp9*)settedTextures[i].samplerData]*/((AITextureImp9*)settedTextures[i].samplerData)->getGALTexture2D());
                    if (((AITextureImp9*)settedTextures[i].samplerData)->usePCF()) {
                        GalDev->sampler(i).setEnableComparison(true);
                        GalDev->sampler(i).setComparisonFunction(libGAL::GAL_TEXTURE_COMPARISON_GEQUAL);
                    }
                    else {
                        GalDev->sampler(i).setEnableComparison(false);
                    }
                }
                else if (textureTextureType[settedTextures[i].samplerData] == D3D9_CUBETEXTURE) {
                    GalDev->sampler(i).setTexture(/*cubeTextures[(AICubeTextureImp9*)settedTextures[i].samplerData]*/((AICubeTextureImp9*)settedTextures[i].samplerData)->getGALCubeTexture());
                    GalDev->sampler(i).setEnableComparison(false);
                }
                else if (textureTextureType[settedTextures[i].samplerData] == D3D9_VOLUMETEXTURE) {
                    GalDev->sampler(i).setTexture(/*volumeTextures[(AIVolumeTextureImp9*)settedTextures[i].samplerData]*/((AIVolumeTextureImp9*)settedTextures[i].samplerData)->getGALVolumeTexture());
                    GalDev->sampler(i).setEnableComparison(false);
                }
               
                GalDev->sampler(i).setSRGBConversion((settedTextures[i].sRGBTexture == 1));
                
                GalDev->sampler(i).setMagFilter(getGALTextureMagFilter((D3DTEXTUREFILTERTYPE)settedTextures[i].magFilter));
                GalDev->sampler(i).setMinFilter(getGALTextureMinFilter((D3DTEXTUREFILTERTYPE)settedTextures[i].minFilter, (D3DTEXTUREFILTERTYPE)settedTextures[i].mipFilter));

                if (settedTextures[i].minFilter == D3DTEXF_ANISOTROPIC)
                    GalDev->sampler(i).setMaxAnisotropy(settedTextures[i].maxAniso);
                else
                    GalDev->sampler(i).setMaxAnisotropy(1);

                GalDev->sampler(i).setTextureAddressMode(libGAL::GAL_TEXTURE_S_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS)settedTextures[i].addressU));
                GalDev->sampler(i).setTextureAddressMode(libGAL::GAL_TEXTURE_T_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS)settedTextures[i].addressV));
                GalDev->sampler(i).setTextureAddressMode(libGAL::GAL_TEXTURE_R_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS)settedTextures[i].addressW));

            }
        //}

    }

    //  Update state for the vertex texture samplers.  Map the vertex texture samplers to sampler 12 - 15 if those are available.
    for (libGAL::gal_uint s = 0; s < maxVertexSamplers; s++)
    {
        //  Check if a texture is set for the vertex texture sampler.
        if (settedVertexTextures[s].samplerData == NULL)
        {
            //  Do nothing.  The corresponding GAL samplers were disabled by the previous loop setting the pixel samplers.
        }
        else
        {
            //  Remap the vertex texture sampler to the GAL samplers 12 to 15.
            U32 mappedSampler = s + 12;
            
            //  Check if the GAL sampler wasn't used by the pixel texture samplers.
            if (!usedSamplers[mappedSampler])
            {
                //  Enable the GAL sampler.
                GalDev->sampler(mappedSampler).setEnabled(true);

                //  Check the type of texture attached to the sampler.
                switch(textureTextureType[settedVertexTextures[s].samplerData])
                {
                
                    case D3D9_TEXTURE2D:

                        GalDev->sampler(mappedSampler).setTexture(((AITextureImp9 *) settedVertexTextures[s].samplerData)->getGALTexture2D());
                        if (((AITextureImp9 *) settedVertexTextures[s].samplerData)->usePCF())
                        {
                            GalDev->sampler(mappedSampler).setEnableComparison(true);
                            GalDev->sampler(mappedSampler).setComparisonFunction(libGAL::GAL_TEXTURE_COMPARISON_GEQUAL);
                        }
                        else 
                        {
                            GalDev->sampler(mappedSampler).setEnableComparison(false);
                        }
                        break;
                        
                    case D3D9_CUBETEXTURE:
                    
                        GalDev->sampler(mappedSampler).setTexture(((AICubeTextureImp9 *) settedVertexTextures[s].samplerData)->getGALCubeTexture());
                        GalDev->sampler(mappedSampler).setEnableComparison(false);
                        break;
                        
                    case D3D9_VOLUMETEXTURE:
                    
                        GalDev->sampler(mappedSampler).setTexture(((AIVolumeTextureImp9 *) settedVertexTextures[s].samplerData)->getGALVolumeTexture());
                        GalDev->sampler(mappedSampler).setEnableComparison(false);
                        break;
                        
                    default:
                        CG_ASSERT("Undefined texture type.");
                        break;
                }
                               
                GalDev->sampler(mappedSampler).setSRGBConversion((settedVertexTextures[s].sRGBTexture == 1));
                
                GalDev->sampler(mappedSampler).setMagFilter(getGALTextureMagFilter((D3DTEXTUREFILTERTYPE) settedVertexTextures[s].magFilter));
                GalDev->sampler(mappedSampler).setMinFilter(getGALTextureMinFilter((D3DTEXTUREFILTERTYPE) settedVertexTextures[s].minFilter,
                                                                                   (D3DTEXTUREFILTERTYPE) settedVertexTextures[s].mipFilter));

                if (settedVertexTextures[s].minFilter == D3DTEXF_ANISOTROPIC)
                    GalDev->sampler(mappedSampler).setMaxAnisotropy(settedVertexTextures[s].maxAniso);
                else
                    GalDev->sampler(mappedSampler).setMaxAnisotropy(1);

                GalDev->sampler(mappedSampler).setTextureAddressMode(libGAL::GAL_TEXTURE_S_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS) settedVertexTextures[s].addressU));
                GalDev->sampler(mappedSampler).setTextureAddressMode(libGAL::GAL_TEXTURE_T_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS) settedVertexTextures[s].addressV));
                GalDev->sampler(mappedSampler).setTextureAddressMode(libGAL::GAL_TEXTURE_R_COORD, getGALTextureAddrMode((D3DTEXTUREADDRESS) settedVertexTextures[s].addressW));
            }
            else
            {
                CG_ASSERT("GAL sampler slot for vertex texture was already taken by a pixel sampler.");
            }
        }
    }
}

void D3D9State::setGALIndices() 
{
    AIIndexBufferImp9* currentIndexBuffer;

    if (settedIndexBuffer) 
        currentIndexBuffer = settedIndexBuffer;
    else
        CG_ASSERT("No index buffer is setted, and a indexded draw call is performed");

    
    D3DINDEXBUFFER_DESC indexBufferDescriptor;

    settedIndexBuffer->GetDesc(&indexBufferDescriptor);

    D3D_DEBUG( D3D_DEBUG( cout << "D3D9State indices lenght: " << indexBufferDescriptor.Size << endl; ) )

    switch (indexBufferDescriptor.Format)
    {
        case D3DFMT_INDEX16:
            GalDev->setIndexBuffer(currentIndexBuffer->getAcdIndexBuffer(), 0, libGAL::GAL_SD_UINT16);
            break;

        case D3DFMT_INDEX32:
            GalDev->setIndexBuffer(currentIndexBuffer->getAcdIndexBuffer(), 0, libGAL::GAL_SD_UINT32);
            break;

        default:
            CG_ASSERT("Unsupported index buffer format.");
            break;
    }

}


libGAL::GAL_FORMAT D3D9State::getGALFormat(D3DFORMAT Format) {

    switch (Format)
    {
        case D3DFMT_X8R8G8B8:
            return libGAL::GAL_FORMAT_XRGB_8888;
            break;

        case D3DFMT_A8R8G8B8:
            return libGAL::GAL_FORMAT_ARGB_8888;
            break;

        case D3DFMT_Q8W8V8U8:
            return libGAL::GAL_FORMAT_QWVU_8888;
            break;

        case D3DFMT_DXT1:
            return libGAL::GAL_COMPRESSED_S3TC_DXT1_RGBA;
            break;

        case D3DFMT_DXT3:
            return libGAL::GAL_COMPRESSED_S3TC_DXT3_RGBA;
            break;

        case D3DFMT_DXT5:
            return libGAL::GAL_COMPRESSED_S3TC_DXT5_RGBA;
            break;

        case D3DFMT_A16B16G16R16:
            return libGAL::GAL_FORMAT_ABGR_161616;
            break;

        case D3DFMT_A16B16G16R16F:
            return libGAL::GAL_FORMAT_RGBA16F;
            break;

        case D3DFMT_R32F:
            return libGAL::GAL_FORMAT_R32F;
            break;

        case D3DFMT_A32B32G32R32F:
            return libGAL::GAL_FORMAT_RGBA32F;
            break;

        case D3DFMT_G16R16F:
            return libGAL::GAL_FORMAT_RG16F;
            break;

        case D3DFMT_A8:
            return libGAL::GAL_FORMAT_ALPHA_8;
            break;

        case D3DFMT_L8:
            return libGAL::GAL_FORMAT_LUMINANCE_8;
            break;

        case D3DFMT_A1R5G5B5:
            return libGAL::GAL_FORMAT_S8D24;
            break;

        case D3DFMT_A8L8:
            return libGAL::GAL_FORMAT_LUMINANCE8_ALPHA8;
            break;
            
        case D3DFMT_D24S8:
            return libGAL::GAL_FORMAT_S8D24;
            break;

        case D3DFMT_D16_LOCKABLE:
            return libGAL::GAL_FORMAT_S8D24;
            break;
            
        case D3DFMT_D24X8:
        case D3DFMT_D16:
            return libGAL::GAL_FORMAT_S8D24;
            break;

        case D3DFMT_A8B8G8R8:
            return libGAL::GAL_FORMAT_RGBA_8888; // TODO: OK???
            break;
            
        case D3DFMT_ATI2: // FOURCC: ATI2
            //  Hack for Crysis.
            return libGAL::GAL_COMPRESSED_S3TC_DXT5_RGBA;
            break;

        case D3DFMT_NULL:
            return libGAL::GAL_FORMAT_NULL;
            break;

        case D3DFMT_R5G6B5:
            ///return libGAL::GAL_FORMAT_R5G6B5;
            
            //  Hack for S.T.A.L.K.E.R. as it seems to be using a surface of this type
            //  but with all the write mask set to false, so seems a way to implement
            //  a null surface (depth only passes) with the minimum texel size (that
            //  is supported for a render target in D3D9).
            return libGAL::GAL_FORMAT_NULL;
            break;

        default:
            {
                char message[50];
                sprintf(message, "Unknown format: 0x%X", Format);
                CG_ASSERT(message);
                return static_cast<libGAL::GAL_FORMAT>(0);
            }
            break;
    }
}


libGAL::GAL_BLEND_OPTION D3D9State::getGALBlendOption(D3DBLEND mode)
{
    switch (mode)
    {
        case D3DBLEND_ZERO:
            return libGAL::GAL_BLEND_ZERO;
            break;

        case D3DBLEND_ONE:
            return libGAL::GAL_BLEND_ONE;
            break;

        case D3DBLEND_SRCCOLOR:
            return libGAL::GAL_BLEND_SRC_COLOR;
            break;

        case D3DBLEND_INVSRCCOLOR:
            return libGAL::GAL_BLEND_INV_SRC_COLOR;
            break;

        case D3DBLEND_SRCALPHA:
            return libGAL::GAL_BLEND_SRC_ALPHA;
            break;

        case D3DBLEND_INVSRCALPHA:
            return libGAL::GAL_BLEND_INV_SRC_ALPHA;
            break;

        case D3DBLEND_DESTALPHA:
            return libGAL::GAL_BLEND_DEST_ALPHA;
            break;

        case D3DBLEND_INVDESTALPHA:
            return libGAL::GAL_BLEND_INV_DEST_ALPHA;
            break;

        case D3DBLEND_DESTCOLOR:
            return libGAL::GAL_BLEND_DEST_COLOR;
            break;

        case D3DBLEND_INVDESTCOLOR:
            return libGAL::GAL_BLEND_INV_DEST_COLOR;
            break;

        case D3DBLEND_SRCALPHASAT:
            return libGAL::GAL_BLEND_SRC_ALPHA_SAT;
            break;

        /*case D3DBLEND_BOTHSRCALPHA:
            return libGAL::;
            break;

        case D3DBLEND_BOTHINVSRCALPHA:
            return libGAL::;
            break;*/

        case D3DBLEND_BLENDFACTOR:
            return libGAL::GAL_BLEND_BLEND_FACTOR;
            break;

        case D3DBLEND_INVBLENDFACTOR:
            return libGAL::GAL_BLEND_INV_BLEND_FACTOR;
            break;

        /*case D3DBLEND_SRCCOLOR2:
            return libGAL::;
            break;

        case D3DBLEND_INVSRCCOLOR2:
            return libGAL::;
            break;*/

        default:
            CG_ASSERT("Undefined blend mode.");
            return static_cast<libGAL::GAL_BLEND_OPTION>(0);
            break;
    }
}

libGAL::GAL_CUBEMAP_FACE D3D9State::getGALCubeMapFace(D3DCUBEMAP_FACES face)
{
    switch (face)
    {
        case D3DCUBEMAP_FACE_POSITIVE_X:
            return libGAL::GAL_CUBEMAP_POSITIVE_X;
            break;

        case D3DCUBEMAP_FACE_NEGATIVE_X:
            return libGAL::GAL_CUBEMAP_NEGATIVE_X;
            break;

        case D3DCUBEMAP_FACE_POSITIVE_Y:
            return libGAL::GAL_CUBEMAP_POSITIVE_y;
            break;

        case D3DCUBEMAP_FACE_NEGATIVE_Y:
            return libGAL::GAL_CUBEMAP_NEGATIVE_Y;
            break;

        case D3DCUBEMAP_FACE_POSITIVE_Z:
            return libGAL::GAL_CUBEMAP_POSITIVE_Z;
            break;

        case D3DCUBEMAP_FACE_NEGATIVE_Z:
            return libGAL::GAL_CUBEMAP_NEGATIVE_Z;
            break;

        default:
            CG_ASSERT("Undefined cubemap face identifier.");
            return static_cast<libGAL::GAL_CUBEMAP_FACE>(0);
            break;
    }
}

libGAL::GAL_BLEND_FUNCTION D3D9State::getGALBlendOperation(D3DBLENDOP operation)
{
    switch (operation)
    {

        case D3DBLENDOP_ADD:
            return libGAL::GAL_BLEND_ADD;
            break;

        case D3DBLENDOP_SUBTRACT:
            return libGAL::GAL_BLEND_SUBTRACT;
            break;

        case D3DBLENDOP_REVSUBTRACT:
            return libGAL::GAL_BLEND_REVERSE_SUBTRACT;
            break;

        case D3DBLENDOP_MIN:
            return libGAL::GAL_BLEND_MIN;
            break;

        case D3DBLENDOP_MAX:
            return libGAL::GAL_BLEND_MAX;
            break;

        default:
            CG_ASSERT("Undefined blend operation.");
            return static_cast<libGAL::GAL_BLEND_FUNCTION>(0);
            break;
    }
}

libGAL::GAL_TEXTURE_FILTER D3D9State::getGALTextureMinFilter(D3DTEXTUREFILTERTYPE Filter, D3DTEXTUREFILTERTYPE FilterMip)
{
    switch (Filter)
    {
        case D3DTEXF_NONE:
            switch (FilterMip)
            {
                case D3DTEXF_NONE:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_POINT:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_LINEAR:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
                    break;
                default:
                    CG_ASSERT("Undefined mip filter mode for filter mode NONE.");
                    return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
                    break;
            }
            break;

        case D3DTEXF_POINT:
            switch (FilterMip)
            {
                case D3DTEXF_NONE:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST;
                    break;
                case D3DTEXF_POINT:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_LINEAR:
                    return libGAL::GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
                    break;
                default:
                    CG_ASSERT("Undefined mip filter mode for filter mode POINT.");
                    return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
                    break;
            }
            break;

        case D3DTEXF_LINEAR:
            switch (FilterMip)
            {
                case D3DTEXF_NONE:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR;
                    break;
                case D3DTEXF_POINT:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_LINEAR:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
                    break;
                default:
                    CG_ASSERT("Undefined mip filter mode for filter mode LINEAR.");
                    return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
                    break;
            }
            break;

        case D3DTEXF_ANISOTROPIC:
            switch (FilterMip)
            {
                case D3DTEXF_NONE:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_POINT:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
                    break;
                case D3DTEXF_LINEAR:
                    return libGAL::GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
                    break;
                default:
                    CG_ASSERT("Undefined mip filter mode for filter mode NEAREST.");
                    return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
                    break;
            }
            break;

        default:
            CG_ASSERT("Undefined filter mode.");
            return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
            break;
    }
}

libGAL::GAL_TEXTURE_FILTER D3D9State::getGALTextureMagFilter(D3DTEXTUREFILTERTYPE Filter)
{
    switch (Filter)
    {
        case D3DTEXF_NONE:
            return libGAL::GAL_TEXTURE_FILTER_NEAREST;
            break;

        case D3DTEXF_POINT:
            return libGAL::GAL_TEXTURE_FILTER_NEAREST;
            break;

        case D3DTEXF_LINEAR:
            return libGAL::GAL_TEXTURE_FILTER_LINEAR;
            break;

        case D3DTEXF_ANISOTROPIC:
            return libGAL::GAL_TEXTURE_FILTER_LINEAR;
            break;

        default:
            CG_ASSERT("Undefined filter mode.");
            return static_cast<libGAL::GAL_TEXTURE_FILTER>(0);
            break;
    }
}

libGAL::gal_uint D3D9State::getElementLength(D3DDECLTYPE Type)
{
    switch (Type)
    {
        case D3DDECLTYPE_FLOAT1:
            return 4;
            break;

        case D3DDECLTYPE_FLOAT2:
            return 8;
            break;

        case D3DDECLTYPE_FLOAT3:
            return 12;
            break;

        case D3DDECLTYPE_FLOAT4:
            return 16;
            break;

        case D3DDECLTYPE_D3DCOLOR:
            return 4;
            break;

        case D3DDECLTYPE_SHORT2:
            return 4;
            break;

        case D3DDECLTYPE_SHORT4N:
            return 4;
            break;

        case D3DDECLTYPE_UBYTE4N:
            return 4;
            break;

        default:
            CG_ASSERT("Unsupported vertex declaration type.");
            return 0;
            break;
    }
}

void D3D9State::getTypeAndComponents(D3DDECLTYPE Type, libGAL::gal_uint* components, libGAL::GAL_STREAM_DATA* GalType)
{
    switch (Type)
    {
        case D3DDECLTYPE_FLOAT1:
            *components = 1;
            *GalType = libGAL::GAL_SD_FLOAT32;
            break;

        case D3DDECLTYPE_FLOAT2:
            *components = 2;
            *GalType = libGAL::GAL_SD_FLOAT32;
            break;

        case D3DDECLTYPE_FLOAT3:
            *components = 3;
            *GalType = libGAL::GAL_SD_FLOAT32;
            break;

        case D3DDECLTYPE_FLOAT4:
            *components = 4;
            *GalType = libGAL::GAL_SD_FLOAT32;
            break;

        case D3DDECLTYPE_D3DCOLOR:
            *components = 4;
            *GalType = libGAL::GAL_SD_INV_UNORM8;
            break;

        case D3DDECLTYPE_SHORT2:
            *components = 2;
            *GalType = libGAL::GAL_SD_SINT16;
            break;

        case D3DDECLTYPE_SHORT2N:
            *components = 2;
            *GalType = libGAL::GAL_SD_SNORM16;
            break;

        case D3DDECLTYPE_SHORT4:
            *components = 4;
            *GalType = libGAL::GAL_SD_SINT16;
            break;
            
        case D3DDECLTYPE_SHORT4N:
            *components = 4;
            *GalType = libGAL::GAL_SD_SNORM16;
            break;

        case D3DDECLTYPE_UBYTE4N:
            *components = 4;
            *GalType = libGAL::GAL_SD_UNORM8;
            break;

        case D3DDECLTYPE_UBYTE4:
            *components = 4;
            *GalType = libGAL::GAL_SD_UINT8;
            break;

        case D3DDECLTYPE_FLOAT16_2:
            *components = 2;
            *GalType = libGAL::GAL_SD_FLOAT16;
            break;

        case D3DDECLTYPE_FLOAT16_4:
            *components = 4;
            *GalType = libGAL::GAL_SD_FLOAT16;
            break;

        default:
            cout << Type << endl;
            CG_ASSERT("Undefined vertex declaration type.");
            break;
    }
}

libGAL::GAL_PRIMITIVE D3D9State::getGALPrimitiveType(D3DPRIMITIVETYPE primitive)
{
    switch (primitive)
    {
        case D3DPT_POINTLIST:
            return libGAL::GAL_POINTS;
            break;

        case D3DPT_LINELIST:
            return libGAL::GAL_LINES;
            break;

        case D3DPT_LINESTRIP:
            return libGAL::GAL_LINE_STRIP;
            break;

        case D3DPT_TRIANGLELIST:
            return libGAL::GAL_TRIANGLES;
            break;

        case D3DPT_TRIANGLESTRIP:
            return libGAL::GAL_TRIANGLE_STRIP;
            break;

        case D3DPT_TRIANGLEFAN:
            return libGAL::GAL_TRIANGLE_FAN;
            break;

        default:
            CG_ASSERT("Unsupported primitive.");
            return static_cast<libGAL::GAL_PRIMITIVE>(0);
            break;
    }
}

libGAL::GAL_TEXTURE_ADDR_MODE D3D9State::getGALTextureAddrMode(D3DTEXTUREADDRESS address)
{
    switch (address)
    {
        case D3DTADDRESS_CLAMP:
            //return libGAL::GAL_TEXTURE_ADDR_CLAMP;
            return libGAL::GAL_TEXTURE_ADDR_CLAMP_TO_EDGE;
            break;

        case D3DTADDRESS_WRAP:
            return libGAL::GAL_TEXTURE_ADDR_REPEAT;
            break;

        case D3DTADDRESS_BORDER:
            return libGAL::GAL_TEXTURE_ADDR_CLAMP_TO_BORDER;
            break;

        case D3DTADDRESS_MIRROR:
            return libGAL::GAL_TEXTURE_ADDR_MIRRORED_REPEAT;
            break;

        default:
            CG_ASSERT("Undefined texture address mode.");
            return static_cast<libGAL::GAL_TEXTURE_ADDR_MODE>(0);
            break;
    }
}

libGAL::GAL_STENCIL_OP D3D9State::getGALStencilOperation(D3DSTENCILOP operation)
{
    switch (operation)
    {
        case D3DSTENCILOP_KEEP:
            return libGAL::GAL_STENCIL_OP_KEEP;
            break;

        case D3DSTENCILOP_ZERO:
            return libGAL::GAL_STENCIL_OP_ZERO;
            break;

        case D3DSTENCILOP_REPLACE:
            return libGAL::GAL_STENCIL_OP_REPLACE;
            break;

        case D3DSTENCILOP_INCRSAT:
            return libGAL::GAL_STENCIL_OP_INCR_SAT;
            break;

        case D3DSTENCILOP_DECRSAT:
            return libGAL::GAL_STENCIL_OP_DECR_SAT;
            break;

        case D3DSTENCILOP_INVERT:
            return libGAL::GAL_STENCIL_OP_INVERT;
            break;

        case D3DSTENCILOP_INCR:
            return libGAL::GAL_STENCIL_OP_INCR;
            break;

        case D3DSTENCILOP_DECR:
            return libGAL::GAL_STENCIL_OP_DECR;
            break;

        default:
            CG_ASSERT("Undefined stencil operation.");
            return static_cast<libGAL::GAL_STENCIL_OP>(0);
            break;

    }
}

libGAL::GAL_COMPARE_FUNCTION D3D9State::getGALCompareFunction(D3DCMPFUNC func)
{
    switch (func)
    {
        case D3DCMP_NEVER:
            return libGAL::GAL_COMPARE_FUNCTION_NEVER;
            break;

        case D3DCMP_LESS:
            return libGAL::GAL_COMPARE_FUNCTION_LESS;
            break;

        case D3DCMP_EQUAL:
            return libGAL::GAL_COMPARE_FUNCTION_EQUAL;
            break;

        case D3DCMP_LESSEQUAL:
            return libGAL::GAL_COMPARE_FUNCTION_LESS_EQUAL;
            break;

        case D3DCMP_GREATER:
            return libGAL::GAL_COMPARE_FUNCTION_GREATER;
            break;

        case D3DCMP_NOTEQUAL:
            return libGAL::GAL_COMPARE_FUNCTION_NOT_EQUAL;
            break;

        case D3DCMP_GREATEREQUAL:
            return libGAL::GAL_COMPARE_FUNCTION_GREATER_EQUAL;
            break;

        case D3DCMP_ALWAYS:
            return libGAL::GAL_COMPARE_FUNCTION_ALWAYS;
            break;

        default:
            CG_ASSERT("Undefined compare function.");
            return static_cast<libGAL::GAL_COMPARE_FUNCTION>(0);
            break;
    }
}

void D3D9State::redefineGALBlend()
{

    for (U32 i = 0; i < MAX_RENDER_TARGETS; i++) {
        // Enable/Disable Alpha Blending state
        GalDev->blending().setEnable(i, alphaBlend);

        if (alphaBlend) {

            GalDev->blending().setSrcBlend(i, srcBlend);
            GalDev->blending().setDestBlend(i, dstBlend);
            GalDev->blending().setBlendFunc(i, blendOp);

            if (separateAlphaBlend) {

                GalDev->blending().setSrcBlendAlpha(i, srcBlendAlpha);
                GalDev->blending().setDestBlendAlpha(i, dstBlendAlpha);
                //GalDev->blending().setBlendFuncAlpha(0, blendOpAlpha);

            }
            else {

                GalDev->blending().setSrcBlendAlpha(i, srcBlend);
                GalDev->blending().setDestBlendAlpha(i, dstBlend);
                //GalDev->blending().setBlendFuncAlpha(0, blendOp);

            }

        }
    }

}

void D3D9State::redefineGALStencil() {

    // Enable/Disable Stencil
    GalDev->zStencil().setStencilEnabled(stencilEnabled || twoSidedStencilEnabled);

    if (stencilEnabled) {
        GalDev->zStencil().setStencilOp(libGAL::GAL_FACE_FRONT, stencilFail, stencilZFail, stencilPass);
        GalDev->zStencil().setStencilFunc(libGAL::GAL_FACE_FRONT, stencilFunc, stencilRef, stencilMask);
    }
    else {
        GalDev->zStencil().setStencilOp(libGAL::GAL_FACE_FRONT, libGAL::GAL_STENCIL_OP_KEEP, libGAL::GAL_STENCIL_OP_KEEP, libGAL::GAL_STENCIL_OP_KEEP);
        GalDev->zStencil().setStencilFunc(libGAL::GAL_FACE_FRONT, libGAL::GAL_COMPARE_FUNCTION_ALWAYS, 0, 0xFFFFFFFF);
    }

    if (twoSidedStencilEnabled) {
        GalDev->zStencil().setStencilOp(libGAL::GAL_FACE_BACK, ccwStencilFail, ccwStencilZFail, ccwStencilPass);
        GalDev->zStencil().setStencilFunc(libGAL::GAL_FACE_BACK, ccwStencilFunc, stencilRef, stencilMask);
    } else {
        GalDev->zStencil().setStencilOp(libGAL::GAL_FACE_BACK, libGAL::GAL_STENCIL_OP_KEEP, libGAL::GAL_STENCIL_OP_KEEP, libGAL::GAL_STENCIL_OP_KEEP);
        GalDev->zStencil().setStencilFunc(libGAL::GAL_FACE_BACK, libGAL::GAL_COMPARE_FUNCTION_ALWAYS, 0, 0xFFFFFFFF);
    }

    GalDev->zStencil().setStencilUpdateMask(stencilWriteMask);

}

void D3D9State::createFVFVertexDeclaration() {

    fvfVertexDeclaration.clear();

    DWORD offset = 0;

    if (settedFVF & D3DFVF_XYZ) {

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State FVF position." << endl; ) )

        D3DVERTEXELEMENT9 tmpElem;

        tmpElem.Stream = 0;
        tmpElem.Offset = offset;
        tmpElem.Type = D3DDECLTYPE_FLOAT3;
        tmpElem.Method = D3DDECLMETHOD_DEFAULT;
        tmpElem.Usage = D3DDECLUSAGE_POSITION;
        tmpElem.UsageIndex = 0;

        fvfVertexDeclaration.push_back(tmpElem);
        offset += 12;

    }

    if (settedFVF & D3DFVF_NORMAL) {

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State FVF normal." << endl; ) )

        D3DVERTEXELEMENT9 tmpElem;

        tmpElem.Stream = 0;
        tmpElem.Offset = offset;
        tmpElem.Type = D3DDECLTYPE_FLOAT3;
        tmpElem.Method = D3DDECLMETHOD_DEFAULT;
        tmpElem.Usage = D3DDECLUSAGE_NORMAL;
        tmpElem.UsageIndex = 0;

        fvfVertexDeclaration.push_back(tmpElem);
        offset += 12;

    }

    if (settedFVF & D3DFVF_DIFFUSE) {

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State FVF diffuse." << endl; ) )

        D3DVERTEXELEMENT9 tmpElem;

        tmpElem.Stream = 0;
        tmpElem.Offset = offset;
        tmpElem.Type = D3DDECLTYPE_D3DCOLOR;
        tmpElem.Method = D3DDECLMETHOD_DEFAULT;
        tmpElem.Usage = D3DDECLUSAGE_COLOR;
        tmpElem.UsageIndex = 0;

        fvfVertexDeclaration.push_back(tmpElem);
        offset += 4;
    }

    if (settedFVF & D3DFVF_TEX1) {

        D3D_DEBUG( D3D_DEBUG( cout << "D3D9State FVF TEX1." << endl; ) )

        D3DVERTEXELEMENT9 tmpElem;

        tmpElem.Stream = 0;
        tmpElem.Offset = offset;
        tmpElem.Type = D3DDECLTYPE_FLOAT2;
        tmpElem.Method = D3DDECLMETHOD_DEFAULT;
        tmpElem.Usage = D3DDECLUSAGE_TEXCOORD;
        tmpElem.UsageIndex = 0;

        fvfVertexDeclaration.push_back(tmpElem);
        offset += 8;
    }

}

void D3D9State::setRenderTarget(DWORD RenderTargetIndex, AISurfaceImp9 *surface)
{
    //  Check for NULL (disable render target).
    if (surface == NULL)
    {
        GalDev->setRenderTarget(RenderTargetIndex, NULL);
        currentRenderSurface[RenderTargetIndex] = surface;
        return;
    }

    D3DSURFACE_DESC sDesc;

    surface->GetDesc(&sDesc);

    // Reset the viewport to the full render target (D3D SDK dixit).  Only for render target 0!!
    if (RenderTargetIndex == 0)
        GalDev->rast().setViewport(0, 0, sDesc.Width, sDesc.Height);
    
    if (sDesc.Format == D3DFMT_NULL)
    {
        GalDev->setRenderTarget(RenderTargetIndex, NULL);
        currentRenderSurface[RenderTargetIndex] = surface;
        return;
    }

    GalDev->setRenderTarget(RenderTargetIndex, surface->getGALRenderTarget());

    currentRenderSurface[RenderTargetIndex] = surface;
    
    /*map<AISurfaceImp9*, libGAL::GALRenderTarget *>::iterator it;

    it = renderTargets.find(surface);

    if (it != renderTargets.end())
    {
        //  Set as the new render target.
        GalDev->setRenderTarget(it->second);

        //  Set as the new render surface.
        currentRenderSurface = surface;
    }
    else
    {
        //  Create a new render target from the provided texture mipmap.
        map<AISurfaceImp9*, TextureType>::iterator it;
        it = mipLevelsTextureType.find(surface);

        //  Check if surface was defined.
        if (it != mipLevelsTextureType.end())
        {
            //  Check if texture type is supported.
            if (it->second != D3D9_TEXTURE2D)
                CG_ASSERT("Render surfaces are only implemented for 2D textures.");

            //  Get the GAL Texture2D associated with the surface.
            AITextureImp9 *tex2D = mipLevelsTextures2D[surface];
            libGAL::GALTexture2D *galTex2D = textures2D[tex2D];

            //  Get the miplevel for the defined surface.
            libGAL::gal_uint mipLevel = mipLevelsLevels[surface];

            //  Create the new render target.
            libGAL::GAL_RT_DESC rtDesc;
            rtDesc.format = galTex2D->getFormat(mipLevel);
            rtDesc.dimension = libGAL::GAL_RT_DIMENSION_TEXTURE2D;
            rtDesc.texture2D.mipmap = mipLevel;
            libGAL::GALRenderTarget *newRT = GalDev->createRenderTarget(galTex2D, libGAL::GAL_RT_DIMENSION_TEXTURE2D, static_cast<libGAL::GAL_CUBEMAP_FACE>(0), mipLevel);

            //  Add the new render target.
            renderTargets[surface] = newRT;

            //  Set as new render target.
            GalDev->setRenderTarget(newRT);

            //  Set as new render surface.
            currentRenderSurface = surface;
        }
        else
        {
            CG_ASSERT("Setting an undefined surface as render target.");
        }
    }*/
}


void D3D9State::addRenderTarget(AISurfaceImp9* rt, UINT width , UINT height , D3DFORMAT format) {

    D3DSURFACE_DESC sDesc;

    rt->GetDesc(&sDesc);

    if (sDesc.Format == D3DFMT_NULL)
        return;

    libGAL::GALTexture2D *tex2D = GalDev->createTexture2D();

    // Get mip level size
    UINT size = getSurfaceSize(width, height, format);

    //BYTE* data = new BYTE[size];

    //tex2D->setData(0, width, height, getGALFormat(format), 0, data, 0);
    tex2D->setData(0, width, height, getGALFormat(format), 0, NULL, 0);

    //renderTargetSurface[rt] = tex2D;
    //mipLevelsTextureType[rt] = D3D9_RENDERTARGET;

    //  Create the new render target.
    /*libGAL::GAL_RT_DESC rtDesc;
    rtDesc.format = getGALFormat(format);
    rtDesc.dimension = libGAL::GAL_RT_DIMENSION_TEXTURE2D;
    rtDesc.texture2D.mipmap = 0;*/
    libGAL::GALRenderTarget *newRT = GalDev->createRenderTarget(tex2D, libGAL::GAL_RT_DIMENSION_TEXTURE2D, static_cast<libGAL::GAL_CUBEMAP_FACE>(0), 0);

    //  Add the new render target.
    //renderTargets[rt] = newRT;

    //delete[] data;

    rt->setAsRenderTarget(newRT);

}

AISurfaceImp9 *D3D9State::getRenderTarget(DWORD RenderTargetIndex)
{
    return currentRenderSurface[RenderTargetIndex];
}


void D3D9State::setZStencilBuffer(AISurfaceImp9 *surface)
{
    currentZStencilSurface = surface;

    //  Check for NULL TODO: Disable depth stencil operation
    if (surface == NULL)
        GalDev->setZStencilBuffer(NULL);
    else
        GalDev->setZStencilBuffer(surface->getGALRenderTarget());

    /*map<AISurfaceImp9*, libGAL::GALRenderTarget *>::iterator it;

    it = renderTargets.find(surface);

    if (it != renderTargets.end())
    {
        //  Set as the new z stencil buffer.
        GalDev->setZStencilBuffer(it->second);

        //  Set as the new z stencil surface.
        currentZStencilSurface = surface;
    }
    else
    {
        //  Create a new render target from the provided texture mipmap.
        map<AISurfaceImp9*, TextureType>::iterator it;
        it = mipLevelsTextureType.find(surface);

        //  Check if surface was defined.
        if (it != mipLevelsTextureType.end())
        {
            //  Check if texture type is supported.
            if (it->second != D3D9_TEXTURE2D)
                CG_ASSERT("Render surfaces are only implemented for 2D textures.");

            //  Get the GAL Texture2D associated with the surface.
            AITextureImp9 *tex2D = mipLevelsTextures2D[surface];
            libGAL::GALTexture2D *galTex2D = textures2D[tex2D];

            //  Get the miplevel for the defined surface.
            libGAL::gal_uint mipLevel = mipLevelsLevels[surface];

            //  Create the new render target.
            libGAL::GAL_RT_DESC rtDesc;
            rtDesc.format = galTex2D->getFormat(mipLevel);
            rtDesc.dimension = libGAL::GAL_RT_DIMENSION_TEXTURE2D;
            rtDesc.texture2D.mipmap = mipLevel;
            libGAL::GALRenderTarget *newRT = GalDev->createRenderTarget(galTex2D, libGAL::GAL_RT_DIMENSION_TEXTURE2D, static_cast<libGAL::GAL_CUBEMAP_FACE>(0), mipLevel);

            //  Add the new render target.
            renderTargets[surface] = newRT;

            //  Set as new render target.
            GalDev->setZStencilBuffer(newRT);

            //  Set as new render surface.
            currentZStencilSurface = surface;
        }
        else
        {
            CG_ASSERT("Setting an undefined surface as z stencil buffer.");
        }
    }*/
}

AISurfaceImp9 *D3D9State::getZStencilBuffer()
{
    return currentZStencilSurface;
}


