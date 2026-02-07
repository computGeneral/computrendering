/**************************************************************************
 *
 */

#include "GALxGenericInstruction.h"
#include <cstdio>
#include <cstring>

using namespace std;
using namespace libGAL::GenerationCode;
using namespace libGAL;


GALxGenericInstruction::GALxGenericInstruction(unsigned int line, const string opcodeString, 
                       GALxGenericInstruction::OperandInfo op1, 
                       GALxGenericInstruction::OperandInfo op2, 
                       GALxGenericInstruction::OperandInfo op3,
                       GALxGenericInstruction::ResultInfo res,
                       GALxGenericInstruction::SwizzleInfo swz, 
                       bool isFixedPoint,
                       unsigned int texImageUnit,
                       unsigned int killSample,
                       unsigned int exportSample,
                       GALxGenericInstruction::TextureTarget texTarget,
                       bool lastInstruction)
                       
    : line(line), opcodeString(opcodeString), op1(op1), op2(op2), op3(op3), res(res),
      swz(swz), isFixedPoint(isFixedPoint), textureImageUnit(texImageUnit), killSample(killSample), 
      exportSample(exportSample), textureTarget(texTarget), lastInstruction(lastInstruction)
{
    if      (!opcodeString.compare("ABS"))      { opcode =  GALxGenericInstruction::G_ABS; num_operands = 1; }
    else if (!opcodeString.compare("ADD"))      { opcode =  GALxGenericInstruction::G_ADD; num_operands = 2; }
    else if (!opcodeString.compare("ARL"))      { opcode =  GALxGenericInstruction::G_ARL; num_operands = 1; }
    else if (!opcodeString.compare("CMP"))      { opcode =  GALxGenericInstruction::G_CMP; num_operands = 3; }
    else if (!opcodeString.compare("CMP_KIL"))  { opcode =  GALxGenericInstruction::G_CMPKIL; num_operands = 3; }
    else if (!opcodeString.compare("CHS"))      { opcode =  GALxGenericInstruction::G_CHS; num_operands = 0; }
    else if (!opcodeString.compare("COS"))      { opcode =  GALxGenericInstruction::G_COS; num_operands = 1; }
    else if (!opcodeString.compare("DP3"))      { opcode =  GALxGenericInstruction::G_DP3; num_operands = 2; }  
    else if (!opcodeString.compare("DP4"))      { opcode =  GALxGenericInstruction::G_DP4; num_operands = 2; }
    else if (!opcodeString.compare("DPH"))      { opcode =  GALxGenericInstruction::G_DPH; num_operands = 2; }
    else if (!opcodeString.compare("DST"))      { opcode =  GALxGenericInstruction::G_DST; num_operands = 2; }
    else if (!opcodeString.compare("EX2"))      { opcode =  GALxGenericInstruction::G_EX2; num_operands = 1; }
    else if (!opcodeString.compare("EXP"))      { opcode =  GALxGenericInstruction::G_EXP; num_operands = 1; }
    else if (!opcodeString.compare("FLR"))      { opcode =  GALxGenericInstruction::G_FLR; num_operands = 1; }
    else if (!opcodeString.compare("FRC"))      { opcode =  GALxGenericInstruction::G_FRC; num_operands = 1; }
    else if (!opcodeString.compare("KIL"))      { opcode =  GALxGenericInstruction::G_KIL; num_operands = 1; }
    else if (!opcodeString.compare("KLS"))      { opcode =  GALxGenericInstruction::G_KLS; num_operands = 1; }
    else if (!opcodeString.compare("LG2"))      { opcode =  GALxGenericInstruction::G_LG2; num_operands = 1; }
    else if (!opcodeString.compare("LIT"))      { opcode =  GALxGenericInstruction::G_LIT; num_operands = 1; }
    else if (!opcodeString.compare("LOG"))      { opcode =  GALxGenericInstruction::G_LOG; num_operands = 1; }
    else if (!opcodeString.compare("LRP"))      { opcode =  GALxGenericInstruction::G_LRP; num_operands = 3; }
    else if (!opcodeString.compare("MAD"))      { opcode =  GALxGenericInstruction::G_MAD; num_operands = 3; }
    else if (!opcodeString.compare("MAX"))      { opcode =  GALxGenericInstruction::G_MAX; num_operands = 2; }
    else if (!opcodeString.compare("MIN"))      { opcode =  GALxGenericInstruction::G_MIN; num_operands = 2; }
    else if (!opcodeString.compare("MOV"))      { opcode =  GALxGenericInstruction::G_MOV; num_operands = 1; }
    else if (!opcodeString.compare("MUL"))      { opcode =  GALxGenericInstruction::G_MUL; num_operands = 2; }
    else if (!opcodeString.compare("POW"))      { opcode =  GALxGenericInstruction::G_POW; num_operands = 2; }
    else if (!opcodeString.compare("RCP"))      { opcode =  GALxGenericInstruction::G_RCP; num_operands = 1; }
    else if (!opcodeString.compare("RSQ"))      { opcode =  GALxGenericInstruction::G_RSQ; num_operands = 1; }
    else if (!opcodeString.compare("SCS"))      { opcode =  GALxGenericInstruction::G_SCS; num_operands = 1; }
    else if (!opcodeString.compare("SGE"))      { opcode =  GALxGenericInstruction::G_SGE; num_operands = 2; }
    else if (!opcodeString.compare("SIN"))      { opcode =  GALxGenericInstruction::G_SIN; num_operands = 1; }
    else if (!opcodeString.compare("SLT"))      { opcode =  GALxGenericInstruction::G_SLT; num_operands = 2; }
    else if (!opcodeString.compare("SUB"))      { opcode =  GALxGenericInstruction::G_SUB; num_operands = 2; }
    else if (!opcodeString.compare("SWZ"))      { opcode =  GALxGenericInstruction::G_SWZ; num_operands = 1; }
    else if (!opcodeString.compare("TEX"))      { opcode =  GALxGenericInstruction::G_TEX; num_operands = 1; }
    else if (!opcodeString.compare("TXB"))      { opcode =  GALxGenericInstruction::G_TXB; num_operands = 1; }
    else if (!opcodeString.compare("TXP"))      { opcode =  GALxGenericInstruction::G_TXP; num_operands = 1; }
    else if (!opcodeString.compare("XPD"))      { opcode =  GALxGenericInstruction::G_XPD; num_operands = 2; }
    else if (!opcodeString.compare("ZXP"))      { opcode =  GALxGenericInstruction::G_ZXP; num_operands = 1; }
    else if (!opcodeString.compare("ZXS"))      { opcode =  GALxGenericInstruction::G_ZXS; num_operands = 1; }
    else if (!opcodeString.compare("FXMUL"))    { opcode =  GALxGenericInstruction::G_FXMUL; num_operands = 2; }
    else if (!opcodeString.compare("FXMAD"))    { opcode =  GALxGenericInstruction::G_FXMAD; num_operands = 3; }
    else if (!opcodeString.compare("FXMAD2"))   { opcode =  GALxGenericInstruction::G_FXMAD2; num_operands = 3; }
    else if (!opcodeString.compare("ABS_SAT"))  { opcode =  GALxGenericInstruction::G_ABS_SAT; num_operands = 1; }
    else if (!opcodeString.compare("ADD_SAT"))  { opcode =  GALxGenericInstruction::G_ADD_SAT; num_operands = 2; }
    else if (!opcodeString.compare("CMP_SAT"))  { opcode =  GALxGenericInstruction::G_CMP_SAT; num_operands = 3; }
    else if (!opcodeString.compare("COS_SAT"))  { opcode =  GALxGenericInstruction::G_COS_SAT; num_operands = 1; }
    else if (!opcodeString.compare("DP3_SAT"))  { opcode =  GALxGenericInstruction::G_DP3_SAT; num_operands = 2; }  
    else if (!opcodeString.compare("DP4_SAT"))  { opcode =  GALxGenericInstruction::G_DP4_SAT; num_operands = 2; }
    else if (!opcodeString.compare("DPH_SAT"))  { opcode =  GALxGenericInstruction::G_DPH_SAT; num_operands = 2; }
    else if (!opcodeString.compare("DST_SAT"))  { opcode =  GALxGenericInstruction::G_DST_SAT; num_operands = 2; }
    else if (!opcodeString.compare("EX2_SAT"))  { opcode =  GALxGenericInstruction::G_EX2_SAT; num_operands = 1; }
    else if (!opcodeString.compare("FLR_SAT"))  { opcode =  GALxGenericInstruction::G_FLR_SAT; num_operands = 1; }
    else if (!opcodeString.compare("FRC_SAT"))  { opcode =  GALxGenericInstruction::G_FRC_SAT; num_operands = 1; }
    else if (!opcodeString.compare("LG2_SAT"))  { opcode =  GALxGenericInstruction::G_LG2_SAT; num_operands = 1; }
    else if (!opcodeString.compare("LIT_SAT"))  { opcode =  GALxGenericInstruction::G_LIT_SAT; num_operands = 1; }
    else if (!opcodeString.compare("LRP_SAT"))  { opcode =  GALxGenericInstruction::G_LRP_SAT; num_operands = 3; }
    else if (!opcodeString.compare("MAD_SAT"))  { opcode =  GALxGenericInstruction::G_MAD_SAT; num_operands = 3; }
    else if (!opcodeString.compare("MAX_SAT"))  { opcode =  GALxGenericInstruction::G_MAX_SAT; num_operands = 2; }
    else if (!opcodeString.compare("MIN_SAT"))  { opcode =  GALxGenericInstruction::G_MIN_SAT; num_operands = 2; }
    else if (!opcodeString.compare("MOV_SAT"))  { opcode =  GALxGenericInstruction::G_MOV_SAT; num_operands = 1; }
    else if (!opcodeString.compare("MUL_SAT"))  { opcode =  GALxGenericInstruction::G_MUL_SAT; num_operands = 2; }
    else if (!opcodeString.compare("POW_SAT"))  { opcode =  GALxGenericInstruction::G_POW_SAT; num_operands = 2; }
    else if (!opcodeString.compare("RCP_SAT"))  { opcode =  GALxGenericInstruction::G_RCP_SAT; num_operands = 1; }
    else if (!opcodeString.compare("RSQ_SAT"))  { opcode =  GALxGenericInstruction::G_RSQ_SAT; num_operands = 1; }
    else if (!opcodeString.compare("SCS_SAT"))  { opcode =  GALxGenericInstruction::G_SCS_SAT; num_operands = 1; }
    else if (!opcodeString.compare("SGE_SAT"))  { opcode =  GALxGenericInstruction::G_SGE_SAT; num_operands = 2; }
    else if (!opcodeString.compare("SIN_SAT"))  { opcode =  GALxGenericInstruction::G_SIN_SAT; num_operands = 1; }
    else if (!opcodeString.compare("SLT_SAT"))  { opcode =  GALxGenericInstruction::G_SLT_SAT; num_operands = 2; }
    else if (!opcodeString.compare("SUB_SAT"))  { opcode =  GALxGenericInstruction::G_SUB_SAT; num_operands = 2; }
    else if (!opcodeString.compare("SWZ_SAT"))  { opcode =  GALxGenericInstruction::G_SWZ_SAT; num_operands = 1; }
    else if (!opcodeString.compare("TEX_SAT"))  { opcode =  GALxGenericInstruction::G_TEX_SAT; num_operands = 1; }
    else if (!opcodeString.compare("TXB_SAT"))  { opcode =  GALxGenericInstruction::G_TXB_SAT; num_operands = 1; }
    else if (!opcodeString.compare("TXP_SAT"))  { opcode =  GALxGenericInstruction::G_TXP_SAT; num_operands = 1; }
    else if (!opcodeString.compare("XPD_SAT"))  { opcode =  GALxGenericInstruction::G_XPD_SAT; num_operands = 2; }
    else { opcode = INVALID_OP; num_operands = 0; }
}

