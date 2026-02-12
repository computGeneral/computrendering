/**************************************************************************
 *
 * Shader Instruction.
 *
 */
#include "ShaderInstr.h"
#include "bmUnifiedShader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace arch;

/**
 *  This table is used to translate from the opcode in the binary encode
 *  of a shader instruction (integer) to the ShOpcode enum type.  A bit
 *  large.
 *
 */

ShOpcode arch::translateShOpcodeTable[] =
{
    CG1_ISA_OPCODE_NOP,    CG1_ISA_OPCODE_ADD,    CG1_ISA_OPCODE_ADDI,   CG1_ISA_OPCODE_ARL,    CG1_ISA_OPCODE_ANDP,   INVOPC, INVOPC, CG1_ISA_OPCODE_COS,        //  Opcodes 00h - 07h
    CG1_ISA_OPCODE_DP3,    CG1_ISA_OPCODE_DP4,    CG1_ISA_OPCODE_DPH,    CG1_ISA_OPCODE_DST,    CG1_ISA_OPCODE_EX2,    CG1_ISA_OPCODE_EXP,    CG1_ISA_OPCODE_FLR,    CG1_ISA_OPCODE_FRC,        //  Opcodes 08h - 0Fh
    CG1_ISA_OPCODE_LG2,    CG1_ISA_OPCODE_LIT,    CG1_ISA_OPCODE_LOG,    CG1_ISA_OPCODE_MAD,    CG1_ISA_OPCODE_MAX,    CG1_ISA_OPCODE_MIN,    CG1_ISA_OPCODE_MOV,    CG1_ISA_OPCODE_MUL,        //  Opcodes 10h - 17h
    CG1_ISA_OPCODE_MULI,   CG1_ISA_OPCODE_RCP,    INVOPC, CG1_ISA_OPCODE_RSQ,    CG1_ISA_OPCODE_SETPEQ, CG1_ISA_OPCODE_SETPGT, CG1_ISA_OPCODE_SGE,    CG1_ISA_OPCODE_SETPLT,     //  Opcodes 18h - 1Fh
    CG1_ISA_OPCODE_SIN,    CG1_ISA_OPCODE_STPEQI, CG1_ISA_OPCODE_SLT,    CG1_ISA_OPCODE_STPGTI, CG1_ISA_OPCODE_STPLTI, CG1_ISA_OPCODE_TXL,    CG1_ISA_OPCODE_TEX,    CG1_ISA_OPCODE_TXB,        //  Opcodes 20h - 27h

    CG1_ISA_OPCODE_TXP,    CG1_ISA_OPCODE_KIL,    CG1_ISA_OPCODE_KLS,    CG1_ISA_OPCODE_ZXP,    CG1_ISA_OPCODE_ZXS,    CG1_ISA_OPCODE_CMP,    CG1_ISA_OPCODE_CMPKIL, CG1_ISA_OPCODE_CHS,        //  Opcodes 28h - 2Fh
    CG1_ISA_OPCODE_LDA,    CG1_ISA_OPCODE_FXMUL,  CG1_ISA_OPCODE_FXMAD,  CG1_ISA_OPCODE_FXMAD2, CG1_ISA_OPCODE_DDX,    CG1_ISA_OPCODE_DDY,    CG1_ISA_OPCODE_JMP,    CG1_ISA_OPCODE_END,        //  Opcodes 30h - 37h
    INVOPC, INVOPC, INVOPC, INVOPC, INVOPC, INVOPC, INVOPC, INVOPC      //  Opcodes 38h - 3Fh
};

/**
 *  Translates from the binary format to the decoded format of
 *  the instruction write mask mode.
 *
 */

MaskMode translateMaskModeTable[] =
{
    NNNN, NNNW, NNZN, NNZW,     //  Mask Modes 0h - 3h  
    NYNN, NYNW, NYZN, NYZW,     //  Mask Modes 4h - 7h  
    XNNN, XNNW, XNZN, XNZW,     //  Mask Modes 8h - Bh  
    XYNN, XYNW, XYZN, mXYZW     //  Mask Modes Ch - Fh  
};

/**
 *  Translates the binary encoding of the swizzle mode to the
 *  enumeration used in the decoded shader instruction.
 *
 */

//  Just a copy of the enum table.  

SwizzleMode arch::translateSwizzleModeTable[] =
{

//  X--- : Swizzle Modes 00h - 3Fh  

      XXXX, XXXY, XXXZ, XXXW,
      XXYX, XXYY, XXYZ, XXYW,
      XXZX, XXZY, XXZZ, XXZW,
      XXWX, XXWY, XXWZ, XXWW,

      XYXX, XYXY, XYXZ, XYXW,
      XYYX, XYYY, XYYZ, XYYW,
      XYZX, XYZY, XYZZ, XYZW,
      XYWX, XYWY, XYWZ, XYWW,

      XZXX, XZXY, XZXZ, XZXW,
      XZYX, XZYY, XZYZ, XZYW,
      XZZX, XZZY, XZZZ, XZZW,
      XZWX, XZWY, XZWZ, XZWW,

      XWXX, XWXY, XWXZ, XWXW,
      XWYX, XWYY, XWYZ, XWYW,
      XWZX, XWZY, XWZZ, XWZW,
      XWWX, XWWY, XWWZ, XWWW,

//  Y--- : Swizzle Modes 40h - 7Fh  

      YXXX, YXXY, YXXZ, YXXW,
      YXYX, YXYY, YXYZ, YXYW,
      YXZX, YXZY, YXZZ, YXZW,
      YXWX, YXWY, YXWZ, YXWW,

      YYXX, YYXY, YYXZ, YYXW,
      YYYX, YYYY, YYYZ, YYYW,
      YYZX, YYZY, YYZZ, YYZW,
      YYWX, YYWY, YYWZ, YYWW,

      YZXX, YZXY, YZXZ, YZXW,
      YZYX, YZYY, YZYZ, YZYW,
      YZZX, YZZY, YZZZ, YZZW,
      YZWX, YZWY, YZWZ, YZWW,

      YWXX, YWXY, YWXZ, YWXW,
      YWYX, YWYY, YWYZ, YWYW,
      YWZX, YWZY, YWZZ, YWZW,
      YWWX, YWWY, YWWZ, YWWW,

//  Z---:  Swizzle Modes 80h - BFh  

      ZXXX, ZXXY, ZXXZ, ZXXW,
      ZXYX, ZXYY, ZXYZ, ZXYW,
      ZXZX, ZXZY, ZXZZ, ZXZW,
      ZXWX, ZXWY, ZXWZ, ZXWW,

      ZYXX, ZYXY, ZYXZ, ZYXW,
      ZYYX, ZYYY, ZYYZ, ZYYW,
      ZYZX, ZYZY, ZYZZ, ZYZW,
      ZYWX, ZYWY, ZYWZ, ZYWW,

      ZZXX, ZZXY, ZZXZ, ZZXW,
      ZZYX, ZZYY, ZZYZ, ZZYW,
      ZZZX, ZZZY, ZZZZ, ZZZW,
      ZZWX, ZZWY, ZZWZ, ZZWW,

      ZWXX, ZWXY, ZWXZ, ZWXW,
      ZWYX, ZWYY, ZWYZ, ZWYW,
      ZWZX, ZWZY, ZWZZ, ZWZW,
      ZWWX, ZWWY, ZWWZ, ZWWW,

//  W--- : Swizzle Modes C0h - FFh  

      WXXX, WXXY, WXXZ, WXXW,
      WXYX, WXYY, WXYZ, WXYW,
      WXZX, WXZY, WXZZ, WXZW,
      WXWX, WXWY, WXWZ, WXWW,

      WYXX, WYXY, WYXZ, WYXW,
      WYYX, WYYY, WYYZ, WYYW,
      WYZX, WYZY, WYZZ, WYZW,
      WYWX, WYWY, WYWZ, WYWW,

      WZXX, WZXY, WZXZ, WZXW,
      WZYX, WZYY, WZYZ, WZYW,
      WZZX, WZZY, WZZZ, WZZW,
      WZWX, WZWY, WZWZ, WZWW,

      WWXX, WWXY, WWXZ, WWXW,
      WWYX, WWYY, WWYZ, WWYW,
      WWZX, WWZY, WWZZ, WWZW,
      WWWX, WWWY, WWWZ, WWWW,

    };

/**
 *  This table translates a register bank in binary encoding to the
 *  enum type Bank used in the decoded instruction.
 *
 */

/*  WARNING:  THE BANK NAMES SHOULD BE GENERIC ONES (VERTEX/PIXEL SHADER).
    THIS WILL BE CHANGED IN A FUTURE VERSION.  */

Bank arch::translateBankTable[] =
{
   IN,          //  Bank 0
   OUT,         //  Bank 1
   PARAM,       //  Bank 2
   TEMP,        //  Bank 3
   ADDR,        //  Bank 4
   PARAM2,      //  Bank 5
   IMM,         //  Bank 6
   INVALID      //  Bank 7
};

//  Tables used for disassembling the instruction.

//  Shader Opcode to string.
const char * shOpcode2Str[] =
{
    "nop",    "add",    "addi",   "arl",    "andp",   "inv",    "inv",    "cos",     //  Opcodes 00h - 07h
    "dp3",    "dp4",    "dph",    "dst",    "ex2",    "exp",    "flr",    "frc",     //  Opcodes 08h - 0Fh
    "lg2",    "lit",    "log",    "mad",    "max",    "min",    "mov",    "mul",     //  Opcodes 10h - 17h
    "muli",   "rcp",    "inv",    "rsq",    "setpeq", "setpgt", "sge",    "setplt",  //  Opcodes 18h - 1Fh
    "sin",    "stpeqi", "slt",    "stpgti", "stplti", "txl",    "tex",    "txb",     //  Opcodes 20h - 27h
    "txp",    "kil",    "kls",    "zxp",    "zxs",    "cmp",    "cmpkil", "chs",     //  Opcodes 28h - 2Fh
    "lda",    "fxmul",  "fxmad",  "fxmad2", "ddx",    "ddy",    "jmp",    "end",     //  Opcodes 30h - 37h
    "invopc"
};

//  Mask Mode to string.
const char  *maskMode2Str[] =
{
    "!INV!", ".w", ".z", ".zw",     //  Mask Modes 0h - 3h
    ".y", ".yw", ".yz", ".yzw",     //  Mask Modes 4h - 7h
    ".x", ".xw", ".xz", ".xzw",     //  Mask Modes 8h - Bh
    ".xy", ".xyw", ".xyz", ""       //  Mask Modes Ch - Fh
};

//  Swizzle Mask to string.
char swizzleMode2Str[] =
{
    'x',
    'y',
    'z',
    'w'
};


/*
 *  Decode and store the shader instruction.
 *
 */

