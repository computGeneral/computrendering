/**************************************************************************
 *
 */

#include "DepthCompressor.h"
#include "HiloCompressor.h"

#include "GPUMath.h"

namespace arch
{

bmoCompressor* bmoDepthCompressor::instance = NULL;

bmoDepthCompressor::bmoDepthCompressor()
{
}

bmoDepthCompressor::~bmoDepthCompressor()
{
}

void bmoDepthCompressor::configureCompressor(int id) {
    switch (id) {
    case 0: // hiloz 
        instance = new bmoHiloCompressor(levels, depthMask, false);
        break;
    
    default:
        CG_ASSERT("Unknown compressor id.");
    }
}

}
