/**************************************************************************
 *
 * Shader Architecture Parameters.
 * This file implements the ShaderArchParam class.
 *
 * The ShaderArchParam class defines a container for architecture parameters (instruction
 * execution latencies, repeation rates, etc) for a simulated shader architecture.
 *
 */
#include "cmShaderArchParam.h"

using namespace cg1gpu;

//  Pointer to the single instance of the class.
ShaderArchParam *ShaderArchParam::shArchParams = NULL;

//
//  Latency execution and and repeat rate tables for the different architecture
//  configurations.
//

//  Execution latency table for Variable Execution Latency SIMD4 Architecture
U32 ShaderArchParam::SIMD4VarLatExecLatencyTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      3,      2,      3,      1,      0,      0,      12,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        3,      3,      3,      4,      5,      9,      0,      3,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        5,      4,      9,      3,      3,      3,      3,      3,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        2,      5,      0,      5,      3,      3,      3,      3,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        12,     2,      3,      2,      2,      1,      1,      1,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        5,      3,      3,      3,      3,      3,      3,      3,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      3,      3,      3,      16,     16,     1,      1
};

//  Repeat Rate table for Variable Execution Latency SIMD4 Architecture
U32 ShaderArchParam::SIMD4VarLatRepeatRateTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      1,      1,      1,      1,      0,      0,      6,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        1,      1,      1,      1,      2,      4,      0,      1,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        2,      1,      4,      1,      1,      1,      1,      1,

    //  MULI    RCP     INV,    RSQ     SETPEQ  SETPGT  SGE     SETPGT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        1,      4,      0,      4,      1,      1,      1,      1,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        6,      1,      1,      1,      1,      1,      1,      1,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        1,      1,      1,      1,      1,      1,      1,      1,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      1,      1,      1,      8,      8,      1,      1

};

//  Execution latency table for Fixed Execution Latency SIMD4 Architecture
U32 ShaderArchParam::SIMD4FixLatExecLatencyTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        3,      3,      3,      3,      3,      0,      0,      12,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        3,      3,      3,      3,      3,      3,      0,      3,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        3,      3,      3,      3,      3,      3,      3,      3,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        3,      3,      0,      3,      3,      3,      3,      3,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        12,     3,      3,      3,      3,      3,      3,      3,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        3,      3,      3,      3,      3,      3,      3,      3,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        3,      3,      3,      3,      16,     16,     3,      3
};

//  Repeat Rate table for Fixed Execution Latency SIMD4 Architecture
U32 ShaderArchParam::SIMD4FixLatRepeatRateTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      1,      1,      1,      1,      0,      0,      6,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        1,      1,      1,      1,      2,      4,      0,      1,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        2,      1,      4,      1,      1,      1,      1,      1,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        1,      4,      0,      4,      1,      1,      1,      1,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        6,      1,      1,      1,      1,      1,      1,      1,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        1,      1,      1,      1,      1,      1,      1,      1,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      1,      1,      1,      8,      8,      1,      1
};

//  Execution latency table for Variable Execution Latency Scalar Architecture
U32 ShaderArchParam::ScalarVarLatExecLatencyTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      3,      2,      3,      1,      0,      0,      12,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        0,      0,      0,      4,      5,      9,      0,      3,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        5,      4,      9,      3,      3,      3,      3,      3,

    //  MULI    RCP     INV,    RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        2,      5,      0,      5,      3,      3,      3,      3,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        12,     2,      3,      2,      2,      4,      3,      4,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        7,      3,      3,      3,      3,      3,      3,      3,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      3,      3,      3,      16,     16,     1,      1
};

//  Repeat Rate table for Variable Execution Latency SIMD4 Architecture
U32 ShaderArchParam::ScalarVarLatRepeatRateTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      1,      1,      1,      1,      0,      0,      6,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        0,      0,      0,      8,      2,      4,      0,      1,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        2,      7,      4,      1,      1,      1,      1,      1,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        1,      4,      0,      4,      1,      1,      1,      1,

    //  SIN     STPEQI  SLT     STGTI   STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        6,      1,      1,      1,      1,      4,      3,      4,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        4,      1,      1,      1,      1,      1,      1,      1,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      1,      1,      1,      8,      8,      1,      1
};

//  Execution latency table for Fixed Execution Latency Scalar Architecture
U32 ShaderArchParam::ScalarFixLatExecLatencyTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        3,      3,      3,      3,      3,      0,      0,      12,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        3,      3,      3,      3,      3,      3,      0,      3,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        3,      3,      3,      3,      3,      3,      3,      3,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        3,      3,      0,      3,      3,      3,      3,      3,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        12,     3,      3,      3,      3,      3,      3,      3,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        3,      3,      3,      3,      3,      3,      3,      3,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        3,      3,      3,      3,      16,     16,     3,      3
};

