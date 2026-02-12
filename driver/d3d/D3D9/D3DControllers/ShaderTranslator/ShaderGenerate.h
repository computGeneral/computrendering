#ifndef SHADINGTYPES_H_INCLUDED
#define SHADINGTYPES_H_INCLUDED

/// Utilities to generate shader instructions

#include "ShaderInstruction.h"
#include "Types.h"

/// Represents a shader instruction operand
struct Operand
{
    GPURegisterId registerId;
    bool negate;
    bool absolute;
    bool addressing;
    arch::SwizzleMode swizzle;    

    //  Default values
    Operand() : negate(false), absolute(false), addressing(false), swizzle(arch::XYZW) {}
};

/// Represents a shader instruction result
struct Result
{
    GPURegisterId registerId;
    bool saturate;
    arch::MaskMode maskMode;
    
    //  Default values
    Result() : saturate(false), maskMode(arch::mXYZW) {}
};


/// Represents a relative mode access
struct RelativeMode
{
    bool enabled;
    U32 registerN;
    U32 component;
    S16 offset;

    //  Default values
    RelativeMode() : enabled(false), registerN(0), component(0), offset(0) {}
};

//  Represents a shader instruction predication parameters.
struct PredicationInfo
{
    bool predicated;
    bool negatePredicate;
    U32 predicateRegister;
    
    //  Default values
    PredicationInfo() : predicated(false), negatePredicate(false), predicateRegister(0) {}
};

/// Generates ShaderInstruction objects with a friendly interface
class  ShaderInstructionBuilder
{
public:
    
    ShaderInstructionBuilder();
    
    void resetParameters();

    void setOpcode(arch::ShOpcode _opc);

    void setOperand(U32 i, Operand _operand);
    
    void setResult(Result _result);

    void setPredication(PredicationInfo _predication);
    
    void setRelativeMode(RelativeMode _relMode);

    void setLastInstr(bool _lastInstr);

    arch::ShaderInstruction *buildInstruction();
    
private:

    arch::ShOpcode opc;
    Operand operand[3];
    Result result;
    PredicationInfo predication;
    RelativeMode relMode;
    bool lastInstr;                                                 //  Marks as the last instruction in a kernel.  
};

/** gpu shaders must end with 8 NOP instructions,
    this function generates this serie **/
list<arch::ShaderInstruction*> generate_ending_nops();

//* generate a mov instruction, not surprisingly *
arch::ShaderInstruction* generate_mov(GPURegisterId dest, GPURegisterId src);


#endif // SHADINGTYPES_H_INCLUDED
