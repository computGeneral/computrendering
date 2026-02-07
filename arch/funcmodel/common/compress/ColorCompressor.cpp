/**************************************************************************
 *
 */

#include "ColorCompressor.h"
#include "HiloCompressor.h"
#include "MsaaCompressor.h"

#include "GPUMath.h"

namespace cg1gpu
{

bmoCompressor* bmoColorCompressor::instance = NULL;

bmoColorCompressor::bmoColorCompressor()
{
}

bmoColorCompressor::~bmoColorCompressor()
{
}

void bmoColorCompressor::configureCompressor(int id) {
    switch (id) {
    case 0: // the old hilo implementation 
        instance = new bmoHiloCompressor(2, colorMask, false);
        break;
    case 1: // hilore 
        instance = new bmoHiloCompressor(3, colorMask, true);
        break;
    case 2: // msaa 
        instance = new bmoMsaaCompressor(blockSize);
        break;
        
    default:
        CG_ASSERT("Unknown compressor id.");
    }
}

}