//  Repeat Rate table for Fixed Execution Latency SIMD4 Architecture
U32 ShaderArchParam::ScalarFixLatRepeatRateTable[LASTOPC] =
{
    //  NOP     ADD     ADDI    ARL     ANDP    INV     INV     COS
    //  00h     01h     02h     03h     04h     05h     06h     07h
        1,      1,      1,      1,      1,      0,      0,      6,

    //  DP3     DP4     DPH     DST     EX2     EXP     FLR     FRC
    //  08h     09h     0Ah     0Bh     0Ch     0Dh     0Eh     0Fh
        0,      0,      0,      8,      2,      4,      0,      1,

    //  LG2     LIT     LOG     MAD     MAX     MIN     MOV     MUL
    //  10h     11h     12h     13h     14h     15h     16h     17h
        2,      7,      4,      1,      1,      1,      1,      1,

    //  MULI    RCP     INV     RSQ     SETPEQ  SETPGT  SGE     SETPLT
    //  18h     19h     1Ah     1Bh     1Ch     1Dh     1Eh     1Fh
        1,      4,      0,      4,      1,      1,      1,      1,

    //  SIN     STPEQI  SLT     STPGTI  STPLTI  TXL     TEX     TXB
    //  20h     21h     22h     23h     24h     25h     26h     27h
        6,      1,      1,      1,      1,      4,      3,      4,

    //  TXP     KIL     KLS     ZXP     ZXS     CMP     CMPKIL  CHS
    //  28h     29h     2Ah     2Bh     2Ch     2Dh     2Eh     2Fh
        4,      1,      1,      1,      1,      1,      1,      1,

    //  LDA     FXMUL   FXMAD   FXMAD2  DDX     DDY     JMP     END
    //  30h     31h     32h     33h     34h     35h     36h     37h
        1,      1,      1,      1,      8,      8,      1,      1
};

//  Private constructor.
ShaderArchParam::ShaderArchParam()
{
    execLatencyTable = NULL;
    repeatRateTable = NULL;
}

//  Obtain pointer to singleton.  Create if required.
ShaderArchParam *ShaderArchParam::getShaderArchParam()
{
    //  Check if singleton has been created.
    if (shArchParams == NULL)
    {
        shArchParams = new ShaderArchParam;
    }

    return shArchParams;
}

//  Returns a list of shader architecture names.
void ShaderArchParam::architectureList(string &archList)
{
    archList = "SIMD4VarLat ScalarVarLat SIMD4FixLat ScalarFixLat";
}

//  Selects the shader architecture to query.
void ShaderArchParam::selectArchitecture(string archName)
{
    if (archName.compare("SIMD4VarLat") == 0)
    {
        execLatencyTable = SIMD4VarLatExecLatencyTable;
        repeatRateTable = SIMD4VarLatRepeatRateTable;
    }
    else if (archName.compare("ScalarVarLat") == 0)
    {
        execLatencyTable = ScalarVarLatExecLatencyTable;
        repeatRateTable = ScalarVarLatRepeatRateTable;
    }
    else if (archName.compare("SIMD4FixLat") == 0)
    {
        execLatencyTable = SIMD4FixLatExecLatencyTable;
        repeatRateTable = SIMD4FixLatRepeatRateTable;
    }
    else if (archName.compare("ScalarFixLat") == 0)
    {
        execLatencyTable = ScalarFixLatExecLatencyTable;
        repeatRateTable = ScalarFixLatRepeatRateTable;
    }
    else
    {
        execLatencyTable = NULL;
        repeatRateTable = NULL;
    }
}

//  Query the execution latency for the shader instruction opcode in the current architecture.
U32 ShaderArchParam::getExecutionLatency(ShOpcode opc)
{
    CG_ASSERT_COND((opc != INVOPC),"Invalid opcode");
    CG_ASSERT_COND((opc < LASTOPC), "Opcode out of valid range");
    CG_ASSERT_COND((execLatencyTable != NULL), "Shader architecture not defined");
    /*
    NOTE: CHECK MOVED. This check is now performed in shader decode execute.  The reason is
    that some instruction may be allowed in intermediate steps in the library but are not
    allowed in the shader (DPx in Scalar mode.
    CG_ASSERT_COND((execLatencyTable[opc] != 0),"Instruction not implemented");
    */
    return execLatencyTable[opc];
}

//  Query the repeat rate for the shader instruction opcode in the current architecture.
U32 ShaderArchParam::getRepeatRate(ShOpcode opc)
{
    CG_ASSERT_COND((opc != INVOPC), "Invalid opcode");
    CG_ASSERT_COND((opc < LASTOPC), "Opcode out of valid range");
    CG_ASSERT_COND((repeatRateTable != NULL), "Shader architecture not defined");

    /*
    NOTE: CHECK MOVED. This check is now performed in shader decode execute.  The reason is
    that some instruction may be allowed in intermediate steps in the library but are not
    allowed in the shader (DPx in Scalar mode.
    CG_ASSERT_COND((repeatRateTable[opc] != 0), "Instruction not implemented");
    */
    return repeatRateTable[opc];
}
