/**************************************************************************
 *
 * Shader Optimization
 *
 */



/**
 *
 *  @file ShaderOptimization.cpp
 *
 *  Implements the ShaderOptimization class.
 *
 *  This class implements a number of optimization and code transformation passes for shader programs
 *  using the cgoShaderInstr class.
 *
 */


#include "ShaderOptimization.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

using namespace cg1gpu;


//
//
// Support tables
//
//

extern SwizzleMode translateSwizzleModeTable[];

U32 ShaderOptimization::mappingTableComp0[] =
{
    0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3
};

U32 ShaderOptimization::mappingTableComp1[] =
{
    1, 1, 2, 2, 3, 3,
    0, 0, 2, 2, 3, 3,
    0, 0, 1, 1, 3, 3,
    0, 0, 1, 1, 2, 2
};

U32 ShaderOptimization::mappingTableComp2[] =
{
    2, 3, 1, 3, 1, 2,
    2, 3, 0, 3, 0, 2,
    1, 3, 0, 3, 0, 1,
    1, 2, 0, 2, 0, 1
};

U32 ShaderOptimization::mappingTableComp3[] =
{
    3, 2, 3, 1, 2, 1,
    3, 2, 3, 0, 2, 0,
    3, 1, 3, 0, 1, 0,
    2, 1, 2, 0, 1, 0
};


//
//
//  Instruction classification.
//
//   Scalar result (broadcast): CG1_ISA_OPCODE_DP3, CG1_ISA_OPCODE_DP4, CG1_ISA_OPCODE_DPH, CG1_ISA_OPCODE_EX2, CG1_ISA_OPCODE_FRC, CG1_ISA_OPCODE_LG2, CG1_ISA_OPCODE_RCP, CG1_ISA_OPCODE_RSQ
//   Vector operation : CG1_ISA_OPCODE_ADD, CG1_ISA_OPCODE_CMP, CG1_ISA_OPCODE_CMP_CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_MAD, CG1_ISA_OPCODE_FXMAD, CG1_ISA_OPCODE_FXMAD2, CG1_ISA_OPCODE_MAX, CG1_ISA_OPCODE_MIN, CG1_ISA_OPCODE_MOV, CG1_ISA_OPCODE_MUL, CG1_ISA_OPCODE_FXMUL, CG1_ISA_OPCODE_SGE, CG1_ISA_OPCODE_SLT
//   SIMD4 result : CG1_ISA_OPCODE_DST, CG1_ISA_OPCODE_EXP, CG1_ISA_OPCODE_LDA, CG1_ISA_OPCODE_LIT, CG1_ISA_OPCODE_LOG, CG1_ISA_OPCODE_TEX, CG1_ISA_OPCODE_TXB, CG1_ISA_OPCODE_TXL, CG1_ISA_OPCODE_TXP
//   No result : CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP, CG1_ISA_OPCODE_ZXS, CG1_ISA_OPCODE_NOP, CG1_ISA_OPCODE_CHS
//   Address register result: CG1_ISA_OPCODE_ARL
//   Not implemented : CG1_ISA_OPCODE_FLR
//

bool ShaderOptimization::hasScalarResult(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_DP3) || (opc == CG1_ISA_OPCODE_DP4) || (opc == CG1_ISA_OPCODE_DPH) || (opc == CG1_ISA_OPCODE_EX2) ||
           (opc == CG1_ISA_OPCODE_FRC) || (opc == CG1_ISA_OPCODE_LG2) || (opc == CG1_ISA_OPCODE_RCP) || (opc == CG1_ISA_OPCODE_RSQ) ||
           (opc == CG1_ISA_OPCODE_COS) || (opc == CG1_ISA_OPCODE_SIN);
}

bool ShaderOptimization::isVectorOperation(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_ADD)    || (opc == CG1_ISA_OPCODE_CMP) || (opc == CG1_ISA_OPCODE_CMPKIL)  || (opc == CG1_ISA_OPCODE_MAD) || (opc == CG1_ISA_OPCODE_FXMAD) ||
           (opc == CG1_ISA_OPCODE_FXMAD2) || (opc == CG1_ISA_OPCODE_MAX) || (opc == CG1_ISA_OPCODE_MIN)     || (opc == CG1_ISA_OPCODE_MOV) || (opc == CG1_ISA_OPCODE_MUL)   ||
           (opc == CG1_ISA_OPCODE_FXMUL)  || (opc == CG1_ISA_OPCODE_SGE) || (opc == CG1_ISA_OPCODE_SLT)     || (opc == CG1_ISA_OPCODE_DDX) || (opc == CG1_ISA_OPCODE_DDY) ||
           (opc == CG1_ISA_OPCODE_ADDI) || (opc == CG1_ISA_OPCODE_MULI);
}

bool ShaderOptimization::hasSIMD4Result(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_DST) || (opc == CG1_ISA_OPCODE_EXP) || (opc == CG1_ISA_OPCODE_LDA) || (opc == CG1_ISA_OPCODE_LIT) ||
           (opc == CG1_ISA_OPCODE_LOG) || (opc == CG1_ISA_OPCODE_TEX) || (opc == CG1_ISA_OPCODE_TXB) || (opc == CG1_ISA_OPCODE_TXL) ||
           (opc == CG1_ISA_OPCODE_TXP);
}

bool ShaderOptimization::hasNoResult(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_KIL) || (opc == CG1_ISA_OPCODE_KLS) || (opc == CG1_ISA_OPCODE_ZXP) || (opc == CG1_ISA_OPCODE_ZXS) || (opc == CG1_ISA_OPCODE_NOP) || (opc == CG1_ISA_OPCODE_CHS) || (opc == CG1_ISA_OPCODE_JMP);
}

bool ShaderOptimization::hasAddrRegResult(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_ARL);
}

bool ShaderOptimization::notImplemented(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_FLR);
}

bool ShaderOptimization::mustDisableEarlyZ(ShOpcode opc)
{
    return (opc == CG1_ISA_OPCODE_KIL) || (opc == CG1_ISA_OPCODE_KLS) || (opc == CG1_ISA_OPCODE_ZXP) || (opc == CG1_ISA_OPCODE_ZXS);
}


//
//
//  Support functions for the optimization and transformation passes.
//
//


cgoShaderInstr *ShaderOptimization::copyInstruction(cgoShaderInstr *origInstr)
{
    cgoShaderInstr *copyInstr;

    copyInstr = new cgoShaderInstr(
                        //  Opcode
                        origInstr->getOpcode(),
                        //  First operand
                        origInstr->getBankOp1(), origInstr->getOp1(), origInstr->getOp1NegateFlag(),
                        origInstr->getOp1AbsoluteFlag(), origInstr->getOp1SwizzleMode(),
                        //  Second operand
                        origInstr->getBankOp2(), origInstr->getOp2(), origInstr->getOp2NegateFlag(),
                        origInstr->getOp2AbsoluteFlag(), origInstr->getOp2SwizzleMode(),
                        //  Third operand
                        origInstr->getBankOp3(), origInstr->getOp3(), origInstr->getOp3NegateFlag(),
                        origInstr->getOp3AbsoluteFlag(), origInstr->getOp3SwizzleMode(),
                        //  Result
                        origInstr->getBankRes(), origInstr->getResult(), origInstr->getSaturatedRes(),
                        origInstr->getResultMaskMode(),
                        //  Predication
                        origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                        //  Relative addressing mode
                        origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                        origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                        //  End flag
                        origInstr->getEndFlag()
                    );

    return copyInstr;
}

cgoShaderInstr *ShaderOptimization::patchedOpsInstruction(cgoShaderInstr *origInstr, U32 patchRegOp1, Bank patchBankOp1,
                                         U32 patchRegOp2, Bank patchBankOp2, U32 patchRegOp3, Bank patchBankOp3)
{
    cgoShaderInstr *patchedInstr;

    patchedInstr = new cgoShaderInstr(
                            //  Opcode
                            origInstr->getOpcode(),
                            //  First operand
                            patchBankOp1, patchRegOp1, origInstr->getOp1NegateFlag(), origInstr->getOp1AbsoluteFlag(),
                            origInstr->getOp1SwizzleMode(),
                            //  Second operand
                            patchBankOp2, patchRegOp2, origInstr->getOp2NegateFlag(), origInstr->getOp2AbsoluteFlag(),
                            origInstr->getOp2SwizzleMode(),
                            //  Third operand
                            patchBankOp3, patchRegOp3, origInstr->getOp3NegateFlag(), origInstr->getOp3AbsoluteFlag(),
                            origInstr->getOp3SwizzleMode(),
                            //  Result
                            origInstr->getBankRes(), origInstr->getResult(), origInstr->getSaturatedRes(),
                            origInstr->getResultMaskMode(),
                            //  Predication
                            origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                            //  Relative addressing mode
                            origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                            origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                            //  End flag
                            origInstr->getEndFlag()
                      );

    return patchedInstr;
}

cgoShaderInstr *ShaderOptimization::patchResMaskInstruction(cgoShaderInstr *origInstr, MaskMode resultMask)
{
    cgoShaderInstr *patchedInstr;

    patchedInstr = new cgoShaderInstr(
                        //  Opcode
                        origInstr->getOpcode(),
                        //  First operand
                        origInstr->getBankOp1(), origInstr->getOp1(), origInstr->getOp1NegateFlag(),
                        origInstr->getOp1AbsoluteFlag(), origInstr->getOp1SwizzleMode(),
                        //  Second operand
                        origInstr->getBankOp2(), origInstr->getOp2(), origInstr->getOp2NegateFlag(),
                        origInstr->getOp2AbsoluteFlag(), origInstr->getOp2SwizzleMode(),
                        //  Third operand
                        origInstr->getBankOp3(), origInstr->getOp3(), origInstr->getOp3NegateFlag(),
                        origInstr->getOp3AbsoluteFlag(), origInstr->getOp3SwizzleMode(),
                        //  Result
                        origInstr->getBankRes(), origInstr->getResult(), origInstr->getSaturatedRes(),
                        resultMask,
                        //  Predication
                        origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                        //  Relative addressing mode
                        origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                        origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                        //  End flag
                        origInstr->getEndFlag()
                    );

    return patchedInstr;
}


cgoShaderInstr *ShaderOptimization::renameRegsInstruction(cgoShaderInstr *origInstr, U32 resName, U32 op1Name, U32 op2Name, U32 op3Name)
{
    cgoShaderInstr *renamedInstr;

    renamedInstr = new cgoShaderInstr(
                        //  Opcode
                        origInstr->getOpcode(),
                        //  First operand
                        origInstr->getBankOp1(), op1Name, origInstr->getOp1NegateFlag(),
                        origInstr->getOp1AbsoluteFlag(), origInstr->getOp1SwizzleMode(),
                        //  Second operand
                        origInstr->getBankOp2(), op2Name, origInstr->getOp2NegateFlag(),
                        origInstr->getOp2AbsoluteFlag(), origInstr->getOp2SwizzleMode(),
                        //  Third operand
                        origInstr->getBankOp3(), op3Name, origInstr->getOp3NegateFlag(),
                        origInstr->getOp3AbsoluteFlag(), origInstr->getOp3SwizzleMode(),
                        //  Result
                        origInstr->getBankRes(), resName, origInstr->getSaturatedRes(),
                        origInstr->getResultMaskMode(),
                        //  Predication
                        origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                        //  Relative addressing mode
                        origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                        origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                        //  End flag
                        origInstr->getEndFlag()
                    );

    return renamedInstr;
}

cgoShaderInstr *ShaderOptimization::setRegsAndCompsInstruction(cgoShaderInstr *origInstr, U32 resReg, MaskMode resMask,
                                              U32 op1Reg, SwizzleMode op1Swz, U32 op2Reg, SwizzleMode op2Swz, U32 op3Reg, SwizzleMode op3Swz)
{
    cgoShaderInstr *setRegCompsInstr;

    setRegCompsInstr = new cgoShaderInstr(
                        //  Opcode
                        origInstr->getOpcode(),
                        //  First operand
                        origInstr->getBankOp1(), op1Reg, origInstr->getOp1NegateFlag(),
                        origInstr->getOp1AbsoluteFlag(), op1Swz,
                        //  Second operand
                        origInstr->getBankOp2(), op2Reg, origInstr->getOp2NegateFlag(),
                        origInstr->getOp2AbsoluteFlag(), op2Swz,
                        //  Third operand
                        origInstr->getBankOp3(), op3Reg, origInstr->getOp3NegateFlag(),
                        origInstr->getOp3AbsoluteFlag(), op3Swz,
                        //  Result
                        origInstr->getBankRes(), resReg, origInstr->getSaturatedRes(),
                        resMask,
                        //  Predication
                        origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                        //  Relative addressing mode
                        origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                        origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                        //  End flag
                        origInstr->getEndFlag()
                    );

    return setRegCompsInstr;
}

cgoShaderInstr *ShaderOptimization::patchSOAInstruction(cgoShaderInstr *origInstr,
                                                           SwizzleMode op1Comp, SwizzleMode op2Comp, SwizzleMode op3Comp, MaskMode resComp)
{
    cgoShaderInstr *soaInstr;

    soaInstr = new cgoShaderInstr(
                        //  Opcode
                        origInstr->getOpcode(),
                        //  First operand
                        origInstr->getBankOp1(), origInstr->getOp1(), origInstr->getOp1NegateFlag(),
                        origInstr->getOp1AbsoluteFlag(), op1Comp,
                        //  Second operand
                        origInstr->getBankOp2(), origInstr->getOp2(), origInstr->getOp2NegateFlag(),
                        origInstr->getOp2AbsoluteFlag(), op2Comp,
                        //  Third operand
                        origInstr->getBankOp3(), origInstr->getOp3(), origInstr->getOp3NegateFlag(),
                        origInstr->getOp3AbsoluteFlag(), op3Comp,
                        //  Result
                        origInstr->getBankRes(), origInstr->getResult(), origInstr->getSaturatedRes(),
                        resComp,
                        //  Predication
                        origInstr->getPredicatedFlag(), origInstr->getNegatePredicateFlag(), origInstr->getPredicateReg(),
                        //  Relative addressing mode
                        origInstr->getRelativeModeFlag(), origInstr->getRelMAddrReg(),
                        origInstr->getRelMAddrRegComp(), origInstr->getRelMOffset(),
                        //  End flag
                        false
                    );

    return soaInstr;
}


void ShaderOptimization::updateTempRegsUsed(cgoShaderInstr *instr, bool *tempInUse)
{
    switch(instr->getOpcode())
    {
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_CHS:

            //  Don't use any register.

            break;

        case CG1_ISA_OPCODE_FLR:

            //  Unimplemented.

            break;

        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:
        case CG1_ISA_OPCODE_JMP:

            //  Does not use the result register.

            break;

        default:

            //  Check result register bank.
            if (instr->getBankRes() == TEMP)
                tempInUse[instr->getResult()] = true;

            //  NOTE:  Temporal registers shouldn't be used if not previously defined!!

            //  Check first operand bank.
            if ((instr->getNumOperands() >= 1) && (instr->getBankOp1() == TEMP))
            {
                //tempInUse[instr->getOp1()] = true;

                //  Check if register was written before being used.
                if (!tempInUse[instr->getOp1()])
                {
                    char buffer[256];
                    sprintf_s(buffer, sizeof(buffer), "Temporal register %d used with no assigned value.", instr->getOp1());
                    CG_ASSERT(buffer);
                }
            }

            //  Check second operand bank.
            if ((instr->getNumOperands() >= 2) && (instr->getBankOp2() == TEMP))
            {
                //tempInUse[instr->getOp2()] = true;

                //  Check if register was written before being used.
                if (!tempInUse[instr->getOp2()])
                {
                    char buffer[256];
                    sprintf_s(buffer, sizeof(buffer), "Temporal register %d used with no assigned value.", instr->getOp2());
                    CG_ASSERT(buffer);
                }
            }

            //  Check third operand bank.
            if ((instr->getNumOperands() == 3) && (instr->getBankOp3() == TEMP))
            {
                //tempInUse[instr->getOp3()] = true;

                //  Check if register was written before being used.
                if (!tempInUse[instr->getOp3()])
                {
                    char buffer[256];
                    sprintf_s(buffer, sizeof(buffer), "Temporal register %d used with no assigned value.", instr->getOp3());
                    CG_ASSERT(buffer);
                }
            }

            break;
    }
}

void ShaderOptimization::updateInstructionType(cgoShaderInstr *instr, unsigned int *instrType)
{
    switch(instr->getOpcode())
    {
        case CG1_ISA_OPCODE_FLR:

            //  Unimplemented.

            break;

        case CG1_ISA_OPCODE_LDA:
        case CG1_ISA_OPCODE_TEX:
        case CG1_ISA_OPCODE_TXB:
        case CG1_ISA_OPCODE_TXL:
        case CG1_ISA_OPCODE_TXP:

            // Texture/Load instructions.
            instrType[TEX_INSTR_TYPE]++;

            break;

        case CG1_ISA_OPCODE_ARL:
        case CG1_ISA_OPCODE_DPH:
        case CG1_ISA_OPCODE_DP3:
        case CG1_ISA_OPCODE_DP4:
        case CG1_ISA_OPCODE_DST:
        case CG1_ISA_OPCODE_LIT:
        case CG1_ISA_OPCODE_EX2:
        case CG1_ISA_OPCODE_EXP:
        case CG1_ISA_OPCODE_FRC:
        case CG1_ISA_OPCODE_LG2:
        case CG1_ISA_OPCODE_LOG:
        case CG1_ISA_OPCODE_RCP:
        case CG1_ISA_OPCODE_RSQ:
        case CG1_ISA_OPCODE_MUL:
        case CG1_ISA_OPCODE_FXMUL:
        case CG1_ISA_OPCODE_MAD:
        case CG1_ISA_OPCODE_FXMAD:
        case CG1_ISA_OPCODE_FXMAD2:
        case CG1_ISA_OPCODE_MAX:
        case CG1_ISA_OPCODE_MIN:
        case CG1_ISA_OPCODE_MOV:
        case CG1_ISA_OPCODE_SGE:
        case CG1_ISA_OPCODE_SLT:
        case CG1_ISA_OPCODE_CMP:
        case CG1_ISA_OPCODE_ADD:
        case CG1_ISA_OPCODE_SETPEQ:
        case CG1_ISA_OPCODE_SETPGT:
        case CG1_ISA_OPCODE_SETPLT:
        case CG1_ISA_OPCODE_ANDP:
        case CG1_ISA_OPCODE_DDX:
        case CG1_ISA_OPCODE_DDY:
        case CG1_ISA_OPCODE_ADDI:
        case CG1_ISA_OPCODE_MULI:
        case CG1_ISA_OPCODE_STPEQI:
        case CG1_ISA_OPCODE_STPGTI:
        case CG1_ISA_OPCODE_STPLTI:

            // ALU instructions.
            instrType[ALU_INSTR_TYPE]++;

            break;

        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:
        case CG1_ISA_OPCODE_CHS:
        case CG1_ISA_OPCODE_JMP:

            // Control instructions.
            instrType[CTRL_INSTR_TYPE]++;

            break;

        default:

            break;
    }
}