cgoShaderInstr::cgoShaderInstr(U08 *instrCode)
{
    // reset binary code
    memset(code.code8, 0, CG1_ISA_INSTR_SIZE);

    //  WARNING:  THIS FUNCTION DOES NOT CHECK OVERFLOW IN THE
    //    ARRAY WITH THE INSTRUCTION CODIFICATION!!!!!

    //  Copies to instruction binary encoding.
    memcpy(code.code8, instrCode, CG1_ISA_INSTR_SIZE);

    //  Gets the instruction opcode.
    if (code.fields.lo64Bits.opcode < LASTOPC)
        opcode = translateShOpcodeTable[code.fields.lo64Bits.opcode];
    else
        opcode = INVOPC;        

    //  Check for invalid encodings.
    if (opcode == INVOPC)
    {
        CG_ASSERT("Invalid opcode\n");
    }

    //
    //  The following expressions compute a number of derived values for the instruction
    //  to be used as help for the decoding of the instruction.
    //

    //  Set the number of operands for this opcode.
    numOperands = setNumOperands(opcode);

    //  Set if the instruction has a result.
    hasResultB = setHasResult(opcode);
    
    //  Set if this instruction is an integer instruction.
    isIntegerB = (opcode == CG1_ISA_OPCODE_ARL) || (opcode == CG1_ISA_OPCODE_ADDI) || (opcode == CG1_ISA_OPCODE_MULI) || (opcode == CG1_ISA_OPCODE_STPEQI) || (opcode == CG1_ISA_OPCODE_STPGTI) || (opcode == CG1_ISA_OPCODE_STPLTI);

    //  Set if this instruction is a load instruction.
    isALoadB = (opcode == CG1_ISA_OPCODE_TEX) || (opcode == CG1_ISA_OPCODE_TXB) || (opcode == CG1_ISA_OPCODE_TXP) || (opcode == CG1_ISA_OPCODE_TXL) || (opcode == CG1_ISA_OPCODE_LDA);

    //  Set if this instruction is a store instruction.
    isAStoreB = false;

    //  Set if this instruction is a Z export instruction.
    isZExportB = (opcode == CG1_ISA_OPCODE_ZXP) || (opcode == CG1_ISA_OPCODE_ZXS); 
    
    //  Set if the instruction result is a predicate register.
    isPredInstrB = resultIsPredicateReg(opcode);
   
    //  Set if the instruction is a jump.
    isAJumpB = (opcode == CG1_ISA_OPCODE_JMP);
    
    //  Reset immediate field.
    immediate = 0;
    
    //  Get the wait point flag from the instruction.
    waitPointFlag = code.fields.lo64Bits.waitpoint;

    //  Get predicated instruction flag.
    predicatedFlag = code.fields.lo64Bits.predicated;
    
    //  Get negate predicate register flag.
    negatePredFlag = code.fields.lo64Bits.invertpred;
    
    //  Get predicate register for the instruction.
    predicateReg = code.fields.lo64Bits.predreg;
    
    //  Check the instruction opcode.
    switch(opcode)
    {
        //  Opcodes without operands.
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_CHS:
        
            //  Get end flag for the instruction.
            endFlag = code.fields.lo64Bits.endflag;

            //  Doesn't have immediate operands.
            hasImmediateB = false;
                        
            //  Set default values in all unused fields.
            relativeModeFlag = false;
            op1 = op2 = op3 = res = 0;
            bankOp1 = bankOp2 = bankOp3 = bankRes = INVALID;
            negateFlagOp1 = negateFlagOp2 = negateFlagOp3 = false;
            absoluteFlagOp1 = absoluteFlagOp2 = absoluteFlagOp3 = false;
            swizzleModeOp1 = swizzleModeOp2 = swizzleModeOp3 = XYZW;
            maskMode = mXYZW;
            relModeAddrReg = 0;
            relModeAddrRegComp = 0;
            relModeOffset = 0;
            saturatedRes = false;
            
            break;

        //  Predicate operator.
        
        case CG1_ISA_OPCODE_ANDP:
        
            //  Get first operand register identifier.
            op1 = code.fields.hi64Bits.operands.op1reg;
            
            //  Get second operand register identifier.
            op2 = code.fields.hi64Bits.operands.op2reg;
            
            //  Get first operand register bank.
            bankOp1 = translateBankTable[code.fields.lo64Bits.op1bank];

            //  Check for constant register bank extension.
            if (bankOp1 == PARAM2)
            {
                //  Add the offset for the start of the constant bank extension.
                op1 += 256;
                
                //  Internally there treat as a normal constant bank register.
                bankOp1 = PARAM;
            }
            
            //  Implicit predicate result of a constant register is not used.
            if (bankOp1 != PARAM)
                bankOp1 = PRED;

            //  Get second operand register bank.
            bankOp2 = translateBankTable[code.fields.lo64Bits.op2bank];
            
            //  Check for constant register bank extension.
            if (bankOp2 == PARAM2)
            {
                //  Add the offset for the start of the constant bank extension.
                op2 += 256;
                
                //  Internally there treat as a normal constant bank register.
                bankOp2 = PARAM;
            }

            //  Implicit predicate result of a constant register is not used.
            if (bankOp2 != PARAM)
                bankOp2 = PRED;

            //  Get first operand negate flag.
            negateFlagOp1 = code.fields.lo64Bits.op1negate;

            //  Get second operand negate flag.
            negateFlagOp2 = code.fields.lo64Bits.op2negate;

            //  Get first operand absolute flag.
            absoluteFlagOp1 = code.fields.lo64Bits.op1absolute;
            
            //  Get second operand absolute flag.
            absoluteFlagOp2 = code.fields.lo64Bits.op2absolute;

            //  Get result register number.  The implicit bank is predicate.
            res = code.fields.hi64Bits.operands.resreg;
            bankRes = PRED;

            //  Get saturated result flag.
            saturatedRes = code.fields.lo64Bits.saturateres;
            
            //  Get end flag for the instruction.
            endFlag = code.fields.lo64Bits.endflag;

            //  Get first operand swizzle mode.
            swizzleModeOp1 = translateSwizzleModeTable[code.fields.hi64Bits.operands.op1swizzle];

            //  Get second operand swizzle mode.
            swizzleModeOp2 = translateSwizzleModeTable[code.fields.hi64Bits.operands.op2swizzle];

            //  Get relative mode flag.
            relativeModeFlag = code.fields.lo64Bits.relmode;

            //  Get Relative Mode Addressing Register number.
            relModeAddrReg = code.fields.lo64Bits.reladreg;

            //  Get Relative Mode Adressing Register Component.
            relModeAddrRegComp = code.fields.lo64Bits.relmode;

            //  Get Relative Mode Addressing Offset.
            relModeOffset = ((code.fields.lo64Bits.reloffset & 0x100) != 0) ? S16(code.fields.lo64Bits.reloffset | 0xff00) :
                                                                              S16(code.fields.lo64Bits.reloffset);

            //  CG1_ISA_OPCODE_ANDP doesn't support immediate operand.
            hasImmediateB = false;

            //  Set default values for undefined parameters.
            negateFlagOp3 = false;
            absoluteFlagOp3 = false;
            swizzleModeOp3 = arch::XYZW;

            break;
            
        //  Jump instructions.
        case CG1_ISA_OPCODE_JMP:
        
            //  Get first operand register identifier.
            op1 = code.fields.hi64Bits.immediate.op1reg;
            
            //  Get first operand register bank.
            bankOp1 = translateBankTable[code.fields.lo64Bits.op1bank];

            //  Check for constant register bank extension.
            if (bankOp1 == PARAM2)
            {
                //  Add the offset for the start of the constant bank extension.
                op1 += 256;
                
                //  Internally there treat as a normal constant bank register.
                bankOp1 = PARAM;
            }
            
            //  Implicit predicate result of a constant register is not used.
            if (bankOp1 != PARAM)
                bankOp1 = PRED;

            //  Get jump offset (immediate field).
            op2 = immediate = S32(code.fields.hi64Bits.immediate.immediate);
            
            //  Jumps have an immediate operand.
            hasImmediateB = true;

            //  Get first operand negate flag.
            negateFlagOp1 = code.fields.lo64Bits.op1negate;

            //  Get first operand absolute flag.
            absoluteFlagOp1 = code.fields.lo64Bits.op1absolute;

            //  Get end flag for the instruction.
            endFlag = code.fields.lo64Bits.endflag;

            //  Get first operand swizzle mode.
            swizzleModeOp1 = translateSwizzleModeTable[code.fields.hi64Bits.immediate.op1swizzle];

            //  Get relative mode flag.
            relativeModeFlag = code.fields.lo64Bits.relmode;

            //  Get Relative Mode Addressing Register number.
            relModeAddrReg = code.fields.lo64Bits.reladreg;

            //  Get Relative Mode Adressing Register Component.
            relModeAddrRegComp = code.fields.lo64Bits.reladcomp;

            //  Get Relative Mode Addressing Offset.
            relModeOffset = ((code.fields.lo64Bits.reloffset & 0x100) != 0) ? S16(code.fields.lo64Bits.reloffset | 0xff00) :
                                                                              S16(code.fields.lo64Bits.reloffset);

            //  Set default values for undefined parameters.
            op3 = res = 0;
            bankOp2 = bankOp3 = bankRes = INVALID;
            negateFlagOp2 = negateFlagOp3 = false;
            absoluteFlagOp2 = absoluteFlagOp3 = false;
            swizzleModeOp2 = swizzleModeOp3 = XYZW;
            maskMode = mXYZW;
            saturatedRes = false;
            
            break;
                        
        //  Regular opcodes
        default:            

            //  Get first operand register number.
            op1 = code.fields.hi64Bits.operands.op1reg;

            //  Check if the instruction supports a second operand.
            if (numOperands > 1)
            {
                //  Get second operand register number.
                op2 = code.fields.hi64Bits.operands.op2reg;
            }
            else
            {
                //  Reset second operand identifier.
                op2 = 0;
            }

            //  Check if the instruction supports a third operand.
            if (numOperands > 2)
            {        
                //  Get third operand register number.
                op3 = code.fields.hi64Bits.operands.op3reg;
            }
            else
            {
                //  Reset third operand identifier.
                op3 = 0;
            }
            
            //  Get first operand register bank.
            bankOp1 = translateBankTable[code.fields.lo64Bits.op1bank];
            
            //  Check for constant register bank extension.
            if (bankOp1 == PARAM2)
            {
                //  Add the offset for the start of the constant bank extension.
                op1 += 256;
                
                //  Internally there treat as a normal constant bank register.
                bankOp1 = PARAM;
            }

            //  Check if it's a valid operand bank.
            if (!validOperandBank(bankOp1))
            {
                char buffer[128];
                sprintf_s(buffer, sizeof(buffer), "Invalid first operand bank %s for opcode %s\n", getBankString(bankOp1), getOpcodeString(opcode));
                CG_ASSERT(buffer);
            }

            //  Check if the instruction supports a second operand.
            if (numOperands > 1)
            {        
                //  Check for texture instructions (CG1_ISA_OPCODE_TEX, CG1_ISA_OPCODE_TXB, CG1_ISA_OPCODE_TXL, CG1_ISA_OPCODE_TXP, CG1_ISA_OPCODE_LDA).
                if (isALoadB)
                {
                    //  Second operand bank is implicit.
                    bankOp2 = TEXT;
                }
                else if ((opcode == CG1_ISA_OPCODE_KLS) || (opcode == CG1_ISA_OPCODE_ZXS))
                {
                    //  Second operand bank is implicit.
                    bankOp2 = SAMP;
                }
                else
                {
                    //  Get second operand register bank.
                    bankOp2 = translateBankTable[code.fields.lo64Bits.op2bank];
                    
                    //  Check for constant register bank extension.
                    if (bankOp2 == PARAM2)
                    {
                        //  Add the offset for the start of the constant bank extension.
                        op2 += 256;
                        
                        //  Internally there treat as a normal constant bank register.
                        bankOp2 = PARAM;
                    }
                    
                    //  Check for immediate operand.
                    if (bankOp2 == IMM)
                    {
                        //  Check the number of operands.
                        if (numOperands > 2)
                            CG_ASSERT("Instructions with immediate second operand don't support three operands.");
                            
                        //  Get the immediate operand.
                        op2 = immediate = code.fields.hi64Bits.immediate.immediate;
                        
                        //  The instruction has an immediate operand.
                        hasImmediateB = true;
                    }
                    else
                    {
                        //  The instruction doesn't have an immediate operand.
                        hasImmediateB = false;
                        
                        //  Check if it's a valid operand bank.
                        if (!validOperandBank(bankOp2))
                        {
                            char buffer[128];
                            sprintf_s(buffer, sizeof(buffer), "Invalid second operand bank %s for opcode %s\n", getBankString(bankOp2), getOpcodeString(opcode));
                            CG_ASSERT(buffer);
                        }
                    }
                }
            }
            else
            {
                //  Set second operand bank to not valid.
                bankOp2 = INVALID;            
            }

            //  Check if the instruction supports a third operand.
            if (numOperands > 2)
            {
                //  Get third operand register bank.
                bankOp3 = translateBankTable[code.fields.lo64Bits.op3bank];
                
                //  Check for constant register bank extension.
                if (bankOp3 == PARAM2)
                {
                    //  Add the offset for the start of the constant bank extension.
                    op3 += 256;
                    
                    //  Internally there treat as a normal constant bank register.
                    bankOp3 = PARAM;
                }
                
                //  Check if it's a valid operand bank.
                if (!validOperandBank(bankOp3))
                {
                    char buffer[128];
                    sprintf_s(buffer, sizeof(buffer), "Invalid third operand bank %s for opcode %s\n", getBankString(bankOp3), getOpcodeString(opcode));
                    CG_ASSERT(buffer);
                }
            }
            else
            {
                //  Set third operand bank to not valid.   
                bankOp3 = INVALID;
            }

            //  Get result register number.
            res = code.fields.hi64Bits.operands.resreg;

            //  Check if the instruction result is to a predicate register.
            if (isPredInstrB)
            {
                bankRes = PRED;
                saturatedRes = false;
                maskMode = XNNN;
            }
            else
            {
                //  Get result register bank.
                bankRes = translateBankTable[code.fields.lo64Bits.resbank];
                
                //  Check if the instruction returns a result and the result bank is valid.
                if (hasResultB && !validResultBank(bankRes))
                {
                    char buffer[128];
                    sprintf_s(buffer, sizeof(buffer), "Invalid result bank %s for opcode %s\n", getBankString(bankRes), getOpcodeString(opcode));
                    CG_ASSERT(buffer);
                }
                   
                //  Get saturated result flag.
                saturatedRes = code.fields.lo64Bits.saturateres;

                //  Get result mask mode.
                maskMode = translateMaskModeTable[code.fields.lo64Bits.mask];
            }
            
            //  Get first operand negate flag.
            negateFlagOp1 = code.fields.lo64Bits.op1negate;
            
            //  Get second operand negate flag.
            negateFlagOp2 = code.fields.lo64Bits.op2negate;

            //  Get third operand negate flag.
            negateFlagOp3 = code.fields.lo64Bits.op3negate;

            //  Get first operand absolute flag.
            absoluteFlagOp1 = code.fields.lo64Bits.op1absolute;

            //  Get second operand absolute flag.
            absoluteFlagOp2 = code.fields.lo64Bits.op2absolute;

            //  Get third operand absolute flag.
            absoluteFlagOp3 = code.fields.lo64Bits.op3absolute;

            //  Get end flag for the instruction.
            endFlag = code.fields.lo64Bits.endflag;

            //  Get first operand swizzle mode.
            swizzleModeOp1 = translateSwizzleModeTable[code.fields.hi64Bits.operands.op1swizzle];
            
            //  Get second operand swizzle mode.
            swizzleModeOp2 = translateSwizzleModeTable[code.fields.hi64Bits.operands.op2swizzle];

            //  Get third operand swizzle mode.
            swizzleModeOp3 = translateSwizzleModeTable[code.fields.hi64Bits.operands.op3swizzle];

            //  Get relative mode flag.
            relativeModeFlag = code.fields.lo64Bits.relmode;
            
            //  Get Relative Mode Addressing Register number.
            relModeAddrReg = code.fields.lo64Bits.reladreg;

            //  Get Relative Mode Adressing Register Component.
            relModeAddrRegComp = code.fields.lo64Bits.reladcomp;

            //  Get Relative Mode Addressing Offset.
            relModeOffset = ((code.fields.lo64Bits.reloffset & 0x100) != 0) ? S16(code.fields.lo64Bits.reloffset | 0xff00) :
                                                                              S16(code.fields.lo64Bits.reloffset);
            
            break;
    }            
     

    //  Set if this instruction is an End instruction or an arithmetic instruction
    //  marked as the last kernel instruction.
    isEndB = (opcode == CG1_ISA_OPCODE_END) || endFlag;

    //  Set if this instruction is a float instruction.
    isFloatB = !isIntegerB;

    //  Set if this instruction is a scalar instruction.
    isScalarB = setIsScalar(opcode, maskMode);

    //  Set if this instruction is compatible with a Scalar architecture.
    isSOACompatibleB = setIsSOACompatible(opcode, maskMode);

    setTag("ShInst");
}

//  Store and encode instruction (math opcodes).  End flag version.
cgoShaderInstr::cgoShaderInstr(ShOpcode opc,                          //  Instruction Opcode.
        Bank op1B, U32 op1R, bool op1N, bool op1A, SwizzleMode op1M,     //  First Operand parameters.
        Bank op2B, U32 op2R, bool op2N, bool op2A, SwizzleMode op2M,     //  Second Operand parameters.
        Bank op3B, U32 op3R, bool op3N, bool op3A, SwizzleMode op3M,     //  Second Operand parameters.
        Bank resB, U32 resR, bool satRes, MaskMode resM,                 //  Result parameters.
        bool predicated, bool negatePred, U32 predR,                     //  Predication parameters.
        bool relMode,                                                       //  Relative Mode Flag.
        U32 relModeReg, U08 relModeComp, S16 relModeOff,            //  Relative Mode parameters.
        bool lastInstr,                                                     //  Last instruction in a kernel.
        bool waitPoint                                                      //  Mark instruction as a wait point
        ):
