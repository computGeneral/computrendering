# computrender GPU Simulator
===============

## Introduction

computrender (computGeneral 1) is a GPU simulator with OpenGL and D3D9 API implementations
that can be used to simulate PC Windows game traces. It replays API call sequences
captured from real applications via **apitrace**, modeling a complete GPU pipeline —
from command processing through rasterization to display output — at multiple
abstraction levels.

Referenced from project "https://attila.ac.upc.edu"

Some of the techniques and algorithms implemented in the simulator may be covered
by patents (for example DXTC/S3TC compression or the Z compression algorithm).

**Primary use cases:**
- Simulating GPU graphics workloads from captured API traces
- Architecture exploration with configurable GPU parameters
- Performance analysis via per-cycle, per-frame, per-batch statistics
- Validation between behavior model (fast) and functional model (cycle-accurate)

---

## Requirements

**Linux:**

    cmake    (>= 3.13.1)
    libpng-dev
    g++ / clang++

**Windows:**

    cmake    (>= 3.13.1)
    mingw    (for POSIX headers)
    Visual Studio 2022 or higher

---

## Directory Structure

```
computrendering/
├── README.md                    # This file
├── CLAUDE.md                    # Architecture deep-dive for coding agents
├── CLEAN_UP.md                  # Cleanup history and migration log
├── OPENISSUES.md                # Known issues
├── CMakeLists.txt               # Top-level build configuration
│
├── arch/                        # Core simulator architecture
│   ├── computrender.cpp/hpp    # ★ MAIN ENTRY POINT (main())
│   ├── CMakeLists.txt           # Arch build config
│   ├── common/                  # Shared utilities
│   │   ├── GPUReg.h             # GPU register definitions & commands
│   │   ├── GPUType.h            # GPU data types (F32, U32, S32, etc.)
│   │   ├── Vec4FP32.h/cpp       # 4D float vector
│   │   ├── Vec4Int.h/cpp        # 4D integer vector
│   │   ├── BitStream*.h/cpp     # Bit-level stream I/O
│   │   ├── Parser.h/cpp         # Config file parser
│   │   ├── Profiler.h/cpp       # Performance profiling
│   │   ├── ImageSaver.h/cpp     # PPM image output
│   │   ├── DynamicObject.h/cpp  # Fast dynamic allocation
│   │   └── params/              # Configuration files
│   │       ├── archParams.csv       # ★ Primary simulator config
│   │       ├── CG1.0.json       # Architecture v1.0 params
│   │       └── CG1.1.json       # Architecture v1.1 params
│   │
│   ├── bhavmodel/               # Behavior model (fast emulation)
│   │   ├── bhavmodel.hpp/cpp    # Top-level behavior model
│   │   ├── bmGpuTop.h/cpp       # GPU top-level behavior model
│   │   ├── UnifiedShader/       # Shader ISA execution
│   │   ├── Rasterizer/          # Triangle setup, fragment generation
│   │   ├── Clipper/             # Primitive clipping
│   │   ├── TextureProcessor/    # Texture sampling
│   │   └── FragmentOperator/    # Fragment operations (Z, blend, write)
│   │
│   ├── perfmodel/               # Performace model (cycle-approximate)
│   │   ├── perfmodel.h/cpp       # Top-level performance model
│   │   ├── common/base/         # ★ Base simulation classes
│   │   │   ├── MduBase.h      # Module base class (all MDUs inherit)
│   │   │   └── GPUSignal.h    # Signal class for inter-module comm
│   │   ├── CommandProcessor/    # Command processor & MetaStream handling
│   │   ├── StreamController/    # Vertex fetch & post-shading cache
│   │   ├── PrimitiveAssembler/  # Primitive assembly
│   │   ├── Clipper/             # Clipper stage
│   │   ├── Rasterizer/          # Triangle setup, fragment gen, HiZ
│   │   ├── UnifiedShader/       # Unified vertex/fragment shader
│   │   ├── TextureProcessor/    # Texture unit
│   │   ├── FragmentOperator/    # Z/stencil test, color write, blend
│   │   ├── DisplayController/   # Display output & blitter
│   │   ├── MemoryController/    # Simple memory model
│   │   ├── MemoryControllerV2/  # ★ Accurate GDDR memory model
│   │   └── CacheSystem/         # All cache implementations
│   │
│   └── utils/                   # Simulator utilities
│       ├── ConfigLoader.h/cpp   # INI config parser
│       ├── CommandLineReader.h  # CLI argument parser
│       ├── CG1ASM/              # Shader ISA assembler tools
│       └── TestCreator/         # Non-API test framework
│
├── driver/                      # Graphics API implementations
│   ├── hal/                     # Hardware Abstraction Layer
│   │   └── HAL.h/cpp            # ★ GPU driver (MetaStream gen, memory mgmt)
│   ├── gal/                     # Graphics Abstraction Layer
│   │   ├── GAL/                 # Core interfaces & implementations
│   │   ├── GALx/                # Extensions (fixed pipeline emu, ARB shaders)
│   │   └── test/                # GAL tests
│   ├── ogl/                     # OpenGL
│   │   ├── OGL2/                # OpenGL 2.0 implementation (GAL-based)
│   │   ├── OGL14/               # Legacy OpenGL 1.4 (deprecated)
│   │   └── inc/                 # OpenGL headers (canonical set in inc/GL)
│   ├── d3d/                     # Direct3D 9
│   │   └── D3D9/                # D3D9 implementation (GAL-based)
│   └── utils/                   # Driver utilities
│       ├── ApitraceParser/      # ★ Apitrace binary format parser & call dispatchers
│       ├── TraceDriver/         # ★ Trace drivers (Apitrace OGL/D3D, MetaStream)
│       ├── generated/           # ★ Pre-generated OpenGL mapping tables (.gen files)
│       └── misc/                # ★ Shared utility sources (buffer/memory/log/zfstream/GL resolver)
│
├── tests/                       # Test suites
│   ├── arch/                    # Architecture unit tests
│   ├── ogl/trace/               # OpenGL test traces with reference outputs
│   ├── d3d/trace/               # D3D9 apitrace test traces
│   └── ocl/                     # OpenCL compute shader tests
│
├── tools/                       # Standalone tools
│   ├── apitrace/                # Apitrace integration tools
│   ├── PPMConvertor/            # PPM image color mapping
│   ├── SnapshotExplainer/       # GPU state snapshot dumper
│   ├── TraceViewer/             # Signal trace visualizer (Qt GUI)
│   └── script/                  # Build & regression scripts
│
└── thirdparty/                  # External dependencies
    ├── zlib-1.2.13/             # Compression
    ├── snappy-1.1.10/           # Snappy decompression (apitrace)
    ├── SDL2-2.0.8/              # Simple DirectMedia Layer
    ├── rapidjson-1.1.0/         # JSON parsing
    └── tinyxml2-8.0.0/          # XML parsing
```

