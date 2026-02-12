/**************************************************************************
 *
 */

#include "GALShaderProgramImp.h"
#include "GALMacros.h"
#include "HAL.h"

using namespace libGAL;

GALShaderProgramImp::GALShaderProgramImp() : _bytecode(0), _bytecodeSize(0), _optBytecode(0), 
        _optBytecodeSize(0), _lastConstantSet(0), _assembler(0), _assemblerSize(0), 
        _maxAliveTemps(0), _killInstructions(false)
{
    defineRegion(0);
    postReallocate(0);
    
    //  Initialize the constant bank.
    for(U32 c = 0; c < CONSTANT_BANK_REGISTERS; c++)
    {
        _constantBank[c][0] = 0.0f;
        _constantBank[c][1] = 0.0f;
        _constantBank[c][2] = 0.0f;
        _constantBank[c][3] = 0.0f;
    }

    for (gal_uint tu = 0; tu < 16 /*MAX_TEX_UNITS*/; tu++)
        _textureUnitUsage[tu] = 0;
}

GALShaderProgramImp::~GALShaderProgramImp()
{
    delete[] _bytecode;
    delete[] _optBytecode;
    delete[] _assembler;
}

void GALShaderProgramImp::setCode(const gal_ubyte* CG1ByteCode, gal_uint sizeInBytes)
{    
    if ( sizeInBytes == _bytecodeSize && memcmp(CG1ByteCode, _bytecode, sizeInBytes) == 0 )
        return ; // Ignore set code

    delete[] _bytecode;
    delete[] _optBytecode;
    
    _optBytecode = 0;
    _optBytecodeSize = 0;

    _bytecode = new gal_ubyte[(_bytecodeSize = sizeInBytes)];
    memcpy(_bytecode, CG1ByteCode, sizeInBytes);

    _computeASMCode();

    postReallocate(0);
}

void GALShaderProgramImp::setProgram(gal_ubyte *CG1ASM)
{
    gal_ubyte *code = new gal_ubyte[MAX_PROGRAM_SIZE];
    gal_uint codeSize = MAX_PROGRAM_SIZE;
    
    codeSize = HAL::assembleShaderProgram(CG1ASM, code, codeSize);
    
    if ((codeSize == _bytecodeSize) && (memcmp(code, _bytecode, codeSize) == 0))
    {
        delete[] code;
        return; // Ignore set code
    }
    
    delete[] _bytecode;
    delete[] _optBytecode;
    
    _optBytecode = 0;
    _optBytecodeSize = 0;
    
    _bytecodeSize = codeSize;
    _bytecode = new gal_ubyte[_bytecodeSize];
    memcpy(_bytecode, code, _bytecodeSize);

    delete[] code;
    
    _computeASMCode();

//printf("GALShaderProgramImp::setProgram => program binary\n");
//printBinary(std::cout);
//printf("GALShaderProgramImp::setProgram => program asm\n");
//printASM(std::cout);

    postReallocate(0);
}

const gal_ubyte* GALShaderProgramImp::getCode() const
{
    return _bytecode;
}

gal_uint GALShaderProgramImp::getSize() const
{
    return _bytecodeSize;
}

void GALShaderProgramImp::setConstant(gal_uint index, const gal_float* vect4)
{
    GAL_ASSERT(
        if ( index >= CONSTANT_BANK_REGISTERS )
            CG_ASSERT("Constant index out of bounds");
    )

    // track the constant as updated
    _touched.insert(index);

    memcpy(_constantBank[index], vect4, 4*sizeof(gal_float));

    // track the last set constant (for constant printing sake of clarity).
    if (_lastConstantSet < index)
        _lastConstantSet = index;
}

void GALShaderProgramImp::getConstant(gal_uint index, gal_float* vect4) const
{
    GAL_ASSERT(
        if ( index >= CONSTANT_BANK_REGISTERS )
            CG_ASSERT("Constant index out of bounds");
    )
    memcpy(vect4, _constantBank[index], 4*sizeof(gal_float));
}

const gal_ubyte* GALShaderProgramImp::memoryData(gal_uint region, gal_uint& memorySizeInBytes) const
{
    if ( _optBytecode != 0 ) {
        memorySizeInBytes = _optBytecodeSize;
        return _optBytecode;
    }
    memorySizeInBytes = _bytecodeSize;
    return _bytecode;
}

const gal_char* GALShaderProgramImp::stringType() const
{
    return "SHADER_PROGRAM_OBJECT";
}

void GALShaderProgramImp::setOptimizedCode(const gal_ubyte* optimizedCode, gal_uint sizeInBytes)
{
    delete[] _optBytecode;

    _optBytecode = new gal_ubyte[(_optBytecodeSize = sizeInBytes)];
    memcpy(_optBytecode, optimizedCode, sizeInBytes);

    _computeASMCode(); // recompute ASM Code

    // More efficient implementation will be coded soon
    // No realocate required, often just updating is enough
    postReallocate(0);
}

