/**************************************************************************
 *
 */

#include "TextureMipmap.h"
#include "support.h"
#include <cstring>
#include "GALMath.h"
#include "HAL.h"
#include <sstream>

#include "Profiler.h"

using namespace libGAL;
using namespace std;

gal_uint TextureMipmap::_tileLevel2Sz = 0;
gal_uint TextureMipmap::_tileLevel1Sz = 0;
HAL* TextureMipmap::_driver = 0;

void TextureMipmap::setTextureTiling(gal_uint tileLevel1Sz, gal_uint tileLevel2Sz)
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")
    static gal_bool alreadyCalled = false;

    if ( !alreadyCalled )
    {
        alreadyCalled = true;
        _tileLevel1Sz = tileLevel1Sz;
        _tileLevel2Sz = tileLevel2Sz;
        TRACING_EXIT_REGION()
    }
}

void TextureMipmap::setDriver(HAL* driver)
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")
    static gal_bool alreadyCalled = false;

    if ( !alreadyCalled )
    {
        alreadyCalled = true;
        _driver = driver;
        TRACING_EXIT_REGION()
    }
}


void TextureMipmap::getTextureTiling(gal_uint& tileLevel1Sz, gal_uint& tileLevel2Sz)
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")
    tileLevel1Sz = _tileLevel1Sz;
    tileLevel2Sz = _tileLevel2Sz;
    TRACING_EXIT_REGION()
}

TextureMipmap::TextureMipmap() : _data(0), _dataSize(0), _rowPitch(0), _planePitch(0), _mortonData(0), 
                                 _width(0), _height(0), _depth(0), _format(GAL_FORMAT_RGBA_8888),
                                 _multisampling(false), _samples(1)
{
    if ( _tileLevel1Sz == 0 || _tileLevel1Sz == 0 )
        CG_ASSERT("Tile level sizes not defined (must be defined before any ctor call)");
    
}

gal_uint TextureMipmap::getWidth() const { return _width; }

gal_uint TextureMipmap::getHeight() const { return _height; }

gal_uint TextureMipmap::getDepth() const { return _depth; }

GAL_FORMAT TextureMipmap::getFormat() const { return _format; }

gal_bool TextureMipmap::isMultisampled() const { return _multisampling; }

gal_uint TextureMipmap::getSamples() const { return _samples; }

gal_float TextureMipmap::getTexelSize() const 
{
    return getTexelSize(_format);
}

gal_float TextureMipmap::getTexelSize(GAL_FORMAT format) 
{
    switch ( format ) 
    {
        case GAL_FORMAT_RGB_565:
            return 2;
            break;
        case GAL_FORMAT_RGB_888:
            return 3;
        case GAL_FORMAT_RGBA_8888:
        case GAL_FORMAT_ARGB_8888:
        case GAL_FORMAT_XRGB_8888:
        case GAL_FORMAT_QWVU_8888:
        case GAL_FORMAT_UNSIGNED_INT_8888:
        case GAL_FORMAT_DEPTH_COMPONENT_24:
            return 4;
        case GAL_FORMAT_RG16F:
            return 4;
            break;  
        case GAL_FORMAT_S8D24:
            return 4;
        case GAL_FORMAT_R32F:
            return 4;
            break;
        case GAL_FORMAT_RGBA32F:
            return 16;
            break;
        case GAL_FORMAT_ABGR_161616:
        case GAL_FORMAT_RGBA16F:
            return 8;
        case GAL_FORMAT_ARGB_1555:
            return 2;
        case GAL_FORMAT_INTENSITY_8:
        case GAL_FORMAT_ALPHA_8:
        case GAL_FORMAT_LUMINANCE_8:
            return 1;
        case GAL_FORMAT_LUMINANCE8_ALPHA8:
            return 2;
        case GAL_FORMAT_NULL:
            return 0;
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
        case GAL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            return 0.5;
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
        case GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            return 1;
        default:
            char error[128];
            sprintf(error, "Unknown format size: %d", format);
            CG_ASSERT(error);
            return 0;
    }
}

bool TextureMipmap::getData(gal_ubyte*& pData, gal_uint& rowPitch, gal_uint& planePitch)
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")

    pData = _data;
    rowPitch = _rowPitch;
    planePitch = _planePitch;

    TRACING_EXIT_REGION()
    return true;
}

