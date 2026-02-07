/**************************************************************************
 *
 */

#ifndef CLUSTER_BANK_ADAPTER_H
    #define CLUSTER_BANK_ADAPTER_H

#include "GALxRBank.h"
#include "GALxConstantBindingImp.h"
#include <list>

namespace libGAL
{

class ClusterBankAdapter
{
private:

    GALxRBank<float>* _clusterBank;

    const GALxConstantBindingImp* searchLocal(gal_uint pos, const GALxConstantBindingList* inputConstantBindings) const;
    const GALxConstantBindingImp* searchEnvironment(gal_uint pos, const GALxConstantBindingList* inputConstantBindings) const;

public:

    ClusterBankAdapter(GALxRBank<float>* clusterBank);

    GALxConstantBindingList* getFinalConstantBindings(const GALxConstantBindingList* inputConstantBindings) const;

};

} // namespace libGAL

#endif // CLUSTER_BANK_ADAPTER_H
