/**************************************************************************
 *
 * Maps pixels to addresses and processing units.
 *
 */

/**
 *
 * @file PixelMapper.h
 *
 * Implements a class used to map pixels to memory addresses or processing units 
 * applying tiling and different distribution algorithms.
 *
 */

#include "PixelMapper.h"
#include <iostream>
#include <cmath>

using namespace std;
using namespace arch;

PixelMapper::PixelMapper()
{
    //  Build the precomputed Morton table.
    buildMortonTable();
}

//  Sets the display parameters.
void PixelMapper::setupDisplay(U32 xRes, U32 yRes, U32 stampW, U32 stampH,
    U32 genW, U32 genH, U32 scanW, U32 scanH, U32 overW, U32 overH,
    U32 samplesP, U32 bytesSampleP)
{
    //  Set pixel mapper parameters.
    hRes = xRes;
    vRes = yRes;
    samples = samplesP;
    bytesSample = bytesSampleP;
    stampTileWidth = stampW;
    stampTileHeight = stampH;
    genTileWidth = genW;
    genTileHeight = genH;
    scanTileWidth = scanW;
    scanTileHeight = scanH;
    overTileWidth = overW;
    overTileHeight = overH;
    
    //  Precompute parameters.
    overTilePixelWidth = overW * scanW * genW * stampW;    
    overTilePixelHeight = overH * scanH * genH * stampH;
    overTilePixelSize = overTilePixelWidth * overTilePixelHeight;
    overTileRowWidth = (U32) ceil((F32) hRes / (F32) (overTilePixelWidth));
    overTileRows = (U32) ceil((F32) vRes / (F32) (overTilePixelHeight));
    overTileSize = overTileWidth * overTileHeight;
    scanTilePixelWidth = scanW * genW * stampW;
    scanTilePixelHeight = scanH * genH * stampH;
    scanTileSize = scanW * scanH;
    scanTilePixelSize = scanTilePixelWidth * scanTilePixelHeight;
    
    //  Precompute sub scan tile parameters.
    scanSubTileWidth = 1;
    scanSubTileHeight = 1;
    genAdjTileWidth = genTileWidth;
    genAdjTileHeight = genTileHeight;
    
    //  Precompute adjusted generation tile sizes for the defined the multisampling level.
    if (samples == 1)
    {
        //  Nothing to do.
    }    
    else if (samples == 2)
    {
        scanSubTileWidth = 2;
        
        if (genAdjTileWidth > 1)
            genAdjTileWidth = genAdjTileWidth / 2;
        else
            CG_ASSERT("For multisampling 2x gen tiles must be at least 2 stamp wide.");   
    }
    else if (samples == 4)
    {
        scanSubTileWidth = 2;
        scanSubTileHeight = 2;
        
        if (genAdjTileWidth > 1)
            genAdjTileWidth = genAdjTileWidth / 2;
        else
            CG_ASSERT("For multisampling 4x gen tiles must be at least 2 stamp wide.");

        if (genAdjTileHeight > 1)
            genAdjTileHeight = genAdjTileHeight / 2;
        else
            CG_ASSERT("For multisampling 4x gen tiles must be at least 2 stamp high.");
    }
    else if (samples == 8)
    {
        scanSubTileWidth = 4;
        scanSubTileHeight = 2;
        
        if (genAdjTileWidth > 3)
            genAdjTileWidth = genAdjTileWidth / 4;
        else
            CG_ASSERT("For multisampling 8x gen tiles must be at least 4 stamp wide.");

        if (genAdjTileHeight > 1)
            genAdjTileHeight = genAdjTileHeight / 2;
        else
            CG_ASSERT("For multisampling 8x gen tiles must be at least 2 stamp high.");
    }    
    else
        CG_ASSERT("Multisampling level not supported.");
    
    //  Precompute the adjusted generation tile sizes for the defined bytes per sample.
    if (bytesSample == 8)
    {
        scanSubTileHeight = scanSubTileHeight * 2;
        if (genAdjTileHeight > 1)
            genAdjTileHeight = genAdjTileHeight / 2;
        else
            CG_ASSERT("For FP16 color gen tiles must be at least 2 stamp high.");
    }

    scanSubTileSize = scanSubTileHeight * scanSubTileWidth;
    scanSubTilePixelWidth = scanTileWidth * genAdjTileWidth * stampTileWidth;
    scanSubTilePixelHeight = scanTileHeight * genAdjTileHeight * stampTileHeight;
    scanSubTileBytes = scanSubTilePixelWidth * scanSubTilePixelHeight * samples * bytesSample;
    genAdjTilePixelWidth = genAdjTileWidth * stampTileWidth;
    genAdjTilePixelHeight = genAdjTileHeight * stampTileHeight; 
    genAdjTileSize = genAdjTileWidth * genAdjTileHeight;
    genAdjTilePixelSize = genAdjTilePixelWidth * genAdjTilePixelHeight;
    stampTileSize = stampTileWidth * stampTileHeight;
}

