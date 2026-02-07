/**************************************************************************
 *
 */

#include "InternalConstantBinding.h"
#include "GALxFixedPipelineStateImp.h"

using namespace libGAL;

InternalConstantBinding::InternalConstantBinding(gal_uint glStateId, gal_bool readGLState, gal_uint constantIndex, const GALxBindingFunction* function, GALxFloatVector4 directSource)
:    _glStateId(glStateId), _readGLState(readGLState), _constantIndex(constantIndex), _function(function), _directSource(directSource)
{

}

InternalConstantBinding::~InternalConstantBinding()
{
    //delete _function;
}

void InternalConstantBinding::resolveConstant(const GALxFixedPipelineState* fpState, GALxFloatVector4& constant) const
{
    vector<GALxFloatVector4> resolvedStates;

    // Initialize the output constant to "zeros".
    constant[0] = constant[1] = constant[2] = constant[3] = gal_float(0);

    // Get the pointer to the GALxFixedPipelineState implementation class 
    const GALxFixedPipelineStateImp* fpStateImp = static_cast<const GALxFixedPipelineStateImp*>(fpState);

    // Check if binding function was initialized
    if (!_function)
        CG_ASSERT("No binding function object specified");

    if (_readGLState)
    {
        if (fpStateImp->getGLState()->isMatrix(_glStateId))
        {
            // It´s matrix state
            const GALxMatrixf& mat = fpStateImp->getGLState()->getMatrix(_glStateId);

            for (int i=0; i<4; i++)
                resolvedStates.push_back(GALxFloatVector4(&(mat.getRows()[i*4])));
        }
        else
        {
            // It´s vector state
            const Quadf& vect = fpStateImp->getGLState()->getVector(_glStateId);
            
            GALxFloatVector4 state;
            state[0] = vect[0]; state[1] = vect[1]; state[2] = vect[2]; state[3] = vect[3]; 

            resolvedStates.push_back(state);
        }
    }
    // Execute the binding function
    _function->function(constant, resolvedStates, _directSource);
}

gal_uint InternalConstantBinding::getConstantIndex() const
{
    return _constantIndex;
}

void InternalConstantBinding::print(std::ostream& os) const
{
    os << "(" << _constantIndex << ") ";

    if (_readGLState)
        os << "GALxGLState Id = " << _glStateId;

    os << "Direct src: {" << _directSource[0] << "," << _directSource[1] << "," << _directSource[2] << "," << _directSource[3] << "} ";
    os << endl;
}
