/**************************************************************************
 *
 */

#include "GALxSemanticTraverser.h"
#include "GALxImplementationLimits.h"
#include "GALxCodeGenTraverser.h"
#include "GPUReg.h"

using namespace libGAL::GenerationCode;
using namespace libGAL;
using namespace std;
using namespace cg1gpu;


bool GALxSemanticTraverser ::swizzleMaskCheck(const char *smask)
{
    unsigned int len = (unsigned int)strlen(smask);
    if (len != 4 && len != 1) return false;
    for (unsigned int i=0;i<len;i++)
    {
        if (smask[i]!='x' && smask[i]!='y' && smask[i]!='z' && smask[i]!='w' &&
            smask[i]!='r' && smask[i]!='g' && smask[i]!='b' && smask[i]!='a')
            return false;
    }
    return true;
}

bool GALxSemanticTraverser ::writeMaskCheck(const char *wmask)
{
    unsigned int len = (unsigned int)strlen(wmask);
    int actual_weight = 0;
    if (len > 4) return false;
    for (unsigned int i=0;i<len;i++)
    {
        switch(wmask[i])
        {
            case 'x': if (actual_weight < 1) actual_weight = 1; else return false; break;
            case 'y': if (actual_weight < 2) actual_weight = 2; else return false; break;
            case 'z': if (actual_weight < 3) actual_weight = 3; else return false; break;
            case 'w': if (actual_weight < 4) actual_weight = 4; else return false; break;
            case 'r': if (actual_weight < 1) actual_weight = 1; else return false; break;
            case 'g': if (actual_weight < 2) actual_weight = 2; else return false; break;
            case 'b': if (actual_weight < 3) actual_weight = 3; else return false; break;
            case 'a': if (actual_weight < 4) actual_weight = 4; else return false; break;
        }
    }
    return true;
}

IdentInfo *GALxSemanticTraverser ::recursiveSearchSymbol(std::string id, bool& found)
{
    bool foundRecursive;
    
    IdentInfo *idinfo = symtab.searchSymbol(id,foundRecursive);
    
    if (foundRecursive)
    {
        found = true;
        if (idinfo->type != ALIAS_REF)
        {
            return idinfo;  // Base case 1 of recursion. Symbol found 
        }
        else
        {
            return recursiveSearchSymbol(idinfo->alias_reference,found);
        }
    }
    else
    {
        found = false;  // Base case 2 of recursion. Symbol not found
        return 0;
    }
}

GALxSemanticTraverser ::GALxSemanticTraverser ()
:   semanticErrorInTraverse(false)
{
    inputRegisterRead.reset();
    outputRegisterWritten.reset();
    
    for (int i=0; i< MAX_TEXTURE_UNITS_ARB; i++)
        TextureImageUnitsTargets[i] = GL_NONE;

    for (int i=0; i< MAX_TEXTURE_UNITS_ARB; i++)
        textureUnitUsage [i] = 0;

    killInstructions = false;
}

GALxSemanticTraverser ::~GALxSemanticTraverser (void)
{
     std::list<IdentInfo*>::iterator it = identinfoCollector.begin();
     while ( it != identinfoCollector.end() )
     {
          delete (*it);
          it++;                  
     }
     identinfoCollector.clear();
}

GALxSemanticTraverser ::AttribMask GALxSemanticTraverser ::getAttribsWritten() const
{
    return outputRegisterWritten;
}

GALxSemanticTraverser ::AttribMask GALxSemanticTraverser ::getAttribsRead() const
{
    return inputRegisterRead;
}

/**
 * Visitor pattern implemented methods
 *
 */