//  Create the instruction and set the instruction parameters.  */

    opcode(opc),
    bankOp1(op1B), op1(op1R), negateFlagOp1(op1N), absoluteFlagOp1(op1A), swizzleModeOp1(op1M),
    bankOp2(op2B), op2(op2R), negateFlagOp2(op2N), absoluteFlagOp2(op2A), swizzleModeOp2(op2M),
    bankOp3(op3B), op3(op3R), negateFlagOp3(op3N), absoluteFlagOp3(op3A), swizzleModeOp3(op3M),
    bankRes(resB), res(resR), maskMode(resM), saturatedRes(satRes),
    predicatedFlag(predicated), negatePredFlag(negatePred), predicateReg(predR),
    relativeModeFlag(relMode),
    relModeAddrReg(relModeReg), relModeAddrRegComp(relModeComp), relModeOffset(relModeOff),
    endFlag(lastInstr), waitPointFlag(waitPoint)
{
    //
    //  This section sets a number of derived values useful for helping decode the
    //  shader instruction.
    //

    //  Set the number of operands for this opcode.
    numOperands = setNumOperands(opcode);

    //  Set if the instruction has a result.
    hasResultB = setHasResult(opcode);

    //  Set if this instruction is an End instruction.
    isEndB = (opcode == CG1_ISA_OPCODE_END) || endFlag;

    //  Set if this instruction is an integer instruction.
    isIntegerB = (opcode == CG1_ISA_OPCODE_ARL) || (opcode == CG1_ISA_OPCODE_ADDI) || (opcode == CG1_ISA_OPCODE_MULI) || (opcode == CG1_ISA_OPCODE_STPEQI) || (opcode == CG1_ISA_OPCODE_STPGTI) || (opcode == CG1_ISA_OPCODE_STPLTI);

    //  Set if this instruction is a float instruction.
    isFloatB = !isIntegerB;

    //  Set if this instruction is a scalar instruction.
    isScalarB = setIsScalar(opcode, maskMode);

    //  Set if this instruction is compatible with a Scalar architecture.
    isSOACompatibleB = setIsSOACompatible(opcode, maskMode);

    //  Set if this instruction is a load instruction.
    isALoadB = (opcode == CG1_ISA_OPCODE_TEX) || (opcode == CG1_ISA_OPCODE_TXP) || (opcode == CG1_ISA_OPCODE_TXB) || (opcode == CG1_ISA_OPCODE_TXL) || (opcode == CG1_ISA_OPCODE_LDA);

    //  Set if this instruction is a store instruction.
    isAStoreB = false;

    //  Set if this instruction is a Z export instruction.
    isZExportB = (opcode == CG1_ISA_OPCODE_ZXP) || (opcode == CG1_ISA_OPCODE_ZXS);

    //  Set if the instruction result is a predicate register.
    isPredInstrB = resultIsPredicateReg(opcode);

    //  Set if the instruction is a jump instruction.
    isAJumpB = (opcode == CG1_ISA_OPCODE_JMP);
    
    //  Check for instructions without operands or result.
    switch(opcode)
    {
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_CHS:
        
            //  Set default values in all unused fields.
            relativeModeFlag = false;
            op1 = op2 = op3 = res = 0;
            bankOp1 = bankOp2 = bankOp3 = bankRes = INVALID;
            negateFlagOp1 = negateFlagOp2 = negateFlagOp3 = false;
            absoluteFlagOp1 = absoluteFlagOp2 = absoluteFlagOp3 = false;
            swizzleModeOp1 = swizzleModeOp2 = swizzleModeOp3 = XYZW;
            maskMode = mXYZW;
            relModeAddrReg = 0;
            relModeAddrRegComp = 0;
            relModeOffset = 0;
            saturatedRes = false;
            immediate = 0;
            hasImmediateB = false;
            
            break;
        
        case CG1_ISA_OPCODE_ANDP:
        
            //  Only predicate or constant operands allowed.
            if (bankOp1 != PARAM)
                bankOp1 = PRED;
            if (bankOp2 != PARAM)
                bankOp2 = PRED;

            //  The result is always a predicate register.
            bankRes = PRED;
                            
            //  Default values for unused fields.
            op3 = 0;
            bankOp3 = INVALID;
            negateFlagOp3 = false;
            absoluteFlagOp3 = false;
            swizzleModeOp3 = XXXX;
            maskMode = mXYZW;
            immediate = 0;
            hasImmediateB = false;
            
            break;

        case CG1_ISA_OPCODE_JMP:
        
            //  Only predicate or constant operands allowed.
            if (bankOp1 != PARAM)
                bankOp1 = PRED;
                            
            //  Jump instruction have an immediate field.
            hasImmediateB = true;
            immediate = op2;
            
            //  Default values for unused fields.
            op3 = res = 0;
            bankOp2 = bankOp3 = bankRes = INVALID;
            negateFlagOp2 = negateFlagOp3 = false;
            absoluteFlagOp2 = absoluteFlagOp3 = false;
            swizzleModeOp2 = swizzleModeOp3 = XXXX;
            maskMode = mXYZW;
            saturatedRes = false;

            break;
            
        default:
        
            //  Check first operand.
            if (!validOperandBank(bankOp1))
            {
                char buffer[128];
                sprintf_s(buffer, sizeof(buffer), "Invalid first operand bank %s for opcode %s\n", getBankString(bankOp1), getOpcodeString(opcode));
                CG_ASSERT(buffer);
            }
            
            //  Check if the instruction supports a third operand.        
            if (numOperands > 1)
            {
                //  Check for texture instructions.
                if (isALoadB)
                {
                    //  Texture unit identifier is implicit.
                    bankOp2 = TEXT;
                    
                    //  The instruction doesn't have an immediate operand.
                    hasImmediateB = false;
                }
                else if ((opcode == CG1_ISA_OPCODE_KLS) || (opcode == CG1_ISA_OPCODE_ZXS))
                {
                    //  Fragment sample identifier is implicit.
                    bankOp2 = SAMP;

                    //  The instruction doesn't have an immediate operand.
                    hasImmediateB = false;
                }
                else
                {
                    //  Check for immediate operand.
                    if (bankOp2 == IMM)
                    {
                        //  Check the instruction operands.
                        if (numOperands > 2)
                            CG_ASSERT("Instructions with immediate operand don't support a third operand.");
                            
                        //  The instruction has an immediate operand.
                        hasImmediateB = true;
                        
                        //  Set immediate value.
                        immediate = op2;
                    }
                    else
                    {
                        //  The instruction doesn't have an immediate operand.
                        hasImmediateB = false;
                        
                        //  Check second operand.
                        if (!validOperandBank(bankOp2))
                        {
                            char buffer[128];
                            sprintf_s(buffer, sizeof(buffer), "Invalid second operand bank %s for opcode %s\n", getBankString(bankOp2), getOpcodeString(opcode));
                            CG_ASSERT(buffer);
                        }
                    }
                }
            }
            else
            {
                //  Reset third operand.
                bankOp2 = INVALID;
                op2 = 0;            

                //  The instruction doesn't have an immediate operand.
                hasImmediateB = false;
            }
            
            //  Check if the instruction supports a second operand.
            if (numOperands > 2)
            {
                //  Check third operand.
                if (!validOperandBank(bankOp3))
                {
                    char buffer[128];
                    sprintf_s(buffer, sizeof(buffer), "Invalid third operand bank %s for opcode %s\n", getBankString(bankOp3), getOpcodeString(opcode));
                    CG_ASSERT(buffer);
                }
            }
            else
            {
                //  Reset second operand.
                bankOp3 = INVALID;
                op3 = 0;
            }
            
            // Check if the result register must be a predicate register.        
            if (isPredInstrB)
            {
                bankRes = PRED;
                saturatedRes = false;
                maskMode = XNNN;
            }
            else
            {
                //  Check if the instruction returns a result and if the bank is valid.
                if (hasResultB && !validResultBank(bankRes))
                {
                    char buffer[128];
                    sprintf_s(buffer, sizeof(buffer), "Invalid result bank %s for opcode %s\n", getBankString(bankRes), getOpcodeString(opcode));
                    CG_ASSERT(buffer);
                }
            }
            
            break;
    }  

    // reset binary code
    memset(code.code8, 0, CG1_ISA_INSTR_SIZE);

    //  Encode the instruction.

    //  Encode instruction opcode.
    code.fields.lo64Bits.opcode = opcode & 0xff;

    //  Encode instruction waitpoint and end flags.
    code.fields.lo64Bits.waitpoint = waitPointFlag ? 1 : 0;
    code.fields.lo64Bits.endflag = endFlag ? 1 : 0;

    //  Encode predication parameters.
    code.fields.lo64Bits.predicated = predicatedFlag ? 1 : 0;
    code.fields.lo64Bits.invertpred = negatePredFlag ? 1 : 0;
    code.fields.lo64Bits.predreg = predicateReg & 0x1f;    

    //  Encode negate and absolute flags for the three operands.
    code.fields.lo64Bits.op1negate = negateFlagOp1 ? 1 : 0;
    code.fields.lo64Bits.op2negate = negateFlagOp2 ? 1 : 0;
    code.fields.lo64Bits.op3negate = negateFlagOp3 ? 1 : 0;
    code.fields.lo64Bits.op1absolute = absoluteFlagOp1 ? 1 : 0;
    code.fields.lo64Bits.op2absolute = absoluteFlagOp2 ? 1 : 0;
    code.fields.lo64Bits.op3absolute = absoluteFlagOp3 ? 1 : 0;

    //  Encode instruction result fields.
    code.fields.lo64Bits.mask = maskMode & 0x0f;
    code.fields.lo64Bits.resbank = bankRes & 0x07;
    code.fields.lo64Bits.saturateres = saturatedRes ? 1 : 0;

    //  Encode relative access to constant bank fields.
    code.fields.lo64Bits.relmode = relativeModeFlag ? 1 : 0;
    code.fields.lo64Bits.reladreg = relModeAddrReg & 0x03;
    code.fields.lo64Bits.reladcomp = relModeAddrRegComp & 0x03;
    code.fields.lo64Bits.reloffset = relModeOffset & 0x1ff;

    U32 op1Aux;
    U32 op2Aux;
    U32 op3Aux;
    Bank op1BankAux;
    Bank op2BankAux;
    Bank op3BankAux;
    
    //  Check for the constant bank extension.
    if ((bankOp1 == PARAM) && (op1 > 255))
    {
        //  Substract the constant bank extension start offset.
        op1Aux = op1 - 256;
        
        //  Set the bank to the constant bank extension.
        op1BankAux = PARAM2;
    }
    else
    {
        op1Aux = op1;
        op1BankAux = bankOp1;
    }

    //  Check for the constant bank extension.
    if ((bankOp2 == PARAM) && (op2 > 255))
    {
        //  Substract the constant bank extension start offset.
        op2Aux = op2 - 256;
        
        //  Set the bank to the constant bank extension.
        op2BankAux = PARAM2;
    }
    else
    {
        op2Aux = op2;
        op2BankAux = bankOp2;
    }

    //  Check for the constant bank extension.
    if ((bankOp3 == PARAM) && (op3 > 255))
    {
        //  Substract the constant bank extension start offset.
        op3Aux = op3 - 256;
        
        //  Set the bank to the constant bank extension.
        op3BankAux = PARAM2;
    }
    else
    {
        op3Aux = op3;
        op3BankAux = bankOp3;
    }
    
    //  Predicate bank is not encoded.
    if (op1BankAux == PRED)
        op1BankAux = TEMP;

    //  Predicate bank is not encoded.
    if (op2BankAux == PRED)
        op2BankAux = TEMP;
    
    //  Encode operand banks.
    code.fields.lo64Bits.op1bank = op1BankAux & 0x07;
    code.fields.lo64Bits.op2bank = op2BankAux & 0x07;
    code.fields.lo64Bits.op3bank = op3BankAux & 0x07;

    //  First operand register.
    code.fields.hi64Bits.operands.op1reg = op1Aux & 0xff;

    //  Encode first operand swizzle mask.
    code.fields.hi64Bits.operands.op1swizzle = swizzleModeOp1 & 0xff;

    //  Encode result register.
    code.fields.hi64Bits.operands.resreg = res & 0x0ff;

    //  Check if the instruction is a jump.
    if (!hasImmediateB)
    {
        //  Encode second operand register.
        code.fields.hi64Bits.operands.op2reg = op2Aux & 0x0ff;

        //  Encode third operand register.
        code.fields.hi64Bits.operands.op3reg = op3Aux & 0x0ff;

        code.fields.hi64Bits.operands.op2swizzle = swizzleModeOp2 & 0xff;
        code.fields.hi64Bits.operands.op3swizzle = swizzleModeOp3 & 0xff;
    }
    else    
    {
        //  Encode instruction immediate.
        code.fields.hi64Bits.immediate.immediate = immediate;
    }
    
    setTag("ShInst");
}

//  Store and encode a shader instruction (instruction without parameters).
cgoShaderInstr::cgoShaderInstr(ShOpcode opc):     //  Instruction Opcode.

    opcode(opc), numOperands(0), hasResultB(false),
    bankOp1(INVALID), op1(0), negateFlagOp1(false), absoluteFlagOp1(false), swizzleModeOp1(XYZW),
    bankOp2(INVALID), op2(0), negateFlagOp2(false), absoluteFlagOp2(false), swizzleModeOp2(XYZW),
    bankOp3(INVALID), op3(0), negateFlagOp3(false), absoluteFlagOp3(false), swizzleModeOp3(XYZW),
    bankRes(INVALID), res(0), maskMode(mXYZW), saturatedRes(FALSE),
    predicatedFlag(false), negatePredFlag(false), predicateReg(0),
    relativeModeFlag(false),    
    relModeAddrReg(0), relModeAddrRegComp(0), relModeOffset(0),
    endFlag(false), waitPointFlag(false)

{
    // reset binary code
    memset(code.code8, 0, CG1_ISA_INSTR_SIZE);

    //  Encode the instruction.

    //  Encode instruction opcode.
    code.fields.lo64Bits.op1absolute = opcode & 0xff;

    //  Encode instruction wait point and end flags.
    code.fields.lo64Bits.waitpoint = waitPointFlag ? 1 : 0;
    code.fields.lo64Bits.endflag = (opcode == CG1_ISA_OPCODE_END) ? 1 : 0;

    //  Encode instruction predication parameters.
    code.fields.lo64Bits.predicated = predicatedFlag ? 1 : 0;
    code.fields.lo64Bits.invertpred = negatePredFlag ? 1 : 0;
    code.fields.lo64Bits.predreg = predicateReg & 0x1f;    
    
    //
    //  This section sets a number of derived values useful for helping decode the
    //  shader instruction.
    //

    //  Set if this instruction is an end instruction.
    isEndB = (opcode == CG1_ISA_OPCODE_END);

    //  Set if this instruction is an integer instruction.
    isIntegerB = (opcode == CG1_ISA_OPCODE_ARL) || (opcode == CG1_ISA_OPCODE_ADDI) || (opcode == CG1_ISA_OPCODE_MULI) || (opcode == CG1_ISA_OPCODE_STPEQI) || (opcode == CG1_ISA_OPCODE_STPGTI) || (opcode == CG1_ISA_OPCODE_STPLTI);

    //  Set if this instruction is a float instruction.
    isFloatB = !isIntegerB;

    //  Set if this instruction is a scalar instruction.
    isScalarB = setIsScalar(opcode, maskMode);

    //  Set if this instruction is compatible with a Scalar architecture.
    isSOACompatibleB = setIsSOACompatible(opcode, maskMode);

    //  Set if this instruction is a load instruction.
    isALoadB = false;

    //  Set if this instruction is a store instruction.
    isAStoreB = false;

    //  Set if this instruction is a Z export instruction.
    isZExportB = (opcode == CG1_ISA_OPCODE_ZXP) || (opcode == CG1_ISA_OPCODE_ZXS);
    
    //  Set if the instruction is a jump instruction.
    isAJumpB = false;
    
    //  Set if the instruction has an immediate value.
    hasImmediateB = false;
    
    //  Set default immediate value.
    immediate = 0;
    
    setTag("ShInst");
}