void ShaderOptimization::testSwizzle(SwizzleMode swizzle, bool& includesX, bool& includesY, bool& includesZ, bool& includesW)
{
    includesX = false;
    includesY = false;
    includesZ = false;
    includesW = false;

    switch((swizzle & 0xC0) >> 6)
    {
        case 0x00: includesX = true; break;
        case 0x01: includesY = true; break;
        case 0x02: includesZ = true; break;
        case 0x03: includesW = true; break;
    }

    switch((swizzle & 0x30) >> 4)
    {
        case 0x00: includesX = true; break;
        case 0x01: includesY = true; break;
        case 0x02: includesZ = true; break;
        case 0x03: includesW = true; break;
    }

    switch((swizzle & 0x0C) >> 2)
    {
        case 0x00: includesX = true; break;
        case 0x01: includesY = true; break;
        case 0x02: includesZ = true; break;
        case 0x03: includesW = true; break;
    }

    switch((swizzle & 0x03))
    {
        case 0x00: includesX = true; break;
        case 0x01: includesY = true; break;
        case 0x02: includesZ = true; break;
        case 0x03: includesW = true; break;
    }
}

void ShaderOptimization::getTempRegsUsed(vector<cgoShaderInstr *> inProgram, bool *tempInUse)
{
    //  Clear the in use flag for all the temporal registers.
    for(U32 reg = 0; reg < MAX_TEMPORAL_REGISTERS; reg++)
        tempInUse[reg] = false;

    //  Detect which temporal registers are in use.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        updateTempRegsUsed(inProgram[instr], tempInUse);
    }
}

void ShaderOptimization::extractOperandComponents(SwizzleMode opSwizzle, vector<SwizzleMode> &components)
{
    components.clear();
    components.resize(4);

    U32 swizzleMode = opSwizzle;

    //  The components are ordered from first component to fourth component
    //  from the MSB to the LSB of the swizzle mode mask.
    for(U32 comp = 4; comp > 0; comp--)
    {
        switch (swizzleMode & 0x03)
        {
            case 0:  //  Select X component.
                components[comp - 1] = XXXX;
                break;

            case 1: //  Select Y component.
                components[comp - 1] = YYYY;
                break;

            case 2: //  Select Y component.
                components[comp - 1] = ZZZZ;
                break;

            case 3: //  Select Y component.
                components[comp - 1] = WWWW;
                break;

        }

        //  Shift to next component in the swizzle mask.
        swizzleMode = swizzleMode >> 2;
    }
}

void ShaderOptimization::extractResultComponents(MaskMode resMask, vector<MaskMode> &componentsMask, vector<SwizzleMode> &componentsSwizzle)
{
    componentsMask.clear();
    componentsSwizzle.clear();

    switch(resMask)
    {
        case NNNN:
            break;

        case NNNW:
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case NNZN:
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            break;

        case NNZW:
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case NYNN:
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            break;

        case NYNW:
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case NYZN:
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            break;

        case NYZW:
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case XNNN:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            break;

        case XNNW:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case XNZN:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            break;

        case XNZW:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case XYNN:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            break;

        case XYNW:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;

        case XYZN:
            componentsMask.push_back(XNNN);
           componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            break;

        case mXYZW:
            componentsMask.push_back(XNNN);
            componentsSwizzle.push_back(XXXX);
            componentsMask.push_back(NYNN);
            componentsSwizzle.push_back(YYYY);
            componentsMask.push_back(NNZN);
            componentsSwizzle.push_back(ZZZZ);
            componentsMask.push_back(NNNW);
            componentsSwizzle.push_back(WWWW);
            break;
    }
}

void ShaderOptimization::getReadOperandComponents(cgoShaderInstr *instr, vector<SwizzleMode> resComponentsSwizzle,
                                                  vector<U32> &readComponentsOp1, vector<U32> &readComponentsOp2)
{

    //  The list of components written in the result register will be used as the reference
    //  to determine which components of the operand registers were actually read.
    //  However for some instructions there is no direct relation between the components written
    //  and the components read so we need to create a special list of components.
    switch(instr->getOpcode())
    {
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_CHS:

            //  These instructions don't set or use any register.

            break;

        case CG1_ISA_OPCODE_FLR:

            //  Unimplemented.

            break;

        case CG1_ISA_OPCODE_JMP:
        
            //  A single component is read.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            break;
            
        case CG1_ISA_OPCODE_DP4:
        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:

            //  For those instructions all the operand components are read.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            readComponentsOp1.push_back(1);
            readComponentsOp1.push_back(2);
            readComponentsOp1.push_back(3);

            readComponentsOp2.clear();
            readComponentsOp2.push_back(0);
            readComponentsOp2.push_back(1);
            readComponentsOp2.push_back(2);
            readComponentsOp2.push_back(3);

            break;

        case CG1_ISA_OPCODE_TXB:
        case CG1_ISA_OPCODE_TXL:
        case CG1_ISA_OPCODE_TXP:

            //  For those instructions all the operand components are read.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            readComponentsOp1.push_back(1);
            readComponentsOp1.push_back(2);
            readComponentsOp1.push_back(3);
            
            readComponentsOp2.clear();
            //readComponentsOp2.push_back(0);
            //readComponentsOp2.push_back(1);
            //readComponentsOp2.push_back(2);
            //readComponentsOp2.push_back(3);


            break;

        case CG1_ISA_OPCODE_DPH:

            //  CG1_ISA_OPCODE_DPH is a very special case.  From the first operand the three first components are read.
            //  But for the second operand all four components are read.

            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            readComponentsOp1.push_back(1);
            readComponentsOp1.push_back(2);

            readComponentsOp1.clear();
            readComponentsOp2.push_back(0);
            readComponentsOp2.push_back(1);
            readComponentsOp2.push_back(2);
            readComponentsOp2.push_back(3);

            break;

        case CG1_ISA_OPCODE_DP3:

            //  The three first components are read.  Not affected by the result components.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            readComponentsOp1.push_back(1);
            readComponentsOp1.push_back(2);

            readComponentsOp2.clear();
            readComponentsOp2.push_back(0);
            readComponentsOp2.push_back(1);
            readComponentsOp2.push_back(2);

            break;

        case CG1_ISA_OPCODE_TEX:

            //  The three first components are read.  Not affected by the result components.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);
            readComponentsOp1.push_back(1);
            readComponentsOp1.push_back(2);
            
            readComponentsOp2.clear();
            //readComponentsOp2.push_back(0);
            //readComponentsOp2.push_back(1);
            //readComponentsOp2.push_back(2);

            break;

        case CG1_ISA_OPCODE_LDA:

            //  CG1_ISA_OPCODE_LDA only reads the index from the first component.
            //  Only the first operand is actually read.
            readComponentsOp1.clear();
            readComponentsOp1.push_back(0);

            readComponentsOp2.clear();
            //readComponentsOp2.push_back(0);

            break;

        case CG1_ISA_OPCODE_DST:

            //  CG1_ISA_OPCODE_DST is a very special case.
            //
            //  For the first operand:
            //
            //    1st component is not read
            //    2nd component is read if the 2nd result component is written
            //    3rd component is read if the 3rd result component is written
            //    4th component is read if the 4th result component is written
            //
            //  For the second operand:
            //
            //    2nd component is read if the 2nd result component is written
            //    all other components are not read
            //

            readComponentsOp1.clear();
            readComponentsOp2.clear();

            for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
            {
                if ((resComponentsSwizzle[comp] & 0x03) != 0)
                    readComponentsOp1.push_back(resComponentsSwizzle[comp] & 0x03);

                if ((resComponentsSwizzle[comp] & 0x03) == 1)
                    readComponentsOp2.push_back(1);
            }

            break;

        case CG1_ISA_OPCODE_LIT:

            //  CG1_ISA_OPCODE_LIT is a special case.
            //
            //  CG1_ISA_OPCODE_LIT has a single operand:
            //
            //    1st component is read if 2nd or 3rd result components are written
            //    2nd component is read if 3rd result component is written
            //    4th component is read if 3rd result component is written
            //

            {
                readComponentsOp1.clear();
                readComponentsOp2.clear();

                bool secondResCompWritten = false;
                bool thirdResCompWritten = false;


                for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                {
                    secondResCompWritten = secondResCompWritten || ((resComponentsSwizzle[comp] & 0x03) == 1);
                    thirdResCompWritten = thirdResCompWritten || ((resComponentsSwizzle[comp] & 0x03) == 2);
                }

                if (thirdResCompWritten)
                {
                    readComponentsOp1.push_back(0);
                    readComponentsOp1.push_back(1);
                    readComponentsOp1.push_back(3);
                }
                else if (secondResCompWritten)
                {
                    readComponentsOp1.push_back(0);
                }
            }

            break;

        case CG1_ISA_OPCODE_EX2:
        case CG1_ISA_OPCODE_EXP:
        case CG1_ISA_OPCODE_FRC:
        case CG1_ISA_OPCODE_LG2:
        case CG1_ISA_OPCODE_LOG:
        case CG1_ISA_OPCODE_RCP:
        case CG1_ISA_OPCODE_RSQ:
        case CG1_ISA_OPCODE_SETPEQ:
        case CG1_ISA_OPCODE_SETPGT:
        case CG1_ISA_OPCODE_SETPLT:
        case CG1_ISA_OPCODE_ANDP:
        case CG1_ISA_OPCODE_STPEQI:
        case CG1_ISA_OPCODE_STPGTI:
        case CG1_ISA_OPCODE_STPLTI:

            //  Scalar operations but with result broadcast (result mask may not be a single component).
            //  For scalar instructions the swizzle mask for the only existing operand should always
            //  be a scalar/broadcast (.x, .y, .z or .w).  As the result can be broadcasted to multiple
            //  result components we need to return multiple read components.

            readComponentsOp1.clear();
            readComponentsOp2.clear();

            for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
            {
                readComponentsOp1.push_back(resComponentsSwizzle[comp] & 0x03);
                readComponentsOp2.push_back(resComponentsSwizzle[comp] & 0x03);
            }

            break;

        default:

            //  For all other cases the read components depend on the written components in
            //  the result register.

            readComponentsOp1.clear();
            readComponentsOp2.clear();

            for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
            {
                readComponentsOp1.push_back(resComponentsSwizzle[comp] & 0x03);
                readComponentsOp2.push_back(resComponentsSwizzle[comp] & 0x03);
            }

            break;
    }
}

//
//
//  Shader program management.
//

void ShaderOptimization::copyProgram(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram)
{
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        cgoShaderInstr *copyInstr;

        copyInstr = copyInstruction(inProgram[instr]);
        outProgram.push_back(copyInstr);
    }
}

void ShaderOptimization::concatenateProgram(vector<cgoShaderInstr *> srcProgram, vector<cgoShaderInstr *>& destProgram)

{
    if (!destProgram.empty())
    {
        //  Reset end flag in the last instruction
        destProgram[destProgram.size() - 1]->setEndFlag(false);
    }

    //  Concatenate source shader code.
    for(U32 instr = 0; instr < srcProgram.size(); instr++)
    {
        cgoShaderInstr *copyInstr;

        //  Copy the instruction.
        copyInstr = copyInstruction(srcProgram[instr]);
        destProgram.push_back(copyInstr);
    }

}

void ShaderOptimization::deleteProgram(vector<cgoShaderInstr *> &inProgram)
{
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        delete inProgram[instr];
    }

    inProgram.clear();
}

void ShaderOptimization::printProgram(vector<cgoShaderInstr *> inProgram)
{
    //  Disassemble the program.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        char instrDisasm[256];
        U08 instrCode[16];

        //  Disassemble the instruction.
        inProgram[instr]->disassemble(instrDisasm, MAX_INFO_SIZE);

        //  Get the instruction code.
        inProgram[instr]->getCode(instrCode);

        //  Print the instruction.
        printf("%04x :", instr * 16);
        for(U32 b = 0; b < 16; b++)
            printf(" %02x", instrCode[b]);
        printf("    %s\n", instrDisasm);
    }
}

MaskMode ShaderOptimization::encodeWriteMask(bool *activeResComp)
{
    if (activeResComp[0] && activeResComp[1] && activeResComp[2] && activeResComp[3])
        return mXYZW;
    else if (activeResComp[0] && activeResComp[1] && activeResComp[2])
        return XYZN;
    else if (activeResComp[0] && activeResComp[1] && activeResComp[3])
        return XYNW;
    else if (activeResComp[0] && activeResComp[2] && activeResComp[3])
        return XNZW;
    else if (activeResComp[1] && activeResComp[2] && activeResComp[3])
        return NYZW;
    else if (activeResComp[0] && activeResComp[1])
        return XYNN;
    else if (activeResComp[0] && activeResComp[2])
        return XNZN;
    else if (activeResComp[0] && activeResComp[3])
        return XNNW;
    else if (activeResComp[1] && activeResComp[2])
        return NYZN;
    else if (activeResComp[1] && activeResComp[3])
        return NYNW;
    else if (activeResComp[2] && activeResComp[3])
        return NNZW;
    else if (activeResComp[0])
        return XNNN;
    else if (activeResComp[1])
        return NYNN;
    else if (activeResComp[2])
        return NNZN;
    else if (activeResComp[3])
        return NNNW;
    else
        return NNNN;
}

MaskMode ShaderOptimization::removeComponentsFromWriteMask(MaskMode inResMask, bool *removeResComp)
{
    bool activeResComp[4];

    activeResComp[0] = (inResMask == XNNN) || (inResMask == XNNW) || (inResMask == XNZN) ||
                       (inResMask == XNZW) || (inResMask == XYNN) || (inResMask == XYNW) ||
                       (inResMask == XYZN) || (inResMask == mXYZW);

    activeResComp[1] = (inResMask == NYNN) || (inResMask == NYNW) || (inResMask == NYZN) ||
                       (inResMask == NYZW) || (inResMask == XYNN) || (inResMask == XYNW) ||
                       (inResMask == XYZN) || (inResMask == mXYZW);

    activeResComp[2] = (inResMask == NNZN) || (inResMask == NNZW) || (inResMask == NYZN) ||
                       (inResMask == NYZW) || (inResMask == XNZN) || (inResMask == XNZW) ||
                       (inResMask == XYZN) || (inResMask == mXYZW);

    activeResComp[3] = (inResMask == NNNW) || (inResMask == NNZW) || (inResMask == NYNW) ||
                       (inResMask == NYZW) || (inResMask == XNNW) || (inResMask == XNZW) ||
                       (inResMask == XYNW) || (inResMask == mXYZW);

    activeResComp[0] = activeResComp[0] && !removeResComp[0];
    activeResComp[1] = activeResComp[1] && !removeResComp[1];
    activeResComp[2] = activeResComp[2] && !removeResComp[2];
    activeResComp[3] = activeResComp[3] && !removeResComp[3];

    return encodeWriteMask(activeResComp);
}

SwizzleMode ShaderOptimization::encodeSwizzle(U32 *opSwizzle)
{
    U32 swizzleMode = 0;

    for(U32 comp = 0; comp < 4; comp++)
    {
        swizzleMode = (swizzleMode << 2) | (opSwizzle[comp] & 0x03);
    }

    //return translateSwizzleMode[swizzleMode];
    return translateSwizzleModeTable[swizzleMode];
}

//
//  Transformation and optimization passes.
//
//

