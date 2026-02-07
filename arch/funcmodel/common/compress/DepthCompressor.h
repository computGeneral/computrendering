/**************************************************************************
 *
 */

#ifndef DEPTHCOMPRESSOREMULATOR_H_
#define DEPTHCOMPRESSOREMULATOR_H_

#include "Compressor.h"

namespace cg1gpu
{

class bmoDepthCompressor
{
private:
    static bmoCompressor* instance;
    
    bmoDepthCompressor();
    virtual ~bmoDepthCompressor();
    
public:
    static bmoCompressor& getInstance() {
        return *instance;
    }
    
    static void configureCompressor(int id);
    
    static const int levels = 3;
    static const U32 depthMask = 0xffffffff;
};

}

#endif //DEPTHCOMPRESSOREMULATOR_H_