//  Sets the processing units parameters.
void PixelMapper::setupUnit(U32 numUnitsP, U32 memUnitInterleavingP)
{
    //  Set processing unit parameters.
    numUnits = numUnitsP;
    memUnitInterleaving = memUnitInterleavingP;
}

//  Change display resolution.
void PixelMapper::changeResolution(U32 hResP, U32 vResP)
{
    hRes = hResP;
    vRes = vResP;
    
    overTileRowWidth = (U32) ceil((F32) hRes / (F32) (overTilePixelWidth));
    overTileRows = (U32) ceil((F32) vRes / (F32) (overTilePixelHeight));
}

//  Compute address for the defined pixel.
U32 PixelMapper::computeAddress(U32 x, U32 y)
{
    /*
        Memory layout:

        Over scan tiles:

            device

            <--- xRes ---->
            O0  O1  O2  O3  ^
            O4  O5  O6  O7  | yRes
            O8  O9  O10 O11 |
            O12 O13 O14 O15 v

            memory (rows)

            O0 O1 O2 O3 O4 O5 O6 O7 O8 O9 O10 O11 O12 O13 O14 O15

        Scan tiles (inside over scan tile):

        For 1 sample:
              
            device

            <--- overW ---->
            T0  T1  T2  T3  ^
            T4  T5  T6  T7  | overH
            T8  T9  T10 T11 |
            T12 T13 T14 T15 v

            memory (rows)

            T0 T1 T2 T3 T4 T5 T6 T7 T8 T9 T10 T11 T12 T13 T14 T15

            memory (morton) <- current!!

            T0 T1 T4 T5  T2 T3 T6 T7  T8 T9 T12 T13  T10 T11 T14 T15

        When the number of samples is > 1 the scan tile is subdivided in as many subtiles as
        the number of samples per pixel.  Subtiles from the different scan tiles are then
        interleaved.
        
        For 2 samples:
        
            <--------------------- overW * 2 -------------------->
            T0S0   T0S1  T1S0   T1S1   T2S0   T2S1   T3S0   T3S1  ^
            T4S0   T4S1  T5S0   T5S1   T6S0   T6S0   T7S0   T7S1  | overH
            T8S0   T8S1  T9S0   T9S1   T10S0  T10S1  T11S0  T11S1 |
            T12S0  T12S0 T13S0  T13S1  T14S0  T14S1  T15S0  T15S1 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1

        For 4 samples:
        
            <--------------------- overW * 2 -------------------->
            T0S0   T0S1  T1S0   T1S1   T2S0   T2S1   T3S0   T3S1  ^
            T0S2   T0S3  T1S2   T1S3   T2S2   T2S3   T3S2   T3S3  |
            T4S0   T4S1  T5S0   T5S1   T6S0   T6S0   T7S0   T7S1  |
            T4S2   T4S3  T5S2   T5S3   T6S2   T6S3   T7S2   T7S3  | overH * 2
            T8S0   T8S1  T9S0   T9S1   T10S0  T10S1  T11S0  T11S1 |
            T8S2   T8S3  T9S2   T9S3   T10S2  T10S3  T11S2  T11S3 |
            T12S0  T12S0 T13S0  T13S1  T14S0  T14S1  T15S0  T15S1 |
            T12S2  T12S3 T13S2  T13S3  T14S2  T14S3  T15S2  T15S3 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
            T0S2 T1S2 T2S2 T3S2 T4S2 T5S2 T6S2 T7S2 T8S2 T9S2 T10S2 T11S2 T12S2 T13S2 T14S2 T15S2
            T0S3 T1S3 T2S3 T3S3 T4S3 T5S3 T6S3 T7S3 T8S3 T9S3 T10S3 T11S3 T12S3 T13S3 T14S3 T15S3
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1
            T0S2 T1S2 T4S2 T5S2  T2S2 T3S2 T6S2 T7S2  T8S2 T9S2 T12S2 T13S2  T10S2 T11S2 T14S2 T15S2
            T0S3 T1S3 T4S3 T5S3  T2S3 T3S3 T6S3 T7S3  T8S3 T9S3 T12S3 T13S3  T10S3 T11S3 T14S3 T15S3

        For 8 samples:
        
            <------------------------------------------------- overW * 4 ----------------------------------------------->
            T0S0   T0S1  T0S2   T0S3   T1S0   T1S1   T1S2   T1S3   T2S0   T2S1   T2S2   T2S3   T3S0   T3S1   T3S2   T3S3  ^
            T0S4   T0S5  T0S6   T0S7   T1S4   T1S5   T1S6   T1S7   T2S4   T2S5   T2S6   T2S7   T3S4   T3S5   T3S6   T3S7  |
            T4S0   T4S1  T0S2   T4S3   T5S0   T5S1   T5S2   T5S3   T6S0   T6S1   T6S2   T6S3   T7S0   T7S1   T7S2   T7S3  |
            T4S4   T4S5  T0S6   T4S7   T5S4   T5S5   T5S6   T5S7   T6S4   T6S5   T6S6   T6S7   T7S4   T7S5   T7S6   T7S7  |
            T8S0   T8S1  T8S2   T8S3   T9S0   T9S1   T9S2   T9S3   T10S0  T10S1  T10S2  T10S3  T11S0  T11S1  T11S2  T11S3 |  overH * 2
            T8S4   T8S5  T8S6   T8S7   T9S4   T9S5   T9S6   T9S7   T10S4  T10S5  T10S6  T10S7  T11S4  T11S5  T11S6  T11S7 |
            T12S0  T12S1 T12S2  T12S3  T13S0  T13S1  T13S2  T13S3  T14S0  T14S1  T14S2  T14S3  T15S0  T51S1  T51S2  T51S3 |
            T12S4  T12S5 T12S6  T12S7  T13S4  T13S5  T13S6  T13S7  T14S4  T14S5  T14S6  T14S7  T15S4  T51S5  T51S6  T51S7 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
            T0S2 T1S2 T2S2 T3S2 T4S2 T5S2 T6S2 T7S2 T8S2 T9S2 T10S2 T11S2 T12S2 T13S2 T14S2 T15S2
            T0S3 T1S3 T2S3 T3S3 T4S3 T5S3 T6S3 T7S3 T8S3 T9S3 T10S3 T11S3 T12S3 T13S3 T14S3 T15S3
            T0S4 T1S4 T2S4 T3S4 T4S4 T5S4 T6S4 T7S4 T8S4 T9S4 T10S4 T11S4 T12S4 T13S4 T14S4 T15S4
            T0S5 T1S5 T2S5 T3S5 T4S5 T5S5 T6S5 T7S5 T8S5 T9S5 T10S5 T11S5 T12S5 T13S5 T14S5 T15S5
            T0S6 T1S6 T2S6 T3S6 T4S6 T5S6 T6S6 T7S6 T8S6 T9S6 T10S6 T11S6 T12S6 T13S6 T14S6 T15S6
            T0S7 T1S7 T2S7 T3S7 T4S7 T5S7 T6S7 T7S7 T8S7 T9S7 T10S7 T11S7 T12S7 T13S7 T14S7 T15S7
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1
            T0S2 T1S2 T4S2 T5S2  T2S2 T3S2 T6S2 T7S2  T8S2 T9S2 T12S2 T13S2  T10S2 T11S2 T14S2 T15S2
            T0S3 T1S3 T4S3 T5S3  T2S3 T3S3 T6S3 T7S3  T8S3 T9S3 T12S3 T13S3  T10S3 T11S3 T14S3 T15S3
            T0S4 T1S4 T4S4 T5S4  T2S4 T3S4 T6S4 T7S4  T8S4 T9S4 T12S4 T13S4  T10S4 T11S4 T14S4 T15S4
            T0S5 T1S5 T4S5 T5S5  T2S5 T3S5 T6S5 T7S5  T8S5 T9S5 T12S5 T13S5  T10S5 T11S5 T14S5 T15S5
            T0S6 T1S6 T4S6 T5S6  T2S6 T3S6 T6S6 T7S6  T8S6 T9S6 T12S6 T13S6  T10S6 T11S6 T14S6 T15S6
            T0S7 T1S7 T4S7 T5S7  T2S7 T3S7 T6S7 T7S7  T8S7 T9S7 T12S7 T13S7  T10S7 T11S7 T14S7 T15S7

        Generation tiles (inside scan tile):

            device

            <--- scanW ---->
            G0  G1  G2  G3  ^
            G4  G5  G6  G7  | scanH
            G8  G9  G10 G11 |
            G12 G13 G14 G15 v

            memory (rows)

            G0 G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 G11 G12 G13 G14 G15              
        
        Stamps (inside generation tile):        

            <---- genW ---->
            S0  S1  S2  S3  ^
            S4  S5  S6  S7  | genH
            S8  S9  S10 S11 |
            S12 S13 S14 S15 v

            memory

            S0 S1 S2 S3 S4 S5 S6 S7 S8 S9 S10 S11 S12 S13 S14 S15

        Pixels (inside a stamp):

            <--- stampW ---->
            P0  P1  P2  P3  ^
            P4  P5  P6  P7  | stampH
            P8  P9  P10 P11 |
            P12 P13 P14 P15 v

            memory

            P0 P1 P2 P3 P4 P5 P6 P7 P8 P9 P10 P11 P12 P13 P14 P15

        Samples (inside a pixel)
        
            memory 
            
            S0 S1 S2 S3             
       
        Bytes (inside a pixel):

            memory

            R G B A


        Other organizations could be used:
            - Hilbert order.
            - Morton order.
            - ...

     */

    //  Position already in device coordinates.
    U32 xDevice = x;
    U32 yDevice = y;

    //  Get over scan tile coordinates inside the device.
    U32 xOverTile = xDevice / overTilePixelWidth;
    U32 yOverTile = yDevice / overTilePixelHeight;

    //  Calculate address for the over scan tile.
    U32 address = yOverTile * overTileRowWidth + xOverTile;

    //  Get scan tile coordinates inside the over scan tile.
    U32 xScanTile = GPU_MOD(xDevice / scanTilePixelWidth, overTileWidth);
    U32 yScanTile = GPU_MOD(yDevice / scanTilePixelHeight, overTileHeight);

    //  Get coordinates of the scan sub tile.
    U32 xScanSubTile = GPU_MOD(xDevice / scanSubTilePixelWidth, scanSubTileWidth);
    U32 yScanSubTile = GPU_MOD(yDevice / scanSubTilePixelHeight, scanSubTileHeight);
    
    //  Calculate address of the scan sub tile.    
    address = address * scanSubTileSize + yScanSubTile * scanSubTileWidth + xScanSubTile;

    //  NOTE:  Replaced with morton order for scan tiles.
    //  Calculate address for the scan tile (row major).
    //address = address * overW * overH + yScanTile * overW + yScanTile;

    //  Calculate address for the scan tile (morton).
    address = address * overTileSize + fastMapMorton(overTileWidth, xScanTile, yScanTile);

    //  Get generation tile coordinates inside the scan tile .
    U32 xGenTile = GPU_MOD(xDevice / genAdjTilePixelWidth, scanTileWidth);
    U32 yGenTile = GPU_MOD(yDevice / genAdjTilePixelHeight, scanTileHeight);

    //  Calculate address for the scan tile.
    address = address * scanTileSize  + yGenTile * scanTileWidth + xGenTile;

    //  Get coordinates stamp coordinates inside the generation tile.
    U32 xStamp = GPU_MOD(xDevice / stampTileWidth, genAdjTileWidth);
    U32 yStamp = GPU_MOD(yDevice / stampTileHeight, genAdjTileHeight);

    //  Calculate address for the stamp.
    address = address * genAdjTileSize + yStamp * genAdjTileWidth + xStamp;

    //  Get pixel coordinates inside the stamp.
    U32 xPixel = GPU_MOD(xDevice, stampTileWidth);
    U32 yPixel = GPU_MOD(yDevice, stampTileHeight);

    //  Get address for the pixel (which pixel).
    address = address * stampTileSize + yPixel * stampTileWidth + xPixel;
    
    //  Get start address.
    address = address * samples * bytesSample;

    return address;
}

