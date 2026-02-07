/**************************************************************************
 *
 */

#ifndef GALx_IRTRAVERSER_H
    #define GALx_IRTRAVERSER_H

#include <iostream>
#include "GALxIRNode.h"

namespace libGAL
{

namespace GenerationCode
{

class GALxIRTraverser {

protected:

    GALxIRTraverser() : 
        depth(0),
        preVisit(true),
        postVisit(false),
        rightToLeft(false) {}

public:
    

    virtual void depthProcess() {};

    virtual bool visitProgram(bool, IRProgram*, GALxIRTraverser*) { return true; }
    virtual bool visitVP1Option(bool, VP1IROption*, GALxIRTraverser*) { return true; }
    virtual bool visitFP1Option(bool, FP1IROption*, GALxIRTraverser*) { return true; }
    virtual bool visitStatement(bool, IRStatement*, GALxIRTraverser*) { return true; }
    virtual bool visitInstruction(bool, IRInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitSampleInstruction(bool, IRSampleInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitKillInstruction(bool, IRKillInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitZExportInstruction(bool, IRZExportInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitCHSInstruction(bool, IRCHSInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitSwizzleComponents(bool, IRSwizzleComponents*, GALxIRTraverser*) { return true; }
    virtual bool visitSwizzleInstruction(bool, IRSwizzleInstruction*, GALxIRTraverser*) { return true; }
    virtual bool visitDstOperand(bool, IRDstOperand*, GALxIRTraverser*) { return true; }
    virtual bool visitSrcOperand(bool, IRSrcOperand*, GALxIRTraverser*) { return true; }
    virtual bool visitArrayAddressing(bool, IRArrayAddressing*, GALxIRTraverser*)    { return true;  }
    virtual bool visitNamingStatement(bool, IRNamingStatement*, GALxIRTraverser*)    { return true; }
    virtual bool visitALIASStatement(bool, IRALIASStatement*, GALxIRTraverser*)  { return true;  }
    virtual bool visitTEMPStatement(bool, IRTEMPStatement*, GALxIRTraverser*)    { return true;  }
    virtual bool visitADDRESSStatement(bool, IRADDRESSStatement*, GALxIRTraverser*) { return true;   }
    virtual bool visitVP1ATTRIBStatement(bool, VP1IRATTRIBStatement*, GALxIRTraverser*)  {   return true;    }
    virtual bool visitFP1ATTRIBStatement(bool, FP1IRATTRIBStatement*, GALxIRTraverser*)  {   return true;    }
    virtual bool visitVP1OUTPUTStatement(bool, VP1IROUTPUTStatement*, GALxIRTraverser*)  {   return true;    }
    virtual bool visitFP1OUTPUTStatement(bool, FP1IROUTPUTStatement*, GALxIRTraverser*)  {   return true;    }
    virtual bool visitPARAMStatement(bool, IRPARAMStatement*, GALxIRTraverser*)  {   return true;    }
    virtual bool visitParamBinding(bool, IRParamBinding*, GALxIRTraverser*) {    return true; }
    virtual bool visitLocalEnvBinding(bool, IRLocalEnvBinding*, GALxIRTraverser*) { return true; }
    virtual bool visitConstantBinding(bool, IRConstantBinding*, GALxIRTraverser*) { return true; }
    virtual bool visitStateBinding(bool, IRStateBinding*, GALxIRTraverser*) { return true;   }
    
    virtual ~GALxIRTraverser() {}    
    
    int  depth;
    bool preVisit;
    bool postVisit;
    bool rightToLeft;
};

class GALxOutputTraverser: public GALxIRTraverser
{
private:
    std::ostream& os;
    unsigned int indentation;
    
    void depthProcess();

public:
    GALxOutputTraverser(std::ostream& ostr): os(ostr), indentation(0) {};
    virtual bool visitProgram(bool preVisitAction, IRProgram*, GALxIRTraverser* it);
    virtual bool visitVP1Option(bool preVisitAction, VP1IROption*, GALxIRTraverser* it);
    virtual bool visitFP1Option(bool preVisitAction, FP1IROption*, GALxIRTraverser* it);
    virtual bool visitStatement(bool preVisitAction, IRStatement*, GALxIRTraverser* it);
    virtual bool visitInstruction(bool preVisitAction, IRInstruction*, GALxIRTraverser* it);
    virtual bool visitSampleInstruction(bool preVisitAction, IRSampleInstruction*, GALxIRTraverser* it);
    virtual bool visitKillInstruction(bool preVisitAction, IRKillInstruction*, GALxIRTraverser* it);
    virtual bool visitZExportInstruction(bool, IRZExportInstruction*, GALxIRTraverser*);
    virtual bool visitCHSInstruction(bool preVisitAction, IRCHSInstruction*, GALxIRTraverser* it);
    virtual bool visitSwizzleComponents(bool preVisitAction, IRSwizzleComponents*, GALxIRTraverser* it);
    virtual bool visitSwizzleInstruction(bool preVisitAction, IRSwizzleInstruction*, GALxIRTraverser* it);
    virtual bool visitDstOperand(bool preVisitAction, IRDstOperand*, GALxIRTraverser* it);
    virtual bool visitSrcOperand(bool preVisitAction, IRSrcOperand*, GALxIRTraverser* it);
    virtual bool visitArrayAddressing(bool preVisitAction, IRArrayAddressing*, GALxIRTraverser* it);
    virtual bool visitNamingStatement(bool preVisitAction, IRNamingStatement*, GALxIRTraverser* it);
    virtual bool visitALIASStatement(bool preVisitAction, IRALIASStatement*, GALxIRTraverser* it);
    virtual bool visitTEMPStatement(bool preVisitAction, IRTEMPStatement*, GALxIRTraverser* it);
    virtual bool visitADDRESSStatement(bool preVisitAction, IRADDRESSStatement*, GALxIRTraverser* it);
    virtual bool visitVP1ATTRIBStatement(bool preVisitAction, VP1IRATTRIBStatement*, GALxIRTraverser* it);
    virtual bool visitFP1ATTRIBStatement(bool preVisitAction, FP1IRATTRIBStatement*, GALxIRTraverser* it);
    virtual bool visitVP1OUTPUTStatement(bool preVisitAction, VP1IROUTPUTStatement*, GALxIRTraverser* it);
    virtual bool visitFP1OUTPUTStatement(bool preVisitAction, FP1IROUTPUTStatement*, GALxIRTraverser* it);
    virtual bool visitPARAMStatement(bool preVisitAction, IRPARAMStatement*, GALxIRTraverser* it);
    virtual bool visitParamBinding(bool preVisitAction, IRParamBinding*, GALxIRTraverser* it);
    virtual bool visitLocalEnvBinding(bool preVisitAction, IRLocalEnvBinding*, GALxIRTraverser* it);
    virtual bool visitConstantBinding(bool preVisitAction, IRConstantBinding*, GALxIRTraverser* it);
    virtual bool visitStateBinding(bool preVisitAction, IRStateBinding*, GALxIRTraverser* it);
    
};  

} // namespace GenerationCode

} // namespace libGAL

#endif // GALx_IRTRAVERSER_H
