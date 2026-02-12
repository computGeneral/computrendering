/**************************************************************************
 *
 */

#include "HiloCompressor.h"
#include "BitStreamWriter.h"
#include "BitStreamReader.h"

namespace arch
{

bmoHiloCompressor::bmoHiloCompressor(int numLevels, U32 dataMask, bool reorderData)
    : numLevels(numLevels), dataMask(dataMask), reorderData(reorderData)
{
    static const int lobits[] = {5, 13, 21};
    static const int csize[] = {64, 128, 192};
    
    // Initialize compressed blocks sizes
    for (int i = 0; i < numLevels; i++) {
        configs[i] = HiloLevelConfig(lobits[i]);
        comprSize[i] = csize[i];
    }
}

bmoHiloCompressor::~bmoHiloCompressor()
{
}

U32 bmoHiloCompressor::reorder(
        U32 data, const HiloLevelConfig& config)
{
    return reorderData ? 
            (data & config.reHighMask1)
                | (data & config.reHighMask2) << config.reHighShift2
                | (data & config.reHighMask3) << config.reHighShift3
                | (data & config.reHighMask4) << config.reHighShift4
                    
                | (data & config.reLowMask1) >> config.reLowShift1
                | (data & config.reLowMask2) >> config.reLowShift2
                | (data & config.reLowMask3) >> config.reLowShift3
                | (data & config.reLowMask4)
            : data;
}

U32 bmoHiloCompressor::unreorder(
        U32 data, const HiloLevelConfig& config)
{
    return reorderData ? 
            (data & config.reHighMask1)
                | (data & config.unreHighMask2) >> config.reHighShift2
                | (data & config.unreHighMask3) >> config.reHighShift3
                | (data & config.unreHighMask4) >> config.reHighShift4
                
                | (data & config.unreLowMask1) << config.reLowShift1
                | (data & config.unreLowMask2) << config.reLowShift2
                | (data & config.unreLowMask3) << config.reLowShift3
                | (data & config.reLowMask4)
            : data;
}

MinMaxInfo bmoHiloCompressor::findMinMax(
        const U32 *data, int size, const HiloLevelConfig& config)
{
    U32 min = 0xffffffff;
    U32 max = 0x00000000;
    
    for (int i = 0; i < size; i++) {
        U32 d = reorder(data[i] & dataMask, config);
        min = d < min ? d : min;
        max = d > max ? d : max;
    }
    
    return MinMaxInfo(min, max);
}

int bmoHiloCompressor::findCompressionLevel(
        const U32 *data, int size, MinMaxInfo& mmi, HiloLevelRefs& refs)
{
    int level = 0;

    const HiloLevelConfig* config = &configs[level];
    mmi = findMinMax(data, size, *config);
    refs = HiloLevelRefs(mmi, *config);
    
    int i = 0;
    while (i < size && level < numLevels) {
        U32 d = reorder(data[i] & dataMask, *config);
        
        while (level < numLevels && !refs.test(d & config->highMask)) {
            level++;
            if (level < numLevels) {
                config = &configs[level];
                mmi = findMinMax(data, size, *config);
                refs = HiloLevelRefs(mmi, *config);
                i = -1;
            }
        }
        i++;
    }
    
    return level;
}

CompressorInfo bmoHiloCompressor::compress(
        const void *input, void *output, int size)
{
    U32* datain = (U32*) input;
    U32* dataout = (U32*) output;
    
    MinMaxInfo mmi;
    HiloLevelRefs refs;
    
    int level = findCompressionLevel(datain, size, mmi, refs);
    bool success = level < numLevels;
    
    if (success) {
        const HiloLevelConfig& config = configs[level];
        
        BitStreamWriter bs(dataout);
        
        //bs.write(level, 2);
        bs.write((mmi.min >> config.lowBits), config.highBits);
        bs.write((mmi.max >> config.lowBits), config.highBits);
        
        for (int i = 0; i < size; i++) {
            U32 d = reorder(datain[i] & dataMask, config);
            U32 index = refs.getIndexOfRef(d & config.highMask);
            bs.write(index, 2);
            bs.write(d, config.lowBits);
        }
        
        //volatile int wb = (bs.getWrittenBits() + 7) / 8;
        //cout << wb << " <--> " << (success ? comprSize[level] : (size * sizeof(U32))) << endl;
    }
    
    int newSize = success ? comprSize[level] : (size * sizeof(U32));
    
    return CompressorInfo(success, level, newSize);
}

CompressorInfo bmoHiloCompressor::uncompress(
        const void *input, void *output, int size, int level)
{
    U32* datain = (U32*) input;
    U32* dataout = (U32*) output;
        
    BitStreamReader bs(datain);
    
    //int level = bs.read(2);
    
    const HiloLevelConfig& config = configs[level];
    
    U32 min = bs.read(config.highBits) << config.lowBits;
    U32 max = bs.read(config.highBits) << config.lowBits;
    
    MinMaxInfo mmi(min, max);
    
    HiloLevelRefs refs(mmi, config);
    
    for (int i = 0; i < size; i++) {
        U32 index = bs.read(2);
        U32 lowPart = bs.read(config.lowBits);
        U32 highPart = refs.getRefByIndex(index);
        dataout[i] = unreorder(highPart | lowPart, config);
    }
    
    return CompressorInfo(true, level, size * sizeof(U32));
}

}
