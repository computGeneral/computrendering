#include "Common.h"
#include "IDeviceImp_9.h"
#include "IStateBlockImp_9.h"

IStateBlockImp9 :: IStateBlockImp9() :
ref_count(0) {}

IStateBlockImp9 & IStateBlockImp9 :: getInstance() {
    static IStateBlockImp9 instance;
    return instance;
}

HRESULT D3D_CALL IStateBlockImp9 :: QueryInterface (  REFIID riid , void** ppvObj ) {
    * ppvObj = cover_buffer_9;
    D3D_DEBUG( cout <<"WARNING:  IDirect3DStateBlock9 :: QueryInterface  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

ULONG D3D_CALL IStateBlockImp9 :: AddRef ( ) {
    D3D_DEBUG( cout <<"IDirect3DStateBlock9 :: AddRef" << endl; )
    return 0;
}

ULONG D3D_CALL IStateBlockImp9 :: Release ( ) {
    D3D_DEBUG( cout <<"IDirect3DStateBlock9 :: Release" << endl; )
    return 0;
}

HRESULT D3D_CALL IStateBlockImp9 :: GetDevice (  IDirect3DDevice9** ppDevice ) {
    * ppDevice = & IDeviceImp9 :: getInstance();
    D3D_DEBUG( cout <<"WARNING:  IDirect3DStateBlock9 :: GetDevice  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL IStateBlockImp9 :: Capture ( ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DStateBlock9 :: Capture  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL IStateBlockImp9 :: Apply ( ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DStateBlock9 :: Apply  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

