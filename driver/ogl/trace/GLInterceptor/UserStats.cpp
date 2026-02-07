/**************************************************************************
 *
 */

#include "UserStats.h"

/**
 * Definitions must be placed here*
 *
 * The objects pointed by this pointers must be created in GLInterceptor::init() method
 */

VertexStat* vcStat = 0;
TriangleStat* tcStat = 0;
InstructionCount* vshInstrCountStat = 0;
InstructionCount* fshInstrCountStat = 0;
TextureLoadsCount* texInstrCountStat = 0;
FPAndAlphaStat* fpAndAlphaStat = 0;