void GALxGenericInstruction::writeOperand(unsigned int numOperand, GALxGenericInstruction::OperandInfo op, std::ostream& os) const
{
    char aux[100];
    char operand[10000];
    
    sprintf(aux,"\t\tOperand %d:\n",numOperand);
    strcpy(operand,aux);
    
    sprintf(aux,"\t\t\tArrayAccessModeFlag: %d\n",(int)op.arrayAccessModeFlag);
    strcat(operand,aux);
    
    if (op.arrayAccessModeFlag && !op.relativeModeFlag)
    {
        sprintf(aux,"\t\t\t\tArrayModeOffset: %d\n",(int)op.arrayModeOffset);
        strcat(operand,aux);
    }
    
    sprintf(aux,"\t\t\tRelativeModeFlag: %d\n",(int)op.relativeModeFlag);
    strcat(operand,aux);
    
    if (op.relativeModeFlag)
    {
        sprintf(aux,"\t\t\t\tRelModeAddrReg: %u\n",op.relModeAddrReg);
        strcat(operand,aux);
        sprintf(aux,"\t\t\t\tRelModeAddrComp: (0:x|1:y|2:z|3:w) %u\n",
                                op.relModeAddrComp);
        strcat(operand,aux);
        sprintf(aux,"\t\t\t\tRelModeOffset: %d\n",(int)op.relModeOffset);
        strcat(operand,aux);
    }

    

    sprintf(aux,"\t\t\tSwizzleMode: 0x%.2X\n",(int)op.swizzleMode);
    strcat(operand,aux);
    
    sprintf(aux,"\t\t\tNegateFlag: %d\n",(int)op.negateFlag);
    strcat(operand,aux);
    
    sprintf(aux,"\t\t\tIdReg: %d\n",(int)op.idReg);
    strcat(operand,aux);
    
    sprintf(aux,"\t\t\tIdBank: %d\n",(int)op.idBank);
    strcat(operand,aux);
    
    os << operand; 
    
}