void ShaderOptimization::attribute2lda(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram)
{
    bool tempInUse[MAX_TEMPORAL_REGISTERS];

    getTempRegsUsed(inProgram, tempInUse);

    outProgram.clear();

    U32 input2temp[MAX_INPUT_ATTRIBUTES];

    for(U32 attr = 0; attr < MAX_INPUT_ATTRIBUTES; attr++)
        input2temp[attr] = MAX_TEMPORAL_REGISTERS;

    //  Now convert input attribute registers into temporal registers loaded using CG1_ISA_OPCODE_LDA.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        //  Check CG1_ISA_OPCODE_MOV instructions.
        if ((inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_MOV) && (inProgram[instr]->getBankOp1() == IN))
        {
            //  Check if the attribute was already loaded into a temporal register.
            if (input2temp[inProgram[instr]->getOp1()] == MAX_TEMPORAL_REGISTERS)
            {
                //  If the attribute was not already loaded check if the attribute data
                //  is modified by swizzling or negate, absolute modifiers.
                if (inProgram[instr]->getOp1AbsoluteFlag() || inProgram[instr]->getOp1NegateFlag() ||
                    inProgram[instr]->getOp1SwizzleMode() != XYZW)
                {
                    //  Add CG1_ISA_OPCODE_LDA instruction to load attribute into a temporal register.

                    U32 newTemp;

                    //  Select the next free temporal register to store the attribute.
                    for(newTemp = 0; (newTemp < MAX_TEMPORAL_REGISTERS) && tempInUse[newTemp]; newTemp++);

                    //  Check if there are no more temporal registers.
                    if (newTemp == MAX_TEMPORAL_REGISTERS)
                    {
                        CG_ASSERT("There no unused temporal registers available.");
                    }

                    //  Set temporal register as in use and associate with attribute.
                    tempInUse[newTemp] = true;
                    input2temp[inProgram[instr]->getOp1()] = newTemp;

                    cgoShaderInstr *newCG1_ISA_OPCODE_LDAInstr;

                    newCG1_ISA_OPCODE_LDAInstr = new cgoShaderInstr(
                                            //  Opcode
                                            CG1_ISA_OPCODE_LDA,
                                            //  First operand
                                            IN, INDEX_ATTRIBUTE, false, false, XXXX,
                                            //  Second operand
                                            TEXT, inProgram[instr]->getOp1(), false, false, XYZW,
                                            //  Third operand
                                            INVALID, 0, false, false, XYZW,
                                            //  Result
                                            TEMP, newTemp, false, mXYZW,
                                            //  Predication
                                            false, false, 0,
                                            //  Relative addressing mode
                                            false, 0, 0, 0,
                                            //  End flag
                                            false
                                      );

                    outProgram.push_back(newCG1_ISA_OPCODE_LDAInstr);

                    //  Patch the CG1_ISA_OPCODE_MOV instruction with the new temporal register.
                    cgoShaderInstr *newMOVInstr;

                    newMOVInstr = patchedOpsInstruction(inProgram[instr], newTemp, TEMP,
                                                        inProgram[instr]->getOp2(), inProgram[instr]->getBankOp2(),
                                                        inProgram[instr]->getOp3(), inProgram[instr]->getBankOp3());

                    outProgram.push_back(newMOVInstr);
                }
                else
                {
                    //  MOVs from an input attribute register are directly converted into CG1_ISA_OPCODE_LDA when
                    //  the read value is not modified (abs, neg or swizzle).
                    cgoShaderInstr *newCG1_ISA_OPCODE_LDAInstr;

                    newCG1_ISA_OPCODE_LDAInstr = new cgoShaderInstr(
                                            //  Opcode
                                            CG1_ISA_OPCODE_LDA,
                                            //  First operand
                                            IN, INDEX_ATTRIBUTE, false, false, XXXX,
                                            //  Second operand
                                            TEXT, inProgram[instr]->getOp1(), false, false, XYZW,
                                            //  Third operand
                                            INVALID, 0, false, false, XYZW,
                                            //  Result
                                            inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), inProgram[instr]->getSaturatedRes(),
                                            inProgram[instr]->getResultMaskMode(),
                                            //  Predication
                                            false, false, 0,
                                            //  Relative addressing mode
                                            false, 0, 0, 0,
                                            //  End flag
                                            inProgram[instr]->getEndFlag()
                                      );

                    outProgram.push_back(newCG1_ISA_OPCODE_LDAInstr);
                }
            }
            else
            {
                //  Recode the CG1_ISA_OPCODE_MOV instruction to use the temporal register.
                cgoShaderInstr *newMOVInstr;

                newMOVInstr = patchedOpsInstruction(inProgram[instr], input2temp[inProgram[instr]->getOp1()], TEMP,
                                                    inProgram[instr]->getOp2(), inProgram[instr]->getBankOp2(),
                                                    inProgram[instr]->getOp3(), inProgram[instr]->getBankOp3());

                outProgram.push_back(newMOVInstr);
            }
        }
        else
        {
            U32 op1Reg = inProgram[instr]->getOp1();
            Bank op1Bank = inProgram[instr]->getBankOp1();
            U32 op2Reg = inProgram[instr]->getOp2();
            Bank op2Bank = inProgram[instr]->getBankOp2();
            U32 op3Reg = inProgram[instr]->getOp3();
            Bank op3Bank = inProgram[instr]->getBankOp3();

            bool patchInstruction = false;

            //  Check if the first operand is an input attribute register.
            if ((inProgram[instr]->getNumOperands() >= 1) && (op1Bank == IN))
            {
                //  Check if the attribute was already loaded.
                if (input2temp[op1Reg] == MAX_TEMPORAL_REGISTERS)
                {
                    //  Add CG1_ISA_OPCODE_LDA instruction to load attribute into a temporal register.
                    U32 newTemp;

                    //  Select the next free temporal register to store the attribute.
                    for(newTemp = 0; (newTemp < MAX_TEMPORAL_REGISTERS) && tempInUse[newTemp]; newTemp++);

                    //  Check if there are no more temporal registers.
                    if (newTemp == MAX_TEMPORAL_REGISTERS)
                    {
                        CG_ASSERT("There no unused temporal registers available.");
                    }

                    //  Set temporal register as in use and associate with attribute.
                    tempInUse[newTemp] = true;
                    input2temp[op1Reg] = newTemp;

                    cgoShaderInstr *newCG1_ISA_OPCODE_LDAInstr;

                    newCG1_ISA_OPCODE_LDAInstr = new cgoShaderInstr(
                                            //  Opcode
                                            CG1_ISA_OPCODE_LDA,
                                            //  First operand
                                            IN, INDEX_ATTRIBUTE, false, false, XXXX,
                                            //  Second operand
                                            TEXT, op1Reg, false, false, XYZW,
                                            //  Third operand
                                            INVALID, 0, false, false, XYZW,
                                            //  Result
                                            TEMP, newTemp, false, mXYZW,
                                            //  Predication
                                            false, false, 0,
                                            //  Relative addressing mode
                                            false, 0, 0, 0,
                                            //  End flag
                                            false
                                      );

                    outProgram.push_back(newCG1_ISA_OPCODE_LDAInstr);

                    //  Use the new temporal register.
                    op1Reg = newTemp;
                }
                else
                {
                    //  Use the temporal register already loaded.
                    op1Reg = input2temp[op1Reg];
                }

                patchInstruction = true;
                op1Bank = TEMP;
            }

            //  Check if the second operand is an input attribute register.
            if ((inProgram[instr]->getNumOperands() >= 2) && (op2Bank == IN))
            {
                //  Check if the attribute was already loaded.
                if (input2temp[op2Reg] == MAX_TEMPORAL_REGISTERS)
                {
                    //  Add CG1_ISA_OPCODE_LDA instruction to load attribute into a temporal register.
                    U32 newTemp;

                    //  Select the next free temporal register to store the attribute.
                    for(newTemp = 0; (newTemp < MAX_TEMPORAL_REGISTERS) && tempInUse[newTemp]; newTemp++);

                    //  Check if there are no more temporal registers.
                    if (newTemp == MAX_TEMPORAL_REGISTERS)
                    {
                        CG_ASSERT("There no unused temporal registers available.");
                    }

                    //  Set temporal register as in use and associate with attribute.
                    tempInUse[newTemp] = true;
                    input2temp[op2Reg] = newTemp;

                    cgoShaderInstr *newCG1_ISA_OPCODE_LDAInstr;

                    newCG1_ISA_OPCODE_LDAInstr = new cgoShaderInstr(
                                            //  Opcode
                                            CG1_ISA_OPCODE_LDA,
                                            //  First operand
                                            IN, INDEX_ATTRIBUTE, false, false, XXXX,
                                            //  Second operand
                                            TEXT, op2Reg, false, false, XYZW,
                                            //  Third operand
                                            INVALID, 0, false, false, XYZW,
                                            //  Result
                                            TEMP, newTemp, false, mXYZW,
                                             //  Predication
                                             false, false, 0,
                                            //  Relative addressing mode
                                            false, 0, 0, 0,
                                            //  End flag
                                            false
                                      );

                    outProgram.push_back(newCG1_ISA_OPCODE_LDAInstr);

                    //  Use the new temporal register.
                    op2Reg = newTemp;
                }
                else
                {
                    //  Use the temporal register already loaded.
                    op2Reg = input2temp[op2Reg];
                }

                patchInstruction = true;
                op2Bank = TEMP;
            }

            //  Check if the third operand is an input attribute register.
            if ((inProgram[instr]->getNumOperands() >= 3) && (op3Bank == IN))
            {
                //  Check if the attribute was already loaded.
                if (input2temp[op3Reg] == MAX_TEMPORAL_REGISTERS)
                {
                    //  Add CG1_ISA_OPCODE_LDA instruction to load attribute into a temporal register.
                    U32 newTemp;

                    //  Select the next free temporal register to store the attribute.
                    for(newTemp = 0; (newTemp < MAX_TEMPORAL_REGISTERS) && tempInUse[newTemp]; newTemp++);

                    //  Check if there are no more temporal registers.
                    if (newTemp == MAX_TEMPORAL_REGISTERS)
                    {
                        CG_ASSERT("There no unused temporal registers available.");
                    }

                    //  Set temporal register as in use and associate with attribute.
                    tempInUse[newTemp] = true;
                    input2temp[op3Reg] = newTemp;

                    cgoShaderInstr *newCG1_ISA_OPCODE_LDAInstr;

                    newCG1_ISA_OPCODE_LDAInstr = new cgoShaderInstr(
                                            //  Opcode
                                            CG1_ISA_OPCODE_LDA,
                                            //  First operand
                                            IN, INDEX_ATTRIBUTE, false, false, XXXX,
                                            //  Second operand
                                            TEXT, op3Reg, false, false, XYZW,
                                            //  Third operand
                                            INVALID, 0, false, false, XYZW,
                                            //  Result
                                            TEMP, newTemp, false, mXYZW,
                                            //  Predication
                                            false, false, 0,
                                            //  Relative addressing mode
                                            false, 0, 0, 0,
                                            //  End flag
                                            false
                                      );

                    outProgram.push_back(newCG1_ISA_OPCODE_LDAInstr);

                    //  Use the new temporal register.
                    op3Reg = newTemp;
                }
                else
                {
                    //  Use the temporal register already loaded.
                    op3Reg = input2temp[op3Reg];
                }

                patchInstruction = true;
                op3Bank = TEMP;
            }

            //  Check if the instruction has to be patched.
            if (patchInstruction)
            {
                //  Patch the instruction with the temporal registers.
                cgoShaderInstr *patchedInstr;

                patchedInstr = patchedOpsInstruction(inProgram[instr], op1Reg, op1Bank, op2Reg, op2Bank, op3Reg, op3Bank);

                outProgram.push_back(patchedInstr);
            }
            else
            {
                //  Copy shader instruction.
                cgoShaderInstr *copyInstr;

                copyInstr = copyInstruction(inProgram[instr]);
                outProgram.push_back(copyInstr);
            }

        }
    }
}




float ShaderOptimization::getALUTEXRatio(vector<cgoShaderInstr *> inProgram)
{
    unsigned int instrType[3];

    for (unsigned int i = 0; i < 3; i++)
    {
        instrType[i] = 0;
    }

    //  replace input registers references with temporary register references in instructions.
    for(unsigned int instr = 0; instr < inProgram.size(); instr++)
    {
        updateInstructionType(inProgram[instr], instrType);
    }

    return (instrType[TEX_INSTR_TYPE] > 0)? ((float)instrType[ALU_INSTR_TYPE] / (float)instrType[TEX_INSTR_TYPE]) : 0.0f;
}

