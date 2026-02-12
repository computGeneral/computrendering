/**************************************************************************
 *
 */

#ifndef ____SHADER_TRANSLATOR
#define ____SHADER_TRANSLATOR

#include "Types.h"
#include "IR.h"
#include "IRTranslator.h"
#include "IRPrinter.h"
#include "IRDisassembler.h"
#include "IRBuilder.h"


class ShaderTranslator  {
public:
	NativeShader *translate(IR *ir, D3DCMPFUNC alpha_test = D3DCMP_ALWAYS, bool fogEnable = false);
	U32 buildIR(const DWORD *source, IR* &ir);
    static ShaderTranslator &get_instance();
private:
    ShaderTranslator();
    ~ShaderTranslator();
    std::string printInstructionsDisassembled(std::vector<arch::cgoShaderInstr *> instr);
    std::string printInstructionsBinary(std::vector<arch::cgoShaderInstr *> instr);
    IRTranslator *translator;
    IRPrinter *printer;
    IRDisassembler *disassembler;
    IRBuilder *builder;
};



#endif
