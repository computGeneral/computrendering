/**************************************************************************
 *
 */

#ifndef SEMANTICTRAVERSER_H
    #define SEMANTICTRAVERSER_H

#include <iostream>
#include <sstream>
#include <bitset>

#include "GALxIRTraverser.h"
#include "GALxSymbolTable.h"
#include "GALxSharedDefinitions.h"
//#include "GALxProgramExecutionEnvironment.h"
#include "GPUType.h"
#include "GALxImplementationLimits.h"
#include "gl.h"
#include "glext.h"

namespace libGAL
{

namespace GenerationCode
{

class GALxSemanticTraverser  : public GALxIRTraverser//, public ShaderSemanticCheck
{
public:

    typedef std::bitset<MAX_PROGRAM_ATTRIBS_ARB> AttribMask;
    
private:
    std::stringstream errorOut;
    
    GALxSymbolTable<IdentInfo *> symtab;
    std::list<IdentInfo*> identinfoCollector;  ///< Required because the Symbol Table doesn�t manages IdentInfo destruction.
    
    bool semanticErrorInTraverse;

    // Auxiliar functions 
    bool swizzleMaskCheck(const char *smask);
    bool writeMaskCheck(const char *wmask);
    IdentInfo *recursiveSearchSymbol(std::string id, bool& found); ///< Needed for ALIAS name resolve

    // Resource usage statistics 
    unsigned int numberOfInstructions;
    unsigned int numberOfStatements;
    unsigned int numberOfParamRegs;
    unsigned int numberOfTempRegs;
    unsigned int numberOfAttribs;
    unsigned int numberOfAddrRegs;
    int textureUnitUsage [MAX_TEXTURE_UNITS_ARB];
    bool killInstructions;
    
    AttribMask inputRegisterRead;
    AttribMask outputRegisterWritten;
    
    /* For semantic check about sampling from multiple texture targets 
       on the same texture image unit */
    
    GLenum TextureImageUnitsTargets[MAX_TEXTURE_UNITS_ARB];
    

public:


    GALxSemanticTraverser ();

    std::string getErrorString() const  {   return errorOut.str();  }
    bool foundErrors() const            {   return semanticErrorInTraverse; }
    
    U32 getNumberOfInstructions() const { return numberOfInstructions; }
    U32 getNumberOfParamRegs() const  { return numberOfParamRegs; }
    U32 getNumberOfTempRegs() const { return numberOfTempRegs; }
    U32 getNumberOfAddrRegs() const { return numberOfAddrRegs; }
    U32 getNumberOfAttribs() const { return numberOfAttribs; }
    
    GLenum getTextureUnitTarget(GLuint tu) const { return TextureImageUnitsTargets[tu];    }
    GLboolean getKillInstructions() const { return killInstructions; }
    
    AttribMask getAttribsWritten() const;
    AttribMask getAttribsRead() const;

    virtual bool visitProgram(bool preVisitAction, IRProgram*, GALxIRTraverser*);
    virtual bool visitVP1Option(bool preVisitAction, VP1IROption*, GALxIRTraverser*);
    virtual bool visitFP1Option(bool preVisitAction, FP1IROption*, GALxIRTraverser*);
    virtual bool visitStatement(bool preVisitAction, IRStatement*, GALxIRTraverser*);
    virtual bool visitInstruction(bool preVisitAction, IRInstruction*, GALxIRTraverser*);
    virtual bool visitSampleInstruction(bool preVisitAction, IRSampleInstruction*, GALxIRTraverser*);
    virtual bool visitKillInstruction(bool preVisitAction, IRKillInstruction*, GALxIRTraverser*);
    virtual bool visitZExportInstruction(bool preVisitAction, IRZExportInstruction*, GALxIRTraverser*);
    virtual bool visitCHSInstruction(bool preVisitAction, IRCHSInstruction*, GALxIRTraverser*);
    virtual bool visitSwizzleComponents(bool preVisitAction, IRSwizzleComponents*, GALxIRTraverser*);
    virtual bool visitSwizzleInstruction(bool preVisitAction, IRSwizzleInstruction*, GALxIRTraverser*);
    virtual bool visitDstOperand(bool preVisitAction, IRDstOperand*, GALxIRTraverser*);
    virtual bool visitArrayAddressing(bool preVisitAction, IRArrayAddressing *, GALxIRTraverser*);
    virtual bool visitSrcOperand(bool preVisitAction, IRSrcOperand*, GALxIRTraverser*);
    virtual bool visitNamingStatement(bool preVisitAction, IRNamingStatement*, GALxIRTraverser*);
    virtual bool visitALIASStatement(bool preVisitAction, IRALIASStatement*, GALxIRTraverser*);
    virtual bool visitTEMPStatement(bool preVisitAction, IRTEMPStatement*, GALxIRTraverser*);
    virtual bool visitADDRESSStatement(bool preVisitAction, IRADDRESSStatement* addrstmnt, GALxIRTraverser* it);
    virtual bool visitVP1ATTRIBStatement(bool preVisitAction, VP1IRATTRIBStatement*, GALxIRTraverser*);
    virtual bool visitFP1ATTRIBStatement(bool preVisitAction, FP1IRATTRIBStatement*, GALxIRTraverser*);
    virtual bool visitVP1OUTPUTStatement(bool preVisitAction, VP1IROUTPUTStatement*, GALxIRTraverser*);
    virtual bool visitFP1OUTPUTStatement(bool preVisitAction, FP1IROUTPUTStatement*, GALxIRTraverser*);
    virtual bool visitPARAMStatement(bool preVisitAction, IRPARAMStatement*, GALxIRTraverser*);
    virtual bool visitParamBinding(bool preVisitAction, IRParamBinding*, GALxIRTraverser*);
    virtual bool visitLocalEnvBinding(bool preVisitAction, IRLocalEnvBinding*, GALxIRTraverser*);
    virtual bool visitConstantBinding(bool preVisitAction, IRConstantBinding*, GALxIRTraverser*);
    virtual bool visitStateBinding(bool preVisitAction, IRStateBinding*, GALxIRTraverser*);

    virtual ~GALxSemanticTraverser (void);
};

} // CodeGeneration

} // namespace libGAL


#endif // SEMANTICTRAVERSER_H
