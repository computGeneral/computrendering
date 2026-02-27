# CG1 GPU Simulator — Apitrace Development Skill

Knowledge base for developing and maintaining apitrace-based trace playback in the CG1 GPU Simulator. Covers the apitrace binary format, D3D9/OGL trace driver architecture, common pitfalls, and extension patterns.

---

## 1. Architecture Overview

### Trace Playback Pipeline

```
.trace file (apitrace binary format)
    │
    ▼
ApitraceParser          — Reads binary events from .trace file
    │
    ▼
CallDispatcher          — Maps API function names to CG1 driver calls
  (OGL: ApitraceCallDispatcher.cpp)
  (D3D9: D3DApitraceCallDispatcher.cpp)
    │
    ▼
CG1 Driver Layer        — OGL2 or D3D9 GAL/HAL
  (OGL: OGL_gl* entry points → GAL → HAL)
  (D3D9: AIDeviceImp9 → GAL → HAL)
    │
    ▼
HAL                     — Generates MetaStream transactions
    │
    ▼
Simulator (bhavmodel or funcmodel)
```

### Key Files

| File | Purpose |
|------|---------|
| `driver/utils/ApitraceParser/ApitraceParser.h/cpp` | Binary format parser for .trace files |
| `driver/utils/ApitraceParser/ApitraceCallDispatcher.h/cpp` | OGL call dispatcher (glxgears etc.) |
| `driver/utils/ApitraceParser/D3DApitraceCallDispatcher.h/cpp` | D3D9 call dispatcher (80+ API calls) |
| `driver/utils/TraceDriver/TraceDriverApitrace.h/cpp` | OGL trace driver (parser → OGL entry points) |
| `driver/utils/TraceDriver/TraceDriverApitraceD3D.h/cpp` | D3D9 trace driver (parser → AIDeviceImp9) |
| `driver/utils/TraceDriver/TraceDriverBase.h` | Base class for all trace drivers |
| `arch/CG1SIM.cpp` | Main entry — auto-detects API type from .trace header |

### API Auto-Detection

CG1SIM detects whether a `.trace` file contains GL or D3D9 calls by scanning the first few call signatures:
- If any function starts with `IDirect3D` or `Direct3DCreate` → `d3d9`
- Otherwise → `gl` (default)

This is done in `ApitraceParser::detectApiType()`.

---

## 2. Apitrace Binary Format

The apitrace format is a Snappy-compressed stream of chunks. Each chunk contains call events.

### File Structure

```
[Header]
  magic: 0x6572617401 ("trace\x01")
  version: uint32 (typically 5 or 6)

[Chunk 0]  (Snappy-compressed)
  [Event 0]
  [Event 1]
  ...

[Chunk 1]
  ...
```

### Event Types

| Type | Value | Description |
|------|-------|-------------|
| `CALL_ENTER` | 0 | Function call with arguments |
| `CALL_LEAVE` | 1 | Function return with return value |

Events come in ENTER/LEAVE pairs. The parser merges them: ENTER provides function name and arguments, LEAVE provides the return value.

### Value Types

| Type Tag | Name | Description |
|----------|------|-------------|
| 0x00 | VALUE_NULL | Null/void |
| 0x01 | VALUE_BOOL | Boolean |
| 0x02 | VALUE_SINT | Signed integer (varint) |
| 0x03 | VALUE_UINT | Unsigned integer (varint) |
| 0x04 | VALUE_FLOAT | 32-bit float |
| 0x05 | VALUE_DOUBLE | 64-bit double |
| 0x06 | VALUE_STRING | Length-prefixed UTF-8 string |
| 0x07 | VALUE_STRING (alt) | Same as 0x06 but read differently in some contexts |
| 0x08 | VALUE_BLOB | Length-prefixed binary data |
| 0x09 | VALUE_ENUM | Reference to enum signature + value |
| 0x0A | VALUE_BITMASK | Reference to bitmask signature + value |
| 0x0B | VALUE_ARRAY | Array of values |
| 0x0C | VALUE_STRUCT | Struct (named fields) |
| 0x0D | VALUE_OPAQUE | Opaque handle/pointer (uint64) |
| 0x0E | VALUE_REPR | **Critical**: text representation + optional binary blob |
| 0x0F | VALUE_WSTRING | Wide string |

### VALUE_REPR — The Shader Bytecode Format

**This is the most critical format detail for D3D9 support.**

Shader bytecode (CreateVertexShader/CreatePixelShader) is stored as `VALUE_REPR`:
```
[VALUE_REPR tag]
  [sub-value 1: VALUE_STRING] — human-readable disassembly text
  [sub-value 2: VALUE_BLOB]   — actual binary DWORD-aligned bytecode
```

The parser must **prefer the blob** when the second sub-value is `VALUE_BLOB`. If only a string is present, it contains the raw bytecode (which needs DWORD-alignment padding since `readString()` strips trailing nulls).

### Argument Indexing

