/**************************************************************************
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include "GALMath.h"
#include "GALDeviceImp.h"
#include "GALSamplerImp.h"
#include "GALMacros.h"
#include "GALTexture2DImp.h"
#include "GALTextureCubeMapImp.h"
#include "TextureAdapter.h"
#include "GlobalProfiler.h"

using namespace libGAL;
using namespace std;

GALSamplerImp::GALSamplerImp(GALDeviceImp* device, HAL* driver, gal_uint samplerID) :
    _device(device), _driver(driver), _SAMPLER_ID(samplerID), _requiredSync(true),
    _enabled(false),
    _texture(0),
    _textureType(GAL_RESOURCE_UNKNOWN),
    _textureWidth(0),
    _textureHeight(0),
    _textureDepth(0),
    _textureWidth2(0),
    _textureHeight2(0),
    _textureDepth2(0),
    _sCoord(GAL_TEXTURE_ADDR_CLAMP),
    _tCoord(GAL_TEXTURE_ADDR_CLAMP),
    _rCoord(GAL_TEXTURE_ADDR_CLAMP),
    _nonNormalizedCoords(false),
    _minFilter(GAL_TEXTURE_FILTER_NEAREST),
    _magFilter(GAL_TEXTURE_FILTER_NEAREST),
    _enableComparison(false),
    _comparisonFunction(GAL_TEXTURE_COMPARISON_LEQUAL),
    _sRGBConversion(false),
    _minLOD(0.0f),
    _maxLOD(12.0f),
    _baseLevel(0),
    _maxLevel(12),
    _maxAniso(1),
    _lodBias(0.0f),
    _unitLodBias(0.0f),
    _minLevel(0),
    _blitTextureBlocking(GAL_LAYOUT_TEXTURE)
{
    forceSync(); // init GPU sampler(i) state registers
}

void GALSamplerImp::setEnabled(gal_bool enable) { _enabled = enable; }

gal_bool GALSamplerImp::isEnabled() const { return _enabled; }

void GALSamplerImp::setTexture(GALTexture* texture)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( texture->getType() == GAL_RESOURCE_TEXTURE1D )
            CG_ASSERT("1D textures are not supported yet");
        /*if ( texture->getType() == GAL_RESOURCE_TEXTURE3D )
            CG_ASSERT("3D textures are not supported yet");*/
    )
    _texture = texture;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setTextureAddressMode(GAL_TEXTURE_COORD coord, GAL_TEXTURE_ADDR_MODE mode)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    switch ( coord )
    {
        case GAL_TEXTURE_S_COORD:
            _sCoord = mode; break;
        case GAL_TEXTURE_T_COORD:
            _tCoord = mode; break;
        case GAL_TEXTURE_R_COORD:
            _rCoord = mode; break;
        default:
            CG_ASSERT("Unknown texture address coordinate");
    }
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setNonNormalizedCoordinates(gal_bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _nonNormalizedCoords = enable;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMinFilter(GAL_TEXTURE_FILTER minFilter)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _minFilter = minFilter;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMagFilter(GAL_TEXTURE_FILTER magFilter)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _magFilter = magFilter;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMinLOD(gal_float minLOD)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _minLOD = minLOD;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setEnableComparison(gal_bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _enableComparison = enable;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setComparisonFunction(GAL_TEXTURE_COMPARISON function)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _comparisonFunction = function;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setSRGBConversion(gal_bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _sRGBConversion = enable;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMaxLOD(gal_float maxLOD)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _maxLOD = maxLOD;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMaxAnisotropy(gal_uint maxAnisotropy)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _maxAniso = maxAnisotropy;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setLODBias(gal_float lodBias)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _lodBias = lodBias;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setUnitLODBias(gal_float unitLodBias)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _unitLodBias = unitLodBias;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::setMinLevel(gal_uint minLevel)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _minLevel = minLevel;
    GLOBAL_PROFILER_EXIT_REGION()
}

GAL_TEXTURE_FILTER GALSamplerImp::getMagFilter() const
{
    return _magFilter;
}

GALTexture* GALSamplerImp::getTexture() const
{
    return _texture;
}

GAL_TEXTURE_FILTER GALSamplerImp::getMinFilter() const
{
    return _minFilter;
}

string GALSamplerImp::getInternalState() const
{
    stringstream ss, ssAux;

    ssAux << "SAMPLER" << _SAMPLER_ID;

    ss << ssAux.str() << stateItemString(_enabled,"_ENABLED",false,&boolPrint);

    ss << ssAux.str() << stateItemString(_sCoord,"_S_COORD_ADDR_MODE",false,&addressModePrint);
    ss << ssAux.str() << stateItemString(_tCoord,"_T_COORD_ADDR_MODE",false,&addressModePrint);
    ss << ssAux.str() << stateItemString(_rCoord,"_R_COORD_ADDR_MODE",false,&addressModePrint);

    ss << ssAux.str() << stateItemString(_nonNormalizedCoords, "_NON_NORMALIZED", false, &boolPrint);

    ss << ssAux.str() << stateItemString(_minFilter,"_MIN_FILTER",false,&filterPrint);
    ss << ssAux.str() << stateItemString(_magFilter,"_MAG_FILTER",false,&filterPrint);

    ss << ssAux.str() << stateItemString(_minLOD,"_MIN_LOD",false);
    ss << ssAux.str() << stateItemString(_maxLOD,"_MAX_LOD",false);

    ss << ssAux.str() << stateItemString(_maxAniso,"_MAX_ANISO",false);
    ss << ssAux.str() << stateItemString(_lodBias,"_LOD_BIAS",false);

    return ss.str().c_str();
}

const char* GALSamplerImp::_getFormatString(GAL_FORMAT textureFormat)
{
    switch ( textureFormat )
    {
        //Noncompressed Formats
        case GAL_FORMAT_RGBA_8888:
            return "RGBA_8888";
        case GAL_FORMAT_INTENSITY_8:
            return "INTENSITY_8";
        case GAL_FORMAT_INTENSITY_12:
            return "INTENSITY_12";
        case GAL_FORMAT_INTENSITY_16:
            return "INTENSITY_16";
        case GAL_FORMAT_ALPHA_8:
            return "ALPHA_8";
        case GAL_FORMAT_ALPHA_12:
            return "ALPHA_12";
        case GAL_FORMAT_ALPHA_16:
            return "ALPHA_16";
        case GAL_FORMAT_LUMINANCE_8:
            return "LUMINANCE_8";
        case GAL_FORMAT_LUMINANCE_12:
            return "LUMINANCE_12";
        case GAL_FORMAT_LUMINANCE_16:
            return "LUMINANCE_16";
        case GAL_FORMAT_LUMINANCE8_ALPHA8:
            return "LUMINANCE8_ALPHA8";            
        case GAL_FORMAT_DEPTH_COMPONENT_16:
            return "DEPTH_COMPONENT_16";
        case GAL_FORMAT_DEPTH_COMPONENT_24:
            return "DEPTH_COMPONENT_24";
        case GAL_FORMAT_DEPTH_COMPONENT_32:
            return "DEPTH_COMPONENT_32";
        case GAL_FORMAT_RGB_888:
            return "RGB_888";
        case GAL_FORMAT_ARGB_8888:
            return "ARGB_8888";
        case GAL_FORMAT_ABGR_161616:
            return "ARGR_161616";
        case GAL_FORMAT_XRGB_8888:
            return "XRGB_8888";
        case GAL_FORMAT_QWVU_8888:
            return "QWVU_8888";
        case GAL_FORMAT_ARGB_1555:
            return "ARGB_1555";
        case GAL_FORMAT_UNSIGNED_INT_8888:
            return "UNSIGNED_INT_8888";
        case GAL_FORMAT_RG16F:
            return "RG16F";
        case GAL_FORMAT_R32F:
            return "R32F";
        case GAL_FORMAT_RGBA32F:
            return "RGBA32F";
        case GAL_FORMAT_S8D24:
            return "S8D24";
        case GAL_FORMAT_RGBA16F:
            return "RGBA16F";
        case GAL_FORMAT_RGB_565:
            return "RGB_565";
        /// Add more here...

        // Compressed formats
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
            return "COMPRESSED_S3TC_DXT1_RGB";
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
            return "COMPRESSED_S3TC_DXT1_RGBA";
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
            return "COMPRESSED_S3TC_DXT3_RGBA";
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
            return "COMPRESSED_S3TC_DXT5_RGBA";
        case GAL_COMPRESSED_LUMINANCE_LATC1_EXT:
            return "COMPRESSED_LUMINANCE_LATC1_EXT";
        case GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            return "COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT";
        case GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
            return "COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT";
        case GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            return "COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT";

        default:
            return "UNKNOWN";
    }
}

void GALSamplerImp::_getGPUTextureFormat(GAL_FORMAT textureFormat,
                                         cg1gpu::GPURegData* dataFormat,
                                         cg1gpu::GPURegData* dataCompression,
                                         cg1gpu::GPURegData* dataInverted)
{
    using namespace cg1gpu;
    switch ( textureFormat )
    {

        // Non-compressed formats
        case GAL_FORMAT_RGB_565:
            dataFormat->txFormat = GPU_RGB565;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = true;
            break;
        case GAL_FORMAT_RGBA_8888:
        case GAL_FORMAT_UNSIGNED_INT_8888:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_RGB_888:
            dataFormat->txFormat = GPU_RGB888;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_XRGB_8888:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = true;
            break;
        case GAL_FORMAT_ARGB_8888:
        case GAL_FORMAT_QWVU_8888:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = true;
            break;
        case GAL_FORMAT_RG16F:
            dataFormat->txFormat = GPU_RG16F;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_R32F:
            dataFormat->txFormat = GPU_R32F;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;                    
        case GAL_FORMAT_RGBA32F:
            dataFormat->txFormat = GPU_RGBA32F;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_S8D24:
            dataFormat->txFormat = GPU_DEPTH_COMPONENT24;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;                    
        case GAL_FORMAT_ABGR_161616:
            dataFormat->txFormat = GPU_RGBA16;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_RGBA16F:
            dataFormat->txFormat = GPU_RGBA16F;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;

        case GAL_FORMAT_ARGB_1555:
            dataFormat->txFormat = GPU_RGBA5551;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = true;
            break;

        case GAL_FORMAT_INTENSITY_8:
            dataFormat->txFormat = GPU_INTENSITY8;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_INTENSITY_12:
            dataFormat->txFormat = GPU_INTENSITY12;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_INTENSITY_16:
            dataFormat->txFormat = GPU_INTENSITY16;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;


        case GAL_FORMAT_ALPHA_8:
            dataFormat->txFormat = GPU_ALPHA8;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_ALPHA_12:
            dataFormat->txFormat = GPU_ALPHA12;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_ALPHA_16:
            dataFormat->txFormat = GPU_ALPHA16;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;


        case GAL_FORMAT_LUMINANCE_8:
            dataFormat->txFormat = GPU_LUMINANCE8;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_LUMINANCE_12:
            dataFormat->txFormat = GPU_LUMINANCE12;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_LUMINANCE_16:
            dataFormat->txFormat = GPU_LUMINANCE16;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;

        case GAL_FORMAT_LUMINANCE8_ALPHA8:
            dataFormat->txFormat = GPU_LUMINANCE8_ALPHA8;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;

        case GAL_FORMAT_DEPTH_COMPONENT_16:
            dataFormat->txFormat = GPU_DEPTH_COMPONENT16;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_DEPTH_COMPONENT_24:
            dataFormat->txFormat = GPU_DEPTH_COMPONENT24;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;
        case GAL_FORMAT_DEPTH_COMPONENT_32:
            dataFormat->txFormat = GPU_DEPTH_COMPONENT32;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            break;

        // Compressed Formats
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
            dataFormat->txFormat = GPU_RGB888;
            dataCompression->txCompression = GPU_S3TC_DXT1_RGB;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_S3TC_DXT1_RGBA;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_S3TC_DXT3_RGBA;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_S3TC_DXT5_RGBA;
            dataInverted->booleanVal = false;
            break;


        case GAL_COMPRESSED_LUMINANCE_LATC1_EXT:
            dataFormat->txFormat = GPU_LUMINANCE8;
            dataCompression->txCompression = GPU_LATC1;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            dataFormat->txFormat = GPU_LUMINANCE8_SIGNED;
            dataCompression->txCompression = GPU_LATC1_SIGNED;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
            dataFormat->txFormat = GPU_LUMINANCE8_ALPHA8;
            dataCompression->txCompression = GPU_LATC2;
            dataInverted->booleanVal = false;
            break;
        case GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            dataFormat->txFormat = GPU_LUMINANCE8_ALPHA8_SIGNED;
            dataCompression->txCompression = GPU_LATC2_SIGNED;
            dataInverted->booleanVal = false;
            break;

        default:
        {
            dataFormat->txFormat = GPU_RGBA8888;
            dataCompression->txCompression = GPU_NO_TEXTURE_COMPRESSION;
            dataInverted->booleanVal = false;
            stringstream ss;
            ss << "Texture format \"" << _getFormatString(textureFormat) << "\" not supported";
            CG_ASSERT(ss.str().c_str());
        }
    }
}

void GALSamplerImp::_getGPUTextureMode(GAL_RESOURCE_TYPE textureType, cg1gpu::GPURegData* data)
{
    using namespace cg1gpu;
    switch ( textureType )
    {
        case GAL_RESOURCE_TEXTURE1D:
            data->txMode = GPU_TEXTURE1D;
            break;
        case GAL_RESOURCE_TEXTURE2D:
            data->txMode = GPU_TEXTURE2D;
            break;
        case GAL_RESOURCE_TEXTURE3D:
            data->txMode = GPU_TEXTURE3D;
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            data->txMode = GPU_TEXTURECUBEMAP;
            break;
        case GAL_RESOURCE_BUFFER:
            CG_ASSERT("GAL_RESOURCE_BUFFER is not a valid texture type");
            break;
        case GAL_RESOURCE_UNKNOWN:
            CG_ASSERT("GAL_RESOURCE_UNKNOWN is not a valid texture type");
            break;
        default:
            CG_ASSERT("Unknown texture type token");
    }
}

void GALSamplerImp::_getGPUTexMemoryLayout(GAL_MEMORY_LAYOUT memLayout, cg1gpu::GPURegData* data)
{
    //  Set texture blocking/memory layout mode.
    switch(memLayout)
    {
        case GAL_LAYOUT_TEXTURE:
            data->txBlocking = cg1gpu::GPU_TXBLOCK_TEXTURE;
            break;
        case GAL_LAYOUT_RENDERBUFFER:
            data->txBlocking = cg1gpu::GPU_TXBLOCK_FRAMEBUFFER;
            break;
        default:
            CG_ASSERT("Undefined memory layout mode.");
            break;
    }
}

void GALSamplerImp::_syncTexture()
{
    using namespace cg1gpu;

    GPURegData data;

    // Create a texture adapter object
    TextureAdapter texAdapter(_texture);

    bool renderBufferTexture = (_texture->getMemoryLayout() == GAL_LAYOUT_RENDERBUFFER);

    // Compute efective base mipmap level
    gal_uint baseLevel = _minLevel;
    _baseLevel = max(_texture->getBaseLevel(), baseLevel);
    if ( _baseLevel.changed() || _requiredSync )
    {
        data.uintVal = _baseLevel;
        _driver->writeGPURegister(GPU_TEXTURE_MIN_LEVEL, _SAMPLER_ID, data);
        _baseLevel.restart();
    }

    // Compute max mipmal level
    _maxLevel = _texture->getMaxLevel();   
    if ( _maxLevel.changed() || _requiredSync ) 
    {
        data.uintVal = _maxLevel;
        _driver->writeGPURegister(GPU_TEXTURE_MAX_LEVEL, _SAMPLER_ID, data);
        _maxLevel.restart();
    }

    // Compute texture width
    _textureWidth = texAdapter.getWidth(static_cast<GAL_CUBEMAP_FACE>(0), 0);
    if ( _textureWidth.changed() || _requiredSync ) 
    {
        data.uintVal = _textureWidth;
        _driver->writeGPURegister(GPU_TEXTURE_WIDTH, _SAMPLER_ID, data);
        _textureWidth.restart();
    }


    // Compute texture height
    _textureHeight = texAdapter.getHeight(static_cast<GAL_CUBEMAP_FACE>(0), 0);
    if ( _textureHeight.changed() || _requiredSync ) 
    {
        data.uintVal = _textureHeight;
        _driver->writeGPURegister(GPU_TEXTURE_HEIGHT, _SAMPLER_ID, data);
        _textureHeight.restart();
    }

    // Compute texture depth
    _textureDepth = texAdapter.getDepth(static_cast<GAL_CUBEMAP_FACE>(0), 0);
    if ( _textureDepth.changed() || _requiredSync ) 
    {
        data.uintVal = _textureDepth;
        _driver->writeGPURegister(GPU_TEXTURE_DEPTH, _SAMPLER_ID, data);
        _textureHeight.restart();
    }

    // Compute texture log two width
    _textureWidth2 = static_cast<gal_uint>(libGAL::logTwo(texAdapter.getWidth(static_cast<GAL_CUBEMAP_FACE>(0),_baseLevel)));
    if ( _textureWidth2.changed() || _requiredSync ) 
    {   
        data.uintVal = _textureWidth2;
        _driver->writeGPURegister(GPU_TEXTURE_WIDTH2, _SAMPLER_ID, data);
        _textureWidth2.restart();
    }

    // Compute texture log two height
    _textureHeight2  = static_cast<gal_uint>(libGAL::logTwo(texAdapter.getHeight(static_cast<GAL_CUBEMAP_FACE>(0),_baseLevel)));
    if ( data.uintVal != _textureHeight2 || _requiredSync ) 
    {
        data.uintVal = _textureHeight2;
        _driver->writeGPURegister(GPU_TEXTURE_HEIGHT2, _SAMPLER_ID, data);
        _textureHeight2.restart();
    }

    // Compute texture log two depth
    _textureDepth2  = static_cast<gal_uint>(libGAL::logTwo(texAdapter.getDepth(static_cast<GAL_CUBEMAP_FACE>(0),_baseLevel)));
    if ( data.uintVal != _textureDepth2 || _requiredSync ) 
    {
        data.uintVal = _textureDepth2;
        _driver->writeGPURegister(GPU_TEXTURE_DEPTH2, _SAMPLER_ID, data);
        _textureDepth2.restart();
    }

    _textureFormat = texAdapter.getFormat(static_cast<GAL_CUBEMAP_FACE>(0), _baseLevel);
    if (_textureFormat.changed() || _requiredSync)
    {
        GPURegData dataInverted;
        GPURegData dataCompression;

        _getGPUTextureFormat(texAdapter.getFormat(static_cast<GAL_CUBEMAP_FACE>(0), _baseLevel), &data, &dataCompression, &dataInverted);
        _driver->writeGPURegister(GPU_TEXTURE_FORMAT, _SAMPLER_ID, data);
        _driver->writeGPURegister(GPU_TEXTURE_COMPRESSION, _SAMPLER_ID, dataCompression);

        data.booleanVal = dataInverted.booleanVal && !renderBufferTexture;
        _driver->writeGPURegister(GPU_TEXTURE_D3D9_COLOR_CONV, _SAMPLER_ID, data);

        // Update texture texels reversing
        data.booleanVal = (_textureFormat == GAL_FORMAT_UNSIGNED_INT_8888 ? true : false);
        _driver->writeGPURegister(GPU_TEXTURE_REVERSE, _SAMPLER_ID, data);

        _textureFormat.restart();
    }

    _textureType = _texture->getType();
    if ( _textureType.changed() || _requiredSync ) {
        _getGPUTextureMode(_textureType, &data);
        _driver->writeGPURegister(GPU_TEXTURE_MODE, _SAMPLER_ID, data);
        _textureType.restart();
    }

    _memoryLayout = _texture->getMemoryLayout();
    if (_memoryLayout.changed() || _requiredSync)
    {
       _getGPUTexMemoryLayout(_memoryLayout, &data);
        _driver->writeGPURegister(GPU_TEXTURE_BLOCKING, _SAMPLER_ID, data);
        _memoryLayout.restart();
    }

    // Compute texture faces
    const gal_uint totalFaces = ( _texture->getType() == GAL_RESOURCE_TEXTURECUBEMAP ? 6 : 1 );

    // Compute the texture unit start register
    const gal_uint samplerUnitOffset = _SAMPLER_ID * (GAL_MAX_TEXTURE_LEVEL+1) * 6;



    GALTexture2DImp* texture2D = 0;
	GALTexture3DImp* texture3D = 0;
    GALTextureCubeMapImp* textureCM = 0;

    if(_texture->getType() == GAL_RESOURCE_TEXTURE2D)
        texture2D = static_cast<GALTexture2DImp*>(_texture);
    else if (_texture->getType() == GAL_RESOURCE_TEXTURE3D)
        texture3D = static_cast<GALTexture3DImp*>(_texture);
	else
        textureCM = static_cast<GALTextureCubeMapImp*>(_texture);

    MemoryObjectAllocator& moa = _device->allocator();
    for ( gal_uint face = 0; face < totalFaces; ++face ) 
    {
        for ( gal_uint level = _baseLevel; level <= _maxLevel; ++level ) 
        {
            gal_uint mipRegion = texAdapter.region(static_cast<GAL_CUBEMAP_FACE>(face),level); // compute the associated region to this mipmap

            gal_uint md;
            if (texture2D)
            {
                moa.syncGPU(texture2D, mipRegion);
                md = moa.md(texture2D, mipRegion);

#ifdef GAL_DUMP_SAMPLERS
                gal_ubyte filename[30];
                sprintf((char*)filename, "Frame_%d_Batch_%d_Sampler%d_Mipmap%d_Format_%d.ppm", _device->getCurrentFrame(), _device->getCurrentBatch(), _SAMPLER_ID, level, _getFormatString(_textureFormat));
                texture2D->dumpMipmap(mipRegion, filename);
#endif
            }
            else if (textureCM)
            {
                moa.syncGPU(textureCM, mipRegion);
                md = moa.md(textureCM, mipRegion);

#ifdef GAL_DUMP_SAMPLERS
                    gal_ubyte filename[30];
                    sprintf((char*)filename, "Frame_%d_Batch_%d_Sampler%d_Face%d_Mipmap%d_Format_%d.ppm", _device->getCurrentFrame(), _device->getCurrentBatch(), _SAMPLER_ID, face, level, _getFormatString(_textureFormat));
                    textureCM->dumpMipmap((GAL_CUBEMAP_FACE)face, level, filename);
#endif
            }
			else
			{
                moa.syncGPU(texture3D, mipRegion);
                md = moa.md(texture3D, mipRegion);
			}

            gal_uint registerIndex = samplerUnitOffset + 6*level + face; // Compute the corresponding register index
            _driver->writeGPUAddrRegister(GPU_TEXTURE_ADDRESS, registerIndex, md);
        }
    }
}

void GALSamplerImp::performBlitOperation2D(gal_uint xoffset, gal_uint yoffset, gal_uint x, gal_uint y, gal_uint width, gal_uint height, gal_uint textureWidth, GAL_FORMAT internalFormat, GALTexture2D* texture, gal_uint level)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")

    using namespace cg1gpu;
    GPURegData data;

    if (xoffset < 0 || yoffset < 0)
        CG_ASSERT("Texture subregions are not allowed to start at negative x,y coordinates");

    if (x < 0 || y < 0 )
        CG_ASSERT("Framebuffer regions to blit are not allowed to start at negative x,y coordinates");

    if (width < 0 || height < 0 )
        CG_ASSERT("Blitting of negative regions of the framebuffer not allowed");


    _blitFormat = internalFormat;
    if (_blitFormat.changed() || _requiredSync)
    {
	    if (internalFormat == GAL_FORMAT_ARGB_8888 || internalFormat == GAL_FORMAT_RGBA_8888 || 
            internalFormat == GAL_FORMAT_RGB_888 || internalFormat == GAL_FORMAT_DEPTH_COMPONENT_24)
        {
            data.txFormat = GPU_RGBA8888;
            _driver->writeGPURegister(GPU_BLIT_DST_TX_FORMAT, 0, data);
        }
        else
        {
            char error[128];
            sprintf(error, "Unsupported Blit operation texture format. Format used: %d", internalFormat);
            CG_ASSERT( error);
        }

        _blitFormat.restart();
    }

    // Compute the ceiling of the texture width power of two.
    _blitWidth2 = static_cast<gal_uint>(libGAL::ceil(libGAL::logTwo(textureWidth)));
    if (_blitWidth2.changed() || _requiredSync)
    {
        data.uintVal = _blitWidth2;
        _driver->writeGPURegister(GPU_BLIT_DST_TX_WIDTH2, 0, data);
        _blitWidth2.restart();
    }
    
    _blitIniX = x;
    if (_blitIniX.changed() || _requiredSync)
    {
        data.uintVal = _blitIniX;
        _driver->writeGPURegister(GPU_BLIT_INI_X, 0, data);
        _blitIniX.restart();
    }

    _blitIniY = y;
    if (_blitIniY.changed() || _requiredSync)
    {
        data.uintVal = _blitIniY;
        _driver->writeGPURegister(GPU_BLIT_INI_Y, 0, data);
        _blitIniY.restart();
    }

    _blitXoffset = xoffset;
    if (_blitXoffset.changed() || _requiredSync)
    {
        data.uintVal = _blitXoffset;
        _driver->writeGPURegister(GPU_BLIT_X_OFFSET, 0, data);
        _blitXoffset.restart();
    }

    _blitYoffset = yoffset;
    if (_blitYoffset.changed() || _requiredSync)
    {
        data.uintVal = _blitYoffset;
        _driver->writeGPURegister(GPU_BLIT_Y_OFFSET, 0, data);
        _blitYoffset.restart();
    }

    _blitWidth = width;
    if (_blitWidth.changed() || _requiredSync)
    {
        data.uintVal = _blitWidth;
        _driver->writeGPURegister(GPU_BLIT_WIDTH, 0, data);
        _blitWidth.restart();
    }

    _blitHeight = height;
    if (_blitHeight.changed() || _requiredSync)
    {
        data.uintVal = _blitHeight;
        _driver->writeGPURegister(GPU_BLIT_HEIGHT, 0, data);
        _blitHeight.restart();
    }

    if (!_requiredSync)
    {
        GALTexture2DImp* tex2D = (GALTexture2DImp*) texture;
        MemoryObjectAllocator& moa = _device->allocator();
        moa.syncGPU(tex2D, level);

        _blitTextureBlocking = tex2D->getMemoryLayout();
        if (_blitTextureBlocking.changed())
        {
            _getGPUTexMemoryLayout(_blitTextureBlocking, &data);
            _driver->writeGPURegister(GPU_BLIT_DST_TX_BLOCK, 0, data);
            _blitTextureBlocking.restart();
        }

        gal_uint md = moa.md(tex2D, level);
        _driver->writeGPUAddrRegister(GPU_BLIT_DST_ADDRESS, 0, md);

        _driver->sendCommand(GPU_BLIT);

        tex2D->postBlit(level);
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALSamplerImp::sync()
{
    GAL_ASSERT(
        // Check texture completeness before synchronize texture in GPU/system memory
        if ( _enabled && _texture && !_texture->wellDefined() )
            CG_ASSERT("Texture is not well defined (texture completness required)");
    )

    cg1gpu::GPURegData data;

    if ( _enabled.changed() ) {
        data.booleanVal = _enabled;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_ENABLE, _SAMPLER_ID, data);
        _enabled.restart();
    }

    /// If the sampler is disabled we are done (no sync required)
    if ( !_enabled && !_requiredSync)
        return;

    /// { S: The texture sampler is enabled }

    /// Synchronize texture unit state with the current bound texture
    if ( _texture )
        _syncTexture();

    /// Update SAMPLER STATE independent of the texture
    if ( _sCoord.changed() || _requiredSync ) {
        _getGPUClampMode(_sCoord, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_WRAP_S, _SAMPLER_ID, data);
        _sCoord.restart();
    }
    if ( _tCoord.changed() || _requiredSync ) {
        _getGPUClampMode(_tCoord, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_WRAP_T, _SAMPLER_ID, data);
        _tCoord.restart();
    }
    if ( _rCoord.changed() || _requiredSync ) {
        _getGPUClampMode(_rCoord, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_WRAP_R, _SAMPLER_ID, data);
        _rCoord.restart();
    }

    if ( _nonNormalizedCoords.changed() || _requiredSync )
    {
        data.booleanVal = _nonNormalizedCoords;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_NON_NORMALIZED, _SAMPLER_ID, data);
        _nonNormalizedCoords.restart();
    }

    if ( _minFilter.changed() || _requiredSync ) {
        _getGPUTexFilter(_minFilter, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_MIN_FILTER, _SAMPLER_ID, data);
        _minFilter.restart();
    }
    if ( _magFilter.changed() || _requiredSync ) {
        _getGPUTexFilter(_magFilter, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_MAG_FILTER, _SAMPLER_ID, data);
        _magFilter.restart();
    }

    if (_enableComparison.changed() || _requiredSync)
    {
        data.booleanVal = _enableComparison;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_ENABLE_COMPARISON, _SAMPLER_ID, data);
        _enableComparison.restart();
    }
    
    if (_comparisonFunction.changed() || _requiredSync)
    {
        _getGPUTexComp(_comparisonFunction, &data);
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_COMPARISON_FUNCTION, _SAMPLER_ID, data);
        _comparisonFunction.restart();        
    }
    
    if (_sRGBConversion.changed() || _requiredSync)
    {
        data.booleanVal = _sRGBConversion;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_SRGB, _SAMPLER_ID, data);
        _sRGBConversion.restart();
    }

    if ( _minLOD.changed() || _requiredSync ) {
        data.f32Val = _minLOD;
        data.f32Val = 0.0f;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_MIN_LOD, _SAMPLER_ID, data);
        _minLOD.restart();
    }
    if ( _maxLOD.changed() || _requiredSync ) {
        data.f32Val = _maxLOD;
        data.f32Val = 12.0f;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_MAX_LOD, _SAMPLER_ID, data);
        _maxLOD.restart();
    }
    if ( _maxAniso.changed() || _requiredSync ) {
        data.uintVal = _maxAniso;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_MAX_ANISOTROPY, _SAMPLER_ID, data);
        _maxAniso.restart();
    }
    if ( _lodBias.changed() || _requiredSync ) {
        data.f32Val = _lodBias;
        _driver->writeGPURegister(cg1gpu::GPU_TEXTURE_LOD_BIAS, _SAMPLER_ID, data);
        _lodBias.restart();
    }
    if ( _unitLodBias.changed() || _requiredSync ) {
        data.f32Val = _unitLodBias;
        _driver->writeGPURegister(cg1gpu::GPU_TEXT_UNIT_LOD_BIAS, _SAMPLER_ID, data);
        _unitLodBias.restart();
    }
}

