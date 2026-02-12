/**************************************************************************
 *
 */

#ifndef SHADER_OPTIMIZER_H
    #define SHADER_OPTIMIZER_H

#include "GALVector.h"
#include <vector>
#include <list>
#include "ShaderInstr.h"
#include "cmShaderArchParam.h"
#include "OptimizationDataStructures.h"

namespace libGAL
{

/**
 * The libGAL_opt namespace groups all the classes and functions related with the
 * GAL Shader Programs code optimization.
 *
 * The shader code optimizer is the same for both vertex and fragment programs and the
 * ouput analysis results are returned in a general proper structure for both types.
 */
namespace libGAL_opt
{
#define MAX(x,y) ((x) > (y))? (x) : (y)
/**
 * The maximum number of attributes for both vertex shader and fragment shader
 * input and output registers.
 */
static const gal_uint MAX_SHADER_ATTRIBUTES = MAX(arch::MAX_MICROFRAGMENT_ATTRIBUTES, 
                                                  MAX(arch::MAX_VERTEX_ATTRIBUTES, arch::MAX_FRAGMENT_ATTRIBUTES));

class ShaderOptimizer;

/**
 * Defines a single shader code optimization step.
 *
 * An optimization step can update the code previously optimized by others optimization steps
 * or can make a simply analysis on the code and return the analysis result.
 *
 * To access to the shader code, the constructor stores a pointer to the ShaderOptimizer
 * that stores the code being touched by all the optimization steps.
 */
class OptimizationStep
{
protected:

    ShaderOptimizer* _shOptimizer;

public:

    OptimizationStep(ShaderOptimizer* shOptimizer);

    virtual void optimize() = 0;
};

/**
 * The CG1 shader instruction latency table
 *
 * @note Used by the Static instruction scheduler optimization step.
 *
 */
/*static const gal_uint CG1LatencyTable[] =
{
//  NOP, ADD, ARA, ARL, ARR, BRA, CAL, COS, (Opcodes 00h - 07h)
     1,   3,   3,   3,   3,   4,   4,   9,
//  DP3, DP4, DPH, DST, EX2, EXP, FLR, FRC, (Opcodes 08h - 0Fh)
     3,   3,   3,   4,   5,   9,   3,   3,
//  LG2, LIT, LOG, MAD, MAX, MIN, MOV, MUL, (Opcodes 10h - 17h)
     5,   4,   9,   3,   3,   3,   3,   3,
//  RCC, RCP, RET, RSQ, SEQ, SFL, SGE, SGT, (Opcodes 18h - 1Fh)
     3,   5,   4,   5,   3,   3,   3,   3,
//  SIN, SLE, SLT, SNE, SSG, STR, TEX, TXB, (Opcodes 20h - 27h)
     9,   3,   3,   3,   3,   3,   1,   1,
//  TXP, KIL, CMP, LDA, END, LAST_OPC.      (Opcodes 28h - 2Dh)
     1,   3,   3,   1,   1,   0
};

static const gal_uint LATENCY_TABLE_SIZE = sizeof(CG1LatencyTable);
*/

/**
 * Thes SHADER_ARCH_PARAMS struct defines the Shader Unit
 * architectural parameters and/or capabilities.
 */
struct SHADER_ARCH_PARAMS
{
    gal_uint nWay;          ///< The issue wide of the fetch unit
    gal_uint temporaries;   ///< The maximum number of temporary registers
    gal_uint outputRegs;    ///< The maximum number of output registers
    gal_uint addrRegs;      ///< The maximum number of address registers
    gal_uint predRegs;      ///< The number of supported predicate registers.
    
    arch::ShaderArchParam *shArchParams;  ///< Pointer to the singleton storing shader architecture parameters.
    
//    gal_uint latencyTable[LATENCY_TABLE_SIZE]; ///< The instruction opcodes latencies

//    SHADER_ARCH_PARAMS(const gal_uint latencyTable[] = CG1LatencyTable);
    SHADER_ARCH_PARAMS();
};

/**
 * The OPTIMIZATION_OUTPUT_INFO struct contains the
 * optimization results apart from the code optimization itself.
 */
struct OPTIMIZATION_OUTPUT_INFO
{
    gal_uint maxAliveTemps;            ///< Maximum number of temporal registers that are
                                      // alive (the value will be further needed) in
                                      // a certain point of the code.
    gal_bool inputsRead[MAX_SHADER_ATTRIBUTES];     ///< Describes if the shader reads or not the input register position
    gal_bool outputsWritten[MAX_SHADER_ATTRIBUTES]; ///< Describes if the shader writes or not the output register positions
};

/**
 * The shader optimizer class performs all the optimizations on the specified
 * shader code.
 *
 * The shader optimizer constructor builds the list of optimization steps that
 * will be applied on the code.
 */
class ShaderOptimizer
{
public:

    /**
     * ShaderOptimizer constructor.
     *
     * @param shArchP The SHADER_ARCH_PARAMS struct that will guide the optimizations
     */
    ShaderOptimizer(const SHADER_ARCH_PARAMS& shArchP);

    /**
     * Sets the unOptimized shader code.
     *
     * @param code            The unoptimized code.
     * @param sizeInBytes    The code length in bytes.
     */
    void setCode(const gal_ubyte* code, gal_uint sizeInBytes);

    /**
     * Sets the shader unoptimized constant bank contents
     *
     * @param constants        The vector of 4 float constants
     */
    void setConstants(const std::vector<GALVector<gal_float,4> >& constants);

    /**
     * Applies the code optimizations.
     */
    void optimize();

    /**
     * Returns the optimized shader code.
     *
     * @retval sizeInBytes    The size in bytes of the resulting optimized code.
     * @returns                The optimized code.
     */
    const gal_ubyte* getCode(gal_uint& sizeInBytes) const;

    /**
     * Returns the optimized shader constants bank.
     *
     * @returns The optimized constant bank.
     */
    const std::vector<GALVector<gal_float,4> >& getConstants() const;

    /**
     * Returns additional optimization results.
     *
     * @retval outputInfo    The OPTIMIZATION_OUTPUT_INFO struct with additional results.
     */
    void getOptOutputInfo(OPTIMIZATION_OUTPUT_INFO& optOutInfo) const;

    /**
     * ShaderOptimizer destructor.
     *
     * Destroys and releases memory used for intermediate optimization structures.
     */
    ~ShaderOptimizer();

private:

    gal_ubyte* _inputCode;
    gal_ubyte* _outputCode;
    gal_uint _inputSizeInBytes, _outputSizeInBytes;

    std::vector<GALVector<gal_float,4> > _inputConstants, _outputConstants;

    std::vector<InstructionInfo*> _inputInstrInfoVect;

    gal_bool _reOptimize;

    const SHADER_ARCH_PARAMS _shArchParams;
    OPTIMIZATION_OUTPUT_INFO _optOutInf;

    std::list<OptimizationStep*> _optimizationSteps;

    friend class MaxLiveTempsAnalysis;
    friend class RegisterUsageAnalysis;
    friend class StaticInstructionScheduling;

};

} // namespace libGAL_opt

} // namespace libGAL

#endif // SHADER_OPTIMIZER_H
