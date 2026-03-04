/**************************************************************************
 * ApitraceCallDispatcherD3D.cpp
 * 
 * Dispatches apitrace CallEvents containing D3D9 API calls to the CG1
 * D3D9 GAL implementation.
 */

#include "ApitraceCallDispatcherD3D.h"
#include "D3D9State.h"
#include "AIRoot_9.h"
#include "AIDirect3DImp_9.h"
#include "AIDeviceImp_9.h"
#include "AIVertexBufferImp_9.h"
#include "AIIndexBufferImp_9.h"
#include "AITextureImp_9.h"
#include "AICubeTextureImp_9.h"
#include "AIVolumeTextureImp_9.h"
#include "AISurfaceImp_9.h"
#include "AIVertexDeclarationImp_9.h"
#include "Utils.h"
#include "AIVertexShaderImp_9.h"
#include "AIPixelShaderImp_9.h"

#include <iostream>
#include <set>
#include <cstring>

using namespace apitrace::d3d9;

// Validate that D3D shader bytecode contains the END token (0x0000FFFF).
// The shader translator scans DWORDs until it finds this token; without it,
// it reads out of bounds.  Returns true if bytecode looks valid.
static bool validateShaderBytecode(const DWORD* pFunction, size_t byteSize) {
    if (!pFunction || byteSize < 8) return false; // need at least version + end
    size_t dwordCount = byteSize / sizeof(DWORD);
    for (size_t i = 0; i < dwordCount; ++i) {
        if ((pFunction[i] & 0x0000FFFF) == 0x0000FFFF) {
            // D3DSIO_END occupies the low 16 bits
            return true;
        }
    }
    return false;
}

// ---- D3D9ObjectTracker ----

void D3D9ObjectTracker::store(uint64_t tracePtr, void* liveObj) {
    objectMap_[tracePtr] = liveObj;
}

void* D3D9ObjectTracker::lookup(uint64_t tracePtr) const {
    auto it = objectMap_.find(tracePtr);
    return (it != objectMap_.end()) ? it->second : nullptr;
}

void D3D9ObjectTracker::remove(uint64_t tracePtr) {
    objectMap_.erase(tracePtr);
}

// ---- Helpers ----

//! Extract a D3DPRESENT_PARAMETERS from a struct Value.
//! Apitrace stores struct members as numbered fields in the arguments map.
static D3DPRESENT_PARAMETERS extractPresentParams(const apitrace::Value& v) {
    D3DPRESENT_PARAMETERS pp;
    memset(&pp, 0, sizeof(pp));
    
    // Default reasonable values for simulation
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = 0;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.PresentationInterval = 0;
    
    // Apitrace wraps pointer-to-struct args in an ARRAY of size 1.
    // Unwrap the outer array to get the inner struct.
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() >= 1) {
        return extractPresentParams(v.arrayVal[0]);
    }
    
    // Try to extract from struct value
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() >= 6) {
        pp.BackBufferWidth  = (UINT)v.arrayVal[0].uintVal;
        pp.BackBufferHeight = (UINT)v.arrayVal[1].uintVal;
        pp.BackBufferFormat = (D3DFORMAT)v.arrayVal[2].uintVal;
        pp.BackBufferCount  = (UINT)v.arrayVal[3].uintVal;
        pp.MultiSampleType  = (D3DMULTISAMPLE_TYPE)v.arrayVal[4].uintVal;
        if (v.arrayVal.size() > 5) pp.MultiSampleQuality = (DWORD)v.arrayVal[5].uintVal;
        if (v.arrayVal.size() > 6) pp.SwapEffect = (D3DSWAPEFFECT)v.arrayVal[6].uintVal;
        if (v.arrayVal.size() > 7) pp.hDeviceWindow = (HWND)v.arrayVal[7].uintVal;
        if (v.arrayVal.size() > 8) pp.Windowed = (BOOL)v.arrayVal[8].uintVal;
        if (v.arrayVal.size() > 9) pp.EnableAutoDepthStencil = (BOOL)v.arrayVal[9].uintVal;
        if (v.arrayVal.size() > 10) pp.AutoDepthStencilFormat = (D3DFORMAT)v.arrayVal[10].uintVal;
        if (v.arrayVal.size() > 11) pp.Flags = (DWORD)v.arrayVal[11].uintVal;
        if (v.arrayVal.size() > 12) pp.FullScreen_RefreshRateInHz = (UINT)v.arrayVal[12].uintVal;
        if (v.arrayVal.size() > 13) pp.PresentationInterval = (UINT)v.arrayVal[13].uintVal;
    }
    else if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(D3DPRESENT_PARAMETERS)) {
        memcpy(&pp, v.blobVal.data(), sizeof(D3DPRESENT_PARAMETERS));
    }
    
    return pp;
}

//! Extract a D3DVIEWPORT9 from a struct/blob Value.
static D3DVIEWPORT9 extractViewport(const apitrace::Value& v) {
    D3DVIEWPORT9 vp;
    memset(&vp, 0, sizeof(vp));
    
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() >= 1)
        return extractViewport(v.arrayVal[0]);
    
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() >= 6) {
        vp.X      = (DWORD)v.arrayVal[0].uintVal;
        vp.Y      = (DWORD)v.arrayVal[1].uintVal;
        vp.Width  = (DWORD)v.arrayVal[2].uintVal;
        vp.Height = (DWORD)v.arrayVal[3].uintVal;
        vp.MinZ   = (v.arrayVal[4].type == apitrace::VALUE_FLOAT) ? v.arrayVal[4].floatVal : 0.0f;
        vp.MaxZ   = (v.arrayVal[5].type == apitrace::VALUE_FLOAT) ? v.arrayVal[5].floatVal : 1.0f;
    }
    else if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(D3DVIEWPORT9)) {
        memcpy(&vp, v.blobVal.data(), sizeof(D3DVIEWPORT9));
    }
    
    return vp;
}

//! Extract a D3DMATRIX from a struct/blob/array Value.
static D3DMATRIX extractMatrix(const apitrace::Value& v) {
    D3DMATRIX m;
    memset(&m, 0, sizeof(m));
    
    //  Unwrap pointer/array wrappers (size 1)
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() == 1)
        return extractMatrix(v.arrayVal[0]);
    
    //  Unwrap struct with 1 member (e.g., D3DMATRIX wrapping m[4][4])
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() == 1)
        return extractMatrix(v.arrayVal[0]);
    
    //  BLOB: direct memcpy
    if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(D3DMATRIX)) {
        memcpy(&m, v.blobVal.data(), sizeof(D3DMATRIX));
        return m;
    }
    
    //  ARRAY[4] of ARRAY[4] — m[4][4] layout
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() == 4 &&
        v.arrayVal[0].type == apitrace::VALUE_ARRAY && v.arrayVal[0].arrayVal.size() == 4) {
        float* fp = &m._11;
        for (int r = 0; r < 4; r++) {
            const auto& row = v.arrayVal[r];
            for (int c = 0; c < 4; c++) {
                fp[r * 4 + c] = (row.arrayVal[c].type == apitrace::VALUE_FLOAT)
                                  ? row.arrayVal[c].floatVal : (float)row.arrayVal[c].uintVal;
            }
        }
        return m;
    }
    
    //  Flat ARRAY or STRUCT of 16 floats
    if ((v.type == apitrace::VALUE_STRUCT || v.type == apitrace::VALUE_ARRAY) && v.arrayVal.size() >= 16) {
        float* fp = &m._11;
        for (int i = 0; i < 16; ++i) {
            fp[i] = (v.arrayVal[i].type == apitrace::VALUE_FLOAT)
                     ? v.arrayVal[i].floatVal : (float)v.arrayVal[i].uintVal;
        }
        return m;
    }
    
    return m;
}

