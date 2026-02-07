/**************************************************************************
 *
 */

#include "ShaderOptimizer.h"
#include "OptimizationSteps.h"

using namespace libGAL;
using namespace libGAL_opt;
using namespace std;
using namespace cg1gpu;

OptimizationStep::OptimizationStep(ShaderOptimizer* shOptimizer)
: _shOptimizer(shOptimizer)
{
}

//SHADER_ARCH_PARAMS::SHADER_ARCH_PARAMS(const gal_uint latTable[])
SHADER_ARCH_PARAMS::SHADER_ARCH_PARAMS()
: 
  nWay(4), 
  temporaries(32), 
  outputRegs(16), 
  addrRegs(1),
  predRegs(32)

{
    //for(gal_uint i=0; i < LATENCY_TABLE_SIZE; i++)
    //    latencyTable[i] = latTable[i];
    shArchParams = ShaderArchParam::getShaderArchParam();
};

ShaderOptimizer::ShaderOptimizer(const SHADER_ARCH_PARAMS& shArchP)
: _reOptimize(true), 
  _inputCode(0), 
  _inputSizeInBytes(0), 
  _outputCode(0), 
  _outputSizeInBytes(0), 
  _shArchParams(shArchP)
{
    // Build the optimization steps. The push_back order will
    // be the optimization step execution order.
    // According to this, if any optimization steps uses
    // a previous step result, it must be added after the required one.

    //_optimizationSteps.push_back(new StaticInstructionScheduling(this));
    _optimizationSteps.push_back(new MaxLiveTempsAnalysis(this));
    _optimizationSteps.push_back(new RegisterUsageAnalysis(this));
    
}

void ShaderOptimizer::setCode(const gal_ubyte* code, gal_uint sizeInBytes)
{
    // Delete the previous input code and structures contents.

    if (_inputCode)
        delete[] _inputCode;

    vector<InstructionInfo*>::iterator iter = _inputInstrInfoVect.begin();
    
    while (iter != _inputInstrInfoVect.end())
    {
        delete (*iter);
        iter++;
    }

    _inputInstrInfoVect.clear();

    // Initialize the code structures again.

    _inputCode = new gal_ubyte[sizeInBytes];
    _inputSizeInBytes = sizeInBytes;
    memcpy((void *)_inputCode, (const void*)code, sizeInBytes);

    // Build the required structures for the code optimization
    // dissassembling the binary code.

    gal_ubyte* binary_pointer = _inputCode;
    gal_uint instrCount = 0;

    gal_bool programEnd = false;
    
    while (!programEnd && (binary_pointer < (_inputCode + _inputSizeInBytes)))
    {
        cgoShaderInstr shInstr((U08*)binary_pointer);

        //if ((gal_uint)shInstr.getOpcode() >= LATENCY_TABLE_SIZE)
        //    CG_ASSERT("Invalid opcode or incomplete latency table");

        char asmStr[MAX_INFO_SIZE];
        shInstr.disassemble(asmStr, MAX_INFO_SIZE);

        //_inputInstrInfoVect.push_back(new InstructionInfo(shInstr, instrCount, _shArchParams.latencyTable[shInstr.getOpcode()], string(asmStr)));        
        _inputInstrInfoVect.push_back(new InstructionInfo(shInstr, instrCount,
                                                          _shArchParams.shArchParams->getExecutionLatency(shInstr.getOpcode()),
                                                          string(asmStr)));
        
        instrCount++;

        binary_pointer += cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE;
        
        programEnd = shInstr.getEndFlag();
    }
}

void ShaderOptimizer::setConstants(const std::vector<GALVector<gal_float,4> >& constants)
{
    _inputConstants = constants;
}

void ShaderOptimizer::optimize()
{
    //////////////////////////////////////////////////
    // Iterate and apply all the optimization steps //
    //////////////////////////////////////////////////

    list<OptimizationStep*>::iterator it = _optimizationSteps.begin();

    while (it != _optimizationSteps.end())
    {
        (*it)->optimize();
        it++;
    }

    // No optimizations are performed related to the constant bank.
    // Thus, it�s directly copied.
    _outputConstants = _inputConstants;

    if (_outputCode)
        delete[] _outputCode;

    _outputSizeInBytes = _inputInstrInfoVect.size() * cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE;
    _outputCode = new gal_ubyte[_outputSizeInBytes];
    
    vector<InstructionInfo*>::const_iterator iter = _inputInstrInfoVect.begin();

    gal_ubyte* outputPointer = _outputCode;

    gal_uint listCount = 0;

    // Find the last non-NOP instruction
    gal_uint pointer = _inputInstrInfoVect.size() - 1;

    gal_bool found = false;

    while ( pointer >= 0 && !found)
    {
        if (_inputInstrInfoVect[pointer]->operation.opcode != CG1_ISA_OPCODE_NOP)
            found = true;
        else    
            pointer--;
    }

    gal_uint lastInstr;
    
    if (pointer >= 0)
        lastInstr = pointer;
    else
        lastInstr = 0;

    while ( iter != _inputInstrInfoVect.end() )
    {
        cgoShaderInstr* shInstr;

        // Put the end flag to the last instruction

        if ( listCount == lastInstr )
            shInstr = (*iter)->getShaderInstructionCopy(true);
        else
            shInstr = (*iter)->getShaderInstructionCopy(false);

        shInstr->getCode((U08 *)outputPointer);
        delete shInstr;
        outputPointer += cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE;
        iter++;
        listCount++;
    }

    ////////////////////////////////////////////////////
    // Perform final CG1 architecture code add-ons: //
    ////////////////////////////////////////////////////
    //    1. Add the 16 required NOPS for the shader fetch unit.

    /*gal_ubyte* auxPointer = new gal_ubyte[_outputSizeInBytes + cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE * 16];
    memcpy((void *)auxPointer,(const void*)_outputCode, _outputSizeInBytes);
    
    for (gal_uint i=0; i < 16; i++)
    {
        cgoShaderInstr shInstr(NOP);
        shInstr.getCode((U08 *)(auxPointer + _outputSizeInBytes + i * cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE));
    }

    delete[] _outputCode;

    _outputCode = auxPointer;

    _outputSizeInBytes += cg1gpu::cgoShaderInstr::CG1_ISA_INSTR_SIZE * 16;*/
}

const gal_ubyte* ShaderOptimizer::getCode(gal_uint& sizeInBytes) const
{
    sizeInBytes = _outputSizeInBytes;
    
    return _outputCode;
}

const std::vector<GALVector<gal_float,4> >& ShaderOptimizer::getConstants() const
{
    return _outputConstants;
}

void ShaderOptimizer::getOptOutputInfo(OPTIMIZATION_OUTPUT_INFO& optOutInfo) const
{
    optOutInfo = _optOutInf;
}

ShaderOptimizer::~ShaderOptimizer()
{
    if (_inputCode)
        delete[] _inputCode;

    if (_outputCode)
        delete[] _outputCode;

    vector<InstructionInfo*>::iterator iter = _inputInstrInfoVect.begin();

    while (iter != _inputInstrInfoVect.end())
    {
        delete (*iter);
        iter++;
    }

    // Delete the optimization steps

    list<OptimizationStep*>::iterator iter2 = _optimizationSteps.begin();

    while (iter2 != _optimizationSteps.end())
    {
        delete (*iter2);
        iter2++;
    }
}
