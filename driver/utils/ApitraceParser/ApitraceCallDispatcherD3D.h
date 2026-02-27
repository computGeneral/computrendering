/**************************************************************************
 * ApitraceCallDispatcherD3D.h
 * 
 * Dispatches apitrace CallEvents containing D3D9 API calls to the CG1
 * D3D9 GAL implementation (AIDeviceImp9 etc.).
 * Extracts typed arguments from apitrace::Value objects and calls the
 * appropriate D3D9 interface methods.
 */

#ifndef APITRACECALLDISPATCHERD3D_H
#define APITRACECALLDISPATCHERD3D_H

#include "ApitraceParser.h"
#include "d3d9_port.h"
#include <map>
#include <string>
#include <vector>
#include <cstdint>

class AIDeviceImp9;
class AIRoot9;
class AIDirect3DImp9;

namespace apitrace { namespace d3d9 {

// ---- D3D9 Object Tracker ----
// Maps apitrace opaque pointer values to live CG1 COM objects.
class D3D9ObjectTracker {
public:
    void store(uint64_t tracePtr, void* liveObj);
    void* lookup(uint64_t tracePtr) const;
    void remove(uint64_t tracePtr);

    template<typename T>
    T* lookupAs(uint64_t tracePtr) const {
        return static_cast<T*>(lookup(tracePtr));
    }

    template<typename T>
    T* lookupAs(const Value& v) const {
        uint64_t ptr = (v.type == VALUE_OPAQUE) ? v.ptrVal : v.uintVal;
        return static_cast<T*>(lookup(ptr));
    }

private:
    std::map<uint64_t, void*> objectMap_;
};

// ---- Helpers: extract D3D9 types from apitrace::Value ----

inline DWORD asDWORD(const Value& v) {
    return (DWORD)v.uintVal;
}

inline UINT asUINT(const Value& v) {
    return (UINT)v.uintVal;
}

inline INT asINT(const Value& v) {
    return (v.type == VALUE_SINT) ? (INT)v.intVal : (INT)v.uintVal;
}

inline FLOAT asFLOAT(const Value& v) {
    return (v.type == VALUE_FLOAT) ? v.floatVal : (FLOAT)v.uintVal;
}

inline BOOL asBOOL(const Value& v) {
    if (v.type == VALUE_TRUE) return TRUE;
    if (v.type == VALUE_FALSE) return FALSE;
    return (BOOL)v.uintVal;
}

inline D3DFORMAT asD3DFormat(const Value& v) {
    return (D3DFORMAT)v.uintVal;
}

inline D3DPRIMITIVETYPE asPrimType(const Value& v) {
    return (D3DPRIMITIVETYPE)v.uintVal;
}

inline D3DRENDERSTATETYPE asRenderStateType(const Value& v) {
    return (D3DRENDERSTATETYPE)v.uintVal;
}

inline D3DTRANSFORMSTATETYPE asTransformStateType(const Value& v) {
    return (D3DTRANSFORMSTATETYPE)v.uintVal;
}

inline D3DSAMPLERSTATETYPE asSamplerStateType(const Value& v) {
    return (D3DSAMPLERSTATETYPE)v.uintVal;
}

inline D3DTEXTURESTAGESTATETYPE asTextureStageStateType(const Value& v) {
    return (D3DTEXTURESTAGESTATETYPE)v.uintVal;
}

inline D3DPOOL asD3DPool(const Value& v) {
    return (D3DPOOL)v.uintVal;
}

inline D3DMULTISAMPLE_TYPE asMultiSampleType(const Value& v) {
    return (D3DMULTISAMPLE_TYPE)v.uintVal;
}

// Get a pointer to the first byte of a struct or blob value
inline const void* asStructPtr(const Value& v) {
    if (v.type == VALUE_BLOB) return v.blobVal.data();
    if (v.type == VALUE_NULL) return nullptr;
    return nullptr;
}

// Get opaque pointer value (for COM object references in trace).
// Handles VALUE_ARRAY wrapping (apitrace stores output pointers as arrays).
inline uint64_t asOpaquePtr(const Value& v) {
    if (v.type == VALUE_OPAQUE) return v.ptrVal;
    if (v.type == VALUE_UINT) return v.uintVal;
    if (v.type == VALUE_ARRAY && v.arrayVal.size() >= 1)
        return asOpaquePtr(v.arrayVal[0]);
    return 0;
}

// Extract raw blob data from a Value, unwrapping arrays if needed.
// Returns pointer to data and sets outSize. Returns nullptr if no blob found.
inline const uint8_t* extractBlob(const Value& v, size_t& outSize) {
    if (v.type == VALUE_BLOB && !v.blobVal.empty()) {
        outSize = v.blobVal.size();
        return v.blobVal.data();
    }
    if (v.type == VALUE_ARRAY && v.arrayVal.size() >= 1) {
        return extractBlob(v.arrayVal[0], outSize);
    }
    // For struct (e.g., D3DLOCKED_RECT), look for a blob member
    if (v.type == VALUE_STRUCT) {
        for (auto& member : v.arrayVal) {
            if (member.type == VALUE_BLOB && !member.blobVal.empty()) {
                outSize = member.blobVal.size();
                return member.blobVal.data();
            }
        }
    }
    outSize = 0;
    return nullptr;
}

// Convenience arg access (reusing the same pattern as OGL dispatcher)
inline const Value& A(const CallEvent& evt, uint32_t idx) {
    static const Value nullVal;
    auto it = evt.arguments.find(idx);
    return (it != evt.arguments.end()) ? it->second : nullVal;
}

// Method arg access — skips 'this' at arg[0] for COM method calls
// Use MA(evt, 0) to get first real parameter of a method call
inline const Value& MA(const CallEvent& evt, uint32_t idx) {
    return A(evt, idx + 1);
}

// ---- D3D9 Dispatcher State ----
struct D3D9DispatcherState {
    D3D9ObjectTracker tracker;
    AIRoot9* root;
    AIDirect3DImp9* d3d9;
    AIDeviceImp9* device;
    
    // Lock/Unlock state: track pending lock data per object
    struct LockInfo {
        void* lockedPtr;
        UINT  offset;
        UINT  size;
    };
    std::map<uint64_t, LockInfo> pendingLocks;  // tracePtr → lock info
    
    D3D9DispatcherState() : root(nullptr), d3d9(nullptr), device(nullptr) {}
};

/**
 * Initialize the D3D9 dispatch state.
 * Creates AIRoot9, IDirect3D9, and prepares for device creation.
 * @param root  The AIRoot9 instance (from D3DInterface)
 */
void initDispatcher(D3D9DispatcherState& state, AIRoot9* root);

/**
 * Dispatch a D3D9 apitrace CallEvent.
 * Returns true if the call was dispatched, false if unmapped/skipped.
 */
bool dispatchCall(D3D9DispatcherState& state, const CallEvent& evt);

/**
 * Check if function name represents a frame boundary (Present).
 */
bool isFrameBoundary(const std::string& functionName);

}}  // namespace apitrace::d3d9

#endif // APITRACECALLDISPATCHERD3D_H