//  Write disassembled an instruction.
void cgoShaderInstr::disassemble(char* dis, int size)
{
    char buffer[20];

    //  Initialize disassembled instruction string.
    dis[0] = 0;
    
    //  Write predication if the instruction is predicated.
    if (predicatedFlag)
        sprintf_s(dis, size, "(%sp%d) ", (negatePredFlag ? "!" : ""), predicateReg);
    
    //  Write the instruction name.
    sprintf_s(dis, size, "%s%s", dis, getOpcodeString(opcode));

    //  Disassemble the instruction based on the specific instruction type/opcode.
    switch(opcode)
    {
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_CHS:
        
            //  Do nothing.  These instructions don't have parameters.
            break;
    
        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:
        
            //  CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP or CG1_ISA_OPCODE_ZXS instructions only have an operand an no results.

            //  Write wizzle mask to buffer.
            if ((swizzleModeOp1 == 0x00) || (swizzleModeOp1 == 0x55) ||
                (swizzleModeOp1 == 0xaa) || (swizzleModeOp1 == 0xff))
            {
                //  Broadcast a single component to all other components.
                sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp1 & 0x03]);
            }
            else if (swizzleModeOp1 == 0x1b)
            {
                //  The default swizzle mask mode isn't printed.
                sprintf_s(buffer, sizeof(buffer), "");
            }
            else
            {
                //  Build swizzle mask for each component.
                sprintf_s(buffer, sizeof(buffer), ".%c%c%c%c",
                swizzleMode2Str[(swizzleModeOp1 & 0xc0) >> 6],
                swizzleMode2Str[(swizzleModeOp1 & 0x30) >> 4],
                swizzleMode2Str[(swizzleModeOp1 & 0x0c) >> 2],
                swizzleMode2Str[swizzleModeOp1 & 0x03]);
            }

            //  Write the negate value modifier if required.
            if (negateFlagOp1)
            {
                sprintf_s(dis, size,"%s -", dis);
            }
            else
                sprintf_s(dis, size, "%s ", dis);

            //  Write the absolute value modifier if required.
            if (absoluteFlagOp1)
            {
                //  Write relative access to constant bank if required.
                if (relativeModeFlag)
                    sprintf_s(dis, size, "%s|c%d[a%d.%c + %d]%s|", dis, op1, relModeAddrReg,
                        swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                else
                    sprintf_s(dis, size, "%s|%s%d%s|", dis, getBankString(bankOp1), op1, buffer);
            }
            else
            {
                //  Write relative access to constant bank if required.
                if (relativeModeFlag)
                    sprintf_s(dis, size, "%s|c%d[a%d.%c + %d]%s|", dis, op1, relModeAddrReg,
                        swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                else
                    sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(bankOp1), op1, buffer);
            }

            //  Print sample identifier.
            if ((opcode == CG1_ISA_OPCODE_KLS) || (opcode == CG1_ISA_OPCODE_ZXS))
            {
                sprintf_s(dis, size,"%s, s%d", dis, op2);
            }
            
            break;
            
        case CG1_ISA_OPCODE_ANDP:
        
            //  The predicate operator instruction requires special processing.
            
            //  Negate result if saturate bit flag is set.
            if (saturatedRes)
                sprintf_s(dis, size, "%s !", dis);
            else
                sprintf_s(dis, size, "%s ", dis);
            
            //  Print result predicate register.            
            sprintf_s(dis, size, "%s%s%d", dis, getBankString(PRED), res);
            
            //  Print first operand.
            
            //  Check operand bank.
            if (bankOp1 == PARAM)
            {
                //  Only scalar swizzle allowed.
                if ((swizzleModeOp1 == 0x00) || (swizzleModeOp1 == 0x55) ||
                    (swizzleModeOp1 == 0xaa) || (swizzleModeOp1 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp1 & 0x03]);
                }
                else
                {
                    sprintf_s(buffer, sizeof(buffer), "");
                }

                //  Check if the operand is inverted (NOT) (aliased to negate flag).
                if (negateFlagOp1)
                    sprintf_s(dis, size, "%s, !", dis);
                else
                    sprintf_s(dis, size, "%s, ", dis);

                //  Write relative access to constant bank if required.
                if (relativeModeFlag)
                    sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(PARAM), op1, relModeAddrReg,
                        swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                else
                    sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(PARAM), op1, buffer);
            }
            else
            {
                //  Check if implicit value mode is used (aliased to the absolute flag).
                if (absoluteFlagOp1)
                {
                    //  Check the implicit value (true/false) (aliased to negate flag).
                    if (negateFlagOp1)                    
                        sprintf_s(dis, size, "%s, true", dis);
                    else
                        sprintf_s(dis, size, "%s, false", dis);
                }
                else
                {
                    //  Check if the operand is inverted (NOT) (aliased to negate flag).
                    if (negateFlagOp1)
                        sprintf_s(dis, size, "%s, !", dis);
                    else
                        sprintf_s(dis, size, "%s, ", dis);
                        
                    //  The operand is a predicate register.                   
                    sprintf_s(dis, size, "%s%s%d", dis, getBankString(PRED), op1);
                }
            }
            
            //  Print second operand.
            
            //  Check operand bank.
            if (bankOp2 == PARAM)
            {
                //  Only scalar swizzle allowed.
                if ((swizzleModeOp2 == 0x00) || (swizzleModeOp2 == 0x55) ||
                    (swizzleModeOp2 == 0xaa) || (swizzleModeOp2 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp2 & 0x03]);
                }
                else
                {
                    sprintf_s(buffer, sizeof(buffer), "");
                }

                //  Check if the operand is inverted (NOT) (aliased to negate flag).
                if (negateFlagOp2)
                    sprintf_s(dis, size, "%s, !", dis);
                else
                    sprintf_s(dis, size, "%s, ", dis);

                //  Write relative access to constant bank if required.
                if (relativeModeFlag)
                    sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(PARAM), op2, relModeAddrReg,
                        swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                else
                    sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(PARAM), op2, buffer);
            }
            else
            {
                //  Check if implicit value mode is used (aliased to the absolute flag).
                if (absoluteFlagOp2)
                {
                    //  Check the implicit value (true/false) (aliased to negate flag).
                    if (negateFlagOp2)                    
                        sprintf_s(dis, size, "%s, true", dis);
                    else
                        sprintf_s(dis, size, "%s, false", dis);
                }
                else
                {
                    //  Check if the operand is inverted (NOT) (aliased to negate flag).
                    if (negateFlagOp2)
                        sprintf_s(dis, size, "%s, !", dis);
                    else
                        sprintf_s(dis, size, "%s, ", dis);
                        
                    //  The operand is a predicate register.                   
                    sprintf_s(dis, size, "%s%s%d", dis, getBankString(PRED), op2);
                }
            }

            break;

        case CG1_ISA_OPCODE_JMP:
        
            //  The predicate operator instruction requires special processing.
            
            //  Print first operand.
            
            //  Check operand bank.
            if (bankOp1 == PARAM)
            {
                //  Only scalar swizzle allowed.
                if ((swizzleModeOp1 == 0x00) || (swizzleModeOp1 == 0x55) ||
                    (swizzleModeOp1 == 0xaa) || (swizzleModeOp1 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp1 & 0x03]);
                }
                else
                {
                    sprintf_s(buffer, sizeof(buffer), "");
                }

                //  Check if the operand is inverted (NOT) (aliased to negate flag).
                if (negateFlagOp1)
                    sprintf_s(dis, size, "%s !", dis);
                else
                    sprintf_s(dis, size, "%s ", dis);

                //  Write relative access to constant bank if required.
                if (relativeModeFlag)
                    sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(PARAM), op1, relModeAddrReg,
                        swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                else
                    sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(PARAM), op1, buffer);
            }
            else
            {
                //  Check if implicit value mode is used (aliased to the absolute flag).
                if (absoluteFlagOp1)
                {
                    //  Check the implicit value (true/false) (aliased to negate flag).
                    if (negateFlagOp1)                    
                        sprintf_s(dis, size, "%s true", dis);
                    else
                        sprintf_s(dis, size, "%s false", dis);
                }
                else
                {
                    //  Check if the operand is inverted (NOT) (aliased to negate flag).
                    if (negateFlagOp1)
                        sprintf_s(dis, size, "%s !", dis);
                    else
                        sprintf_s(dis, size, "%s ", dis);
                        
                    //  The operand is a predicate register.                   
                    sprintf_s(dis, size, "%s%s%d", dis, getBankString(PRED), op1);
                }
            }
            
            //  Print the offset.
            sprintf_s(dis, size, "%s, %d", dis, S32(immediate));
            
            break;
                        
        default:
        
            //
            //  All the other instructions follow the pattern:
            //
            //        op res, op1, op2, op3
            //

            //  Write saturated extension if saturated result is active.
            if (saturatedRes)
                sprintf_s(dis, size, "%s_sat", dis);

            //  Write the result register, mask and condition.
            if (bankRes != PRED)
                sprintf_s(dis, size, "%s %s%d%s", dis, getBankString(bankRes), res, maskMode2Str[maskMode]);
            else
                sprintf_s(dis, size, "%s %s%d", dis, getBankString(bankRes), res);

            //  Write the operands.

            //  First operand.
            if (numOperands > 0)
            {
                //  Write wizzle mask to buffer.
                if ((swizzleModeOp1 == 0x00) || (swizzleModeOp1 == 0x55) ||
                    (swizzleModeOp1 == 0xaa) || (swizzleModeOp1 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp1 & 0x03]);
                }
                else if (swizzleModeOp1 == 0x1b)
                {
                    //  The default swizzle mask mode isn't printed.
                    sprintf_s(buffer, sizeof(buffer), "");
                }
                else
                {
                    //  Build swizzle mask for each component.
                    sprintf_s(buffer, sizeof(buffer), ".%c%c%c%c",
                    swizzleMode2Str[(swizzleModeOp1 & 0xc0) >> 6],
                    swizzleMode2Str[(swizzleModeOp1 & 0x30) >> 4],
                    swizzleMode2Str[(swizzleModeOp1 & 0x0c) >> 2],
                    swizzleMode2Str[swizzleModeOp1 & 0x03]);
                }

                //  Write the negate value modifier if required.
                if (negateFlagOp1)
                {
                    sprintf_s(dis, size,"%s, -", dis);
                }
                else
                    sprintf_s(dis, size, "%s, ", dis);

                //  Write the absolute value modifier if required.
                if (absoluteFlagOp1)
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp1 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s|%s%d[a%d.%c + %d]%s|", dis, getBankString(bankOp1), op1, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else
                        sprintf_s(dis, size, "%s|%s%d%s|", dis, getBankString(bankOp1), op1, buffer);
                }
                else
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp1 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(bankOp1), op1, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else
                        sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(bankOp1), op1, buffer);
                }
            }

            //  Second Operand.
            if (numOperands > 1)
            {
                //  Write wizzle mask to buffer.
                if ((swizzleModeOp2 == 0x00) || (swizzleModeOp2 == 0x55) ||
                    (swizzleModeOp2 == 0xaa) || (swizzleModeOp2 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp2 & 0x03]);
                }
                else if (swizzleModeOp2 == 0x1b)
                {
                    //  The default swizzle mask mode isn't printed.
                    sprintf_s(buffer, sizeof(buffer), "");
                }
                else
                {
                    //  Build swizzle mask for each component.
                    sprintf_s(buffer, sizeof(buffer), ".%c%c%c%c",
                    swizzleMode2Str[(swizzleModeOp2 & 0xc0) >> 6],
                    swizzleMode2Str[(swizzleModeOp2 & 0x30) >> 4],
                    swizzleMode2Str[(swizzleModeOp2 & 0x0c) >> 2],
                    swizzleMode2Str[swizzleModeOp2 & 0x03]);
                }

                //  Write the negate value modifier if required.
                if (negateFlagOp2)
                {
                    sprintf_s(dis, size,"%s, -", dis);
                }
                else
                    sprintf_s(dis, size, "%s, ", dis);

                //  Write the absolute value modifier if required.
                if (absoluteFlagOp2)
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp2 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s|%s%d[a%d.%c + %d]%s|", dis, getBankString(bankOp2), op2, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else if (bankOp2 == IMM)
                    {
                        //  Check the type of immediate.
                        if (isIntegerB)
                            sprintf_s(dis, size, "%s|%d|", dis, *((S32 *) &immediate));
                        else
                            sprintf_s(dis, size, "%s|%f|", dis, *((F32 *) &immediate));
                    }
                    else
                        sprintf_s(dis, size, "%s|%s%d%s|", dis, getBankString(bankOp2), op2, buffer);
                }
                else
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp2 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(bankOp2), op2, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else if (bankOp2 == IMM)
                    {
                        //  Check the type of the immediate.
                        if (isIntegerB)
                            sprintf_s(dis, size, "%s%d", dis, *((S32 *) &immediate));
                        else
                            sprintf_s(dis, size, "%s%f", dis, *((F32 *) &immediate));
                    }
                    else
                        sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(bankOp2), op2, buffer);
                }
            }

            //  Third operand.
            if (numOperands > 2)
            {
                //  Write wizzle mask to buffer.
                if ((swizzleModeOp3 == 0x00) || (swizzleModeOp3 == 0x55) ||
                    (swizzleModeOp3 == 0xaa) || (swizzleModeOp3 == 0xff))
                {
                    //  Broadcast a single component to all other components.
                    sprintf_s(buffer, sizeof(buffer), ".%c", swizzleMode2Str[swizzleModeOp3 & 0x03]);
                }
                else if (swizzleModeOp3 == 0x1b)
                {
                    //  The default swizzle mask mode isn't printed.
                    sprintf_s(buffer, sizeof(buffer), "");
                }
                else
                {
                    //  Build swizzle mask for each component.
                    sprintf_s(buffer, sizeof(buffer), ".%c%c%c%c",
                    swizzleMode2Str[(swizzleModeOp3 & 0xc0) >> 6],
                    swizzleMode2Str[(swizzleModeOp3 & 0x30) >> 4],
                    swizzleMode2Str[(swizzleModeOp3 & 0x0c) >> 2],
                    swizzleMode2Str[swizzleModeOp3 & 0x03]);
                }

                //  Write the negate value modifier if required.
                if (negateFlagOp3)
                {
                    sprintf_s(dis, size,"%s, -", dis);
                }
                else
                    sprintf_s(dis, size, "%s, ", dis);

                //  Write the absolute value modifier if required.
                if (absoluteFlagOp3)
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp3 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s|%s%d[a%d.%c + %d]%s|", dis, getBankString(bankOp3), op3, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else
                        sprintf_s(dis, size, "%s|%s%d%s|", dis, getBankString(bankOp3), op3, buffer);
                }
                else
                {
                    //  Write relative access to constant bank if required.
                    if ((bankOp3 == PARAM) && relativeModeFlag)
                        sprintf_s(dis, size, "%s%s%d[a%d.%c + %d]%s", dis, getBankString(bankOp3), op3, relModeAddrReg,
                            swizzleMode2Str[relModeAddrRegComp], relModeOffset, buffer);
                    else
                        sprintf_s(dis, size, "%s%s%d%s", dis, getBankString(bankOp3), op3, buffer);
                }
            }
            
            break;
    }

    //  Write end instruction mark if required.
    if (endFlag)
        sprintf_s(dis, size, "%s <- end", dis);

    //  Write wait point mark if required.
    if (waitPointFlag)
        sprintf_s(dis, size, "%s <- wait", dis);

}

#define SKIP_BLANK_CHARS for(; (i < length) && ((line[i] ==' ') || (line[i] == '\t') || (line[i] == '\n')); i++);

#define CHECK_PARSING_ERROR(cond, str) if (cond) {errorString = (str); return NULL;}