Apitrace stores arguments with 0-based indexing that includes `this` for COM methods:
- `A(evt, 0)` = `this` pointer (for COM methods like `IDirect3DDevice9::*`)
- `A(evt, 1)` = first real parameter
- `MA(evt, 0)` = first real parameter (skips `this` automatically via `+1` offset)

The `MA()` macro is defined as: `#define MA(evt, idx) A(evt, (idx) + 1)`

---

## 3. D3D9 Call Dispatcher Architecture

### DispatcherState

The `D3D9DispatcherState` struct holds all live D3D9 objects:
```cpp
struct D3D9DispatcherState {
    AIRoot9* root;              // Factory
    AIDirect3DImp9* d3d9;       // IDirect3D9
    AIDeviceImp9* device;       // IDirect3DDevice9
    OpaquePointerTracker tracker; // Maps trace pointers → live objects
    std::map<uint64_t, PendingLock> pendingLocks; // Lock/Unlock data
};
```

### OpaquePointerTracker

Apitrace records COM object pointers from the original trace session. These are meaningless in replay. The `OpaquePointerTracker` maps original trace pointers to live CG1 objects:

```cpp
// Store: associate trace pointer with live object
tracker.store(tracePtr, liveObject);

// Lookup: retrieve live object from trace pointer
auto* tex = tracker.lookupAs<IDirect3DTexture9>(value);
```

### Lock/Unlock Pattern

D3D9 Lock/Unlock data transfer follows this sequence:
1. `Lock(resource)` → apitrace records the lock; we create a `PendingLock`
2. `memcpy(dest, src, size)` → apitrace records the data copy; we match it to the pending lock and write data to the resource
3. `Unlock(resource)` → we finalize and remove the pending lock

The `memcpy` handler uses a heuristic: it finds the first pending lock and copies data into it. This works for single outstanding locks but may fail with multiple simultaneous locks.

### Adding a New D3D9 API Call Handler

1. Open `D3DApitraceCallDispatcher.cpp`
2. Find the appropriate section (render state, texture, resource creation, etc.)
3. Add a handler following this pattern:

```cpp
if (fn == "IDirect3DDevice9::NewMethod") {
    // MA(evt, 0) = first parameter (skips 'this')
    DWORD param1 = asDWORD(MA(evt, 0));
    // ... extract more parameters ...
    dev->NewMethod(param1);
    return true;
}
```

4. If the method creates a COM object, track it:
```cpp
if (shader && evt.arguments.count(outputArgIndex)) {
    uint64_t key = asOpaquePtr(A(evt, outputArgIndex));
    state.tracker.store(key, shader);
}
```

5. If the method is a no-op for simulation, add to `skipCalls`:
```cpp
skipCalls.insert("IDirect3DDevice9::NewMethod");
```

---

## 4. OGL Call Dispatcher Architecture

The OGL dispatcher is simpler — it calls OGL entry points directly:

```cpp
if (fn == "glDrawArrays") {
    OGL_glDrawArrays(asEnum(A(0)), asInt(A(1)), asInt(A(2)));
    return true;
}
```

### Display Lists

The OGL dispatcher supports `glNewList`/`glEndList`/`glCallList` by recording call events and replaying them. Display list state is file-scoped (static).

---

## 5. Frame Limiting

All trace drivers support frame limiting via base class members:

```cpp
// In TraceDriverBase.h
U32 maxFrames_;
bool frameLimitReached_;
```

When `maxFrames_ > 0` and the driver detects `currentFrame >= startFrame + maxFrames`, it sets `frameLimitReached_ = true`. Subsequent `nxtMetaStream()` calls drain remaining MetaStreams from HAL and then return `nullptr`.

The frame count is passed from `CG1SIM.cpp` via `ArchConf.sim.simFrames`.

---

## 6. Output Formats

### Bhavmodel (BM)
- `frame0000.bm.png` — PNG output (via ImageSaver::savePNG)
- `frame0000.bm.ppm` — PPM output (P6 binary, BGRA→RGB conversion)

### Funcmodel (FM)
- `frame0000.cm.ppm` — PPM output (default, via `#ifndef SAVE_AS_PNG`)
- `frame0000.cm.png` — PNG output (if `SAVE_AS_PNG` is defined)

### PPM Format
P6 binary: `P6\n<width> <height>\n255\n<RGB bytes>`

The bhavmodel framebuffer is BGRA ordered:
```cpp
data[idx + 0] = blue
data[idx + 1] = green
data[idx + 2] = red
data[idx + 3] = alpha
```

---

## 7. Regression Testing

### regression_list Format
```
test_dir, arch_version, trace_file, frames, start_frame, tolerance
ogl/glxgears, 1.0, glxgears.trace, 1, 0, 0
```

### Running Regression

