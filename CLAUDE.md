# CLAUDE.md — CG1 GPU Simulator Project Guide

## Project Overview

CG1 (computGeneral 1) is a cycle-accurate GPU simulator capable of replaying OpenGL and D3D9 API traces captured from PC Windows games. It models a complete GPU pipeline — from command processing through rasterization to display output — at multiple abstraction levels. The project is referenced from the ATTILA GPU simulator (https://attila.ac.upc.edu).

**Primary use cases:**
- Simulating GPU graphics workloads from captured API traces
- Architecture exploration with configurable GPU parameters
- Performance analysis via per-cycle, per-frame, per-batch statistics
- Validation between behavior model (fast) and functional model (cycle-accurate)

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Trace Input Layer                        │
│  OpenGL Traces (.txt/.gz)  │  D3D9 PIX Traces (.PIXRun)    │
│  MetaStream Traces (.tracefile.gz)                          │
└─────────────────────┬───────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────┐
│                     Driver Layer                             │
│  TraceDriverOGL / TraceDriverD3D / TraceDriverMeta          │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────────┐     │
│  │  OGL2   │  │  D3D9   │  │   GAL   │  │    HAL    │     │
│  │(OpenGL) │  │(Direct3D)│  │(Abstrac)│  │(HW Driver)│     │
│  └────┬────┘  └────┬────┘  └────┬────┘  └─────┬─────┘     │
│       └─────────────┴───────────┘              │            │
│                                    MetaStream  │            │
└────────────────────────────────────────────────┼────────────┘
                                                 │
┌────────────────────────────────────────────────▼────────────┐
│                    Simulator Core                            │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ bhavmodel (CG1BMDL) — Fast behavior-level emulation  │   │
│  │ funcmodel (CG1CMDL) — Cycle-accurate simulation      │   │
│  │ archmodel           — SystemC-based (experimental)    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  GPU Pipeline Modules (funcmodel MDUs):                      │
│  CommandProcessor → Streamer → PrimitiveAssembler →          │
│  Clipper → Rasterizer → UnifiedShader ↔ TextureProcessor →  │
│  ZStencilTest → ColorWrite → DisplayController               │
│              e                                                │
│  MemoryController (V1 simple / V2 GDDR-accurate)            │
│  CacheSystem (Texture, Z, Color, Fetch, Input, ROP caches)  │
└─────────────────────────────────────────────────────────────┘
```

### Modeling Layers

| Layer | Class | Purpose | Speed |
|-------|-------|---------|-------|
| **bhavmodel** | `CG1BMDL` / `bmoGpuTop` | Behavior-level functional emulation, no cycle timing | Fast |
| **funcmodel** | `CG1CMDL` / `cmoGpuTop` | Cycle-accurate simulation with Signal-based inter-module communication | Slow (accurate) |
| **archmodel** | (experimental) | SystemC-based cycle-accurate model | N/A |

## Directory Structure

```
computrendering/
├── CLAUDE.md                    # This file
├── CMakeLists.txt               # Top-level build configuration
├── README.md                    # Project introduction and usage
├── OPENISSUES.md                # Known issues
│
├── arch/                        # Core simulator architecture
│   ├── CG1SIM.cpp/h            # ★ MAIN ENTRY POINT (main())
│   ├── CMakeLists.txt           # Arch build config
│   ├── common/                  # Shared utilities
│   │   ├── GPUReg.h             # GPU register definitions & commands
│   │   ├── GPUType.h            # GPU data types
│   │   ├── Vec4FP32.h/cpp       # 4D float vector
│   │   ├── Vec4Int.h/cpp        # 4D integer vector
│   │   ├── BitStream*.h/cpp     # Bit-level stream I/O
│   │   ├── Parser.h/cpp         # Config file parser
│   │   ├── Profiler.h/cpp       # Performance profiling
│   │   ├── ImageSaver.h/cpp     # PPM image output
│   │   ├── DynamicObject.h/cpp  # Fast dynamic allocation
│   │   ├── DynamicMemoryOpt.h   # Memory optimization
│   │   ├── support.h/cpp        # General utility functions
│   │   ├── params/              # Configuration files
│   │   │   ├── CG1GPU.ini       # ★ Primary simulator config
│   │   │   ├── CG1.0.json       # Architecture v1.0 params
│   │   │   ├── CG1.1.json       # Architecture v1.1 params
│   │   │   └── *.cfg, *.ini     # Other config variants
│   │   └── metrics/             # Performance metric definitions
│   │
│   ├── bhavmodel/               # Behavior model (fast emulation)
│   │   ├── CG1BMDL.h/cpp       # Top-level behavior model
│   │   ├── bmGpuTop.h/cpp       # GPU top-level behavior model
│   │   ├── common/base/         # Base classes (bmMduBase, ValidationInfo)
│   │   ├── common/math/         # Fixed-point math, GPU math
│   │   ├── UnifiedShader/       # Shader ISA execution (ShaderInstr, ShaderOptimization)
│   │   ├── Rasterizer/          # Triangle setup, fragment generation, tiles
│   │   ├── Clipper/             # Primitive clipping
│   │   ├── TextureProcessor/    # Texture sampling (PixelMapper)
│   │   └── FragmentOperator/    # Fragment operations (Z, blend, write)
│   │
│   ├── funcmodel/               # Functional model (cycle-accurate)
│   │   ├── CG1CMDL.h/cpp       # Top-level functional model
│   │   ├── common/base/         # ★ Base simulation classes
│   │   │   ├── cmMduBase.h      # Module base class (all MDUs inherit this)
│   │   │   └── cmGPUSignal.h    # Signal class for inter-module comm
│   │   ├── CommandProcessor/    # Command processor & MetaStream handling
│   │   │   └── MetaStream.h     # ★ MetaStream transaction format
│   │   ├── StreamController/    # Vertex fetch & post-shading cache
│   │   ├── PrimitiveAssembler/  # Primitive assembly
│   │   ├── Clipper/             # Clipper stage
│   │   ├── Rasterizer/          # Triangle setup, fragment gen, HiZ, fragment FIFO
│   │   ├── UnifiedShader/       # Unified vertex/fragment shader
│   │   ├── TextureProcessor/    # Texture unit
│   │   ├── FragmentOperator/    # Z/stencil test, color write, blend, ROP
│   │   ├── DisplayController/   # Display output & blitter
│   │   ├── MemoryController/    # Simple memory model
│   │   ├── MemoryControllerV2/  # ★ Accurate GDDR memory model
│   │   └── CacheSystem/         # All cache implementations
│   │       ├── cmTextureCache*  # Texture cache (L1, L2, Gen)
│   │       ├── cmZCache*        # Z-buffer cache
│   │       ├── cmColorCache*    # Color buffer cache
│   │       ├── cmFetchCache*    # Vertex fetch cache
│   │       ├── cmInputCache*    # Input cache
│   │       └── cmROPCache*      # ROP cache
│   │
│   └── utils/                   # Simulator utilities
│       ├── ConfigLoader.h/cpp   # INI config parser
│       ├── CommandLineReader.h  # CLI argument parser
│       ├── CG1ASM/              # Shader ISA assembler tools
│       ├── TestCreator/         # Non-API test framework
│       └── CGAPI/               # Low-level API (experimental)
│
├── driver/                      # Graphics API implementations
│   ├── hal/                     # Hardware Abstraction Layer
│   │   └── HAL.h/cpp            # ★ GPU driver (MetaStream gen, memory mgmt)
│   ├── gal/                     # Graphics Abstraction Layer
│   │   ├── GAL/                 # Core interfaces & implementations
│   │   │   ├── Interface/       # Abstract interfaces (GALDevice, GALTexture, etc.)
│   │   │   └── Implementation/  # Concrete implementations
│   │   ├── GALx/                # Extensions (fixed pipeline emu, ARB shaders)
│   │   └── test/                # GAL tests
│   ├── ogl/                     # OpenGL
│   │   ├── OGL2/                # OpenGL 2.0 implementation (GAL-based)
│   │   ├── OGL14/               # Legacy OpenGL 1.4 (deprecated)
│   │   ├── trace/
│   │   │   ├── GLInterceptor/   # OpenGL call interceptor (creates DLL)
│   │   │   ├── GLTracePlayer/   # OpenGL trace replayer
│   │   │   └── GLInstrument*/   # Trace instrumentation
│   │   └── inc/                 # OpenGL headers
│   ├── d3d/                     # Direct3D 9 (Windows only)
│   │   ├── D3D9/                # D3D9 implementation (GAL-based)
│   │   │   ├── D3DInterface/    # Interface wrappers
│   │   │   ├── D3DImplement/    # Concrete implementation
│   │   │   ├── D3DState/        # State management
│   │   │   └── ShaderTranslator/# D3D→CG1 shader translation
│   │   ├── trace/
│   │   │   ├── D3DTraceCore/    # Core PIX trace player library
│   │   │   ├── D3DTracePlayer/  # D3D9 PIX trace player app
│   │   │   └── D3DTraceStat/    # Trace statistics tool
│   │   └── inc/                 # D3D9 headers
│   └── utils/                   # Driver utilities
│       ├── TraceReader/         # Trace file reader
│       ├── OGLApiCodeGen/       # ★ OpenGL API code generator (flex/bison)
│       └── D3DApiCodeGen/       # ★ D3D API code generator
│
├── tests/                       # Test suites
│   ├── arch/                    # Architecture unit tests
│   ├── ogl/trace/               # OpenGL test traces with reference outputs
│   ├── d3d/                     # D3D9 PIX test traces
│   ├── ocl/                     # OpenCL compute shader tests
│   └── regression/              # Regression scripts (Perl-based)
│
├── tools/                       # Standalone tools
│   ├── PIXparser/               # PIXRun file parser
│   ├── PPMConvertor/            # PPM image color mapping
│   ├── SnapshotExplainer/       # GPU state snapshot dumper
│   ├── TraceViewer/             # Signal trace visualizer (Qt GUI)
│   ├── DXInterceptor/           # D3D9 trace capturer (Detours-based)
│   └── script/                  # Build scripts (CompilerGcc.cmake, etc.)
│
└── common/thirdparty/           # External dependencies
    ├── zlib-1.2.13/             # Compression
    ├── systemc-2.3.3/           # System-level modeling (archmodel)
    ├── SDL2/                    # Simple DirectMedia Layer
    ├── rapidjson/               # JSON parsing
    └── tinyxml2/                # XML parsing
```

## Key Concepts

### Signal-Based Communication (funcmodel)
Modules in the functional model communicate through **Signal** objects (`cmGPUSignal`). Each Signal has:
- **Bandwidth**: max data items per cycle
- **Latency**: propagation delay in cycles
Signals provide a cycle-accurate model of inter-module data transfer.

### MDU (Module Design Unit)
All simulator modules inherit from `cmoMduBase` (funcmodel) or `bmoMduBase` (bhavmodel). MDUs provide:
- Signal registration and management
- Statistics collection
- Debug/stall detection
- Clock-domain awareness (GPU clock, shader clock, memory clock)

### MetaStream
The communication protocol between the driver (HAL) and the simulator. MetaStream transactions include:
- `WRITE` / `READ`: Memory transfers between system and GPU memory
- `REG_WRITE` / `REG_READ`: GPU register access
- `COMMAND`: GPU control commands (draw, clear, swap, etc.)
- `EVENT`: Synchronization events
- Packet size: 32 bytes, stored in gzip-compressed trace files

### Trace-Driven Simulation
The simulator is driven by API traces:
1. **OpenGL traces** (`.txt` / `.txt.gz`): Captured by `GLInterceptor`
2. **D3D9 PIX traces** (`.PIXRun` / `.PIXRunz`): Captured by PIX or DXInterceptor
3. **MetaStream traces** (`.tracefile.gz`): Pre-translated binary GPU command stream

The trace flow: `API Trace → TraceDriver → HAL → MetaStream → Simulator`

### DynamicObject
Fast dynamic allocation system for simulation objects (MetaStreams, fragments, etc.). Uses bucket-based memory pools for performance.

### GPU Register System
Defined in `arch/common/GPUReg.h`. Register writes from the driver configure the simulated GPU's state (shader programs, texture state, render state, etc.).

## Build Instructions

### Prerequisites
- CMake 3.13.1+
- GCC / Clang / MSVC (Visual Studio 2022+)
- flex & bison (for API code generation)
- mingw (Windows only, for POSIX headers)

### Linux Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Build targets:
- `CG1SIM` — Main simulator executable
- `CG1Common` — Common utilities library
- `CG1BMDL` — Behavior model library
- `CG1CMDL` — Functional model library
- `HAL`, `GAL`, `GALx`, `OGL2` — Driver libraries

### Windows Build
Open `CG1.sln` in Visual Studio 2022+. Select configuration (Debug/Optimized/Profile) and architecture (Win32/x64). Build the desired project.

### Key CMake Options
| Option | Default | Description |
|--------|---------|-------------|
| `WARNINGS_AS_ERRORS` | Yes | Treat warnings as errors |
| `USE_PTHREADS` | No | Use pthreads library |
| `PERFORMANCE_COUNTERS` | No | Enable performance counters |
| `POWER_COUNTERS` | No | Enable power/energy counters |
| `SYSTEMC_VERSION` | 2.3.3 | SystemC version for archmodel |

### Key Compiler Definitions
- `UNIFIEDSHADER` — Enable unified shader architecture
- `CG_STREAM_PROCESSOR` — Enable stream processor model
- `LOAD_JUMPTABLE_STATICALLY` — Static jump table loading for OGL

## Configuration

The primary configuration file is `arch/common/params/CG1GPU.ini`. Key sections:

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

JSON configs (`CG1.0.json`, `CG1.1.json`) define architecture parameter schemas.

## Running the Simulator

```bash
# Basic usage (reads CG1GPU.ini from current directory)
./CG1SIM

# With trace file
./CG1SIM tracefile.txt

# With frame count (n < 10000) or cycle count (n >= 10000)
./CG1SIM tracefile.txt 5

# With start frame offset
./CG1SIM tracefile.txt 5 2    # Simulate 5 frames, starting from frame 2
```

### Simulation Output
1. **Standard output**: Progress dots (10K cycles each), 'B' per draw call, frame timing
2. **PPM files**: Rendered frames as images
3. **Statistics CSV**: `stats.csv`, `stats.frame.csv`, `stats.batch.csv`
4. **Signal trace**: `signaltrace.txt` (for debugging)
5. **Latency maps**: Per-pixel quad property maps

## Testing

### Regression Tests
```bash
cd tests/regression
perl regression.pl
```

### Test Traces
- `tests/ogl/trace/` — OpenGL traces with reference PPMs and statistics
- `tests/d3d/` — D3D9 PIX traces
- `tests/ocl/` — OpenCL compute shader tests

Each test trace directory typically contains:
- `tracefile.txt.gz` — The trace input
- `BufferDescriptors.dat`, `MemoryRegions.dat` — Memory layout
- `reference/` — Expected output (PPM images, statistics)

## Coding Conventions

- **C++ standard**: C++11/14 style
- **Naming**: `cm` prefix for funcmodel classes, `bm` prefix for bhavmodel classes, `cgo` for common objects
- **Class naming**: PascalCase (e.g., `cmoGpuTop`, `bmoUnifiedShader`)
- **File naming**: Matches class name (e.g., `cmClipper.h` → `cmoClipper` class)
- **Module pattern**: Each MDU has a `.h/.cpp` pair, optional `Command`, `StateInfo`, `StatusInfo` companion classes
- **Build**: Each subdirectory has its own `CMakeLists.txt`
- **Headers**: Use `#ifndef` / `#define` include guards (not `#pragma once`)

## Common Development Tasks

### Adding a New Funcmodel Module
1. Create `arch/funcmodel/YourModule/cmYourModule.h/cpp`
2. Inherit from `cmoMduBase`
3. Implement `clock()` method for per-cycle logic
4. Register Signals in constructor
5. Add to `cmoGpuTop` instantiation and wiring
6. Update `arch/funcmodel/CMakeLists.txt`

### Adding a New Bhavmodel Module
1. Create `arch/bhavmodel/YourModule/bmYourModule.h/cpp`
2. Inherit from `bmoMduBase`
3. Implement functional (non-timed) simulation logic
4. Add to `bmoGpuTop`
5. Update `arch/bhavmodel/CMakeLists.txt`

### Modifying GPU Configuration
1. Edit `arch/common/params/CG1GPU.ini` or create a new variant
2. Ensure `ConfigLoader` (`arch/utils/ConfigLoader.h`) supports new parameters
3. Wire new params through to the appropriate MDU constructors

### Debugging a Simulation
- Use the built-in debug mode (interactive shell with `run`, `state`, `snapshot` commands)
- Check `signaltrace.txt` for cycle-by-cycle signal activity
- Use `tools/TraceViewer` for visual signal trace inspection
- Use `tools/SnapshotExplainer` to dump GPU register state
- Check statistics CSV files for performance metrics

## Third-Party Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| zlib | 1.2.13 | Trace file compression/decompression |
| SystemC | 2.3.3 | System-level modeling (archmodel, experimental) |
| SDL2 | 2.0.8 | Windowing and display |
| RapidJSON | 1.1.0 | JSON config parsing |
| TinyXML2 | 8.0.0 | XML parsing |

All third-party libraries are auto-downloaded and built via CMake (`common/thirdparty/`).

## Important Files Quick Reference

| File | Purpose |
|------|---------|
| `arch/CG1SIM.cpp` | Main entry point (`main()`) |
| `arch/common/GPUReg.h` | GPU register & command definitions |
| `arch/funcmodel/common/base/cmMduBase.h` | Funcmodel module base class |
| `arch/funcmodel/common/base/cmGPUSignal.h` | Signal class for inter-module comm |
| `arch/funcmodel/CommandProcessor/MetaStream.h` | MetaStream protocol definition |
| `arch/bhavmodel/CG1BMDL.h` | Behavior model top-level |
| `arch/funcmodel/CG1CMDL.h` | Functional model top-level |
| `driver/hal/HAL.h` | Hardware abstraction layer |
| `driver/gal/GAL/Interface/` | GAL abstract interfaces |
| `arch/common/params/CG1GPU.ini` | Primary configuration file |
| `arch/utils/ConfigLoader.h` | Configuration file parser |
