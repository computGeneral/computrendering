/**************************************************************************
 *
 */

#include "GALxConstantBindingImp.h"
#include "support.h"
#include <ostream>

using namespace libGAL;
using namespace std;

GALxConstantBindingImp::GALxConstantBindingImp(GALx_BINDING_TARGET target, gal_uint constantIndex, std::vector<GALx_STORED_FP_ITEM_ID> vStateIds, const GALxBindingFunction* function, GALxFloatVector4 directSource)
: 
_target(target), 
_constantIndex(constantIndex), 
_vStateIds(vStateIds), 
_directSource(directSource), 
_function(function)
{
};

GALxConstantBindingImp::~GALxConstantBindingImp()
{
    _vStateIds.clear();
    //delete _function;
}

void GALxConstantBindingImp::resolveGLSMatrixState(const GALxFixedPipelineStateImp* fpStateImp, MatrixId mId, MatrixType type, gal_uint unit, std::vector<GALxFloatVector4>& mState) const
{
    // Read the current matrix state value.
    
    GALxMatrixf mat;

    mat = fpStateImp->getGLState()->getMatrix(mId,type,unit);

    for (int i=0; i<4; i++)
        mState.push_back(GALxFloatVector4(&(mat.getRows()[i*4])));
}

void GALxConstantBindingImp::resolveGLSVectorState(const GALxFixedPipelineStateImp* fpStateImp, VectorId vId, gal_uint componentMask, GALxFloatVector4& vState) const
{
    Quadf vect;
    
    vect = fpStateImp->getGLState()->getVector(vId);

    switch (componentMask)
    {
        case 0x08: {
                       vState[0] = vect[0];
                       vState[1] = gal_float(0);
                       vState[2] = gal_float(0);
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x04: {
                       vState[0] = vect[1];
                         vState[1] = gal_float(0);
                       vState[2] = gal_float(0);
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x02: {
                       vState[0] = vect[2];
                          vState[1] = gal_float(0);
                       vState[2] = gal_float(0);
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x01: {
                       vState[0] = vect[3];
                          vState[1] = gal_float(0);
                       vState[2] = gal_float(0);
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x07: {
                       vState[0] = vect[1]; 
                       vState[1] = vect[2]; 
                       vState[2] = vect[3];
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x0B: {
                       vState[0] = vect[0]; 
                       vState[1] = vect[2]; 
                       vState[2] = vect[3]; 
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x0D: {
                       vState[0] = vect[0]; 
                       vState[1] = vect[1]; 
                       vState[2] = vect[3];
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x0E: {
                       vState[0] = vect[0]; 
                       vState[1] = vect[1]; 
                       vState[2] = vect[2];
                       vState[3] = gal_float(1);
                   }
                   break;
        case 0x0F: {
                       vState[0] = vect[0];
                       vState[1] = vect[1];
                       vState[2] = vect[2];
                       vState[3] = vect[3];
                   }
                   break;
        default:
            CG_ASSERT("Unexpected component mask");
    }

}

void GALxConstantBindingImp::resolveConstant(const GALxFixedPipelineState* fpState, GALxFloatVector4& constant) const
{
    // GALxGLState indexing parameters 
    gal_bool isMatrix;
    VectorId vId;
    gal_uint componentMask = 0x0F;
    MatrixId mId;
    MatrixType type;
    gal_uint unit;

    vector<GALxFloatVector4> resolvedStates;

    // Initialize the output constant to "zeros".
    constant[0] = constant[1] = constant[2] = constant[3] = gal_float(0);

    // Get the pointer to the GALxFixedPipelineState implementation class 
    const GALxFixedPipelineStateImp* fpStateImp = static_cast<const GALxFixedPipelineStateImp*>(fpState);

    // Check if binding function was initialized
    if (!_function)
        CG_ASSERT("No binding function object specified");

    // Iterate over all requested states and push them back in the resolved states
    // (resolvedStates) vector.

    std::vector<GALx_STORED_FP_ITEM_ID>::const_iterator iter = _vStateIds.begin();

    while ( iter != _vStateIds.end() )
    {
        GALxFloatVector4 vState(gal_float(0));
        std::vector<GALxFloatVector4> mState(4, GALxFloatVector4(gal_float(0)));

        switch ((*iter))
        {
            //
            // Put here all the Fixed Pipeline states not present in GALxGLState
            // The original state value in the GALxFixedPipelineState will be forwarded instead
            ///
            case GALx_ALPHA_TEST_REF_VALUE: 
                // Get value from fpStateImp

                vState[0] = fpStateImp->getSingleState(GALx_ALPHA_TEST_REF_VALUE);
                vState[1] = gal_float(0);
                vState[2] = gal_float(0);
                vState[3] = gal_float(1);
                
                resolvedStates.push_back(vState);

                break;

            default: 
                
                // The remainder states need to be read from GALxGLState.
                
                fpStateImp->getGLStateId((*iter), vId, componentMask, mId, unit, type, isMatrix);  

                if (isMatrix)
                {
                    resolveGLSMatrixState(fpStateImp, mId, type, unit, mState);
                    
                    resolvedStates.push_back(mState[0]);
                    resolvedStates.push_back(mState[1]);
                    resolvedStates.push_back(mState[2]);
                    resolvedStates.push_back(mState[3]);
                }
                else
                {    // It´s vector or single state
                    
                    resolveGLSVectorState(fpStateImp, vId, componentMask, vState);

                    resolvedStates.push_back(vState);
                }
                break;
        }
        iter++;
    }
    
    // Execute the binding function
    _function->function(constant, resolvedStates, _directSource);
}

gal_uint GALxConstantBindingImp::getConstantIndex() const
{
    return _constantIndex;
}

GALxFloatVector4 GALxConstantBindingImp::getDirectSource() const
{
    return _directSource;
}

GALx_BINDING_TARGET GALxConstantBindingImp::getTarget() const
{
    return _target;
}

std::vector<GALx_STORED_FP_ITEM_ID> GALxConstantBindingImp::getStateIds() const
{
    return _vStateIds;
}

const GALxBindingFunction* GALxConstantBindingImp::getBindingFunction() const
{
    return _function;
}

void GALxConstantBindingImp::print(std::ostream& os) const
{
    printTargetEnum(os, _target);
    os << "(" << _constantIndex << ") ";

    std::vector<GALx_STORED_FP_ITEM_ID>::const_iterator iter = _vStateIds.begin();

    os << "StoredFPIds = ";

    while ( iter != _vStateIds.end() )
    {
        GALxFixedPipelineStateImp::printStateEnum(os, (*iter));

        iter++;

        if ( iter != _vStateIds.end() ) os << ", ";
    }

    os << "Direct src: {" << _directSource[0] << "," << _directSource[1] << "," << _directSource[2] << "," << _directSource[3] << "} ";

    os << endl;
}

void GALxConstantBindingImp::printTargetEnum(std::ostream& os, GALx_BINDING_TARGET target)
{
    switch(target)
    {
        case GALx_BINDING_TARGET_ENVIRONMENT: os << "ENVIRONMENT"; break;
        case GALx_BINDING_TARGET_LOCAL: os << "LOCAL"; break;
        case GALx_BINDING_TARGET_FINAL: os << "FINAL"; break;
    }
}
