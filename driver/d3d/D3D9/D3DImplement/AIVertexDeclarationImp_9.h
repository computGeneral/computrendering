/**************************************************************************
 *
 */

#ifndef AIVERTEXDECLARATIONIMP_9_H
#define AIVERTEXDECLARATIONIMP_9_H

#include <vector>

class AIVertexDeclarationImp9 : public IDirect3DVertexDeclaration9{

public:
   /// Singleton method, maintained to allow unimplemented methods to return valid interface addresses.
    static AIVertexDeclarationImp9 &getInstance();

    AIVertexDeclarationImp9(AIDeviceImp9* i_parent, CONST D3DVERTEXELEMENT9* elements);

private:
    /// Singleton constructor method
    AIVertexDeclarationImp9();

    AIDeviceImp9* i_parent;

    std::vector< D3DVERTEXELEMENT9 > vElements;
    //UINT numElements;

    ULONG refs;

public:

    HRESULT D3D_CALL QueryInterface(REFIID riid, void** ppvObj);
    ULONG D3D_CALL AddRef();
    ULONG D3D_CALL Release();
    HRESULT D3D_CALL GetDevice(IDirect3DDevice9** ppDevice);
    HRESULT D3D_CALL GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements);

    vector< D3DVERTEXELEMENT9 >& getVertexElements();
    //UINT getNumElements();

};

#endif

