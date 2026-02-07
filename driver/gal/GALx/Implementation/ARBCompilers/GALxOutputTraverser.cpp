/**************************************************************************
 *
 */

#include "GALxIRTraverser.h"
#include "support.h"

using namespace libGAL::GenerationCode;
using namespace libGAL;
using namespace std;

void GALxOutputTraverser::depthProcess()
{
    int i;
    
    for (i = 0; i < depth; ++i)
        os << "  ";
}

bool GALxOutputTraverser::visitProgram(bool , IRProgram* program, GALxIRTraverser* )
{
    os << program->getLine() << ": " << program->getHeaderString() << endl;
    return true;
}
bool GALxOutputTraverser::visitVP1Option(bool , VP1IROption* option, GALxIRTraverser* )
{
    os << option->getLine() << ": " << "ARBVp 1.0 Option: "<< option->getOptionString() << endl;
    return true;
}

bool GALxOutputTraverser::visitFP1Option(bool , FP1IROption* option, GALxIRTraverser* )
{
    os << option->getLine() << ": " << "ARBFp 1.0 Option: "<< option->getOptionString() << endl;
    return true;
}

bool GALxOutputTraverser::visitStatement(bool , IRStatement* statement, GALxIRTraverser* )
{
    os << statement->getLine() << ": ";
    return true;
}
bool GALxOutputTraverser::visitInstruction(bool preVisitAction, IRInstruction* instruction, GALxIRTraverser* it)
{
    visitStatement(preVisitAction, instruction, it);
    os << "Instruction: " << instruction->getOpcode();
    if (instruction->getIsFixedPoint())
    {
        os << " (Fixed Point)" << endl;
    }
    else
    {
        os << endl;
    }
    return true;
}

bool GALxOutputTraverser::visitSampleInstruction(bool preVisitAction, IRSampleInstruction *sinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction, sinstr,it);
    depthProcess();
    os << " Texture Image Unit: " << sinstr->getTextureImageUnit();
    os << " Texture Target: " << sinstr->getGLTextureTarget() << endl;
    return true;
}

bool GALxOutputTraverser::visitKillInstruction(bool preVisitAction, IRKillInstruction *kinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction, kinstr, it);
    depthProcess();
    if (kinstr->getIsKillSampleInstr())
    {
        os << " Kill Sample: " << kinstr->getKillSample() << endl;
    }
    return true;
}

bool GALxOutputTraverser::visitZExportInstruction(bool preVisitAction, IRZExportInstruction *zxpinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction, zxpinstr, it);
    depthProcess();
    if (zxpinstr->getIsExpSampleInstr())
    {
        os << " Export Sample: " << zxpinstr->getExportSample() << endl;
    }
    return true;
}

bool GALxOutputTraverser::visitCHSInstruction(bool preVisitAction, IRCHSInstruction *chsinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction, chsinstr, it);
    return true;
}

bool GALxOutputTraverser::visitSwizzleComponents(bool , IRSwizzleComponents* swzcomps, GALxIRTraverser* )
{
    os << "Swizzle: ";
    for (unsigned int i=0;i<4;i++)
    {
        if (swzcomps->getNegFlag(i)) os << "-";
        switch (swzcomps->getComponent(i))
        {
            case IRSwizzleComponents::ZERO: os << "0"; break;
            case IRSwizzleComponents::ONE:  os << "1"; break;
            case IRSwizzleComponents::X:       os << "X"; break;
            case IRSwizzleComponents::Y:       os << "Y"; break;
            case IRSwizzleComponents::Z:       os << "Z"; break;
            case IRSwizzleComponents::W:       os << "W"; break;
        }
        if (i<3) os << ",";
    }

    os << endl;
    return true;
}

bool GALxOutputTraverser::visitSwizzleInstruction(bool preVisitAction, IRSwizzleInstruction *swinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction,swinstr,it);
    return true;
}

bool GALxOutputTraverser::visitDstOperand(bool , IRDstOperand *dstop, GALxIRTraverser * )
{
    os << "DstOp: ";
    
    if (dstop->getIsVertexResultRegister() || dstop->getIsFragmentResultRegister()) os << "result";
    
    os << dstop->getDestination();
    
    if (dstop->getWriteMask().compare("xyzw"))
        os << " WriteMask: " << dstop->getWriteMask();
    
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitArrayAddressing(bool , IRArrayAddressing *arrayaddr, GALxIRTraverser* )
{
    os << "Array Access:";
    if (!arrayaddr->getIsRelativeAddress())
    {
        os << " Absolute Offset: " << arrayaddr->getArrayAddressOffset() << endl;
    }
    else // relative addressing
    {
        os << " Address Register: " << arrayaddr->getRelativeAddressReg();
        os << " Component: " << arrayaddr->getRelativeAddressComp();
        os << " Relative Offset: " << arrayaddr->getRelativeAddressOffset();
    }
    return true;
}

bool GALxOutputTraverser::visitSrcOperand(bool , IRSrcOperand *srcop, GALxIRTraverser *)
{
    os << "SrcOp: ";

    if (srcop->getIsFragmentRegister()) os << "fragment";

    if (srcop->getIsVertexRegister()) os << "vertex";

    os << srcop->getSource();

    if (srcop->getSwizzleMask().compare("xyzw"))
        os << " SwizzleMask: " << srcop->getSwizzleMask();

    if (srcop->getNegateFlag()) os << " Negated(-)";    

    os << endl;

    return true;
}

bool GALxOutputTraverser::visitNamingStatement(bool preVisitAction, IRNamingStatement* namingstmnt, GALxIRTraverser* it)
{
    visitStatement(preVisitAction,namingstmnt,it);
    os << "Naming Statement: ";
    return true;
}

bool GALxOutputTraverser::visitALIASStatement(bool preVisitAction, IRALIASStatement* aliasstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,aliasstmnt,it);
    os << "Alias: " << aliasstmnt->getName() << " = " << aliasstmnt->getAlias() << endl;
    return true;    
}