void GALSamplerImp::forceSync()
{
    _requiredSync = true;
    sync();
    performBlitOperation2D(0, 0, 0, 0, 0, 0, 0, GAL_FORMAT_RGBA_8888, 0, 0);
    _requiredSync = false;
}

void GALSamplerImp::_getGPUClampMode(GAL_TEXTURE_ADDR_MODE mode, cg1gpu::GPURegData* data)
{
    switch ( mode ) {
        case GAL_TEXTURE_ADDR_CLAMP:
            data->txClamp = cg1gpu::GPU_TEXT_CLAMP;
            break;
        case GAL_TEXTURE_ADDR_CLAMP_TO_EDGE:
            data->txClamp = cg1gpu::GPU_TEXT_CLAMP_TO_EDGE;
            break;
        case GAL_TEXTURE_ADDR_REPEAT:
            data->txClamp = cg1gpu::GPU_TEXT_REPEAT;
            break;
        case GAL_TEXTURE_ADDR_CLAMP_TO_BORDER:
            data->txClamp = cg1gpu::GPU_TEXT_CLAMP_TO_BORDER;
            break;
        case GAL_TEXTURE_ADDR_MIRRORED_REPEAT:
            data->txClamp = cg1gpu::GPU_TEXT_MIRRORED_REPEAT;
            break;
        default:
            CG_ASSERT("GAL_TEXTURE_ADDR_MODE unknown");
    }
}