void TextureMipmap::dump2PPM (gal_ubyte* filename)
{
    FILE *fout;

    U08 red;
    U08 green;
    U08 blue;
    gal_bool alpha = false;

    switch (_format)
    {
        case GAL_FORMAT_RGBA_8888:
        case GAL_FORMAT_DEPTH_COMPONENT_32:
            alpha = true;

        case GAL_FORMAT_RGB_888:
        case GAL_FORMAT_DEPTH_COMPONENT_24:
        case GAL_COMPRESSED_S3TC_DXT1_RGB:

            fout = fopen((const char*)filename, "wb");

            //  Write magic number.  
            fprintf(fout, "P6\n");

            //  Write frame size.  
            fprintf(fout, "%d %d\n", _width, _height);

            //  Write color component maximum value.  
            fprintf(fout, "255\n");

            // Supose unsigned byte

            gal_ubyte* index;
            index = _data;
         
            for (  int i = 0 ; i < _height; i++ )
            {
                for ( int j = 0; j < _width; j++ )
                {
                    fputc( char(*index), fout ); index++;
                    fputc( char(*index), fout ); index++;
                    fputc( char(*index), fout ); index++;
                    if (alpha) index++;
                }
            }

            fclose(fout);

            break;

        default:
            break;
    }

}

bool TextureMipmap::getData(const gal_ubyte*& pData, gal_uint& rowPitch, gal_uint& planePitch) const
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")

    pData = _data;
    rowPitch = _rowPitch;
    planePitch = _planePitch;

    TRACING_EXIT_REGION()

    return true;    
}

void TextureMipmap::setData( gal_uint width, gal_uint height, gal_uint depth, GAL_FORMAT format, 
                             gal_uint rowPitch, const gal_ubyte* srcTexelData, gal_uint texSize )
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")

    // Delete previous mipmap contents
    delete[] _data;
    delete[] _mortonData;

    // Mipmap data not converted to morton order.
    _mortonData = 0;
    _mortonDataSize = 0;

    // Update mipmap properties
    _format = format;
    _width = width;
    _height = height;
    _depth = depth;

    gal_bool compressed;

    switch ( format ) 
    {
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
        case GAL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:

            _dataSize = texSize;
            compressed = true;
            _rowPitch = max((gal_uint) 4, width) / 2;
            _planePitch = max((gal_uint) 16, width*height) / 2;
            break;

        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
        case GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:

            _dataSize = texSize;
            compressed = true;
            _rowPitch = max((gal_uint) 4, width);
            _planePitch = max((gal_uint) 16, width*height);
            break;

        default:
            _dataSize = width * height * depth * getTexelSize();
            _rowPitch = width * getTexelSize();
            _planePitch = height * width * getTexelSize();
            compressed = false;
    }

    // Allocate mipmap data
    _data = new gal_ubyte[_dataSize];

    if ( srcTexelData != 0 ) {
        if ( compressed || rowPitch == 0 || rowPitch == width )
            memcpy(_data, srcTexelData, _dataSize);
        else 
        {
            // Copy texture memory contiguous
            for ( gal_uint i = 0; i < (height*depth); ++i )
                memcpy(_data + i*_rowPitch, srcTexelData + i*rowPitch, _dataSize);
        }
    }

    TRACING_EXIT_REGION()
}

void TextureMipmap::setData(gal_uint width, gal_uint height, gal_uint depth, gal_bool multisampling, gal_uint samples, GAL_FORMAT format)
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")

    // Delete previous mipmap contents
    delete[] _data;
    delete[] _mortonData;

    // Mipmap data not converted to morton order.
    _mortonData = 0;
    _mortonDataSize = 0;

    //  Create mipmap with no defined data.
    _data = 0;
    _dataSize = 0;
    _rowPitch = 0;
    _planePitch = 0;

    // Update mipmap properties
    _format = format;
    _width = width;
    _height = height;
    _depth = depth;
    _multisampling = multisampling;
    _samples = samples;   

    TRACING_EXIT_REGION()
}

void TextureMipmap::updateData(const gal_ubyte* srcTexelData)
{
    // Invalidate current precomputed morton data
    if (_mortonData != NULL)
        delete[] _mortonData;

    _mortonData = NULL;

    memcpy(_data, srcTexelData, _dataSize);    
}

void TextureMipmap::copyData( gal_uint mipLevel, GALTexture2D* destTexture)
{
    destTexture->updateData(mipLevel,_data);
}

