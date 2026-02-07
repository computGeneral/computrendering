/**************************************************************************
 *
 */

#include "Common.h"
#include "AIDeviceImp_9.h"
#include "AIQueryImp_9.h"

AIQueryImp9 :: AIQueryImp9() {}

AIQueryImp9 & AIQueryImp9 :: getInstance() 
{
    static AIQueryImp9 instance;
    return instance;
}

HRESULT D3D_CALL AIQueryImp9 :: QueryInterface (  REFIID riid , void** ppvObj ) 
{
    D3D9_CALL(false, "AIQueryImp9::QueryInterface")
    * ppvObj = cover_buffer_9;
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

ULONG D3D_CALL AIQueryImp9 :: AddRef ( ) 
{
    D3D9_CALL(false, "AIQueryImp9::AddRef")
    ULONG ret = static_cast< ULONG >(0);
    return ret;
}

ULONG D3D_CALL AIQueryImp9 :: Release ( ) 
{
    D3D9_CALL(false, "AIQueryImp9::Release")
    ULONG ret = static_cast< ULONG >(0);
    return ret;
}

HRESULT D3D_CALL AIQueryImp9 :: GetDevice (  IDirect3DDevice9** ppDevice ) 
{
    D3D9_CALL(false, "AIQueryImp9::GetDevice")
    * ppDevice = & AIDeviceImp9 :: getInstance();
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

D3DQUERYTYPE D3D_CALL AIQueryImp9 :: GetType ( ) 
{
    D3D9_CALL(false, "AIQueryImp9::GetType")
    D3DQUERYTYPE ret = static_cast< D3DQUERYTYPE >(0);
    return ret;
}

DWORD D3D_CALL AIQueryImp9 :: GetDataSize ( ) 
{
    D3D9_CALL(false, "AIQueryImp9::GetDataSize")
    DWORD ret = static_cast< DWORD >(0);
    return ret;
}

HRESULT D3D_CALL AIQueryImp9 :: Issue (  DWORD dwIssueFlags ) 
{
    D3D9_CALL(false, "AIQueryImp9::Issue")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL AIQueryImp9 :: GetData (  void* pData , DWORD dwSize , DWORD dwGetDataFlags ) 
{
    D3D9_CALL(false, "AIQueryImp9::GetData")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}