void GALSamplerImp::_getGPUTexFilter(GAL_TEXTURE_FILTER filter, cg1gpu::GPURegData* data)
{
    switch ( filter ) {
        case GAL_TEXTURE_FILTER_NEAREST:
            data->txFilter = cg1gpu::GPU_NEAREST;
            break;
        case GAL_TEXTURE_FILTER_LINEAR:
            data->txFilter = cg1gpu::GPU_LINEAR;
            break;
        case GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            data->txFilter = cg1gpu::GPU_NEAREST_MIPMAP_NEAREST;
            break;
        case GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            data->txFilter = cg1gpu::GPU_NEAREST_MIPMAP_LINEAR;
            break;
        case GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            data->txFilter = cg1gpu::GPU_LINEAR_MIPMAP_NEAREST;
            break;
        case GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            data->txFilter = cg1gpu::GPU_LINEAR_MIPMAP_LINEAR;
            break;
        default:
            CG_ASSERT("Unknown GAL_TEXTURE_FILTER");
    }
}

void GALSamplerImp::_getGPUTexComp(GAL_TEXTURE_COMPARISON function, cg1gpu::GPURegData* data)
{
    switch(function)    
    {
        case GAL_TEXTURE_COMPARISON_NEVER:
            data->compare = cg1gpu::GPU_NEVER;
            break;
        case GAL_TEXTURE_COMPARISON_ALWAYS:
            data->compare = cg1gpu::GPU_ALWAYS;
            break;
        case GAL_TEXTURE_COMPARISON_LESS:
            data->compare = cg1gpu::GPU_LESS;
            break;
        case GAL_TEXTURE_COMPARISON_LEQUAL:
            data->compare = cg1gpu::GPU_LEQUAL;
            break;
        case GAL_TEXTURE_COMPARISON_EQUAL:
            data->compare = cg1gpu::GPU_EQUAL;
            break;
        case GAL_TEXTURE_COMPARISON_GEQUAL:
            data->compare = cg1gpu::GPU_GEQUAL;
            break;
        case GAL_TEXTURE_COMPARISON_GREATER:
            data->compare = cg1gpu::GPU_GREATER;
            break;
        case GAL_TEXTURE_COMPARISON_NOTEQUAL:
            data->compare = cg1gpu::GPU_NOTEQUAL;
            break;
        default:
            CG_ASSERT("Unknown GAL_TEXTURE_COMPARISON");
            break;            
    }
}

