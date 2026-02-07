/**************************************************************************
 *
 */

#ifndef COMPRESSOREMULATOR_H_
#define COMPRESSOREMULATOR_H_

#include "GPUType.h"

namespace cg1gpu
{

class CompressorInfo
{
public:
    bool success;        // Has been compressed/decompressed 
    int level;            // Compression level
    int size;            // Block size after compression/decompression
    
    CompressorInfo() {}
    
    CompressorInfo(bool success, int level, int size)
        : success(success), level(level), size(size) { }
    
    CompressorInfo(const CompressorInfo& info)
        : success(info.success), level(info.level), size(info.size) {}
    
    /*CompressorInfo& operator= (const CompressorInfo& info) {
        success = info.success;
        level = info.level;
        size = info.size;
        return *this;
    }*/
};

class bmoCompressor
{
public:
    bmoCompressor() { }
    virtual ~bmoCompressor() { }
    
    virtual CompressorInfo compress(const void *input, void *output, int size) = 0;
    
    virtual CompressorInfo uncompress(const void *input, void *output, int size, int level) = 0;
    
    virtual U32 getLevelBlockSize(unsigned int level) = 0;
};

}
#endif //COMPRESSOREMULATOR_H_
