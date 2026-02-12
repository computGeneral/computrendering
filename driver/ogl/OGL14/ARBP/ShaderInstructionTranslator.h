/**************************************************************************
 *
 */

#ifndef SHADERINSTRUCTIONTRANSLATOR_H
    #define SHADERINSTRUCTIONTRANSLATOR_H

#include <list>
#include <iostream>

#include "RBank.h"
#include "GenericInstruction.h"
#include "ProgramExecutionEnvironment.h"
#include "ShaderInstr.h"
#include "Scheduler.h"

namespace libgl
{

namespace GenerationCode
{

class ShaderInstructionTranslator: public ShaderCodeGeneration
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

    RBank<F32> paramBank;
    RBank<F32> tempBank;
    
    unsigned int readPortsInBank;
    unsigned int readPortsOutBank;
    unsigned int readPortsParamBank;
    unsigned int readPortsTempBank;
    unsigned int readPortsAddrBank;

    //void clean();

    // Auxiliar functions 
    
    void translateOperandReferences(const GenericInstruction::OperandInfo& genop, ShaderInstrOperand& shop) const;                                                                         
    
    void translateResultReferences(const GenericInstruction::ResultInfo& genres, ShaderInstrResult& shres) const;

    bool thereIsEquivalentInstruction(GenericInstruction::Opcode opc) const;

    bool thereAreOperandAccessConflicts( const ShaderInstrOperand& shop1, const ShaderInstrOperand& shop2,
                                         const ShaderInstrOperand& shop3, unsigned int num_operands,
                                         bool& preloadOp1, bool& preloadOp2) const;
    
    std::list<arch::cgoShaderInstr*> preloadOperands(int line, 
                                                        ShaderInstrOperand& shop1, ShaderInstrOperand& shop2,
                                                        ShaderInstrOperand& shop3,
                                                        bool preloadOp1, bool preloadOp2, 
                                                        unsigned preloadReg1, unsigned preloadReg2) const;
                                             
    
    
    arch::ShOpcode translateOpcode(GenericInstruction::Opcode opc, bool &saturated, bool &texture, bool &sample);
    
    std::list<arch::cgoShaderInstr*> translateInstruction(GenericInstruction::Opcode opcode, 
                                                              int line, int num_operands,
                                                              ShaderInstrOperand shop1,
                                                              ShaderInstrOperand shop2,
                                                              ShaderInstrOperand shop3,
                                                              ShaderInstrResult shres,
                                                              unsigned int textureImageUnit,
                                                              bool lastInstruction);
    
    arch::SwizzleMode composeSwizzles(arch::SwizzleMode swz1, arch::SwizzleMode swz2) const;
    
    std::list<arch::cgoShaderInstr*> generateEquivalentInstructionList(GenericInstruction::Opcode opcode, 
                                                                          int line,int num_operands,
                                                                          ShaderInstrOperand shop1,
                                                                          ShaderInstrOperand shop2,
                                                                          ShaderInstrOperand shop3,
                                                                          ShaderInstrResult shres,
                                                                          GenericInstruction::SwizzleInfo swz,
                                                                          bool& calculatedPreloadReg1,
                                                                          unsigned int& preloadReg1,
                                                                          bool lastInstruction);
    void clean();
    
    void applyOptimizations(bool reorderCode);

public:
    
    // Constructor 
    ShaderInstructionTranslator(RBank<F32>& parametersBank,
                                RBank<F32>& temporariesBank,
                                unsigned int readPortsInBank = 3,
                                unsigned int readPortsOutBank = 0,
                                unsigned int readPortsParamBank = 3,
                                unsigned int readPortsTempBank = 3,
                                unsigned int readPortsAddrBank = 1);
    
    RBank<F32> getParametersBank() {    return paramBank;   }

    void returnGPUCode(U08* bin, U32& size);

    U32 getMaxAliveTemps();
    
    void translateCode( std::list<GenericInstruction*>& lgi, bool optimize = true, bool reorderCode = true);
    
    void returnASMCode(U08 *ptr, U32& size);
    
    void printTranslated(std::ostream& os=std::cout);

    ~ShaderInstructionTranslator();
    
};

} // CodeGeneration

} // namespace libgl

#endif
