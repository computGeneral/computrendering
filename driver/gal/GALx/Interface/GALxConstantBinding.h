/**************************************************************************
 *
 */

#ifndef GALx_CONSTANT_BINDING_H
    #define GALx_CONSTANT_BINDING_H

#include "GALxGlobalTypeDefinitions.h"
#include "GALxStoredFPItemID.h"
#include "GALxFixedPipelineState.h"
#include <vector>

namespace libGAL
{

/**
 * The GALxConstantBinding interface allows defining the assignation/attachment 
 * of a function of several Fixed Pipeline state values and an additional direct 
 * source value to a program constant.
 *
 * This function is specified using GALxBindingFunction subclass objects.
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @version 0.8
 * @date 03/16/2007
 */

/**
 * The binding constant targets
 */
enum GALx_BINDING_TARGET
{
    GALx_BINDING_TARGET_LOCAL,            ///< Bind a constant accesed through an "ARB" local parameter in the program
    GALx_BINDING_TARGET_ENVIRONMENT,    ///< Bind a constant accesed through an "ARB" environment parameter in the program
    GALx_BINDING_TARGET_FINAL,            ///< Bind a constant to the final position at the internal parameters bank.
};


/**
 * The GALxBindingFunction interface
 *
 * Defines the interface for the constant binding function.
 */
class GALxBindingFunction
{
public:

