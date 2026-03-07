# driver/utils — Driver Utilities

Shared utilities and infrastructure used by the computrender driver layer. Provides trace parsing, API code generation, memory management, logging, and OpenGL/D3D header support.

## Directory Structure

```
driver/utils/
├── ApitraceParser/         # Apitrace binary format parser & call dispatchers
├── GALTester/              # [DEAD] Legacy GAL test project
├── GL/                     # OpenGL headers and type definitions
├── log/                    # Logging facility (LogObject, IncludeLog)
├── MetaTraceGenerator/     # MetaStream trace format definitions & tools
├── OGLApiCodeGen/          # OpenGL API code generator (build tool)
├── Parsers/                # GL header parsers (dependency of OGLApiCodeGen)
├── TraceDriver/            # Trace driver implementations (Apitrace, Meta)
│
├── BufferDescriptor.h/cpp  # GPU memory buffer descriptor management
├── DArray.h/cpp            # Cluster-based dynamic array allocator
├── GLJumpTable.h/cpp       # OpenGL function pointer jump table
├── GLResolver.h/cpp        # OpenGL function name/ID resolution
├── MemoryRegion.h/cpp      # GPU memory region management
├── UserCallTable.h/cpp     # [DEAD] Legacy user callback table
├── zfstream.h/cpp          # gzip-compressed file stream (wraps zlib)
├── zlib.h / zconf.h        # zlib headers (local copies)
```

---

## Component Details

### TraceDriver/ — Trace Driver Implementations
**Status: ACTIVELY USED** — Core simulation infrastructure.

Provides the abstract base class `cgoTraceDriverBase` and concrete implementations for driving the simulator from captured API traces.

| File | Description |
|------|-------------|
| `TraceDriverBase.h` | Abstract base class defining the `startTrace()`, `nxtMetaStream()`, `getTracePosition()` interface. All trace drivers inherit from this. |
| `TraceDriverApitraceOGL.h/cpp` | Drives simulation from **apitrace** `.trace` files containing OpenGL calls. Parses binary format via `ApitraceParser` and dispatches calls through the OGL driver stack. |
| `TraceDriverApitraceD3D.h/cpp` | Drives simulation from **apitrace** `.trace` files containing D3D9 calls. Routes calls through `ApitraceCallDispatcherD3D` into the D3D9 GAL layer. |
| `TraceDriverMeta.h/cpp` | Drives simulation from pre-translated **MetaStream** trace files (`.tracefile.gz`). Bypasses the API driver entirely — reads binary GPU command streams directly. Handles frame skipping, register caching, and shader program preloading. |

**Used by:** `arch/computrender.cpp` (main entry), `arch/perfmodel/PerfModel`, `arch/bhavmodel/BhavModel`, `cmCommandProcessor`, `cmUnifiedShader`.

---

### ApitraceParser/ — Apitrace Binary Format Parser
**Status: ACTIVELY USED** — Built as the `ApitraceParser` CMake library, linked into `computrender`.

Parses the apitrace binary trace format (`.trace` files) with Snappy decompression, and dispatches parsed calls into the computrender driver layers.

| File | Description |
|------|-------------|
| `ApitraceParser.h/cpp` | Core parser. Reads apitrace binary format: signatures (call/enum/bitmask/struct), events (enter/leave), and typed `Value` objects (int, float, blob, array, struct, opaque pointers). Uses Snappy decompression. |
| `ApitraceCallDispatcherOGL.h/cpp` | Dispatches `CallEvent` objects to computrender **OpenGL** entry points (`OGL_gl*`). Includes type-safe helpers (`asUInt`, `asFloat`, `asVoidPtr`, etc.) for extracting GL types from apitrace `Value` objects. |
| `ApitraceCallDispatcherD3D.h/cpp` | Dispatches `CallEvent` objects to computrender **D3D9** interface methods (`AIDeviceImp9`). Includes `D3D9ObjectTracker` for mapping apitrace opaque pointers to live COM objects, and `D3D9DispatcherState` for maintaining session state. |
| `ApitraceToGLI.h` | Utility to convert apitrace `CallEvent` to GLI text format strings (for compatibility with legacy `TraceDriverOGL`/`GLExec` infrastructure). |

