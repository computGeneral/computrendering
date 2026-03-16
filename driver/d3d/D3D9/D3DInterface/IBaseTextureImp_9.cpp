#include "Common.h"
#include "IDeviceImp_9.h"
#include "IBaseTextureImp_9.h"

IBaseTextureImp9 :: IBaseTextureImp9() {
    ref_count = 0;
    priority = 0;   // Default priority
    lod = 0;        // Default LOD
}

IBaseTextureImp9 & IBaseTextureImp9 :: getInstance() {
    static IBaseTextureImp9 instance;
    return instance;
}

HRESULT D3D_CALL IBaseTextureImp9 :: QueryInterface (  REFIID riid , void** ppvObj ) {
    * ppvObj = cover_buffer_9;
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: QueryInterface  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

ULONG D3D_CALL IBaseTextureImp9 :: AddRef ( ) {
    D3D_DEBUG( cout <<"IDirect3DBaseTexture9 :: AddRef" << endl; )
    return 0;
}

ULONG D3D_CALL IBaseTextureImp9 :: Release ( ) {
    D3D_DEBUG( cout <<"IDirect3DBaseTexture9 :: Release" << endl; )
    return 0;
}

HRESULT D3D_CALL IBaseTextureImp9 :: GetDevice (  IDirect3DDevice9** ppDevice ) {
    * ppDevice = & IDeviceImp9 :: getInstance();
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: GetDevice  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL IBaseTextureImp9 :: SetPrivateData (  REFGUID refguid , CONST void* pData , DWORD SizeOfData , DWORD Flags ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: SetPrivateData" << endl; )
    return D3D_OK;
}

HRESULT D3D_CALL IBaseTextureImp9 :: GetPrivateData (  REFGUID refguid , void* pData , DWORD* pSizeOfData ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: GetPrivateData" << endl; )
    return D3D_OK;
}

HRESULT D3D_CALL IBaseTextureImp9 :: FreePrivateData (  REFGUID refguid ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: FreePrivateData" << endl; )
    return D3D_OK;
}

DWORD D3D_CALL IBaseTextureImp9 :: SetPriority (  DWORD PriorityNew ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: SetPriority" << endl; )
    DWORD old_priority = priority;
    priority = PriorityNew;
    return old_priority;
}

DWORD D3D_CALL IBaseTextureImp9 :: GetPriority ( ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: GetPriority" << endl; )
    return priority;
}

void D3D_CALL IBaseTextureImp9 :: PreLoad ( ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: PreLoad" << endl; )
}

D3DRESOURCETYPE D3D_CALL IBaseTextureImp9 :: GetType ( ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: GetType" << endl; )
    return (D3DRESOURCETYPE)0;  // Base texture type
}

DWORD D3D_CALL IBaseTextureImp9 :: SetLOD (  DWORD LODNew ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: SetLOD" << endl; )
    DWORD old_lod = lod;
    lod = LODNew;
    return old_lod;
}

DWORD D3D_CALL IBaseTextureImp9 :: GetLOD ( ) {
    D3D_DEBUG( cout <<"IBaseTexture9 :: GetLOD" << endl; )
    return lod;
}

DWORD D3D_CALL IBaseTextureImp9 :: GetLevelCount ( ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: GetLevelCount  NOT IMPLEMENTED" << endl; )
    DWORD ret = static_cast< DWORD >(0);
    return ret;
}

HRESULT D3D_CALL IBaseTextureImp9 :: SetAutoGenFilterType (  D3DTEXTUREFILTERTYPE FilterType ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: SetAutoGenFilterType  NOT IMPLEMENTED" << endl; )
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

D3DTEXTUREFILTERTYPE D3D_CALL IBaseTextureImp9 :: GetAutoGenFilterType ( ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: GetAutoGenFilterType  NOT IMPLEMENTED" << endl; )
    D3DTEXTUREFILTERTYPE ret = static_cast< D3DTEXTUREFILTERTYPE >(0);
    return ret;
}

void D3D_CALL IBaseTextureImp9 :: GenerateMipSubLevels ( ) {
    D3D_DEBUG( cout <<"WARNING:  IDirect3DBaseTexture9 :: GenerateMipSubLevels  NOT IMPLEMENTED" << endl; )
}