bool GALShaderProgramImp::isOptimized() const
{
    return _optBytecode != 0;
}

void GALShaderProgramImp::updateConstants(HAL* driver, arch::GPURegister constantsType)
{
    arch::GPURegData data;

    set<gal_uint>::iterator it = _touched.begin();
    const set<gal_uint>::iterator end = _touched.end();

    for ( ; it != end; ++it ) {
        const gal_uint constantIndex = *it;
        const gal_float* cb = _constantBank[constantIndex];
        gal_float* qfVal = data.qfVal;
        *qfVal = *cb;
        *(qfVal+1) = *(cb+1);
        *(qfVal+2) = *(cb+2);
        *(qfVal+3) = *(cb+3);
        driver->writeGPURegister(constantsType, constantIndex, data);
    }
}

void GALShaderProgramImp::setInputRead(gal_uint inputReg, gal_bool read)
{
    if (inputReg >= MAX_SHADER_ATTRIBUTES)
        CG_ASSERT("Input register out of bounds");

    _inputsRead.set(inputReg, read);
}

void GALShaderProgramImp::setOutputWritten(gal_uint outputReg, gal_bool written)
{
    if (outputReg >= MAX_SHADER_ATTRIBUTES)
        CG_ASSERT("Output register out of bounds");

    _outputsWritten.set(outputReg, written);
}

void GALShaderProgramImp::setMaxAliveTemps(gal_uint maxAlive)
{
    _maxAliveTemps = maxAlive;
}

gal_bool GALShaderProgramImp::getInputRead(gal_uint inputReg) const
{
    if (inputReg >= MAX_SHADER_ATTRIBUTES)
        CG_ASSERT("Input register out of bounds");

    return _inputsRead.test(inputReg);
}

gal_bool GALShaderProgramImp::getOutputWritten(gal_uint outputReg) const
{
    if (outputReg >= MAX_SHADER_ATTRIBUTES)
        CG_ASSERT("Output register out of bounds");

    return _outputsWritten.test(outputReg);
}

gal_uint GALShaderProgramImp::getMaxAliveTemps() const
{
    return _maxAliveTemps;
}

void GALShaderProgramImp::setTextureUnitsUsage(gal_uint tu, gal_enum usage)
{
    _textureUnitUsage[tu] = usage;
}

gal_enum GALShaderProgramImp::getTextureUnitsUsage(gal_uint tu)
{
    return _textureUnitUsage[tu];
}

void GALShaderProgramImp::setKillInstructions(gal_bool kill)
{
    _killInstructions = kill;
}

gal_bool GALShaderProgramImp::getKillInstructions()
{
    return _killInstructions;
}


void GALShaderProgramImp::printASM(std::ostream& os) const
{
    os << _assembler;
}

void GALShaderProgramImp::printBinary(std::ostream& os) const
{
    // Select binary code
    gal_ubyte* binary = ( _optBytecode ? _optBytecode : _bytecode );
    gal_uint binSize = ( _optBytecode ? _optBytecodeSize : _bytecodeSize );

    char buffer[16];
    string byteCodeLine;
    for ( unsigned int i = 1; i <= binSize; i++ )
    {
        sprintf(buffer, "%02x ", binary[i - 1]);
        byteCodeLine.append(buffer);
        if ((i % 16) == 0)
        {
            os << byteCodeLine;
            os << endl;
            byteCodeLine.clear();
        }
    }
}

void GALShaderProgramImp::printConstants(std::ostream& os) const
{
    for ( gal_uint i = 0; i <= _lastConstantSet; i++)
        os << "c" << i << ": {" << _constantBank[i][0] << "," << _constantBank[i][1] << "," << _constantBank[i][2] << "," << _constantBank[i][3] << "}" << endl;

    os << endl;
}

void GALShaderProgramImp::_computeASMCode()
{
    delete[] _assembler; // delete previous computed assembler 

    // Select binary code
    gal_ubyte* binary = ( _optBytecode ? _optBytecode : _bytecode );
    gal_uint binSize = ( _optBytecode ? _optBytecodeSize : _bytecodeSize );

    if ( !binary )
        CG_ASSERT("Binary code not available");

    gal_uint asmCodeSize = binSize * 16; 
    _assembler = new gal_ubyte[asmCodeSize];
    
    _assemblerSize = HAL::disassembleShaderProgram(binary, binSize, _assembler, asmCodeSize, _killInstructions);
}

//////////////////////////////////////////////
//  END OF PRINT DEFINITIONS AND FUNCTIONS  //
//////////////////////////////////////////////