---

## How to Build

### Linux (CMake)

**Quick Build:**
```bash
bash tools/script/build.sh
```

**Manual Build:**
```bash
mkdir -p _BUILD_ && cd _BUILD_
cmake ..
make -j$(nproc)
```

The compiled binaries are placed in `_BUILD_/arch/` (computrender simulator).

### Windows (Visual Studio)

**Quick Build:**
```bat
tools\script\build.bat
```
Optional arguments: `build.bat [Debug|Release] [Win32|x64]` (defaults: Debug, Win32).

**Or open the solution directly:**
```
Open _BUILD_\ComputRendering.sln in Visual Studio 2022+
Select architecture: Win32 (32-bit) or x64 (64-bit)
Select configuration: Debug / Optimized / Profile
Build target: computrender
```

**Or via CMake manually:**
```powershell
mkdir _BUILD_; cd _BUILD_
cmake .. -A Win32
cmake --build . --config Debug --target computrender -- /m /v:m
```

### Build Targets

| Target | Description |
|--------|-------------|
| `computrender` | Main simulator executable |
| `archcommon` | Common utilities library |
| `BhavModel` | Behavior model library |
| `PerfModel` | Functional model library |
| `HAL`, `GAL`, `GALx`, `OGL2` | Driver libraries |
| `ApitraceParser` | Apitrace binary format parser library |

### Key CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `WARNINGS_AS_ERRORS` | Yes | Treat warnings as errors |
| `USE_PTHREADS` | No | Use pthreads library |
| `PERFORMANCE_COUNTERS` | No | Enable performance counters |
| `POWER_COUNTERS` | No | Enable power/energy counters |

---

## Supported Trace Formats

computrender is **trace-driven**: it replays API call sequences captured from real applications.
Three trace driver paths exist, all converging at the same MetaStream interface consumed
by the simulation models.

| Format | Extension | Capture Tool | Status | Use Case |
|--------|-----------|--------------|--------|----------|
| **Apitrace (OGL)** | `.trace` | apitrace (cross-platform) | ✅ Full support | OpenGL traces, regression tests |
| **Apitrace (D3D9)** | `.trace` | apitrace (cross-platform) | ✅ Full support | D3D9 application traces |
| **MetaStream** | `.tracefile.gz` | (legacy translator removed) | ✅ Optimized | Pre-translated GPU commands |

### Apitrace Format (Primary)

- **Capture**: Cross-platform (`apitrace trace --api gl <app>` or `apitrace trace --api d3d9 <app>`)
- **Format**: Binary with Snappy compression
- **API auto-detection**: The simulator automatically detects whether a `.trace` file contains OpenGL or D3D9 calls
- **OGL**: 111 GL calls dispatched to OGL_gl* entry points — verified byte-identical output
- **D3D9**: 80+ D3D9 API calls dispatched to AIDeviceImp9 — functional rendering from game traces

### MetaStream Format (Optimized)

- **Format**: Pre-compiled GPU command stream (bypasses API emulation)
- **Performance**: Fastest simulation (no API overhead)
- The legacy `traceTranslator` tool has been removed; existing `.tracefile.gz` files can still be replayed

### Pipeline Overview

```
apitrace .trace file (binary, Snappy-compressed)
        ↓
ApitraceParser (format parsing, event extraction)
        ↓
    ┌── OGL detected ──────────┐   ┌── D3D9 detected ─────────┐
    │ ApitraceCallDispatcherOGL │   │ ApitraceCallDispatcherD3D │
    │ (111 GL calls)            │   │ (80+ D3D9 calls)          │
    └──────────┬────────────────┘   └──────────┬────────────────┘
               │                               │
               ▼                               ▼
        OGL2 entry points              AIDeviceImp9
               │                               │
               └──────────────┬────────────────┘
                              ▼
                    GAL → HAL → MetaStream
                              ▼
                computrender Simulator (bhavmodel / perfmodel)
```

---

## How to Run

### Quick Start (Linux)

```bash
# After building
cd _BUILD_/arch

# Run with an OpenGL apitrace (archParams.csv is auto-discovered)
./computrender --trace ../../tests/ogl/trace/glxgears/glxgears.trace --frames 1
```

### Quick Start (Windows)

```bat
REM Build (from project root)
tools\script\build.bat

REM Run with an OpenGL apitrace (archParams.csv is auto-discovered)
cd _BUILD_\arch\Debug
.\computrender.exe --trace ..\..\..\tests\ogl\trace\glxgears\glxgears.trace --frames 1
```

### Configuration File Loading

The simulator **automatically locates** `archParams.csv` using the following search order:
1. Current working directory: `archParams.csv`
2. Relative from `_BUILD_/arch/`: `../../../arch/common/params/archParams.csv`
3. Relative paths: `../../arch/common/params/archParams.csv`
4. From project root: `arch/common/params/archParams.csv`

You can override the configuration file path and architecture version:
```bash
# Use a custom parameter file
./computrender --param /path/to/custom.csv --trace <file> --frames 1

# Select a different ARCH_VERSION column from the CSV
./computrender --arch 1.1 --trace <file> --frames 1
```

### General Usage

```
computrender --trace <file> --frames N    # Apitrace file with frame limit
computrender --param <csv> --arch <ver>   # Override config file and arch version
computrender <filename>                   # MetaStream trace file
computrender <filename> <n>               # n < 10K → frames; n >= 10K → cycles
computrender <filename> <n> <m>           # n frames, starting from frame m
```