bool GALxSemanticTraverser ::visitProgram(bool preVisitAction, IRProgram* program, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        // Visitor order setup for semantic traverser
        preVisit = true;
        postVisit = false;

        // Initialization of error string and statistics
        errorOut << "Semantical Errors String:" << endl;
        numberOfStatements = 0;
        numberOfInstructions = 0;
        numberOfParamRegs = 0;
        numberOfTempRegs = 0;
        numberOfAttribs = 0;
        numberOfAddrRegs = 0;

        for(int tu = 0; tu < MAX_TEXTURE_UNITS_ARB; tu++)
            textureUnitUsage[tu] = 0;

        killInstructions = false;

        symtab.pushBlock();
        
        return true;
    }
    else // Post Visit
    {
        symtab.popBlock();
        return true;
    }
}

bool GALxSemanticTraverser ::visitVP1Option(bool preVisitAction, VP1IROption *option , GALxIRTraverser*)
{
    if (preVisitAction)
    {
        if (!option->getOptionString().compare("ARB_position_invariant")) 
        {
            outputRegisterWritten.set(POSITION_ATTRIBUTE);
            return true;
        }

        errorOut << "Line: " << option->getLine();
        errorOut << " Option parameter " << option->getOptionString();
        errorOut << " not supported. Remember that language is case sensitive" << endl;

        semanticErrorInTraverse = true;

        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitFP1Option(bool preVisitAction, FP1IROption *option , GALxIRTraverser*)
{
    if (preVisitAction)
    {
        if (!option->getOptionString().compare("ARB_fog_exp")) return true;
        else if (!option->getOptionString().compare("ARB_fog_exp2")) return true;
        else if (!option->getOptionString().compare("ARB_fog_linear")) return true;
        else if (!option->getOptionString().compare("ARB_precision_hint_fastest")) return true;
        else if (!option->getOptionString().compare("ARB_precision_hint_nicest")) return true;
    
        errorOut << "Line: " << option->getLine();
        errorOut << " Option parameter " << option->getOptionString();
        errorOut << " not supported. Remember that language is case sensitive" << endl;
        semanticErrorInTraverse = true;

        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitStatement(bool preVisitAction, IRStatement*, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        numberOfStatements++;
        return true;
    }
    else
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitInstruction(bool preVisitAction, IRInstruction*, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        numberOfInstructions++;
        return true;
    }
    else
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitSampleInstruction(bool preVisitAction, IRSampleInstruction* sample, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction,sample,it);
    
    GLenum target;
    
    if (!sample->getGLTextureTarget().compare("1D"))
        target = GL_TEXTURE_1D;
        
    else if (!sample->getGLTextureTarget().compare("2D"))
        target = GL_TEXTURE_2D;
        
    else if (!sample->getGLTextureTarget().compare("3D"))
        target = GL_TEXTURE_3D;
    
    else if (!sample->getGLTextureTarget().compare("CUBE"))
        target = GL_TEXTURE_CUBE_MAP;
        
    else if (!sample->getGLTextureTarget().compare("RECT"))
        target = GL_TEXTURE_RECTANGLE;
    
    if ( (TextureImageUnitsTargets[sample->getTextureImageUnit()] != GL_NONE) && 
         (TextureImageUnitsTargets[sample->getTextureImageUnit()] != target) )
    {
        errorOut << "Line: " << sample->getLine();
        errorOut << " Programs are not allowed to reference different texture targets in the same texture unit" << endl;

        semanticErrorInTraverse = true;
    }
    else
        TextureImageUnitsTargets[sample->getTextureImageUnit()] = target;
    
    return true;
}

bool GALxSemanticTraverser ::visitKillInstruction(bool preVisitAction, IRKillInstruction* kinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction,kinstr,it);

    if (kinstr->getIsKillSampleInstr() && ((kinstr->getKillSample() == 0) || kinstr->getKillSample() > MAX_MSAA_SAMPLES))
    {
        errorOut << "Line: " << kinstr->getLine();
        errorOut << " Incorrect sample specified in KLS instruction" << endl;
        
        semanticErrorInTraverse = true;

        return true;
    }
    else
        killInstructions = true;

    return true;
}

bool GALxSemanticTraverser ::visitZExportInstruction(bool preVisitAction, IRZExportInstruction* zxpinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction,zxpinstr,it);

    if (zxpinstr->getIsExpSampleInstr() && ((zxpinstr->getExportSample() == 0) || zxpinstr->getExportSample() > MAX_MSAA_SAMPLES))
    {
        errorOut << "Line: " << zxpinstr->getLine();
        errorOut << " Incorrect export sample in ZXS instruction" << endl;
        
        semanticErrorInTraverse = true;

        return true;
    }

    return true;
}

bool GALxSemanticTraverser::visitCHSInstruction(bool preVisitAction, IRCHSInstruction* chsinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction, chsinstr, it);
    return true;
}