void ShaderOptimization::aos2soa(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram)
{
    bool tempInUse[MAX_TEMPORAL_REGISTERS];

    for(U32 tr = 0; tr < MAX_TEMPORAL_REGISTERS; tr++)
        tempInUse[tr] = false;

    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        cgoShaderInstr *copyInstr;

        vector<SwizzleMode> op1Components;
        vector<SwizzleMode> op2Components;
        vector<SwizzleMode> op3Components;

        vector<MaskMode> resComponentsMask;
        vector<SwizzleMode> resComponentsSwizzle;

        vector <U32> readComponentsOp1;
        vector <U32> readComponentsOp2;

/*printf("aos2soa => Converting instruction ");

char instrDisasm[256];
U08 instrCode[16];

//  Disassemble the instruction.
inProgram[instr]->disassemble(instrDisasm, MAX_INFO_SIZE);

//  Get the instruction code.
inProgram[instr]->getCode(instrCode);

//  Print the instruction.
printf("%04x :", instr * 16);
for(U32 b = 0; b < 16; b++)
    printf(" %02x", instrCode[b]);
printf("    %s\n", instrDisasm);*/

        //  Update the temporal registers used.
        updateTempRegsUsed(inProgram[instr], tempInUse);

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_TEX:
            case CG1_ISA_OPCODE_TXB:
            case CG1_ISA_OPCODE_TXL:
            case CG1_ISA_OPCODE_TXP:
            case CG1_ISA_OPCODE_LDA:
            case CG1_ISA_OPCODE_KIL:
            case CG1_ISA_OPCODE_KLS:
            case CG1_ISA_OPCODE_ZXP:
            case CG1_ISA_OPCODE_ZXS:
            case CG1_ISA_OPCODE_CHS:
            case CG1_ISA_OPCODE_SETPEQ:
            case CG1_ISA_OPCODE_SETPGT:
            case CG1_ISA_OPCODE_SETPLT:
            case CG1_ISA_OPCODE_ANDP:
            case CG1_ISA_OPCODE_JMP:
            case CG1_ISA_OPCODE_STPEQI:
            case CG1_ISA_OPCODE_STPGTI:
            case CG1_ISA_OPCODE_STPLTI:

                //  Special instructions that are not affected by the conversion.

                //  Copy the instruction.
                copyInstr = copyInstruction(inProgram[instr]);
                outProgram.push_back(copyInstr);

                break;

            case CG1_ISA_OPCODE_DP3:
            case CG1_ISA_OPCODE_DP4:
            case CG1_ISA_OPCODE_DPH:
                //  Dot product instruction requires special conversion.

                //  Extract components for first operand.
                extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                //  Extract components for second operand.
                extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                //  Extract components for third operand.
                extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                {
                    cgoShaderInstr *mulInstr;
                    cgoShaderInstr *mad0Instr;
                    cgoShaderInstr *mad1Instr;
                    cgoShaderInstr *mad2Instr;
                    cgoShaderInstr *mad3Instr;

                    Bank resBank;
                    U32 resReg;

                    //  Check if the result bank can be read back (is not an output attribute register).
                    if (inProgram[instr]->getBankRes() != OUT)
                    {
                        resBank = inProgram[instr]->getBankRes();
                        resReg = inProgram[instr]->getResult();
                    }
                    else
                    {
                        U32 newTemp;

                        //  Select the next free temporal register to be used to store the intermediate results.
                        for(newTemp = 0; (newTemp < MAX_TEMPORAL_REGISTERS) && tempInUse[newTemp]; newTemp++);

                        //  Check if there are no more temporal registers.
                        if (newTemp == MAX_TEMPORAL_REGISTERS)
                        {
                            CG_ASSERT("There no unused temporal registers available.");
                        }

                        //  Set temporal register as in use.
                        tempInUse[newTemp] = true;

                        //  The temporal register will be used for the intermediate values of the dot product operation.
                        //  The final result will be stored in the output attribute register.
                        resBank = TEMP;
                        resReg = newTemp;
                    }

                    //
                    // NOTE: When translating a DPx instruction with the saturated result flag it must be
                    //       taken into account that the result has only to be saturated for the last CG1_ISA_OPCODE_MAD.
                    //

                    if (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_DPH)
                    {
                        mulInstr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MUL,
                                                         //  First operand
                                                         inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                         inProgram[instr]->getOp1AbsoluteFlag(), op1Components[0],
                                                         //  Second operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[0],
                                                         //  Third operand
                                                         INVALID, 0, false, false, XYZW,
                                                         //  Result
                                                         resBank, resReg, false,
                                                         resComponentsMask[0],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         false
                                                         );
                    }
                    else
                    {
                        mad0Instr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MAD,
                                                         //  First operand
                                                         inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                         inProgram[instr]->getOp1AbsoluteFlag(), op1Components[0],
                                                         //  Second operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[0],
                                                         //  Third operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[3],
                                                         //  Result
                                                         resBank, resReg, false,
                                                         resComponentsMask[0],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         false
                                                         );
                    }

                    mad1Instr = new cgoShaderInstr(
                                                     //  Opcode
                                                     CG1_ISA_OPCODE_MAD,
                                                     //  First operand
                                                     inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                     inProgram[instr]->getOp1AbsoluteFlag(), op1Components[1],
                                                     //  Second operand
                                                     inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                     inProgram[instr]->getOp2AbsoluteFlag(), op2Components[1],
                                                     //  Third operand
                                                     resBank, resReg, false, false, resComponentsSwizzle[0],
                                                     //  Result
                                                     resBank, resReg, false,
                                                     resComponentsMask[0],
                                                     //  Predication
                                                     inProgram[instr]->getPredicatedFlag(),
                                                     inProgram[instr]->getNegatePredicateFlag(),
                                                     inProgram[instr]->getPredicateReg(),
                                                     //  Relative addressing mode
                                                     inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                     inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                     //  End flag
                                                     false
                                                     );

                    if (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_DP4)
                    {
                        mad2Instr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MAD,
                                                         //  First operand
                                                         inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                         inProgram[instr]->getOp1AbsoluteFlag(), op1Components[2],
                                                         //  Second operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[2],
                                                         //  Third operand
                                                         resBank, resReg, false, false, resComponentsSwizzle[0],
                                                         //  Result
                                                         inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), inProgram[instr]->getSaturatedRes(),
                                                         resComponentsMask[0],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         inProgram[instr]->getEndFlag() && (resComponentsMask.size() == 1)
                                                         );

                    }
                    else
                    {
                        mad2Instr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MAD,
                                                         //  First operand
                                                         inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                         inProgram[instr]->getOp1AbsoluteFlag(), op1Components[2],
                                                         //  Second operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[2],
                                                         //  Third operand
                                                         resBank, resReg, false, false, resComponentsSwizzle[0],
                                                         //  Result
                                                         resBank, resReg, false,
                                                         resComponentsMask[0],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         false
                                                         );

                        mad3Instr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MAD,
                                                         //  First operand
                                                         inProgram[instr]->getBankOp1(), inProgram[instr]->getOp1(), inProgram[instr]->getOp1NegateFlag(),
                                                         inProgram[instr]->getOp1AbsoluteFlag(), op1Components[3],
                                                         //  Second operand
                                                         inProgram[instr]->getBankOp2(), inProgram[instr]->getOp2(), inProgram[instr]->getOp2NegateFlag(),
                                                         inProgram[instr]->getOp2AbsoluteFlag(), op2Components[3],
                                                         //  Third operand
                                                         resBank, resReg, false, false, resComponentsSwizzle[0],
                                                         //  Result
                                                         inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), inProgram[instr]->getSaturatedRes(),
                                                         resComponentsMask[0],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         inProgram[instr]->getEndFlag() && (resComponentsMask.size() == 1)
                                                         );
                    }


                    if (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_DPH)
                        outProgram.push_back(mulInstr);
                    else
                        outProgram.push_back(mad0Instr);

                    outProgram.push_back(mad1Instr);
                    outProgram.push_back(mad2Instr);

                    if (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_DP4)
                        outProgram.push_back(mad3Instr);

                    //  Broadcast result to other result components if required.
                    for(U32 resComp = 1; resComp < resComponentsMask.size(); resComp++)
                    {
                        cgoShaderInstr *movInstr;

                        movInstr = new cgoShaderInstr(
                                                         //  Opcode
                                                         CG1_ISA_OPCODE_MOV,
                                                         //  First operand
                                                         resBank, resReg, false, false,
                                                         resComponentsSwizzle[0],
                                                         //  Second operand
                                                         INVALID, 0, false, false, XYZW,
                                                         //  Third operand
                                                         INVALID, 0, false, false, XYZW,
                                                         //  Result
                                                         inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), inProgram[instr]->getSaturatedRes(),
                                                         resComponentsMask[resComp],
                                                         //  Predication
                                                         inProgram[instr]->getPredicatedFlag(),
                                                         inProgram[instr]->getNegatePredicateFlag(),
                                                         inProgram[instr]->getPredicateReg(),
                                                         //  Relative addressing mode
                                                         inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                         inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                         //  End flag
                                                         inProgram[instr]->getEndFlag() && (resComp == (resComponentsMask.size() - 1))
                                                         );

                        outProgram.push_back(movInstr);
                    }

                    //  If a temporal register was allocated mark it as reusable again.
                    if (inProgram[instr]->getBankRes() == OUT)
                    {
                        tempInUse[resReg] = false;
                    }

                }

                break;

            case CG1_ISA_OPCODE_DST:

                //  CG1_ISA_OPCODE_DST instruction requires special conversion.


            case CG1_ISA_OPCODE_EXP:

                //  CG1_ISA_OPCODE_EXP instruction requires special conversion.


            case CG1_ISA_OPCODE_LIT:

                //  CG1_ISA_OPCODE_LIT instruction requires special conversion.


            case CG1_ISA_OPCODE_LOG:

                //  CG1_ISA_OPCODE_LOG instruction requires special conversion.

                //  NOTE:  The implementation of some of these instructions may be complex in a Scalar architecture
                //         due to limitations in the current shader ISA.  For example loading a constant value
                //         of 0.0f or 1.0f (the constant bank can not be manipulated directly) or because some
                //         functions are quite complex like the CG1_ISA_OPCODE_LIT function for the .z component.
                //         As a temporal solution the instructions will be treated as standard SIMD instruction
                //         and make the assumption that the ALUs implement the write of the four components
                //         in multiple cycles.

                /*
                //  Extract components for first operand.
                extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                //  Extract components for second operand.
                extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                //  Extract components for third operand.
                extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Convert instruction into multiple scalar instructions.  One per component of the result.
                for(U32 resComp = 0; resComp < resComponentsMask.size(); resComp++)
                {
                    cgoShaderInstr *soaInstr;

                    soaInstr = patchSOAInstruction(inProgram[instr], op1Components[resComp], op2Components[resComp],
                                                    op3Components[resComp], resComponentsMask[resComp]);

                    if (inProgram[instr]->isEnd() && (resComp == (resComponentsMask.size() - 1)))
                        soaInstr->setEndFlag(true);

                    outProgram.push_back(soaInstr);
                }*/

                //  Copy the instruction.
                copyInstr = copyInstruction(inProgram[instr]);
                outProgram.push_back(copyInstr);

                break;

            case CG1_ISA_OPCODE_ADD:
            case CG1_ISA_OPCODE_ARL:
            case CG1_ISA_OPCODE_CMP:
            case CG1_ISA_OPCODE_MAD:
            case CG1_ISA_OPCODE_FXMAD:
            case CG1_ISA_OPCODE_FXMAD2:
            case CG1_ISA_OPCODE_MAX:
            case CG1_ISA_OPCODE_MIN:
            case CG1_ISA_OPCODE_MOV:
            case CG1_ISA_OPCODE_MUL:
            case CG1_ISA_OPCODE_FXMUL:
            case CG1_ISA_OPCODE_SGE:
            case CG1_ISA_OPCODE_SLT:
            case CG1_ISA_OPCODE_DDX:
            case CG1_ISA_OPCODE_DDY:
            case CG1_ISA_OPCODE_ADDI:
            case CG1_ISA_OPCODE_MULI:

                //  SIMD instructions.

                //  Extract components for first operand.
                extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                //  Extract components for second operand.
                extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                //  Extract components for third operand.
                extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Convert instruction into multiple scalar instructions.  One per component of the result.
                for(U32 resComp = 0; resComp < resComponentsMask.size(); resComp++)
                {
                    cgoShaderInstr *soaInstr;

                    soaInstr = patchSOAInstruction(inProgram[instr], op1Components[readComponentsOp1[resComp]],
                                                   op2Components[readComponentsOp2[resComp]],
                                                   op3Components[readComponentsOp1[resComp]], resComponentsMask[resComp]);

                    if (inProgram[instr]->isEnd() && (resComp == (resComponentsMask.size() - 1)))
                        soaInstr->setEndFlag(true);

                    outProgram.push_back(soaInstr);
                }

                break;

            case CG1_ISA_OPCODE_EX2:
            case CG1_ISA_OPCODE_FRC:
            case CG1_ISA_OPCODE_LG2:
            case CG1_ISA_OPCODE_RCP:
            case CG1_ISA_OPCODE_RSQ:

                //  Scalar instructions.

                //  Extract components for first operand.
                extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                //  Extract components for second operand.
                extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                //  Extract components for third operand.
                extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  The instruction is scalar.  Compute the scalar result and then broadcast.
                cgoShaderInstr *soaInstr;

                soaInstr = patchSOAInstruction(inProgram[instr], op1Components[readComponentsOp1[0]],
                                               op2Components[readComponentsOp2[0]],
                                               op3Components[readComponentsOp1[0]], resComponentsMask[0]);

                outProgram.push_back(soaInstr);

                if (inProgram[instr]->isEnd() && (resComponentsMask.size() == 1))
                    soaInstr->setEndFlag(true);

                //  Broadcast result to other result components if required.
                for(U32 resComp = 1; resComp < resComponentsMask.size(); resComp++)
                {
                    cgoShaderInstr *movInstr;

                    movInstr = new cgoShaderInstr(
                                                     //  Opcode
                                                     CG1_ISA_OPCODE_MOV,
                                                     //  First operand
                                                     inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), false, false,
                                                     resComponentsSwizzle[0],
                                                     //  Second operand
                                                     INVALID, 0, false, false, XYZW,
                                                     //  Third operand
                                                     INVALID, 0, false, false, XYZW,
                                                     //  Result
                                                     inProgram[instr]->getBankRes(), inProgram[instr]->getResult(), inProgram[instr]->getSaturatedRes(),
                                                     resComponentsMask[resComp],
                                                     //  Predication
                                                     inProgram[instr]->getPredicatedFlag(),
                                                     inProgram[instr]->getNegatePredicateFlag(),
                                                     inProgram[instr]->getPredicateReg(),
                                                     //  Relative addressing mode
                                                     inProgram[instr]->getRelativeModeFlag(), inProgram[instr]->getRelMAddrReg(),
                                                     inProgram[instr]->getRelMAddrRegComp(), inProgram[instr]->getRelMOffset(),
                                                     //  End flag
                                                     inProgram[instr]->getEndFlag() && (resComp == (resComponentsMask.size() - 1))
                                                     );

                    if (inProgram[instr]->isEnd() && (resComp == (resComponentsMask.size() - 1)))
                        movInstr->setEndFlag(true);

                    outProgram.push_back(movInstr);
                }

                break;

            case CG1_ISA_OPCODE_FLR:

                // Unimplemented instructions.

                break;

        }
    }
}

bool ShaderOptimization::deadCodeElimination(vector<cgoShaderInstr *> inProgram, U32 namesUsed, vector<cgoShaderInstr *> &outProgram)
{
    vector<RegisterState> tempRegState;

    //  Set size of the temporal register state to the number of names used.
    tempRegState.clear();
    tempRegState.resize(namesUsed + 1);

    vector<InstructionInfo> instructionInfo;

    //  Clear the state of the temporal registers.
    for(U32 gpr = 0; gpr < tempRegState.size(); gpr++)
    {
        for(U32 comp = 0; comp < 4; comp++)
        {
            tempRegState[gpr].compWasWritten[comp] = false;
            tempRegState[gpr].compWasRead[comp] = false;
        }
    }

    //  Set and clear the state of the instruction database.
    instructionInfo.resize(inProgram.size());

    for(U32 instr = 0; instr < instructionInfo.size(); instr++)
    {
        for(U32 comp = 0; comp < 4; comp++)
            instructionInfo[instr].removeResComp[comp] = false;
    }

    //  Traverse the program to mark the instructions that can be removed.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        vector<MaskMode> resComponentsMask;
        vector<SwizzleMode> resComponentsSwizzle;
        vector<U32> readComponentsOp1;
        vector<U32> readComponentsOp2;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                //  These instructions don't set or use any register.

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

                //  Only writes to temporal registers are considered for removing unnecesary instructions.
                //  The usage of the output registers is not known and for now the usage of the address
                //  registers is not taken into account.

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Check if the first operand is active and a temporal register is being read.
                if ((inProgram[instr]->getNumOperands() > 0) && (inProgram[instr]->getBankOp1() == TEMP))
                {
                    vector <SwizzleMode> op1Components;

                    //  Extract components for first operand.
                    extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                    U32 op1Reg = inProgram[instr]->getOp1();

                    //  Set register component usage flag.  Only the components of the swizzled operand
                    //  that are actually read.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op1RegComp = op1Components[readComp] & 0x03;

                        //  Set register component as used.
                        tempRegState[op1Reg].compWasRead[op1RegComp] = true;
                    }
                }

                //  Check if the second operand is active and a temporal register is being read.
                if ((inProgram[instr]->getNumOperands() > 1) && (inProgram[instr]->getBankOp2() == TEMP))
                {
                    vector <SwizzleMode> op2Components;

                    //  Extract components for second operand.
                    extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                    U32 op2Reg = inProgram[instr]->getOp2();

                    //  Set register component usage flag.  Only the components of the swizzled operand
                    //  that are actually read.
                    for(U32 comp = 0; comp < readComponentsOp2.size(); comp++)
                    {
                        U32 readComp = readComponentsOp2[comp];

                        U32 op2RegComp = op2Components[readComp] & 0x03;

                        //  Set register component as used.
                        tempRegState[op2Reg].compWasRead[op2RegComp] = true;
                    }
                }

                //  Check if the first operand is active and a temporal register is being read.
                if ((inProgram[instr]->getNumOperands() == 3) && (inProgram[instr]->getBankOp3() == TEMP))
                {
                    vector <SwizzleMode> op3Components;

                    //  Extract components for first operand.
                    extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                    U32 op3Reg = inProgram[instr]->getOp3();

                    //  Set register component usage flag.  Only the components of the swizzled operand
                    //  that are actually read.  The components read are the same than those read for the
                    //  first operand (CG1_ISA_OPCODE_MAD instruction).
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op3RegComp = op3Components[readComp] & 0x03;

                        //  Set register component as used.
                        tempRegState[op3Reg].compWasRead[op3RegComp] = true;
                    }
                }

                //  Check if the result is written into a temporal register.
                //  The CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP and CG1_ISA_OPCODE_ZXS instructions don't write a register.
                if ((inProgram[instr]->getBankRes() == TEMP) && inProgram[instr]->hasResult())
                {
                    U32 resReg = inProgram[instr]->getResult();

                    for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                    {
                        //  Extract component index.
                        U32 resRegComp = resComponentsSwizzle[comp] & 0x03;

                        //  Check if the register component was already written but not read.
                        if (tempRegState[resReg].compWasWritten[resRegComp] && !tempRegState[resReg].compWasRead[resRegComp])
                        {
                            //  Set the result component of the instruction that set the value as to be removed if
                            //  the instruction is not predicated.
                            instructionInfo[tempRegState[resReg].compWriteInstruction[resRegComp]].removeResComp[resRegComp] = !inProgram[instr]->getPredicatedFlag();
                        }

                        //  Set the register component as written.  Set instruction that create the value in the register.
                        //  For predicated instructions is the previous value was read keep the component as read.
                        tempRegState[resReg].compWasWritten[resRegComp] = true;
                        tempRegState[resReg].compWasRead[resRegComp] = (inProgram[instr]->getPredicatedFlag() && tempRegState[resReg].compWasRead[resRegComp]);
                        tempRegState[resReg].compWriteInstruction[resRegComp] = instr;
                    }
                }

                break;
        }
    }

    //  Mark final results that are not used.
    for(U32 gpr = 0; gpr < tempRegState.size(); gpr++)
    {
        for(U32 comp = 0; comp < 4; comp++)
        {
            //  Check if the register component was written but not read
            if (tempRegState[gpr].compWasWritten[comp] && !tempRegState[gpr].compWasRead[comp])
            {
                //  Set the result component of the instruction that wrote the register as to be removed.
                instructionInfo[tempRegState[gpr].compWriteInstruction[comp]].removeResComp[comp] = true;
            }
        }
    }

    bool instrComponentsRemoved = false;

    //  Regenerate the program removing result components and instructions.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        vector<SwizzleMode> resComponentsSwizzle;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_KIL:
            case CG1_ISA_OPCODE_KLS:
            case CG1_ISA_OPCODE_ZXP:
            case CG1_ISA_OPCODE_ZXS:
            case CG1_ISA_OPCODE_CHS:
            case CG1_ISA_OPCODE_JMP:

                //  These instruction don't set any result so they can't be removed.

                {
                    cgoShaderInstr *copyInstr;

                    copyInstr = copyInstruction(inProgram[instr]);

                    outProgram.push_back(copyInstr);
                }

                break;

            case CG1_ISA_OPCODE_CMPKIL:

                //  This instruction is special, as it sets a result but can�t be removed if
                //  the result is no later used, because it in addition sets the kill flag.

                {
                    cgoShaderInstr *copyInstr;

                    copyInstr = copyInstruction(inProgram[instr]);

                    outProgram.push_back(copyInstr);
                }

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

                {
                    MaskMode resultMask;

                    resultMask = removeComponentsFromWriteMask(inProgram[instr]->getResultMaskMode(), instructionInfo[instr].removeResComp);

                    //  Check if result components were removed.
                    instrComponentsRemoved = instrComponentsRemoved || (resultMask != inProgram[instr]->getResultMaskMode());

                    if (resultMask != NNNN)
                    {
                        cgoShaderInstr *patchedResMaskInstr;

                        patchedResMaskInstr = patchResMaskInstruction(inProgram[instr], resultMask);

                        outProgram.push_back(patchedResMaskInstr);
                    }
                    else
                    {
                        //  Check if we are removing the instruction marked with the end flag.
                        if (inProgram[instr]->isEnd())
                        {
                            //  Check that the out program is not empty.
                            if (outProgram.size() > 0)
                            {
                                //  Mark the last instruction added to the program with end flag.
                                outProgram.back()->setEndFlag(true);
                            }
                            else
                            {
                                printf("WARNING: The whole program was eliminated!!\n");
                            }
                        }
                    }

                }

                break;

        }
    }

    return instrComponentsRemoved;
}

