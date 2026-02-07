/**************************************************************************
 *
 * Map pixels to addresses and processing units.
 *
 */

/**
 *
 * @file PixelMapper.h
 *
 * Defines a class used to map pixels to memory addresses or processing units 
 * applying tiling and different distribution algorithms.
 *
 */

#include "GPUType.h"
#include "GPUReg.h"

#ifndef _PIXELMAPPER_

#define _PIXELMAPPER_

namespace cg1gpu
{

/**
 *
 *  Defines a class to map pixels to memory addresses and processing units.
 *
 */
class PixelMapper
{
private:

    //  Display parameters.
    
    U32 hRes;            //  Display horizontal resolution.  
    U32 vRes;            //  Display vertical resolution.  
    U32 samples;         //  Samples per pixel.  
    U32 bytesSample;     //  Bytes per sample.   
    
    //  Tiling parameters.
    U32 overTileWidth;   //  Over scan tile width in scan tiles.  
    U32 overTileHeight;  //  Over scan tile height in scan tiles.  
    U32 scanTileWidth;   //  Scan tile width in generation tiles.  
    U32 scanTileHeight;  //  Scan tile height in generation tiles.  
    U32 genTileWidth;    //  Generation tile width in stamp tiles.  
    U32 genTileHeight;   //  Generation tile height in stamp tiles.  
    U32 stampTileWidth;  //  Stamp tile width in pixels.  
    U32 stampTileHeight; //  Stamp tile height in pixels.  
    
    //  Processing units parameters.
    
    U32 numUnits;            //  Number of processing units to which pixels are mapped.  
    U32 memUnitInterleaving; //  Memory interleaving between processing units.  
    
    //  Precomputed parameters.
    U32 overTilePixelWidth;  //  Width of the over scan tile in pixels.  
    U32 overTilePixelHeight; //  Height of the over scan tile in pixels.  
    U32 overTileRowWidth;    //  Number of over scan tiles per over scan tile row.   
    U32 overTileRows;        //  Number of over scan tile rows.  
    U32 overTileSize;        //  Size of the over scan tile in scan tiles.  
    U32 overTilePixelSize;   //  Size of the over scan tile in pixels.  
    U32 scanTilePixelWidth;  //  Width of the scan tile in pixels.  
    U32 scanTilePixelHeight; //  Height of the scan tile in pixels.      
    U32 scanTileSize;        //  Size of the scan tile in generation tiles.  
    U32 scanTilePixelSize;   //  Size of the scan tile in pixels.  
    U32 scanSubTileWidth;    //  Scan sub tile width in adjusted generation tiles.  
    U32 scanSubTileHeight;   //  Scan sub tile height in adjusted generation tiles.  
    U32 scanSubTileSize;     //  Size of the scan sub tile in generation tiles.  
    U32 scanSubTilePixelWidth;   //  Scan sub tile width in pixels.  
    U32 scanSubTilePixelHeight;  //  Scan sub tile heigh in pixels.  
    U32 scanSubTileBytes;        //  Bytes per scan sub tile.  
    U32 genAdjTileWidth;         //  Adjusted generation tile width in stamp tiles.  
    U32 genAdjTileHeight;        //  Adjusted generation tile height in stamp tiles.  
    U32 genAdjTileSize;          //  Adjusted generation tile size in stamp tiles.  
    U32 genAdjTilePixelWidth;    //  Adjusted generation tile width in pixels.  
    U32 genAdjTilePixelHeight;   //  Adjusted generation tile height in pixels.  
    U32 genAdjTilePixelSize;     //  Adjusted generation tile pixel size.  
    U32 stampTileSize;           //  Stamp tile size in pixels.  
    
    //  Precomputed Morton table.
    U08 mortonTable[256];         //  Precomputed table to be used for Morton order mapping.  
    
    
    //  Private functions.
    
    
    /**
     *
     *  Build the precomputed Morton table to be used for fast Morton order mapping.
     *
     */
     
    void buildMortonTable();
    
    /**
     *
     *  Maps an element inside a square power of two tile (2^size x 2^size) using Morton order.
     *  This function implements a fast version of the Morton mapping algorithm using the
     *  precomputed Morton table.
     *
     *  @param size Log2(dimension of tile).
     *  @param i Horizontal position of the element inside the tile.
     *  @param j Vertical position of the element inside the tile.
     *
     *  @return The position of the element in Morton order.
     *
     */
     
    U32 fastMapMorton(U32 size, U32 i, U32 j);
     
    /**
     *
     *  Maps an element inside a square power of two tile (2^size x 2^size) using Morton order.
     *
     *  @param size Log2(dimension of tile).
     *  @param i Horizontal position of the element inside the tile.
     *  @param j Vertical position of the element inside the tile.
     *
     *  @return The position of the element in Morton order.
     *
     */
     
    U32 mapMorton(U32 size, U32 i, U32 j);
    
    
public:

    /**
     *
     *  Class constructor.
     *
     */
     
    PixelMapper();
    
    /**
     *
     *  Sets the display parameters for the mapping.
     *
     *  @param xRes Display horizontal resolution.
     *  @param yRes Display vertical resolution.
     *  @param stampW The width in pixels of a stamp.
     *  @param stampH The height in pixels of a stamp.
     *  @param genTileW The generation tile width in stamps.
     *  @param genTileH The generation tile height in stamps.
     *  @param scanTileW The scan tile width in generation tiles.
     *  @param scanTileH The scan tile height in generation tiles.
     *  @param overTileW The over scan tile width in scan tiles.
     *  @param overTileH The over scan tile height in scan tiles.
     *  @param samples Number of samples per pixel.
     *  @param bytesSample Bytes per sample.
     *
     */
     
    void setupDisplay(U32 xRes, U32 yRes, U32 stampW, U32 stampH,
                      U32 genW, U32 genH, U32 scanW, U32 scanH,
                      U32 overW, U32 overH,
                      U32 samples, U32 bytesSample);
                      
    /**
     *
     *  Sets the processing unit parameters for the mapping.
     *
     *  @param numUnits Number of processing units between which the pixels are distributed.
     *  @param memInterleaving Memory interleaving between the processing units.
     *
     */
  
    void setupUnit(U32 numUnits, U32 memInterleaving);
    
    /**
     *
     *  Changes the display resolution (used in bmoTextureProcessor).
     *
     *  @param hRes Horizontal display resolution.
     *  @param vRes Vertical display resolution.
     *
     */
     
    void changeResolution(U32 hRes, U32 vRes);
         
    /**
     *
     *  Computes the memory address corresponding with the given pixel position.
     *
     *  @param x Pixel horizontal position.
     *  @param y Pixel vertical position.     
     *
     *  @return The memory address corresponding with the defined pixel position.
     *
     */
    
    U32 computeAddress(U32 x, U32 y);
        
    /**
     *
     *  Maps the defined pixel position to a processing unit.
     *
     *  @param x Pixel horizontal position.
     *  @param y Pixel vertical position.
     *
     *  @return The identifier of the processing unit to which the pixel is mapped.
     *
     */
          
    U32 mapToUnit(U32 x, U32 y);
    
    /**
     *
     *  Maps the defined pixel address to a processing unit.
     *
     *  @param address Pixel address.
     *
     *  @return The identifier of the processing unit to which the pixel is mapped.
     *
     */
          
    U32 mapToUnit(U32 address);

    /**
     *
     *  Compute framebuffer size.
     *     
     */
     
    U32 computeFrameBufferSize();
    
    
}; // class PixelMapper

} // namespace cg1gpu

#endif