const char* GALSamplerImp::AddressModePrint::print(const GAL_TEXTURE_ADDR_MODE &var) const
{
    switch(var)
    {
        case GAL_TEXTURE_ADDR_CLAMP:
            return "CLAMP"; break;
        case GAL_TEXTURE_ADDR_CLAMP_TO_EDGE:
            return "CLAMP_TO_EDGE"; break;
        case GAL_TEXTURE_ADDR_REPEAT:
            return "REPEAT"; break;
        case GAL_TEXTURE_ADDR_CLAMP_TO_BORDER:
            return "CLAMP_TO_BORDER"; break;
        case GAL_TEXTURE_ADDR_MIRRORED_REPEAT:
            return "MIRRORED_REPEAT"; break;
        default:
            return "UNDEFINED";
    }
}

const char* GALSamplerImp::FilterPrint::print(const libGAL::GAL_TEXTURE_FILTER &var) const
{
    switch(var)
    {
        case GAL_TEXTURE_FILTER_NEAREST:
            return "NEAREST"; break;
        case GAL_TEXTURE_FILTER_LINEAR:
            return "LINEAR"; break;
        case GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return "NEAREST_MIPMAP_NEAREST"; break;
        case GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return "NEAREST_MIPMAP_LINEAR"; break;
        case GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return "LINEAR_MIPMAP_NEAREST"; break;
        case GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return "LINEAR_MIPMAP_LINEAR"; break;
        default:
            return "UNDEFINED";
    };
}

