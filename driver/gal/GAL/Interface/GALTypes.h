/**************************************************************************
 *
 */

#ifndef GAL_TYPES
    #define GAL_TYPES

namespace libGAL
{
    typedef void gal_void;
    typedef bool gal_bool;
    typedef char gal_char;

    typedef char gal_byte;
    typedef short gal_short;
    typedef int gal_int;
    
    typedef unsigned char gal_ubyte;
    typedef unsigned short gal_ushort;
    typedef unsigned int gal_uint;

    typedef unsigned long long gal_ulonglong;

    typedef float gal_float;
    typedef double gal_double;
    
    typedef gal_ushort gal_enum;   

    /**
     * Represents/defines a 3D bounding box
     */
    struct GAL_BOX
    {
        gal_uint left; ///< x position of the left handside of the box
        gal_uint top; ///< y position of the top of the box
        gal_uint front; ///< z position of the front of the box
        gal_uint right; ///< x position of the right handside of the box
        gal_uint bottom; ///< y position of the bottom of the box 
        gal_uint back; ///< z position of the back of the box
    };

    /**
     * Types of mapping
     */
    enum GAL_MAP
    {
        GAL_MAP_READ,
        GAL_MAP_WRITE,
        GAL_MAP_READ_WRITE,
        GAL_MAP_WRITE_DISCARDPREV,
        GAL_MAP_WRITE_NOOVERWRITE
    };

    /**
     * Data formats supported
     *
     * @note More to be defined.
     */
    enum GAL_FORMAT
    {
        GAL_FORMAT_UNKNOWN,

        // Uncompressed formats
        GAL_FORMAT_RGBA_8888,
        GAL_FORMAT_RGB_565,
        GAL_FORMAT_INTENSITY_8,
        GAL_FORMAT_INTENSITY_12,
        GAL_FORMAT_INTENSITY_16,
        GAL_FORMAT_ALPHA_8,
        GAL_FORMAT_ALPHA_12,
        GAL_FORMAT_ALPHA_16,
        GAL_FORMAT_LUMINANCE_8,
        GAL_FORMAT_LUMINANCE_12,
        GAL_FORMAT_LUMINANCE_16,
        GAL_FORMAT_LUMINANCE8_ALPHA8,
        GAL_FORMAT_DEPTH_COMPONENT_16,
        GAL_FORMAT_DEPTH_COMPONENT_24,
        GAL_FORMAT_DEPTH_COMPONENT_32,
        GAL_FORMAT_RGB_888,
        GAL_FORMAT_ARGB_8888,
        GAL_FORMAT_ABGR_161616,
        GAL_FORMAT_XRGB_8888,
        GAL_FORMAT_QWVU_8888,
        GAL_FORMAT_ARGB_1555,
        GAL_FORMAT_UNSIGNED_INT_8888,
        GAL_FORMAT_RG16F,
        GAL_FORMAT_R32F,
        GAL_FORMAT_S8D24,
        GAL_FORMAT_RGBA16F,
        GAL_FORMAT_RGBA32F,

        GAL_FORMAT_NULL,

        // Compressed formats
        GAL_COMPRESSED_S3TC_DXT1_RGB,
        GAL_COMPRESSED_S3TC_DXT1_RGBA,
        GAL_COMPRESSED_S3TC_DXT3_RGBA,
        GAL_COMPRESSED_S3TC_DXT5_RGBA,
        GAL_COMPRESSED_LUMINANCE_LATC1_EXT,
        GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT,
        GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,
        GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT
        // Add more compressed formats here
    };

    /**
     * Render target dimension identifier
     */
    enum GAL_RT_DIMENSION
    {
        GAL_RT_DIMENSION_UNKNOWN,
        GAL_RT_DIMENSION_BUFFER,
        GAL_RT_DIMENSION_TEXTURE1D,
        GAL_RT_DIMENSION_TEXTURE2D,
        GAL_RT_DIMENSION_TEXTURE2DMS,
        GAL_RT_DIMENSION_TEXTURECM,
        GAL_RT_DIMENSION_TEXTURE3D,
    };


    /**
     * Render target parameters for a buffer used as a render target
     */
    struct GAL_RT_BUFFER 
    {
        gal_uint elementOffset; ///< Number of bytes from the beginning of the buffer to the first element to be accessed
        gal_uint elementWidth; ///< Number of bytes in each element in the buffer
    };

    /**
     * Render target parameters for a texture 1D mipmap
     */
    struct GAL_RT_TEXTURE1D
    {
        gal_uint mipmap;
    };

    /**
     * Render target parameters for a texture 2D mipmap
     */
    struct GAL_RT_TEXTURE2D 
    { 
        gal_uint mipmap; 
    };

    /**
     * Render target parameters for a texture 3D mipmap
     */
    struct GAL_RT_TEXTURE3D 
    { 
        gal_uint mipmap;
        gal_uint depth;
    };

    /**
     * Descriptor to define how a render target is created
     */
    struct GAL_RT_DESC
    {
        GAL_FORMAT format;
        GAL_RT_DIMENSION dimension;
        union // Per dimension type informaton
        {
            GAL_RT_BUFFER buffer;
            GAL_RT_TEXTURE1D texture1D;
            GAL_RT_TEXTURE2D texture2D;
            GAL_RT_TEXTURE3D texture3D;
        };
    };

}





#endif // GAL_TYPES