void GALxGenericInstruction::writeResult(GALxGenericInstruction::ResultInfo res, std::ostream& os) const 
{
    char aux[100];
    char result[10000];
    
    sprintf(aux,"\t\tResult:\n");
    strcpy(result,aux);
    
    sprintf(aux,"\t\t\tWriteMask: 0x%.1X\n",(int)res.writeMask);
    strcat(result,aux);
    
    sprintf(aux,"\t\t\tIdReg: %d\n",(int)res.idReg);
    strcat(result,aux);
    
    sprintf(aux,"\t\t\tIdBank: %d\n",(int)res.idBank);
    strcat(result,aux);
    
    os << result;
}

void GALxGenericInstruction::writeSwizzle(GALxGenericInstruction::SwizzleInfo swz, std::ostream& os) const
{
    char aux[100];
    char swizzle[10000];
    
    sprintf(aux,"\t\tSwizzle Info: Select(0: 0.0|1: 1.0|2: x|3: y|4: z|5: w)\n");
    strcpy(swizzle,aux);
    
    sprintf(aux,"\t\t\txSelect: %d Negate: %d\n",(int)swz.xSelect,(int)swz.xNegate);
    strcat(swizzle,aux);
    
    sprintf(aux,"\t\t\tySelect: %d Negate: %d\n",(int)swz.ySelect,(int)swz.yNegate);
    strcat(swizzle,aux);
    
    sprintf(aux,"\t\t\tzSelect: %d Negate: %d\n",(int)swz.zSelect,(int)swz.zNegate);
    strcat(swizzle,aux);
    
    sprintf(aux,"\t\t\twSelect: %d Negate: %d\n",(int)swz.wSelect,(int)swz.wNegate);
    strcat(swizzle,aux);
    
    os << swizzle; 
}