U32 ShaderOptimization::renameRegisters(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram, bool AOStoSOA)
{
    U32 registerName[MAX_TEMPORAL_REGISTERS][4];

    U32 nextRegisterName = 1;

    for(U32 gpr = 0; gpr < MAX_TEMPORAL_REGISTERS; gpr++)
    {
        for(U32 comp = 0; comp < 4; comp++)
            registerName[gpr][comp] = 0;
    }

    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        cgoShaderInstr *renamedInstr;
        cgoShaderInstr *copyInstr;

        vector<MaskMode> resComponentsMask;
        vector<SwizzleMode> resComponentsSwizzle;

        vector <U32> readComponentsOp1;
        vector <U32> readComponentsOp2;

        U32 resName;
        U32 op1Name;
        U32 op2Name;
        U32 op3Name;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                //  Those instruction don't set or use registers.

                //  Copy the instruction.
                copyInstr = copyInstruction(inProgram[instr]);

                outProgram.push_back(copyInstr);

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Check if the first operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 1) && (inProgram[instr]->getBankOp1() == TEMP))
                {
                    vector <SwizzleMode> op1Components;

                    //  Extract components for first operand.
                    extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                    op1Name = 0;

                    //  Get the most recent name based on the operand components that are read.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op1RegComp = op1Components[readComp] & 0x03;

                        //  The renamed name for the register component.
                        U32 op1CompName = registerName[inProgram[instr]->getOp1()][op1RegComp];

                        //  Get the most recent name of all components.
                        if (op1Name < op1CompName)
                            op1Name = op1CompName;
                    }

                    //  Check if the register was actually renamed.
                    if (op1Name == 0)
                    {
                        //  Check for special case.  CG1_ISA_OPCODE_SLT/CG1_ISA_OPCODE_SGE can be used to set a register to zero or one.
                        if (((inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_SLT) || (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_SGE)) &&
                            (inProgram[instr]->getBankOp1() == TEMP) && (inProgram[instr]->getBankOp2() == TEMP) &&
                            (inProgram[instr]->getOp1SwizzleMode() == inProgram[instr]->getOp2SwizzleMode()) &&
                            (inProgram[instr]->getOp1NegateFlag() == inProgram[instr]->getOp2NegateFlag()) &&
                            (inProgram[instr]->getOp1AbsoluteFlag() == inProgram[instr]->getOp2AbsoluteFlag()))
                        {
                            //  Setting result to constant value 0.0f or 1.0f.
                        }
                        else
                        {
                            printf("At instruction %d temporal register %d was used with no assigned value.\n", instr,
                                inProgram[instr]->getOp1());
                        }
                    }
                }
                else
                {
                    //  Input, constant, address registers and sampler identifiers are not renamed.
                    op1Name = inProgram[instr]->getOp1();
                }

                //  Check if the second operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 2) && (inProgram[instr]->getBankOp2() == TEMP))
                {
                    vector <SwizzleMode> op2Components;

                    //  Extract components for second operand.
                    extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                    op2Name = 0;

                    //  Get the most recent name based on the operand components that are read.
                    for(U32 comp = 0; comp < readComponentsOp2.size(); comp++)
                    {
                        U32 readComp = readComponentsOp2[comp];

                        U32 op2RegComp = op2Components[readComp] & 0x03;

                        //  The renamed name for the register component.
                        U32 op2CompName = registerName[inProgram[instr]->getOp2()][op2RegComp];

                        //  Get the most recent name of all components.
                        if (op2Name < op2CompName)
                            op2Name = op2CompName;
                    }

                    //  Check if the register was actually renamed.
                    if (op2Name == 0)
                    {
                        //  Check for special case.  CG1_ISA_OPCODE_SLT/CG1_ISA_OPCODE_SGE can be used to set a register to zero or one.
                        if (((inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_SLT) || (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_SGE)) &&
                            (inProgram[instr]->getBankOp1() == TEMP) && (inProgram[instr]->getBankOp2() == TEMP) &&
                            (inProgram[instr]->getOp1SwizzleMode() == inProgram[instr]->getOp2SwizzleMode()) &&
                            (inProgram[instr]->getOp1NegateFlag() == inProgram[instr]->getOp2NegateFlag()) &&
                            (inProgram[instr]->getOp1AbsoluteFlag() == inProgram[instr]->getOp2AbsoluteFlag()))
                        {
                            //  Setting result to constant value 0.0f or 1.0f.
                        }
                        else
                        {
                            printf("At instruction %d temporal register %d was used with no assigned value.\n", instr,
                                inProgram[instr]->getOp2());
                        }
                    }
                }
                else
                {
                    //  Input, constant, address registers and sampler identifiers are not renamed.
                    op2Name = inProgram[instr]->getOp2();
                }

                //  Check if the third operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() == 3) && (inProgram[instr]->getBankOp3() == TEMP))
                {
                    vector <SwizzleMode> op3Components;

                    //  Extract components for third operand.
                    extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                    op3Name = 0;

                    //  Get the most recent name based on the operand components that are read.
                    //  The order of the operands actually read for the third operand is the same than for the first operand.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op3RegComp = op3Components[readComp] & 0x03;

                        //  The renamed name for the register component.
                        U32 op3CompName = registerName[inProgram[instr]->getOp3()][op3RegComp];

                        //  Get the most recent name of all components.
                        if (op3Name < op3CompName)
                            op3Name = op3CompName;
                    }

                    //  Check if the register was actually renamed.
                    if (op3Name == 0)
                    {
                        printf("At instruction %d temporal register %d was used with no assigned value.\n", instr,
                            inProgram[instr]->getOp3());
                    }
                }
                else
                {
                    //  Input, constant, address registers and sampler identifiers are not renamed.
                    op3Name = inProgram[instr]->getOp3();
                }

                //  Check if the result register is a temporal register.
                //  Discard instructions that don't write a result.
                if ((inProgram[instr]->getBankRes() == TEMP) && inProgram[instr]->hasResult())
                {
                    //  Extract components for result.
                    extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                    //  If some of the result components were not written copy those components from the previous name.
                    if (resComponentsSwizzle.size() != 4)
                    {
                        bool writtenComponents[4];

                        //  Reset the components to remove.
                        for(U32 comp = 0; comp < 4; comp++)
                            writtenComponents[comp] = false;

                        //  Set the components to remove based on the components written by the instruction.
                        //  For predicated instructions we don't know which component are actually written.
                        for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                            writtenComponents[resComponentsSwizzle[comp] & 0x03] = !inProgram[instr]->getPredicatedFlag();

                        resName = 0;

                        //  Get most recent name for the result register from all it's components.
                        for(U32 comp = 0; comp < 4; comp++)
                        {
                            U32 resNameComp = registerName[inProgram[instr]->getResult()][comp];

                            //  Get the most recent name but only from the components that are not written.
                            if ((resName < resNameComp) && !writtenComponents[comp])
                                resName = resNameComp;
                        }


                        //  If none of the components was actually written before there is no need to
                        //  copy the component data.
                        if (resName != 0)
                        {
                            MaskMode copyComponentsMask;

                            //  Check if the input program is in Scalar format.
                            if (AOStoSOA)
                            {
                                for(U32 comp = 0; comp < 4; comp++)
                                {
                                    //  Get the most recent name for the register component.
                                    //U32 resNameComp = registerName[inProgram[instr]->getResult()][comp];

                                    //  Check if the instructions was not written but was defined.
                                    if (!writtenComponents[comp])
                                    {
                                        //  Set component to copy.
                                        switch(comp)
                                        {
                                            case 0 :
                                                copyComponentsMask = XNNN;
                                                break;
                                            case 1 :
                                                copyComponentsMask = NYNN;
                                                break;
                                            case 2 :
                                                copyComponentsMask = NNZN;
                                                break;
                                            case 3 :
                                                copyComponentsMask = NNNW;
                                                break;
                                        }

                                        cgoShaderInstr *movInstr;

                                        movInstr = new cgoShaderInstr(
                                                                         //  Opcode
                                                                         CG1_ISA_OPCODE_MOV,
                                                                         //  First operand
                                                                         TEMP, resName, false, false, XYZW,
                                                                         //  Second operand
                                                                         INVALID, 0, false, false, XYZW,
                                                                         //  Third operand
                                                                         INVALID, 0, false, false, XYZW,
                                                                         //  Result
                                                                         TEMP, nextRegisterName, false, copyComponentsMask,
                                                                         //  Predication
                                                                         false, false, 0,
                                                                         //  Relative addressing mode
                                                                         false, 0, 0, 0,
                                                                         //  End flag
                                                                         false
                                                                         );

                                        outProgram.push_back(movInstr);
                                    }
                                }
                            }
                            else
                            {
                                //  Compute the result mask for components not written by the instruction (remove written components).
                                copyComponentsMask = removeComponentsFromWriteMask(mXYZW, writtenComponents);

                                cgoShaderInstr *movInstr;

                                movInstr = new cgoShaderInstr(
                                                                 //  Opcode
                                                                 CG1_ISA_OPCODE_MOV,
                                                                 //  First operand
                                                                 TEMP, resName, false, false, XYZW,
                                                                 //  Second operand
                                                                 INVALID, 0, false, false, XYZW,
                                                                 //  Third operand
                                                                 INVALID, 0, false, false, XYZW,
                                                                 //  Result
                                                                 TEMP, nextRegisterName, false, copyComponentsMask,
                                                                 //  Predication
                                                                 false, false, 0,
                                                                 //  Relative addressing mode
                                                                 false, 0, 0, 0,
                                                                 //  End flag
                                                                 false
                                                                 );

                                outProgram.push_back(movInstr);
                            }
                        }
                    }

                    //  Associate original register and component with the new name.  All the components
                    //  of the result register get the new name because we are copying in the components not written
                    //  the value of the components of the previous most recent name.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        registerName[inProgram[instr]->getResult()][comp] = nextRegisterName;
                    }

                    //  Set the new name for the result register.
                    resName = nextRegisterName;

                    //  Update counter used to assign new names to the registers.
                    nextRegisterName++;

                }
                else
                {
                    //  Get the result register name.  Output and address registers are not renamed.
                    resName = inProgram[instr]->getResult();
                }

                //  Copy the shader instruction renaming the temporal registers.
                renamedInstr = renameRegsInstruction(inProgram[instr], resName, op1Name, op2Name, op3Name);

                //  Add renamed instruction to the program.
                outProgram.push_back(renamedInstr);


                break;
        }
    }

    //  Return how many 'register' names have been used for the program.
    return (nextRegisterName - 1);
}