//  Map pixel (position) to processing unit.
U32 PixelMapper::mapToUnit(U32 x, U32 y)
{
    //  Position already in device coordinates.
    U32 xDevice = x;
    U32 yDevice = y;

    //  Get scan tile coordinates inside the over scan tile.
    U32 xScanTile = GPU_MOD(xDevice / scanTilePixelWidth, overTileWidth);
    U32 yScanTile = GPU_MOD(yDevice / scanTilePixelHeight, overTileHeight);

    //  Calculate address for the scan tile (morton).
    return GPU_MOD(fastMapMorton(overTileWidth, xScanTile, yScanTile), numUnits);
}

//  Map pixel (address) to processing unit.
U32 PixelMapper::mapToUnit(U32 address)
{
    U32 scanSubTile = address / scanSubTileBytes;
    U32 scanTile = GPU_MOD(scanSubTile, overTileSize);
    return GPU_MOD(scanTile, numUnits);
}


//  Compute the size of the framebuffer for the defined display.
U32 PixelMapper::computeFrameBufferSize()
{
    return overTileRowWidth * overTileRows * overTilePixelSize * samples * bytesSample;
}


//  Builds the precomputed Morton table.
void PixelMapper::buildMortonTable()
{
    for(U32 i = 0; i < 256; i++)
    {
        U32 t1 = i & 0x0F;
        U32 t2 = (i >> 4) & 0x0F;
        U32 m = 0;

        for(U32 nextBit = 0; nextBit < 4; nextBit++)
        {
            m += ((t1 & 0x01) << (2 * nextBit)) + ((t2 & 0x01) << (2 * nextBit + 1));

            t1 = t1 >> 1;
            t2 = t2 >> 1;
        }

        mortonTable[i] = m;
    }
}

