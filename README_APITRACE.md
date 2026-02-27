# Apitrace Support in CG1 Simulator

## Status: Fully Functional (OGL & D3D9)

The CG1 GPU simulator uses **apitrace** as the primary trace format for both OpenGL and D3D9 API traces.

### What Works

- **Apitrace binary format parsing** (Snappy decompression, event reading, signature caching)
- **OpenGL trace playback**: 111 GL calls dispatched to OGL_gl* entry points — byte-identical PPM output verified
- **D3D9 trace playback**: 80+ D3D9 API calls dispatched to AIDeviceImp9 — functional rendering from game traces
- **API auto-detection**: `.trace` files are auto-detected as OGL or D3D9 from the first few call signatures
- **Full pipeline**: apitrace → ApitraceParser → CallDispatcher → GAL → HAL → MetaStream → Simulator
- **Both simulation models**: bhavmodel (fast emulation) and funcmodel (cycle-accurate) from .trace input
- **Frame limiting**: `--frames N` flag supported across all trace drivers

### Architecture

```
apitrace .trace file (binary, Snappy-compressed)
        ↓
ApitraceParser (format parsing, event extraction)
        ↓
    ┌── OGL detected ──┐   ┌── D3D9 detected ──┐
    │ ApitraceCall      │   │ D3DApitraceCall   │
    │ Dispatcher        │   │ Dispatcher        │
    │ (111 GL calls)    │   │ (80+ D3D9 calls)  │
    └──────┬────────────┘   └──────┬────────────┘
           │                       │
           ▼                       ▼
    OGL2 entry points        AIDeviceImp9
           │                       │
           └──────────┬────────────┘
                      ▼
                GAL → HAL → MetaStream
                      ▼
            CG1 Simulator (bhavmodel / funcmodel)
```

### Usage

```bash
# Capture with apitrace (OpenGL)
apitrace trace --api gl glxgears
# → glxgears.trace

# Capture with apitrace (D3D9, Windows)
apitrace trace --api d3d9 game.exe
# → game.trace

# Simulate with CG1
cd _BUILD_/arch/Debug    # or Release
.\CG1SIM.exe --trace ..\..\..\tests\ogl\trace\glxgears\glxgears.trace --frames 1
.\CG1SIM.exe --trace ..\..\..\tests\d3d\trace\fruitNinja\FruitNinja.trace --frames 2
```

### Implementation Files

| File | Purpose |
|------|---------|
| `driver/utils/ApitraceParser/ApitraceParser.h/cpp` | Binary format parser (Snappy, varuint, typed Values) |
| `driver/utils/ApitraceParser/ApitraceCallDispatcher.h/cpp` | OGL call dispatcher (111 GL calls) |
| `driver/utils/ApitraceParser/D3DApitraceCallDispatcher.h/cpp` | D3D9 call dispatcher (80+ calls) |
| `driver/utils/TraceDriver/TraceDriverApitrace.h/cpp` | OGL trace driver |
| `driver/utils/TraceDriver/TraceDriverApitraceD3D.h/cpp` | D3D9 trace driver |
| `driver/utils/TraceDriver/TraceDriverBase.h` | Base class (frame limiting, virtual destructor) |
| `thirdparty/snappy-1.1.10/` | Snappy decompression library |

### Differences: Legacy Formats vs Apitrace

| Feature | GLInterceptor (.txt.gz) | D3D9 PIX (.PIXRun) | Apitrace (.trace) |
|---------|-------------------------|---------------------|-------------------|
| Format | Text-based, human-readable | Binary (Microsoft) | Binary, compact |
| Buffer data | External .dat files | Embedded | Embedded as blobs |
| Compression | gzip | None/LZMA | Snappy (faster) |
| Platform | Windows capture only | Windows only | Cross-platform |
| Status | **Removed** | **Removed** | **Active** |

### Known Limitations

| Limitation | Impact |
|-----------|--------|
| OpenGL version | CG1 supports GL 1.4-2.0 subset; traces using GL 3.0+ features will fail |
| No GLSL shaders | Only ARB vertex/fragment programs supported |
| D3D9 only | D3D10/11/12 traces detected but not supported |
| 32-bit build | Win32 platform has 2GB address space limit; large traces may OOM |
| Thread support | Single-threaded only; multi-threaded traces may misbehave |
| D3D frame rendering | Some pixel format handling incomplete (GPU_RGBA32F bytesPixel) |

### Adding Support for New API Calls

**OGL:** Edit `driver/utils/ApitraceParser/ApitraceCallDispatcher.cpp`, add a handler using `A()` for argument access.

**D3D9:** Edit `driver/utils/ApitraceParser/D3DApitraceCallDispatcher.cpp`, add a handler using `MA()` for method argument access (skips COM `this` pointer). See `APITRACE_DEV_SKILL.md` for detailed patterns.

