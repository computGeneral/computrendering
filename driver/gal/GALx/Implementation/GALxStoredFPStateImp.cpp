/**************************************************************************
 *
 */

#include "GALxStoredFPStateImp.h"

using namespace libGAL;

GALxStoredFPStateItem::~GALxStoredFPStateItem()
{
};


GALxStoredFPStateImp::GALxStoredFPStateImp()
{
};

void GALxStoredFPStateImp::addStoredStateItem(const StoredStateItem* ssi)
{
    _ssiList.push_back(ssi);
}

std::list<const StoredStateItem*> GALxStoredFPStateImp::getSSIList() const
{
    return _ssiList;
}