**Apitrace examples:**
```bash
# OpenGL trace
./computrender --trace tests/ogl/trace/glxgears/glxgears.trace --frames 1

# D3D9 trace (Windows)
.\computrender.exe --trace tests\d3d\trace\fruitNinja\FruitNinja.trace --frames 2
```

### Simulation Output

| Output | Description |
|--------|-------------|
| Standard output | Progress dots (10K cycles), 'B' per draw call, frame timing |
| `frame0000.cm.ppm` | Rendered frame (perfmodel, cycle-accurate) |
| `frame0000.bm.png` | Rendered frame (bhavmodel, fast emulation) |
| `stats.frames.csv.gz` | Per-frame statistics |
| `stats.batches.csv.gz` | Per-batch (draw call) statistics |
| `stats.general.csv.gz` | Accumulated statistics at configurable cycle rate |
| `signaltrace.txt` | Signal trace for debugging (if enabled) |

---

## Testing & Regression

### Automated Regression

**Windows:**
```powershell
& .\tools\script\regression\regression.ps1
```

**Linux:**
```bash
bash tools/script/regression/regression.sh
```

See `tools/script/regression/regression_list` for the list of test cases.

### Manual Verification

```bash
cd _BUILD_/arch
./computrender --trace ../../tests/ogl/trace/glxgears/glxgears.trace --frames 1

# Byte-for-byte comparison against reference
cmp frame0000.cm.ppm ../../tests/ogl/trace/glxgears/glxgears.ppm && echo "PASS" || echo "FAIL"
```

### Available Test Traces

| Trace | Path |
|-------|------|
| glxgears (OGL) | `tests/ogl/trace/glxgears/glxgears.trace` |

### Known Platform Differences

| Item | Notes |
|------|-------|
| PNG output (bhavmodel) | Requires `libpng-dev` on Linux; uses GDI+ on Windows |
| Cycle counts | Minor variations (< 0.1%) across compilers are expected |

---

## Trace Driver Architecture

### Overview

```
    ┌───────────────────────────────────────────────────────────────┐
    │                       Input Traces                           │
    │                                                               │
    │  .trace (apitrace binary, Snappy-compressed)                  │
    │  └─ Auto-detected: OpenGL or D3D9                             │
    │                                                               │
    │  .metaStream.txt.gz (Pre-compiled MetaStreams)                │
    └──────────────┬────────────────────────────┬──────────────────┘
                   │                            │
    ┌──────────────▼────────────────────────────▼──────────────────┐
    │                     computrender main()                            │
    │                   (arch/computrender.cpp)                  │
    │                                                               │
    │  .trace → API auto-detect → TraceDriver instantiation        │
    └──────┬──────────┬──────────┬────────────────────────────────────┘
           │          │          │
    ┌──────▼───┐ ┌────▼────┐ ┌──▼───┐
    │ Apitrace │ │Apitrace │ │ Meta │
    │   OGL   │ │  D3D9   │ │Driver│
    └──────┬───┘ └────┬────┘ └──┬───┘
           │          │         │
    ┌──────▼──────────▼─────────│───────────│───────────────────────┐
    │                           │           │                       │
    │   OGL/GAL/HAL Pipeline    │           │  (future)             │
    │                           │           │                       │
    │   GLExec → OGL2 →        │           │                       │
    │   GAL → HAL              │           │                       │
    │         │                 │           │                       │
    │   ┌─────▼─────────────────▼───────────▼──────────────────┐   │
    │   │              MetaStream Buffer                        │   │
    │   │   (HAL::metaStreamBuffer[] — circular queue)          │   │
    │   └─────────────────────────┬────────────────────────────┘   │
    └─────────────────────────────┼────────────────────────────────┘
                                  │
    ┌─────────────────────────────▼────────────────────────────────┐
    │                    Simulation Models                         │
    │                                                               │
    │  BhavModel (bhavmodel)        PerfModel (perfmodel)         │
    │  - Fast emulation              - Cycle-accurate simulation   │
    │  - emulateCommandProcessor()   - clock() all MDUs per cycle  │
    │                                                               │
    │  GPU Pipeline: CommandProcessor → Streamer → Clipper →       │
    │  Rasterizer → UnifiedShader → Z/Stencil → ColorWrite → DAC  │
    └──────────────────────────────────────────────────────────────┘
```

### Apitrace OGL Call Flow

```
computrender::main()
  │
  ├─ fileExtensionTester("trace") + detectApiType() == "gl"
  │
  ▼
TraceDriverApitraceOGL(inputFile, HAL, startFrame)
  │
  │  Constructor:
  │  ├─ ApitraceParser::open(traceFile)
  │  │   ├─ SnappyStream::open()           // verify 'at' header, init chunk reader
  │  │   ├─ readVarUInt(version)            // trace format version (typically 4-6)
  │  │   ├─ readProperties()               // if version >= 6: OS, driver info
  │  │   └─ Signature cache initialized     // for efficient repeated call lookup
  │  │
  │  ├─ ogl::initOGL(driver, startFrame)   // init OGL/GAL subsystem
  │  └─ Skip to startFrame (count SwapBuffers events)
  │
  │  nxtMetaStream():
  │  ├─ HAL::nextMetaStream() → if MetaStream available, return it
  │  │
  │  ├─ (no MetaStream pending) → read and dispatch next call:
  │  │   ├─ ApitraceParser::readEvent(evt)
  │  │   │   ├─ Read event type (CALL_ENTER / CALL_LEAVE)
  │  │   │   ├─ Read/cache call signature (function name + arg names)
  │  │   │   └─ Read call details: typed arguments (uint/float/blob/enum/array)
  │  │   │
  │  │   ├─ ApitraceCallDispatcherOGL::dispatchCall(evt)
  │  │   │   ├─ Extract typed args: asUInt(A(0)), asFloat(A(1)), asVoidPtr(A(2))...
  │  │   │   └─ Call OGL entry point: OGL_glClear(mask), OGL_glVertex3f(x,y,z), etc.
  │  │   │       │
  │  │   │       ▼
  │  │   │   OGLEntryPoints → GALDeviceImp → HAL::writeGPURegister()
  │  │   │       └─ RegisterWriteBuffer::flush() → MetaStream enqueued
  │  │   │
  │  │   └─ SwapBuffers → ogl::swapBuffers() → end-of-frame event
  │  │
  │  └─ return MetaStream to simulation
```

