/**************************************************************************
 *
 */

#ifndef GAL_SHADERPROGRAM_IMP
    #define GAL_SHADERPROGRAM_IMP

#include "GALShaderProgram.h"
#include "GALStoredStateImp.h"
#include "MemoryObject.h"
#include "HAL.h"
#include "StateItem.h"
#include <set>
#include <bitset>

namespace libGAL
{


class GALShaderProgramImp : public GALShaderProgram, public MemoryObject
{
public:

    GALShaderProgramImp();

    //////////////////////////////////////////////////////
    /// Methods required by GALShaderProgram interface ///
    //////////////////////////////////////////////////////

    virtual void setCode(const gal_ubyte* CG1ByteCode, gal_uint sizeInBytes);

    virtual void setProgram(gal_ubyte *CG1ASM);
    
    virtual const gal_ubyte* getCode() const;

    virtual gal_uint getSize() const;

    virtual void setConstant(gal_uint index, const gal_float* vect4);

    virtual void getConstant(gal_uint index, gal_float* vect4) const;

    virtual void printASM(std::ostream& os) const;

    virtual void printBinary(std::ostream& os) const;

    virtual void printConstants(std::ostream& os) const;

    ////////////////////////////////////////////////////////
    /// Methods required by MemoryObject derived classes ///
    ////////////////////////////////////////////////////////

    virtual const gal_ubyte* memoryData(gal_uint region, gal_uint& memorySizeInBytes) const;

    virtual const gal_char* stringType() const;

    ~GALShaderProgramImp();

    ///////////////////////////////////////////////////////
    // Method only available for backend CG1 compiler //
    ///////////////////////////////////////////////////////
    void setOptimizedCode(const gal_ubyte* optimizedCode, gal_uint sizeInBytes);

    bool isOptimized() const;

    void updateConstants(HAL* driver, arch::GPURegister constantsType);

    static const gal_uint MAX_SHADER_ATTRIBUTES = (arch::MAX_VERTEX_ATTRIBUTES > arch::MAX_FRAGMENT_ATTRIBUTES? arch::MAX_VERTEX_ATTRIBUTES: arch::MAX_FRAGMENT_ATTRIBUTES);

    void setInputRead(gal_uint inputReg, gal_bool read);

    void setOutputWritten(gal_uint outputReg, gal_bool written);

    void setMaxAliveTemps(gal_uint maxAlive);

    gal_bool getInputRead(gal_uint inputReg) const;

    gal_bool getOutputWritten(gal_uint outputReg) const;

    gal_uint getMaxAliveTemps() const;

    void setTextureUnitsUsage(gal_uint tu, gal_enum usage);

    gal_enum getTextureUnitsUsage(gal_uint tu);

    void setKillInstructions(gal_bool kill);

    gal_bool getKillInstructions();


private:

    enum { CONSTANT_BANK_REGISTERS = 512 };
    
    static const gal_uint MAX_PROGRAM_SIZE = 1024 * 16;

    mutable gal_ubyte* _assembler;
    mutable gal_uint _assemblerSize;

    void _computeASMCode();

    gal_ubyte* _bytecode;
    gal_ubyte* _optBytecode;
    gal_uint _bytecodeSize;
    gal_uint _optBytecodeSize;
    gal_float _constantBank[CONSTANT_BANK_REGISTERS][4];
    StateItem<GALVector<gal_float,4> > _constantCache[CONSTANT_BANK_REGISTERS];
    gal_uint _lastConstantSet;

    gal_enum _textureUnitUsage[16/*MAX_TEXTURE_UNITS_ARB*/];
    gal_bool _killInstructions;


    std::set<gal_uint> _touched;    

    std::bitset<MAX_SHADER_ATTRIBUTES> _inputsRead;
    std::bitset<MAX_SHADER_ATTRIBUTES> _outputsWritten;
    gal_uint _maxAliveTemps;

};

}

#endif // GAL_SHADERPROGRAM_IMP
