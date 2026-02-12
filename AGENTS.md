# GPU Simulator (CG1) Developer Guide

This repository contains the source code for the CG1 GPU Simulator. This guide provides instructions for building, testing, and maintaining the codebase.

## 1. Build Instructions

The project uses **CMake** as the primary build system.

### Prerequisites
- CMake 3.13.1+
- GCC / Clang / MSVC (Visual Studio 2022+)
- `flex` & `bison` (required for API code generation)

### Standard Build
To build the main simulator executable `CG1SIM`:

```bash
# Create build directory
mkdir -p build
cd build

# Configure (Release mode recommended for performance)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build using all available cores
make -j$(nproc)
```

This will generate the `CG1SIM` executable in the `build/arch` directory (or just `build` depending on configuration).

### Clean Build
To clean the build artifacts:
```bash
rm -rf build
```

## 2. Testing Instructions

### Trace-Driven Simulation (Primary Method)
The primary way to verify the simulator is by running trace files (captured from OpenGL/D3D9 applications).

**Command:**
```bash
# From the build directory
./arch/CG1SIM <path_to_trace_file> [num_frames] [start_frame]
```

**Example:**
```bash
# Run the 'triangle' trace
./arch/CG1SIM ../tests/ogl/trace/triangle/triangle.txt
```

**Trace Locations:**
- `tests/ogl/trace/` - OpenGL traces
- `tests/d3d/` - Direct3D traces

### Unit Tests
Unit tests are located in `tests/arch/`.
*Note: These tests are currently standalone and not integrated into the main CMake build.*

To run a specific unit test (e.g., `testGPUMath.cpp`), you may need to compile it manually or add a target to `tests/arch/CMakeLists.txt` (if you create one) or the root `CMakeLists.txt`.

**Manual Compilation Example:**
```bash
cd tests/arch
g++ -o testGPUMath testGPUMath.cpp -I../../arch/common -I../../arch/funcmodel -std=c++14
./testGPUMath
```
*(You may need to link against `CG1Common` or other libraries built in the main build)*

## 3. Code Style & Conventions

Adhere strictly to the existing style to maintain consistency.

### General
- **Language Standard:** C++11 / C++14.
- **Indentation:** 4 spaces (no tabs).
- **Line Endings:** Unix (LF).
- **Column Limit:** 80-100 characters generally, but be reasonable.

### Naming Conventions
- **Classes:** PascalCase (e.g., `Vec4FP32`, `CommandProcessor`).
- **Methods:** camelCase (e.g., `setComponents`, `getVector`).
- **Variables:** camelCase (e.g., `component`, `index`, `vertexCount`).
- **Member Variables:** camelCase (no specific prefix like `m_` observed in `Vec4FP32`).
- **Constants/Macros:** UPPER_CASE (e.g., `GPU_ERROR_CHECK`, `CG1_ISA_OPCODE_DP4`).
- **Namespaces:** lowercase (e.g., `arch`).
- **Files:** PascalCase, matching the class name (e.g., `Vec4FP32.cpp`, `Vec4FP32.h`).

### Project Structure & Prefixes
- **Functional Model:** Classes often prefixed with `cm` or `cmo` (e.g., `cmoGpuTop`). located in `arch/funcmodel`.
- **Behavioral Model:** Classes often prefixed with `bm` or `bmo` (e.g., `bmoGpuTop`). located in `arch/bhavmodel`.
- **Common Objects:** `cgo` prefix.

### Types
Use the project-specific types defined in `arch/common/GPUType.h`:
- `F32` instead of `float`
- `F64` instead of `double`
- `U32` instead of `unsigned int`
- `S32` instead of `int`
- `U8`, `S8`, `U16`, `S16`, `U64`, `S64`
- `BIT` for boolean/bit types

### Formatting
- **Braces:**
    - Function definitions: Open brace on the same line.
    - Control statements (`if`, `for`): Open brace on the same line.
    - Example:
      ```cpp
      void MyClass::myMethod(S32 value) {
          if (value > 0) {
              // code
          }
      }
      ```
- **Spacing:**
    - Space after keywords (`if`, `for`, `while`).
    - Spaces inside parentheses are common in this codebase: `if ( index < 0 )`.
    - Space before and after binary operators (`=`, `+`, `==`).

### Headers & Imports
- **Include Guards:** Use `#ifndef __HEADER_NAME__` / `#define __HEADER_NAME__`. Do *not* use `#pragma once`.
- **Order:**
    1. Corresponding header file.
    2. Project headers (e.g., `#include "GPUType.h"`).
    3. System headers (e.g., `#include <iostream>`).
- **Paths:** Use relative paths or paths configured in CMake include directories.

### Error Handling
- Use the `GPU_ERROR_CHECK` macro for runtime checks.
- Use `GPU_ASSERT` or standard `assert` for invariants.
- Print errors to `std::cerr` or `cout` with "Error" prefix if appropriate, or use the project's logging facilities if available (`IncludeLog.h` / `LogObject.h`).

### Documentation
- Use Doxygen-style comments (`//!` for single line, `/*! ... */` for blocks).
- Document classes and public methods.

## 4. Development Workflow

1.  **Understand:** Read `CLAUDE.md` for high-level architecture.
2.  **Locate:** Find the relevant MDU (Module Design Unit) in `arch/funcmodel` or `arch/bhavmodel`.
3.  **Modify:** Apply changes following the coding style.
4.  **Verify:**
    -   Run the standard trace (`tests/ogl/trace/triangle/triangle.txt`) to ensure no regression.
    -   If modifying a specific unit (e.g., Math), run the relevant unit test in `tests/arch/`.

## 5. Directory Structure Key

- `arch/`: Core simulator code.
    - `funcmodel/`: Cycle-accurate model.
    - `bhavmodel/`: Behavior model.
    - `common/`: Shared utilities and types.
- `driver/`: OpenGL/D3D driver implementations.
- `tests/`: Trace files and unit tests.
- `tools/`: Ancillary tools.

---
*Generated for coding agents operating in the CG1 Simulator repository.*
