/**************************************************************************
 *
 */

#include "MsaaCompressor.h"
#include "BitStreamWriter.h"
#include "BitStreamReader.h"

namespace arch
{

const MsaaLevelConfig bmoMsaaCompressor::configs[] = {
        MsaaLevelConfig(1, 4, 64),
        MsaaLevelConfig(2, 16, 64),
        MsaaLevelConfig(1, 2, 128),
        MsaaLevelConfig(2, 8, 128),
        MsaaLevelConfig(2, 4, 192)
};

bmoMsaaCompressor::bmoMsaaCompressor(/*int numLevels,*/ int blockSize)
    : /*numLevels(numLevels),*/ blockSize(blockSize)
{
}

bmoMsaaCompressor::~bmoMsaaCompressor()
{
}

bool bmoMsaaCompressor::checkSubblock(
        const U32 *data, int start, const MsaaLevelConfig* conf, MsaaRefs* refs) 
{
    //printf("refs = 0x%0p\n", refs);
    
    bool *bdiff = new bool[blockSize];

    int end = start + conf->samples;
    
    for (int i = start; i < end; i++)
        bdiff[i] = true;
    
    int diff = 0;

    refs->ref1 = data[start];
    refs->ref2 = refs->ref1;
    refs->mask = 0;

    for (int i = start; (i < end) && (diff <= conf->numRefs); i++) {
        if (bdiff[i]) {
            diff += 1;
            refs->ref2 = (data[i] == refs->ref1) ? refs->ref2 : data[i];
            for (int j = i + 1; j < end; j++)
                bdiff[j] = bdiff[j] && (data[i] != data[j]);
        }
        int index = (data[i] == refs->ref1) ? 0 : 1;
        refs->mask = (refs->mask << 1) | index;
    }
    
    delete[] bdiff;
    
    return diff <= conf->numRefs;
}

int bmoMsaaCompressor::findCompressionLevel(
        const U32 *datain, int size, MsaaRefs *refsArray, int &refsArraySize) 
{
    int level = 0;
        
    const MsaaLevelConfig* conf = &configs[level];
    
    bool ok = true;    
    int start = 0;
    int refsIndex = 0;

    while (ok && (start < size)) {
        
        ok = ok && checkSubblock(datain, start, conf, &refsArray[refsIndex]);
        
        if (!ok) {
            level++;
            if (level < numLevels) {
                conf = &configs[level];
                ok = true;
            }
            start = 0;
            refsIndex = 0;
        }
        else {
            start += conf->samples;
            refsArray[refsIndex].samples = conf->samples;
            refsIndex++;
        }
    }

    refsArraySize = refsIndex;

    return level;
}

CompressorInfo bmoMsaaCompressor::compress(
        const void *input, void *output, int size) 
{
    const U32* datain = (U32*) input;
    U32* dataout = (U32*) output;

    MsaaRefs refsArray[32];
    int refsArraySize = 0;

    int level = findCompressionLevel(datain, size, &refsArray[0], refsArraySize);

    bool success = level < numLevels;

    if (success) {
        BitStreamWriter bs(dataout);

        const MsaaLevelConfig& conf = configs[level];
        for (int i = 0; i < refsArraySize; i++) {
            MsaaRefs& refs = refsArray[i];
            int samples = refs.samples;
            int pmask = 1 << (refs.samples - 1);
            
            while (samples > 0) {
                bs.write(refs.ref1, 32);
                
                if (conf.numRefs > 1) {
                    bs.write(refs.ref2, 32);
    
                    for (int j = 0; j < conf.samples; j++) {
                        int mask = (refs.mask & pmask) == 0 ? 0 : 1;
                        bs.write(mask, 1);
                        pmask >>= 1;
                    }
                }

                samples -= conf.samples;
            }
        }
    }

    int newSize = success ? configs[level].size : (size * sizeof(U32));
    
    return CompressorInfo(success, level, newSize);
}

CompressorInfo bmoMsaaCompressor::uncompress(
        const void *input, void *output, int size, int level) 
{
    U32* datain = (U32*) input;
    U32* dataout = (U32*) output;
        
    BitStreamReader bs(datain);

    const MsaaLevelConfig& conf = configs[level];

    int i = 0;
    
    if (conf.numRefs > 1) {
        while (i < blockSize) {
            U32 ref1 = bs.read(32);
            U32 ref2 = bs.read(32);
            
            for (int j = 0; j < conf.samples; j++) {
                U32 mask = bs.read(1);
                dataout[i++] = (mask == 0) ? ref1 : ref2;
            }
        }
    }
    else {
        while (i < blockSize) {
            U32 ref1 = bs.read(32);
            
            for (int j = 0; j < conf.samples; j++)
                dataout[i++] = ref1;
        }
    }

    return CompressorInfo(true, level, size * sizeof(U32));
}

}
