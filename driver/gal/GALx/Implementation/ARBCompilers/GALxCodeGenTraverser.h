/**************************************************************************
 *
 */

#ifndef GALx_CODEGENTRAVERSER_H
    #define GALx_CODEGENTRAVERSER_H

#include "GALxIRTraverser.h"
#include "GALxRBank.h"
#include "GALxSymbolTable.h"
#include "GALxGenericInstruction.h"
#include "GALxSharedDefinitions.h"
#include "GALxImplementationLimits.h"
#include "GALxProgramExecutionEnvironment.h"
#include <string>
#include <list>

namespace libGAL
{

namespace GenerationCode
{

class GALxCodeGenTraverser : public GALxIRTraverser
{
private:

    GALxSymbolTable<IdentInfo* > symtab;            ///< Reference to symbol Table filled in semantic check
    std::list<IdentInfo*> identinfoCollector;  ///< Required because the Symbol Table doesnït manages IdentInfo destruction.
    
    std::list<GALxGenericInstruction*> genericCode;  ///< Generic Instruction code generated.
    
    // Auxiliar attributes to construct instructions

    libGAL::GALxRBank<F32> fragmentInBank;   ///< Bank for vertex in attributes. Not used
    libGAL::GALxRBank<F32> fragmentOutBank;      ///< Bank for vertex out attributes. Not used

    libGAL::GALxRBank<F32> parametersBank;   ///< Bank for constants, local parameters,
                                    ///< environment parameters and GL states.
    libGAL::GALxRBank<F32> temporariesBank;   ///< Bank for temporaries in code.
    libGAL::GALxRBank<F32> addressBank;      ///< Bank for address registers in code.

    GALxGenericInstruction::OperandInfo op[3];          ///< Operands structs info for instruction generation
    GALxGenericInstruction::ResultInfo res;             ///< Result struct info for instruction generation
    GALxGenericInstruction::SwizzleInfo swz;            ///< Swizzle struct info for the SWZ instruction generation
    GALxGenericInstruction::TextureTarget texTarget;    ///< Texture Target info for 
                                                    ///< the TEX instruction generation
    unsigned int texImageUnit;  ///< Texture Unit info for the TEX instruction generation
    
    unsigned int killSample;    ///< killed sample for the KLS instruction generation.
    unsigned int exportSample;  ///< export sample for the ZXS instruction generation.
    bool isFixedPoint; 

    unsigned int paramBindingRegister[MAX_PROGRAM_PARAMETERS_ARB];
    unsigned int paramBindingIndex;
    unsigned int operandIndex;  ///< Index of operand in process
    unsigned int registerPosition;

public:
    // Auxiliar functions
    static unsigned int processSwizzleSuffix(const char* suffix);
    static unsigned int processWriteMask(const char* mask);
    static unsigned int fragmentAttributeBindingMapToRegister(const std::string attribute);
    static unsigned int vertexAttributeBindingMapToRegister(const std::string attribute);
    static unsigned int fragmentResultAttribBindingMaptoRegister(const std::string attribute);
    static unsigned int vertexResultAttribBindingMaptoRegister(const std::string attribute);
    static unsigned int paramStateBindingIdentifier(const std::string state, unsigned int& rowIndex);
    static std::string getWordWithoutIndex(std::string str, unsigned int& index);
    static std::string getWordBetweenDots(std::string str, unsigned int level);
    IdentInfo *recursiveSearchSymbol(std::string id); ///< Needed for ALIAS name resolve

public:
    GALxCodeGenTraverser();
    
    libGAL::GALxRBank<F32>& getParametersBank() { return parametersBank; }
    libGAL::GALxRBank<F32>& getTemporariesBank() { return temporariesBank;   }

    std::list<GALxGenericInstruction*>& getGenericCode() {  return genericCode; }

    bool visitProgram(bool preVisitAction, IRProgram*, GALxIRTraverser*);
    bool visitVP1Option(bool preVisitAction, VP1IROption*, GALxIRTraverser*);
    bool visitFP1Option(bool preVisitAction, FP1IROption*, GALxIRTraverser*);
    bool visitStatement(bool preVisitAction, IRStatement*, GALxIRTraverser*);
    bool visitInstruction(bool preVisitAction, IRInstruction*, GALxIRTraverser*);
    bool visitSampleInstruction(bool preVisitAction, IRSampleInstruction*, GALxIRTraverser*);
    bool visitKillInstruction(bool preVisitAction, IRKillInstruction*, GALxIRTraverser*);
    bool visitZExportInstruction(bool preVisitAction, IRZExportInstruction*, GALxIRTraverser*);
    bool visitCHSInstruction(bool preVisitAction, IRCHSInstruction*, GALxIRTraverser*);
    bool visitSwizzleComponents(bool preVisitAction, IRSwizzleComponents*, GALxIRTraverser*);
    bool visitSwizzleInstruction(bool preVisitAction, IRSwizzleInstruction*, GALxIRTraverser*);
    bool visitDstOperand(bool preVisitAction, IRDstOperand*, GALxIRTraverser*);
    bool visitSrcOperand(bool preVisitAction, IRSrcOperand*, GALxIRTraverser*);
    bool visitArrayAddressing(bool preVisitAction, IRArrayAddressing*, GALxIRTraverser*);
    bool visitNamingStatement(bool preVisitAction, IRNamingStatement*, GALxIRTraverser*);
    bool visitALIASStatement(bool preVisitAction, IRALIASStatement*, GALxIRTraverser*);
    bool visitTEMPStatement(bool preVisitAction, IRTEMPStatement*, GALxIRTraverser*);
    bool visitADDRESSStatement(bool preVisitAction, IRADDRESSStatement*, GALxIRTraverser*);
    bool visitVP1ATTRIBStatement(bool preVisitAction, VP1IRATTRIBStatement*, GALxIRTraverser*);
    bool visitFP1ATTRIBStatement(bool preVisitAction, FP1IRATTRIBStatement*, GALxIRTraverser*);
    bool visitVP1OUTPUTStatement(bool preVisitAction, VP1IROUTPUTStatement*, GALxIRTraverser*);
    bool visitFP1OUTPUTStatement(bool preVisitAction, FP1IROUTPUTStatement*, GALxIRTraverser*);
    bool visitPARAMStatement(bool preVisitAction, IRPARAMStatement*, GALxIRTraverser*);
    bool visitParamBinding(bool preVisitAction, IRParamBinding*, GALxIRTraverser*);
    bool visitLocalEnvBinding(bool preVisitAction, IRLocalEnvBinding*, GALxIRTraverser*);
    bool visitConstantBinding(bool preVisitAction, IRConstantBinding*, GALxIRTraverser*);
    bool visitStateBinding(bool preVisitAction, IRStateBinding*, GALxIRTraverser*);
    
    void dumpGenericCode(std::ostream& os);

    ~GALxCodeGenTraverser(void);

};

} // namespace GenerationCode

} // namespace libGAL

#endif // GALx_CODEGENTRAVERSER_H
