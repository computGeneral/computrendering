/**************************************************************************
 *
 */

#ifndef MSAACOMPRESSOREMULATOR_H_
#define MSAACOMPRESSOREMULATOR_H_

#include "GPUType.h"
#include "Compressor.h"

namespace cg1gpu
{

// This class represents the configuration used to compress a block. 

class MsaaLevelConfig {
public:
    int numRefs;    // Number of references used for each subblock. 
    int samples;    // Number of samples used for each subblock. 
    int size;        // The compressed block size (if compression success). 
    
    MsaaLevelConfig() {}

    MsaaLevelConfig(int numRefs, int samples, int size) 
        : numRefs(numRefs), samples(samples), size(size) {}
};

// This class represents the references found in a block. 

class MsaaRefs {
public:
    U32 ref1;    // first reference 
    U32 ref2;    // second reference (if using 2 references) 
    U32 mask;    // references index mask 

    int samples;    // number of samples used in the subblock 

    MsaaRefs() {}

    MsaaRefs(U32 ref1, U32 ref2, U32 mask) 
        : ref1(ref1), ref2(ref2), mask(mask), samples(0) {}
};

// This class implements the msaa compression algorithm. 

class bmoMsaaCompressor : public bmoCompressor
{
public:
    //* blockSize     number of elements for each block 
    bmoMsaaCompressor(/*int numLevels, */int blockSize);
    virtual ~bmoMsaaCompressor();

    CompressorInfo compress(const void *input, void *output, int size);

    CompressorInfo uncompress(const void *input, void *output, int size, int level);

    U32 getLevelBlockSize(unsigned int level) { 
        GPU_ASSERT(
                if (level >= numLevels)
                    CG_ASSERT(
                            "Compressed block size requested for not supported level."); )
        return configs[level].size; }

private:
    bool checkSubblock(const U32 *data, int start, const MsaaLevelConfig* conf, MsaaRefs* refs);
    int findCompressionLevel(const U32 *data, int size, MsaaRefs* refsArray, int &refsArraySize);

private:
    // Maximum number of compression levels supported
    static const int maxLevels = 5;

    // One compressor supports numLevels levels of compression, and each
    // level of compression have different configuration parameters.
    static const int numLevels = maxLevels;
    
    static const MsaaLevelConfig configs[maxLevels];

    // number of elements in a block
    int blockSize;
};

}

#endif //MSAACOMPRESSOREMULATOR_H_