//! Extract a D3DMATERIAL9 from a struct/blob Value.
static D3DMATERIAL9 extractMaterial(const apitrace::Value& v) {
    D3DMATERIAL9 mat;
    memset(&mat, 0, sizeof(mat));
    
    // Unwrap size-1 array/struct wrappers
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() == 1)
        return extractMaterial(v.arrayVal[0]);
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() == 1)
        return extractMaterial(v.arrayVal[0]);
    
    if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(D3DMATERIAL9)) {
        memcpy(&mat, v.blobVal.data(), sizeof(D3DMATERIAL9));
    }
    // Flat array/struct of 17 floats (4 D3DCOLORVALUE + Power)
    else if ((v.type == apitrace::VALUE_STRUCT || v.type == apitrace::VALUE_ARRAY) &&
             v.arrayVal.size() >= 17) {
        float* fp = reinterpret_cast<float*>(&mat);
        for (int i = 0; i < 17; ++i) {
            fp[i] = (v.arrayVal[i].type == apitrace::VALUE_FLOAT)
                     ? v.arrayVal[i].floatVal : (float)v.arrayVal[i].uintVal;
        }
    }
    
    return mat;
}

//! Extract a D3DLIGHT9 from a struct/blob Value.
static D3DLIGHT9 extractLight(const apitrace::Value& v) {
    D3DLIGHT9 light;
    memset(&light, 0, sizeof(light));
    
    // Unwrap size-1 array/struct wrappers
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() == 1)
        return extractLight(v.arrayVal[0]);
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() == 1)
        return extractLight(v.arrayVal[0]);
    
    if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(D3DLIGHT9)) {
        memcpy(&light, v.blobVal.data(), sizeof(D3DLIGHT9));
    }
    // Flat struct/array — D3DLIGHT9 has 26 fields (1 DWORD + 25 floats)
    else if ((v.type == apitrace::VALUE_STRUCT || v.type == apitrace::VALUE_ARRAY) &&
             v.arrayVal.size() >= 26) {
        uint32_t* ip = reinterpret_cast<uint32_t*>(&light);
        // First field is Type (DWORD)
        ip[0] = (uint32_t)v.arrayVal[0].uintVal;
        float* fp = reinterpret_cast<float*>(&light);
        for (int i = 1; i < 26; ++i) {
            fp[i] = (v.arrayVal[i].type == apitrace::VALUE_FLOAT)
                     ? v.arrayVal[i].floatVal : (float)v.arrayVal[i].uintVal;
        }
    }
    
    return light;
}

//! Extract a RECT from a struct/blob Value.
static RECT extractRect(const apitrace::Value& v) {
    RECT r = {0, 0, 0, 0};
    
    // Unwrap size-1 array/struct wrappers
    if (v.type == apitrace::VALUE_ARRAY && v.arrayVal.size() == 1)
        return extractRect(v.arrayVal[0]);
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() == 1)
        return extractRect(v.arrayVal[0]);
    
    if (v.type == apitrace::VALUE_STRUCT && v.arrayVal.size() >= 4) {
        r.left   = (LONG)v.arrayVal[0].intVal;
        r.top    = (LONG)v.arrayVal[1].intVal;
        r.right  = (LONG)v.arrayVal[2].intVal;
        r.bottom = (LONG)v.arrayVal[3].intVal;
    }
    else if (v.type == apitrace::VALUE_BLOB && v.blobVal.size() >= sizeof(RECT)) {
        memcpy(&r, v.blobVal.data(), sizeof(RECT));
    }
    
    return r;
}

// ---- Dispatcher Init ----

void apitrace::d3d9::initDispatcher(D3D9DispatcherState& state, AIRoot9* root) {
    state.root = root;
    state.d3d9 = nullptr;
    state.device = nullptr;
}

// ---- Frame Boundary ----

bool apitrace::d3d9::isFrameBoundary(const std::string& fn) {
    return fn.find("::Present") != std::string::npos ||
           fn == "IDirect3DDevice9::Present" ||
           fn == "IDirect3DSwapChain9::Present" ||
           fn == "IDirect3DDevice9Ex::PresentEx";
}

// ---- Extract method name from "InterfaceName::MethodName" ----

static std::string extractMethodName(const std::string& fullName) {
    size_t pos = fullName.rfind("::");
    if (pos != std::string::npos) return fullName.substr(pos + 2);
    return fullName;
}

static std::string extractInterfaceName(const std::string& fullName) {
    size_t pos = fullName.rfind("::");
    if (pos != std::string::npos) return fullName.substr(0, pos);
    return "";
}

// ---- Main Dispatch ----