void GALxGenericInstruction::writeTextureTarget(GALxGenericInstruction::TextureTarget texTarget, std::ostream& os) const
{
    os << "\t\tTexture Target: ";
    
    switch(texTarget)
    {
        case TEXTURE_1D: os << "TEXTURE_1D" << endl; break;
        case TEXTURE_2D: os << "TEXTURE_1D" << endl; break;
        case TEXTURE_3D: os << "TEXTURE_1D" << endl; break;
        case CUBE_MAP: os << "CUBE_MAP" << endl; break;
        case RECT_MAP: os << "RECT" << endl; break;
    }
}

void GALxGenericInstruction::writeTextureImageUnit(unsigned int texImageUnit, std::ostream& os) const
{
    os << "\t\tTexture Image Unit: " << texImageUnit << endl;
}

void GALxGenericInstruction::writeSample(unsigned int sample, std::ostream& os) const
{
    os << "\t\tSample: " << sample << endl;
}

void GALxGenericInstruction::print(std::ostream& os) const
{
    
    char instr[1000];
    char aux[100];

    if (opcode != INVALID_OP) 
    {
        sprintf(aux,"[Line: %d] ",line);
        strcpy(instr,aux);
        os << opcodeString;
        if (lastInstruction)
            os << "(LAST)";
        if (opcode != G_KIL && opcode != G_KLS && opcode != G_ZXP && opcode != G_ZXS && opcode != G_CHS)
            writeResult(res,os);
        writeOperand(1,op1,os);
        switch(opcode)
        {
            case GALxGenericInstruction::G_ABS: break;
            case GALxGenericInstruction::G_ADD: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_ARL: break;
            case GALxGenericInstruction::G_CMP: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_CMPKIL: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_CHS: break;
            case GALxGenericInstruction::G_COS: break;
            case GALxGenericInstruction::G_DP3: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DP4: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DPH: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DST: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_EX2: break;
            case GALxGenericInstruction::G_EXP: break;
            case GALxGenericInstruction::G_FLR: break;
            case GALxGenericInstruction::G_FRC: break;
            case GALxGenericInstruction::G_KIL: break;
            case GALxGenericInstruction::G_KLS: writeSample(exportSample, os); break;
            case GALxGenericInstruction::G_LG2: break;
            case GALxGenericInstruction::G_LIT: break;
            case GALxGenericInstruction::G_LOG: break;
            case GALxGenericInstruction::G_LRP: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_MAD: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_FXMAD: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_FXMAD2: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_MAX: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_MIN: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_MOV: break;
            case GALxGenericInstruction::G_MUL: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_FXMUL: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_POW: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_RCP: break;
            case GALxGenericInstruction::G_RSQ: break;
            case GALxGenericInstruction::G_SCS: break;
            case GALxGenericInstruction::G_SGE: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SIN: break;
            case GALxGenericInstruction::G_SLT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SUB: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SWZ: writeSwizzle(swz,os); break;
            case GALxGenericInstruction::G_TEX: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_TXB: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_TXP: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_XPD: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_ZXP: break;
            case GALxGenericInstruction::G_ZXS: writeSample(exportSample, os); break;
            case GALxGenericInstruction::G_ABS_SAT: break;
            case GALxGenericInstruction::G_ADD_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_CMP_SAT: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_COS_SAT: break;
            case GALxGenericInstruction::G_DP3_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DP4_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DPH_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_DST_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_EX2_SAT: break;
            case GALxGenericInstruction::G_FLR_SAT: break;
            case GALxGenericInstruction::G_FRC_SAT: break;
            case GALxGenericInstruction::G_LG2_SAT: break;
            case GALxGenericInstruction::G_LIT_SAT: break;
            case GALxGenericInstruction::G_LRP_SAT: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_MAD_SAT: writeOperand(2,op2,os); writeOperand(3,op3,os); break;
            case GALxGenericInstruction::G_MAX_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_MIN_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_MOV_SAT: break;
            case GALxGenericInstruction::G_MUL_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_POW_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_RCP_SAT: break;
            case GALxGenericInstruction::G_RSQ_SAT: break;
            case GALxGenericInstruction::G_SCS_SAT: break;
            case GALxGenericInstruction::G_SGE_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SIN_SAT: break;
            case GALxGenericInstruction::G_SLT_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SUB_SAT: writeOperand(2,op2,os); break;
            case GALxGenericInstruction::G_SWZ_SAT: writeSwizzle(swz,os); break;
            case GALxGenericInstruction::G_TEX_SAT: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_TXB_SAT: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_TXP_SAT: writeTextureImageUnit(textureImageUnit,os); writeTextureTarget(textureTarget,os); break;
            case GALxGenericInstruction::G_XPD_SAT: writeOperand(2,op2,os); break;
        }
    }

}
