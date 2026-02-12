/**************************************************************************
 *
 */

#ifndef GALx_SHADERINSTRUCTIONTRANSLATOR_H
    #define GALx_SHADERINSTRUCTIONTRANSLATOR_H

#include <list>
#include <iostream>

#include "GALxRBank.h"
#include "GALxGenericInstruction.h"
#include "GALxProgramExecutionEnvironment.h"
#include "ShaderInstr.h"

namespace libGAL
{

namespace GenerationCode
{

class GALxShaderInstructionTranslator: public GALxShaderCodeGeneration
{
private:

    class ShaderInstrOperand
    {
    public:
        U32 idReg;
        arch::Bank idBank;
        arch::SwizzleMode swizzleMode;
        bool absoluteFlag;
        bool negateFlag;
        bool relativeModeFlag;
        U32 relModeAddrReg;
        U08 relModeAddrComp;
        S16 relModeOffset;
    
        ShaderInstrOperand();
    };
    
    class ShaderInstrResult 
    {
    public:
        arch::MaskMode writeMask;
        U32 idReg;
        arch::Bank idBank;
        ShaderInstrResult();
    };    
        
    bool maxLiveTempsComputed;
    U32 maxLiveTemps;

    std::list<arch::cgoShaderInstr*> shaderCode; ///< Specific hardware-dependent instruction code

    libGAL::GALxRBank<F32> paramBank;
    libGAL::GALxRBank<F32> tempBank;
    
    unsigned int readPortsInBank;
    unsigned int readPortsOutBank;
    unsigned int readPortsParamBank;
    unsigned int readPortsTempBank;
    unsigned int readPortsAddrBank;

    //void clean();

    // Auxiliar functions 
    
    void translateOperandReferences(const GALxGenericInstruction::OperandInfo& genop, ShaderInstrOperand& shop) const;                                                                         
    
    void translateResultReferences(const GALxGenericInstruction::ResultInfo& genres, ShaderInstrResult& shres) const;

    bool thereIsEquivalentInstruction(GALxGenericInstruction::Opcode opc) const;

    bool thereAreOperandAccessConflicts( const ShaderInstrOperand& shop1, const ShaderInstrOperand& shop2,
                                         const ShaderInstrOperand& shop3, unsigned int num_operands,
                                         bool& preloadOp1, bool& preloadOp2) const;
    
    std::list<arch::cgoShaderInstr*> preloadOperands(int line, 
                                                        ShaderInstrOperand& shop1, ShaderInstrOperand& shop2,
                                                        ShaderInstrOperand& shop3,
                                                        bool preloadOp1, bool preloadOp2, 
                                                        unsigned preloadReg1, unsigned preloadReg2) const;
                                             
    
    
    std::list<arch::cgoShaderInstr*> translateInstruction(GALxGenericInstruction::Opcode opcode, 
                                                         int line, int num_operands,
                                                         ShaderInstrOperand shop1,
                                                         ShaderInstrOperand shop2,
                                                         ShaderInstrOperand shop3,
                                                         ShaderInstrResult shres,
                                                         unsigned int textureImageUnit,
                                                         unsigned int killedSample,
                                                         unsigned int exportSample,
                                                         bool lastInstruction);

    arch::ShOpcode translateOpcode(GALxGenericInstruction::Opcode opc, bool &saturated, bool &texture, bool &sample);    
       
    arch::SwizzleMode composeSwizzles(arch::SwizzleMode swz1, arch::SwizzleMode swz2) const;
    
    std::list<arch::cgoShaderInstr*> generateEquivalentInstructionList(GALxGenericInstruction::Opcode opcode, 
                                                                          int line,int num_operands,
                                                                          ShaderInstrOperand shop1,
                                                                          ShaderInstrOperand shop2,
                                                                          ShaderInstrOperand shop3,
                                                                          ShaderInstrResult shres,
                                                                          GALxGenericInstruction::SwizzleInfo swz,
                                                                          bool& calculatedPreloadReg1,
                                                                          unsigned int& preloadReg1,
                                                                          bool lastInstruction);
    void clean();
    

public:
    
    // Constructor 
    GALxShaderInstructionTranslator(libGAL::GALxRBank<F32>& parametersBank,
                                libGAL::GALxRBank<F32>& temporariesBank,
                                unsigned int readPortsInBank = 3,
                                unsigned int readPortsOutBank = 0,
                                unsigned int readPortsParamBank = 3,
                                unsigned int readPortsTempBank = 3,
                                unsigned int readPortsAddrBank = 1);
    
    libGAL::GALxRBank<F32> getParametersBank() {    return paramBank;   }

    void returnGPUCode(U08* bin, U32& size);

    void translateCode( std::list<GALxGenericInstruction*>& lgi, bool optimize = true, bool reorderCode = true);
    
    void returnASMCode(U08 *ptr, U32& size);
    
    void printTranslated(std::ostream& os=std::cout);

    ~GALxShaderInstructionTranslator();
    
};

} // CodeGeneration

} // namespace libGAL

#endif // GALx_SHADERINSTRUCTIONTRANSLATOR_H