bool apitrace::d3d9::dispatchCall(D3D9DispatcherState& state, const CallEvent& evt) {
    const std::string& fn = evt.signature.functionName;
    if (fn.empty()) return false;

    // ============ Top-level D3D9 functions ============
    
    if (fn == "Direct3DCreate9") {
        // Create the IDirect3D9 object
        UINT sdkVersion = asUINT(A(evt, 0));
        IDirect3D9* d3d9 = state.root->Direct3DCreate9(sdkVersion);
        state.d3d9 = static_cast<AIDirect3DImp9*>(d3d9);
        
        // Track the returned pointer
        if (evt.hasReturn) {
            uint64_t retPtr = asOpaquePtr(evt.returnValue);
            if (retPtr != 0) {
                state.tracker.store(retPtr, d3d9);
            }
        }
        return true;
    }
    
    // ============ IDirect3D9 methods ============
    
    if (fn == "IDirect3D9::CreateDevice") {
        if (!state.d3d9) {
            std::cerr << "ERROR: CreateDevice called but no IDirect3D9 object" << std::endl;
            return false;
        }
        
        UINT adapter = asUINT(MA(evt, 0));
        D3DDEVTYPE devType = (D3DDEVTYPE)asDWORD(MA(evt, 1));
        HWND hwnd = (HWND)asDWORD(MA(evt, 2));
        DWORD behaviorFlags = asDWORD(MA(evt, 3));
        
        D3DPRESENT_PARAMETERS pp = extractPresentParams(MA(evt, 4));
        
        IDirect3DDevice9* device = nullptr;
        HRESULT hr = state.d3d9->CreateDevice(adapter, devType, hwnd, behaviorFlags, &pp, &device);
        
        if (SUCCEEDED(hr) && device) {
            state.device = static_cast<AIDeviceImp9*>(device);
            
            // Track the returned device pointer (arg 6 is the output param, after this+5 params)
            uint64_t devPtr = asOpaquePtr(A(evt, 6));
            if (devPtr != 0) {
                state.tracker.store(devPtr, device);
            }
        }
        return true;
    }
    
    // ============ IDirect3DDevice9 methods ============
    
    // From here, all calls require a device
    std::string iface = extractInterfaceName(fn);
    std::string method = extractMethodName(fn);
    
    // Skip calls we can't handle without a device
    if (!state.device && iface == "IDirect3DDevice9") {
        return false;
    }
    
    AIDeviceImp9* dev = state.device;
    
    // ---- Frame lifecycle ----
    
    if (fn == "IDirect3DDevice9::BeginScene") {
        dev->BeginScene();
        return true;
    }
    if (fn == "IDirect3DDevice9::EndScene") {
        dev->EndScene();
        return true;
    }
    if (fn == "IDirect3DDevice9::Present") {
        dev->Present(nullptr, nullptr, 0, nullptr);
        return true;
    }
    if (fn == "IDirect3DDevice9::Clear") {
        DWORD count = asDWORD(MA(evt, 0));
        // pRects (arg 1) — typically NULL for full clear
        DWORD flags = asDWORD(MA(evt, 2));
        D3DCOLOR color = (D3DCOLOR)asDWORD(MA(evt, 3));
        float z = asFLOAT(MA(evt, 4));
        DWORD stencil = asDWORD(MA(evt, 5));
        dev->Clear(count, nullptr, flags, color, z, stencil);
        return true;
    }
    
    // ---- Render state ----
    
    if (fn == "IDirect3DDevice9::SetRenderState") {
        D3DRENDERSTATETYPE state_type = asRenderStateType(MA(evt, 0));
        DWORD value = asDWORD(MA(evt, 1));
        dev->SetRenderState(state_type, value);
        return true;
    }
    
    // ---- Transforms ----
    
    if (fn == "IDirect3DDevice9::SetTransform") {
        D3DTRANSFORMSTATETYPE type = asTransformStateType(MA(evt, 0));
        D3DMATRIX mat = extractMatrix(MA(evt, 1));
        dev->SetTransform(type, &mat);
        return true;
    }
    
    // ---- Viewport ----
    
    if (fn == "IDirect3DDevice9::SetViewport") {
        D3DVIEWPORT9 vp = extractViewport(MA(evt, 0));
        dev->SetViewport(&vp);
        return true;
    }
    
    // ---- Material / Light ----
    
    if (fn == "IDirect3DDevice9::SetMaterial") {
        D3DMATERIAL9 mat = extractMaterial(MA(evt, 0));
        dev->SetMaterial(&mat);
        return true;
    }
    if (fn == "IDirect3DDevice9::SetLight") {
        DWORD index = asDWORD(MA(evt, 0));
        D3DLIGHT9 light = extractLight(MA(evt, 1));
        dev->SetLight(index, &light);
        return true;
    }
    if (fn == "IDirect3DDevice9::LightEnable") {
        DWORD index = asDWORD(MA(evt, 0));
        BOOL enable = asBOOL(MA(evt, 1));
        dev->LightEnable(index, enable);
        return true;
    }
    
    // ---- Scissor ----
    
    if (fn == "IDirect3DDevice9::SetScissorRect") {
        RECT r = extractRect(MA(evt, 0));
        dev->SetScissorRect(&r);
        return true;
    }
    
    // ---- Texture Stage State ----
    
    if (fn == "IDirect3DDevice9::SetTextureStageState") {
        DWORD stage = asDWORD(MA(evt, 0));
        D3DTEXTURESTAGESTATETYPE type = asTextureStageStateType(MA(evt, 1));
        DWORD value = asDWORD(MA(evt, 2));
        dev->SetTextureStageState(stage, type, value);
        return true;
    }
    
    // ---- Sampler State ----
    
    if (fn == "IDirect3DDevice9::SetSamplerState") {
        DWORD sampler = asDWORD(MA(evt, 0));
        D3DSAMPLERSTATETYPE type = asSamplerStateType(MA(evt, 1));
        DWORD value = asDWORD(MA(evt, 2));
        dev->SetSamplerState(sampler, type, value);
        return true;
    }
    
    // ---- Texture binding ----
    
    if (fn == "IDirect3DDevice9::SetTexture") {
        DWORD stage = asDWORD(MA(evt, 0));
        IDirect3DBaseTexture9* tex = state.tracker.lookupAs<IDirect3DBaseTexture9>(MA(evt, 1));
        dev->SetTexture(stage, tex);  // tex may be nullptr (which clears the stage)
        return true;
    }
    
    // ---- Resource creation: Textures ----
    
    if (fn == "IDirect3DDevice9::CreateTexture") {
        UINT width   = asUINT(MA(evt, 0));
        UINT height  = asUINT(MA(evt, 1));
        UINT levels  = asUINT(MA(evt, 2));
        DWORD usage  = asDWORD(MA(evt, 3));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 4));
        D3DPOOL pool = asD3DPool(MA(evt, 5));
        
        IDirect3DTexture9* texture = nullptr;
        dev->CreateTexture(width, height, levels, usage, fmt, pool, &texture, nullptr);
        
        // Track returned pointer (raw arg 7 = MA arg 6)
        if (texture && evt.arguments.count(7)) {
            state.tracker.store(asOpaquePtr(A(evt, 7)), texture);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreateCubeTexture") {
        UINT edgeLen = asUINT(MA(evt, 0));
        UINT levels  = asUINT(MA(evt, 1));
        DWORD usage  = asDWORD(MA(evt, 2));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 3));
        D3DPOOL pool = asD3DPool(MA(evt, 4));
        
        IDirect3DCubeTexture9* texture = nullptr;
        dev->CreateCubeTexture(edgeLen, levels, usage, fmt, pool, &texture, nullptr);
        
        if (texture && evt.arguments.count(6)) {
            state.tracker.store(asOpaquePtr(A(evt, 6)), texture);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreateVolumeTexture") {
        UINT width   = asUINT(MA(evt, 0));
        UINT height  = asUINT(MA(evt, 1));
        UINT depth   = asUINT(MA(evt, 2));
        UINT levels  = asUINT(MA(evt, 3));
        DWORD usage  = asDWORD(MA(evt, 4));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 5));
        D3DPOOL pool = asD3DPool(MA(evt, 6));
        
        IDirect3DVolumeTexture9* texture = nullptr;
        dev->CreateVolumeTexture(width, height, depth, levels, usage, fmt, pool, &texture, nullptr);
        
        if (texture && evt.arguments.count(8)) {
            state.tracker.store(asOpaquePtr(A(evt, 8)), texture);
        }
        return true;
    }
    
    // ---- Resource creation: Vertex/Index Buffers ----
    
    if (fn == "IDirect3DDevice9::CreateVertexBuffer") {
        UINT length  = asUINT(MA(evt, 0));
        DWORD usage  = asDWORD(MA(evt, 1));
        DWORD fvf    = asDWORD(MA(evt, 2));
        D3DPOOL pool = asD3DPool(MA(evt, 3));
        
        IDirect3DVertexBuffer9* vb = nullptr;
        dev->CreateVertexBuffer(length, usage, fvf, pool, &vb, nullptr);
        
        if (vb && evt.arguments.count(5)) {
            state.tracker.store(asOpaquePtr(A(evt, 5)), vb);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreateIndexBuffer") {
        UINT length   = asUINT(MA(evt, 0));
        DWORD usage   = asDWORD(MA(evt, 1));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 2));
        D3DPOOL pool  = asD3DPool(MA(evt, 3));
        
        IDirect3DIndexBuffer9* ib = nullptr;
        dev->CreateIndexBuffer(length, usage, fmt, pool, &ib, nullptr);
        
        if (ib && evt.arguments.count(5)) {
            state.tracker.store(asOpaquePtr(A(evt, 5)), ib);
        }
        return true;
    }
    
    // ---- Resource creation: Render Targets / Depth Stencil ----
    
    if (fn == "IDirect3DDevice9::CreateRenderTarget") {
        UINT width  = asUINT(MA(evt, 0));
        UINT height = asUINT(MA(evt, 1));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 2));
        D3DMULTISAMPLE_TYPE ms = asMultiSampleType(MA(evt, 3));
        DWORD msQuality = asDWORD(MA(evt, 4));
        BOOL lockable = asBOOL(MA(evt, 5));
        
        IDirect3DSurface9* surface = nullptr;
        dev->CreateRenderTarget(width, height, fmt, ms, msQuality, lockable, &surface, nullptr);
        
        if (surface && evt.arguments.count(7)) {
            state.tracker.store(asOpaquePtr(A(evt, 7)), surface);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreateDepthStencilSurface") {
        UINT width  = asUINT(MA(evt, 0));
        UINT height = asUINT(MA(evt, 1));
        D3DFORMAT fmt = asD3DFormat(MA(evt, 2));
        D3DMULTISAMPLE_TYPE ms = asMultiSampleType(MA(evt, 3));
        DWORD msQuality = asDWORD(MA(evt, 4));
        BOOL discard = asBOOL(MA(evt, 5));
        
        IDirect3DSurface9* surface = nullptr;
        dev->CreateDepthStencilSurface(width, height, fmt, ms, msQuality, discard, &surface, nullptr);
        
        if (surface && evt.arguments.count(7)) {
            state.tracker.store(asOpaquePtr(A(evt, 7)), surface);
        }
        return true;
    }
    
    // ---- Render target binding ----
    
    if (fn == "IDirect3DDevice9::SetRenderTarget") {
        DWORD rtIndex = asDWORD(MA(evt, 0));
        IDirect3DSurface9* surface = state.tracker.lookupAs<IDirect3DSurface9>(MA(evt, 1));
        dev->SetRenderTarget(rtIndex, surface);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetDepthStencilSurface") {
        IDirect3DSurface9* surface = state.tracker.lookupAs<IDirect3DSurface9>(MA(evt, 0));
        dev->SetDepthStencilSurface(surface);
        return true;
    }
    
    // ---- Stream source / Index buffer binding ----
    
    if (fn == "IDirect3DDevice9::SetStreamSource") {
        UINT streamNum = asUINT(MA(evt, 0));
        IDirect3DVertexBuffer9* vb = state.tracker.lookupAs<IDirect3DVertexBuffer9>(MA(evt, 1));
        UINT offset = asUINT(MA(evt, 2));
        UINT stride = asUINT(MA(evt, 3));
        dev->SetStreamSource(streamNum, vb, offset, stride);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetIndices") {
        IDirect3DIndexBuffer9* ib = state.tracker.lookupAs<IDirect3DIndexBuffer9>(MA(evt, 0));
        dev->SetIndices(ib);
        return true;
    }
    
    // ---- FVF / Vertex Declaration ----
    
    if (fn == "IDirect3DDevice9::SetFVF") {
        DWORD fvf = asDWORD(MA(evt, 0));
        dev->SetFVF(fvf);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreateVertexDeclaration") {
        // Extract vertex elements from blob data
        const apitrace::Value& elemVal = MA(evt, 0);
        const D3DVERTEXELEMENT9* pElements = nullptr;
        std::vector<D3DVERTEXELEMENT9> parsedElements;
        
        if (elemVal.type == apitrace::VALUE_BLOB) {
            pElements = reinterpret_cast<const D3DVERTEXELEMENT9*>(elemVal.blobVal.data());
        }
        else if (elemVal.type == apitrace::VALUE_ARRAY) {
            // Parse array of VALUE_STRUCT entries into D3DVERTEXELEMENT9 elements.
            // Field order per apitrace spec: Stream, Offset, Type, Method, Usage, UsageIndex
            for (size_t i = 0; i < elemVal.arrayVal.size(); ++i) {
                const apitrace::Value& sv = elemVal.arrayVal[i];
                if (sv.type == apitrace::VALUE_STRUCT && sv.arrayVal.size() >= 6) {
                    D3DVERTEXELEMENT9 e;
                    e.Stream     = (WORD)sv.arrayVal[0].uintVal;
                    e.Offset     = (WORD)sv.arrayVal[1].uintVal;
                    e.Type       = (BYTE)sv.arrayVal[2].uintVal;
                    e.Method     = (BYTE)sv.arrayVal[3].uintVal;
                    e.Usage      = (BYTE)sv.arrayVal[4].uintVal;
                    e.UsageIndex = (BYTE)sv.arrayVal[5].uintVal;
                    parsedElements.push_back(e);
                }
            }
            // Append end sentinel (D3DDECL_END)
            D3DVERTEXELEMENT9 endElem = D3DDECL_END();
            parsedElements.push_back(endElem);
            pElements = parsedElements.data();
        }
        
        IDirect3DVertexDeclaration9* decl = nullptr;
        if (pElements) {
            dev->CreateVertexDeclaration(pElements, &decl);
        }
        
        if (decl && evt.arguments.count(2)) {
            state.tracker.store(asOpaquePtr(A(evt, 2)), decl);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetVertexDeclaration") {
        IDirect3DVertexDeclaration9* decl = state.tracker.lookupAs<IDirect3DVertexDeclaration9>(MA(evt, 0));
        dev->SetVertexDeclaration(decl);
        return true;
    }
    
    // ---- Shaders ----
    
    if (fn == "IDirect3DDevice9::CreateVertexShader") {
        const apitrace::Value& funcVal = MA(evt, 0);
        const DWORD* pFunction = nullptr;
        std::vector<uint8_t> bytecodeBuffer; // keep alive for scope
        
        if (funcVal.type == apitrace::VALUE_BLOB && !funcVal.blobVal.empty()) {
            pFunction = reinterpret_cast<const DWORD*>(funcVal.blobVal.data());
        } else if (funcVal.type == apitrace::VALUE_STRING && !funcVal.strVal.empty()) {
            // Apitrace may store shader bytecode as VALUE_STRING.
            // readString strips trailing null which corrupts DWORD-aligned data.
            // Pad to DWORD boundary to ensure proper alignment.
            size_t sz = funcVal.strVal.size();
            size_t paddedSz = (sz + 3) & ~3u;
            bytecodeBuffer.resize(paddedSz, 0);
            memcpy(bytecodeBuffer.data(), funcVal.strVal.data(), sz);
            pFunction = reinterpret_cast<const DWORD*>(bytecodeBuffer.data());
        }
        
        IDirect3DVertexShader9* shader = nullptr;
        if (pFunction) {
            size_t bcSize = bytecodeBuffer.empty() ? funcVal.blobVal.size()
                                                   : bytecodeBuffer.size();
            if (!validateShaderBytecode(pFunction, bcSize)) {
                // Invalid shader bytecode — skip creation
            } else {
                try {
                    dev->CreateVertexShader(pFunction, &shader);
                } catch (...) {
                    shader = nullptr;
                }
            }
        }
        
        if (shader && evt.arguments.count(2)) {
            uint64_t key = asOpaquePtr(A(evt, 2));
            state.tracker.store(key, shader);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::CreatePixelShader") {
        const apitrace::Value& funcVal = MA(evt, 0);
        const DWORD* pFunction = nullptr;
        std::vector<uint8_t> bytecodeBuffer; // keep alive for scope
        
        if (funcVal.type == apitrace::VALUE_BLOB && !funcVal.blobVal.empty()) {
            pFunction = reinterpret_cast<const DWORD*>(funcVal.blobVal.data());
        } else if (funcVal.type == apitrace::VALUE_STRING && !funcVal.strVal.empty()) {
            size_t sz = funcVal.strVal.size();
            size_t paddedSz = (sz + 3) & ~3u;
            bytecodeBuffer.resize(paddedSz, 0);
            memcpy(bytecodeBuffer.data(), funcVal.strVal.data(), sz);
            pFunction = reinterpret_cast<const DWORD*>(bytecodeBuffer.data());
        }
        
        IDirect3DPixelShader9* shader = nullptr;
        if (pFunction) {
            size_t bcSize = bytecodeBuffer.empty() ? funcVal.blobVal.size()
                                                   : bytecodeBuffer.size();
            if (!validateShaderBytecode(pFunction, bcSize)) {
                // Invalid shader bytecode — skip creation
            } else {
                try {
                    dev->CreatePixelShader(pFunction, &shader);
                } catch (...) {
                    shader = nullptr;
                }
            }
        }
        
        if (shader && evt.arguments.count(2)) {
            uint64_t key = asOpaquePtr(A(evt, 2));
            state.tracker.store(key, shader);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetVertexShader") {
        IDirect3DVertexShader9* vs = state.tracker.lookupAs<IDirect3DVertexShader9>(MA(evt, 0));
        dev->SetVertexShader(vs);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetPixelShader") {
        IDirect3DPixelShader9* ps = state.tracker.lookupAs<IDirect3DPixelShader9>(MA(evt, 0));
        dev->SetPixelShader(ps);
        return true;
    }
    
    // ---- Shader Constants ----
    
    if (fn == "IDirect3DDevice9::SetVertexShaderConstantF") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT vec4Count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB && !dataVal.blobVal.empty()) {
            dev->SetVertexShaderConstantF(startReg,
                reinterpret_cast<const float*>(dataVal.blobVal.data()), vec4Count);
        }
        else if (dataVal.type == apitrace::VALUE_ARRAY) {
            static thread_local float buf[1024];
            for (size_t i = 0; i < dataVal.arrayVal.size() && i < 1024; ++i) {
                buf[i] = (dataVal.arrayVal[i].type == apitrace::VALUE_FLOAT)
                          ? dataVal.arrayVal[i].floatVal : (float)dataVal.arrayVal[i].uintVal;
            }
            dev->SetVertexShaderConstantF(startReg, buf, vec4Count);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetPixelShaderConstantF") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT vec4Count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB && !dataVal.blobVal.empty()) {
            dev->SetPixelShaderConstantF(startReg,
                reinterpret_cast<const float*>(dataVal.blobVal.data()), vec4Count);
        }
        else if (dataVal.type == apitrace::VALUE_ARRAY) {
            static thread_local float buf[1024];
            for (size_t i = 0; i < dataVal.arrayVal.size() && i < 1024; ++i) {
                buf[i] = (dataVal.arrayVal[i].type == apitrace::VALUE_FLOAT)
                          ? dataVal.arrayVal[i].floatVal : (float)dataVal.arrayVal[i].uintVal;
            }
            dev->SetPixelShaderConstantF(startReg, buf, vec4Count);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetVertexShaderConstantI") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT vec4Count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB) {
            dev->SetVertexShaderConstantI(startReg,
                reinterpret_cast<const int*>(dataVal.blobVal.data()), vec4Count);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetPixelShaderConstantI") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT vec4Count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB) {
            dev->SetPixelShaderConstantI(startReg,
                reinterpret_cast<const int*>(dataVal.blobVal.data()), vec4Count);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetVertexShaderConstantB") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB) {
            dev->SetVertexShaderConstantB(startReg,
                reinterpret_cast<const BOOL*>(dataVal.blobVal.data()), count);
        }
        return true;
    }
    
    if (fn == "IDirect3DDevice9::SetPixelShaderConstantB") {
        UINT startReg = asUINT(MA(evt, 0));
        const apitrace::Value& dataVal = MA(evt, 1);
        UINT count = asUINT(MA(evt, 2));
        
        if (dataVal.type == apitrace::VALUE_BLOB) {
            dev->SetPixelShaderConstantB(startReg,
                reinterpret_cast<const BOOL*>(dataVal.blobVal.data()), count);
        }
        return true;
    }
    
    // ---- Draw calls ----
    
    if (fn == "IDirect3DDevice9::DrawPrimitive") {
        D3DPRIMITIVETYPE primType = asPrimType(MA(evt, 0));
        UINT startVertex = asUINT(MA(evt, 1));
        UINT primCount   = asUINT(MA(evt, 2));
        dev->DrawPrimitive(primType, startVertex, primCount);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::DrawIndexedPrimitive") {
        D3DPRIMITIVETYPE primType = asPrimType(MA(evt, 0));
        INT baseVertexIndex = asINT(MA(evt, 1));
        UINT minVertexIndex = asUINT(MA(evt, 2));
        UINT numVertices    = asUINT(MA(evt, 3));
        UINT startIndex     = asUINT(MA(evt, 4));
        UINT primCount      = asUINT(MA(evt, 5));
        dev->DrawIndexedPrimitive(primType, baseVertexIndex, minVertexIndex,
                                   numVertices, startIndex, primCount);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::DrawPrimitiveUP") {
        D3DPRIMITIVETYPE primType = asPrimType(MA(evt, 0));
        UINT primCount = asUINT(MA(evt, 1));
        const apitrace::Value& vtxData = MA(evt, 2);
        UINT vtxStride = asUINT(MA(evt, 3));
        
        const void* pData = nullptr;
        size_t blobSize = 0;
        const uint8_t* blobData = extractBlob(vtxData, blobSize);
        if (blobData && blobSize > 0) {
            pData = blobData;
        }
        
        dev->DrawPrimitiveUP(primType, primCount, pData, vtxStride);
        return true;
    }
    
    if (fn == "IDirect3DDevice9::DrawIndexedPrimitiveUP") {
        D3DPRIMITIVETYPE primType = asPrimType(MA(evt, 0));
        UINT minVertexIndex = asUINT(MA(evt, 1));
        UINT numVertices    = asUINT(MA(evt, 2));
        UINT primCount      = asUINT(MA(evt, 3));
        const apitrace::Value& idxData = MA(evt, 4);
        D3DFORMAT idxFmt = asD3DFormat(MA(evt, 5));
        const apitrace::Value& vtxData = MA(evt, 6);
        UINT vtxStride = asUINT(MA(evt, 7));
        
        const void* pIdxData = (idxData.type == apitrace::VALUE_BLOB) ? idxData.blobVal.data() : nullptr;
        const void* pVtxData = (vtxData.type == apitrace::VALUE_BLOB) ? vtxData.blobVal.data() : nullptr;
        
        dev->DrawIndexedPrimitiveUP(primType, minVertexIndex, numVertices, primCount,
                                     pIdxData, idxFmt, pVtxData, vtxStride);
        return true;
    }
    
    // ---- Lock/Unlock: Vertex Buffers ----
    
    if (fn == "IDirect3DVertexBuffer9::Lock") {
        AIVertexBufferImp9* vb = state.tracker.lookupAs<AIVertexBufferImp9>(A(evt, 0));
        if (!vb) return true;
        
        UINT offset = asUINT(MA(evt, 0));
        UINT size   = asUINT(MA(evt, 1));
        DWORD flags = asDWORD(MA(evt, 3));
        
        void* ptr = nullptr;
        vb->Lock(offset, size, &ptr, flags);
        
        if (ptr) {
            // Extract trace ppbData address for memcpy routing
            uint64_t tracePBits = 0;
            const apitrace::Value& ppbDataVal = MA(evt, 2);
            if (ppbDataVal.type == apitrace::VALUE_OPAQUE) {
                tracePBits = ppbDataVal.ptrVal;
            } else if (ppbDataVal.type == apitrace::VALUE_ARRAY && ppbDataVal.arrayVal.size() >= 1) {
                const auto& arr0vb = ppbDataVal.arrayVal[0];
                if (arr0vb.ptrVal != 0) {
                    tracePBits = arr0vb.ptrVal;  // Works for VALUE_OPAQUE and VALUE_BLOB with address
                }
            }
            
            // Determine actual buffer size
            UINT actualSize = size;
            if (actualSize == 0) {
                D3DVERTEXBUFFER_DESC desc;
                vb->GetDesc(&desc);
                actualSize = desc.Size;
            }
            
            // Register pBits mapping for memcpy routing
            if (tracePBits != 0 && actualSize > 0) {
                uint64_t resPtr = asOpaquePtr(A(evt, 0));
                auto revKey = std::make_pair(resPtr, (uint64_t)0);
                auto prevIt = state.lockRevMap.find(revKey);
                if (prevIt != state.lockRevMap.end()) {
                    state.textureLocksByPBits.erase(prevIt->second);
                }
                state.textureLocksByPBits[tracePBits] = {ptr, actualSize};
                state.lockRevMap[revKey] = tracePBits;
            }
            
            // Also try to copy blob data directly (some traces embed data at Lock time)
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 2), blobSize);
            if (blob && blobSize > 0) {
                size_t copySize = (blobSize <= actualSize) ? blobSize : actualSize;
                memcpy(ptr, blob, copySize);
            }
            state.pendingLocks[asOpaquePtr(A(evt, 0))] = {ptr, offset, size};
        }
        return true;
    }
    
    if (fn == "IDirect3DVertexBuffer9::Unlock") {
        AIVertexBufferImp9* vb = state.tracker.lookupAs<AIVertexBufferImp9>(A(evt, 0));
        if (vb) {
            // Clean up pBits mapping before Unlock frees the buffer
            uint64_t resPtr = asOpaquePtr(A(evt, 0));
            auto revKey = std::make_pair(resPtr, (uint64_t)0);
            auto revIt = state.lockRevMap.find(revKey);
            if (revIt != state.lockRevMap.end()) {
                state.textureLocksByPBits.erase(revIt->second);
                state.lockRevMap.erase(revIt);
            }
            vb->Unlock();
        }
        state.pendingLocks.erase(asOpaquePtr(A(evt, 0)));
        return true;
    }
    
    // ---- Lock/Unlock: Index Buffers ----
    
    if (fn == "IDirect3DIndexBuffer9::Lock") {
        AIIndexBufferImp9* ib = state.tracker.lookupAs<AIIndexBufferImp9>(A(evt, 0));
        if (!ib) return true;
        
        UINT offset = asUINT(MA(evt, 0));
        UINT size   = asUINT(MA(evt, 1));
        DWORD flags = asDWORD(MA(evt, 3));
        
        void* ptr = nullptr;
        ib->Lock(offset, size, &ptr, flags);
        
        if (ptr) {
            // Extract trace ppbData address for memcpy routing
            uint64_t tracePBits = 0;
            const apitrace::Value& ppbDataVal = MA(evt, 2);
            if (ppbDataVal.type == apitrace::VALUE_OPAQUE) {
                tracePBits = ppbDataVal.ptrVal;
            } else if (ppbDataVal.type == apitrace::VALUE_ARRAY && ppbDataVal.arrayVal.size() >= 1) {
                const auto& arr0ib = ppbDataVal.arrayVal[0];
                if (arr0ib.ptrVal != 0) {
                    tracePBits = arr0ib.ptrVal;
                }
            }
            
            // Determine actual buffer size
            UINT actualSize = size;
            if (actualSize == 0) {
                D3DINDEXBUFFER_DESC desc;
                ib->GetDesc(&desc);
                actualSize = desc.Size;
            }
            
            // Register pBits mapping for memcpy routing
            if (tracePBits != 0 && actualSize > 0) {
                uint64_t resPtr = asOpaquePtr(A(evt, 0));
                auto revKey = std::make_pair(resPtr, (uint64_t)0);
                auto prevIt = state.lockRevMap.find(revKey);
                if (prevIt != state.lockRevMap.end()) {
                    state.textureLocksByPBits.erase(prevIt->second);
                }
                state.textureLocksByPBits[tracePBits] = {ptr, actualSize};
                state.lockRevMap[revKey] = tracePBits;
            }
            
            // Also try to copy blob data directly
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 2), blobSize);
            if (blob && blobSize > 0) {
                size_t copySize = (blobSize <= actualSize) ? blobSize : actualSize;
                memcpy(ptr, blob, copySize);
            }
            state.pendingLocks[asOpaquePtr(A(evt, 0))] = {ptr, offset, size};
        }
        return true;
    }
    
    if (fn == "IDirect3DIndexBuffer9::Unlock") {
        AIIndexBufferImp9* ib = state.tracker.lookupAs<AIIndexBufferImp9>(A(evt, 0));
        if (ib) {
            // Clean up pBits mapping before Unlock frees the buffer
            uint64_t resPtr = asOpaquePtr(A(evt, 0));
            auto revKey = std::make_pair(resPtr, (uint64_t)0);
            auto revIt = state.lockRevMap.find(revKey);
            if (revIt != state.lockRevMap.end()) {
                state.textureLocksByPBits.erase(revIt->second);
                state.lockRevMap.erase(revIt);
            }
            ib->Unlock();
        }
        state.pendingLocks.erase(asOpaquePtr(A(evt, 0)));
        return true;
    }
    
    // ---- Lock/Unlock: Textures (2D) ----
    
    if (fn == "IDirect3DTexture9::LockRect") {
        AITextureImp9* tex = state.tracker.lookupAs<AITextureImp9>(A(evt, 0));
        if (!tex) return true;
        
        UINT level = asUINT(MA(evt, 0));
        DWORD flags = asDWORD(MA(evt, 3));
        
        const apitrace::Value& rectVal = MA(evt, 2);
        RECT rect = {0};
        RECT* pRect = nullptr;
        if (rectVal.type == apitrace::VALUE_STRUCT || rectVal.type == apitrace::VALUE_ARRAY) {
            rect = extractRect(rectVal);
            pRect = &rect;
        }
        
        D3DLOCKED_RECT lockedRect;
        memset(&lockedRect, 0, sizeof(lockedRect));
        tex->LockRect(level, &lockedRect, pRect, flags);
        
        // Extract pBits trace address for memcpy routing
        uint64_t tracePBits = 0;
        {
            const apitrace::Value& plr = MA(evt, 1);
            const apitrace::Value* pStruct = &plr;
            if (pStruct->type == apitrace::VALUE_ARRAY && pStruct->arrayVal.size() == 1)
                pStruct = &pStruct->arrayVal[0];
            if (pStruct->type == apitrace::VALUE_STRUCT && pStruct->arrayVal.size() >= 2) {
                if (pStruct->arrayVal[1].type == apitrace::VALUE_OPAQUE) {
                    tracePBits = pStruct->arrayVal[1].ptrVal;
                }
            }
        }
        
        // Register mapping with buffer size
        if (lockedRect.pBits && tracePBits != 0) {
            size_t bufSize = 0;
            IDirect3DSurface9* tmpSurf = nullptr;
            tex->GetSurfaceLevel(level, &tmpSurf);
            if (tmpSurf) {
                bufSize = static_cast<AISurfaceImp9*>(tmpSurf)->getLockedDataSize();
                tmpSurf->Release();
            }
            
            if (bufSize > 0) {
                uint64_t resPtr = asOpaquePtr(A(evt, 0));
                auto revKey = std::make_pair(resPtr, (uint64_t)level);
                auto prevIt = state.lockRevMap.find(revKey);
                if (prevIt != state.lockRevMap.end()) {
                    state.textureLocksByPBits.erase(prevIt->second);
                }
                state.textureLocksByPBits[tracePBits] = {lockedRect.pBits, bufSize};
                state.lockRevMap[revKey] = tracePBits;
            }
        }
        
        if (lockedRect.pBits) {
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 1), blobSize);
            if (blob && blobSize > 0) {
                memcpy(lockedRect.pBits, blob, blobSize);
            }
        }
        
        return true;
    }
    
    if (fn == "IDirect3DTexture9::UnlockRect") {
        AITextureImp9* tex = state.tracker.lookupAs<AITextureImp9>(A(evt, 0));
        if (tex) {
            UINT level = asUINT(MA(evt, 0));
            // Remove stale pBits mapping before UnlockRect frees the buffer
            uint64_t resPtr = asOpaquePtr(A(evt, 0));
            auto revKey = std::make_pair(resPtr, (uint64_t)level);
            auto revIt = state.lockRevMap.find(revKey);
            if (revIt != state.lockRevMap.end()) {
                state.textureLocksByPBits.erase(revIt->second);
                state.lockRevMap.erase(revIt);
            }
            tex->UnlockRect(level);
        }
        return true;
    }
    
    // ---- Lock/Unlock: Surfaces ----
    
    if (fn == "IDirect3DSurface9::LockRect") {
        AISurfaceImp9* surf = state.tracker.lookupAs<AISurfaceImp9>(A(evt, 0));
        if (!surf) return true;
        
        DWORD flags = asDWORD(MA(evt, 2));
        
        const apitrace::Value& rectVal = MA(evt, 1);
        RECT rect = {0};
        RECT* pRect = nullptr;
        if (rectVal.type == apitrace::VALUE_STRUCT || rectVal.type == apitrace::VALUE_ARRAY) {
            rect = extractRect(rectVal);
            pRect = &rect;
        }
        
        D3DLOCKED_RECT lockedRect;
        memset(&lockedRect, 0, sizeof(lockedRect));
        surf->LockRect(&lockedRect, pRect, flags);
        
        if (lockedRect.pBits) {
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 0), blobSize);
            if (blob && blobSize > 0) {
                memcpy(lockedRect.pBits, blob, blobSize);
            }
        }
        return true;
    }
    
    if (fn == "IDirect3DSurface9::UnlockRect") {
        AISurfaceImp9* surf = state.tracker.lookupAs<AISurfaceImp9>(A(evt, 0));
        if (surf) surf->UnlockRect();
        return true;
    }
    
    // ---- Lock/Unlock: Cube Textures ----
    
    if (fn == "IDirect3DCubeTexture9::LockRect") {
        AICubeTextureImp9* tex = state.tracker.lookupAs<AICubeTextureImp9>(A(evt, 0));
        if (!tex) return true;
        
        D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)asUINT(MA(evt, 0));
        UINT level = asUINT(MA(evt, 1));
        DWORD flags = asDWORD(MA(evt, 4));
        
        const apitrace::Value& rectVal = MA(evt, 3);
        RECT rect = {0};
        RECT* pRect = nullptr;
        if (rectVal.type == apitrace::VALUE_STRUCT || rectVal.type == apitrace::VALUE_ARRAY) {
            rect = extractRect(rectVal);
            pRect = &rect;
        }
        
        D3DLOCKED_RECT lockedRect;
        memset(&lockedRect, 0, sizeof(lockedRect));
        tex->LockRect(face, level, &lockedRect, pRect, flags);
        
        if (lockedRect.pBits) {
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 2), blobSize);
            if (blob && blobSize > 0) {
                memcpy(lockedRect.pBits, blob, blobSize);
            }
        }
        return true;
    }
    
    if (fn == "IDirect3DCubeTexture9::UnlockRect") {
        AICubeTextureImp9* tex = state.tracker.lookupAs<AICubeTextureImp9>(A(evt, 0));
        if (tex) {
            D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)asUINT(MA(evt, 0));
            UINT level = asUINT(MA(evt, 1));
            tex->UnlockRect(face, level);
        }
        return true;
    }
    
    // ---- Lock/Unlock: Volume Textures ----
    
    if (fn == "IDirect3DVolumeTexture9::LockBox") {
        AIVolumeTextureImp9* tex = state.tracker.lookupAs<AIVolumeTextureImp9>(A(evt, 0));
        if (!tex) return true;
        
        UINT level = asUINT(MA(evt, 0));
        // MA(evt, 1) = pLockedVolume (output)
        // MA(evt, 2) = pBox
        DWORD flags = asDWORD(MA(evt, 3));
        
        D3DLOCKED_BOX lockedBox;
        memset(&lockedBox, 0, sizeof(lockedBox));
        tex->LockBox(level, &lockedBox, nullptr, flags);
        
        if (lockedBox.pBits) {
            size_t blobSize = 0;
            const uint8_t* blob = extractBlob(MA(evt, 1), blobSize);
            if (blob && blobSize > 0) {
                memcpy(lockedBox.pBits, blob, blobSize);
            }
        }
        return true;
    }
    
    if (fn == "IDirect3DVolumeTexture9::UnlockBox") {
        AIVolumeTextureImp9* tex = state.tracker.lookupAs<AIVolumeTextureImp9>(A(evt, 0));
        if (tex) {
            UINT level = asUINT(MA(evt, 0));
            tex->UnlockBox(level);
        }
        return true;
    }
    
    // ---- Texture GetSurfaceLevel (needed for Lock/Unlock tracking) ----
    
    if (fn == "IDirect3DTexture9::GetSurfaceLevel") {
        AITextureImp9* tex = state.tracker.lookupAs<AITextureImp9>(A(evt, 0));
        if (tex) {
            UINT level = asUINT(MA(evt, 0));
            IDirect3DSurface9* surface = nullptr;
            tex->GetSurfaceLevel(level, &surface);
            if (surface) {
                uint64_t surfPtr = asOpaquePtr(MA(evt, 1));
                if (surfPtr != 0) {
                    state.tracker.store(surfPtr, surface);
                }
            }
        }
        return true;
    }
    
    // ---- GetType (return object type, safe to skip) ----
    
    if (fn == "IDirect3DTexture9::GetType") return true;
    if (fn == "IDirect3DCubeTexture9::GetType") return true;
    if (fn == "IDirect3DVolumeTexture9::GetType") return true;
    
    // ---- memcpy: apitrace records data writes to locked buffers ----
    
    if (fn == "memcpy") {
        uint64_t destPtr = asOpaquePtr(A(evt, 0));
        size_t dataSize = 0;
        const uint8_t* data = extractBlob(A(evt, 1), dataSize);
        
        if (data && dataSize > 0) {
            const char* route = "DROP";
            
            // Check textureLocksByPBits: exact match first
            auto texIt = state.textureLocksByPBits.find(destPtr);
            if (texIt != state.textureLocksByPBits.end() && texIt->second.size > 0) {
                size_t copySize = (dataSize <= texIt->second.size) ? dataSize : texIt->second.size;
                memcpy(texIt->second.ptr, data, copySize);
                route = "exact";
            }
            
            // Check textureLocksByPBits: offset match (memcpy to pBits + offset for rows)
            if (route[0] == 'D') {
                for (auto& kv : state.textureLocksByPBits) {
                    uint64_t base = kv.first;
                    if (destPtr > base && destPtr < base + kv.second.size) {
                        size_t offset = (size_t)(destPtr - base);
                        size_t avail = kv.second.size - offset;
                        size_t copySize = (dataSize <= avail) ? dataSize : avail;
                        memcpy((uint8_t*)kv.second.ptr + offset, data, copySize);
                        route = "offset";
                        break;
                    }
                }
            }
            
            if (route[0] == 'D') {
                for (auto& kv : state.pendingLocks) {
                    void* lockBase = kv.second.lockedPtr;
                    if (lockBase) {
                        memcpy(lockBase, data, dataSize);
                        route = "pending";
                        break;
                    }
                }
            }
        }
        return true;
    }
    
    // ---- Reset ----
    
    if (fn == "IDirect3DDevice9::Reset") {
        D3DPRESENT_PARAMETERS pp = extractPresentParams(MA(evt, 0));
        dev->Reset(&pp);
        return true;
    }
    
    // ---- Stream source frequency ----
    
    if (fn == "IDirect3DDevice9::SetStreamSourceFreq") {
        UINT streamNum = asUINT(MA(evt, 0));
        UINT setting = asUINT(MA(evt, 1));
        dev->SetStreamSourceFreq(streamNum, setting);
        return true;
    }
    
    // ---- Clip plane ----
    
    if (fn == "IDirect3DDevice9::SetClipPlane") {
        DWORD index = asDWORD(MA(evt, 0));
        const apitrace::Value& planeVal = MA(evt, 1);
        if (planeVal.type == apitrace::VALUE_BLOB) {
            dev->SetClipPlane(index, reinterpret_cast<const float*>(planeVal.blobVal.data()));
        }
        else if (planeVal.type == apitrace::VALUE_ARRAY && planeVal.arrayVal.size() >= 4) {
            float plane[4];
            for (int i = 0; i < 4; ++i)
                plane[i] = (planeVal.arrayVal[i].type == apitrace::VALUE_FLOAT) 
                            ? planeVal.arrayVal[i].floatVal : 0.0f;
            dev->SetClipPlane(index, plane);
        }
        return true;
    }
    
    // ---- GetBackBuffer (needed for render target tracking) ----
    
    if (fn == "IDirect3DDevice9::GetBackBuffer") {
        UINT swapChain = asUINT(MA(evt, 0));
        UINT backBuf = asUINT(MA(evt, 1));
        D3DBACKBUFFER_TYPE type = (D3DBACKBUFFER_TYPE)asDWORD(MA(evt, 2));
        IDirect3DSurface9* surface = nullptr;
        dev->GetBackBuffer(swapChain, backBuf, type, &surface);
        if (surface && evt.arguments.count(4)) {
            state.tracker.store(asOpaquePtr(A(evt, 4)), surface);
        }
        return true;
    }
    
    // ---- Safely skip read-only / query calls ----
    
    static std::set<std::string> skipCalls;
    if (skipCalls.empty()) {
        // Getters and query calls — no side effects
        skipCalls.insert("IDirect3DDevice9::GetDeviceCaps");
        skipCalls.insert("IDirect3DDevice9::GetDisplayMode");
        skipCalls.insert("IDirect3DDevice9::GetCreationParameters");
        skipCalls.insert("IDirect3DDevice9::GetRenderState");
        skipCalls.insert("IDirect3DDevice9::GetTransform");
        skipCalls.insert("IDirect3DDevice9::GetViewport");
        skipCalls.insert("IDirect3DDevice9::GetMaterial");
        skipCalls.insert("IDirect3DDevice9::GetLight");
        skipCalls.insert("IDirect3DDevice9::GetLightEnable");
        skipCalls.insert("IDirect3DDevice9::GetClipPlane");
        skipCalls.insert("IDirect3DDevice9::GetTextureStageState");
        skipCalls.insert("IDirect3DDevice9::GetSamplerState");
        skipCalls.insert("IDirect3DDevice9::GetTexture");
        skipCalls.insert("IDirect3DDevice9::GetRenderTarget");
        skipCalls.insert("IDirect3DDevice9::GetDepthStencilSurface");
        skipCalls.insert("IDirect3DDevice9::GetStreamSource");
        skipCalls.insert("IDirect3DDevice9::GetIndices");
        skipCalls.insert("IDirect3DDevice9::GetVertexDeclaration");
        skipCalls.insert("IDirect3DDevice9::GetFVF");
        skipCalls.insert("IDirect3DDevice9::GetVertexShader");
        skipCalls.insert("IDirect3DDevice9::GetPixelShader");
        skipCalls.insert("IDirect3DDevice9::GetVertexShaderConstantF");
        skipCalls.insert("IDirect3DDevice9::GetPixelShaderConstantF");
        skipCalls.insert("IDirect3DDevice9::GetScissorRect");
        skipCalls.insert("IDirect3DDevice9::GetSwapChain");
        skipCalls.insert("IDirect3DDevice9::GetNumberOfSwapChains");
        skipCalls.insert("IDirect3DDevice9::GetAvailableTextureMem");
        skipCalls.insert("IDirect3DDevice9::GetRasterStatus");
        skipCalls.insert("IDirect3DDevice9::GetGammaRamp");
        skipCalls.insert("IDirect3DDevice9::ValidateDevice");
        skipCalls.insert("IDirect3DDevice9::TestCooperativeLevel");
        skipCalls.insert("IDirect3DDevice9::EvictManagedResources");
        skipCalls.insert("IDirect3DDevice9::GetDirect3D");
        skipCalls.insert("IDirect3DDevice9::GetFrontBufferData");
        skipCalls.insert("IDirect3DDevice9::GetRenderTargetData");
        skipCalls.insert("IDirect3DDevice9::GetBackBuffer");
        skipCalls.insert("IDirect3DDevice9::GetStreamSourceFreq");
        skipCalls.insert("IDirect3DDevice9::GetSoftwareVertexProcessing");
        skipCalls.insert("IDirect3DDevice9::GetClipStatus");
        skipCalls.insert("IDirect3DDevice9::GetPaletteEntries");
        skipCalls.insert("IDirect3DDevice9::GetCurrentTexturePalette");
        skipCalls.insert("IDirect3DDevice9::GetNPatchMode");
        
        // COM lifecycle
        skipCalls.insert("IDirect3DDevice9::AddRef");
        skipCalls.insert("IDirect3DDevice9::Release");
        skipCalls.insert("IDirect3DDevice9::QueryInterface");
        skipCalls.insert("IDirect3D9::AddRef");
        skipCalls.insert("IDirect3D9::Release");
        skipCalls.insert("IDirect3D9::QueryInterface");
        skipCalls.insert("IDirect3DTexture9::AddRef");
        skipCalls.insert("IDirect3DTexture9::Release");
        skipCalls.insert("IDirect3DVertexBuffer9::AddRef");
        skipCalls.insert("IDirect3DVertexBuffer9::Release");
        skipCalls.insert("IDirect3DIndexBuffer9::AddRef");
        skipCalls.insert("IDirect3DIndexBuffer9::Release");
        skipCalls.insert("IDirect3DSurface9::AddRef");
        skipCalls.insert("IDirect3DSurface9::Release");
        skipCalls.insert("IDirect3DVertexShader9::AddRef");
        skipCalls.insert("IDirect3DVertexShader9::Release");
        skipCalls.insert("IDirect3DPixelShader9::AddRef");
        skipCalls.insert("IDirect3DPixelShader9::Release");
        skipCalls.insert("IDirect3DVertexDeclaration9::AddRef");
        skipCalls.insert("IDirect3DVertexDeclaration9::Release");
        skipCalls.insert("IDirect3DStateBlock9::AddRef");
        skipCalls.insert("IDirect3DStateBlock9::Release");
        skipCalls.insert("IDirect3DSwapChain9::AddRef");
        skipCalls.insert("IDirect3DSwapChain9::Release");
        skipCalls.insert("IDirect3DCubeTexture9::AddRef");
        skipCalls.insert("IDirect3DCubeTexture9::Release");
        skipCalls.insert("IDirect3DVolumeTexture9::AddRef");
        skipCalls.insert("IDirect3DVolumeTexture9::Release");
        skipCalls.insert("IDirect3DQuery9::AddRef");
        skipCalls.insert("IDirect3DQuery9::Release");
        
        // IDirect3D9 query methods
        skipCalls.insert("IDirect3D9::GetAdapterCount");
        skipCalls.insert("IDirect3D9::GetAdapterIdentifier");
        skipCalls.insert("IDirect3D9::GetAdapterModeCount");
        skipCalls.insert("IDirect3D9::EnumAdapterModes");
        skipCalls.insert("IDirect3D9::GetAdapterDisplayMode");
        skipCalls.insert("IDirect3D9::CheckDeviceType");
        skipCalls.insert("IDirect3D9::CheckDeviceFormat");
        skipCalls.insert("IDirect3D9::CheckDeviceMultiSampleType");
        skipCalls.insert("IDirect3D9::CheckDepthStencilMatch");
        skipCalls.insert("IDirect3D9::CheckDeviceFormatConversion");
        skipCalls.insert("IDirect3D9::GetDeviceCaps");
        skipCalls.insert("IDirect3D9::GetAdapterMonitor");
        skipCalls.insert("IDirect3D9::RegisterSoftwareDevice");
        
        // Surface/texture query methods
        skipCalls.insert("IDirect3DSurface9::GetDesc");
        skipCalls.insert("IDirect3DTexture9::GetLevelDesc");
        skipCalls.insert("IDirect3DTexture9::GetLevelCount");
        skipCalls.insert("IDirect3DCubeTexture9::GetLevelDesc");
        skipCalls.insert("IDirect3DCubeTexture9::GetLevelCount");
        skipCalls.insert("IDirect3DVolumeTexture9::GetLevelDesc");
        skipCalls.insert("IDirect3DVolumeTexture9::GetLevelCount");
        skipCalls.insert("IDirect3DVertexBuffer9::GetDesc");
        skipCalls.insert("IDirect3DIndexBuffer9::GetDesc");
        
        // Query methods
        skipCalls.insert("IDirect3DDevice9::CreateQuery");
        skipCalls.insert("IDirect3DQuery9::Issue");
        skipCalls.insert("IDirect3DQuery9::GetData");
        
        // Non-D3D9 calls that may appear in traces
        skipCalls.insert("DirectDrawCreateEx");
        skipCalls.insert("IDirectDraw7::SetCooperativeLevel");
        skipCalls.insert("IDirectDraw7::Release");
        skipCalls.insert("wglDescribePixelFormat");
        skipCalls.insert("D3DPERF_BeginEvent");
        skipCalls.insert("D3DPERF_EndEvent");
        skipCalls.insert("D3DPERF_SetMarker");
        skipCalls.insert("D3DPERF_SetOptions");
        skipCalls.insert("D3DPERF_GetStatus");
        skipCalls.insert("Direct3DCreate9Ex");
        
        // Cursor
        skipCalls.insert("IDirect3DDevice9::SetCursorProperties");
        skipCalls.insert("IDirect3DDevice9::SetCursorPosition");
        skipCalls.insert("IDirect3DDevice9::ShowCursor");
        skipCalls.insert("IDirect3DDevice9::SetDialogBoxMode");
        skipCalls.insert("IDirect3DDevice9::SetGammaRamp");
    }
    
    if (skipCalls.count(fn)) {
        return true;  // Silently skip
    }
    
    // ---- Unhandled call ----
    
    std::cerr << "[DBG-UNHANDLED] callNo=" << evt.callNo << " fn=" << fn 
              << " nArgs=" << evt.arguments.size() << std::endl;
    return false;
}