bool GALxOutputTraverser::visitTEMPStatement(bool preVisitAction, IRTEMPStatement* tempstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,tempstmnt,it);
    os << "Temp: " << tempstmnt->getName();
    list<string>::iterator iter = tempstmnt->getOtherTemporaries()->begin();
        
    while (iter != tempstmnt->getOtherTemporaries()->end()) {
        os << ", " << (*iter);
        iter++;
    }
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitADDRESSStatement(bool preVisitAction,IRADDRESSStatement* addrstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,addrstmnt,it);
    os << "Addr: " << addrstmnt->getName();

    list<string>::iterator iter = addrstmnt->getOtherAddressRegisters()->begin();
        
    while (iter != addrstmnt->getOtherAddressRegisters()->end()) {
        os << ", " << (*iter);
        iter++;
    }
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitVP1ATTRIBStatement(bool preVisitAction, VP1IRATTRIBStatement* attribstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,attribstmnt,it);
    os << "Vertex Input Attrib: " << attribstmnt->getName() << " = vertex";
    os << attribstmnt->getInputAttribute();
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitFP1ATTRIBStatement(bool preVisitAction, FP1IRATTRIBStatement* attribstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,attribstmnt,it);
    os << "Fragment Input Attrib: " << attribstmnt->getName() << " = fragment";
    os << attribstmnt->getInputAttribute();
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitVP1OUTPUTStatement(bool preVisitAction, VP1IROUTPUTStatement *outputstmnt, GALxIRTraverser*it)
{
    visitNamingStatement(preVisitAction,outputstmnt,it);
    os << "Vertex Output Attrib: " << outputstmnt->getName() << " = result";
    os << outputstmnt->getOutputAttribute();
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitFP1OUTPUTStatement(bool preVisitAction, FP1IROUTPUTStatement *outputstmnt, GALxIRTraverser*it)
{
    visitNamingStatement(preVisitAction,outputstmnt,it);
    os << "Fragment Output Attrib: " << outputstmnt->getName() << " = result";
    os << outputstmnt->getOutputAttribute();
    os << endl;
    return true;
}


bool GALxOutputTraverser::visitPARAMStatement(bool preVisitAction, IRPARAMStatement* paramstmnt, GALxIRTraverser* it)
{
    visitNamingStatement(preVisitAction,paramstmnt,it);
    os << "Param: " << paramstmnt->getName();
    if (paramstmnt->getIsMultipleBindings()) os << "[";
    if (paramstmnt->getSize() >= 0)
    {
        os << paramstmnt->getSize();
    }
    if (paramstmnt->getIsMultipleBindings()) os << "]";
    os << " = ";
    return true;
}

bool GALxOutputTraverser::visitParamBinding(bool , IRParamBinding* parambind, GALxIRTraverser* )
{
    if (parambind->getIsImplicitBinding()) os << "Implicit ";
    return true;
}

bool GALxOutputTraverser::visitLocalEnvBinding(bool preVisitAction, IRLocalEnvBinding* localenvbind, GALxIRTraverser* it)
{
    visitParamBinding(preVisitAction,localenvbind,it);
    switch(localenvbind->getType())
    {
        case IRLocalEnvBinding::LOCALPARAM: os << "Local "; break;
        case IRLocalEnvBinding::ENVPARAM: os << "Environment "; break;
    }
    os << "Parameter Binding [" << localenvbind->getMinIndex() << "..";
    os << localenvbind->getMaxIndex() << "]" << endl;
    return true;
}

bool GALxOutputTraverser::visitConstantBinding(bool preVisitAction, IRConstantBinding* constbind, GALxIRTraverser* it)
{
    visitParamBinding(preVisitAction,constbind,it);
    os << "Constant: ";
    if (constbind->getIsScalarConst())
    {
        os << constbind->getValue1();
    }
    else // Vector constant 
    {
        os << "{ " << constbind->getValue1() << ", " << constbind->getValue2();
        os << ", " << constbind->getValue3() << ", " << constbind->getValue4() << " }";
    }
    os << endl;
    return true;
}

bool GALxOutputTraverser::visitStateBinding(bool preVisitAction, IRStateBinding *statebind, GALxIRTraverser* it)
{
    visitParamBinding(preVisitAction,statebind,it);
    os << "State: " << statebind->getState() << endl;
    return true;
}
