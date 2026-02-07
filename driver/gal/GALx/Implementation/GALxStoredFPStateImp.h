/**************************************************************************
 *
 */

#ifndef GALx_STORED_FP_STATE_IMP_H
    #define GALx_STORED_FP_STATE_IMP_H

#include "GALxStoredFPState.h"
#include "StateItem.h"
#include <list>
#include "StoredStateItem.h"
#include "GALxGlobalTypeDefinitions.h"
#include "GALxStoredFPItemID.h"

namespace libGAL
{

class GALxStoredFPStateItem: public StoredStateItem
{
protected:
    
    GALx_STORED_FP_ITEM_ID    _itemId;

public:

    GALx_STORED_FP_ITEM_ID getItemId() const {    return _itemId;    }

    void setItemId(GALx_STORED_FP_ITEM_ID itemId)    {    _itemId = itemId;    }

    virtual ~GALxStoredFPStateItem() = 0;
};

class GALxFloatVector4StoredStateItem: public GALxStoredFPStateItem
{
private:

    GALxFloatVector4    _value;

public:
    
    GALxFloatVector4StoredStateItem(GALxFloatVector4 value) : _value(value) {};

    inline operator const GALxFloatVector4&() const { return _value; }

    virtual ~GALxFloatVector4StoredStateItem() {;};
};

class GALxFloatVector3StoredStateItem: public GALxStoredFPStateItem
{
private:

    GALxFloatVector3    _value;

public:

    GALxFloatVector3StoredStateItem(GALxFloatVector3 value)    : _value(value) {};

    inline operator const GALxFloatVector3&() const { return _value; }

    virtual ~GALxFloatVector3StoredStateItem() {;};
};

class GALxSingleFloatStoredStateItem: public GALxStoredFPStateItem
{
private:

    gal_float    _value;

public:

    GALxSingleFloatStoredStateItem(gal_float value) : _value(value) {};

    inline operator const gal_float&() const { return _value; }

    virtual ~GALxSingleFloatStoredStateItem() {;};
};

class GALxFloatMatrix4x4StoredStateItem: public GALxStoredFPStateItem
{
private:

    GALxFloatMatrix4x4    _value;

public:
    
    GALxFloatMatrix4x4StoredStateItem(GALxFloatMatrix4x4 value)    : _value(value) {};

    inline operator const GALxFloatMatrix4x4&() const { return _value; }

    virtual ~GALxFloatMatrix4x4StoredStateItem() {;};
};


class GALxStoredFPStateImp: public GALxStoredFPState
{
//////////////////////////
//  interface extension //
//////////////////////////
private:

    std::list<const StoredStateItem*> _ssiList;

public:
    
    GALxStoredFPStateImp();

    void addStoredStateItem(const StoredStateItem* ssi);

    std::list<const StoredStateItem*> getSSIList() const;
};

} // namespace libGAL

#endif // GALx_STORED_FP_STATE_IMP_H
