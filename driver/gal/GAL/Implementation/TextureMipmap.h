/**************************************************************************
 *
 */

#ifndef TEXTUREMIPMAP
    #define TEXTUREMIPMAP

#include "GALTypes.h"
#include "GALTexture2D.h"

class HAL;

namespace libGAL
{

class TextureMipmap
{
private:

    friend class GALDeviceImp;

    static void setTextureTiling(gal_uint tileLevel1Sz, gal_uint tileLevel2Sz);
    static void getTextureTiling(gal_uint& tileLevel1Sz, gal_uint& tileLevel2Sz);
    static void setDriver(HAL* driver);

    static gal_uint _tileLevel1Sz;
    static gal_uint _tileLevel2Sz;
    static HAL* _driver;

    // Temporary all is mutable... (bad practise)
    mutable gal_ubyte* _data;
    mutable gal_uint _dataSize;
    mutable gal_uint _rowPitch;
    mutable gal_uint _planePitch;

    mutable gal_uint _width;
    mutable gal_uint _height;
    mutable gal_uint _depth;
    mutable gal_bool _multisampling;
    mutable gal_uint _samples;
    mutable GAL_FORMAT _format;

    mutable gal_ubyte* _mortonData;
    mutable gal_uint _mortonDataSize;
    
    public:

    TextureMipmap();

    gal_uint getWidth() const;
    gal_uint getHeight() const;
    gal_uint getDepth() const;
    GAL_FORMAT getFormat() const;   
    gal_bool isMultisampled() const;    
    gal_uint getSamples() const;

    // Returns the texel size in bytes (only for uncompressed textures)
    gal_float getTexelSize () const;
    static gal_float getTexelSize(GAL_FORMAT format);

    // To implement MAP operation on textures
    // Return true if data has been defined
    bool getData(gal_ubyte*& pData, gal_uint& rowPitch, gal_uint& planePitch);
    bool getData(const gal_ubyte*& pData, gal_uint& rowPitch, gal_uint& planePitch) const;

    void setData( gal_uint width, gal_uint height, gal_uint depth, GAL_FORMAT format, 
                  gal_uint rowPitch, const gal_ubyte* srcTexelData, gal_uint texSize );

    virtual void setData(gal_uint width, gal_uint height, gal_uint depth, gal_bool multisampling, gal_uint samples, GAL_FORMAT format);

    void updateData(const gal_ubyte* srcTexelData);

    void copyData( gal_uint mipLevel, GALTexture2D* destTexture);

    void updateData( gal_uint xoffset, gal_uint yoffset, gal_uint zoffset, gal_uint width, gal_uint height, gal_uint depth,
                        GAL_FORMAT format, gal_uint rowPitch, const gal_ubyte* srcTexelData );

    void clear();

    const gal_ubyte* getDataInMortonOrder(gal_uint& sizeInBytes) const;

    void dump2PPM (gal_ubyte* filename);
};

}

#endif
