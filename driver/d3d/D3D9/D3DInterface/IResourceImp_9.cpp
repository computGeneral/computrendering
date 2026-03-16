#include "Common.h"
#include "IDeviceImp_9.h"
#include "IResourceImp_9.h"

IResourceImp9 :: IResourceImp9() {
    ref_count = 0;
    priority = 0;  // Default priority
}

IResourceImp9 & IResourceImp9 :: getInstance() {
    static IResourceImp9 instance;
    return instance;
}

HRESULT D3D_CALL IResourceImp9 :: QueryInterface (  REFIID riid , void** ppvObj ) {
    * ppvObj = cover_buffer_9;
    D3D_DEBUG( cout <<"WARNING:  IDirect3DResource9 :: QueryInterface  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

ULONG D3D_CALL IResourceImp9 :: AddRef ( ) {
    D3D_DEBUG( cout <<"IDirect3DResource9 :: AddRef" << endl; )
    return 0;
}

ULONG D3D_CALL IResourceImp9 :: Release ( ) {
    D3D_DEBUG( cout <<"IDirect3DResource9 :: Release" << endl; )
    return 0;
}

HRESULT D3D_CALL IResourceImp9 :: GetDevice (  IDirect3DDevice9** ppDevice ) {
    * ppDevice = & IDeviceImp9 :: getInstance();
    D3D_DEBUG( cout <<"WARNING:  IDirect3DResource9 :: GetDevice  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL IResourceImp9 :: SetPrivateData (  REFGUID refguid , CONST void* pData , DWORD SizeOfData , DWORD Flags ) {
    D3D_DEBUG( cout <<"IResource9 :: SetPrivateData" << endl; )
    return D3D_OK;
}

HRESULT D3D_CALL IResourceImp9 :: GetPrivateData (  REFGUID refguid , void* pData , DWORD* pSizeOfData ) {
    D3D_DEBUG( cout <<"IResource9 :: GetPrivateData" << endl; )
    return D3D_OK;
}

HRESULT D3D_CALL IResourceImp9 :: FreePrivateData (  REFGUID refguid ) {
    D3D_DEBUG( cout <<"IResource9 :: FreePrivateData" << endl; )
    return D3D_OK;
}

DWORD D3D_CALL IResourceImp9 :: SetPriority (  DWORD PriorityNew ) {
    D3D_DEBUG( cout <<"IResource9 :: SetPriority" << endl; )
    DWORD old_priority = priority;
    priority = PriorityNew;
    return old_priority;
}

DWORD D3D_CALL IResourceImp9 :: GetPriority ( ) {
    D3D_DEBUG( cout <<"IResource9 :: GetPriority" << endl; )
    return priority;
}

void D3D_CALL IResourceImp9 :: PreLoad ( ) {
    D3D_DEBUG( cout <<"IResource9 :: PreLoad" << endl; )
}

D3DRESOURCETYPE D3D_CALL IResourceImp9 :: GetType ( ) {
    D3D_DEBUG( cout <<"IDirect3DResource9 :: GetType" << endl; )
    return (D3DRESOURCETYPE)0;  // Base resource type
}

