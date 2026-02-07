/**************************************************************************
 *
 */

#include "GALStoredStateImp.h"

using namespace libGAL;

GALStoredStateItem::~GALStoredStateItem()
{
};

GALStoredStateImp::GALStoredStateImp()
{
};

void GALStoredStateImp::addStoredStateItem(const StoredStateItem* ssi)
{
    _ssiList.push_back(ssi);
}

std::list<const StoredStateItem*> GALStoredStateImp::getSSIList() const
{
    return _ssiList;
}
