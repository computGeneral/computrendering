/**************************************************************************
 *
 */

#ifndef INTERNAL_CONSTANT_BINDING_H
    #define INTERNAL_CONSTANT_BINDING_H

#include "GALxConstantBinding.h"

namespace libGAL
{

class InternalConstantBinding: public GALxConstantBinding
{
public:
    
    /**
     * The resolveConstant() method executes the binding function and resolves
     * the constant value using the current FP state.
     *
     * @param    fpStateImp The GALxFixedPipelineImp object.
     * @retval    constant   The resolved constant value.        
     */
    virtual void resolveConstant(const GALxFixedPipelineState* fpState, GALxFloatVector4& constant) const;

    /**
     * Returns the constant binding index
     *
     * @returns The constant binding index
     */
    virtual gal_uint getConstantIndex() const;

    /**
     * Prints the GALxConstantBinding object internal data state.
     *
     * @retval    os    The output stream to print to.
     */
    virtual void print(std::ostream& os) const;

//////////////////////////
//  interface extension //
//////////////////////////
private:

    gal_bool _readGLState;                  ///< Tells if GALxGLState read is required (thus _glStateId is valid).
    gal_uint _glStateId;                  ///< The GALxGLState state Id
    
    gal_uint _constantIndex;              ///< The final position in the constant bank.
    GALxFloatVector4 _directSource;          ///< The direct input source for the binding function
    const GALxBindingFunction* _function; ///< The binding function

public:

    InternalConstantBinding(gal_uint glStateId, gal_bool readGLState,
                            gal_uint constantIndex, 
                            const GALxBindingFunction* function,
                            GALxFloatVector4 directSource = GALxFloatVector4(gal_float(0)));    

    
    ~InternalConstantBinding();
    
};

} // namespace libGAL

#endif // INTERNAL_CONSTANT_BINDING_H