    /**
     * The function being called when the constant binding is resolved.
     *
     * @retval    constant        The binded output constant.
     * @param    vState            The vector of requested Fixed Pipeline states
     * @param    directSource    The optional/complementary direct source value
     */
    virtual void function(GALxFloatVector4& constant, 
                          std::vector<GALxFloatVector4> vState,
                          const GALxFloatVector4& directSource) const = 0;
};

/**
 * The GALxStateCopyBindFunction constant binding function
 *
 * This function just copies the first element of the request state
 * vector to the constant.
 *
 * @code
 *  // The function is the following:
 *    virtual void function(GALxFloatVector4& constant, 
 *                          std::vector<GALxFloatVector4> vState,
 *                          const GALxFloatVector4& directSource) const
 *    {
 *        constant = vState[0];
 *    };    
 * @endcode
 */
class GALxStateCopyBindFunction: public GALxBindingFunction
{
public:
    virtual void function(GALxFloatVector4& constant, 
                          std::vector<GALxFloatVector4> vState,
                          const GALxFloatVector4&) const
    {
        constant = vState[0];
    };
};

/**
 * The GALxDirectSrcCopyBindFunction constant binding function.
 *
 * This function just copies the direct source to the the constant.
 *
 * @code
 *  // The function is the following:
 *    virtual void function(GALxFloatVector4& constant, 
 *                          std::vector<GALxFloatVector4> vState,
 *                          const GALxFloatVector4& directSource) const
 *    {
 *        constant = directSource;
 *    };    
 * @endcode
 */
class GALxDirectSrcCopyBindFunction: public GALxBindingFunction
{
public:
    virtual void function(GALxFloatVector4& constant, 
                          std::vector<GALxFloatVector4> vState,
                          const GALxFloatVector4& directSource) const
    {
        constant = directSource;
    };
};

/**
 * The GALxConstantBinding interface.
 *
 * The GALxBindingFunction interface tells the way of computing the final 
 * constant value using as a source either state contents and/or a direct source value. 
 * This GALxBindingFunction and, in addition, the vector of requested state ids and 
 * the direct source value are passed to the GALx API construction function 
 * (GALxCreateConstantBinding()) that returns the corresponding GALxConstantBinding 
 * object.
 *
 * A GALxConstantBinding can be resolved using its resolveConstant() public
 * method or when passing it in the constant binding list to the GALxResolveProgram()
 * function, once the internal Fixed Pipeline State has been properly set up.
 *
 * To match the constant refered in the program with the defined in the 
 * GALxConstantBinding object, in addition to the constant index, the target stands
 * for the parameter type. In the ARB program specification, the constant parameters 
 * can be accessed as 'local parameters' (only seen by the program) or as 
 * 'environment parameters' (seen by all the programs attached to the same target). 
 * The GALx_BINDING_TARGET enumeration allows specifying either of these targets to 
 * match the binding.
 *
 * In addition, the GALx_BINDING_TARGET_FINAL allows defining directly the constant position 
 * in the final resolved list of constants. This target type should be used carefully because
 * the program resolver can overwrite the final constant positions due to the
 * program compilation and final constant position assignments. In fact, the use of this target
 * doesn´t make sense for "conventional" constant bindings made by the interface users and 
 * the purpose is to be used internally by the constant resolving engine.
 *
 * @note The vector of resolved states passed to the binding function can contain either
 *       single, vector and matrix states. Depending on the type each state will fill
 *       different consecutive vector positions, The convention is as follows:
 *
 * @code
 * State type specified  |  Vector positions
 * -----------------------------------------------
 *       Single          |  [0] (s,0,0,1)
 * -----------------------------------------------
 *       Vector3         |  [0] (v,v,v,1)
 * -----------------------------------------------
 *       Vector4         |  [0] (v,v,v,v)
 * -----------------------------------------------
 *       Matrix          |  [0] (m,m,m,m) (row 0)
 *                       |  [1] (m,m,m,m) (row 1)
 *                       |  [2] (m,m,m,m) (row 2)
 *                       |  [3] (m,m,m,m) (row 3)
 * -----------------------------------------------
 * @endcode
 * @endnote
 *
 * @code 
 * // EXAMPLE SAMPLES:
 * // USE1: Avoid repeating the same computation for all the program
 * //       executions. The normalization of the light direction vector
 * //       can be performed outside the shader execution.
 *
 * // This sample code binds the normalized direction to
 * // the local parameter 41.
 *
 * class NormalizeBindFunction: public GALxBindingFunction
 * {
 * public:
 *      virtual void function(GALxFloatVector4& constant, 
 *                            std::vector<GALxFloatVector4> vState, 
 *                            const GALxFloatVector4& directSource) const
 *      {
 *           constant = libGAL::_normalize(vState[0]);
 *      };
 * };
 *
 *
 * // Create the constant binding object
 * GALxConstantBinding* cb = GALxCreateConstantBinding(GALx_BINDING_TARGET_LOCAL, 
 *                                                     41, GALx_LIGHT_DIRECTION, 
 *                                                     new NormalizeBindFunction);
 *
 * 
 * // USE2: Specify the value for the environment parameter 5
 * //       accessed by the program.
 *
 * ProgramTarget pt = GET_CURRENT_VERTEX_SHADER.getTarget();
 *
 * // Create the constant binding object
 * GALxConstantBinding* cb = GALxCreateConstantBinding(GALx_BINDING_TARGET_ENVIRONMENT, 
 *                                                     5, 
 *                                                     std::vector<GALx_STORED_FP_ITEM_ID>(), // Passing an empty list
 *                                                     new GALxDirectSourceBindFunction,
 *                                                     pt.getEnvironmentParameters()[5]);
 *
 *
 *
 * @endcode
 */

class GALxConstantBinding
{
public:
    
    /**
     * The resolveConstant() method executes the binding function and resolves
     * the constant value using the current FP state.
     *
     * @param    fpStateImp The GALxFixedPipelineImp object.
     * @retval    constant   The resolved constant value.        
     */
    virtual void resolveConstant(const GALxFixedPipelineState* fpState, GALxFloatVector4& constant) const = 0;

    /**
     * Returns the constant binding index
     *
     * @returns The constant binding index
     */
    virtual gal_uint getConstantIndex() const = 0;

    /**
     * Prints the GALxConstantBinding object internal data state.
     *
     * @retval    os    The output stream to print to.
     */
    virtual void print(std::ostream& os) const = 0;
};

/**
 * Constant binding list definition
 */
typedef std::list<GALxConstantBinding*> GALxConstantBindingList;

} // namespace libGAL

#endif // GALx_CONSTANT_BINDING_H