U32 ShaderOptimization::reduceLiveRegisters(vector<cgoShaderInstr *> inProgram, U32 names, vector<cgoShaderInstr *> &outProgram)
{
    vector<NameUsage> nameUsage;

    U32 nextFreeReg = 0;

    //  Resize table storing name usage to the number of names used by the program.
    nameUsage.resize(names + 1);

    //  Clear name usage table.
    for(U32 name = 0; name < (names + 1); name++)
    {
        U32 numInstructions = (U32) inProgram.size();

        nameUsage[name].allocated = false;

        for(U32 comp = 0; comp < 4; comp++)
        {
            nameUsage[name].createdByInstr[comp] = 0;
            nameUsage[name].usedByInstrOp1[comp].resize(numInstructions);
            nameUsage[name].usedByInstrOp2[comp].resize(numInstructions);
            nameUsage[name].usedByInstrOp3[comp].resize(numInstructions);
            nameUsage[name].lastUsedByInstr[comp] = 0;

            for(U32 instr = 0; instr < numInstructions; instr++)
            {
                nameUsage[name].usedByInstrOp1[comp][instr] = false;
                nameUsage[name].usedByInstrOp2[comp][instr] = false;
                nameUsage[name].usedByInstrOp3[comp][instr] = false;
            }

            nameUsage[name].copiedFromInstr[comp] = 0;
            nameUsage[name].copiedRegister[comp] = 0;
            nameUsage[name].copiedComponent[comp] = 0;
            nameUsage[name].copiedByInstr[comp].clear();
            nameUsage[name].copies[comp].clear();

            nameUsage[name].allocReg[comp] = -1;
            nameUsage[name].allocComp[comp] = -1;
        }

        nameUsage[name].masterName = 0;

        nameUsage[name].maxPackedCompUse = 4;  //  Used to force register aggregation!!!
        nameUsage[name].instrPackedCompUse.resize(numInstructions);

        for(U32 instr = 0; instr < numInstructions; instr++)
            nameUsage[name].instrPackedCompUse[instr] = 0;
    }

    //  Analyze register usage in the program.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        vector<MaskMode> resComponentsMask;
        vector<SwizzleMode> resComponentsSwizzle;

        vector <U32> readComponentsOp1;
        vector <U32> readComponentsOp2;

        bool resultIsACopy = false;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                //  Those instruction don't set or use registers.

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Check if the first operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 1) && (inProgram[instr]->getBankOp1() == TEMP))
                {
                    vector <SwizzleMode> op1Components;

                    //  Extract components for first operand.
                    extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                    //  Get the name for the first operand.
                    U32 op1Name = inProgram[instr]->getOp1();

                    //  Set usage for all the components actually used of the first operand.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op1RegComp = op1Components[readComp] & 0x03;

                        //  Set name component as used by the current instruction.
                        nameUsage[op1Name].usedByInstrOp1[op1RegComp][instr] = true;

                        //  Set name component as last used by the current instruction (analysis is performed in program order).
                        nameUsage[op1Name].lastUsedByInstr[op1RegComp] = instr + 1;
                    }

                    //  Compute how many components of the are used packed in the instruction.
                    U32 packedCompUse = 0;
                    for(U32 comp = 0; comp < 4; comp++)
                        if (nameUsage[op1Name].usedByInstrOp1[comp][instr])
                            packedCompUse++;

                    //  Set the maximum packed component use for the name.
                    nameUsage[op1Name].maxPackedCompUse = (nameUsage[op1Name].maxPackedCompUse < packedCompUse) ?
                                                          packedCompUse : nameUsage[op1Name].maxPackedCompUse;

                    //  Set the packed component use for the name at the current instruction.
                    nameUsage[op1Name].instrPackedCompUse[instr] = (nameUsage[op1Name].instrPackedCompUse[instr] < packedCompUse) ?
                                                                   packedCompUse : nameUsage[op1Name].instrPackedCompUse[instr];

                    //  Add copy information for CG1_ISA_OPCODE_MOV instructions without result or source modifiers.
                    if ((inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_MOV) && (!inProgram[instr]->getSaturatedRes()) &&
                        (!inProgram[instr]->getOp1AbsoluteFlag()) && (!inProgram[instr]->getOp1NegateFlag()))
                    {
                        //  Get result register bank.
                        Bank resBank = inProgram[instr]->getBankRes();

                        //  Get result register.
                        U32 resReg = inProgram[instr]->getResult();

                        if (resBank == TEMP)
                        {
                            //  Track register copies.

                            //  Set copies.
                            for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                            {
                                //  Get the actual input component (swizzled) to be written over the output component.
                                U32 readComp = readComponentsOp1[comp];
                                U32 op1RegComp = op1Components[readComp] & 0x03;

                                //  Get the result component being written.
                                U32 resComp = resComponentsSwizzle[comp] & 0x03;

                                RegisterComponent regComp;

                                regComp.reg = resReg;
                                regComp.comp = resComp;

                                //  Register the copy.
                                nameUsage[resReg].copiedFromInstr[resComp] = nameUsage[op1Name].createdByInstr[op1RegComp];
                                nameUsage[resReg].copiedRegister[resComp] = op1Name;
                                nameUsage[resReg].copiedComponent[resComp] = op1RegComp;
                                nameUsage[op1Name].copiedByInstr[op1RegComp].push_back(instr + 1);
                                nameUsage[op1Name].copies[op1RegComp].push_back(regComp);
                            }

                            //  Result is a copy.  Don't update the name usage table.
                            resultIsACopy = true;
                        }
                    }
                }

                //  Check if the second operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 2) && (inProgram[instr]->getBankOp2() == TEMP))
                {
                    vector <SwizzleMode> op2Components;

                    //  Extract components for second operand.
                    extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                    //  Get the name for the second operand.
                    U32 op2Name = inProgram[instr]->getOp2();

                    //  Set usage for all the components actually used of the second operand.
                    for(U32 comp = 0; comp < readComponentsOp2.size(); comp++)
                    {
                        U32 readComp = readComponentsOp2[comp];

                        U32 op2RegComp = op2Components[readComp] & 0x03;

                        //  Set name component as used by the current instruction.
                        nameUsage[op2Name].usedByInstrOp2[op2RegComp][instr] = true;

                        //  Set name component as last used by the current instruction (analysis is performed in program order).
                        nameUsage[op2Name].lastUsedByInstr[op2RegComp] = instr + 1;
                    }

                    //  Compute how many components of the are used packed in the instruction.
                    U32 packedCompUse = 0;
                    for(U32 comp = 0; comp < 4; comp++)
                        if (nameUsage[op2Name].usedByInstrOp2[comp][instr])
                            packedCompUse++;

                    //  Set the maximum packed component use for the name.
                    nameUsage[op2Name].maxPackedCompUse = (nameUsage[op2Name].maxPackedCompUse < packedCompUse) ?
                                                          packedCompUse : nameUsage[op2Name].maxPackedCompUse;

                    //  Set the packed component use for the name at the current instruction.
                    nameUsage[op2Name].instrPackedCompUse[instr] = (nameUsage[op2Name].instrPackedCompUse[instr] < packedCompUse) ?
                                                                   packedCompUse : nameUsage[op2Name].instrPackedCompUse[instr];
                }

                //  Check if the third operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() == 3) && (inProgram[instr]->getBankOp3() == TEMP))
                {
                    vector <SwizzleMode> op3Components;

                    //  Extract components for third operand.
                    extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                    //  Get the name for the third operand.
                    U32 op3Name = inProgram[instr]->getOp3();

                    //  Set usage for all the components actually used of the third operand.
                    //  The components actually used for the third operand are the same that the
                    //  components used for the first operand.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        U32 readComp = readComponentsOp1[comp];

                        U32 op3RegComp = op3Components[readComp] & 0x03;

                        //  Set name component as used by the current instruction.
                        nameUsage[op3Name].usedByInstrOp3[op3RegComp][instr] = true;

                        //  Set name component as last used by the current instruction (analysis is performed in program order).
                        nameUsage[op3Name].lastUsedByInstr[op3RegComp] = instr + 1;
                    }

                    //  Compute how many components of the are used packed in the instruction.
                    U32 packedCompUse = 0;
                    for(U32 comp = 0; comp < 4; comp++)
                        if (nameUsage[op3Name].usedByInstrOp3[comp][instr])
                            packedCompUse++;

                    //  Set the maximum packed component use for the name.
                    nameUsage[op3Name].maxPackedCompUse = (nameUsage[op3Name].maxPackedCompUse < packedCompUse) ?
                                                          packedCompUse : nameUsage[op3Name].maxPackedCompUse;

                    //  Set the packed component use for the name at the current instruction.
                    nameUsage[op3Name].instrPackedCompUse[instr] = (nameUsage[op3Name].instrPackedCompUse[instr] < packedCompUse) ?
                                                                   packedCompUse : nameUsage[op3Name].instrPackedCompUse[instr];
                }

                //  Check if the result register is a temporal register.
                //  Discard CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP and CG1_ISA_OPCODE_ZXS instructions as they don't write a result.
                if ((inProgram[instr]->getBankRes() == TEMP) && (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_KIL) &&
                    (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_KLS) && (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_ZXP) &&
                    (inProgram[instr]->getOpcode() != CG1_ISA_OPCODE_ZXS))
                {
                    //  Extract components for result.
                    extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                    //  Get the result register name.
                    U32 resName = inProgram[instr]->getResult();

                    //  Set creation instruction for all the name components.
                    for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                    {
                        U32 resComp = resComponentsSwizzle[comp] & 0x03;

                        //  Check that the name component was not already created.
                        if (nameUsage[resName].createdByInstr[resComp] != 0)
                        {
                            //  Check if the current instruction is predicated.
                            if (!inProgram[instr]->getPredicatedFlag())
                            {
                                char buffer[256];
                                sprintf_s(buffer, sizeof(buffer), "Name %d component %d was already created at instruction %d.  Current instruction is %d.", resName,
                                    resComp, nameUsage[resName].createdByInstr[resComp] - 1, instr);
                                CG_ASSERT(buffer);
                            }
                        }

                        //  Set name component as created by the current instruction.
                        nameUsage[resName].createdByInstr[resComp] = instr + 1;

                        //  Check if the result is a copy from another name (CG1_ISA_OPCODE_MOV instruction).
                        if (!resultIsACopy)
                        {
                            //  The current value isn't a copy.
                            nameUsage[resName].copiedFromInstr[resComp] = 0;
                            nameUsage[resName].copiedRegister[resComp] = 0;
                            nameUsage[resName].copiedComponent[resComp] = 0;

                            //  The current value doesn't have copies.
                            nameUsage[resName].copiedByInstr[resComp].clear();
                            nameUsage[resName].copies[resComp].clear();
                        }
                    }

                    //  Some shader instructions produce a SIMD4 result (not a vector result) that prevents result component
                    //  renaming because the result register doesn't allows swizzling.  For those instructions the full
                    //  temporal register must be allocated for the name.  To this effect the packed usage is forced to 4
                    //  and a fake use for all four components is added in the same instruction.
                    bool forcePackedUseTo4 = hasSIMD4Result(inProgram[instr]->getOpcode());

                    //  Set fake first use for all components.
                    if (forcePackedUseTo4)
                    {
                        for(U32 comp = 0; comp < 4; comp++)
                        {
                            nameUsage[resName].createdByInstr[comp] = instr + 1;
                            nameUsage[resName].lastUsedByInstr[comp] = instr + 1;
                        }
                    }

                    //  Force max packed component use to 4 if required.
                    U32 packedWrite = forcePackedUseTo4 ? 4 : (U32) resComponentsSwizzle.size();

                    //  Set the maximum packed component use for the name.
                    nameUsage[resName].maxPackedCompUse = (nameUsage[resName].maxPackedCompUse < packedWrite) ?
                                                           packedWrite : nameUsage[resName].maxPackedCompUse;
                }

                break;
        }
    }


    //  Aggregate compatible names related by copies.
    for(U32 name = 1; name < (names + 1); name++)
    {
//printf("---------------------------------------\n");
//printf("Evaluating name %d for aggregation\n", name);
        //  Evaluate all the name components.
        for(U32 comp = 0; comp < 4; comp++)
        {
            //  Get master for this name.  If the name has no master the master name is
            //  the current name.
            U32 masterName = nameUsage[name].masterName;
            masterName = (masterName == 0) ? name : masterName;

            //  Evaluate if the copies of the name are compatible
            for(U32 copy = 0; copy < nameUsage[name].copies[comp].size(); copy++)
            {
//if (copy == 0)
//printf(" -> Master name is %d\n", masterName);
                //  Get the copy name and component.
                U32 copyName = nameUsage[name].copies[comp][copy].reg;

//printf(" -> Evaluating copy %d = (reg %d comp %d)\n", copy, nameUsage[name].copies[comp][copy].reg, nameUsage[name].copies[comp][copy].comp);
                //  Check if the name wasn't already aggregated.
                if (nameUsage[copyName].masterName == 0)
                {
                    bool aggregateName = true;

                    //  Evaluate if all the components of the copy name can be aggregated to the
                    //  master name.
                    for(U32 copyComp = 0; copyComp < 4; copyComp++)
                    {
                        //  Evaluate if the component in they copy name is free.
                        bool copyComponentIsFree = (nameUsage[copyName].createdByInstr[copyComp] == 0);
                        
                        //  Evaluate if the component in the copy name is a copy from the same master (or master master) component.
                        bool copyCompIsCopyFromMasterComp = (((nameUsage[copyName].copiedRegister[copyComp] == masterName) ||
                                                              (nameUsage[nameUsage[copyName].copiedRegister[copyComp]].masterName == masterName)) &&
                                                             (nameUsage[copyName].copiedComponent[copyComp] == copyComp));

                        //  Evaluate if the same component in the master name is free before the component in the copy gets a value.
                        bool masterCompIsFree = (nameUsage[copyName].createdByInstr[copyComp] >= nameUsage[masterName].lastUsedByInstr[copyComp]);
           
//printf(" -> Evaluate copy comp %d | created by instr %d | copy from (%d, %d) | copy created by instr %d | master comp last used by instr %d | aggregate? %s\n",
//copyComp, nameUsage[copyName].createdByInstr[copyComp],
//nameUsage[copyName].copiedRegister[copyComp],
//nameUsage[copyName].copiedComponent[copyComp],
//nameUsage[copyName].createdByInstr[copyComp],
//nameUsage[masterName].lastUsedByInstr[copyComp],
//aggregateName ? "T" : "F");

                        //  Evalute if the copy component can be aggregated to the same component in the master.
                        aggregateName = aggregateName && (copyComponentIsFree || copyCompIsCopyFromMasterComp || masterCompIsFree);

                        //  Evaluate if a component of the copy name can be aggregated.
                        //  A component of the copy name can be aggregated to the master name if:
                        //
                        //   - The component was not created
                        //       or
                        //   - The component is a copy of the same master component
                        //       or
                        //   - The master component is the same that the copy component and it was used for the
                        //     last time before the copy component creation
                        //
                        /*aggregateName = aggregateName &&
                                        ((nameUsage[copyName].createdByInstr[copyComp] == 0) ||
                                         (((nameUsage[copyName].copiedRegister[copyComp] == masterName) ||
                                           (nameUsage[nameUsage[copyName].copiedRegister[copyComp]].masterName == masterName)) &&
                                          (masterComp == copyComp)) ||
                                         ((nameUsage[copyName].createdByInstr[copyComp] >= nameUsage[masterName].lastUsedByInstr[masterComp]) &&
                                          (masterComp == copyComp)));*/
//printf(" -> Evaluate copy comp %d | created by instr %d | copy from (%d, %d) | copy source master %d | copy created by instr %d | master comp last used by instr %d | aggregate? %s\n",
//copyComp, nameUsage[copyName].createdByInstr[copyComp],
//nameUsage[copyName].copiedRegister[copyComp], masterComp,
//nameUsage[nameUsage[copyName].copiedRegister[copyComp]].masterName,
//nameUsage[copyName].createdByInstr[copyComp],
//nameUsage[masterName].lastUsedByInstr[masterComp],
//aggregateName ? "T" : "F");
                    }

                    //  Check if the copy name can be aggregated
                    if (aggregateName)
                    {
                        //  Set the master name for the copy name.
                        nameUsage[copyName].masterName = masterName;

                        //  Update usage information for the master name.
                        for(U32 copyComp = 0; copyComp < 4; copyComp++)
                        {
                            //
                            // NOTE : Translation no longer required due to the restriction that copy components and master components
                            //        must be the same (see above).
                            //
                            //  Translate copy component into the corresponing master component.
                            //U32 masterComp = nameUsage[copyName].copiedComponent[copyComp];
                            U32 masterComp = copyComp;

//printf(" -> Updating master creation/usage information => copy comp %d | master comp %d | copy created by instr %d | copy last used by instr %d\n",
//copyComp, masterComp, nameUsage[copyName].createdByInstr[copyComp], nameUsage[copyName].lastUsedByInstr[copyComp]);

                            //  Set the master name component (first) creation if the name component was not
                            //  created.
                            if (nameUsage[masterName].createdByInstr[copyComp] == 0)
                                nameUsage[masterName].createdByInstr[copyComp] = nameUsage[copyName].createdByInstr[copyComp];
                                
                            //  Update the last use of the master component with the copy component last use.
                            if ((nameUsage[copyName].lastUsedByInstr[copyComp] != 0) &&
                                (nameUsage[copyName].lastUsedByInstr[copyComp] > nameUsage[masterName].lastUsedByInstr[copyComp]))
                                nameUsage[masterName].lastUsedByInstr[copyComp] = nameUsage[copyName].lastUsedByInstr[copyComp];
                            
                            //  Update packed usage for the master name.
                            if (nameUsage[masterName].maxPackedCompUse < nameUsage[copyName].maxPackedCompUse)
                                nameUsage[masterName].maxPackedCompUse = nameUsage[copyName].maxPackedCompUse;

                            //  Update per instruction information in the master name.
                            for(U32 instr = 0; instr < inProgram.size(); instr++)
                            {
                                //  Update operands accessed.
                                if (nameUsage[copyName].usedByInstrOp1[copyComp][instr])
                                {
                                    GPU_ASSERT(
                                        if (nameUsage[masterName].usedByInstrOp1[copyComp][instr])
                                        {
                                            char buffer[256];
                                            sprintf_s(buffer, sizeof(buffer), "Aggregating name %d to master name %d but master comp %d already used for operand 1 at instr %d\n",
                                                copyName, masterName, copyComp, instr);
                                            CG_ASSERT(buffer);
                                        }
                                    )

                                    nameUsage[masterName].usedByInstrOp1[copyComp][instr] = true;
                                }


                                if (nameUsage[copyName].usedByInstrOp2[copyComp][instr])
                                {
                                    GPU_ASSERT(
                                        if (nameUsage[masterName].usedByInstrOp2[copyComp][instr])
                                        {
                                            char buffer[256];
                                            sprintf_s(buffer, sizeof(buffer), "Aggregating name %d to master name %d but master comp %d already used for operand 2 at instr %d\n",
                                                copyName, masterName, copyComp, instr);
                                            CG_ASSERT(buffer);
                                        }
                                    )

                                    nameUsage[masterName].usedByInstrOp2[copyComp][instr] = true;
                                }

                                if (nameUsage[copyName].usedByInstrOp3[copyComp][instr])
                                {
                                    GPU_ASSERT(
                                        if (nameUsage[masterName].usedByInstrOp3[copyComp][instr])
                                        {
                                            char buffer[256];
                                            sprintf_s(buffer, sizeof(buffer), "Aggregating name %d to master name %d but master comp %d already used for operand 3 at instr %d\n",
                                                copyName, masterName, copyComp, instr);
                                            CG_ASSERT(buffer);
                                        }
                                    )

                                    nameUsage[masterName].usedByInstrOp3[masterComp][instr] = true;
                                }

                                //  Updated packed component usage.
                                if (nameUsage[masterName].instrPackedCompUse[instr] < nameUsage[copyName].instrPackedCompUse[instr])
                                    nameUsage[masterName].instrPackedCompUse[instr] = nameUsage[copyName].instrPackedCompUse[instr];
                            }
                        }
                    }
                }
            }
        }
    }


    /*for(U32 name = 1; name < (names + 1); name++)
    {
        printf("Name %d, packed component max use is %d\n", name, nameUsage[name].maxPackedCompUse);
        printf(" Packet component usage : \n");
        for(U32 instr = 0; instr < inProgram.size(); instr++)
            printf(" %d", nameUsage[name].instrPackedCompUse[instr]);
        printf("\n");

        if (nameUsage[name].masterName != 0)
            printf(" Master name %d\n", nameUsage[name].masterName);
        else
            printf(" The name has no master.\n");

        for(U32 comp = 0; comp < 4; comp++)
        {
            if (nameUsage[name].createdByInstr[comp] != 0)
                printf("    Component %d was created by instruction %d (%04x)\n",
                    comp, nameUsage[name].createdByInstr[comp], (nameUsage[name].createdByInstr[comp] - 1) * 16);
            else
                printf("    Component %d was not created by any instruction\n", comp);

            if (nameUsage[name].lastUsedByInstr > 0)
            {
                printf("    Component %d was last used by instruction %d (%04x)\n", comp,
                    nameUsage[name].lastUsedByInstr[comp], (nameUsage[name].lastUsedByInstr[comp] - 1) * 16);
                printf("      Per operand usage:\n");
                printf("        Operand1 ->");
                for(U32 instr = 0; instr < inProgram.size(); instr++)
                    printf(" %s", nameUsage[name].usedByInstrOp1[comp][instr] ? "Y" : "N");
                printf("\n");
                printf("        Operand2 ->");
                for(U32 instr = 0; instr < inProgram.size(); instr++)
                    printf(" %s", nameUsage[name].usedByInstrOp2[comp][instr] ? "Y" : "N");
                printf("\n");
                printf("        Operand3 ->");
                for(U32 instr = 0; instr < inProgram.size(); instr++)
                    printf(" %s", nameUsage[name].usedByInstrOp3[comp][instr] ? "Y" : "N");
                printf("\n");
            }
            else
                printf("    Component %d was not used by any instruction\n", comp);

            if (nameUsage[name].copiedFromInstr[comp] > 0)
                printf("    Value was copied from reg %d comp %d created by instruction %d (%04x)\n",
                    nameUsage[name].copiedRegister[comp], nameUsage[name].copiedComponent[comp],
                    nameUsage[name].copiedFromInstr[comp], (nameUsage[name].copiedFromInstr[comp] - 1) * 16);
            else
                printf("    Value is not a copy\n");
            if (nameUsage[name].copiedByInstr[comp].size() > 0)
            {
                printf("    Value was copied at instructions :");
                for(U32 instr = 0; instr < nameUsage[name].copiedByInstr[comp].size(); instr++)
                    printf(" %d (%04x)", nameUsage[name].copiedByInstr[comp][instr], (nameUsage[name].copiedByInstr[comp][instr] - 1) * 16);
                printf("\n");
            }
            else
            {
                printf("    Value was not copied.\n");
            }
            if (nameUsage[name].copies[comp].size() > 0)
            {
                printf("    Copies of the value :");
                for(U32 copy = 0; copy < nameUsage[name].copies[comp].size(); copy++)
                    printf(" (%d.%d)", nameUsage[name].copies[comp][copy].reg, nameUsage[name].copies[comp][copy].comp);
                printf("\n");
            }
            else
            {
                printf("    No copies of the value.\n");
            }
        }
    }*/

    NameComponent registerMapping[MAX_TEMPORAL_REGISTERS][4];

    //  Set all the temporal register components as free;
    for(U32 gpr = 0; gpr < MAX_TEMPORAL_REGISTERS; gpr++)
    {
        for(U32 comp = 0; comp < 4; comp++)
        {
            registerMapping[gpr][comp].name = 0;
            registerMapping[gpr][comp].freeFromInstr = 0;
        }
    }


    U32 maxLiveRegisters = 0;

    //  Allocate registers.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {

        U32 patchedResReg;
        MaskMode patchedResMask;
        U32 patchedOp1Reg;
        U32 patchedOp2Reg;
        U32 patchedOp3Reg;
        SwizzleMode patchedSwzModeOp1;
        SwizzleMode patchedSwzModeOp2;
        SwizzleMode patchedSwzModeOp3;

        U32 currentLiveRegisters = 0;

        //  Count the number of live allocated temporal registers.
        for(U32 gpr = 0; gpr < MAX_TEMPORAL_REGISTERS; gpr++)
        {
            bool allCompsFree = true;

            for(U32 comp = 0; comp < 4; comp++)
                allCompsFree = allCompsFree && (registerMapping[gpr][comp].freeFromInstr <= instr);

            if (!allCompsFree)
                currentLiveRegisters++;
        }

        maxLiveRegisters = (maxLiveRegisters < currentLiveRegisters) ? currentLiveRegisters : maxLiveRegisters;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                //  Those instruction don't set or use registers.

                cgoShaderInstr *copyInstr;

                //  Copy the instruction to the output program.
                copyInstr = copyInstruction(inProgram[instr]);
                outProgram.push_back(copyInstr);

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

//printf(" => Instr %d\n", instr);

                //  Check if the first operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() > 0) && (inProgram[instr]->getBankOp1() == TEMP))
                {
//printf(" => Renaming first operand.\n");

                    //  Get the third operand name.
                    U32 name = inProgram[instr]->getOp1();

//printf(" => Name = %d\n", name);

                    bool readComponents[4];

                    U32 firstComponent = 5;

                    //  Get name components read from the operand.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        readComponents[comp] = nameUsage[name].usedByInstrOp1[comp][instr];

                        if ((firstComponent == 5) && (nameUsage[name].usedByInstrOp1[comp][instr]))
                            firstComponent = comp;
                    }

//printf(" => firstComponent = %d readComponents = {%d, %d, %d, %d}\n", firstComponent, readComponents[0], readComponents[1],
//readComponents[2], readComponents[3]);

                    //  Check if the name is aggregated to a master name.
                    if ((nameUsage[name].maxPackedCompUse > 1) && (nameUsage[name].masterName != 0))
                    {
                        //  Change the name to use for register allocation information to the
                        //  master name.
                        name = nameUsage[name].masterName;
                    }

//printf(" => Name = %d\n", name);

                    //  Set translated first operand register.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                    {
                        //  The first used component in the name was not defined/renamed.                        
                        //  Search for a defined/renamed component for the name.
                        U32 c;
                        for(c = 0; (c < 4) && (nameUsage[name].allocReg[c] == -1); c++);
                        
                        if (c == 4)
                        {
                            //  None of the name components was defined/renamed!!!
                            CG_ASSERT("None of the first operand components was defined.");
                        }
                        else
                        {
                            patchedOp1Reg = nameUsage[name].allocReg[c];
                        }
                    }
                    else
                        patchedOp1Reg = nameUsage[name].allocReg[firstComponent];

//printf(" => patchedOp1Reg = %d\n", patchedOp1Reg);

                    //  Translate the swizzle mode for the operand.

                    vector <SwizzleMode> opComponents;

                    //  Extract components for operand.
                    extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), opComponents);

//printf(" => components for operand = %d %d %d %d\n", opComponents[0], opComponents[1], opComponents[2], opComponents[3]);

                    U32 translatedSwz[4];

                    //  Translate the components in the swizzle mode.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        //  If the instruction is actually read rename component name.
                        if (readComponents[opComponents[comp] & 0x03])
                            translatedSwz[comp] = nameUsage[name].allocComp[opComponents[comp] & 0x03];
                        else
                            translatedSwz[comp] = opComponents[comp] & 0x03;
                    }

//printf(" => translated swizzle = %d %d %d %d\n", translatedSwz[0], translatedSwz[1], translatedSwz[2], translatedSwz[3]);

                    //  Recreate the swizzle mode for the operand.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                        patchedSwzModeOp1 = inProgram[instr]->getOp1SwizzleMode();
                    else
                        patchedSwzModeOp1 = encodeSwizzle(translatedSwz);

                }
                else
                {
                    patchedOp1Reg = inProgram[instr]->getOp1();
                    patchedSwzModeOp1 = inProgram[instr]->getOp1SwizzleMode();
                }

                //  Check if the second operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() > 1) && (inProgram[instr]->getBankOp2() == TEMP))
                {
//printf(" => Renaming second operand.\n");

                    //  Get the third operand name.
                    U32 name = inProgram[instr]->getOp2();

//printf(" => Name = %d\n", name);

                    bool readComponents[4];

                    U32 firstComponent = 5;

                    //  Get name components read from the operand.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        readComponents[comp] = nameUsage[name].usedByInstrOp2[comp][instr];

                        if ((firstComponent == 5) && (nameUsage[name].usedByInstrOp2[comp][instr]))
                            firstComponent = comp;
                    }

