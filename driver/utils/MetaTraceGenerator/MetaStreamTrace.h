/**************************************************************************
 *
 * MetaStream Trace File Definitions File
 * This file contains definitions related with the MetaStream trace files.
 *
 */

#ifndef __META_STREAM_TRACEFILE__
#define __META_STREAM_TRACEFILE__

#include "GPUType.h"


static const char MetaStreamTRACEFILE_SIGNATURE[19] = "computGeneral.com";//  Defines the signature string for MetaStream trace files.
static const U32  MetaStreamTRACEFILE_VERSION = 0x0100;//  Defines the current version identifier for MetaStream trace files.
static const U32  MetaStreamTRACEFILE_HEADER_SIZE = 16384;//  Defines the size of the header section of the MetaStream trace file.

// This structure defines the parameters associated with the MetaStream trace file.
struct MetaStreamParameters
{
    //  Translation related parameters.
    U32 startFrame;            //  First frame from the original OpenGL trace in the MetaStream trace file.
    U32 traceFrames;           //  Number of frames in the MetaStream trace file (may not be accurate).
    //  Simulation related parameters.
    U32 memSize;               //  Size of the simulated GPU memory (GDDR).
    U32 mappedMemSize;         //  Size of the simulated system memory accessable from the GPU.
    U32 texBlockDim;           //  Texture blocking first level tile size in 2^n x 2^n texels.
    U32 texSuperBlockDim;      //  Texture blocking second level tile size in 2^n x 2^n first level tiles.
    U32 scanWidth;             //  Rasterization scan tile (first level) width in pixels.
    U32 scanHeight;            //  Rasterization scan tile (first level) heigh in pixels.
    U32 overScanWidth;         //  Rasterization over scan tile (second level) width in pixels.
    U32 overScanHeight;        //  Rasterization over scan tile (second level) height in pixels.
    U32 fetchRate;             //  Hint to the OpenGL library to optimize shader programs for the given number of parallel SIMD ALUs.
    bool doubleBuffer;         //  Flag that enabled double buffering for the color buffer.
    bool memoryControllerV2;   //  Flag that when enabled tells that the second version of the Memory Controller will be used for simulation.
    bool v2SecondInterleaving; //  Flag that enables the second data interleaving mode in the second version of the Memory Controller.
    
};

/**
 *  This structure defines the content of the header section of an MetaStream trace file
 */
struct MetaStreamHeader
{
    char signature[20];      //  Signature string for MetaStream trace files.
    U32 version;             //  Version number of the MetaStream file.
    union
    {
        MetaStreamParameters parameters;      //  Trace file parameters.
        U08 padding[MetaStreamTRACEFILE_HEADER_SIZE];  // Padding to the header size.
    };
};

#endif
