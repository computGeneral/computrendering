/**************************************************************************
 *
 */

#ifndef GALx_CONSTANT_BINDING_IMP_H
    #define GALx_CONSTANT_BINDING_IMP_H

#include "GALxConstantBinding.h"
#include "GALxFixedPipelineStateImp.h"

namespace libGAL
{

class GALxConstantBindingImp: public GALxConstantBinding
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
    
    GALx_BINDING_TARGET _target;    ///< Binding target used by the program to reference the target (environment or local parameter)
    gal_uint _constantIndex;        ///< Binding index used by the program
    std::vector<GALx_STORED_FP_ITEM_ID> _vStateIds;    ///< Id of the requested binding state
    GALxFloatVector4 _directSource;        ///< The direct input source for the binding function
    const GALxBindingFunction* _function; ///< The binding function

    void resolveGLSVectorState(const GALxFixedPipelineStateImp* fpStateImp, VectorId vId, gal_uint componentMask, GALxFloatVector4& vState) const;

    void resolveGLSMatrixState(const GALxFixedPipelineStateImp* fpStateImp, MatrixId mId, MatrixType type, gal_uint unit, std::vector<GALxFloatVector4>& mState) const;

public:
    
    GALxConstantBindingImp(GALx_BINDING_TARGET target, 
                           gal_uint constantIndex, 
                           std::vector<GALx_STORED_FP_ITEM_ID> vStateIds, 
                           const GALxBindingFunction* function,
                           GALxFloatVector4 directSource = GALxFloatVector4(gal_float(0)));    
                           
    ~GALxConstantBindingImp();

    /**
     * Returns the constant binding target
     *
     * @returns The constant target
     */
    virtual GALx_BINDING_TARGET getTarget() const;

    /**
     * Returns the state id
     *
     * @returns The state id
     */
    virtual std::vector<GALx_STORED_FP_ITEM_ID> getStateIds() const;

    /**
     * Returns a pointer to the binding function.
     *
     * @returns The pointer to the binding function.
     */
    virtual const GALxBindingFunction* getBindingFunction() const;

    /**
     * Returns a copy of the direct source value
     *
     * @returns The copy of the direct source value
     */
    virtual GALxFloatVector4 getDirectSource() const;


    static void printTargetEnum(std::ostream& os, GALx_BINDING_TARGET target);

    friend std::ostream& operator<<(std::ostream& os, const GALxConstantBindingImp& cbi)
    {
        cbi.print(os);
        return os;
    }
};

} // namespace libGAL

#endif // GALx_CONSTANT_BINDING_IMP_H
