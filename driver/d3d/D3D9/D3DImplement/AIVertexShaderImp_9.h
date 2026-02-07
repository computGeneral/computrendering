/**************************************************************************
 *
 */

#ifndef AIVERTEXSHADERIMP_9_H
#define AIVERTEXSHADERIMP_9_H

#include "GALShaderProgram.h" 
#include "ShaderTranslator.h"
struct NativeShader;

class AIVertexShaderImp9 : public IDirect3DVertexShader9 {

public:
    /// Singleton method, maintained to allow unimplemented methods to return valid interface addresses.
    static AIVertexShaderImp9 &getInstance();

    AIVertexShaderImp9(AIDeviceImp9* i_parent, CONST DWORD* pFunction);

private:
    /// Singleton constructor method
    AIVertexShaderImp9();

    AIDeviceImp9* i_parent;

    ULONG refs;


    DWORD* program;
    IR* programIR;
    
    libGAL::GALShaderProgram* galVertexShader;

    NativeShader* nativeVertexShader;

public:
    HRESULT D3D_CALL QueryInterface(REFIID riid, void** ppvObj);
    ULONG D3D_CALL AddRef();
    ULONG D3D_CALL Release();
    HRESULT D3D_CALL GetDevice(IDirect3DDevice9** ppDevice);
    HRESULT D3D_CALL GetFunction(void* pData, UINT* pSizeOfData);

    libGAL::GALShaderProgram* getAcdVertexShader();
    NativeShader* getNativeVertexShader();

};

#endif