### Apitrace D3D9 Call Flow

```
computrender::main()
  │
  ├─ fileExtensionTester("trace") + detectApiType() == "d3d9"
  │
  ▼
TraceDriverApitraceD3D(inputFile, HAL, startFrame)
  │
  │  Constructor:
  │  ├─ ApitraceParser::open(traceFile)
  │  │   ├─ SnappyStream::open()           // verify 'at' header, init chunk reader
  │  │   └─ readVarUInt(version)            // trace format version (typically 4-6)
  │  │
  │  ├─ D3D9::initialize()             // init D3D9/GAL subsystem
  │  └─ ApitraceCallDispatcherD3D setup
  │
  │  nxtMetaStream():
  │  ├─ HAL::nextMetaStream()  →  if MetaStream available, return it
  │  │
  │  ├─ (no MetaStream pending) → read and dispatch next call:
  │  │   ├─ ApitraceParser::readEvent(evt)
  │  │   │   ├─ Read event type (CALL_ENTER / CALL_LEAVE)
  │  │   │   ├─ Read/cache call signature
  │  │   │   └─ Read typed arguments (uint/float/blob/enum/array/opaque)
  │  │   │
  │  │   ├─ ApitraceCallDispatcherD3D::dispatchCall(evt, state)
  │  │   │   ├─ Extract typed args via MA() macro (skips COM 'this' ptr)
  │  │   │   ├─ Map opaque pointers via OpaquePointerTracker
  │  │   │   └─ Call D3D9 interface: dev->SetRenderState(...), dev->DrawPrimitive(...)
  │  │   │       │
  │  │   │       ▼
  │  │   │   D3D9 → GALDeviceImp → HAL::writeGPURegister()
  │  │   │       └─ RegisterWriteBuffer::flush() → MetaStream enqueued
  │  │   │
  │  │   └─ Present() → end-of-frame event
  │  │
  │  └─ return MetaStream to simulation
```

### MetaStream Trace Call Flow

```
computrender::main()
  │
  ├─ (else clause — not apitrace extension)
  │
  ▼
TraceDriverMeta(ProfilingFile, startFrame, headerStartFrame)
  │
  │  nxtMetaStream():
  │  ├─ Phase: PREINIT → load initial GPU register state
  │  ├─ Phase: LOAD_REGS → load register writes from header
  │  ├─ Phase: LOAD_SHADERS → load shader programs
  │  ├─ Phase: SIMULATION → read MetaStreams from file
  │  │   └─ Reads binary MetaStream directly from .gz file
  │  │      (No API translation needed — pre-compiled)
  │  │
  │  └─ return MetaStream to simulation
```

### Simulation Consumption

All trace drivers produce `cgoMetaStream` objects consumed by the simulation models.

**Bhavmodel (Fast Emulation):**
```
BhavModel::simulationLoop()
  └─ Loop: TraceDriver->nxtMetaStream() → GpuBMdl.emulateCommandProcessor(ms)
      └─ Decode MetaStream → update GPU state → emulate pipeline on DRAW
```

**Perfmodel (Cycle-Accurate):**
```
PerfModel::simulationLoop()
  └─ Cycle loop: clock all MDUs per cycle
      └─ CommandProcessor.clock() → TraceDriver->nxtMetaStream() → process
```

---

## Apitrace Binary Format

```
file = 'a' 't' chunk*                // Snappy-compressed chunks

chunk = compressed_length(uint32_LE) compressed_data(bytes)

trace = version_no event*            // after decompression

event = 0x00 [thread_no] call_sig call_detail*    // CALL_ENTER
      | 0x01 call_no call_detail*                  // CALL_LEAVE

call_sig = id function_name count arg_name*        // first occurrence
         | id                                       // follow-on (cached)

call_detail = 0x00                                  // end
            | 0x01 arg_no value                     // argument
            | 0x02 value                            // return value

value = 0x00         // null
      | 0x01         // false
      | 0x02         // true
      | 0x03 uint    // negative int
      | 0x04 uint    // positive int
      | 0x05 float32 // float
      | 0x07 string  // string (length-prefixed)
      | 0x08 blob    // binary blob (length-prefixed)
      | 0x09 enum_sig value  // enum
      | 0x0b count value*    // array
```

---

## Implementation Files

### Core Files

| File | Purpose |
|------|---------|
| `arch/computrender.cpp` | Main entry point (`main()`) |
| `arch/common/GPUReg.h` | GPU register & command definitions |
| `arch/common/GPUType.h` | GPU data types (F32, U32, etc.) |
| `arch/common/params/archParams.csv` | Primary configuration file |
| `arch/utils/ConfigLoader.h` | Configuration file parser |

### Trace Drivers

| File | Purpose |
|------|---------|
| `driver/utils/TraceDriver/TraceDriverBase.h` | Base class (frame limiting, virtual destructor) |
| `driver/utils/TraceDriver/TraceDriverApitraceOGL.h/cpp` | OGL apitrace trace driver |
| `driver/utils/TraceDriver/TraceDriverApitraceD3D.h/cpp` | D3D9 apitrace trace driver |
| `driver/utils/TraceDriver/TraceDriverMeta.h/cpp` | MetaStream trace driver |

### Apitrace Parser & Dispatchers

| File | Purpose |
|------|---------|
| `driver/utils/ApitraceParser/ApitraceParser.h/cpp` | Binary format parser (Snappy, varuint, typed Values) |
| `driver/utils/ApitraceParser/ApitraceCallDispatcherOGL.h/cpp` | OGL call dispatcher (111 GL calls) |
| `driver/utils/ApitraceParser/ApitraceCallDispatcherD3D.h/cpp` | D3D9 call dispatcher (80+ calls) |
| `thirdparty/snappy-1.1.10/` | Snappy decompression library |