//  Creates a shader instruction from an assembly text line.
cgoShaderInstr *cgoShaderInstr::assemble(char *line, string &errorString)
{

    U32 i, j, length;
    char opcodeStr[CG1_ISA_OPCODE_LENGTH_MAX + 1];
    char addrStr[12];
    char maskStr[4];
    cgoShaderInstr *shInstr;
    U32 numOps;

    //  Shader Instruction parameters.
    ShOpcode opc;
    Bank op1B, op2B, op3B, resB;
    U32 op1R, op2R, op3R, resR;
    bool op1N, op2N, op3N, op1A, op2A, op3A;
    bool satRes;
    SwizzleMode op1M, op2M, op3M;
    MaskMode resM;
    bool predicated, negPredicate;
    U32 predicateReg;
    bool relMode;
    U32 relModeReg;
    U08 relModeComp;
    S16 relModeOff;

    //  Get assembly line length.
    length = (U32) strlen(line);

    //  Set parse pointer to the start of the line.
    i = 0;

    //  Remove spaces and tabs from the start of the line.
    SKIP_BLANK_CHARS

    //  Empty line.
    CHECK_PARSING_ERROR(i == length, "empty line")

    //  Check if the instruction is predicated.
    if (line[i] == '(')
    {
        predicated = true;
        
        //  Advance to he next character.
        i++;
        
        //  Check end of line.
        CHECK_PARSING_ERROR(i == length, "unexpected end of line")

        //  Check if the predicate register is negated.
        if (line[i] == '!')
        {
            negPredicate = true;
            
            //  Advance to the next character.
            i++;
            
            //  Check end of line.
            CHECK_PARSING_ERROR(i == length, "unexpected end of line")
        }
        else
        {
            negPredicate = false;            
        }
        
        //  Parse predicate register.
        
        //  Check predicate register prefix.
        CHECK_PARSING_ERROR(line[i] != 'p', "predicated register expected")
        
        //  Advance to the next character.
        i++;
        
        //  Check end of line.
        CHECK_PARSING_ERROR(i == length, "unexpected end of line")
        
        //  Parse the predicate register identifier.
        
        //  Get register number.  Two digits only.  Copy and convert to int with atoi.
        for(j = 0; (j < 3) && (i < length) && (line[i] != ')'); i++, j++)
            addrStr[j] = line[i];

        //  Address was too large.
        CHECK_PARSING_ERROR(j == 3, "predicate register identifier out of range")

        //  No register identifier found.
        CHECK_PARSING_ERROR(j == 0, "predicate register identifier not found")
        
        //  Mark end of string.
        addrStr[j] = '\0';

        //  Convert result register identifier to register number.
        predicateReg = atoi(addrStr);
        
        //  Skip closing predication closing character ')'
        i++;
        
        //  Remove spaces and tabs from the start of the line.
        SKIP_BLANK_CHARS

        //  Check end of line.
        CHECK_PARSING_ERROR(i == length, "unexpected end of line")
    }
    else
    {
        predicated = false;
        negPredicate = false;
        predicateReg = 0;
    }
    
    //  Get the opcode.
    for(j = 0; (i < length) && (j < CG1_ISA_OPCODE_LENGTH_MAX) && ((line[i] !=' ') && (line[i] != '_') && (line[i] != '\t') && (line[i] != '\n')); i++, j++)
        opcodeStr[j] = line[i];

    //  End of string mark.
    opcodeStr[j] = '\0';

    //  Compare with the opcode string table.
    for(j = 0; (j < LASTOPC) && (strcmp(opcodeStr, shOpcode2Str[j])) ; j++);

    //  Opcode not found.
    CHECK_PARSING_ERROR(j == LASTOPC, "unknown instruction name")

    //  Invalid opcode.
    CHECK_PARSING_ERROR((strcmp(opcodeStr, "inv") == 0), "invalid opcode")
    
    //  Gets the instruction opcode.
    opc = translateShOpcodeTable[j];

    //  Set if this instruction is an integer instruction.
    bool integerOp = (opc == CG1_ISA_OPCODE_ARL) || (opc == CG1_ISA_OPCODE_ADDI) || (opc == CG1_ISA_OPCODE_MULI) || (opc == CG1_ISA_OPCODE_STPEQI) || (opc == CG1_ISA_OPCODE_STPGTI) || (opc == CG1_ISA_OPCODE_STPLTI);

    bool parseOK;
    
    //  Continue assembling based on the opcode.
    switch(opc)
    {
    
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_CHS:
        
            //  Instruction without arguments/parameters.

            //  Check until the end of the line.
            SKIP_BLANK_CHARS

            //  Garbage found?
            CHECK_PARSING_ERROR(i != length, "characters left at the end of the line")

            //  Create instruction.
            shInstr = new cgoShaderInstr(opc);

            //  Return the assembled instruction.
            return shInstr;
            
            break;

        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:
        
            //  The CG1_ISA_OPCODE_KIL, CG1_ISA_OPCODE_KLS, CG1_ISA_OPCODE_ZXP and CG1_ISA_OPCODE_ZXS instructions have a single operand an no result register.

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing first operand")

            //  Parse first operand.
            parseOK = parseOperand(line, i, length, op1R, op1B, op1N, op1A, op1M, false, relMode, relModeReg, relModeComp, relModeOff);

            CHECK_PARSING_ERROR(!parseOK, "error parsing first operand")

            //  Check until the end of the line for non blank characters.
            SKIP_BLANK_CHARS

            //  Garbage found?
            CHECK_PARSING_ERROR(i != length, "unexpected characters at the end of line")
            
            //  Default values for unused fields.
            op2B = op3B = INVALID;
            op2R = op3R = 0;
            op2N = op3N = op2A = op3A = false;
            op2M = op3M = XYZW;
            resB = INVALID;
            resR = 0;
            satRes = false;
            resM = mXYZW;

            //  Create the shader instruction.
            shInstr = new cgoShaderInstr(opc,
                op1B, op1R, op1N, op1A, op1M,
                op2B, op2R, op2N, op2A, op2M,
                op3B, op3R, op3N, op3A, op3M,
                resB, resR, satRes, resM,
                predicated, negPredicate, predicateReg,
                relMode, relModeReg, relModeComp, relModeOff);

            //  Return the created instruction.
            return shInstr;
            break;

        case CG1_ISA_OPCODE_ANDP:
            
            //  The predicate operator instruction is special.
            
            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing result")
        
            //  Check for negate result (aliased to saturate flag)
            if (line[i] == '!')
            {
                //  Result is negated.
                satRes = true;
                
                //  Skip character.
                i++;
            }
            else
            {
                //  Result is not negated.
                satRes = false;
            }
            
            //  Check that the output register is a predicate register.
            CHECK_PARSING_ERROR(line[i] != 'p', "expected predicate register")
            
            //  Skip predicate bank identifier.
            i++;
            
            //  Get register number.  Three digits only.  Copy and convert to int with atoi.
            for(j = 0; (j < 4) && (i < length) &&
                !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                (line[i] == '.') || (line[i] == '(') || (line[i] == ','));
                i++, j++)
                addrStr[j] = line[i];

            //  Address was too large.
            CHECK_PARSING_ERROR(j == 4, "result register identifier out of range")

            //  No register identifier found.
            CHECK_PARSING_ERROR(j == 0, "result register identifier not found")

            //  Mark end of string.
            addrStr[j] = '\0';

            //  Result bank is always predicate.
            resB = PRED;
            
            //  Convert result register identifier to register number.
            resR = atoi(addrStr);

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing first operand")

            //  Check ','.
            CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

            //  Skip ','.
            i++;

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing first operand")

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing first operand")

            //  Parse first operand.
            
            //  No implicit operand (aliased to absolute flag).
            op1A = false;

            //  Check for implicit operand 'true'.
            if ((length - i) >= 4)
            {
                if ((line[i] == 't') && (line[i + 1] == 'r') && (line[i + 2] == 'u') && (line[i + 3] == 'e'))                
                {
                    //  Implicit operand true.
                    op1A = true;
                    op1N = true;
                    
                    //  Set default values for unused fields.
                    op1B = PRED;
                    op1M = XXXX;
                    
                    //  Skip 'true'.
                    i += 4;
                }
            }
            
            //  Check for implicit operand 'false'
            if (!op1A && ((length - i) >= 5))
            {
                if ((line[i] == 'f') && (line[i + 1] == 'a') && (line[i + 2] == 'l') && (line[i + 3] == 's') && (line[i + 4] == 'e'))
                {
                    //  Implicit operand true.
                    op1A = true;
                    op1N = false;
                    
                    //  Set default values for unused fields.
                    op1B = PRED;
                    op1M = XXXX;
                    
                    //  Skip 'false'.
                    i += 5;
                }
            }

            //  Check if the operand was implicit.
            if (!op1A)
            {
                //  Check if first operand is negated.
                if (line[i] == '!')
                {
                    //  Set first operand as negated.
                    op1N = true;
                    
                    //  Skip '!'.
                    i++;
                }
                else
                    op1N = false;

                //  Get first operand bank.  Only predicate or constant bank.
                if (line[i] == 'p')
                    op1B = PRED;
                else if (line[i] == 'c')
                    op1B = PARAM;
                else    
                {
                    //  Invalid operand bank.
                    CHECK_PARSING_ERROR(true, "invalid operand bank for predicate first operator")
                }
                
                //  Skip operand bank identifier.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                char regStr[5];
                
                //  Get register number.  Three digits only.  Copy and convert to int with atoi.
                for(j = 0; (j < 4) && (i < length) &&
                    !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                    (line[i] =='.') || (line[i] == '[') || (line[i] == ',') || (line[i] == '|'));
                    i++, j++)
                    regStr[j] = line[i];

                //  Register identifier out of range.
                CHECK_PARSING_ERROR((j == 4), "register identifier out of range")

                //  Register identifier not specified.
                CHECK_PARSING_ERROR((j == 0), "register identifier not specified")
                    
                //  Mark end of string.
                regStr[j] = '\0';

                //  Convert to int.
                op1R = atoi(regStr);

                //  Constant bank can be accessed using two addressing modes.
                if ((op1B == PARAM) && (line[i] == '['))
                {
                    //  Relative mode addressing used in the instruction.
                    relMode = true;

                    //  Parse relative mode access to constant bank.
                    if (!parseRelAddr(line, i, length, relModeReg, relModeComp, relModeOff))
                    {
                        CHECK_PARSING_ERROR(true, "error parsing relative adddressing to constant bank")
                    }
                }

                //  Get operand swizzle mode.
                if (line[i] == '.')
                {
                    //  Only constant operand supports swizzling.
                    CHECK_PARSING_ERROR((op1B != PARAM), "swizzling only supported for constant operands with predicate operator")

                    //  Skip '.'.
                    i++;

                    //  Mising instruction parameters.
                    CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                    //  Parse swizzle mode.  Only single component swizzle allowed.
                    if (line[i] == 'x')
                        op1M = XXXX;
                    else if (line[i] == 'y')
                        op1M = YYYY;
                    else if (line[i] == 'z')
                        op1M = ZZZZ;
                    else if (line[i] == 'w')
                        op1M = WWWW;
                    else
                    {
                        CHECK_PARSING_ERROR(true, "unexpected swizzle component selector")
                    }
                    
                    //  Skip swizzle component selector.
                    i++;
                }
                else
                {
                    op1M = XXXX;
                }
            }

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing second operand")

            //  Check ','.
            CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

            //  Skip ','.
            i++;

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing second operand")

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing second operand")

            //  Parse second operand.
            
            //  No implicit operand (aliased to absolute flag).
            op2A = false;

            //  Check for implicit operand 'true'.
            if ((length - i) >= 4)
            {
                if ((line[i] == 't') && (line[i + 1] == 'r') && (line[i + 2] == 'u') && (line[i + 3] == 'e'))                
                {
                    //  Implicit operand true.
                    op2A = true;
                    op2N = true;
                    
                    //  Set default values for unused fields.
                    op2B = PRED;
                    op2M = XXXX;
                    
                    //  Skip 'true'.
                    i += 4;
                }
            }
            
            //  Check for implicit operand 'false'.
            if (!op2A && ((length - i) >= 5))
            {
                if ((line[i] == 'f') && (line[i + 1] == 'a') && (line[i + 2] == 'l') && (line[i + 3] == 's') && (line[i + 4] == 'e'))
                {
                    //  Implicit operand true.
                    op2A = true;
                    op2N = false;
                    
                    //  Set default values for unused fields.
                    op2B = PRED;
                    op2M = XXXX;
                    
                    //  Skip 'false'.
                    i += 5;
                }
            }
            
            //  Check if the operand was implicit.
            if (!op2A)
            {
                //  Check if first operand is negated.
                if (line[i] == '!')
                {
                    //  Set first operand as negated.
                    op2N = true;
                    
                    //  Skip '!'.
                    i++;
                }
                else
                    op2N = false;

                //  Get first operand bank.  Only predicate or constant bank.
                if (line[i] == 'p')
                    op2B = PRED;
                else if (line[i] == 'c')
                    op2B = PARAM;
                else    
                {
                    //  Invalid operand bank.
                    CHECK_PARSING_ERROR(true, "invalid operand bank for predicate second operator")
                }
                
                //  Skip operand bank identifier.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                char regStr[5];
                
                //  Get register number.  Three digits only.  Copy and convert to int with atoi.
                for(j = 0; (j < 4) && (i < length) &&
                    !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                    (line[i] =='.') || (line[i] == '[') || (line[i] == ',') || (line[i] == '|'));
                    i++, j++)
                    regStr[j] = line[i];

                //  Register identifier out of range.
                CHECK_PARSING_ERROR((j == 4), "register identifier out of range")

                //  Register identifier not specified.
                CHECK_PARSING_ERROR((j == 0), "register identifier not specified")
                    
                //  Mark end of string.
                regStr[j] = '\0';

                //  Convert to int.
                op2R = atoi(regStr);

                //  Constant bank can be accessed using two addressing modes.
                if ((op2B == PARAM) && (line[i] == '['))
                {
                    //  Check if relative addressing was used for the first operand.
                    CHECK_PARSING_ERROR(relMode, "relative addressing already used for first operand")
                    
                    //  Relative mode addressing used in the instruction.
                    relMode = true;

                    //  Parse relative mode access to constant bank.
                    if (!parseRelAddr(line, i, length, relModeReg, relModeComp, relModeOff))
                    {
                        CHECK_PARSING_ERROR(true, "error parsing relative adddressing to constant bank")
                    }
                }

                //  Get operand swizzle mode.
                if (line[i] == '.')
                {
                    //  Only constant operand supports swizzling.
                    CHECK_PARSING_ERROR((op2B != PARAM), "swizzling only supported for constant operands with predicate operator")

                    //  Skip '.'.
                    i++;

                    //  Mising instruction parameters.
                    CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                    //  Parse swizzle mode.  Only single component swizzle allowed.
                    if (line[i] == 'x')
                        op2M = XXXX;
                    else if (line[i] == 'y')
                        op2M = YYYY;
                    else if (line[i] == 'z')
                        op2M = ZZZZ;
                    else if (line[i] == 'w')
                        op2M = WWWW;
                    else
                    {
                        CHECK_PARSING_ERROR(true, "unexpected swizzle component selector")
                    }
                    
                    //  Skip swizzle component selector.
                    i++;
                }
                else
                {
                    op2M = XXXX;
                }
            }
            
            //  Check until the end of the line for non blank characters.
            SKIP_BLANK_CHARS

            //  Garbage found?
            CHECK_PARSING_ERROR(i != length, "unexpected characters at the end of line")
            
            //  Default values for unused fields.
            op3B = INVALID;
            op3R = 0;
            op3N = op3A = false;
            op3M = XYZW;
            resM = mXYZW;

            //  Create the shader instruction.
            shInstr = new cgoShaderInstr(opc,
                op1B, op1R, op1N, op1A, op1M,
                op2B, op2R, op2N, op2A, op2M,
                op3B, op3R, op3N, op3A, op3M,
                resB, resR, satRes, resM,
                predicated, negPredicate, predicateReg,
                relMode, relModeReg, relModeComp, relModeOff);

            //  Return the created instruction.
            return shInstr;

            break;
            
        case CG1_ISA_OPCODE_JMP:
        
            //  The jump instruction is special.
            
            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing first operand")
        
            //  Parse first operand.
            
            //  No implicit operand (aliased to absolute flag).
            op1A = false;

            //  Check for implicit operand 'true'.
            if ((length - i) >= 4)
            {
                if ((line[i] == 't') && (line[i + 1] == 'r') && (line[i + 2] == 'u') && (line[i + 3] == 'e'))                
                {
                    //  Implicit operand true.
                    op1A = true;
                    op1N = true;
                    
                    //  Set default values for unused fields.
                    op1B = PRED;
                    op1M = XXXX;
                    
                    //  Skip 'true'.
                    i += 4;
                }
            }
            
            //  Check for implicit operand 'false'
            if (!op1A && ((length - i) >= 5))
            {
                if ((line[i] == 'f') && (line[i + 1] == 'a') && (line[i + 2] == 'l') && (line[i + 3] == 's') && (line[i + 4] == 'e'))
                {
                    //  Implicit operand true.
                    op1A = true;
                    op1N = false;
                    
                    //  Set default values for unused fields.
                    op1B = PRED;
                    op1M = XXXX;
                    
                    //  Skip 'false'.
                    i += 5;
                }
            }

            //  Check if the operand was implicit.
            if (!op1A)
            {
                //  Check if first operand is negated.
                if (line[i] == '!')
                {
                    //  Set first operand as negated.
                    op1N = true;
                    
                    //  Skip '!'.
                    i++;
                }
                else
                    op1N = false;

                //  Get first operand bank.  Only predicate or constant bank.
                if (line[i] == 'p')
                    op1B = PRED;
                else if (line[i] == 'c')
                    op1B = PARAM;
                else    
                {
                    //  Invalid operand bank.
                    CHECK_PARSING_ERROR(true, "invalid operand bank for predicate first operator")
                }
                
                //  Skip operand bank identifier.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                char regStr[5];
                
                //  Get register number.  Three digits only.  Copy and convert to int with atoi.
                for(j = 0; (j < 4) && (i < length) &&
                    !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                    (line[i] =='.') || (line[i] == '[') || (line[i] == ',') || (line[i] == '|'));
                    i++, j++)
                    regStr[j] = line[i];

                //  Register identifier out of range.
                CHECK_PARSING_ERROR((j == 4), "register identifier out of range")

                //  Register identifier not specified.
                CHECK_PARSING_ERROR((j == 0), "register identifier not specified")
                    
                //  Mark end of string.
                regStr[j] = '\0';

                //  Convert to int.
                op1R = atoi(regStr);

                //  Constant bank can be accessed using two addressing modes.
                if ((op1B == PARAM) && (line[i] == '['))
                {
                    //  Relative mode addressing used in the instruction.
                    relMode = true;

                    //  Parse relative mode access to constant bank.
                    if (!parseRelAddr(line, i, length, relModeReg, relModeComp, relModeOff))
                    {
                        CHECK_PARSING_ERROR(true, "error parsing relative adddressing to constant bank")
                    }
                }

                //  Get operand swizzle mode.
                if (line[i] == '.')
                {
                    //  Only constant operand supports swizzling.
                    CHECK_PARSING_ERROR((op1B != PARAM), "swizzling only supported for constant operands with predicate operator")

                    //  Skip '.'.
                    i++;

                    //  Mising instruction parameters.
                    CHECK_PARSING_ERROR((i == length), "missing operand parameters")

                    //  Parse swizzle mode.  Only single component swizzle allowed.
                    if (line[i] == 'x')
                        op1M = XXXX;
                    else if (line[i] == 'y')
                        op1M = YYYY;
                    else if (line[i] == 'z')
                        op1M = ZZZZ;
                    else if (line[i] == 'w')
                        op1M = WWWW;
                    else
                    {
                        CHECK_PARSING_ERROR(true, "unexpected swizzle component selector")
                    }
                    
                    //  Skip swizzle component selector.
                    i++;
                }
                else
                {
                    op1M = XXXX;
                }
            }

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing jump offset")

            //  Check ','.
            CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

            //  Skip ','.
            i++;

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing jump offset")

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing jump offset")

            //  Check if the offset is negative.
            if (line[i] == '-')
            {
                //  Set the jump offset as negative.
                *((S32 *) &op2R) = -1;
                
                //  Skip '-'.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing jump offset")
                
                //  Skip spaces and tabs.
                SKIP_BLANK_CHARS

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing jump offset")
            }
            else
            {
                //  Set the jump offset as positive.
                op2R = 1;
            }
            
            //  Get register number.  Three digits only.  Copy and convert to int with atoi.
            for(j = 0; (j < 11) && (i < length) &&
                !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n'));
                i++, j++)
                addrStr[j] = line[i];

            //  Address was too large.
            CHECK_PARSING_ERROR(j == 11, "result register identifier out of range")

            //  No register identifier found.
            CHECK_PARSING_ERROR(j == 0, "result register identifier not found")

            //  Mark end of string.
            addrStr[j] = '\0';

            // Set the jump offset (aliased to second operand register identifier).
            *((S32 *) &op2R) = *((S32 *) &op2R) * atoi(addrStr);

            //  Check until the end of the line for non blank characters.
            SKIP_BLANK_CHARS

            //  Garbage found?
            CHECK_PARSING_ERROR(i != length, "unexpected characters at the end of line")
            
            //  Default values for unused fields.
            op2B = op3B = resB = INVALID;
            op3R = resR = 0;
            op2N = op2A = op3N = op3A = false;
            op2M = op3M = XYZW;
            resM = mXYZW;
            satRes = false;

            //  Create the shader instruction.
            shInstr = new cgoShaderInstr(opc,
                op1B, op1R, op1N, op1A, op1M,
                op2B, op2R, op2N, op2A, op2M,
                op3B, op3R, op3N, op3A, op3M,
                resB, resR, satRes, resM,
                predicated, negPredicate, predicateReg,
                relMode, relModeReg, relModeComp, relModeOff);

            //  Return the created instruction.
            return shInstr;

            break;
            
        default:            
            //  Math instructions with 1 to 3 operands.

            //  Check saturated result mark.
            if (((i + 3) < length) &&
                (((line[i] == '_') && (line[i + 1] == 's') && (line[i + 2] == 'a') && (line[i + 3] == 't')) ||
                 (line[i] == '_') && (line[i + 1] == 'S') && (line[i + 2] == 'A') && (line[i + 3] == 'T')))
            {
                //  Set saturated result flag to true.
                satRes = true;

                //  Skip saturated result mark.
                i = i + 4;
            }
            else
            {
                //  Set saturated result flag to false.
                satRes = false;
            }

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length,  "missing instruction parameters.")

            //  Set the number of operands for this opcode.
            numOps = setNumOperands(opc);

            //  Get result parameters.

            //  Get result bank.  Only writeable banks.
            if (line[i] == 'o')
                resB = OUT;
            else if (line[i] == 'r')
                resB = TEMP;
            else if (line[i] == 'a')
                resB = ADDR;
            else if (line[i] == 'p')
                resB = PRED;
            else    //  Invalid result register bank.
            {
                errorString = "unknown register type";
                return NULL;
            }

            //  Skip bank identifier.
            i++;

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing instruction parameters")

            //  Get register number.  Three digits only.  Copy and convert to int with atoi.
            for(j = 0; (j < 4) && (i < length) &&
                !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                (line[i] == '.') || (line[i] == '(') || (line[i] == ','));
                i++, j++)
                addrStr[j] = line[i];

            //  Address was too large.
            CHECK_PARSING_ERROR(j == 4, "result register identifier out of range")

            //  No register identifier found.
            CHECK_PARSING_ERROR(j == 0, "result register identifier not found")

            //  Mark end of string.
            addrStr[j] = '\0';

            //  Convert result register identifier to register number.
            resR = atoi(addrStr);

            //  Get Result mask mode.
            if (line[i] == '.')
            {        
                //  Parse result register mask mode.

                CHECK_PARSING_ERROR(resB == PRED, "result mask not supported for predicate registers")
                
                //  Copy mask.  Max 4 characters.
                for(j = 0; (j < 5) && (i < length) &&
                    !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
                    (line[i] == ',') || (line[i] == '(')); i++, j++)
                    maskStr[j] = line[i];

                //  Check number of characters in the mask.
                CHECK_PARSING_ERROR(j == 5, "too many characters in write mask")

                //  Mark end of string.
                maskStr[j] = '\0';

                //  Search in mask table and translate to MaskMode.
                for(j = 0; (j < 16) && strcmp(maskStr, maskMode2Str[j]); j++);

                //  Incorrect mask mode.
                CHECK_PARSING_ERROR(j == 16, "unknown write mask mode")

                //  Translate to MaskMode.
                resM = translateMaskModeTable[j];
            }
            else
            {
                //  Check for predicate result register.
                if (resB != PRED)
                {
                    //  Default mask mode (SIMD4) for normal result registers.
                    resM = mXYZW;
                }
                else
                {
                    //  Default mask mode (scalar) for predicate result registers.
                    resM = XNNN;
                }
            }

            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Missing instruction parameters.
            CHECK_PARSING_ERROR(i == length, "missing instruction parameters")

            //  Default value, constant bank relative mode addressing not used.
            relMode = false;

            //  Get first operand parameters.
            if (numOps > 0)
            {
                //  Skip spaces and tabs.
                SKIP_BLANK_CHARS

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing first operand")

                //  Check ','.
                CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

                //  Skip ','.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing first operand")

                //  Parse first operand.
                bool parseOK = parseOperand(line, i, length, op1R, op1B, op1N, op1A, op1M, true, relMode, relModeReg, relModeComp, relModeOff);

                CHECK_PARSING_ERROR(!parseOK, "error parsing first operand")
            }
            else
            {
                //  Default values for unused fields.
                op1B = op2B = op3B = INVALID;
                op1R = op2R = op3R = INVALID;
                op1N = op2N = op3N = op1A = op2A = op3A = false;
                op1M = op2M = op3M = XYZW;
            }

            //  Instruction with 2+ operands.
            if (numOps > 1)
            {
                //  Skip spaces and tabs.
                SKIP_BLANK_CHARS

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing second operand")

                //  Check ','.
                CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

                //  Skip ','.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing second operand")

                //  Parse second operand.
                bool parseOK = parseOperand(line, i, length, op2R, op2B, op2N, op2A, op2M, integerOp, relMode, relModeReg, relModeComp, relModeOff);

                CHECK_PARSING_ERROR(!parseOK, "error parsing second operand")
            }
            else
            {
                //  Default values for unused fields.
                op2B = op3B = INVALID;
                op2R = op3R = INVALID;
                op2N = op3N = op2A = op3A = false;
                op2M = op3M = XYZW;
            }

            //  Three operand instruction.
            if (numOps > 2)
            {
                //  Skip spaces and tabs.
                SKIP_BLANK_CHARS

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing third operand")

                //  Check ','.
                CHECK_PARSING_ERROR((line[i] != ','), "missing \',\' before operand")

                //  Skip ','.
                i++;

                //  Missing instruction parameters.
                CHECK_PARSING_ERROR(i == length, "missing first operand")

                //  Parse third operand.
                bool parseOK = parseOperand(line, i, length, op3R, op3B, op3N, op3A, op3M, true, relMode, relModeReg, relModeComp, relModeOff);

                CHECK_PARSING_ERROR(!parseOK, "error parsing third operand")
            }
            else
            {
                //  Default values for unused fields.
                op3B = INVALID;
                op3R = INVALID;
                op3N = op3A = false;
                op3M = XYZW;
            }

            //  Check until the end of the line for non blank characters.
            SKIP_BLANK_CHARS

            //  Garbage found?
            CHECK_PARSING_ERROR(i != length, "missing instruction parameters")

            //  Create the shader instruction.
            shInstr = new cgoShaderInstr(opc,
                op1B, op1R, op1N, op1A, op1M,
                op2B, op2R, op2N, op2A, op2M,
                op3B, op3R, op3N, op3A, op3M,
                resB, resR, satRes, resM,
                predicated, negPredicate, predicateReg,
                relMode, relModeReg, relModeComp, relModeOff);

            //  Return the created instruction.
            return shInstr;
            break;
    }
}