//  Calculates the address in morton order of an element in a 2^size x 2^size tile.
U32 PixelMapper::fastMapMorton(U32 size, U32 i, U32 j)
{
    U32 address;
    U32 t1, t2;

    t1 = i;
    t2 = j;

    switch(size)
    {
        case 0:

            address = 0;

            return address;

            break;

        case 1:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x03;

            return address;

            break;

        case 2:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x0F;

            return address;

            break;

        case 3:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x3F;

            return address;

            break;

        case 4:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];

            return address;

            break;

        case 5:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x03) << 8;

            return address;

            break;

        case 6:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x0F) << 8;

            return address;

            break;

        case 7:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x3F) << 8;

            return address;

            break;

        case 8:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] << 8;

            return address;

            break;

        default:
            CG_ASSERT("Fast morton not supported for this tile size");

            return 0;
            break;
    }
}

//  Maps an element inside a square power of two tile to Morton order.
U32 PixelMapper::mapMorton(U32 size, U32 i, U32 j)
{
    U32 address;
    U32 nextBit;
    U32 t1, t2;

    t1 = i;
    t2 = j;
    address = 0;

    //  Get a bit from each coordinate each time.
    for(nextBit = 0; nextBit < size; nextBit++)
    {
        address += ((t1 & 0x01) << (2 * nextBit)) + ((t2 & 0x01) << (2 * nextBit + 1));
        t1 = t1 >> 1;
        t2 = t2 >> 1;
    }

    return address;
}
