/**************************************************************************
 *
 */

#ifndef HILOCOMPRESSOREMULATOR_H_
#define HILOCOMPRESSOREMULATOR_H_

#include "GPUType.h"
#include "Compressor.h"

using namespace std;

namespace cg1gpu 
{

// This class saves information about minimum and maximum values. 

class MinMaxInfo 
{
public:
    U32 min;
    U32 max;
    
    MinMaxInfo() {}
    
    MinMaxInfo(U32 min, U32 max)
        : min(min), max(max) {}
    
    MinMaxInfo(const MinMaxInfo& minmax) {
        min = minmax.min;
        max = minmax.max;
    }
};

/* This class represents the configuration used to compress a block.
 * It precalculates all the parameters from the number of low bits desired. */

class HiloLevelConfig 
{
public:
    int lowBits;
    U32 lowMask;
    
    int highBits;
    U32 highMask;
    
    U32 reHighMask1, reHighMask2, reHighMask3, reHighMask4;
    U32 reLowMask1, reLowMask2, reLowMask3, reLowMask4;
    
    U32 unreHighMask2, unreHighMask3, unreHighMask4;
    U32 unreLowMask1, unreLowMask2, unreLowMask3;
        
    int reHighShift2, reHighShift3, reHighShift4;
    int reLowShift1, reLowShift2, reLowShift3;

    HiloLevelConfig() {}
    
    HiloLevelConfig(int lowBits) 
        : lowBits(lowBits) 
    {
        lowMask = (1 << lowBits) - 1;
        highBits = 32 - lowBits;
        highMask = ~lowMask;
        
        // Precalculate mask's and shift's needed to make bits reordering
        int reLowBits = lowBits / 4;
        int reHighBits = 8 - reLowBits;
        
        U32 msk = (1 << reHighBits) - 1;
        reHighMask1 = msk << (reHighBits * 3 + reLowBits * 4);
        reHighMask2 = msk << (reHighBits * 2 + reLowBits * 3);
        reHighMask3 = msk << (reHighBits * 1 + reLowBits * 2);
        reHighMask4 = msk << (reHighBits * 0 + reLowBits * 1);
        
        unreHighMask2 = msk << (reHighBits * 2 + reLowBits * 4);
        unreHighMask3 = msk << (reHighBits * 1 + reLowBits * 4);
        unreHighMask4 = msk << (reHighBits * 0 + reLowBits * 4);
                
        msk = (1 << reLowBits) - 1;
        reLowMask1 = msk << (reHighBits * 3 + reLowBits * 3);
        reLowMask2 = msk << (reHighBits * 2 + reLowBits * 2);
        reLowMask3 = msk << (reHighBits * 1 + reLowBits * 1);
        reLowMask4 = msk;
        
        unreLowMask1 = msk << reLowBits * 3;
        unreLowMask2 = msk << reLowBits * 2;
        unreLowMask3 = msk << reLowBits * 1;

        reHighShift2 = reLowBits;
        reHighShift3 = reLowBits * 2;
        reHighShift4 = reLowBits * 3;
        
        reLowShift1 = reHighBits * 3;
        reLowShift2 = reHighBits * 2;
        reLowShift3 = reHighBits;
    }
};

// This class represents the references found in a block. 

class HiloLevelRefs 
{
public:
    U32 min;
    U32 max;
    U32 a;
    U32 b;
    
    HiloLevelRefs() {}
    
    HiloLevelRefs(const MinMaxInfo& mmi, const HiloLevelConfig& config) {        
        min = mmi.min & config.highMask;
        max = mmi.max & config.highMask;
        
        U32 inc = (1 << config.lowBits);
        a = min + inc;
        b = max - inc;
    }
    
    HiloLevelRefs(const HiloLevelRefs& refs)
        : min(refs.min), max(refs.max), a(refs.a), b(refs.b) {}
    
    bool test(U32 d) {
        return d == min || d == max || d == a || d == b;
    }

    int getIndexOfRef(U32 ref) {
        int index =  ref == min ? 0 :
                        ref == max ? 1 :
                        ref == a ? 2 : 3;
        return index;
    }
    
    U32 getRefByIndex(int index) {
        return index == 0 ? min : 
                index == 1 ? max :
                index == 2 ? a : b;
    }
};

/* This class implements the hilo and hilore compression algorithms,
 * and can be used with both depth and color. */

class bmoHiloCompressor : public bmoCompressor
{
public:
    bmoHiloCompressor(int numLevels, U32 dataMask = 0xffffffff, bool reorderData = true);
    
    virtual ~bmoHiloCompressor();

    CompressorInfo compress(const void *input, void *output, int size);

    CompressorInfo uncompress(const void *input, void *output, int size, int level);

    U32 getLevelBlockSize(unsigned int level) {
        GPU_ASSERT(
                if (level >= (unsigned int)numLevels)
                    CG_ASSERT(
                            "Compressed block size requested for not supported level."); )

        return comprSize[level];
    }
    
private:
    U32 reorder(U32 data, const HiloLevelConfig& config);
    U32 unreorder(U32 data, const HiloLevelConfig& config);
    
    MinMaxInfo findMinMax(const U32 *data, int size, const HiloLevelConfig& config);
    
    int findCompressionLevel(const U32 *data, int size, MinMaxInfo& mmi, HiloLevelRefs& refs);

private:
    // Maximum number of compression levels supported
    static const int maxLevels = 3;

    // One compressor supports numLevels levels of compression, and each
    // level of compression have different configuration parameters.
    int numLevels;
    HiloLevelConfig configs[maxLevels];
    
    // Precalculated size for compressed blocks
    int comprSize[maxLevels];

    // Mask applyed to each block data when searching for min and max and encoding
    // Can be used to remove stencil data from the 8 msb of the depth. 
    const U32 dataMask;
    
    // Is reordering of the data bits activated ?
    bool reorderData;
};

}

#endif //HILOCOMPRESSOREMULATOR_H_
