/**************************************************************************
 *
 * Texture Behavior Model class definition file.
 *
 */

/**
 *
 *  @file bmTexture.h
 *
 *  This file defines the Texture Behavior Model class.
 *
 *  This class defines functions that emulate the
 *  texture unit functionality of a GPU.
 *
 */

#ifndef _TEXTUREEMULATOR_

#define _TEXTUREEMULATOR_

#include "GPUType.h"
#include "GPUReg.h"
#include "support.h"
#include "DynamicMemoryOpt.h"
#include "PixelMapper.h"

namespace arch
{

//**  Defines the maximum texture lod bias.  
static const F32 MAX_TEXTURE_LOD_BIAS = 1000.0f;

/***  Defines the magnification to minification transition point.
      The first value is used when the magnification filter is LINEAR and
      the minification filter is GPU_NEAREST_MIPMAP_NEAREST or
      GPU_NEAREST_MIPMAP_LINEAR.  The second is used for all other filter
      modes.  */
static const F32 C1 = 0.5f;
static const F32 C2 = 0.0f;

/**
 *
 *  Defines the address space for uncompressed textures.
 *
 */
//#define UNCOMPRESSED_TEXTURE_SPACE 0x0000000000000000ULL
static const U64 UNCOMPRESSED_TEXTURE_SPACE = 0x0000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using DXT1 RGB mode (1:8 ratio).
 *
 */

//#define COMPRESSED_TEXTURE_SPACE_DXT1_RGB 0x1000000000000000ULL
static const U64 COMPRESSED_TEXTURE_SPACE_DXT1_RGB = 0x1000000000000000ULL;


/**
 *
 *  Defines the address space for compressed textures using DXT1 RGBA mode (1:8 ratio).
 *
 */

// #define COMPRESSED_TEXTURE_SPACE_DXT1_RGBA 0x2000000000000000ULL
static const U64 COMPRESSED_TEXTURE_SPACE_DXT1_RGBA = 0x2000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using DXT3 RGBA mode (1:4 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_DXT3_RGBA = 0x3000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using DXT5 RGBA mode (1:4 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_DXT5_RGBA = 0x4000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using LATC1 mode (1:2 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_LATC1 = 0x5000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using LATC1 signed mode (1:2 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_LATC1_SIGNED = 0x6000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using LATC2 mode (1:2 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_LATC2 = 0x7000000000000000ULL;

/**
 *
 *  Defines the address space for compressed textures using LATC2 signed mode (1:2 ratio).
 *
 */

static const U64 COMPRESSED_TEXTURE_SPACE_LATC2_SIGNED = 0x8000000000000000ULL;

/**
 *
 *  Defines the black texel address.
 *
 */
static const U64 BLACK_TEXEL_ADDRESS = 0x00ffffffffffffffULL;

/**
 *
 *  Defines the texture address space mask.
 *
 */

static const U64 TEXTURE_ADDRESS_SPACE_MASK = 0xff00000000000000ULL;

/**
 *
 *  Defines the size of a S3TC DXT1 compressed 4x4 block in bytes.
 *
 */

static const U32 S3TC_DXT1_BLOCK_SIZE_SHIFT = 3;

/**
 *
 *  Defines the size of a S3TC DXT3/DXT5 compressed 4x4 block in bytes.
 *
 */

static const U32 S3TC_DXT3_DXT5_BLOCK_SIZE_SHIFT = 4;

/**
 *
 *  Defines the size of a LATC1 compressed 4x4 block in bytes.
 *
 */
static const U32 LATC1_BLOCK_SIZE_SHIFT = 3;

/**
 *
 *  Defines the size of a LATC2 compressed 4x4 block in bytes.
 *
 */ 
static const U32 LATC2_BLOCK_SIZE_SHIFT = 4;
 
/**
 *
 *  Defines the shift to translate a GPU memory address to/from DXT1 RGB/RGBA address space.
 *
 */

static const U32 DXT1_SPACE_SHIFT = 3;

/**
 *
 *  Defines the shift to translate GPU memory address to/from DXT3/DXT5 RGBA address space.
 *
 */

static const U32 DXT3_DXT5_SPACE_SHIFT = 2;

/**
 *
 *  Defines the shift to translate a GPU memory address to/from LATC1/LATC2 RGBA address space.
 *
 */
 
static const U32 LATC1_LATC2_SPACE_SHIFT = 1;
 
/**
 *
 *  Defines the maximum amount of data required for an attribute.
 *
 */

static const U32 MAX_ATTRIBUTE_DATA = 16; 


/**
 *
 *  Defines the type of the operation requested to the Texture Unit.
 */
enum TextureOperation
{
    TEXTURE_READ,
    TEXTURE_READ_WITH_LOD,
    ATTRIBUTE_READ
};

/**
 *
 *  This class stores information for a fragment stamp (quad)
 *  texture access.
 *
 *  Inherits from the DynamicMemoryOpt class that offers
 *  cheap memory management.
 *
 */

class TextureAccess: public DynamicMemoryOpt
{
public:

    /**
     *
     *  Defines the main axis for anisotropic filtering.
     *
     */
    enum AnisotropyAxis
    {
        X_AXIS,      //  Horizontal axis.  
        Y_AXIS,      //  Vertical axis.  
        XY_AXIS,     //  Lineal combination.  
        YX_AXIS
    };

    /**
     *
     *  Defines a trilinear texture sample.
     *
     *      - for 1D: up to two texels.
     *      - for 2D: up to eight texels.
     *      - for 3D: up to sixteen texels.
     *
     */

    class Trilinear : public DynamicMemoryOpt
    {
        public:

        U32 i[STAMP_FRAGMENTS][16];          //  Texel i coordinate (in texels) for the trilinear sample.  
        U32 j[STAMP_FRAGMENTS][16];          //  Texel j coordinate (in texels) for the trilinear sample.  
        U32 k[STAMP_FRAGMENTS][16];          //  Texel k coordinate (in texels) for the trilinear sample.  
        F32 a[STAMP_FRAGMENTS][2];           //  Weight factors for horizontal texture dimension.  
        F32 b[STAMP_FRAGMENTS][2];           //  Weight factors for vertical texture dimension.  
        F32 c[STAMP_FRAGMENTS][2];           //  Weight factor for the depth texture dimension.  
        U64 address[STAMP_FRAGMENTS][16];    //  Memory address for each texel in the the trilinear sample.  
        U32 way[STAMP_FRAGMENTS][16];        //  Texture cache way for each texel in the trilinear sample.  
        U32 line[STAMP_FRAGMENTS][16];       //  Texture cache line for each texel the trilinear sample.  
        U64 tag[STAMP_FRAGMENTS][16];        //  Stores the tag for each texel in the trilinear sample.  
        bool fetched[STAMP_FRAGMENTS][16];      //  Flag storing if the texel has been fetched.  
        bool ready[STAMP_FRAGMENTS][16];        //  Flag storing if the texel data is available in the cache.  
        bool read[STAMP_FRAGMENTS][16];         //  Flag storing if the texel has been read.  
        bool sampleFromTwoMips[STAMP_FRAGMENTS];    //  Flag storing if two mips have to be sampled.  
        U32 loops[STAMP_FRAGMENTS];          //  Number of fetch/read/filter loops required to calculate the trilinear texture sample.  
        U32 texelsLoop[STAMP_FRAGMENTS];     //  Number of texels to fetch/read for each loop.  
        U32 fetchLoop[STAMP_FRAGMENTS];      //  Current fetch loop.  
        U32 readLoop[STAMP_FRAGMENTS];       //  Current read/filter loop.  
        Vec4FP32 texel[STAMP_FRAGMENTS][16];   //  Read texels for the trilinear sample.  
        Vec4FP32 sample[STAMP_FRAGMENTS];      //  Filtered texture sample for the trilinear sample.  
        U08 attrData[STAMP_FRAGMENTS][MAX_ATTRIBUTE_DATA];    //  Maximum size of an attribute read.  
        U32 attrFirstOffset[STAMP_FRAGMENTS];                //  Offset for the first attribute data byte in the first cache read.  
        U32 attrFirstSize[STAMP_FRAGMENTS];                  //  Bytes from the first cache read to store.  
        U32 attrSecondSize[STAMP_FRAGMENTS];                 //  Bytes from the first cache read to store.  
    };

    TextureOperation texOperation;          //  Type of texture operation to perform.  
    Vec4FP32 coordinates[STAMP_FRAGMENTS]; //  The fragment texture access coordinates.  
    Vec4FP32 originalCoord[STAMP_FRAGMENTS];   //  Stores the original fragment texture access coordinates.  
    F32 parameter[STAMP_FRAGMENTS];      //  Fragment parameter (lod/bias).  
    F32 reference[STAMP_FRAGMENTS];      //  Reference value for comparison filter (PCF).  
    U32 textUnit;                        //  Texture unit where the access is performed.  
    F32 lod[STAMP_FRAGMENTS];            //  Calculated LOD for each fragment.  
    CubeMapFace cubemap;                    //  Cubemap image to be accessed by the texture access.  
    U32 level[STAMP_FRAGMENTS][2];       //  Mipmaps for each fragment.  
    U32 anisoSamples;                    //  Number of anisotropic samples to take.  
    U32 currentAnisoSample;              //  Current anisotropic sample.  
    F32 anisodsOffset;                   //  Per anisotropic sample offset for texture coordinate s component.  
    F32 anisodtOffset;                   //  Per anisotropic sample offset for texture coordinate t component.  
    FilterMode filter[STAMP_FRAGMENTS];     //  Filter to apply to the sampled texels.  
    U32 magnified;                       //  Number of pixels in the quad that are magnified.  
    U32 accessID;                        //  Identifier of the texture access/Pointer to the entry in the shader behaviorModel texture queue.  
    U64 cycle;                           //  Stores the cycle when the texture access was created.  
    Vec4FP32 sample[STAMP_FRAGMENTS];      //  Final filtered texture sample.  
    Trilinear *trilinear[MAX_ANISOTROPY];   //  Stores the trilinear samples to take for the texture access.  
    U32 nextTrilinearFetch;              //  Next trilinear sample in the texture access for which to fetch texels.  
    U32 trilinearFiltered;               //  Number of trilinear samples already filtered.  
    U32 trilinearToFilter;               //  Number of trilinear samples remaining to be filtered.  
    bool addressCalculated;                 //  All the trilinear addresses have been calculated.  
    U32 texelSize[STAMP_FRAGMENTS];      //  Bytes per texel.  

    /**
     *
     *  Texture Access constructor.
     *
     *  @param texOp Type of texture operation to perform.
     *  @param coord Pointer to texture coordinates being accessed by the fragments in
     *  the stamp (quad).
     *  @param parameter Pointer to a parameter (lod/bias) of the fragments in the stamp (quad).
     *  @param textUnit Identifier of the texture unit being accessed by the texture
     *  access.
     *
     *  @return A new texture access object.
     *
     */

    TextureAccess(U32 id, TextureOperation texOp, Vec4FP32 *coord, F32 *parameter, U32 textUnit);

    /**
     *
     *  Texture access destructor.
     *
     */

    ~TextureAccess();

};

//  Defines the different anisotropy algorithm available in the behaviorModel
enum AnisotropyAlgorithm
{
    ANISO_TWO_AXIS    = 0,  //  Detect anisotropy on the X and Y screen axis.  
    ANISO_FOUR_AXIS   = 1,  //  Detect anisotropy on the X and Y and 45 degree rotated X and Y screen axis.  
    ANISO_RECTANGULAR = 2,  /**<  Detect anisotropy based on a rectangular approximation of
                                  the pixel projection on texture space along the X and Y and
                                  45 degree rotated X and Y screen axis.  */
    ANISO_EWA          = 3, //  Detect anisotropy using Heckbert's EWA algorithm.  
    ANISO_EXPERIMENTAL = 4  //  Experimental anisotropy algorithm.  
};

/**
 *
 *  This class implements emulation functions that implement
 *  the texture unit in a GPU.
 *
 */


class bmoTextureProcessor
{

private:

    //  Parameters.
    U32 stampFragments;      //  Fragments per stamp.  
    U32 textCacheBlockDim;   //  Dimension of a texture cache block.  Dimension is the n exponent in a block with a size of 2^n x 2^n texels.  
    U32 textCacheBlockSize;  //  Texels in a block (derived from textCacheBlockDim).  
    U32 textCacheSBlockDim;  //  Dimension of a texture cache superblock (in blocks).  Dimension is the n exponent in a block with the size of 2^n x 2^n blocks.  
    U32 textCacheSBlockSize; //  Size in blocks of a texture cache superblock.  
    U32 anisoAlgorithm;      //  Selected anisotropy algorithm.  
    bool forceMaxAnisotropy;    //  Forces the anisotropy of all textures to the maximum anisotropy from the configuration file.  
    U32 confMaxAniso;        //  Maximum anisotropy allowed by the configuration file.  
    U32 trilinearPrecision;  //  Bits of precision for the fractional part of the trilinear LOD.  
    U32 brilinearThreshold;  //  Threshold used to decide when to enable trilinear (sample from two mipmaps).  Based on trilinear precision.  
    U32 anisoRoundPrecision; //  Bits of precision for the fractional part of the aniso ratio.  Used to compute rounding.  
    U32 anisoRoundThreshold; //  Threshold used to round the computed aniso ratio to the next supported integral aniso ratio.  
    bool anisoRatioMultOfTwo;   //  Flag that selects if the final aniso ratio must be a multiple of 2.  
    U32 overScanTileWidth;   //  Over scan tile width in scan tiles.  
    U32 overScanTileHeight;  //  Over scan tile height in scan tiles.  
    U32 scanTileWidth;       //  Scan tile width in generation tiles.  
    U32 scanTileHeight;      //  Scan tile height in generation tiles.  
    U32 genTileWidth;        //  Scan tile width in pixels/texels.  
    U32 genTileHeight;       //  Scan tile height in pixels/texels.  

    //  Precomputed constants.
    F32 trilinearRangeMin;   //  Minimum LOD fractional value at which trilinear (sampling from two mipmaps) is triggered.  
    F32 trilinearRangeMax;   //  Maximum LOD fractional value at which trilinear (sampling from two mipmaps) is triggered.  
    F32 anisoRoundUp;        //  Point at which the next valid number of aniso samples is used.      
    
    //  GPU Texture Unit registers.  
    bool textureEnabled[MAX_TEXTURES];      //  Texture unit enable flag.  
    TextureMode textureMode[MAX_TEXTURES];  //  Current texture mode active in the texture unit.  
    U32 textureAddress[MAX_TEXTURES][MAX_TEXTURE_SIZE][CUBEMAP_IMAGES];  //  Address in GPU memory of the active texture mipmaps.  
    U32 textureWidth[MAX_TEXTURES];      //  Active texture width in texels.  
    U32 textureHeight[MAX_TEXTURES];     //  Active texture height in texels.  
    U32 textureDepth[MAX_TEXTURES];      //  Active texture depth in texels.  
    U32 textureWidth2[MAX_TEXTURES];     //  Log2 of the texture width (base mipmap size).  
    U32 textureHeight2[MAX_TEXTURES];    //  Log2 of the texture height (base mipmap size).  
    U32 textureDepth2[MAX_TEXTURES];     //  Log2 of the texture depth (base mipmap size).  
    U32 textureBorder[MAX_TEXTURES];     //  Texture border in texels.  
    TextureFormat textureFormat[MAX_TEXTURES];  //  Texture format of the active texture.  
    bool textureReverse[MAX_TEXTURES];          //  Reverses texture data (from little to big endian).  
    bool textD3D9ColorConv[MAX_TEXTURES];       //  Sets the color component order to the order defined by D3D9.  
    bool textD3D9VInvert[MAX_TEXTURES];         //  Forces the u coordinate to be inverted as specified by D3D9.  
    TextureCompression textureCompr[MAX_TEXTURES];  //  Texture compression mode of the active texture.  
    TextureBlocking textureBlocking[MAX_TEXTURES];  //  Texture blocking mode for the texture.  
    Vec4FP32 textBorderColor[MAX_TEXTURES];    //  Texture border color.  
    ClampMode textureWrapS[MAX_TEXTURES];       //  Texture wrap mode for s coordinate.  
    ClampMode textureWrapT[MAX_TEXTURES];       //  Texture wrap mode for t coordinate.  
    ClampMode textureWrapR[MAX_TEXTURES];       //  Texture wrap mode for r coordinate.  
    bool textureNonNormalized[MAX_TEXTURES];    //  Texture coordinates are non-normalized.  
    FilterMode textureMinFilter[MAX_TEXTURES];  //  Texture minification filter.  
    FilterMode textureMagFilter[MAX_TEXTURES];  //  Texture Magnification filter.  
    bool textureEnableComparison[MAX_TEXTURES]; //  Texture Enable Comparison filter (PCF).  
    CompareMode textureComparisonFunction[MAX_TEXTURES];  //  Texture Comparison function (PCF).  
    bool textureSRGB[MAX_TEXTURES];             //  Texture sRGB space to linear space conversion.  
    F32 textureMinLOD[MAX_TEXTURES];         //  Texture minimum lod.  
    F32 textureMaxLOD[MAX_TEXTURES];         //  Texture maximum lod.  
    F32 textureLODBias[MAX_TEXTURES];        //  Texture lod bias.  
    U32 textureMinLevel[MAX_TEXTURES];       //  Texture minimum mipmap level.  
    U32 textureMaxLevel[MAX_TEXTURES];       //  Texture maximum mipmap level.  
    F32 textureUnitLODBias[MAX_TEXTURES];    //  Texture unit lod bias (not texture lod!!).  
    U32 maxAnisotropy[MAX_TEXTURES];         //  Maximum anisotropy for the texture.  

    //  Pixel mapper (for framebuffer textures).
    PixelMapper texPixelMapper[MAX_TEXTURES];   //  Maps texels/pixels to addresses.  
    bool pixelMapperConfigured[MAX_TEXTURES];   //  Stores if the corresponding pixel mapper has been properly configured.  
    
    //  Stream input registers.
    U32 attributeMap[MAX_VERTEX_ATTRIBUTES];     //  Mapping from vertex input attributes and vertex streams.  
    Vec4FP32 attrDefValue[MAX_VERTEX_ATTRIBUTES];  //  Defines the vertex attribute default values.  
    U32 streamAddress[MAX_STREAM_BUFFERS];       //  Address in GPU memory for the vertex stream buffers.  
    U32 streamStride[MAX_STREAM_BUFFERS];        //  Stride for the stream buffers.  
    StreamData streamData[MAX_STREAM_BUFFERS];      //  Data type for the stream buffer.  
    U32 streamElements[MAX_STREAM_BUFFERS];      //  Number of stream data elements (vectors) per stream buffer entry.  
    bool d3d9ColorStream[MAX_STREAM_BUFFERS];       //  Read components of the color attributes in the order defined by D3D9.  

    //  Derived from the registers.
    U32 streamDataSize[MAX_STREAM_BUFFERS];      //  Bytes per stream read.  
    U32 streamElementSize[MAX_STREAM_BUFFERS];   //  Bytes per stream element.  
    
    
    //  Private functions.  

    /**
     *
     *  Expands the texel coordinates for a bilinear texture access from the first
     *  texel coordinates.
     *
     *  @param textUnit The texture unit where the bilinear access is being done.
     *  @param textAccess A reference to a TextureAccess object describing a stamp
     *  of fragments performing a texture access.  The expanded texel coordinates
     *  are stored for a fragment and a mipmap level inside this object.
     *  @param trilinearAccess The trilinear access for which to expand the texel coordinates.
     *  @param frag The fragment for which to expand the texel coordinates.
     *  @param level The mipmap level for the bilinear access.
     *  @param i Horizontal texel coordinate.
     *  @param j Vertical texel coordinate.
     *  @param k Depth texel coordinate.
     *  @param l Identifies one of two bilinear access for trilinear.
     *
     */

    void genBilinearTexels(U32 textUnit, TextureAccess &textAccess, U32 trilinearAccess,
        U32 frag, U32 level, U32 i, U32 j, U32 k, U32 l);

    /**
     *
     *  Calculates the derivatives in x and y for fragment stamp.
     *
     *  @param coordinates Pointer to the texture coordinates of the fragments in the stamp
     *  accessing the texture unit.
     *  @param textUnit Texture unit/stage that is being accessed.
     *  @param dudx Reference to a float variable where to store the derivative.
     *  @param dudy Reference to a float variable where to store the derivative.
     *  @param dvdx Reference to a float variable where to store the derivative.
     *  @param dvdy Reference to a float variable where to store the derivative.
     *  @param dwdx Reference to a float variable where to store the derivative.
     *  @param dwdy Reference to a float variable where to store the derivative.
     *
     */
    void  derivativesXY(Vec4FP32 *coordinates, U32 textUnit, F32 &dudx, F32 &dudy,
        F32 &dvdx, F32 &dvdy, F32 &dwdx, F32 &dwdy);

    /**
     *
     *  Calculates the level of detail scale factor for GPU_TEXTURE1D textures.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 calculateScale1D(F32 dudx, F32 dudy);

    /**
     *
     *  Calculates the level of detail scale factor for GPU_TEXTURE2D textures.
     *
     *  @param textUnit Texture unit being accessed.
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 calculateScale2D(U32 textUnit, F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Computes the number of anisotropic samples based on the anisotropic ratio.
     *
     *  @param anisoRatio The compute anisotropic ratio.
     *
     *  @return The number of aniso samples to process based on the aniso rounding parameters.
     *
     */
     
    U32 computeAnisoSamples(F32 anisoRatio);

    /**
     *
     *  Computes anisotropy for the texture access based on the projection of the pixel
     *  over the texture area in terms of the screen aligned X and Y axis.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 anisoTwoAxis(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Computes anisotropy for the texture access based on the projection of the pixel
     *  over the texture area in terms of the screen aligned X and Y axis and the 45 degree
     *  rotation of the screen X and Y axis.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 anisoFourAxis(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Computes anisotropy for the texture access based on a rectangular approximation of
     *  the projection of a pixel over the texture.  The screen X and Y axis and the 45 degree
     *  rotated screen X and Y axis are used to select the directing/major edge of the rectangle.
     *
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 anisoRectangle(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Computes anisotropy for the texture access based on Heckbert's EWA algorithm.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 anisoEWA(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Computes anisotropy for the texture access based on an experimental algorithm that takes into account
     *  the angle between the X and Y axis.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param maxAniso Maximum anisotropy supported for the texture.
     *  @param anisoSamples Reference to a variable where to store the number of anisotropic samples
     *  to take.
     *  @param dsOffset Increment offset for texture coordinate s component per anisotropic sample in the
     *  anisotropy axis direction.
     *  @param dtOffsett Increment offset for texture coordinate t component per anisotropic sample in the
     *  anisotropy axis direction.
     *
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 anisoExperimentalAngle(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, U32 maxAniso,
        U32 &anisoSamples, F32 &dsOffset, F32 &dtOffset);

    /**
     *
     *  Calculates the level of detail scale factor for GPU_TEXTURE3D textures.
     *
     *  @param dudx Derivative on x.
     *  @param dudy Derivative on y.
     *  @param dvdx Derivative on x.
     *  @param dvdy Derivative on y.
     *  @param dwdx Derivative on x.
     *  @param dwdy Derivative on y.
     *
     *  @return The scale factor for the level of detail equation.
     *
     */

    F32 calculateScale3D(F32 dudx, F32 dudy, F32 dvdx, F32 dvdy, F32 dwdx, F32 dwdy);

    /**
     *
     *  Calculates, biases and clamps the lod for a fragment.
     *
     *  @param textUnit The texture unit being accessed by the fragment.
     *  @param fragmentBias The fragment lod bias.
     *  @param scaleFactor The scale factor calculated from the derivatives.
     *
     *  @return The clamped lod for the fragment.
     *
     */

    F32 calculateLOD(U32 textUnit, F32 fragmentBias, F32 scaleFactor);

    /**
     *
     *  Calculates texel coordinates for the first texel and weight factors for the
     *  filter for a fragment using accessing a texture unit at coordinates {s, t, r}.
     *
     *  @param textUnit The texture unit that is being accessed by the fragment.
     *  @param l The mipmap level where the access is performed.
     *  @param filter The filter that is going to be used at the mipmap level
     *  (either GPU_LINEAR or GPU_NEAREST).
     *  @param currentSample Current anisotropic sample for which to calculate the texture coordinates.
     *  @param samples Number of total anisotropic samples to take for the texture access.
     *  @param dsOffset Texture coordinate component s offset in the anisotropy axis direction per anisotropic sample.
     *  @param dtOffset Texture coordinate component t offset in the anisotropy axis direction per anisotropic sample.
     *  @param s Fragment s texture coordinate for the access.
     *  @param t Fragment t texture coordinate for the access.
     *  @param r Fragment r texture coordinate for the access.
     *  @param i Reference to an integer variable where to store the texel horizontal coordinate.
     *  @param j Reference to an integer variable where to store the texel vertical coordinate.
     *  @param k Reference to an integer variable where to store the texel depth coordinate.
     *  @param a Reference to a float point variable where to store the horizontal weight factor for the linear filter.
     *  @param b Reference to a float point variable where to store the vertical weight factor for the linear filter.
     *  @param c Reference to a float point variable where to store the depth weight factor for the linear filter.
     *
     */

    void texelCoord(U32 textUnit, U32 l, FilterMode filter, U32 currentSample, U32 samples,
        F32 dsOffset, F32 dtOffset, F32 s, F32 t, F32 r,
        U32 &i, U32 &j, U32 &k, F32 &a, F32 &b, F32 &c);

    /**
     *
     *  Calculates the first texel cooordinates for a mipmap level of a GPU_TEXTURE1D texture
     *  being accesed by a fragment.
     *
     *  @param textUnit The texture unit being accssed.
     *  @param l The mipmap level being accessed.
     *  @param filter The filter being used at the mipmap level (GPU_LINEAR or GPU_NEAREST).
     *  @param s Fragment s texture coordinate for the access.
     *  @param i Reference an integer variable where to store the texel horizontal coordinate.
     *  @param a Reference to a float point variable where to store the vertical weight factor for the linear filter.
     *
     */

    void texelCoord1D(U32 textUnit, U32 l, FilterMode filter, F32 s, U32 &i, F32 &a);

    /**
     *
     *  Calculates the first texel cooordinates for a mipmap level of a GPU_TEXTURE2D texture
     *  being accesed by a fragment.
     *
     *  @param textUnit The texture unit being accssed.
     *  @param l The mipmap level being accessed.
     *  @param filter The filter being used at the mipmap level (GPU_LINEAR or GPU_NEAREST).
     *  @param currentSample Current anisotropic sample for which to calculate the texture coordinates.
     *  @param samples Number of total anisotropic samples to take for the texture access.
     *  @param dsOffset Texture coordinate component s offset in the anisotropy axis direction per anisotropic sample.
     *  @param dtOffset Texture coordinate component t offset in the anisotropy axis direction per anisotropic sample.
     *  @param s Fragment s texture coordinate for the access.
     *  @param t Fragment t texture coordinate for the access.
     *  @param i Reference an integer variable where to store the texel horizontal coordinate.
     *  @param j Reference an integer variable where to store the texel vertical coordinate.
     *  @param a Reference to a float point variable where to store the horizontal weight factor for the linear filter.
     *  @param b Reference to a float point variable where to store the vertical weight factor for the linear filter.
     *
     */

    void texelCoord2D(U32 textUnit, U32 l, FilterMode filter, U32 currentSample, U32 samples,
        F32 dsOffset, F32 dtOffset, F32 s, F32 t, U32 &i, U32 &j, F32 &a, F32 &b);

    /**
     *
     *  Calculates the first texel cooordinates for a mipmap level of a GPU_TEXTURE3D texture
     *  being accesed by a fragment.
     *
     *  @param textUnit The texture unit being accssed.
     *  @param l The mipmap level being accessed.
     *  @param filter The filter being used at the mipmap level (GPU_LINEAR or GPU_NEAREST).
     *  @param s Fragment s texture coordinate for the access.
     *  @param t Fragment t texture coordinate for the access.
     *  @param r Fragment r texture coordinate for the access.
     *  @param i Reference an integer variable where to store the texel horizontal coordinate.
     *  @param j Reference an integer variable where to store the texel vertical coordinate.
     *  @param k Reference an integer variable where to store the texel depth coordinate.
     *  @param a Reference to a float point variable where to store the horizontal weight factor for the linear filter.
     *  @param b Reference to a float point variable where to store the vertical weight factor for the linear filter.
     *  @param c Reference to a float point variable where to store the vertical depth factor for the linear filter.
     *
     */

    void texelCoord3D(U32 textUnit, U32 l, FilterMode filter,
        F32 s, F32 t, F32 r, U32 &i, U32 &j, U32 &k, F32 &a, F32 &b, F32 &c);

    /**
     *
     *  Applies a texture wrapping mode to a texture coordinate component.
     *
     *  @param wrapMode Wrapping mode to be applied to the texture coordinate component.
     *  @param a The texture coordinate componet being wrapped.
     *  @param size Size in texels of the mipmap level being accessed.
     *
     *  @return The wrapped texture coordinate.
     *
     */

    F32 applyWrap(ClampMode wrapMode, F32 a, U32 size);

    /**
     *
     *  Applies a texture wrapping mode to a texel coordinate.
     *
     *  @param wrapMode Wrapping mode to be applied to the texel coordinate.
     *  @param i The texel coordinate component being wrapped.
     *  @param size Size in texels of the mipmap level being accessed.
     *
     *  @return The wrapped texel coordinate.
     *
     */

    U32 applyWrap(ClampMode wrapMode, U32 i, U32 size);

    /**
     *
     *  Performs a bilinear filtering at a mipmap level for a fragment in a stamp texture access.
     *
     *  @param textAccess The stamp texture access for which a bilinear sample must be calculated.
     *  @param trilinearAccess The trilinear access for which a bilinear sample must be calculated.
     *  @param level Which mipmap level (for trilinear) is being sampled.
     *  @param frag The fragment inside the stamp for which the bilinear sample must be calculated.
     *
     *  @return The filtered value.
     *
     */

    Vec4FP32 bilinearFilter(TextureAccess &textAccess, U32 trilinearAccess, U32 level, U32 frag);

    /**
     *
     *  Adjust a texel address to the size and type of the texture format.
     *
     *  @param textUnit Identifier of the texture unit being addressed.
     *  @param texelAddress The linear address of the texel inside the texture.
     *
     *  @return The modified address of the texel taking into account the size,
     *  components and type of the texture format.
     *
     */

    U64 adjustToFormat(U32 textUnit, U64 texelAddres);

    /**
     *
     *  Calculates the size (as 2^n exponent) of the texture mipmap level dimension.
     *
     *  @param topSize Logarithm of 2 of the largest mipmap level for the texture in
     *  a given dimension.
     *  @param level The mipmap level for which we want to calculate the logarithm of 2 of
     *  the dimension size.
     *
     *  @return The logarithm of 2 of the size of a given dimension of the mipmap level.
     *
     */

    U32 mipmapSize(U32 topSize, U32 level);

    /**
     *
     *  Translates a texture texel address to a physical texture memory address for 1D textures.
     *
     *  @param textUnit The texture unit that is being accessed.
     *  @param level The mipmap level that is being accessed.
     *  @param i The texel coordinate inside the mipmap level of the linear 1D accessed texture.
     *
     *  @return The memory address of the addressed texel.
     *
     */

    U64 texel2address(U32 textUnit, U32 level, U32 i);

    /**
     *
     *  Translates a texture texel address to a physical texture memory address for 2D textures.
     *
     *  @param textUnit The texture unit of the texture being accessed.
     *  @param level The mipmap level of the texture being accessed.
     *  @param cubemap Cubemap image index for the texel.
     *  @param i The horizontal coordinate of the texel inside the 2D mipmap level.
     *  @param j The vertical coordinate of the texel inside the 2D mipmap level.
     *
     *  @return The memory address of the texel.
     *
     */

    U64 texel2address(U32 textUnit, U32 level, U32 cubemap, U32 i, U32 j);

    /**
     *
     *  Translates a texture texel address to a physical texture memory address for 3D textures.
     *
     *  @param textUnit The texture unit of the texture being accessed.
     *  @param level The mipmap level of the texture being accessed.
     *  @param cubemap Cubemap image index for the texel.
     *  @param i The horizontal coordinate of the texel inside the 3D mipmap level.
     *  @param j The vertical coordinate of the texel inside the 3D mipmap level.
     *  @param k The depth coordinate of the texel inside the 3D mipmap level.
     *
     *  @return The memory address of the texel.
     *
     */

    U64 texel2address(U32 textUnit, U32 level, U32 cubemap, U32 i, U32 j, U32 k);

    /**
     *
     *  Translates a attribute stream index to a serie of physical memory addresses.
     *
     *  @param attribute The attribute stream to read.
     *  @param textAccess Texture Access where to store the addresses to read.
     *  @param trilinearAccess Trilinear element inside the Texture Access object where to store the addresses.
     *  @param frag The element in the 4-group for which the addresses are computed.
     *  @param index The stream index used to read from the atttribute stream.
     *
     */

    void index2address(U32 attribute, TextureAccess &textAccess, U32 trilinearAccess, U32 frag, U32 index);

    /**
     *
     *  Converts the data read for an attribute into the attribute final format (SIMD4 float32) and fills
     *  the elements of the attribute vector with valid or default data based on the attribute/stream
     *  register settings.
     *
     *  @param texAccess Reference to the Texture Access object where to store the attribute data.
     *  @param trilinearAccess Identifier of the TrilinearAccess object inside the TextureAccess object
     *  where to store the attribute data.
     *  @param frag The element in the 4-group for which the attribute data is being converted/filled.
     *
     */
     
    void loadAttribute(TextureAccess &texAccess, U32 trilinearAccess, U32 frag);

    /**
     *
     *  Converts data read for an attribute element to the attribute final format (float32).
     *
     *  @param format Format of the attribute.
     *  @param data Pointer to the data read for the attribute element.
     *
     *  @result The attribute element value converted to float32.
     *
     */
     
    F32 attributeDataConvert(StreamData format, U08 *data);

    /**
     *
     *  Selects the face of a cubemap being accessed by a set of texture coordinates for
     *  a stamp and converts those coordinates into coordinates to the selected 2D texture
     *  image.
     *
     *  @param coord Vec4FP32 array with the texture coordinates of the texture access to
     *  a cubemap for a stamp.  The coordinates are recalculated to address the selected
     *  2D texture image.
     *
     *  @return The identifier of the cubemap face selected.
     *
     */

    CubeMapFace selectCubeMapFace(Vec4FP32 *coord);

    /**
     *
     *  Converts a Vec4FP32 color into a given format.
     *
     *  @param format Texture format to which the input Vec4FP32 color is to be converted.
     *  @param color Vec4FP32 source color.
     *
     *  @return A 32 bit unsigned integer that stores the color converted into the destination
     *  format.
     *
     */

    static U32 format(TextureFormat format, Vec4FP32 color);

    /**
     *
     *  Decodes and selects the proper alpha value for S3TC alpha encoding.
     *
     *  @param code The 3 bit alpha code for the texel.
     *  @param alpha0 First reference alpha value.
     *  @param alpha1 Second reference alpha value.
     *
     *  @return The decoded alpha value.
     *
     */

    static F32 decodeS3TCAlpha(U32 code, F32 alpha0, F32 alpha1);


    /**
     *
     *  Decodes and selects the proper color for non-transparent S3TC color encoding.
     *
     *  @param code The 2 bit color code for the texel.
     *  @param RGB0 The first reference color.
     *  @param RGB1 The second reference color.
     *  @param output A reference to the Vec4FP32 variable where to store the decoded
     *  color.
     *
     */

    static void nonTransparentS3TCRGB(U32 code, Vec4FP32 RGB0, Vec4FP32 RGB1, Vec4FP32 &output);

    /**
     *
     *  Decodes and selects the proper color for transparent ST3C color encoding.
     *
     *  @param code The 2 bit color code for the texel.
     *  @param RGB0 The first reference color.
     *  @param RGB1 The second reference color.
     *  @param output A reference to the Vec4FP32 variable where to store the decoded
     *  color.
     *
     */

    static void transparentS3TCRGB(U32 code, Vec4FP32 RGBA0, Vec4FP32 RGBA1, Vec4FP32 &output);


public:

    /**
     *
     *  Texture Behavior Model constructor.
     *
     *  @param stampFrags Number of fragments per stamp.
     *  @param blockDim Dimension of a texture cache block (the exponent in a block of 2^n x 2^n texels).
     *  @param superBlockDim Dimension of a texture cache super block (the exponent in a superblock of 2^n x 2^n blocks).
     *  @param anisoAlgo Selects the anisotropy algorithm to be used by the Texture Behavior Model.
     *  @param forceAniso Flag used to force the maximum anisotropy from the configuration file to all textures.
     *  @param maxAniso Maximum anisotropy defined in the configuration file.
     *  @param triPrecision Bits of precision of the fractional part of the LOD used to decide when to trigger trilinear.
     *  @param brilinearThreshold Threshold, based on the trilinear precision, used to trigger trilinear (sampling from two mipmaps).
     *  @param anisoRoundPrecision Bits of precision of the fractional part of the aniso ratio used to round to the final integer
     *  number of samples to use for anisotropic filtering.
     *  @param anisoRoundThreshold Threshold, based on the trilinear precision, used to round to the next valid integer number of
     *  samples to use for anisotropic filtering.
     *  @param anisoRatioMultOfTwo Flag that selects if the number of samples for anisotropic filtering must be a multiple of two.
     *  @param overScanWidth Width of a frame buffer over scan tile (in scan tiles).
     *  @param overScanHeight Height of a frame buffer over scan tile (in scan tiles).
     *  @param scanWidth Width of a frame buffer scan tile (in pixels).
     *  @param scanHeight Height of a frame buffer scan tile (in pixels).
     *  @param genWidth Width of a frame buffer generation tile (in pixels).
     *  @param genHeight Height of a frame buffer generation tile (in pixels).     
     *
     *  @return A new texture behaviorModel object.
     *
     */

    bmoTextureProcessor(U32 stampFrags, U32 blockDim, U32 superBlockDim, U32 anisoAlgo,
                    bool forceAniso, U32 maxAniso, U32 triPrecision, U32 brilinearThreshold,
                    U32 anisoRoundPrecision, U32 anisoRoundThreshold, bool anisoRatioMultOfTwo,
                    U32 overScanWidth, U32 overScanHeight, U32 scanWidth, U32 scanHeight,
                    U32 genWidth, U32 genHeight);

    /**
     *
     *  Calculates the texel address for a stamp.
     *
     *  @param id Identifier of the texture access in the Shader Behavior Model.
     *  @param texOp Operation requested to the texture behaviorModel.
     *  @param stampCoord  Pointer to the array of texture coordinates generated by the
     *  fragments in the stamp accessing the texture unit.
     *  @param stampParameter Per fragment (in the stamp) parameter (lod/bias).
     *  @param textUnit Texture unit that is being accessed by the fragments in the stamp.
     *
     *  @return A TextureAccess object that describes the current texture access for
     *  for the given fragment stamp and texture unit.
     *
     */

    TextureAccess *textureOperation(U32 id, TextureOperation texOp, Vec4FP32 *stampCoord, F32 *stampParameter, U32 textUnit);

    /**
     *
     *  Calculates the addresses for all the texels in the texture access.
     *
     *  @param textAccess Texture access for which to calculate the texel addresses.
     *
     */

    void calculateAddress(TextureAccess *textAccess);

    /**
     *  Filters the texels read for a stamp texture access and generates the final sample
     *  value.
     *
     *  @param textAccess Reference to the stamp texture access that is going to be
     *  filtered.
     *  @param nextTrilinear Trilinear sample to filter.
     *
     */

    void filter(TextureAccess &textAccess, U32 nextTrilinear);

    /**
     *  Filters the texels read for a trilinear and generates the final sample
     *  value.
     *
     *  @param textAccess Reference to the stamp texture access that is going to be filtered.
     *  @param trilinearAccess Trilinear access in the texture access that is to be filtered.
     *
     */

    void filterTrilinear(TextureAccess &textAccess, U32 trilinearAccess);

    /**
     *  Converts and stores texel data from their native source to float point format.
     *
     *  @param textAccess Reference to a texture access object.
     *  @param trilinearAccess Trilinear access in the texture access to be converted.
     *  @param frag The fragment in the texture access.
     *  @param tex  The texel in the texture access.
     *  @param data Pointer to the texel data.
     *
     */

    void convertFormat(TextureAccess &textAccess, U32 trilinearAccess, U32 frag, U32 tex, U08 *data);


    /**
     *
     *  Compares the texel value with the reference value using the defined comparison function
     *  to implement PCF (Percentage Close Filtering).
     *  Returns 0.0f (false) or 1.0f (true) based on the result of the comparison.
     *
     *  @param function The comparison function to use.
     *  @param reference The reference value to compare the texel value with.
     *  @param texel The texel value.
     *
     *  @return If the result of the comparison is TRUE return 1.0f, otherwise return 0.0f.
     *
     */
     
    F32 comparisonFilter(CompareMode function, F32 reference, F32 texel);

    /**
     *
     *  Decodes a S3TC compressed 4x4 block using DXT1 encoding for RGB format.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockDXT1RGB(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a S3TC compressed 4x4 block using DXT1 encoding for RGBA format.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockDXT1RGBA(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a S3TC compressed 4x4 block using DXT3 encoding for RGBA format.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockDXT3RGBA(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a S3TC compressed 4x4 block using DXT5 encoding for RGBA format.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockDXT5RGBA(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a LATC1 compressed 4x4 block.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockLATC1(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a LATC1_SIGNED compressed 4x4 block.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockLATC1Signed(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a LATC2 compressed 4x4 block.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockLATC2(U08 *inBuffer, U08 *outBuffer);

    /**
     *
     *  Decodes a LATC2_SIGNED compressed 4x4 block.
     *  The output block texels are stored in Morton order.
     *
     *  @param inBuffer Pointer to an array of bytes where the compressed input data
     *  is stored.
     *  @param outBuffer Pointer to an array of bytes where the decompressed block
     *  is to be stored.
     *
     */

    static void decodeBlockLATC2Signed(U08 *inBuffer, U08 *outBuffer);
    
    /**
     *
     *  Decompresses a number of S3TC DXT1 RGB compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressDXT1RGB(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of S3TC DXT1 RGBA compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressDXT1RGBA(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of S3TC DXT3 RGBA compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressDXT3RGBA(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of S3TC DXT5 RGBA compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressDXT5RGBA(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of LATC1 compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressLATC1(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of LATC1_SIGNED compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressLATC1Signed(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of LATC2 compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressLATC2(U08 *input, U08 *output, U32 size);

    /**
     *
     *  Decompresses a number of LATC2_SIGNED compressed blocks.
     *
     *  @param input Pointer to an input byte array with the compressed data.
     *  @param output Pointer to an output byte array where to store the uncompressed data.
     *  @param size Size of the input data.
     *
     */

    static void decompressLATC2Signed(U08 *input, U08 *output, U32 size);

    
    
    /**
     *
     *  Reset the Texture Behavior Model internal state.
     *
     */

    void reset();

    /**
     *
     *  Writes a texture behaviorModel register.
     *
     *  @param reg Texture Behavior Model register identifier.
     *  @param subreg Texture Behavior Model register subregister.
     *  @param data Data to write into the Texture Behavior Model register.
     *
     */

    void writeRegister(GPURegister reg, U32 subReg, GPURegData data);

};

} // namespace arch

#endif