### Models

| File | Purpose |
|------|---------|
| `arch/bhavmodel/bhavmodel.hpp` | Behavior model top-level |
| `arch/perfmodel/perfmodel.h` | Functional model top-level |
| `arch/perfmodel/common/base/MduBase.h` | Perfmodel module base class |
| `arch/perfmodel/common/base/GPUSignal.h` | Signal class for inter-module comm |
| `arch/perfmodel/CommandProcessor/MetaStream.h` | MetaStream protocol definition |

### Driver Stack

| File | Purpose |
|------|---------|
| `driver/hal/HAL.h/cpp` | Hardware abstraction layer |
| `driver/gal/GAL/Interface/` | GAL abstract interfaces |
| `driver/ogl/OGL2/` | OpenGL 2.0 implementation |
| `driver/d3d/D3D9/` | D3D9 implementation |

---

## MetaStream Protocol

All trace paths converge at the `cgoMetaStream` interface:

```cpp
struct cgoMetaStream {
    enum Type { REG_WRITE, REG_READ, MEM_WRITE, MEM_READ, COMMAND, EVENT };
    Type type;
    U32  regId;        // GPU register ID (from GPUReg.h)
    U32  subRegId;     // Sub-register index
    GPURegData data;   // Register data (union of U32/F32/bool/etc.)
};
```

| Type | Meaning | Example |
|------|---------|---------|
| `REG_WRITE` | Write GPU register | Set blend mode, shader program, texture params |
| `MEM_WRITE` | Write GPU memory | Upload texture data, vertex buffers |
| `COMMAND` | Execute GPU command | `GPU_DRAW`, `GPU_SWAPBUFFERS`, `GPU_CLEARCOLORBUFFER` |
| `EVENT` | Synchronization | Frame boundary, fence |

---

## Format Comparison

| Feature | MetaStream | Apitrace (OGL) | Apitrace (D3D9) |
|---------|------------|----------------|------------------|
| **Format** | Binary | Binary (Snappy) | Binary (Snappy) |
| **Compression** | gzip | Snappy | Snappy |
| **Buffer data** | Embedded | Embedded blobs | Embedded blobs |
| **API level** | GPU commands | OpenGL calls | D3D9 calls |
| **Translation** | None (direct) | Full stack | Full stack |
| **Speed** | Fast (no translation) | Medium | Medium |
| **Standard** | computrender-specific | Community standard | Community standard |
| **Tools** | (removed) | apitrace CLI | apitrace CLI |

---

## Implemented GL Call Categories

| Category | Calls | Examples |
|----------|-------|---------|
| State management | 7 | glEnable, glDisable, glShadeModel, glCullFace |
| Clear | 4 | glClear, glClearColor, glClearDepth, glClearStencil |
| Viewport/Scissor | 3 | glViewport, glScissor, glDepthRange |
| Matrix | 13 | glMatrixMode, glLoadIdentity, glOrtho, glTranslatef, ... |
| Immediate mode | 14 | glBegin/glEnd, glVertex*, glColor*, glNormal*, glTexCoord* |
| Vertex arrays | 8 | glEnableClientState, glVertexPointer, glDrawArrays/Elements |
| VBO (ARB) | 6 | glBindBuffer, glBufferData, glVertexAttribPointer |
| Texture | 18 | glBindTexture, glTexImage2D, glTexParameter*, glTexEnv* |
| Lighting | 8 | glLightfv, glMaterialfv, glColorMaterial |
| Depth/Stencil/Blend | 11 | glDepthFunc, glStencilOp, glBlendFunc, glAlphaFunc |
| Fog | 4 | glFogf, glFogi, glFogfv, glFogiv |
| ARB Programs | 5 | glBindProgramARB, glProgramStringARB, glProgramEnvParameter* |
| Push/Pop | 2 | glPushAttrib, glPopAttrib |
| Display Lists | 3 | glNewList, glEndList, glCallList |
| Misc | 2 | glFlush, glGetString |
| **Total** | **111** | |

---

## Adding Support for New API Calls

**OGL:** Edit `driver/utils/ApitraceParser/ApitraceCallDispatcherOGL.cpp`, add a handler using `A()` for argument access.

**D3D9:** Edit `driver/utils/ApitraceParser/ApitraceCallDispatcherD3D.cpp`, add a handler using `MA()` for method argument access (skips COM `this` pointer). See `APITRACE_DEV_SKILL.md` for detailed patterns.

---

## Adding a New Trace Format

1. **Create parser** in `driver/utils/NewParser/`:
   Read the file format, extract API calls and arguments.

2. **Create trace driver** in `driver/utils/TraceDriver/TraceDriverNew.h/cpp`:
   Inherit from `cgoTraceDriverBase`. Implement `startTrace()`, `nxtMetaStream()`, `getTracePosition()`.

3. **Register in computrender.cpp**:
   Add `#include "TraceDriverNew.h"`, add extension check, instantiate the driver.

4. **Update build system**:
   Add source to `arch/CMakeLists.txt`, add include paths, link dependencies.

---

## Known Limitations