//  Parses a substring for relative (or indexed for bra/cal) addressing mode.
bool cgoShaderInstr::parseRelAddr(char *line, U32 &i, U32 length,
    U32 &relModeReg, U08 &relModeComp, S16 &relModeOff)
{
    U32 j;
    char addrStr[10];

    //  Absolute/Relative mode or indexed mode?
    if (line[i] =='[')
    {
        //  Skip '['
        i++;

        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Check address bank used.
        if (line[i] != 'a')
            return false;

        //  Skip address bank identifier.
        i++;

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Get address register number.  Only 1 digit allowed.
        if ((line[i] >= '0') && (line[i] <= '9'))
        {
            relModeReg = line[i] - '0';
        }
        else    //  Not a decimal digit.
            return false;

        //  Skip register number.
        i++;

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Check address register component separator '.'.
        if (line[i] != '.')
            return false;

        //  Skip '.' number.
        i++;

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Get address register component.  Allowed chars: 'xyzw'.
        if (line[i] == 'x')
            relModeComp = 0;
        else if (line[i] == 'y')
            relModeComp = 1;
        else if (line[i] == 'z')
            relModeComp = 2;
        else if (line[i] == 'w')
            relModeComp = 3;
        else    //  Invalid register component identifier.
            return false;

        //  Skip register component identifier.
        i++;

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        bool parseOffset = false;
        
        //  Check for positive offset.
        if (line[i] == '+')
        {
            //  Parse offset.
            parseOffset = true;
            
            //  Offset positive.
            relModeOff = 1;
            
            //  Skip '+'.
            i++;
            
            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Check end of line.  Missing instruction parameters.
            if (i == length)
                return false;
        }
        
        //  Check for negative offset.
        if (line[i] == '-')
        {
            //  Parse offset.
            parseOffset = true;
            
            //  Offset positive.
            relModeOff = -1;
            
            //  Skip '-'.
            i++;
            
            //  Skip spaces and tabs.
            SKIP_BLANK_CHARS

            //  Check end of line.  Missing instruction parameters.
            if (i == length)
                return false;
        }
        
        //  Check if an offset is expected.
        if (parseOffset)
        {
            //  Get the index offset.  Copy the number and use atoi.
            for(j = 0; (j < 10) && (i < length) && !((line[i] ==' ') ||
                (line[i] == '\t') || (line[i] == '\n') || (line[i] == ']'));
                i++, j++)
                addrStr[j] = line[i];

            //  Address was too large.
            if (j == 10)
                return FALSE;

            //  Mark end of string.
            addrStr[j] = '\0';

            //  Convert to int.
            relModeOff = relModeOff * atoi(addrStr);
        }
        
        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Check end of line.  Missing instruction parameters.
        if (i == length)
            return false;

        //  Check ']'.
        if (line[i] != ']')
            return false;

        //  Skip ']'.
        i++;

        //  Relative (indexed) mode addressing parsed correctly.
        return true;
    }
    else
    {
        //  Relative (indexed) mode addressing could not be parsed.
        return false;
    }

}