bool GALxSemanticTraverser ::visitSwizzleComponents(bool preVisitAction, IRSwizzleComponents*, GALxIRTraverser*)
{
    return true;
}

bool GALxSemanticTraverser ::visitSwizzleInstruction(bool preVisitAction, IRSwizzleInstruction* swzinstr, GALxIRTraverser* it)
{
    visitInstruction(preVisitAction,swzinstr,it);
    return true;
}

bool GALxSemanticTraverser ::visitDstOperand(bool preVisitAction, IRDstOperand *dstop, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        if (dstop->getIsVertexResultRegister())
        {
            outputRegisterWritten.set(GALxCodeGenTraverser::vertexResultAttribBindingMaptoRegister(dstop->getDestination()));
        }
        else if (dstop->getIsFragmentResultRegister()) 
        {
            outputRegisterWritten.set(GALxCodeGenTraverser::fragmentResultAttribBindingMaptoRegister(dstop->getDestination()));
        }
        else // It has to be a temporary register. Possibly aliased.
        {
            bool found;
            IdentInfo *idinfo = recursiveSearchSymbol(dstop->getDestination(),found);
            if (!found)
            {   // Undefined identifier
                errorOut << "Line: " << dstop->getLine();
                errorOut << " Identifier " << dstop->getDestination() << " not declared previously" << endl;
                semanticErrorInTraverse = true;
            }
            else
            {   // Defined register identifier that cannot be written.
                if (idinfo->type == VERTEX_ATTRIB_REG ||
                    idinfo->type == FRAGMENT_ATTRIB_REG ||
                    idinfo->type == PROGRAM_PARAMETER)
                {
                    errorOut << "Line: " << dstop->getLine();
                    errorOut << " Program parameter variables and fragment or vertex attributes"
                                " variables can never be modified" << endl;
                    semanticErrorInTraverse = true;
                }
                else if (idinfo->type == VERTEX_RESULT_REG || 
                         idinfo->type == FRAGMENT_RESULT_REG)
                {
                    outputRegisterWritten.set(idinfo->regId);
                }
            }
        }
        
        // Check Swizzle mask
        if (!writeMaskCheck(dstop->getWriteMask().c_str()))
        {
            errorOut << "Line: " << dstop->getLine();
            errorOut << " Incorrect write mask \"" << dstop->getWriteMask() << "\" in result register" << endl;
            semanticErrorInTraverse = true;
        }
    
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitArrayAddressing(bool preVisitAction,IRArrayAddressing* arrayaddr, GALxIRTraverser *it)
{
    if (preVisitAction)
    {
        if (arrayaddr->getIsRelativeAddress())
        {
            bool found;
            IdentInfo *idinfo = recursiveSearchSymbol(arrayaddr->getRelativeAddressReg(),found);
            if (!found)
            {
                errorOut << "Line: " << arrayaddr->getLine();
                errorOut << " Identifier " << arrayaddr->getRelativeAddressReg() << " not declared previously" << endl;
                semanticErrorInTraverse = true;
            }
            else // identifier found
            {   
                if (idinfo->type != ADDRESS_REG)
                {   // Addressing with not an address register????
                    errorOut << "Line: " << arrayaddr->getLine();
                    errorOut << " Symbol " << arrayaddr->getRelativeAddressReg() << " is not an address register" << endl;
                    semanticErrorInTraverse = true;
                }
            }
            // Check swizzle scalar component 
            if (arrayaddr->getRelativeAddressComp().compare("x"))
            {
                errorOut << "Line: " << arrayaddr->getLine();
                errorOut << " No valid address register component specified " << arrayaddr->getRelativeAddressReg() << endl;
                semanticErrorInTraverse = true;
            }
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitSrcOperand(bool preVisitAction, IRSrcOperand* srcop, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        if (!srcop->getIsVertexRegister() && 
            !srcop->getIsFragmentRegister() &&
            !srcop->getIsParamBinding())
        {
            // It is just an identifier name. Search it 
            bool found;
            IdentInfo *idinfo = recursiveSearchSymbol(srcop->getSource(),found);
            if (!found)
            {
                errorOut << "Line: " << srcop->getLine();
                errorOut << " Identifier " << srcop->getSource() << " not declared previously" << endl;
                semanticErrorInTraverse = true;
            }
            // Identifier is an output register 
            else if (idinfo->type == VERTEX_RESULT_REG || idinfo->type == FRAGMENT_RESULT_REG) 
            {
                errorOut << "Line: " << srcop->getLine();
                errorOut << " Vertex or Fragment result variables can be never read" << endl;
                semanticErrorInTraverse = true;
            }
            // Identifier is not a program parameter and it is array accessed 
            else if (idinfo->type != PROGRAM_PARAMETER && srcop->getIsArrayAddressing())
            {
                // Identifier is not a program parameter
                errorOut << "Line: " << srcop->getLine();
                errorOut << " Identifier " << srcop->getSource();
                errorOut << " is not a program parameter array" << endl;
                semanticErrorInTraverse = true;
            }
            // Identifier is a program parameter but is not a array and it is array accessed 
            else if (idinfo->type == PROGRAM_PARAMETER && !idinfo->parameter_array && srcop->getIsArrayAddressing())
            {
                errorOut << "Line: " << srcop->getLine();
                errorOut << " Identifier " << srcop->getSource();
                errorOut << " is not a program parameter array" << endl;
                semanticErrorInTraverse = true;
            }
            /* Identifier is a program parameter array but it�s not array accessed */
            else if (idinfo->type == PROGRAM_PARAMETER && idinfo->parameter_array && !srcop->getIsArrayAddressing())
            {
                // Identifier is a program parameter array but is not array accessed
                errorOut << "Line: " << srcop->getLine();
                errorOut << " Missing row number in parameter " << srcop->getSource();
                errorOut << " array access" << endl;
                semanticErrorInTraverse = true;
            }
            else // All correct
            {
                if (idinfo->type == VERTEX_ATTRIB_REG || 
                    idinfo->type == FRAGMENT_ATTRIB_REG)
                {
                    inputRegisterRead.set(idinfo->regId);
                }
            }    
            
        }
        
        if (srcop->getIsVertexRegister())
        {
            inputRegisterRead.set(GALxCodeGenTraverser::vertexAttributeBindingMapToRegister(srcop->getSource()));
        }
        else if (srcop->getIsFragmentRegister())
        {
            inputRegisterRead.set(GALxCodeGenTraverser::fragmentAttributeBindingMapToRegister(srcop->getSource()));
        }
        
        // Check Swizzle mask
        if (!swizzleMaskCheck(srcop->getSwizzleMask().c_str()))
        {
            errorOut << "Line: " << srcop->getLine();
            errorOut << " Incorrect swizzle mask \"" << srcop->getSwizzleMask()<< "\" in operand" << endl;
            semanticErrorInTraverse = true;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitNamingStatement(bool preVisitAction, IRNamingStatement* namestmnt, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        bool found;
        symtab.searchSymbol(namestmnt->getName(),found);
        if (found)
        {
            // Check not using an already declared identifier
            errorOut << "Line: " << namestmnt->getLine();
            errorOut << " Identifier " << namestmnt->getName() << " already declared" << endl;
            semanticErrorInTraverse = true;
            return false;
        }
        else
        {   // Insert in symbol table. The specialized naming statement will
            // rescue this struct and will fill it with specialized naming information.
            IdentInfo *idinfo = new IdentInfo();
            identinfoCollector.push_back(idinfo);
            symtab.insertSymbol(namestmnt->getName(),idinfo);
        }
        return true;
    }
    else // Post Visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitALIASStatement(bool preVisitAction, IRALIASStatement* aliasstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,aliasstmnt,it);
    if (preVisitAction)
    {
        bool found;
        IdentInfo *idinfo2 = recursiveSearchSymbol(aliasstmnt->getAlias(),found);
        if (!found)
        {
            errorOut << "Line: " << aliasstmnt->getLine();
            errorOut << " Identifier " << aliasstmnt->getAlias() << " not declared previously" << endl;
            semanticErrorInTraverse = true;
        }
        else // found a valid root identifier that is not an alias (If more than one alias chained). 
             // All is correct.
        {
            if (correctNameBinding) // Check if Name binding correct.
            {
                IdentInfo *idinfo = symtab.searchSymbol(aliasstmnt->getName());
                idinfo->type = ALIAS_REF;
                idinfo->alias_reference = aliasstmnt->getAlias();
            }
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitTEMPStatement(bool preVisitAction, IRTEMPStatement* tmpstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,tmpstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {   // Rescue Identifier info and set type
            IdentInfo* idinfo = symtab.searchSymbol(tmpstmnt->getName());
            idinfo->type = TEMP_REGISTER;
        }

        numberOfTempRegs += (unsigned int)tmpstmnt->getOtherTemporaries()->size() + 1;

        // Check the auxiliar list of temporaries declared

        list<string>* temporariesList = tmpstmnt->getOtherTemporaries();

        list<string>::iterator iter = temporariesList->begin();
        
        bool found;
        
        while(iter != temporariesList->end()) 
        {
            symtab.searchSymbol((*iter),found);
            if (found)
            {
                errorOut << "Line: " << tmpstmnt->getLine();
                errorOut << " Identifier " << (*iter) << "already declared" << endl;
                semanticErrorInTraverse = true;
            }       
            else // Put new identifier in Symbol Table
            {
                IdentInfo* idinfo = new IdentInfo();
                identinfoCollector.push_back(idinfo);
                idinfo->type = TEMP_REGISTER;
                symtab.insertSymbol((*iter),idinfo);
            }
            iter++;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitADDRESSStatement(bool preVisitAction, IRADDRESSStatement* addrstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,addrstmnt,it);
    
    if (preVisitAction)
    {
        // Rescue Identifier info and set type
        if (correctNameBinding)
        {
            IdentInfo* idinfo = symtab.searchSymbol(addrstmnt->getName());
            idinfo->type = ADDRESS_REG;
        }

        // Check auxiliar list of address registers declared
        
        list<string>* addressRegisterList = addrstmnt->getOtherAddressRegisters();

        list<string>::iterator iter = addressRegisterList->begin();
        
        bool found;

        while(iter != addressRegisterList->end()) 
        {
            symtab.searchSymbol((*iter),found);
            if (found)
            {
                errorOut << "Line: " << addrstmnt->getLine();
                errorOut << " Identifier " << (*iter) << "already declared" << endl;
                semanticErrorInTraverse = true;
            }   
            else // Put new identifier in Symbol Table
            {
                IdentInfo* idinfo = new IdentInfo();
                identinfoCollector.push_back(idinfo);
                idinfo->type = ADDRESS_REG;
                symtab.insertSymbol((*iter),idinfo);
            }
            iter++;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitVP1ATTRIBStatement(bool preVisitAction, VP1IRATTRIBStatement* attrstmnt, GALxIRTraverser* it)
{
    numberOfAttribs++;
    bool correctNameBinding = visitNamingStatement(preVisitAction,attrstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {
            IdentInfo *idinfo = symtab.searchSymbol(attrstmnt->getName());
            idinfo->regId = GALxCodeGenTraverser::vertexAttributeBindingMapToRegister(attrstmnt->getInputAttribute());
            idinfo->type = VERTEX_ATTRIB_REG;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitFP1ATTRIBStatement(bool preVisitAction, FP1IRATTRIBStatement* attrstmnt, GALxIRTraverser* it)
{
    numberOfAttribs++;
    bool correctNameBinding = visitNamingStatement(preVisitAction,attrstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {
            IdentInfo *idinfo = symtab.searchSymbol(attrstmnt->getName());
            idinfo->regId = GALxCodeGenTraverser::fragmentAttributeBindingMapToRegister(attrstmnt->getInputAttribute());
            idinfo->type = FRAGMENT_ATTRIB_REG;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitVP1OUTPUTStatement(bool preVisitAction, VP1IROUTPUTStatement* outputstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,outputstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {
            IdentInfo *idinfo = symtab.searchSymbol(outputstmnt->getName());
            idinfo->type = VERTEX_RESULT_REG;
            idinfo->regId = GALxCodeGenTraverser::vertexResultAttribBindingMaptoRegister(outputstmnt->getOutputAttribute());
        }
        return true;
    }
    else
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitFP1OUTPUTStatement(bool preVisitAction, FP1IROUTPUTStatement* outputstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,outputstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {
            IdentInfo *idinfo = symtab.searchSymbol(outputstmnt->getName());
            idinfo->type = FRAGMENT_RESULT_REG;
            idinfo->regId = GALxCodeGenTraverser::fragmentResultAttribBindingMaptoRegister(outputstmnt->getOutputAttribute());
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitPARAMStatement(bool preVisitAction, IRPARAMStatement* paramstmnt, GALxIRTraverser* it)
{
    bool correctNameBinding = visitNamingStatement(preVisitAction,paramstmnt,it);
    if (preVisitAction)
    {
        if (correctNameBinding)
        {
            IdentInfo* idinfo = symtab.searchSymbol(paramstmnt->getName());
            idinfo->type = PROGRAM_PARAMETER;
            idinfo->implicit_binding = false;
        
            if (paramstmnt->getIsMultipleBindings())
                idinfo->parameter_array = true;
            else
                idinfo->parameter_array = false;
        }

        if (paramstmnt->getIsMultipleBindings())    // Array vector parameter
        {
            // Check if specified size match with list size
            unsigned int totalVectorsBinded = 0;
        
            list<IRParamBinding*>* paramList =  paramstmnt->getParamBindings();
            list<IRParamBinding*>::iterator iter = paramList->begin();

            while(iter!= paramList->end())
            {
                totalVectorsBinded += (*iter)->getVectorsBinded();
                iter++;
            }

            if (paramstmnt->getSize() > 0) // An array size was specificated
            {
                if (totalVectorsBinded != paramstmnt->getSize())
                {
                    errorOut << "Line: " << paramstmnt->getLine();
                    errorOut << " The optional size array does not match the number of parameter bindings in the initialization list" << endl;
                    semanticErrorInTraverse = true;
                }
            }
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitParamBinding(bool preVisitAction, IRParamBinding*, GALxIRTraverser*)
{
    return true;
}

bool GALxSemanticTraverser ::visitLocalEnvBinding(bool preVisitAction, IRLocalEnvBinding* localenvbind, GALxIRTraverser*)
{
    if (preVisitAction)
    {
        if (localenvbind->getMinIndex() > localenvbind->getMaxIndex())
        {
            errorOut << "Line: " << localenvbind->getLine();
            errorOut << " Invalid range in multiple array parameter binding" << endl;
            semanticErrorInTraverse = true;
        }
        return true;
    }
    else // Post visit
    {
        return true;
    }
}

bool GALxSemanticTraverser ::visitConstantBinding(bool preVisitAction, IRConstantBinding*, GALxIRTraverser*)
{
    return true;
}

bool GALxSemanticTraverser ::visitStateBinding(bool preVisitAction, IRStateBinding*, GALxIRTraverser*)
{
    return true;
}
