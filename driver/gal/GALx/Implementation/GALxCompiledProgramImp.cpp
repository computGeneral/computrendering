/**************************************************************************
 *
 */

#include "GALxCompiledProgramImp.h"

using namespace libGAL;

GALxCompiledProgramImp::GALxCompiledProgramImp()
: _compiledBytecode(0), _compiledBytecodeSize(0), _compiledConstantBank(250,"clusterBank"), _killInstructions(false)
{
    for (int tu = 0; tu < MAX_TEXTURE_UNITS_ARB; tu++)
        _textureUnitUsage[tu] = 0;
}

GALxCompiledProgramImp::~GALxCompiledProgramImp()
{
    if (_compiledBytecode)
        delete[] _compiledBytecode;
}

void GALxCompiledProgramImp::setCode(const gal_ubyte* compiledBytecode, gal_uint compiledBytecodeSize)
{
    if (_compiledBytecode)
        delete[] _compiledBytecode;

    _compiledBytecode = new gal_ubyte[compiledBytecodeSize];

    memcpy(_compiledBytecode, compiledBytecode, compiledBytecodeSize);

    _compiledBytecodeSize = compiledBytecodeSize;
}

void GALxCompiledProgramImp::setTextureUnitsUsage(gal_uint tu, gal_enum usage)
{
    _textureUnitUsage[tu] = usage;
}

void GALxCompiledProgramImp::setKillInstructions(gal_bool kill)
{
    _killInstructions = kill;
}

gal_enum GALxCompiledProgramImp::getTextureUnitsUsage(gal_uint tu) const
{
    return _textureUnitUsage[tu];
}

gal_bool GALxCompiledProgramImp::getKillInstructions() const
{
    return _killInstructions;
}

const gal_ubyte* GALxCompiledProgramImp::getCode() const
{
    return _compiledBytecode;
}

gal_uint GALxCompiledProgramImp::getCodeSize() const
{
    return _compiledBytecodeSize;
}

void GALxCompiledProgramImp::setCompiledConstantBank(const GALxRBank<float>& ccBank)
{
    _compiledConstantBank = ccBank;
}

const GALxRBank<float>& GALxCompiledProgramImp::getCompiledConstantBank() const
{
    return _compiledConstantBank;
}