**Windows:**
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& .\tools\script\regression\regression.ps1
# or: & .\tools\script\regression\regression.ps1 -Config Release
```

**Linux:**
```bash
bash tools/script/regression/regression.sh
```

### Adding a New Test
1. Place trace + reference PPM in `tests/<api>/trace/<name>/`
2. Add entry to `tools/script/regression/regression_list`
3. Run regression to confirm PASS

---

## 8. Build Commands

### Windows (MSVC, Debug, Win32)
```powershell
cd _BUILD_
cmake --build . --config Debug --target CG1SIM -- /m /v:m
```

### Windows (MSVC, Release, Win32)
```powershell
cd _BUILD_
cmake --build . --config Release --target CG1SIM -- /m /v:m
```

### Linux (GCC, Release)
```bash
cd build && make -j$(nproc)
```

### Running the Simulator
```powershell
# From _BUILD_\arch\Debug (or Release):
.\CG1SIM.exe --trace ..\..\..\tests\ogl\trace\glxgears\glxgears.trace --frames 1
.\CG1SIM.exe --trace ..\..\..\tests\d3d\trace\fruitNinja\FruitNinja.trace --frames 2
```

---

## 9. Common Pitfalls & Lessons Learned

### Apitrace Format

| Pitfall | Symptom | Fix |
|---------|---------|-----|
| VALUE_REPR not handled | Shader bytecode is empty/garbage, CreatePixelShader crashes | Parse VALUE_REPR as `[text string][binary blob]`, prefer blob |
| Argument indexing off-by-one | Wrong parameters passed to D3D9 methods | COM methods: `A(evt, 0)` = this, `MA(evt, 0)` = first param |
| VALUE_STRING for bytecode | Shader data truncated or misaligned | readString() strips trailing nulls; pad to DWORD boundary |
| VALUE_ARRAY nested in VALUE_REPR | VertexDeclaration elements not extracted | Unwrap VALUE_ARRAY inside VALUE_REPR before processing |
| ENTER/LEAVE not merged | Missing return values; duplicate calls | Parser merges LEAVE into preceding ENTER via `pendingEnter_` |

### D3D9 Specifics

| Pitfall | Symptom | Fix |
|---------|---------|-----|
| No D3DTrace dependency | Build fails: D3DTrace.h not found | Forward-declare or remove; PIX trace driver was deleted |
| GetBackBuffer in both handler + skip list | Dead code, confusing logic | Remove from skipCalls if explicitly handled |
| `bytesPixel` uninitialized for GPU_RGBA32F | D3D rendered frames appear black (all zeros) | Add `case GPU_RGBA32F: bytesPixel = 16;` in dumpFrame() |
| 32-bit address space | Large traces (>1GB) crash with OOM | Use Release build; consider 64-bit port |
| memcpy handler heuristic | Data goes to wrong buffer if multiple locks active | Track trace dest address for matching |

### General

| Pitfall | Symptom | Fix |
|---------|---------|-----|
| Debug prints left in code | Noisy output, performance regression | Search `cerr\|cout\|fprintf\|printf` before commit |
| Missing virtual destructor | Undefined behavior when deleting through base pointer | Added `virtual ~cgoTraceDriverBase() {}` |
| .gen files regenerated | Massive unrelated diffs in commit | Unstage with `git reset HEAD *.gen` |
| CRT assertion dialogs (Windows) | Simulator hangs on error in non-interactive mode | `_set_abort_behavior(0, ...)` in CG1SIM.cpp |

---

## 10. Known Limitations

1. **D3D9 only**: D3D10/11/12 traces are detected but not supported
2. **32-bit build**: Win32 platform has 2GB address space limit; large traces may OOM
3. **D3D frame rendering**: `bytesPixel` for `GPU_RGBA32F` not set in bhavmodel `dumpFrame()` — D3D frames render black
4. **Multiple simultaneous locks**: memcpy handler uses first-pending-lock heuristic
5. **Unhandled D3D9 calls**: GetFunction, GetDevice, BeginStateBlock, EndStateBlock, Capture, Apply
6. **GL_COMPILE_AND_EXECUTE**: Display list mode not fully supported (compile-only)
7. **OGL dispatcher global state**: Display list storage is file-scoped static, preventing multi-instance use

---

## 11. File Change Patterns

### When modifying the apitrace parser:
- `driver/utils/ApitraceParser/ApitraceParser.h/cpp`
- Rebuild: touches all trace driver targets

### When adding D3D9 API call support:
- `driver/utils/ApitraceParser/D3DApitraceCallDispatcher.cpp`
- Only need to rebuild CG1SIM

### When modifying trace driver flow:
- `driver/utils/TraceDriver/TraceDriverApitraceD3D.cpp` (D3D9)
- `driver/utils/TraceDriver/TraceDriverApitrace.cpp` (OGL)
- Only need to rebuild CG1SIM

### When modifying frame output:
- `arch/bhavmodel/bmGpuTop.cpp` — dumpFrame() for bhavmodel
- `arch/funcmodel/DisplayController/cmDisplayController.cpp` — for funcmodel

---

*This skill document captures lessons learned during D3D9 apitrace support development for the CG1 GPU Simulator. Keep it updated as new API support or format changes are added.*