**Dependencies:** Snappy (thirdparty), GLResolver, GL headers.

---

### MetaTraceGenerator/ — MetaStream Trace Format & Tools
**Status: ACTIVELY USED** — Headers and sources compiled into `computrender` and `PerfModel`.

Defines the binary MetaStream trace file format and provides tools for generating/extracting MetaStream traces.

| File/Dir | Description |
|----------|-------------|
| `MetaStreamTrace.h` | Defines `MetaStreamHeader`, `MetaStreamParameters`, file signature, version, and header layout for `.tracefile.gz` files. Core format definition. |
| `traceExtractor/` | Standalone tool to extract register write sequences from MetaStream traces. Contains `RegisterWriteBufferMeta` (register write cache used during trace replay with frame skipping). Built as a sub-project. |
| ~~`traceTranslator/`~~ | **Removed.** Was `Api2MetaTranslator` — converted GLInterceptor traces to MetaStream. Depended on removed `TraceReader`. |
| `CMakeLists.txt` | Standalone CMake project (not part of main build). Has its own compiler setup for building the tools independently. |

**Used by:** `TraceDriverBase.h`, `TraceDriverMeta`, `computrender.hpp`, `ComputeGeneralLanguage`.

---

### OGLApiCodeGen/ — OpenGL API Code Generator
**Status: ACTIVELY USED** — Build tool that generates `.gen` files consumed by the main build.

Parses the OpenGL function declarations in `GL/OGL_functions_supported_by_CG1GPU.h` and generates C++ code fragments:

| Generated File | Consumer | Purpose |
|----------------|----------|---------|
| `APICall.gen` | `GLResolver.h` | Enum of all supported API call IDs (`APICall_glVertex2i`, etc.) |
| `GLFunctionNames.gen` | `GLResolver.cpp` | Function name string table |
| `GLConstantNames.gen` | `GLResolver.cpp` | OpenGL constant name/value mappings |
| `GLJumpTableFields.gen` | `GLJumpTable.h`, `UserCallTable.h` | Function pointer fields for the jump table struct |
| `GLJumpTableSL.gen` | `GLJumpTable.cpp` | Static loading of function pointers |
| `JumpTableWrapper.gen` | OGL driver | Wrapper functions for jump table dispatch |
| `PlayableCalls.gen` | Trace player | Playable call classifications |
| `SwitchBranches.gen` | Trace player | Switch-case branches for call dispatch |

**Dependencies:** `Parsers/` directory (GL header parsing library).

---

### ~~D3DApiCodeGen/~~ — D3D9 API Code Generator
**Status: REMOVED** — Build tool that generated D3D9 PIX trace dispatch code. Its output was consumed by the removed `D3DTraceCore`. Both `D3DApiCodeGen` and its output stubs have been deleted.

---

### GL/ — OpenGL Headers
**Status: ACTIVELY USED** — Provides GL types, constants, and function prototypes.

| File | Description |
|------|-------------|
| `glAll.h` | Umbrella header — includes `gl.h`, `glext.h`, and platform-specific WGL headers. |
| `gl.h` | Core OpenGL type and function declarations. |
| `glext.h` | OpenGL extension declarations. |
| `glATI.h` | ATI-specific GL extensions. |
| `mesa_wgl.h` | Mesa WGL (Windows GL) interface. |
| `wglext.h` | WGL extension declarations. |
| `GLIOthers.h` | Additional GLI-related declarations. |
| `OGL_functions_supported_by_CG1GPU.h` | **Input file for OGLApiCodeGen** — authoritative list of OpenGL functions the simulator supports. |

