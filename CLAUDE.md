# CLAUDE.md — CG1 GPU Simulator Developer Guide

> This file supplements `README.md` with architecture internals, coding conventions,
> and development how-tos for AI coding agents and developers working on CG1.
> For build instructions, running, testing, and trace driver details see `README.md`.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Trace Input Layer                        │
│  Apitrace (.trace) — OpenGL & D3D9 traces                   │
│  MetaStream Traces (.tracefile.gz)                          │
└─────────────────────┬───────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────┐
│                     Driver Layer                             │
│  TraceDriverApitraceOGL / TraceDriverApitraceD3D / TraceDriverMeta │
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
│  │ perfmodel (CG1CMDL) — Cycle-accurate simulation      │   │
│  │ archmodel           — SystemC-based (experimental)    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  GPU Pipeline Modules (perfmodel MDUs):                      │
│  CommandProcessor → Streamer → PrimitiveAssembler →          │
│  Clipper → Rasterizer → UnifiedShader ↔ TextureProcessor →  │
│  ZStencilTest → ColorWrite → DisplayController               │
│                                                               │
│  MemoryController (V1 simple / V2 GDDR-accurate)            │
│  CacheSystem (Texture, Z, Color, Fetch, Input, ROP caches)  │
└─────────────────────────────────────────────────────────────┘
```

### Modeling Layers

| Layer | Class | Purpose | Speed |
|-------|-------|---------|-------|
| **bhavmodel** | `CG1BMDL` / `bmoGpuTop` | Behavior-level functional emulation, no cycle timing | Fast |
| **perfmodel** | `CG1CMDL` / `cmoGpuTop` | Cycle-accurate simulation with Signal-based inter-module communication | Slow (accurate) |
| **archmodel** | (experimental) | SystemC-based cycle-accurate model | N/A |

---

## Key Concepts

### Signal-Based Communication (perfmodel)

Modules in the functional model communicate through **Signal** objects (`cmGPUSignal`). Each Signal has:
- **Bandwidth**: max data items per cycle
- **Latency**: propagation delay in cycles

Signals provide a cycle-accurate model of inter-module data transfer.

### MDU (Module Design Unit)

All simulator modules inherit from `cmoMduBase` (perfmodel) or `bmoMduBase` (bhavmodel). MDUs provide:
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

### DynamicObject

Fast dynamic allocation system for simulation objects (MetaStreams, fragments, etc.). Uses bucket-based memory pools for performance.

### GPU Register System

Defined in `arch/common/GPUReg.h`. Register writes from the driver configure the simulated GPU's state (shader programs, texture state, render state, etc.).

### Key Compiler Definitions

- `UNIFIEDSHADER` — Enable unified shader architecture
- `CG_STREAM_PROCESSOR` — Enable stream processor model
- `LOAD_JUMPTABLE_STATICALLY` — Static jump table loading for OGL

---

## Code Style & Conventions

Adhere strictly to the existing style to maintain consistency.

### General

- **Language Standard:** C++11 / C++14
- **Indentation:** 4 spaces (no tabs)
- **Line Endings:** Unix (LF)
- **Column Limit:** 80-100 characters generally

### Naming Conventions

| Element | Convention | Examples |
|---------|-----------|----------|
| Classes | PascalCase | `Vec4FP32`, `CommandProcessor` |
| Methods | camelCase | `setComponents`, `getVector` |
| Variables | camelCase | `component`, `index`, `vertexCount` |
| Member Variables | camelCase (no prefix) | `parser_`, `driver_` |
| Constants/Macros | UPPER_CASE | `GPU_ERROR_CHECK`, `CG1_ISA_OPCODE_DP4` |
| Namespaces | lowercase | `arch`, `apitrace` |
| Files | PascalCase, matching class | `Vec4FP32.cpp`, `Vec4FP32.h` |

### Project Structure Prefixes

| Prefix | Layer | Location |
|--------|-------|----------|
| `cm` / `cmo` | Functional model | `arch/perfmodel` |
| `bm` / `bmo` | Behavioral model | `arch/bhavmodel` |
| `cgo` | Common objects | `arch/common` |

### Types

Use the project-specific types defined in `arch/common/GPUType.h`:

| Type | Standard Equivalent |
|------|-------------------|
| `F32` | `float` |
| `F64` | `double` |
| `U32` | `unsigned int` |
| `S32` | `int` |
| `U8`, `S8` | `unsigned char`, `char` |
| `U16`, `S16` | `unsigned short`, `short` |
| `U64`, `S64` | `unsigned long long`, `long long` |
| `BIT` | boolean/bit |

### Formatting

**Braces:** Open brace on the same line for functions and control statements.
```cpp
void MyClass::myMethod(S32 value) {
    if (value > 0) {
        // code
    }
}
```

**Spacing:**
- Space after keywords (`if`, `for`, `while`)
- Spaces inside parentheses are common: `if ( index < 0 )`
- Space before and after binary operators (`=`, `+`, `==`)

### Headers & Imports

- **Include Guards:** Use `#ifndef __HEADER_NAME__` / `#define __HEADER_NAME__`. Do *not* use `#pragma once`.
- **Include Order:**
    1. Corresponding header file
    2. Project headers (`#include "GPUType.h"`)
    3. System headers (`#include <iostream>`)
- **Paths:** Use relative paths or paths configured in CMake include directories.

### Error Handling

- Use `GPU_ERROR_CHECK` macro for runtime checks
- Use `GPU_ASSERT` or standard `assert` for invariants
- Print errors to `std::cerr` with "Error" prefix, or use the project's logging facilities (`IncludeLog.h` / `LogObject.h`)

### Documentation

- Use Doxygen-style comments (`//!` for single line, `/*! ... */` for blocks)
- Document classes and public methods

---

## Common Development Tasks

### Adding a New Perfmodel Module

1. Create `arch/perfmodel/YourModule/cmYourModule.h/cpp`
2. Inherit from `cmoMduBase`
3. Implement `clock()` method for per-cycle logic
4. Register Signals in constructor
5. Add to `cmoGpuTop` instantiation and wiring
6. Update `arch/perfmodel/CMakeLists.txt`

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

---

## Development Workflow

1. **Understand:** Read this file for architecture and conventions; read `README.md` for build/run/test
2. **Locate:** Find the relevant MDU in `arch/perfmodel` or `arch/bhavmodel`
3. **Modify:** Apply changes following the coding style above
4. **Verify:**
    - Build: `cmake --build _BUILD_ --config Debug --target CG1SIM`
    - Run the standard trace: `./CG1SIM --trace tests/ogl/trace/glxgears/glxgears.trace --frames 1`
    - Run regression: `bash tools/script/regression/regression.sh` (or `.ps1` on Windows)
    - If modifying a specific unit (e.g., Math), run the relevant unit test in `tests/arch/`
