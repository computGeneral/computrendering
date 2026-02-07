/**************************************************************************
 *
 */

#ifndef COLORCOMPRESSOREMULATOR_H_
#define COLORCOMPRESSOREMULATOR_H_

#include "Compressor.h"

namespace cg1gpu
{

class bmoColorCompressor
{
private:
    static bmoCompressor* instance;
    
    bmoColorCompressor();
    virtual ~bmoColorCompressor();
    
public:
    static bmoCompressor& getInstance() {
        return *instance;
    }
    
    static void configureCompressor(int id);
    
    static const int blockSize = 64;
    
    static const int colorMask = 0xffffffff;
};

}

#endif //COLORCOMPRESSOREMULATOR_H_