//  Parses the swizzle mode.
bool cgoShaderInstr::parseSwizzle(char *line, U32 &i, U32 length, SwizzleMode &swz)
{
    U32 j;
    U32 auxSwz;

    //  Check end of line.  Missing instruction parameters.
    if (i == length)
        return false;

    //  Get cc swizzle.

    //  Check if it is a single component broadcasted to all other components.
    if (((i + 1) == length) ||
        (line[i + 1] == ' ') || (line[i + 1] == '\t') ||
        (line[i + 1] == ')') || (line[i + 1] == ',') ||
        (line[i + 1] == '|') || (line[i + 1] == '\n'))
    {
        if (line[i] == 'x')
            swz = XXXX;
        else if (line[i] == 'y')
            swz = YYYY;
        else if (line[i] == 'z')
            swz = ZZZZ;
        else if (line[i] == 'w')
            swz = WWWW;
        else    //  Incorrect swizzle mode.
            return false;

        //  Skip component identifier.
        i++;
    }
    else
    {
        //  More than one input component.  Build the swizzle mask.

        auxSwz = 0;

        //  For each component of the mask determine the input component.
        for(j = 0; (j < 4) && (i < length); j++, i++)
        {
            auxSwz = auxSwz << 2;

            if (line[i] == 'x')
                auxSwz += 0;
            else if (line[i] == 'y')
                auxSwz += 1;
            else if (line[i] == 'z')
                auxSwz += 2;
            else if (line[i] == 'w')
                auxSwz += 3;
            else  //  Incorrect swizzle mode.
                return false;
        }

        //  Translate the swizzle mode.
        swz = translateSwizzleModeTable[auxSwz];
    }

    return true;
}