void TextureMipmap::updateData( gal_uint xoffset, gal_uint yoffset, gal_uint zoffset, gal_uint width, gal_uint height, gal_uint depth, GAL_FORMAT format, 
                                gal_uint rowPitch, const gal_ubyte* srcTexelData )
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")
    if ( _data == 0 )
        CG_ASSERT("Mipmap has no defined data yet");

    if ( format != _format )
        CG_ASSERT("Incompatible formats");

    if ( xoffset + width > _width )
        CG_ASSERT("xoffset + width out of mipmap width bounds");

    if ( yoffset + height > _height )
        CG_ASSERT("yoffset + height out of mipmap height bounds");

    if ( srcTexelData == 0 )
        CG_ASSERT("updateData requires to supply data (not NULL)");

    // Invalidate current precomputed morton data
    if (_mortonData != NULL)
        delete[] _mortonData;

    _mortonData = NULL;

    //  Clamp the 
    gal_uint heightCompr;
    gal_uint depthCompr = 1;
    gal_uint blockSize = 0;

    switch ( _format)
    {
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
            blockSize = 8;
            break;
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
            blockSize = 16;
            break;
        default:
            break;
    }

    switch ( _format )
    {
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:

            {

                if ((width % 4 != 0) && (width > 4))
                    CG_ASSERT("Width is not multiple of 4");

                if ((height % 4 != 0) && (height > 4))
                    CG_ASSERT("Height is not multiple of 4");

                if (xoffset % 4 != 0)
                    CG_ASSERT("Xoffset is not multiple of 4");

                if (yoffset % 4 != 0)
                    CG_ASSERT("Yoffset is not multiple of 4");

                gal_uint widthBlockSize = max(_width / 4, gal_uint(1));
                gal_uint heightBlockSize = max(_height / 4, gal_uint(1));
                gal_uint copyWidthBlockSize = max(width / 4, gal_uint(1));
                gal_uint copyHeightBlockSize = max(height / 4, gal_uint(1));

                gal_uint firstBlock = yoffset/4 * widthBlockSize  + xoffset/4;
                gal_uint lastBlock = (copyHeightBlockSize * widthBlockSize + copyWidthBlockSize);

                gal_ubyte* _initialData = _data + ( blockSize * firstBlock);

                gal_uint blockRowPitch = blockSize * widthBlockSize;
                gal_uint copyBlockRowPitch = blockSize * copyWidthBlockSize;

                for (gal_uint h = 0; h < copyHeightBlockSize; h++)
                        memcpy(_initialData + blockRowPitch*h, srcTexelData + copyBlockRowPitch*h, blockSize*copyWidthBlockSize);

            }
            break;

        default:
            heightCompr = height;
            depthCompr = depth;
   
            // Update mipmap data
            const gal_uint offset = xoffset * getTexelSize() + yoffset*_rowPitch + zoffset*_planePitch;

            gal_uint k = 0;

            for ( gal_uint j = 0; j < depthCompr; ++j)
                for ( gal_uint i = 0; i < heightCompr; ++i )
                {
                    memcpy(_data + offset + i*_rowPitch + j*_planePitch, srcTexelData + k*rowPitch, rowPitch);
                    k++;
                }

            break;
    }


    TRACING_EXIT_REGION()
}

const gal_ubyte* TextureMipmap::getDataInMortonOrder( gal_uint& sizeInBytes ) const
{
    TRACING_ENTER_REGION("TextureMipmap", "", "")

    //  Check if the mipmap was already converted to morton order.
    if (!_mortonData)
    {
        cg1gpu::TextureCompression compressed;
        switch ( _format )
        {
            case GAL_COMPRESSED_S3TC_DXT1_RGB:
                compressed = cg1gpu::GPU_S3TC_DXT1_RGB;
                break;
            case GAL_COMPRESSED_S3TC_DXT1_RGBA:
                compressed = cg1gpu::GPU_S3TC_DXT1_RGBA;
                break;
            case GAL_COMPRESSED_S3TC_DXT3_RGBA:
                compressed = cg1gpu::GPU_S3TC_DXT3_RGBA;
                break;
            case GAL_COMPRESSED_S3TC_DXT5_RGBA:
                compressed = cg1gpu::GPU_S3TC_DXT5_RGBA;
                break;

            case GAL_COMPRESSED_LUMINANCE_LATC1_EXT:
                compressed = cg1gpu::GPU_LATC1;
                break;
            case GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
                compressed = cg1gpu::GPU_LATC2_SIGNED;
                break;
            case GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
                compressed = cg1gpu::GPU_LATC2;
                break;
            case GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
                compressed = cg1gpu::GPU_LATC2_SIGNED;
                break;
            default:
                compressed = cg1gpu::GPU_NO_TEXTURE_COMPRESSION;
        }

        _mortonData = _driver->getDataInMortonOrder(_data, _width, _height, _depth, compressed, getTexelSize(), _mortonDataSize);

    }

    sizeInBytes = _mortonDataSize;
    TRACING_EXIT_REGION()
    return _mortonData;
}