const char* GALSamplerImp::CompFuncPrint::print(const libGAL::GAL_TEXTURE_COMPARISON &var) const
{
    switch(var)
    {
        case GAL_TEXTURE_COMPARISON_NEVER:
            return "NEVER";
            break;
        case GAL_TEXTURE_COMPARISON_ALWAYS:
            return "ALWAYS";
            break;
        case GAL_TEXTURE_COMPARISON_LESS:
            return "LESS";
            break;
        case GAL_TEXTURE_COMPARISON_LEQUAL:
            return "LEQUAL";
            break;
        case GAL_TEXTURE_COMPARISON_EQUAL:
            return "EQUAL";
            break;
        case GAL_TEXTURE_COMPARISON_GEQUAL:
            return "GEQUAL";
            break;
        case GAL_TEXTURE_COMPARISON_GREATER:
            return "GREATER";
            break;
        case GAL_TEXTURE_COMPARISON_NOTEQUAL:
            return "NOTEQUAL";
            break;
        default:
            return "UNDEFINED";
            break;
    }
}


const StoredStateItem* GALSamplerImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GALStoredStateItem* ret;
    gal_uint rTarget;

    if (stateId >= GAL_SAMPLER_FIRST_ID && stateId < GAL_SAMPLER_LAST)
    {    
        // It�s a blending state
        switch(stateId)
        {
            case GAL_SAMPLER_ENABLED:           ret = new GALSingleBoolStoredStateItem(_enabled);           break;
            case GAL_SAMPLER_CLAMP_S:           ret = new GALSingleEnumStoredStateItem(_sCoord);            break;
            case GAL_SAMPLER_CLAMP_T:           ret = new GALSingleEnumStoredStateItem(_tCoord);            break;
            case GAL_SAMPLER_CLAMP_R:           ret = new GALSingleEnumStoredStateItem(_rCoord);            break;
            case GAL_SAMPLER_NON_NORMALIZED:    ret = new GALSingleBoolStoredStateItem(_nonNormalizedCoords); break;
            case GAL_SAMPLER_MIN_FILTER:        ret = new GALSingleEnumStoredStateItem(_minFilter);         break;
            case GAL_SAMPLER_MAG_FILTER:        ret = new GALSingleEnumStoredStateItem(_magFilter);         break;
            case GAL_SAMPLER_ENABLE_COMP:       ret = new GALSingleBoolStoredStateItem(_enableComparison);  break;
            case GAL_SAMPLER_COMP_FUNCTION:     ret = new GALSingleEnumStoredStateItem(_comparisonFunction);break;
            case GAL_SAMPLER_SRGB_CONVERSION:   ret = new GALSingleBoolStoredStateItem(_sRGBConversion);    break;
            case GAL_SAMPLER_MIN_LOD:           ret = new GALSingleFloatStoredStateItem(_minLOD);           break;
            case GAL_SAMPLER_MAX_LOD:           ret = new GALSingleFloatStoredStateItem(_maxLOD);           break;
            case GAL_SAMPLER_MAX_ANISO:         ret = new GALSingleUintStoredStateItem(_maxAniso);          break;
            case GAL_SAMPLER_LOD_BIAS:          ret = new GALSingleFloatStoredStateItem(_lodBias);          break;
            case GAL_SAMPLER_UNIT_LOD_BIAS:     ret = new GALSingleFloatStoredStateItem(_unitLodBias);      break;
            case GAL_SAMPLER_TEXTURE:           ret = new GALSingleVoidStoredStateItem(_texture);           break;
            
            // case GAL_STREAM_... <- add here future additional blending states.

            default:
                CG_ASSERT("Unexpected blending state id");
        }          
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    ret->setItemId(stateId);

    GLOBAL_PROFILER_EXIT_REGION()

    return ret;
}