//  Parses an instruction operand.
bool cgoShaderInstr::parseOperand(char *line, U32 &i, U32 length,
    U32 &opR, Bank &opB, bool &opN, bool &opA, SwizzleMode &opM, bool integer,
    bool &relMode, U32 &relModeR, U08 &relModeComp, S16 &relModeOff)
{
    U32 j;
    char regStr[4];

    //  Skip spaces and tabs.
    SKIP_BLANK_CHARS

    //  Missing instruction parameters.
    if (i == length)
        return false;

    //  Test negate operand option.
    if (line[i] == '-')
    {
        //  Negate operand.
        opN = true;

        //  Skip '-'.
        i++;

        //  Missing instruction parameters.
        if (i == length)
            return false;

        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Missing instruction parameters.
        if (i == length)
            return false;
    }
    else    //  Default option.  No negate.
        opN = false;

    //  Test operand absolute value option.
    if (line[i] == '|')
    {
        //  Absolute value of the operand.
        opA = true;

        //  Skip '|'.
        i++;

        //  Missing instruction parameters.
        if (i == length)
            return false;

        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Missing instruction parameters.
        if (i == length)
            return false;
    }
    else    //  Default option.  No absolute value.
        opA = false;

    //  Get first operand bank.  Only readable banks.
    if (line[i] == 'i')
        opB = IN;
    else if (line[i] == 'r')
        opB = TEMP;
    else if (line[i] == 'a')
        opB = ADDR;
    else if (line[i] == 'c')
        opB = PARAM;
    else if (line[i] == 't')
        opB = TEXT;
    else if (line[i] == 's')
        opB = SAMP;
    else if ((line[i] >= '0') && (line[i] <= '9'))
        opB = IMM;
    else   //  Invalid operand bank.
        return false;

    //  Skip operand bank identifier unless the operand is an immediate.
    if (opB != IMM)
        i++;

    //  Missing instruction parameters.
    if (i == length)
        return false;

    //  Get register number.  Three digits only.  Copy and convert to int with atoi.
    for(j = 0; (j < 4) && (i < length) &&
        !((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\n') ||
        ((line[i] =='.') && (opB != IMM)) || (line[i] == '[') || (line[i] == ',') || (line[i] == '|'));
        i++, j++)
        regStr[j] = line[i];

    //  Register identifier out of range.
    if (j == 4)
        return false;

    //  Register identifier not specified.
    if (j == 0)
        return false;
        
    //  Mark end of string.
    regStr[j] = '\0';

    //  Check for immediate and immediate mode.
    if ((opB != IMM) || integer)
    {
        //  Convert to int.
        opR = atoi(regStr);
    }
    else
    {
        //  Read as float point.
        *((F32 *) &opR) = atof(regStr);
    }

    //  Constant bank can be accessed using two addressing modes.
    if ((opB == PARAM) && (line[i] == '['))
    {
        //  Relative mode addressing only allowed once in an instruction.
        if (relMode)
            return false;

        //  Relative mode addressing used in the instruction.
        relMode = true;

        //  Parse relative mode access to constant bank.
        if (!parseRelAddr(line, i, length, relModeR, relModeComp, relModeOff))
            return false;
    }

    //  Get operand swizzle mode.
    if (line[i] == '.')
    {
        if (opB == TEXT)
            return false;

        //  Skip '.'.
        i++;

        //  Mising instruction parameters.
        if (i == length)
            return false;

        //  Parse swizzle mode.
        if(!parseSwizzle(line, i, length, opM))
            return false;
    }
    else
    {
        opM = XYZW;
    }

    //  If absolute value option enabled, check closing.
    if (opA)
    {
        //  Skip spaces and tabs.
        SKIP_BLANK_CHARS

        //  Missing instruction parameters.
        if (i == length)
            return false;

        //  Check absolute value option closing.
        if (line[i] != '|')
            return false;

        //  Skip '|'.
        i++;
    }

    return true;
}

/*
 *  Determines the number of the operands of the instruction based
 *  in the instruction opcode.
 *
 */
U32 cgoShaderInstr::setNumOperands(ShOpcode opcode)
{

    switch(opcode)
    {
        case CG1_ISA_OPCODE_NOP: case CG1_ISA_OPCODE_END: case CG1_ISA_OPCODE_CHS:
            //  Instructions with 0 input operands.
            return 0;
            break;

        case CG1_ISA_OPCODE_ARL: case CG1_ISA_OPCODE_COS: case CG1_ISA_OPCODE_EX2: case CG1_ISA_OPCODE_EXP: case CG1_ISA_OPCODE_FLR: case CG1_ISA_OPCODE_FRC: case CG1_ISA_OPCODE_LG2: case CG1_ISA_OPCODE_LIT:
        case CG1_ISA_OPCODE_LOG: case CG1_ISA_OPCODE_MOV: case CG1_ISA_OPCODE_RCP: case CG1_ISA_OPCODE_RSQ: case CG1_ISA_OPCODE_SIN: case CG1_ISA_OPCODE_KIL: case CG1_ISA_OPCODE_ZXP: case CG1_ISA_OPCODE_DDX:
        case CG1_ISA_OPCODE_DDY: case CG1_ISA_OPCODE_JMP:
            //  Instructions with 1 input operand.
            return 1;
            break;

        case CG1_ISA_OPCODE_ADD: case CG1_ISA_OPCODE_DP3: case CG1_ISA_OPCODE_DP4: case CG1_ISA_OPCODE_DPH: case CG1_ISA_OPCODE_DST: case CG1_ISA_OPCODE_MAX: case CG1_ISA_OPCODE_MIN: case CG1_ISA_OPCODE_MUL:
        case CG1_ISA_OPCODE_SGE: case CG1_ISA_OPCODE_SLT: case CG1_ISA_OPCODE_TEX: case CG1_ISA_OPCODE_TXB: case CG1_ISA_OPCODE_TXP: case CG1_ISA_OPCODE_LDA: case CG1_ISA_OPCODE_KLS: case CG1_ISA_OPCODE_ZXS:
        case CG1_ISA_OPCODE_FXMUL: case CG1_ISA_OPCODE_SETPEQ: case CG1_ISA_OPCODE_SETPGT: case CG1_ISA_OPCODE_SETPLT: case CG1_ISA_OPCODE_TXL: case CG1_ISA_OPCODE_ANDP: case CG1_ISA_OPCODE_ADDI:
        case CG1_ISA_OPCODE_MULI: case CG1_ISA_OPCODE_STPEQI: case CG1_ISA_OPCODE_STPGTI: case CG1_ISA_OPCODE_STPLTI:
            //  Instructions with 2 input operands.
            return 2;
            break;

        case CG1_ISA_OPCODE_MAD: case CG1_ISA_OPCODE_CMP: case CG1_ISA_OPCODE_CMPKIL: case CG1_ISA_OPCODE_FXMAD: case CG1_ISA_OPCODE_FXMAD2:
            //  Instructions with 3 input operands.
            return 3;
            break;


        default:
            //  Invalid opcodes.
            return 0;
            break;
    }

}

//  Determines if the instruction is a scalar operation based on the opcode and the result write mask.
bool cgoShaderInstr::setIsScalar(ShOpcode opcode, MaskMode writeMask)
{
    switch(opcode)
    {
        //  Not implemented.
        case CG1_ISA_OPCODE_FLR:
            {
                char buffer[128];
                sprintf_s(buffer, sizeof(buffer), "Instruction opcode %02x not implemented.\n", opcode);
                CG_ASSERT(buffer);
            }           
            break;

        //  Pure vector instructions
        case CG1_ISA_OPCODE_DP3: case CG1_ISA_OPCODE_DP4: case CG1_ISA_OPCODE_DPH: case CG1_ISA_OPCODE_DST: case CG1_ISA_OPCODE_EXP: case CG1_ISA_OPCODE_LIT: case CG1_ISA_OPCODE_LOG: case CG1_ISA_OPCODE_TEX:
        case CG1_ISA_OPCODE_TXB: case CG1_ISA_OPCODE_TXP: case CG1_ISA_OPCODE_LDA: case CG1_ISA_OPCODE_KIL: case CG1_ISA_OPCODE_KLS: case CG1_ISA_OPCODE_ZXP: case CG1_ISA_OPCODE_ZXS: case CG1_ISA_OPCODE_TXL:

            return false;
            break;

        //  Vector instructions treated as a scalar instruction when using a scalar write mask.
        case CG1_ISA_OPCODE_ADD:   case CG1_ISA_OPCODE_ARL:   case CG1_ISA_OPCODE_CMP:    case CG1_ISA_OPCODE_CMPKIL: case CG1_ISA_OPCODE_EX2: case CG1_ISA_OPCODE_FRC: case CG1_ISA_OPCODE_LG2: case CG1_ISA_OPCODE_MAD:
        case CG1_ISA_OPCODE_MAX:   case CG1_ISA_OPCODE_MIN:   case CG1_ISA_OPCODE_MOV:    case CG1_ISA_OPCODE_MUL:    case CG1_ISA_OPCODE_SGE: case CG1_ISA_OPCODE_SLT: case CG1_ISA_OPCODE_RCP: case CG1_ISA_OPCODE_RSQ:
        case CG1_ISA_OPCODE_FXMUL: case CG1_ISA_OPCODE_FXMAD: case CG1_ISA_OPCODE_FXMAD2: case CG1_ISA_OPCODE_COS:    case CG1_ISA_OPCODE_SIN: case CG1_ISA_OPCODE_DDX: case CG1_ISA_OPCODE_DDY: case CG1_ISA_OPCODE_ADDI:
        case CG1_ISA_OPCODE_MULI:

            if ((writeMask == XNNN) || (writeMask == NYNN) || (writeMask == NNZN) || (writeMask == NNNW))
                return true;
            else
                return false;

            break;

        //  The predicate set instructions only work with scalar values and results.
        case CG1_ISA_OPCODE_SETPEQ: case CG1_ISA_OPCODE_SETPGT: case CG1_ISA_OPCODE_SETPLT: case CG1_ISA_OPCODE_ANDP: case CG1_ISA_OPCODE_STPEQI: case CG1_ISA_OPCODE_STPGTI: case CG1_ISA_OPCODE_STPLTI:
        
            return true;            
            break;
        
        //  Jump instructions are scalar.
        case CG1_ISA_OPCODE_JMP:
            return TRUE;
            break;

        //  Invalid opcodes.
        default:

            return false;
            break;
    }

    return true;
}

//  Determines if the instruction is Scalar compatible (scalar ALU) based on the opcode and the result write mask
bool cgoShaderInstr::setIsSOACompatible(ShOpcode opcode, MaskMode writeMask)
{
    switch(opcode)
    {
        //  Not implemented.
        case CG1_ISA_OPCODE_FLR:
            {
                char buffer[128];
                sprintf_s(buffer, sizeof(buffer), "Instruction opcode %02x not implemented.\n", opcode);
                CG_ASSERT(buffer);
            }
            
            break;

        //  Instructions with no equivalent in Scalar.
        case CG1_ISA_OPCODE_DP3: case CG1_ISA_OPCODE_DP4: case CG1_ISA_OPCODE_DPH:

            return false;
            break;

        //  Scalar instructions
        case CG1_ISA_OPCODE_EX2:  case CG1_ISA_OPCODE_LG2: case CG1_ISA_OPCODE_RCP: case CG1_ISA_OPCODE_RSQ: case CG1_ISA_OPCODE_FRC: case CG1_ISA_OPCODE_COS: case CG1_ISA_OPCODE_SIN:

            if ((writeMask == XNNN) || (writeMask == NYNN) || (writeMask == NNZN) || (writeMask == NNNW))
                return true;
            else
                return false;
                
            break;

        //  Instructions with fake Scalar support.  They would require special conversion
        //  code that is not currently implemented in the Scalar to SIMD4 translator.
        case CG1_ISA_OPCODE_DST: case CG1_ISA_OPCODE_EXP: case CG1_ISA_OPCODE_LIT: case CG1_ISA_OPCODE_LOG:

            //if ((writeMask == XNNN) || (writeMask == NYNN) || (writeMask == NNZN) || (writeMask == NNNW))
            //    return true;
            //else
            //    return false;

            return true;

            break;

        //  Special instructions supported in Scalar mode.
        case CG1_ISA_OPCODE_NOP: case CG1_ISA_OPCODE_TEX: case CG1_ISA_OPCODE_TXB: case CG1_ISA_OPCODE_TXP: case CG1_ISA_OPCODE_LDA: case CG1_ISA_OPCODE_KIL: case CG1_ISA_OPCODE_KLS: case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS: case CG1_ISA_OPCODE_CHS: case CG1_ISA_OPCODE_TXL:

            return true;

            break;

        //  Vector/SIMD4 instructions treated as a scalar instruction when using a scalar write mask.
        case CG1_ISA_OPCODE_ADD: case CG1_ISA_OPCODE_ARL: case CG1_ISA_OPCODE_CMP: case CG1_ISA_OPCODE_CMPKIL: case CG1_ISA_OPCODE_MAD:   case CG1_ISA_OPCODE_MAX:    case CG1_ISA_OPCODE_MIN: case CG1_ISA_OPCODE_MOV:
        case CG1_ISA_OPCODE_MUL: case CG1_ISA_OPCODE_SGE: case CG1_ISA_OPCODE_SLT: case CG1_ISA_OPCODE_FXMUL:  case CG1_ISA_OPCODE_FXMAD: case CG1_ISA_OPCODE_FXMAD2: case CG1_ISA_OPCODE_DDX: case CG1_ISA_OPCODE_DDY:
        case CG1_ISA_OPCODE_ADDI: case CG1_ISA_OPCODE_MULI:

            if ((writeMask == XNNN) || (writeMask == NYNN) || (writeMask == NNZN) || (writeMask == NNNW))
                return true;
            else
                return false;

            break;

        //  Set predicate instructions are only scalar.
        case CG1_ISA_OPCODE_SETPEQ: case CG1_ISA_OPCODE_SETPGT: case CG1_ISA_OPCODE_SETPLT: case CG1_ISA_OPCODE_ANDP: case CG1_ISA_OPCODE_STPEQI: case CG1_ISA_OPCODE_STPGTI: case CG1_ISA_OPCODE_STPLTI:
            return true;
            break;

        //  Jump instructions are scalar.
        case CG1_ISA_OPCODE_JMP:
            return true;
            break;
            
        //  Invalid opcodes.
        default:

            return false;
            break;
    }

    return true;
}

/*
 *  Gets the number of input operands in the instruction.
 *
 */

U32 cgoShaderInstr::getNumOperands()
{
    return numOperands;
}

/*
 *  Check if the instruction is an end.
 *
 */

bool cgoShaderInstr::isEnd()
{
    return isEndB;
}

/*
 *  Check if the instruction is an integer instruction.
 *
 */

bool cgoShaderInstr::isInteger()
{
    return isIntegerB;
}

/*
 *  Check if the instruction is a float point instruction.
 *
 */

bool cgoShaderInstr::isFloat()
{
    return isFloatB;
}

/*
 *  Check if the instruction is a scalar operation.
 *
 */

bool cgoShaderInstr::isScalar()
{
    return isScalarB;
}

/*
 *  Check if the instruction is compatible with a Scalar architecture.
 *
 */

bool cgoShaderInstr::isSOACompatible()
{
    return isSOACompatibleB;
}

/*
 *  Check if the instruction is a load instruction.
 *
 */

bool cgoShaderInstr::isALoad()
{
    return isALoadB;
}

/*
 *  Check if the instruction is a store instruction.
 *
 */

bool cgoShaderInstr::isAStore()
{
    return isAStoreB;
}

/*
 *  Check if the instruction is a Z export instruction.
 *
 */

bool cgoShaderInstr::isZExport()
{
    return isZExportB;
}

/*
 *  Get the instruction opcode.
 *
 */

ShOpcode cgoShaderInstr::getOpcode()
{
    return opcode;
}

/*
 *  Get the instruction binary codification.
 *
 */

void cgoShaderInstr::getCode(U08 *encode)
{
    //  WARNING:  DOES NOT CHECK BUFFER OVERFLOWS!!!!
    memcpy(encode, code.code8, CG1_ISA_INSTR_SIZE);
}

U32 cgoShaderInstr::getOperandIndex(int opIdx)
{
    return (opIdx == 1) ? op1 :
           (opIdx == 2) ? op2 :
           (opIdx == 3) ? op3 : 0;
}

/*
 *  Get the first operand register number.
 *
 */

U32 cgoShaderInstr::getOp1()
{
    return op1;
}


/*
 *  Get the second operand register number.
 *
 */

U32 cgoShaderInstr::getOp2()
{
    return op2;
}

/*
 *  Get the third operand register number.
 *
 */

U32 cgoShaderInstr::getOp3()
{
    return op3;
}

/*
 *  Get the first operand register bank.
 *
 */

Bank cgoShaderInstr::getBankOp1()
{
    return bankOp1;
}

/*
 *  Get the third operand register bank.
 *
 */

Bank cgoShaderInstr::getBankOp2()
{
    return bankOp2;
}

/*
 *  Get the third operand register bank.
 *
 */

Bank cgoShaderInstr::getBankOp3()
{
    return bankOp3;
}

/*
 *  Get the first operand swizzle mode.
 *
 */

SwizzleMode cgoShaderInstr::getOp1SwizzleMode()
{
    return swizzleModeOp1;
}

/*
 *  Get if the first operand must be negated.
 *
 */

bool cgoShaderInstr::getOp1NegateFlag()
{
    return negateFlagOp1;

}

/*
 *  Get if the first operand must be the absolute value of the register.
 *
 */

bool cgoShaderInstr::getOp1AbsoluteFlag()
{
    return absoluteFlagOp1;
}

/*
 *  Get the second operand swizzle mode.
 *
 */

SwizzleMode cgoShaderInstr::getOp2SwizzleMode()
{
    return swizzleModeOp2;
}

/*
 *  Get if the second operand must be negated.
 *
 */

bool cgoShaderInstr::getOp2NegateFlag()
{
    return negateFlagOp2;

}

/*
 *  Get if the second operand must be the absolute value of the register.
 *
 */

bool cgoShaderInstr::getOp2AbsoluteFlag()
{
    return absoluteFlagOp2;
}

/*
 *  Get the third operand swizzle mode.
 *
 */

SwizzleMode cgoShaderInstr::getOp3SwizzleMode()
{
    return swizzleModeOp3;
}

/*
 *  Get if the third operand must be negated.
 *
 */

bool cgoShaderInstr::getOp3NegateFlag()
{
    return negateFlagOp3;

}

/*
 *  Get if the third operand must be the absolute value of the register.
 *
 */

bool cgoShaderInstr::getOp3AbsoluteFlag()
{
    return absoluteFlagOp3;
}

/*
 *  Get the register number for the result register.
 *
 */

U32 cgoShaderInstr::getResult()
{
    return res;
}

/*
 *  Get the result register bank.
 *
 */

Bank cgoShaderInstr::getBankRes()
{
    return bankRes;
}

//  Gets the saturated result flag.  
bool cgoShaderInstr::getSaturatedRes()
{
    return saturatedRes;
}

//  Gets the end flag.  
bool cgoShaderInstr::getEndFlag()
{
    return endFlag;
}

/*
 *  Get the result write mask mode.
 *
 */

MaskMode cgoShaderInstr::getResultMaskMode()
{
    return maskMode;
}

/*
 *  Check if the instruction uses relative mode accessing to
 *  the constant bank.
 *
 */

bool cgoShaderInstr::getRelativeModeFlag()
{
    return relativeModeFlag;
}

/*
 *  Get the address register used for the relative mode addressing.
 *
 */

U32 cgoShaderInstr::getRelMAddrReg()
{
    return relModeAddrReg;
}


/*
 *  Get the address register component used for the relative mode addressing.
 *
 */

U32 cgoShaderInstr::getRelMAddrRegComp()
{
    return relModeAddrRegComp;
}

/*
 *  Get the relative addressing mode offset.
 *
 */

S16 cgoShaderInstr::getRelMOffset()
{
    return relModeOffset;
}

//  Sets the end flag of the instruction.
void cgoShaderInstr::setEndFlag(bool end)
{
    endFlag = end;
    isEndB = end;

    code.fields.lo64Bits.endflag = end ? 1 : 0;
}

//  Sets the wait point flag of the instruction.
void cgoShaderInstr::setWaitPointFlag()
{
    //  Set the instruction as being a wait point.
    waitPointFlag = true;

    //  Encode the wait point flag.
    code.fields.lo64Bits.waitpoint = 1;
}

//  Returns if the instruction is marked as a wait point.
bool cgoShaderInstr::isWaitPoint()
{
    return waitPointFlag;
}

//  Returns if the bank identifier is valid for operands.
bool cgoShaderInstr::validOperandBank(Bank opBank)
{
    switch (opBank)
    {
        case IN:
        case TEMP:
        case PARAM:
        case ADDR:

            return true;
            break;

        case OUT:
        case TEXT:
        case SAMP:
        case PRED:
            return false;
        
        default:
            CG_ASSERT("Undefined bank.");
            return false;
            break;
    }    
}

//  Returns if the bank identifier is valid for a result.
bool cgoShaderInstr::validResultBank(Bank opBank)
{
    switch(opBank)
    {
        case TEMP:
        case ADDR:
        case OUT:
        case PRED:
            return true;
            
        case IN:
        case PARAM:
        case TEXT:
        case SAMP:
            return false;
            
        default:
            CG_ASSERT("Undefined bank.");
            return false;
            break;
    }
}

//  Determine if the instruction writes a result.
bool cgoShaderInstr::setHasResult(ShOpcode opcode)
{
    switch(opcode)
    {
        case CG1_ISA_OPCODE_END:
        case CG1_ISA_OPCODE_NOP:
        case CG1_ISA_OPCODE_KIL:
        case CG1_ISA_OPCODE_KLS:
        case CG1_ISA_OPCODE_ZXP:
        case CG1_ISA_OPCODE_ZXS:
        case CG1_ISA_OPCODE_CHS:
        case CG1_ISA_OPCODE_JMP:
        case INVOPC:
            return false;
            break;
            
        default:
            return true;
            break;
    }
}

bool cgoShaderInstr::hasResult()
{
    return hasResultB;
}

bool cgoShaderInstr::resultIsPredicateReg(ShOpcode opcode)
{
    switch(opcode)
    {
        case CG1_ISA_OPCODE_SETPEQ:
        case CG1_ISA_OPCODE_SETPGT:
        case CG1_ISA_OPCODE_SETPLT:
        case CG1_ISA_OPCODE_ANDP:
        case CG1_ISA_OPCODE_STPEQI:
        case CG1_ISA_OPCODE_STPGTI:
        case CG1_ISA_OPCODE_STPLTI:
            return true;
            break;
       
        default:
            return false;
            break;
    }
}

const char *cgoShaderInstr::getBankString(Bank bank)
{
    switch (bank)
    {
        case IN:
            return "i";
            break;
        case OUT:
            return "o";
            break;
        case PARAM:
            return "c";
            break;
        case TEMP:
            return "r";
            break;
        case ADDR:
            return "a";
            break;
        case TEXT:
            return "t";
            break;
        case SAMP:
            return "s";
            break;
        case PRED:
            return "p";
            break;
        case INVALID:
            return "invalid";
            break;
        default:
            CG_ASSERT("Undefined register bank.");
            return "";
            break;
    }
}

const char *cgoShaderInstr::getOpcodeString(ShOpcode opcode)
{
    switch(opcode)
    {
        case LASTOPC:
            CG_ASSERT("Opcode LASTOPC not expected in encoded instructions.");
            return NULL;
            break;
        case MAXOPC:
            CG_ASSERT("Opcode MAXOPC not expected in encoded instructions.");
            return NULL;
            break;
        case INVOPC:
            return shOpcode2Str[LASTOPC];
            break;            
        default:
            return shOpcode2Str[opcode];
            break;
    }
    
}


bool cgoShaderInstr::getPredicatedFlag()
{
    return predicatedFlag;
}

bool cgoShaderInstr::getNegatePredicateFlag()
{
    return negatePredFlag;
}

U32 cgoShaderInstr::getPredicateReg()
{
    return predicateReg;
}

bool cgoShaderInstr::isAJump()
{
    return isAJumpB;
}

S32 cgoShaderInstr::getJumpOffset()
{
    return S32(immediate);
}

void cgoShaderInstr::setJumpOffset(S32 offset)
{
    *((S32 *) &immediate) = offset;
    
    //  Encode the new immediate value.
    code.fields.hi64Bits.immediate.immediate = immediate;
}

U32 cgoShaderInstr::getImmediate()
{
    return immediate;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of the cgoShaderInstrEncoding class
//
////////////////////////////////////////////////////////////////////////////////////////////////

//  cgoShaderInstrEncoding constructor.
cgoShaderInstr::cgoShaderInstrEncoding::cgoShaderInstrEncoding(cgoShaderInstr *shaderInstr, U32 pc, U32 nThread) :
shInstr(shaderInstr), PC(pc), numThread(nThread)
{
    //  Set default emulation function.
    emulFunc = NULL;

    //  Default pointer to instruction operands and result registers.  */
    shEmulOp1 = NULL;
    shEmulOp2 = NULL;
    shEmulOp3 = NULL;
    shEmulResult = NULL;
    shEmulPredicate = NULL;
    
    setTag("ShIDec");
}

//  Returns the shader instruction.
cgoShaderInstr *cgoShaderInstr::cgoShaderInstrEncoding::getShaderInstruction() const
{
    return shInstr;
}

/*
 *  Sets the emulation function for the instruction.
 *
 */

void cgoShaderInstr::cgoShaderInstrEncoding::setEmulFunc(void (*function)(cgoShaderInstr::cgoShaderInstrEncoding &, bmoUnifiedShader &))
{
    emulFunc = function;
}

/*
 *  Returns the function that emulates the instruction.
 *
 */

void (*cgoShaderInstr::cgoShaderInstrEncoding::getEmulFunc())(cgoShaderInstr::cgoShaderInstrEncoding &, bmoUnifiedShader &)
{
    return emulFunc;
}

//  Sets the Instruction PC.  
void cgoShaderInstr::cgoShaderInstrEncoding::setPC(U32 pc)
{
    PC = pc;
}

//  Returns the Shader Instruction PC.  
U32 cgoShaderInstr::cgoShaderInstrEncoding::getPC() const
{
    return PC;
}

//  Sets the Shader Instruction thread.  
void cgoShaderInstr::cgoShaderInstrEncoding::setNumThread(U32 nThread)
{
    numThread = nThread;
}

//  Returns the Shader Instruction thread.  
U32 cgoShaderInstr::cgoShaderInstrEncoding::getNumThread() const
{
    return numThread;
}

//  Sets pointer to instruction first operand.  
void cgoShaderInstr::cgoShaderInstrEncoding::setShEmulOp1(void *reg)
{
    //  Use the local store if NULL pointer.
    if (reg == 0)
        shEmulOp1 = (void *) op1;
    else
        shEmulOp1 = reg;
}

//  Gets the pointer to the instruction first operand.  
void *cgoShaderInstr::cgoShaderInstrEncoding::getShEmulOp1()
{
    return shEmulOp1;
}

//  Sets pointer to instruction second operand.  
void cgoShaderInstr::cgoShaderInstrEncoding::setShEmulOp2(void *reg)
{
    //  Use the local store if NULL pointer.
    if (reg == 0)
        shEmulOp2 = (void *) op2;
    else
        shEmulOp2 = reg;
}

//  Gets the pointer to the instruction second operand.  
void *cgoShaderInstr::cgoShaderInstrEncoding::getShEmulOp2()
{
    return shEmulOp2;
}

//  Sets pointer to instruction third operand.  
void cgoShaderInstr::cgoShaderInstrEncoding::setShEmulOp3(void *reg)
{
    //  Use the local store if NULL pointer.
    if (reg == 0)
        shEmulOp3 = (void *) op3;
    else
        shEmulOp3 = reg;
}

//  Gets the pointer to the instruction third operand.  
void *cgoShaderInstr::cgoShaderInstrEncoding::getShEmulOp3()
{
    return shEmulOp3;
}

//  Sets pointer to instruction result register.  
void cgoShaderInstr::cgoShaderInstrEncoding::setShEmulResult(void *reg)
{
    //  Use the local store if NULL pointer.
    if (reg == 0)
        shEmulResult = (void *) res;
    else
        shEmulResult = reg;
}

//  Gets the pointer to the instruction result register.  
void *cgoShaderInstr::cgoShaderInstrEncoding::getShEmulResult()
{
    return shEmulResult;
}

// Set the pointer to the predicate register.
void cgoShaderInstr::cgoShaderInstrEncoding::setShEmulPredicate(void *reg)
{
    shEmulPredicate = reg;
}

// Get the pointer to the predicate register.
void *cgoShaderInstr::cgoShaderInstrEncoding::getShEmulPredicate()
{
    return shEmulPredicate;
}

//  Get the pointer to the local data array for the instruction first operand.
U08 *cgoShaderInstr::cgoShaderInstrEncoding::getOp1Data()
{
    return op1;
}

//  Get the pointer to the local data array for the instruction second operand.
U08 *cgoShaderInstr::cgoShaderInstrEncoding::getOp2Data()
{
    return op2;
}

//  Get the pointer to the local data array for the instruction third operand.
U08 *cgoShaderInstr::cgoShaderInstrEncoding::getOp3Data()
{
    return op3;
}

//  Get the pointer to the local data array for the instruction result.
U08 *cgoShaderInstr::cgoShaderInstrEncoding::getResultData()
{
    return res;
}