//printf(" => firstComponent = %d readComponents = {%d, %d, %d, %d}\n", firstComponent, readComponents[0], readComponents[1],
//readComponents[2], readComponents[3]);

                    //  Check if the name is aggregated to a master name.
                    if ((nameUsage[name].maxPackedCompUse > 1) && (nameUsage[name].masterName != 0))
                    {
                        //  Change the name to use for register allocation information to the
                        //  master name.
                        name = nameUsage[name].masterName;
                    }

//printf(" => Name = %d\n", name);

                    //  Set translated second operand register.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                    {
                        //  The first used component in the name was not defined/renamed.                        
                        //  Search for a defined/renamed component for the name.
                        U32 c;
                        for(c = 0; (c < 4) && (nameUsage[name].allocReg[c] == -1); c++);
                        
                        if (c == 4)
                        {
                            //  None of the name components was defined/renamed!!!
                            CG_ASSERT("None of the second operand components was defined.");
                        }
                        else
                        {
                            patchedOp2Reg = nameUsage[name].allocReg[c];
                        }
                    }
                    else
                        patchedOp2Reg = nameUsage[name].allocReg[firstComponent];

//printf(" => patchedOp2Reg = %d\n", patchedOp2Reg);

                    //  Translate the swizzle mode for the operand.

                    vector <SwizzleMode> opComponents;

                    //  Extract components for operand.
                    extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), opComponents);

//printf(" => components for operand = %d %d %d %d\n", opComponents[0], opComponents[1], opComponents[2], opComponents[3]);

                    U32 translatedSwz[4];

                    //  Translate the components in the swizzle mode.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        //  If the instruction is actually read rename component name.
                        if (readComponents[opComponents[comp] & 0x03])
                            translatedSwz[comp] = nameUsage[name].allocComp[opComponents[comp] & 0x03];
                        else
                            translatedSwz[comp] = opComponents[comp] & 0x03;
                    }

//printf(" => translated swizzle = %d %d %d %d\n", translatedSwz[0], translatedSwz[1], translatedSwz[2], translatedSwz[3]);

                    //  Recreate the swizzle mode for the operand.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                        patchedSwzModeOp2 = inProgram[instr]->getOp2SwizzleMode();
                    else
                        patchedSwzModeOp2 = encodeSwizzle(translatedSwz);
                }
                else
                {
                    patchedOp2Reg = inProgram[instr]->getOp2();
                    patchedSwzModeOp2 = inProgram[instr]->getOp2SwizzleMode();
                }

                //  Check if the third operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() == 3) && (inProgram[instr]->getBankOp3() == TEMP))
                {
//printf(" => Renaming third operand.\n");

                    //  Get the third operand name.
                    U32 name = inProgram[instr]->getOp3();

//printf(" => Name = %d\n", name);

                    bool readComponents[4];

                    U32 firstComponent = 5;

                    //  Get name components read from the operand.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        readComponents[comp] = nameUsage[name].usedByInstrOp3[comp][instr];

                        if ((firstComponent == 5) && (nameUsage[name].usedByInstrOp3[comp][instr]))
                            firstComponent = comp;
                    }

//printf(" => firstComponent = %d readComponents = {%d, %d, %d, %d}\n", firstComponent, readComponents[0], readComponents[1],
//readComponents[2], readComponents[3]);

                    //  Check if the name is aggregated to a master name.
                    if ((nameUsage[name].maxPackedCompUse > 1) && (nameUsage[name].masterName != 0))
                    {
                        //  Change the name to use for register allocation information to the
                        //  master name.
                        name = nameUsage[name].masterName;
                    }

//printf(" => Name = %d\n", name);

                    //  Set translated third operand register.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                    {
                        //  The first used component in the name was not defined/renamed.                        
                        //  Search for a defined/renamed component for the name.
                        U32 c;
                        for(c = 0; (c < 4) && (nameUsage[name].allocReg[c] == -1); c++);
                        
                        if (c == 4)
                        {
                            //  None of the name components was defined/renamed!!!
                            CG_ASSERT("None of the third operand components was defined.");
                        }
                        else
                        {
                            patchedOp3Reg = nameUsage[name].allocReg[c];
                        }                        
                    }
                    else
                        patchedOp3Reg = nameUsage[name].allocReg[firstComponent];

//printf(" => patchedOp3Reg = %d\n", patchedOp3Reg);

                    //  Translate the swizzle mode for the operand.

                    vector <SwizzleMode> opComponents;

                    //  Extract components for operand.
                    extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), opComponents);

//printf(" => components for operand = %d %d %d %d\n", opComponents[0], opComponents[1], opComponents[2], opComponents[3]);

                    U32 translatedSwz[4];

                    //  Translate the components in the swizzle mode.
                    for(U32 comp = 0; comp < 4; comp++)
                    {
                        //  If the instruction is actually read rename component name.
                        if (readComponents[opComponents[comp] & 0x03])
                            translatedSwz[comp] = nameUsage[name].allocComp[opComponents[comp] & 0x03];
                        else
                            translatedSwz[comp] = opComponents[comp] & 0x03;
                    }

//printf(" => translated swizzle = %d %d %d %d\n", translatedSwz[0], translatedSwz[1], translatedSwz[2], translatedSwz[3]);

                    //  Recreate the swizzle mode for the operand.
                    if (nameUsage[name].allocReg[firstComponent] == -1)
                        patchedSwzModeOp3 = inProgram[instr]->getOp3SwizzleMode();
                    else
                        patchedSwzModeOp3 = encodeSwizzle(translatedSwz);

                }
                else
                {
                    patchedOp3Reg = inProgram[instr]->getOp3();
                    patchedSwzModeOp3 = inProgram[instr]->getOp3SwizzleMode();
                }

                //  Check if the result register is a temporal register.
                //  Discard CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP and CG1_ISA_OPCODE_ZXS instructions as they don't write a result.
                if ((inProgram[instr]->getBankRes() == TEMP) && inProgram[instr]->hasResult())
                {
                    //  Get name written by the instruction.
                    U32 name = inProgram[instr]->getResult();

                    //  Check if the name requires packed allocation.
                    if (nameUsage[name].maxPackedCompUse > 1)
                    {
                        //  Check if the name is aggregated to a master name.
                        if (nameUsage[name].masterName != 0)
                        {
                            //  If so use the allocation information from the master name.
                            name = nameUsage[name].masterName;
                        }

                        //  Check if name was already allocated.
                        if (nameUsage[name].allocated)
                        {
                            //  Get the allocation and update the register mapping table.
                            for(U32 comp = 0; comp < 4; comp++)
                            {
                                //  Check if the component was created by the instruction.
                                if (nameUsage[name].createdByInstr[comp] == (instr + 1))
                                {
                                    //  Allocating name component with register component.
                                    registerMapping[nameUsage[name].allocReg[comp]][nameUsage[name].allocComp[comp]].name = name;
                                    registerMapping[nameUsage[name].allocReg[comp]][nameUsage[name].allocComp[comp]].component = comp;
                                }
                            }
                        }
                        else
                        {
                            U32 gpr = 0;
                            bool foundTempReg = false;
                            U32 nameMapping = 0;

                            //  Search in the register mapping table for a suitable free register.
                            while ((gpr < MAX_TEMPORAL_REGISTERS) && !foundTempReg)
                            {
                                bool nameCompCanBeMappedToRegComp[4][4];

                                //  Check for every name component the temporal register components that could be used.
                                for(U32 nameComp = 0; nameComp < 4; nameComp++)
                                    for(U32 regComp = 0; regComp < 4; regComp++)
                                    {
                                        nameCompCanBeMappedToRegComp[nameComp][regComp] =
                                            (nameUsage[name].createdByInstr[nameComp] == 0) ||
                                            (registerMapping[gpr][regComp].freeFromInstr <= nameUsage[name].createdByInstr[nameComp]);

                                        //if (hasSIMD4Result(inProgram[instr]->getOpcode()) && (nameComp != regComp))
                                        //    nameCompCanBeMappedToRegComp[nameComp][regComp] = false;
                                        for(U32 i = 0; i < inProgram.size(); i++)
                                        {
                                            if (nameUsage[name].usedByInstrOp1[nameComp][i] ||
                                                nameUsage[name].usedByInstrOp2[nameComp][i] ||
                                                nameUsage[name].usedByInstrOp3[nameComp][i])
                                            {
                                                if (hasSIMD4Result(inProgram[i]->getOpcode()) && (nameComp != regComp))
                                                    nameCompCanBeMappedToRegComp[nameComp][regComp] = false;
                                            }
                                        }
                                    }
                                bool mappingFound = false;

                                for(U32 mapping = 0; (mapping < 24) && !mappingFound; mapping++)
                                {
                                    //  Get the components for the mapping.
                                    U32 compToRegComp0 = mappingTableComp0[mapping];
                                    U32 compToRegComp1 = mappingTableComp1[mapping];
                                    U32 compToRegComp2 = mappingTableComp2[mapping];
                                    U32 compToRegComp3 = mappingTableComp3[mapping];

                                    //  Check if the mapping is possible.
                                    mappingFound = nameCompCanBeMappedToRegComp[0][compToRegComp0] &&
                                        nameCompCanBeMappedToRegComp[1][compToRegComp1] &&
                                        nameCompCanBeMappedToRegComp[2][compToRegComp2] &&
                                        nameCompCanBeMappedToRegComp[3][compToRegComp3];

                                    //  Name mapping to use.
                                    nameMapping = mapping;
                                }

                                //  Associate name and components to register and components.
                                if (mappingFound)
                                {
                                    //  The temporal register used to allocate the name components was found.
                                    foundTempReg = true;
                                }
                                else
                                {
                                    //  Try with the next temporal register.
                                    gpr++;
                                }
                            }

                            if (!foundTempReg)
                            {
                                CG_ASSERT("No temporal register available to allocate name.");
                            }

                            //  Set the final mapping between name components and register components.
                            U32 nameCompToRegComp[4];

                            nameCompToRegComp[0] = mappingTableComp0[nameMapping];
                            nameCompToRegComp[1] = mappingTableComp1[nameMapping];
                            nameCompToRegComp[2] = mappingTableComp2[nameMapping];
                            nameCompToRegComp[3] = mappingTableComp3[nameMapping];

                            //  Set name as allocated to a register.
                            nameUsage[name].allocated = true;

                            //  Allocate registers for the components written by the instruction.
                            for(U32 comp = 0; comp < 4; comp++)
                            {
                                //  Check if the component was created by the instruction.
                                if (nameUsage[name].createdByInstr[comp] == (instr + 1))
                                {
                                    //  Allocating name component with register component.
                                    registerMapping[gpr][nameCompToRegComp[comp]].name = name;
                                    registerMapping[gpr][nameCompToRegComp[comp]].component = comp;
                                    registerMapping[gpr][nameCompToRegComp[comp]].freeFromInstr = nameUsage[name].lastUsedByInstr[comp];

                                    nameUsage[name].allocReg[comp] = gpr;
                                    nameUsage[name].allocComp[comp] = nameCompToRegComp[comp];
                                }
                                else if (nameUsage[name].createdByInstr[comp] > (instr + 1))
                                {
                                    //  Associate name component with register component for future allocation.
                                    registerMapping[gpr][nameCompToRegComp[comp]].freeFromInstr = nameUsage[name].lastUsedByInstr[comp];
                                    nameUsage[name].allocReg[comp] = gpr;
                                    nameUsage[name].allocComp[comp] = nameCompToRegComp[comp];
                                }
                            }
                        }
                    }
                    else
                    {
                        //  Allocate registers for the components written by the instruction.
                        for(U32 nameComp = 0; nameComp < 4; nameComp++)
                        {
                            //  Check if the component was created by the instruction.
                            if (nameUsage[name].createdByInstr[nameComp] == (instr + 1))
                            {
                                //  Search for the first free register component.

                                U32 gpr = 0;
                                U32 regComp = 0;
                                bool foundTempRegComp = false;

                                //  Search in the register mapping table for a suitable free register component.
                                while ((gpr < MAX_TEMPORAL_REGISTERS) && !foundTempRegComp)
                                {
                                    //  Check the temporal register component is free.
                                    if (registerMapping[gpr][regComp].freeFromInstr <= nameUsage[name].createdByInstr[nameComp])
                                    {
                                        //  A register component was found.
                                        foundTempRegComp = true;
                                    }
                                    else
                                    {
                                        //  Check the next component.
                                        regComp++;

                                        //  Check if it was the last component in the register.
                                        if (regComp == 4)
                                        {
                                            //  Check the next temporal register.
                                            gpr++;
                                            regComp = 0;
                                        }
                                    }
                                }

                                if (!foundTempRegComp)
                                {
                                    CG_ASSERT("No temporal register available to allocate name.");
                                }

                                //  Allocating name component with register component.
                                registerMapping[gpr][regComp].name = name;
                                registerMapping[gpr][regComp].component = nameComp;
                                registerMapping[gpr][regComp].freeFromInstr = nameUsage[name].lastUsedByInstr[nameComp];

                                nameUsage[name].allocReg[nameComp] = gpr;
                                nameUsage[name].allocComp[nameComp] = regComp;
                            }
                        }
                    }

                    vector<MaskMode> resComponentsMask;
                    vector<SwizzleMode> resComponentsSwizzle;

                    //  Extract components for result.
                    extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                    //  Set translated result register.
                    patchedResReg = nameUsage[name].allocReg[resComponentsSwizzle[0] & 0x03];

                    //  Translate write mask.
                    bool resCompMapping[4];

                    for(U32 resComp = 0; resComp < 4; resComp++)
                        resCompMapping[resComp] = false;

                    bool resultCompRenamed = false;

                    for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                    {
                        resCompMapping[nameUsage[name].allocComp[resComponentsSwizzle[comp] & 0x03]] = true;

                        //  Check if any of the result components were renamed.
                        resultCompRenamed |=  (resComponentsSwizzle[comp] & 0x03) != nameUsage[name].allocComp[resComponentsSwizzle[comp] & 0x03];
                    }

                    patchedResMask = encodeWriteMask(resCompMapping);

                    //  Vector operations require changes in the operand swizzle modes if the result register
                    //  components were renamed.
                    if (resultCompRenamed && isVectorOperation(inProgram[instr]->getOpcode()))
                    {
                        //  Apply the result register remapping to the swizzle modes of the operands.
                        vector<SwizzleMode> op1Comps;
                        vector<SwizzleMode> op2Comps;
                        vector<SwizzleMode> op3Comps;

                        U32 translatedSwzOp1[4];
                        U32 translatedSwzOp2[4];
                        U32 translatedSwzOp3[4];

                        //  Extract components accessed by the operands.
                        extractOperandComponents(patchedSwzModeOp1, op1Comps);
                        extractOperandComponents(patchedSwzModeOp2, op2Comps);
                        extractOperandComponents(patchedSwzModeOp3, op3Comps);

                        //  Check for scalar instructions (single component output).
                        if (resComponentsSwizzle.size() == 1)
                        {
                            //  Use broadcast to make clear that it's a scalar instruction.
                            for(U32 comp = 0; comp < 4; comp++)
                            {
                                translatedSwzOp1[comp] = op1Comps[resComponentsSwizzle[0] & 0x03];
                                translatedSwzOp2[comp] = op2Comps[resComponentsSwizzle[0] & 0x03];
                                translatedSwzOp3[comp] = op3Comps[resComponentsSwizzle[0] & 0x03];
                            }
                        }
                        else
                        {
                            //  Reset translated modes for all operands.
                            for(U32 comp = 0; comp < 4; comp++)
                            {
                                translatedSwzOp1[comp] = 0;
                                translatedSwzOp2[comp] = 0;
                                translatedSwzOp3[comp] = 0;
                            }

                            //  Swizzle the original result name component to the renamed result register component.
                            for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                            {
                                translatedSwzOp1[nameUsage[name].allocComp[resComponentsSwizzle[comp] & 0x03]] = op1Comps[resComponentsSwizzle[comp] & 0x03];
                                translatedSwzOp2[nameUsage[name].allocComp[resComponentsSwizzle[comp] & 0x03]] = op2Comps[resComponentsSwizzle[comp] & 0x03];
                                translatedSwzOp3[nameUsage[name].allocComp[resComponentsSwizzle[comp] & 0x03]] = op3Comps[resComponentsSwizzle[comp] & 0x03];
                            }
                        }
                        
                        //  Recreate the swizzle mode for the operands.
                        patchedSwzModeOp1 = encodeSwizzle(translatedSwzOp1);
                        patchedSwzModeOp2 = encodeSwizzle(translatedSwzOp2);
                        patchedSwzModeOp3 = encodeSwizzle(translatedSwzOp3);
                    }
                    else if (resultCompRenamed && hasSIMD4Result(inProgram[instr]->getOpcode()))
                    {
                        printf("instr = %d\n", instr);
                        CG_ASSERT(
                            "Result component renaming not possible for instructions that produce a SIMD4 result.");
                    }
                }
                else
                {
                    patchedResReg = inProgram[instr]->getResult();
                    patchedResMask = inProgram[instr]->getResultMaskMode();
                }

                //  Patch instruction with the allocated temporal registers.
                cgoShaderInstr *patchedInstr;

                patchedInstr = setRegsAndCompsInstruction(inProgram[instr], patchedResReg, patchedResMask,
                                                          patchedOp1Reg, patchedSwzModeOp1,
                                                          patchedOp2Reg, patchedSwzModeOp2,
                                                          patchedOp3Reg, patchedSwzModeOp3);

                //  Add patched instruction to the output program.
                outProgram.push_back(patchedInstr);

                break;
        }
    }

    /*
    for(U32 name = 1; name < (names + 1); name++)
    {
        for(U32 comp = 0; comp < 4; comp++)
        {
            if (nameUsage[name].createdByInstr[comp] != 0)
                printf("Name %d component %d was allocated to register %d component %d\n", name, comp,
                    nameUsage[name].allocReg[comp], nameUsage[name].allocComp[comp]);

        }
    }
    */

    return maxLiveRegisters;
}