| Limitation | Impact |
|-----------|--------|
| OpenGL version | computrender supports GL 1.4-2.0 subset; traces using GL 3.0+ features will fail |
| No GLSL/HLSL shaders | Only ARB vertex/fragment programs supported; see [Shader Compiler Modernization](#shader-compiler-modernization-glsl--hlsl--llvm) for the LLVM migration plan |
| D3D9 only | D3D10/11/12 traces detected but not supported |
| 32-bit build | Win32 platform has 2GB address space limit; large traces may OOM |
| Thread support | Single-threaded only; multi-threaded traces may misbehave |
| D3D frame rendering | Some pixel format handling incomplete (GPU_RGBA32F bytesPixel) |

---

## Remaining Work

### Rendering Improvements (Deferred)

- [ ] Fix `bytesPixel` for `GPU_RGBA32F` in bhavmodel `dumpFrame()` — D3D frames render black
- [ ] Multiple simultaneous Lock/Unlock support (current memcpy uses first-pending heuristic)
- [ ] Image quality validation for D3D9 traces (pixel correctness, format handling)

### Shader Compiler Modernization (Next Major Milestone)

- [ ] Drop legacy ARB-Program-only shader path; add GLSL and HLSL front-ends
- [ ] Integrate LLVM as the compiler back-end targeting the CG1 ISA
- [ ] See **[Shader Compiler Modernization](#shader-compiler-modernization-glsl--hlsl--llvm)** section below for the full plan

### Coverage Expansion

- [ ] Additional D3D9 calls: GetFunction, GetDevice, BeginStateBlock, EndStateBlock, Capture, Apply
- [ ] Additional GL calls for complex traces (GL 2.0+ GLSL shaders, FBO, MRT)
- [ ] Enum signature name preservation from apitrace (currently numeric-only)

### Regression Tests

- [ ] Add D3D9 regression test entries to regression_list
- [ ] Convert more legacy GLInterceptor test traces to apitrace format
- [ ] Add tolerance-based image comparison for D3D9 tests

### Performance & Robustness

- [ ] 64-bit build support for large traces
- [ ] Performance tuning for large trace files
- [ ] Better error reporting for unsupported API calls

---

## Shader Compiler Modernization: GLSL / HLSL + LLVM

### Motivation

The simulator currently only supports **ARB vertex/fragment programs** (OpenGL `GL_ARB_vertex_program` / `GL_ARB_fragment_program` extensions). These are a fixed-function-era assembly language — no high-level shading language is supported. This limits the simulator to very old OpenGL 1.4-era traces and prevents running any real-world application that uses GLSL (OpenGL 2.0+) or HLSL (Direct3D 9 Shader Model 2.0/3.0).

The goal is to replace the hand-rolled ARB parser + custom code generator with a modern, LLVM-based compiler stack that can accept **GLSL** and **HLSL** source and compile them down to the simulator's **CG1 ISA** binary.

### Current Shader Pipeline (Status Quo)

```
  ┌─── OpenGL Path ─────────────────────────────────────────────────────────┐
  │                                                                         │
  │  glProgramStringARB(GL_VERTEX_PROGRAM_ARB, source)                      │
  │         │                                                               │
  │         ▼                                                               │
  │  OGL14/ARBP/  (Flex/Bison parser, now pre-generated .gen files)         │
  │  ProgramObject  →  GenericInstruction (HW-independent IR)               │
  │         │                                                               │
  │         ▼                                                               │
  │  ShaderInstructionTranslator  (GenericInstruction → cgoShaderInstr)     │
  │         │                                                               │
  │         ▼                                                               │
  │  GALShaderProgramImp.setCode()  →  CG1 bytecode (16-byte instrs)       │
  │         │                                                               │
  │         ▼                                                               │
  │  HAL MetaStream WRITE  →  GPU instruction memory                        │
  └─────────────────────────────────────────────────────────────────────────┘

  ┌─── Direct3D 9 Path ────────────────────────────────────────────────────┐
  │                                                                         │
  │  CreateVertexShader / CreatePixelShader (DWORD bytecode)                │
  │         │                                                               │
  │         ▼                                                               │
  │  D3D9/ShaderTranslator/  (IR.h → IRTranslator → IRBuilder)             │
  │  D3D DWORD tokens  →  IR tree (visitor pattern)  →  cgoShaderInstr     │
  │         │                                                               │
  │         ▼                                                               │
  │  NativeShader  →  CG1 bytecode  →  HAL MetaStream WRITE                │
  └─────────────────────────────────────────────────────────────────────────┘

  ┌─── Simulator Execution ────────────────────────────────────────────────┐
  │                                                                         │
  │  CG1 ISA bytecode (16 bytes/instruction) loaded into shader memory      │
  │         │                                                               │
  │         ▼                                                               │
  │  bhavmodel: bmUnifiedShader.loadShaderProgram() → decode → execute      │
  │  perfmodel: ShaderFetch → ShaderDecodeExecute (cycle-accurate)          │
  │                                                                         │
  │  Register model: IN, OUT, PARAM (constants), TEMP, ADDR, PRED banks    │
  │  ~56 opcodes: ALU (ADD, MUL, MAD, DP3, DP4, ...), SFU (RCP, RSQ,      │
  │    SIN, COS, EX2, LG2), TEX (TEX, TXB, TXP, TXL), control (JMP,      │
  │    KIL, CMP, SETPXX, ANDP), misc (MOV, LDA, DDX, DDY, END)            │
  └─────────────────────────────────────────────────────────────────────────┘
```

### Key Files in the Current System

| Path | Purpose |
|------|---------|
| `driver/ogl/OGL14/ARBP/` | ARB program parser (Flex/Bison .gen), `GenericInstruction`, `ShaderInstructionTranslator` |
| `driver/ogl/OGL2/ARBProgramObject/` | OGL2-level ARB program management (`ARBProgramObject`, `ARBProgramTarget`, `ARBProgramManager`) |
| `driver/gal/GAL/Interface/GALShaderProgram.h` | Abstract shader program interface (CG1 bytecode + constants) |
| `driver/gal/GALx/Implementation/GALxCompiledProgramImp.h` | Compiled program storage (bytecode, constant bank, texture usage) |
| `driver/d3d/D3D9/*/ShaderTranslator/` | D3D9 DWORD-token → IR → CG1 translator (`IR.h`, `IRTranslator`, `IRBuilder`) |
| `arch/bhavmodel/UnifiedShader/ShaderInstr.h` | CG1 ISA definition: `ShOpcode` enum, `cgoShaderInstr` class (16-byte encoding) |
| `arch/bhavmodel/UnifiedShader/bmUnifiedShader.h` | Behavioral shader execution engine |
| `arch/perfmodel/UnifiedShader/ShaderDecodeExecute*.h` | Cycle-accurate shader decode + execute |
| `arch/utils/CGAPI/ComputeGeneralLanguage.h` | CGL interface: `cglCompileARBProgram()`, `cglLoadVertexShader()` |
| `arch/utils/CGASM/` | Standalone CG1 assembler / disassembler / optimizer tools |

### CG1 ISA Summary

| Property | Value |
|----------|-------|
| Instruction size | 16 bytes (128 bits) |
| Encoding | Two 64-bit halves: Lo64 (opcode, flags, banks, masks, rel-addressing) + Hi64 (registers, swizzles or immediate) |
| Register banks | IN (0), OUT (1), PARAM (2), TEMP (3), ADDR (4), PARAM2 (5), IMM (6), PRED (0xA) |
| Operands | Up to 3 source operands + 1 result; per-operand swizzle, negate, absolute |
| Write masking | 4-bit XYZW mask on result |
| Predication | Per-instruction predicate register with inversion |
| Special | Relative addressing mode for constant bank access |
| Opcode count | ~56 defined (0x00–0x50); see `ShOpcode` enum |

### Target Architecture

```
                              GLSL source            HLSL source
                                  │                      │
                                  ▼                      ▼
                          ┌──────────────┐      ┌──────────────┐
                          │  glslang or  │      │   DXC/HLSL   │
                          │  Mesa NIR    │      │   front-end   │
                          └──────┬───────┘      └──────┬───────┘
                                 │                      │
                                 ▼                      ▼
                          ┌────────────────────────────────────┐
                          │         SPIR-V (optional)          │
                          │   Common intermediate for both     │
                          │   front-ends (via SPIRV-Tools)     │
                          └────────────────┬───────────────────┘
                                           │
                                           ▼
                          ┌────────────────────────────────────┐
                          │          LLVM IR Module            │
                          │   SPIR-V → LLVM IR translator      │
                          │   (or direct glslang → LLVM)       │
                          └────────────────┬───────────────────┘
                                           │
                                           ▼
                          ┌────────────────────────────────────┐
                          │       CG1 LLVM Back-End            │
                          │                                    │
                          │  • CG1TargetMachine                │
                          │  • CG1InstrInfo (map LLVM → CG1)   │
                          │  • CG1RegisterInfo (IN/OUT/TEMP/    │
                          │    PARAM/ADDR/PRED banks)           │
                          │  • CG1AsmPrinter (emit 16-byte     │
                          │    encoded instructions)            │
                          │  • SelectionDAG / GlobalISel        │
                          │    lowering for TEX, KIL, DDX/DDY   │
                          └────────────────┬───────────────────┘
                                           │
                                           ▼
                          ┌────────────────────────────────────┐
                          │   CG1 ISA bytecode (unchanged)     │
                          │   16-byte encoded instructions     │
                          │   loaded via existing HAL path     │
                          └────────────────────────────────────┘
```

### Implementation Plan

#### Phase 1 — LLVM CG1 Back-End (Foundation)

Create a custom LLVM target back-end that emits CG1 ISA bytecode. This is the core piece that everything else depends on.

| Task | Details |
|------|---------|
| **CG1 target triple** | Define `cg1-none-none`; register with LLVM `TargetRegistry` |
| **`CG1TargetMachine`** | Subclass `LLVMTargetMachine`; data layout for 32-bit float vector machine |
| **`CG1RegisterInfo`** | Model the CG1 register banks: 16 IN regs, 16 OUT regs, 512 PARAM regs, 32 TEMP regs, 4 ADDR regs, 32 PRED regs |
| **`CG1InstrInfo`** | TableGen `.td` file mapping LLVM opcodes to CG1 ISA opcodes (ADD, MUL, MAD, DP3, DP4, MOV, etc.) |
| **`CG1FrameLowering`** | Trivial — no stack; TEMP registers are the "stack" |
| **`CG1ISelLowering`** | Lower LLVM IR intrinsics to CG1 special instructions: `@llvm.cg1.tex`, `@llvm.cg1.kill`, `@llvm.cg1.ddx`, `@llvm.cg1.ddy`, `@llvm.cg1.sincos` |
| **`CG1MCCodeEmitter`** | Emit the 16-byte `cgsInstrFieldsLo64` + `cgsInstrFieldsHi64` binary encoding |
| **Standalone test** | `llc -march=cg1 < test.ll` produces `.cg1bin` matching hand-assembled output from `arch/utils/CGASM/` |

New directory: `arch/compiler/llvm-cg1/` (or integrated into a forked LLVM `lib/Target/CG1/`).

#### Phase 2 — GLSL Front-End

Accept GLSL shader source (vertex + fragment) from `glShaderSource` / `glCompileShader` / `glLinkProgram` calls.

| Task | Details |
|------|---------|
| **GLSL parsing** | Use [glslang](https://github.com/KhronosGroup/glslang) to compile GLSL → SPIR-V |
| **SPIR-V → LLVM IR** | Use a SPIR-V reader (e.g., [SPIRV-LLVM-Translator](https://github.com/AcademySoftwareFoundation/SPIRV-LLVM-Translator)) to convert SPIR-V modules to LLVM IR |
| **Built-in functions** | Map GLSL built-ins (`texture2D`, `normalize`, `reflect`, `clamp`, `mix`, etc.) to CG1 intrinsics or instruction sequences |
| **OGL driver hooks** | Add `glCreateShader`, `glShaderSource`, `glCompileShader`, `glAttachShader`, `glLinkProgram`, `glUseProgram` to the OGL2 dispatch tables (new handler files alongside existing ARBProgramObject) |
| **Uniform handling** | Map GLSL uniforms to CG1 PARAM bank registers; `glUniform*` writes go through existing `HAL::writeGPUReg(GPU_VERTEX_CONSTANT_*)` path |
| **Varying linkage** | Link VS outputs to FS inputs via the existing IN/OUT banks; respect `gl_Position` → position output mapping |
| **Apitrace support** | GLSL calls are already captured by apitrace tools — no changes needed on the tracing side |

New directories:
- `driver/ogl/OGL2/GLSLObject/` — GLSL shader/program object management
- `arch/compiler/` — LLVM pipeline driver and SPIR-V utilities

#### Phase 3 — HLSL Front-End (D3D9 Shader Model 2.0/3.0)

Replace the current D3D9 DWORD-token → IR → CG1 hand-translation with an LLVM-based pipeline.

| Task | Details |
|------|---------|
| **HLSL source path** | For traces with embedded HLSL: use [DXC](https://github.com/microsoft/DirectXShaderCompiler) to compile HLSL → SPIR-V (via `-spirv` flag), then reuse Phase 2 SPIR-V → LLVM → CG1 pipeline |
| **D3D bytecode path** | For traces with only compiled SM2/SM3 bytecode: keep existing `IRTranslator` as a fallback, or write a DXBC → SPIR-V lifter |
| **SM3 features** | Dynamic branching (`if`/`else`/`loop`) maps to CG1 `JMP` + predication; derivative instructions map to `DDX`/`DDY` |
| **Constant registers** | D3D constant registers (c0–c255) map directly to CG1 PARAM bank |

Modified directories:
- `driver/d3d/D3D9/D3DImplement/ShaderTranslator/` — add LLVM code path alongside existing translator
- `driver/d3d/D3D9/D3DControllers/ShaderTranslator/` — same

#### Phase 4 — Optimization & ISA Extensions

| Task | Details |
|------|---------|
| **LLVM optimization passes** | Standard `-O2` passes (dead code elimination, constant propagation, loop unrolling) run on LLVM IR before CG1 lowering |
| **Register allocation** | LLVM's register allocator handles TEMP bank allocation (replaces current `Scheduler` and `maxAliveTemps` tracking in ARBP) |
| **Instruction scheduling** | LLVM scheduling pass replaces the hand-written `ShaderOptimization` class |
| **ISA extensions** | Extend `ShOpcode` enum for new instructions as needed (integer ops, bitwise ops for GLSL `ivec`/`uvec` types) |
| **Shader cache** | Cache compiled CG1 binaries keyed by shader source hash to avoid re-compilation |

### Integration Points (What Stays, What Changes)

| Component | Status | Notes |
|-----------|--------|-------|
| `cgoShaderInstr` (128-bit encoding) | **Unchanged** | ISA binary format is the compilation *target* — no changes |
| `GALShaderProgram` / `GALShaderProgramImp` | **Unchanged** | Still stores CG1 bytecode + constants |
| `HAL` MetaStream shader loading | **Unchanged** | `cglLoadVertexShader()` / `cglLoadFragmentShader()` still write CG1 bytecode |
| `bmUnifiedShader` / `cmoShaderDecExe` | **Unchanged** | Shader execution decodes CG1 bytecode — compiler-agnostic |
| `OGL14/ARBP/` parser + translator | **Deprecated** | Replaced by glslang → SPIR-V → LLVM → CG1. Keep as legacy fallback |
| `D3D9/ShaderTranslator/` IR pipeline | **Deprecated** | Replaced by DXC/DXBC → SPIR-V → LLVM → CG1. Keep as fallback for SM2 bytecode |
| `ARBProgramObject` | **Kept** | Still handles `glProgramStringARB` — route through LLVM if desired, or keep existing path |
| `driver/utils/generated/*.gen` | **Kept** | Flex/Bison-generated tables for ARB parsing — still needed for legacy path |
| `arch/utils/CGASM/` | **Kept** | Standalone assembler/disassembler remain useful for debugging |

### New Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| LLVM | ≥ 17.0 | Compiler infrastructure, optimization passes, CG1 back-end hosting |
| glslang | ≥ 14.0 | GLSL → SPIR-V compiler (Khronos reference) |
| SPIRV-Tools | ≥ 2024.1 | SPIR-V validation and optimization |
| SPIRV-LLVM-Translator | matching LLVM | SPIR-V ↔ LLVM IR bidirectional translation |
| DXC (optional) | latest | HLSL → SPIR-V compilation |

### Build Configuration (CMake)

```cmake
# In CMakeLists.txt — new options for the compiler subsystem
option(CG_ENABLE_LLVM_COMPILER  "Build LLVM-based shader compiler (requires LLVM)"  OFF)
option(CG_ENABLE_GLSL           "Enable GLSL front-end (requires glslang)"          OFF)
option(CG_ENABLE_HLSL           "Enable HLSL front-end (requires DXC)"              OFF)

if (CG_ENABLE_LLVM_COMPILER)
    find_package(LLVM REQUIRED CONFIG)
    add_subdirectory(arch/compiler/llvm-cg1)
endif()

if (CG_ENABLE_GLSL)
    find_package(glslang REQUIRED)
    # links glslang::glslang, glslang::SPIRV
endif()
```

When `CG_ENABLE_LLVM_COMPILER` is OFF, the build uses the existing ARB parser and D3D IR translator only — no LLVM dependency. This keeps the default build lightweight.

---


## Configuration

The primary configuration file is `arch/common/params/archParams.csv`. Key sections:

| Section | Purpose |
|---------|---------|
| `[SIMULATOR]` | Simulation frames, cycles, statistics settings |
| `[GPU]` | GPU architecture params (shader units, ALUs, stamp units) |
| `[COMMANDPROCESSOR]` | Command queue settings |
| `[MEMORYCONTROLLER]` | Memory bus width, frequency, burst size |
| `[STREAMER]` | Vertex fetch configuration |
| `[VERTEXSHADER]` / `[FRAGMENTSHADER]` | Shader thread counts, latencies |
| `[RASTERIZER]` | Tile size, scan conversion, HiZ settings |
| `[ZSTENCILTEST]` | Z/stencil buffer format |
| `[COLORWRITE]` | Framebuffer format |
| `[DISPLAYCONTROLLER]` | Display resolution, refresh |

Config variants include: `CG1GPUx1.ini` (1 shader), `CG1GPUx2.ini` (2 shaders), `CG1GPU_MSAA4x.ini`, `CG1GPU_mcv2.ini` (V2 memory controller).

---

## Third-Party Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| zlib | 1.2.13 | Trace file compression/decompression |
| Snappy | 1.1.10 | Apitrace Snappy decompression |
| SDL2 | 2.0.8 | Windowing and display |
| RapidJSON | 1.1.0 | JSON config parsing |
| TinyXML2 | 8.0.0 | XML parsing |

All third-party libraries are auto-downloaded and built via CMake (`thirdparty/`).