**Used by:** `GLJumpTable.h`, `UserCallTable.h`, `ApitraceCallDispatcherOGL.h`, OGLApiCodeGen, OGL driver, GALx, `ComputeGeneralLanguage`.

---

### Parsers/ — GL Header Parsing Library
**Status: ACTIVELY USED** — Exclusively by `OGLApiCodeGen`.

Library for parsing OpenGL C function declarations from header files. Extracts function signatures, parameter descriptions, and constant values.

| File | Description |
|------|-------------|
| `FuncExtractor.h/cpp` | Extracts function declarations from GL headers. |
| `FuncDescription.h/cpp` | Represents a parsed function signature. |
| `ParamDescription.h/cpp` | Represents a single function parameter. |
| `ConstExtractor.h/cpp` | Extracts `#define` constants from GL headers. |
| `ConstDescription.h/cpp` | Represents a parsed constant definition. |
| `SpecificExtractor.h/cpp` | Extracts computrender-specific annotations. |
| `StringToFuncDescription.h/cpp` | Converts string representations to `FuncDescription` objects. |
| `StringTokenizer.h/cpp` | General-purpose string tokenizer utility. |
| `NameModifiers.h/cpp` | Name transformation utilities. |

**Note:** This library has no consumers outside `OGLApiCodeGen`. If the code generator is ever replaced, this directory can be removed with it.

---

### log/ — Logging Facility
**Status: ACTIVELY USED** — Compiled into `computrender`.

| File | Description |
|------|-------------|
| `LogObject.h/cpp` | Multi-level logging class supporting 32 log levels (`[0..31]`). Can open log files, filter by level mask, and queue messages. |
| `IncludeLog.h/cpp` | Convenience wrapper providing a global `logfile()` accessor and predefined log levels: `Init`, `Panic`, `Warning`, `Debug`. |

**Used by:** `BufferDescriptor.cpp` and potentially other driver components. Provides the `includelog::logfile()` global logger.

---

### Root-Level Files

#### BufferDescriptor.h/cpp
**Status: ACTIVELY USED** — Compiled into `computrender`.

Manages GPU memory buffer descriptors. Each `BufferDescriptor` represents a contiguous region of GPU-addressable memory with an ID, start address, size, and optional deferred allocation. Supports data comparison (via checksum) and dump/show debugging. Works with `BufferManager` (factory/registry) and `MemoryRegion`.

#### MemoryRegion.h/cpp
**Status: ACTIVELY USED** — Compiled into `computrender`.

Manages raw memory regions for the simulated GPU memory system. `MemoryRegion` provides resizable, cluster-allocated storage via `DArray`. `MemoryManager` handles creation, serialization (store/restore to file), and lifecycle management of regions. Supports two modes: `MM_NORMAL` (in-memory) and `MM_VIRTUAL` (file-backed for large traces).

#### DArray.h/cpp
**Status: ACTIVELY USED** — Compiled into `computrender`.

Custom cluster-based dynamic array allocator using pre-allocated memory pools. Designed for high-frequency allocation scenarios (trace replay, buffer management). Provides `init()` for pool setup, and supports both object-level and data-level arena allocation with a bitmap allocator.

#### GLJumpTable.h/cpp
**Status: ACTIVELY USED** — Compiled into `computrender`.

Defines `GLJumpTable` struct containing function pointers for all supported OpenGL calls (fields from generated `GLJumpTableFields.gen`). Provides `loadGLJumpTable()` to populate the table from a DLL at runtime.

#### GLResolver.h/cpp
**Status: ACTIVELY USED** — Compiled into `computrender`.

Maps OpenGL function **names** to `APICall` enum IDs and vice versa. Also maps OpenGL constant names to integer values. Used by the trace replay system to identify which API function is being called. Supports wildcard pattern matching on function names.

**Used by:** OGL14 driver (`BaseObject.cpp`, `GLContext.cpp`), `ApitraceCallDispatcherOGL`.

