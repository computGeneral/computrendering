/**************************************************************************
 *
 */

#ifndef GALx_COMPILED_PROGRAM_IMP_H
    #define GALx_COMPILED_PROGRAM_IMP_H

#include "GALx.h"
#include "GALxRBank.h"
#include "GALxImplementationLimits.h"

namespace libGAL
{

class GALxCompiledProgramImp: public GALxCompiledProgram
{
//////////////////////////
//  interface extension //
//////////////////////////
private:
    
    gal_ubyte* _compiledBytecode;
    gal_uint _compiledBytecodeSize;
    GALxRBank<float> _compiledConstantBank;
    gal_enum _textureUnitUsage[MAX_TEXTURE_UNITS_ARB];
    gal_bool _killInstructions;

public:
    
    GALxCompiledProgramImp();

    void setCode(const gal_ubyte* compiledBytecode, gal_uint compiledBytecodeSize);

    void setTextureUnitsUsage(gal_uint tu, gal_enum usage);

    void setKillInstructions(gal_bool kill);

    const gal_ubyte* getCode() const;

    gal_uint getCodeSize() const;

    gal_enum getTextureUnitsUsage(gal_uint tu) const;

    gal_bool getKillInstructions() const;

    void setCompiledConstantBank(const GALxRBank<float>& ccBank);

    const GALxRBank<float>& getCompiledConstantBank() const;

    ~GALxCompiledProgramImp();
};

} // namespace libGAL

#endif // GALx_COMPILED_PROGRAM_IMP_H
