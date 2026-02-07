/**************************************************************************
 *
 */

#ifndef GAL_STORED_STATE_IMP_H
    #define GAL_STORED_STATE_IMP_H

#include "GALStoredState.h"
#include "StateItem.h"
#include <list>
#include "StoredStateItem.h"
#include "GALStoredItemID.h"
#include "GALVector.h"

namespace libGAL
{

class GALStoredStateItem: public StoredStateItem
{
protected:
    
    GAL_STORED_ITEM_ID    _itemId;

public:

    GAL_STORED_ITEM_ID getItemId() const {    return _itemId;    }

    void setItemId(GAL_STORED_ITEM_ID itemId)    {    _itemId = itemId;    }

    virtual ~GALStoredStateItem() = 0;
};

/*
class GALxFloatVector4StoredStateItem: public GALStoredStateItem
{
private:

    GALxFloatVector4    _value;

public:
    
    GALxFloatVector4StoredStateItem(GALxFloatVector4 value) : _value(value) {};

    inline operator const GALxFloatVector4&() const { return _value; }

    virtual ~GALxFloatVector4StoredStateItem() { ; }
};

class GALxFloatVector3StoredStateItem: public GALxStoredFPStateItem
{
private:

    GALxFloatVector3    _value;

public:

    GALxFloatVector3StoredStateItem(GALxFloatVector3 value)    : _value(value) {};

    inline operator const GALxFloatVector3&() const { return _value; }

    virtual ~GALxFloatVector3StoredStateItem() { ; }
};
*/

class GALSingleBoolStoredStateItem: public GALStoredStateItem
{
private:

    gal_bool _value;

public:

    GALSingleBoolStoredStateItem(gal_bool value) : _value(value) {};

    inline operator const gal_bool&() const { return _value; }

    virtual ~GALSingleBoolStoredStateItem() { ; }
};

class GALSingleFloatStoredStateItem: public GALStoredStateItem
{
private:

    gal_float    _value;

public:

    GALSingleFloatStoredStateItem(gal_float value) : _value(value) {};

    inline operator const gal_float&() const { return _value; }

    virtual ~GALSingleFloatStoredStateItem() { ; }
};

class GALSingleUintStoredStateItem: public GALStoredStateItem
{
private:

    gal_uint    _value;

public:

    GALSingleUintStoredStateItem(gal_uint value) : _value(value) {};

    inline operator const gal_uint&() const { return _value; }

    virtual ~GALSingleUintStoredStateItem() { ; }
};

class GALSingleEnumStoredStateItem: public GALStoredStateItem
{
private:

    gal_enum _value;

public:

    GALSingleEnumStoredStateItem(gal_enum value) : _value(value) {};

    inline operator const gal_enum&() const { return _value; }

    virtual ~GALSingleEnumStoredStateItem() { ; }
};

class GALSingleVoidStoredStateItem: public GALStoredStateItem
{
private:

    void* _value;

public:

    GALSingleVoidStoredStateItem(void* value) : _value(value) {};

    inline operator void*() const { return _value; }

    virtual ~GALSingleVoidStoredStateItem() { ; }
};

typedef GALVector<gal_float,4> GALFloatVector4;

class GALFloatVector4StoredStateItem: public GALStoredStateItem
{
private:

    GALFloatVector4 _value;

public:

    GALFloatVector4StoredStateItem(GALFloatVector4 value) : _value(value) {};

    inline operator const GALFloatVector4&() const { return _value; }

    virtual ~GALFloatVector4StoredStateItem() { ; }
};

typedef GALVector<gal_enum,3> GALEnumVector3;

class GALEnumVector3StoredStateItem: public GALStoredStateItem
{
private:

    GALEnumVector3 _value;

public:

    GALEnumVector3StoredStateItem(GALEnumVector3 value) : _value(value) {};

    inline operator const GALEnumVector3&() const { return _value; }

    virtual ~GALEnumVector3StoredStateItem() { ; }
};

class GALViewportStoredStateItem: public GALStoredStateItem
{
private:

    gal_int _xViewport;
    gal_int _yViewport;
    gal_uint _widthViewport;
    gal_uint _heightViewport;

public:

    GALViewportStoredStateItem(gal_int xViewport, gal_int yViewport, gal_uint widthViewport, gal_uint heightViewport) 
        : _xViewport(xViewport), _yViewport(yViewport), _widthViewport(xViewport), _heightViewport(xViewport) {};

    gal_int getXViewport() const { return _xViewport; } 
    gal_int getYViewport() const { return _yViewport; } 
    gal_uint getWidthViewport() const { return _widthViewport; } 
    gal_uint getHeightViewport() const { return _heightViewport; }

    virtual ~GALViewportStoredStateItem() { ; }
};

/*
class GALxFloatMatrix4x4StoredStateItem: public GALxStoredFPStateItem
{
private:

    GALxFloatMatrix4x4    _value;

public:
    
    GALxFloatMatrix4x4StoredStateItem(GALxFloatMatrix4x4 value)    : _value(value) {};

    inline operator const GALxFloatMatrix4x4&() const { return _value; }

    virtual ~GALxFloatMatrix4x4StoredStateItem() { ; }
};
*/

class GALStoredStateImp: public GALStoredState
{
//////////////////////////
//  interface extension //
//////////////////////////
private:

    std::list<const StoredStateItem*> _ssiList;

public:
    
    GALStoredStateImp();

    void addStoredStateItem(const StoredStateItem* ssi);

    std::list<const StoredStateItem*> getSSIList() const;
};

} // namespace libGAL

#endif // GAL_STORED_STATE_IMP_H