void ShaderOptimization::removeRedundantMOVs(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram)
{
    //  Removes redundant MOVs introduced by renaming.
    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        bool removeInstruction = false;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            case CG1_ISA_OPCODE_MOV:

                //  Check if the move is saturating the result.  A mov_sat is a saturation operation,
                //  not an actual mov.
                if (!inProgram[instr]->getSaturatedRes())
                {
                    //  Check if the first operand is a temporal register and doesn't uses the negate or absolute modifiers.
                    if ((inProgram[instr]->getNumOperands() >= 1) && (inProgram[instr]->getBankOp1() == TEMP) &&
                        (!inProgram[instr]->getOp1AbsoluteFlag()) && (!inProgram[instr]->getOp1NegateFlag()))
                    {
                        //  Get operand register.
                        U32 op1Reg = inProgram[instr]->getOp1();

                        //  Get result register bank.
                        Bank resBank = inProgram[instr]->getBankRes();

                        //  Get result register.
                        U32 resReg = inProgram[instr]->getResult();

                        //  Check if the input and result register are the same.
                        if ((resBank == TEMP) && (resReg == op1Reg))
                        {
                            vector<MaskMode> resComponentsMask;
                            vector<SwizzleMode> resComponentsSwizzle;

                            vector <U32> readComponentsOp1;
                            vector <U32> readComponentsOp2;

                            vector <SwizzleMode> op1Components;

                            //  Extract components for result.
                            extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                            //  Retrieve which operand components are actually read.
                            getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                            //  Extract components for first operand.
                            extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                            bool sameComponents = true;

                            //  Check if it's copying the input components over the same output component.
                            for(U32 comp = 0; (comp < readComponentsOp1.size()) && sameComponents; comp++)
                            {
                                //  Get the actual input component (swizzled) to be written over the output component.
                                U32 readComp = readComponentsOp1[comp];
                                U32 op1RegComp = op1Components[readComp] & 0x03;

                                //  Get the result component being written.
                                U32 resComp = resComponentsSwizzle[comp] & 0x03;

                                //  Check if it is the same component.
                                sameComponents = sameComponents && (op1RegComp == resComp);
                            }

                            //  Set the instruction as to be removed if the input components are copied over the
                            //  same output components.
                            removeInstruction = sameComponents;
                        }
                    }
                }

            default:

                break;
        }

        //  Check if the instruction can be removed.
        if (!removeInstruction)
        {
            //  Copy the instruction to the output program.
            cgoShaderInstr *copyInstr;

            copyInstr = copyInstruction(inProgram[instr]);
            outProgram.push_back(copyInstr);
        }
    }
}

void ShaderOptimization::optimize(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram,
                                  U32 &maxLiveTempRegs, bool noRename, bool AOStoSOA, bool verbose)
{

    bool codeToEliminate = true;
    U32 pass = 0;
    U32 namesUsed = 32;
    U32 liveRegisters = 0;

    vector<cgoShaderInstr *> tempInProgram;
    vector<cgoShaderInstr *> tempOutProgram;
    copyProgram(inProgram, tempInProgram);

    if (!noRename)
    {
        //  Rename temporal registers.
        deleteProgram(tempOutProgram);
        namesUsed = renameRegisters(tempInProgram, tempOutProgram, AOStoSOA);

        if (verbose)
        {
            printf("Pass %d. Register renaming : \n", pass);
            printf("----------------------------------------\n");

            printProgram(tempOutProgram);

            printf("\n");
        }

        //  Prepare for next optimization pass.
        deleteProgram(tempInProgram);
        copyProgram(tempOutProgram, tempInProgram);
        pass++;
    }

    //  Eliminate dead code (registers and components).
    while(codeToEliminate)
    {
        deleteProgram(tempOutProgram);
        codeToEliminate = deadCodeElimination(tempInProgram, namesUsed, tempOutProgram);

        if (verbose)
        {
            printf("Pass %d. Dead code elimination : \n", pass);
            printf("----------------------------------------\n");

            if (codeToEliminate)
                printProgram(tempOutProgram);
            else
                printf(" No code eliminated.\n");

            printf("\n");
        }

        //  Prepare for next optimization pass.
        deleteProgram(tempInProgram);
        copyProgram(tempOutProgram, tempInProgram);
        pass++;
    }

    /*deleteProgram(tempOutProgram);
    copyPropagation(tempInProgram, namesUsed, tempOutProgram);
    copyProgram(tempInProgram, tempOutProgram);

    if (verbose)
    {
        printf("Pass %d.  Copy Propagation : \n", pass);
        printf("----------------------------------------\n");

        printProgram(tempOutProgram);

        printf("\n");
    }*/

    if (!noRename)
    {
        deleteProgram(tempOutProgram);
        liveRegisters = reduceLiveRegisters(tempInProgram, namesUsed, tempOutProgram);

        if (verbose)
        {
            printf("Pass %d.  Live registers reduction : \n", pass);
            printf("----------------------------------------\n");

            printProgram(tempOutProgram);

            printf("\n");
            printf("Max live registers used by the program : %d\n", liveRegisters);

            printf("\n");
        }

        maxLiveTempRegs = liveRegisters;

        deleteProgram(tempInProgram);
        copyProgram(tempOutProgram, tempInProgram);
        pass++;
    }

    deleteProgram(tempOutProgram);
    removeRedundantMOVs(tempInProgram, tempOutProgram);

    if (verbose)
    {
        printf("Pass %d.  Remove redundant MOVs : \n", pass);
        printf("----------------------------------------\n");

        printProgram(tempOutProgram);

        printf("\n");
    }

    deleteProgram(tempInProgram);
    copyProgram(tempOutProgram, tempInProgram);
    pass++;

    codeToEliminate = true;

    //  Eliminate dead code (registers and components).
    while(codeToEliminate)
    {
        deleteProgram(tempOutProgram);
        codeToEliminate = deadCodeElimination(tempInProgram, namesUsed, tempOutProgram);

        if (verbose)
        {
            printf("Pass %d. Dead code elimination : \n", pass);
            printf("----------------------------------------\n");

            if (codeToEliminate)
                printProgram(tempOutProgram);
            else
                printf(" No code eliminated.\n");

            printf("\n");
        }

        //  Prepare for next optimization pass.
        deleteProgram(tempInProgram);
        copyProgram(tempOutProgram, tempInProgram);
        pass++;
    }

    deleteProgram(tempInProgram);
    //copyProgram(tempOutProgram, tempInProgram);
    pass++;

    copyProgram(tempOutProgram, outProgram);
    deleteProgram(tempOutProgram);
}

void ShaderOptimization::assignWaitPoints(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram)
{
    bool regPendingFromLoad[MAX_TEMPORAL_REGISTERS][4];

    //  Clear all registers/components as not pending from a previous texture or attribute load.
    for(U32 reg = 0; reg < MAX_TEMPORAL_REGISTERS; reg++)
    {
        for(U32 comp = 0; comp < 4; comp++)
        {
            regPendingFromLoad[reg][comp] = false;
        }
    }

    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        cgoShaderInstr *copyInstr;

        vector<MaskMode> resComponentsMask;
        vector<SwizzleMode> resComponentsSwizzle;

        vector <U32> readComponentsOp1;
        vector <U32> readComponentsOp2;

        bool setWaitPoint = false;

        switch(inProgram[instr]->getOpcode())
        {
            case CG1_ISA_OPCODE_NOP:
            case CG1_ISA_OPCODE_END:
            case CG1_ISA_OPCODE_CHS:

                //  Those instruction don't set or use registers.

                break;

            case CG1_ISA_OPCODE_FLR:

                //  Unimplemented.

                break;

            default:

                //  Extract components for result.
                extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                //  Retrieve which operand components are actually read.
                getReadOperandComponents(inProgram[instr], resComponentsSwizzle, readComponentsOp1, readComponentsOp2);

                //  Check if the first operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 1) && (inProgram[instr]->getBankOp1() == TEMP))
                {
                    vector <SwizzleMode> op1Components;

                    //  Extract components for first operand.
                    extractOperandComponents(inProgram[instr]->getOp1SwizzleMode(), op1Components);

                    //  Check if any of the operand components is pending from a previous texture or attribute load
                    //  instruction.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        //  Get the actual component being accessed after applying swizzle.
                        U32 readComp = readComponentsOp1[comp];
                        U32 op1RegComp = op1Components[readComp] & 0x03;

                        //  Set a wait point in the previous instruction if the component is still pending from
                        //  a previous texture or attribute load operation.
                        setWaitPoint |= regPendingFromLoad[inProgram[instr]->getOp1()][op1RegComp];
                    }
                }

                //  Check if the second operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() >= 2) && (inProgram[instr]->getBankOp2() == TEMP))
                {
                    vector <SwizzleMode> op2Components;

                    //  Extract components for second operand.
                    extractOperandComponents(inProgram[instr]->getOp2SwizzleMode(), op2Components);

                    //  Check if any of the operand components is pending from a previous texture or attribute load
                    //  instruction.
                    for(U32 comp = 0; comp < readComponentsOp2.size(); comp++)
                    {
                        //  Get the actual component being accessed after applying swizzle.
                        U32 readComp = readComponentsOp2[comp];
                        U32 op2RegComp = op2Components[readComp] & 0x03;

                        //  Set a wait point in the previous instruction if the component is still pending from
                        //  a previous texture or attribute load operation.
                        setWaitPoint |= regPendingFromLoad[inProgram[instr]->getOp2()][op2RegComp];
                    }
                }

                //  Check if the third operand is a temporal register.
                if ((inProgram[instr]->getNumOperands() == 3) && (inProgram[instr]->getBankOp3() == TEMP))
                {
                    vector <SwizzleMode> op3Components;

                    //  Extract components for third operand.
                    extractOperandComponents(inProgram[instr]->getOp3SwizzleMode(), op3Components);

                    //  Check if any of the operand components is pending from a previous texture or attribute load
                    //  instruction.
                    //  The order of the operands actually read for the third operand is the same than for the first operand.
                    for(U32 comp = 0; comp < readComponentsOp1.size(); comp++)
                    {
                        //  Get the actual component being accessed after applying swizzle.
                        U32 readComp = readComponentsOp1[comp];
                        U32 op3RegComp = op3Components[readComp] & 0x03;

                        //  Set a wait point in the previous instruction if the component is still pending from
                        //  a previous texture or attribute load operation.
                        setWaitPoint |= regPendingFromLoad[inProgram[instr]->getOp3()][op3RegComp];
                    }
                }
                
                //  Check for WAW dependences.  Writes on temporary register with a pending result from the texture unit
                //  must wait until the texture unit writes the result.                
                if (inProgram[instr]->getBankRes() == TEMP)
                {
                    for(U32 comp = 0; comp < 4; comp++)
                        setWaitPoint |= regPendingFromLoad[inProgram[instr]->getResult()][comp];
                }

                //  Check if the previous instruction must be marked as a wait point for texture or attributes loaded.
                if (setWaitPoint)
                {
                    //  Mark the previous instruction as a wait point.
                    outProgram[instr - 1]->setWaitPointFlag();

                    //  After a wait point all the load operations are finished so the pending registers can be cleared.

                    //  Clear all registers/components as not pending from a previous texture or attribute load.
                    for(U32 reg = 0; reg < MAX_TEMPORAL_REGISTERS; reg++)
                    {
                        for(U32 comp = 0; comp < 4; comp++)
                        {
                            regPendingFromLoad[reg][comp] = false;
                        }
                    }
                }

                //  Check for texture or attribute load instructions.  If the result register is a temporal register
                //  mark the register as pending from load operation.
                if ((inProgram[instr]->getBankRes() == TEMP) &&
                        ((inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_TEX) || (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_TXB) ||
                         (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_TXL) || (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_TXP) ||
                         (inProgram[instr]->getOpcode() == CG1_ISA_OPCODE_LDA)))
                {
                    //  Extract components for result.
                    extractResultComponents(inProgram[instr]->getResultMaskMode(), resComponentsMask, resComponentsSwizzle);

                    //  Set the written components as pending from load operation.
                    for(U32 comp = 0; comp < resComponentsSwizzle.size(); comp++)
                    {
                        //  Get the actual component being written.
                        U32 writtenComp = resComponentsSwizzle[comp] & 0x03;
                        
                        //  Set written component as pending from load operation.
                        regPendingFromLoad[inProgram[instr]->getResult()][writtenComp] = true;
                    }
                }

                break;
        }

        //  Copy the instruction.
        copyInstr = copyInstruction(inProgram[instr]);
        outProgram.push_back(copyInstr);
    }
}


void ShaderOptimization::decodeProgram(U08 *code, U32 size, vector<cgoShaderInstr *> &outProgram, U32 &numTemps, bool &hasJumps)
{
    bool usedTemps[MAX_TEMPORAL_REGISTERS];
    
    //  Initialize the decoded program.
    outProgram.clear();

    //  No jumps found yet in the program.
    hasJumps = false;

    //  Initialize the used temporary register array.
    for(U32 r = 0; r < MAX_TEMPORAL_REGISTERS; r++)
        usedTemps[r] = false;

    //  No temporary registers used yet.
    numTemps = 0;
        
    //  Decode the instructions of the program.
    for(U32 b = 0; b < size; b += cgoShaderInstr::CG1_ISA_INSTR_SIZE)
    {
        cgoShaderInstr *shInstr;

        shInstr = new cgoShaderInstr(&code[b]);
        
        //  Check for new temporary register used as first operand.
        if ((shInstr->getNumOperands() > 0) && (shInstr->getBankOp1() == TEMP) && !usedTemps[shInstr->getOp1()])
        {
            //  Update the number of temporary registers found in the program.
            numTemps++;
            
            //  Update the array of used temporary registers.
            usedTemps[shInstr->getOp1()];
        }

        //  Check for new temporary register used as second operand.
        if ((shInstr->getNumOperands() > 1) && (shInstr->getBankOp2() == TEMP) && !usedTemps[shInstr->getOp2()])
        {
            //  Update the number of temporary registers found in the program.
            numTemps++;
            
            //  Update the array of used temporary registers.
            usedTemps[shInstr->getOp2()];
        }
            
        //  Check for new temporary register used as second operand.
        if ((shInstr->getNumOperands() > 2) && (shInstr->getBankOp3() == TEMP) && !usedTemps[shInstr->getOp3()])
        {
            //  Update the number of temporary registers found in the program.
            numTemps++;
            
            //  Update the array of used temporary registers.
            usedTemps[shInstr->getOp3()];
        }
        
        //  Check if the instruction is a jump.
        hasJumps |= shInstr->isAJump();

        outProgram.push_back(shInstr);
    }
}


void ShaderOptimization::encodeProgram(vector<cgoShaderInstr *> inProgram, U08 *code, U32 &size)
{
    //  Check size buffer size.
    CG_ASSERT_COND(!(size < (inProgram.size() * cgoShaderInstr::CG1_ISA_INSTR_SIZE)), "Buffer for shader program is too small.");
    size = (U32) inProgram.size() * cgoShaderInstr::CG1_ISA_INSTR_SIZE;

    for(U32 instr = 0; instr < inProgram.size(); instr++)
    {
        inProgram[instr]->getCode(&code[instr * cgoShaderInstr::CG1_ISA_INSTR_SIZE]);
    }
}

void ShaderOptimization::assembleProgram(U08 *program, U08 *code, U32 &size)
{
    stringstream inputProgram;
    vector<cgoShaderInstr*> shaderProgram;

    inputProgram.clear();
    inputProgram.str((char *) program);
    shaderProgram.clear();

    U32 line = 1;

    bool error = false;

    //  Read until the end of the input program or until there is an error assembling the instruction.
    while(!error && !inputProgram.eof())
    {
        cgoShaderInstr *shInstr;

        string currentLine;
        string errorString;

        //  Read a line from the input file.
        getline(inputProgram, currentLine);

        if ((currentLine.length() == 0) && inputProgram.eof())
        {
            //  End loop.
        }
        else
        {
//printf(">> Reading Line %d -> %s\n", line, currentLine.c_str());

            //  Assemble the line.
            shInstr = cgoShaderInstr::assemble((char *) currentLine.c_str(), errorString);

            //  Check if the instruction was assembled.
            if (shInstr == NULL)
            {
                printf("Line %d.  ERROR : %s\n", line, errorString.c_str());
                error = true;
            }
            else
            {
                //  Add instruction to the program.
                shaderProgram.push_back(shInstr);
            }

            //  Update line number.
            line++;
        }
    }

    //  Check if an error occurred.
    if (!error)
    {
        //  Set the end instruction flag to the last instruction in the program.
        shaderProgram[shaderProgram.size() - 1]->setEndFlag(true);
        
        //  Encode the shader program.
        encodeProgram(shaderProgram, code, size);
        deleteProgram(shaderProgram);
    }
    else
    {
        CG_ASSERT("Error assembling program.");
    }
}

void ShaderOptimization::disassembleProgram(U08 *code, U32 codeSize, U08 *program, U32 &size, bool &disableEarlyZ)
{
    vector<cgoShaderInstr*> shaderProgram;
    
    U32 numTemps;
    bool hasJumps;
    
    decodeProgram(code, codeSize, shaderProgram, numTemps, hasJumps);
    
    U32 asmCodeSize = 0;
    
    string asmCode;
    asmCode.clear();
    
    disableEarlyZ = false;
    for(U32 instr = 0; instr < shaderProgram.size(); instr++)
    {
        char buffer[1024];
        shaderProgram[instr]->disassemble(buffer, MAX_INFO_SIZE);
        
        disableEarlyZ |= mustDisableEarlyZ(shaderProgram[instr]->getOpcode());
        
        asmCode.append(buffer);
        asmCode.append("\n");
    }
    
    deleteProgram(shaderProgram);
    
    if (asmCode.size() > size)
    {
        char buffer[128];
        sprintf_s(buffer, sizeof(buffer), "The buffer for the disassembled shader program is too small %d bytes, required %d bytes\n",
            size, asmCode.size());
    }
    
    strcpy((char *) program, asmCode.c_str());
    size = asmCode.size();
}
