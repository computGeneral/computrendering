/**************************************************************************
 *
 */

#include "ShaderGenerate.h"

using namespace cg1gpu;

ShaderInstructionBuilder::ShaderInstructionBuilder() {
    resetParameters();
}


void ShaderInstructionBuilder::resetParameters() {
    opc = CG1_ISA_OPCODE_NOP;
    for(U32 i = 0; i < 3; i ++) operand[i] = Operand();
    result = Result();
    relMode = RelativeMode();
    lastInstr = false;
}

cgoShaderInstr *ShaderInstructionBuilder::buildInstruction()
{
    return new cgoShaderInstr(
        opc,
        operand[0].registerId.bank , operand[0].registerId.num, operand[0].negate,
        operand[0].absolute, operand[0].swizzle,
        operand[1].registerId.bank , operand[1].registerId.num, operand[1].negate,
        operand[1].absolute, operand[1].swizzle,
        operand[2].registerId.bank , operand[2].registerId.num, operand[2].negate,
        operand[2].absolute, operand[2].swizzle,
        result.registerId.bank,  result.registerId.num, result.saturate,  result.maskMode,
        predication.predicated, predication.negatePredicate, predication.predicateRegister,
        relMode.enabled, relMode.registerN, relMode.component, relMode.offset,
        lastInstr
    );
}

cgoShaderInstr *ShaderInstructionBuilder::copyInstruction(cgoShaderInstr *source)
{
    return new cgoShaderInstr(source->getOpcode(),
                                 source->getBankOp1(), source->getOp1(), source->getOp1NegateFlag(), source->getOp1AbsoluteFlag(),
                                 source->getOp1SwizzleMode(),
                                 source->getBankOp2(), source->getOp2(), source->getOp2NegateFlag(), source->getOp2AbsoluteFlag(),
                                 source->getOp2SwizzleMode(),
                                 source->getBankOp3(), source->getOp3(), source->getOp3NegateFlag(), source->getOp3AbsoluteFlag(),
                                 source->getOp3SwizzleMode(),
                                 source->getBankRes(), source->getResult(), source->getSaturatedRes(), source->getResultMaskMode(),
                                 source->getPredicatedFlag(), source->getNegatePredicateFlag(), source->getPredicateReg(),
                                 source->getRelativeModeFlag(), source->getRelMAddrReg(), source->getRelMAddrRegComp(),
                                 source->getRelMOffset(), source->getEndFlag());
}

list< cgoShaderInstr* >  generate_ending_nops()
{
    list< cgoShaderInstr* >   instructions;
    ShaderInstructionBuilder builder;
    builder.resetParameters();
    builder.setOpcode(CG1_ISA_OPCODE_NOP);
    for(DWORD i = 0; i < 8; i ++)
        instructions.push_back(builder.buildInstruction());
    return instructions;
}

cgoShaderInstr* generate_mov(GPURegisterId dest, GPURegisterId src)
{
    ShaderInstructionBuilder builder;
    builder.setOpcode(CG1_ISA_OPCODE_MOV);
    Operand op;
    op.registerId = src;
    builder.setOperand(0, op);
    Result res;
    res.registerId = dest;
    builder.setResult(res);
    return builder.buildInstruction();
}

void ShaderInstructionBuilder::setOpcode(ShOpcode _opc)
{
    opc = _opc;
}

void ShaderInstructionBuilder::setOperand(U32 i, Operand _operand)
{
    operand[i] = _operand;
}
    
void ShaderInstructionBuilder::setResult(Result _result)
{
    result = _result;
}

void ShaderInstructionBuilder::setPredication(PredicationInfo _predication)
{
    predication = _predication;
}

void ShaderInstructionBuilder::setRelativeMode(RelativeMode _relMode)
{
    relMode = _relMode;
}

void ShaderInstructionBuilder::setLastInstr(bool _lastInstr)
{
    lastInstr = _lastInstr;
}



