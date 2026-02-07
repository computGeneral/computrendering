/**************************************************************************
 *
 */

#include "ClusterBankAdapter.h"
#include "ImplementedConstantBindingFunctions.h"
#include "InternalConstantBinding.h"

using namespace std;
using namespace libGAL;


ClusterBankAdapter::ClusterBankAdapter(GALxRBank<float>* clusterBank)
: _clusterBank(clusterBank)
{
}




const GALxConstantBindingImp* ClusterBankAdapter::searchLocal(gal_uint pos, const GALxConstantBindingList* inputConstantBindings) const
{
    list<GALxConstantBinding*>::const_iterator iter = inputConstantBindings->begin();

    while ( iter!= inputConstantBindings->end() )
    {
        GALxConstantBindingImp* cbi = static_cast<GALxConstantBindingImp*>(*iter);
        if (cbi->getTarget() == GALx_BINDING_TARGET_LOCAL && cbi->getConstantIndex() == pos)
        {
            return cbi;
        }
        iter++;
    }

    return 0;
}

const GALxConstantBindingImp* ClusterBankAdapter::searchEnvironment(gal_uint pos, const GALxConstantBindingList* inputConstantBindings) const
{
    list<GALxConstantBinding*>::const_iterator iter = inputConstantBindings->begin();

    while ( iter!= inputConstantBindings->end() )
    {
        GALxConstantBindingImp* cbi = static_cast<GALxConstantBindingImp*>(*iter);
        if (cbi->getTarget() == GALx_BINDING_TARGET_ENVIRONMENT && cbi->getConstantIndex() == pos)
        {
            return cbi;
        }
        iter++;
    }

    return 0;
}

GALxConstantBindingList* ClusterBankAdapter::getFinalConstantBindings(const GALxConstantBindingList* inputConstantBindings) const
{
    GALxConstantBindingList* retList = new GALxConstantBindingList;

    for ( gal_uint i = 0; i < _clusterBank->size(); i++ )
    {
        gal_int pos, pos2;
        GALxRType regType = _clusterBank->getType(i,pos,pos2);

        switch ( regType )
        {
            case QR_CONSTANT:
                {
                    GALxFloatVector4 constant;

                    constant[0] = (*_clusterBank)[i][0];
                    constant[1] = (*_clusterBank)[i][1];
                    constant[2] = (*_clusterBank)[i][2];
                    constant[3] = (*_clusterBank)[i][3];

                    InternalConstantBinding* icb = new InternalConstantBinding(0, false, i, new GALxDirectSrcCopyBindFunction, constant);
                    
                    retList->push_back(icb);
                }
                break;

            case QR_PARAM_LOCAL:
                {
                    const GALxConstantBindingImp* local = searchLocal(pos, inputConstantBindings);
                    
                    if (local) 
                    {
                        // There´s a constant binding for the program local parameter 
                        // referenced in the program. 
                        // Forward the constant binding properties.
                        GALxConstantBindingImp* cbi = new GALxConstantBindingImp(GALx_BINDING_TARGET_FINAL, i, local->getStateIds(), local->getBindingFunction(), local->getDirectSource());                    
                        retList->push_back(cbi);
                    }
                    else
                    {
                        // There isn´t any constant binding for the program local
                        // Bind a "zero" vector for the local parameter

                        InternalConstantBinding* icb = new InternalConstantBinding(0, false, i, new GALxDirectSrcCopyBindFunction, GALxFloatVector4(gal_float(0)));
                        retList->push_back(icb);
                    }
                    
                }
                break;

            case QR_PARAM_ENV:
                {
                    const GALxConstantBindingImp* env = searchEnvironment(pos, inputConstantBindings);

                    if (env) 
                    {
                        // There´s a constant binding for the program environment parameter 
                        // referenced in the program. 
                        // Forward the constant binding properties.
                          GALxConstantBindingImp* cbi = new GALxConstantBindingImp(GALx_BINDING_TARGET_FINAL, i, env->getStateIds(), env->getBindingFunction(), env->getDirectSource());
                        retList->push_back(cbi);
                    }
                    else
                    {
                        // There isn´t any constant binding for the program environment
                        // Bind a "zero" vector for the environment parameter
                        InternalConstantBinding* icb = new InternalConstantBinding(0, false, i, new GALxDirectSrcCopyBindFunction, GALxFloatVector4(gal_float(0)));
                        retList->push_back(icb);
                    }
                }    
                break;

            case QR_GLSTATE:
                {
                    if ( pos < BASE_STATE_MATRIX )
                    {    
                        // Vector state
                        InternalConstantBinding* icb = new InternalConstantBinding(pos, true, i, new GALxStateCopyBindFunction);
                        retList->push_back(icb);
                    }
                    else
                    {
                        // Matrix state
                        InternalConstantBinding* icb = new InternalConstantBinding(pos, true, i, new CopyMatrixRowFunction(pos2));
                        retList->push_back(icb);
                    }
                }
                break;

            case QR_UNKNOWN: break;
            default:
                CG_ASSERT("Unexpected ClusterBank register type");
        }
    }
    return retList;
}