#### zfstream.h/cpp + zlib.h + zconf.h
**Status: ACTIVELY USED** — Compiled into `PerfModel`.

C++ iostream-compatible wrapper around zlib for reading/writing gzip-compressed files. Provides `gzifstream` (input) and `gzofstream` (output) classes. Critical for reading compressed trace files (`.txt.gz`, `.tracefile.gz`).

**Used by:** `MetaStream.h`, `modelbase.h`, `perfmodel.h`, `cmFragmentFIFO`, `cmTextureProcessor`, `ComputeGeneralLanguage`, unit tests.

**Note:** `zlib.h` and `zconf.h` are local copies of zlib headers. The project also has `thirdparty/zlib-1.2.13/` — these local copies exist for historical reasons and point to the same API.

---

## Dead / Unused Components

### UserCallTable.h/cpp
**Status: DEAD CODE — Recommend REMOVAL.**

Defines a `UserCallTable` struct identical in structure to `GLJumpTable` (both `#include "GLJumpTableFields.gen"`). Intended to provide user-defined callback hooks for GL calls. However:
- **Not compiled** into any CMake target
- **Not included** by any other source file
- No evidence of historical usage beyond its own `.cpp` file

**Recommendation:** Remove `UserCallTable.h` and `UserCallTable.cpp`. The functionality (if ever needed) can be trivially recreated from `GLJumpTable`.

### GALTester/
**Status: DEAD CODE — Recommend REMOVAL.**

Contains a single `GALTest.vcxproj` (legacy Visual Studio project) referencing `tests/gal/GALTest.cpp`. However:
- **No CMake integration** — not built by the current build system
- **No references** from any other source or build file
- The `.vcxproj` targets an old Visual Studio toolchain

**Recommendation:** Remove the `GALTester/` directory. If GAL testing is needed, create a proper CMake test target in `tests/` instead.

---

## Dependency Graph

```
                    OGLApiCodeGen ──→ Parsers/
                         │
                    (generates .gen files)
                         │
              ┌──────────┼──────────┐
              ▼          ▼          ▼
         GLResolver  GLJumpTable  (TracePlayer)
              │
              ▼
     ApitraceParser/ ──→ GL/ headers
         │        │
         ▼        ▼
  TraceDriverApitraceOGL  TraceDriverApitraceD3D
         │                    │
         ▼                    ▼
    TraceDriverBase ←── TraceDriverMeta
         │                    │
         ▼                    ▼
      computrender            MetaTraceGenerator/
                              │
                         MetaStreamTrace.h
                              │
                    RegisterWriteBufferMeta

  BufferDescriptor ←→ MemoryRegion ──→ DArray
         │
         ▼
       log/ (LogObject, IncludeLog)

  zfstream ──→ zlib.h/zconf.h ──→ thirdparty/zlib
```

---

## Build Integration

Components are integrated into the main build via two mechanisms:

1. **Direct source compilation** (in `arch/CMakeLists.txt`):
   - `BufferDescriptor.cpp`, `MemoryRegion.cpp`, `DArray.cpp`
   - `GLJumpTable.cpp`, `GLResolver.cpp`
   - `IncludeLog.cpp`, `LogObject.cpp`
   - `TraceDriverApitraceOGL.cpp`, `TraceDriverApitraceD3D.cpp`
   - `ApitraceCallDispatcherD3D.cpp`
   - `zfstream.cpp` (in `arch/perfmodel/CMakeLists.txt`)
   - `TraceDriverMeta.cpp`, `RegisterWriteBufferMeta.cpp` (in `arch/perfmodel/CMakeLists.txt`)

2. **CMake library targets**:
   - `ApitraceParser` — built from `ApitraceParser/CMakeLists.txt`, linked into `computrender`
   - `OGLApiCodeGen` — build tool executable

3. **Standalone projects** (not part of main build):
   - `MetaTraceGenerator/CMakeLists.txt` — independent build for trace extraction tools