#define CAST_TO_UINT(X)         *(static_cast<const GALSingleUintStoredStateItem*>(X))
#define CAST_TO_BOOL(X)         *(static_cast<const GALSingleBoolStoredStateItem*>(X))
#define CAST_TO_FLOAT(X)        *(static_cast<const GALSingleFloatStoredStateItem*>(X))
#define CAST_TO_ENUM(X)         *(static_cast<const GALSingleEnumStoredStateItem*>(X))
#define CAST_TO_VOID(X)         static_cast<void*>(*(static_cast<const GALSingleVoidStoredStateItem*> (X)))

void GALSamplerImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    gal_uint rTarget;

    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    if (stateId >= GAL_SAMPLER_FIRST_ID && stateId < GAL_SAMPLER_LAST)
    {    

        // It�s a blending state
        switch(stateId)
        {
            case GAL_SAMPLER_ENABLED:           _enabled = CAST_TO_BOOL(galssi);                                        break;
            //case GAL_SAMPLER_CLAMP_S:           _sCoord = static_cast<GAL_TEXTURE_ADDR_MODE>(CAST_TO_ENUM(galssi));     break;
            //case GAL_SAMPLER_CLAMP_T:           _tCoord = static_cast<GAL_TEXTURE_ADDR_MODE>(CAST_TO_ENUM(galssi));     break;
            //case GAL_SAMPLER_CLAMP_R:           _rCoord = static_cast<GAL_TEXTURE_ADDR_MODE>(CAST_TO_ENUM(galssi));     break;
            case GAL_SAMPLER_NON_NORMALIZED:    _nonNormalizedCoords = CAST_TO_BOOL(galssi);                            break;
            //case GAL_SAMPLER_MIN_FILTER:        _minFilter = static_cast<GAL_TEXTURE_FILTER>(CAST_TO_ENUM(galssi));     break;
            //case GAL_SAMPLER_MAG_FILTER:        _magFilter = static_cast<GAL_TEXTURE_FILTER>(CAST_TO_ENUM(galssi));     break;
            case GAL_SAMPLER_ENABLE_COMP:       _enableComparison = CAST_TO_BOOL(galssi);                               break;
            //case GAL_SAMPLER_COMP_FUNCTION:     _comparisonFunction = static_cast<GAL_TEXTURE_COMPARISON>(CAST_TO_ENUM(galssi));  break;
            case GAL_SAMPLER_SRGB_CONVERSION:   _sRGBConversion = CAST_TO_BOOL(galssi);                                 break;
            case GAL_SAMPLER_MIN_LOD:           _minLOD = CAST_TO_FLOAT(galssi);                                        break;
            case GAL_SAMPLER_MAX_LOD:           _maxLOD = CAST_TO_FLOAT(galssi);                                        break;
            case GAL_SAMPLER_MAX_ANISO:         _maxAniso = CAST_TO_UINT(galssi);                                       break;
            case GAL_SAMPLER_LOD_BIAS:          _lodBias = CAST_TO_FLOAT(galssi);                                       break;
            case GAL_SAMPLER_UNIT_LOD_BIAS:     _unitLodBias = CAST_TO_FLOAT(galssi);                                   break;
            case GAL_SAMPLER_TEXTURE:           _texture = static_cast<GALTexture*>(CAST_TO_VOID(galssi));              break;

            // case GAL_STREAM_... <- add here future additional blending states.
            default:
                CG_ASSERT("Unexpected blending state id");
        }           
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    GLOBAL_PROFILER_EXIT_REGION()
}

#undef CAST_TO_UINT
#undef CAST_TO_BOOL             
#undef CAST_TO_FLOAT
#undef CAST_TO_ENUM
#undef CAST_TO_VOID

