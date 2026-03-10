# Project Guidelines — computrender (AI agent brief)

This file gives concise, actionable guidance for AI coding agents working in this repository.

## Code Style
- **Language:** C++ (C++11 / C++14). Follow existing code in `arch/`.
- **Indentation:** 4 spaces (no tabs). Line endings: LF. Column ~80-100.
- **Headers:** use include guards (`#ifndef __HEADER__` / `#define`) — do not use `#pragma once`.
- **Naming:** Classes PascalCase (`Vec4FP32`), methods camelCase (`setComponents`), variables camelCase, macros UPPER_CASE (`GPU_ERROR_CHECK`).
- **Types:** prefer project types in `arch/common/GPUType.h` (`U32`, `F32`, `S32`, `BIT` instead of standard C++ types).
- **Prefixes:** `cm`/`cmo` = perfmodel (`arch/perfmodel`), `bm`/`bmo` = bhavmodel (`arch/bhavmodel`), `cgo` = common objects.
- **Braces:** Open brace on same line. Spaces inside parens common: `if ( value > 0 ) {`.

## Architecture (quick map)
- **Entry point:** `arch/computrender.cpp` (main function).
- **Drivers:** `driver/` contains `ogl` (OpenGL 2.0), `d3d` (D3D9), `hal` (HW abstraction), `gal` (graphics abstraction).
- **Simulator cores:** `arch/bhavmodel` (fast functional), `arch/perfmodel` (cycle-accurate).
- **Key modules (MDUs):** live under `arch/perfmodel/` — wired in `cmoGpuTop.cpp`.
- **Pipeline flow:** CommandProcessor → Streamer → PrimitiveAssembler → Clipper → Rasterizer → UnifiedShader ↔ TextureProcessor → FragmentOperator → DisplayController.
- **Communication:** perfmodel uses Signal-based communication (`cmGPUSignal`) with bandwidth and latency modeling.

## Build & Test
- **Build (preferred):** `tools/script/build.bat` (Windows) or `tools/script/build.sh` (Linux/POSIX).
- **Build (manual):** `cmake --build _BUILD_ --config Debug --target computrender`
- **Regression tests:** `tools/script/regression/regression.ps1` (Windows) or `regression.sh` (Linux).
- **Quick test:** `./computrender --trace tests/ogl/trace/glxgears/glxgears.trace --frames 1`
- **When modifying modules:** update corresponding `CMakeLists.txt` under `arch/`.

## Project Conventions (practical rules)
- **New perfmodel module:** add `arch/perfmodel/YourModule/cmYourModule.{h,cpp}`, inherit `cmoMduBase`, implement `clock()` method. Example: `arch/perfmodel/Rasterizer/cmRasterizer.{h,cpp}`.
- **New bhavmodel module:** same pattern under `arch/bhavmodel` using `bmoMduBase`. Example: `arch/bhavmodel/Rasterizer/bmRasterizer.{h,cpp}`.
- **Include order:** 1) corresponding header, 2) project headers, 3) system headers.
- **Error handling:** use `GPU_ERROR_CHECK` macro for runtime checks, `GPU_ASSERT` for invariants.
- **Documentation:** use Doxygen-style comments (`//!` single line, `/*! ... */` blocks) for public APIs.
- **Config params:** edit `arch/common/params/archParams.csv` or JSON variants in `arch/common/params/`.

## Integration Points
- **Trace inputs:** `apitrace` traces (`.trace` files, OpenGL/D3D9) and MetaStream traces (`.tracefile.gz`). See `tools/apitrace` and README.
- **MetaStream protocol:** driver–simulator communication (REG_READ/WRITE, READ/WRITE, COMMAND, EVENT). 32-byte packets, gzip-compressed.
- **API auto-detection:** simulator detects OGL vs D3D9 from `.trace` files automatically.
- **Third-party libs:** in `thirdparty/` (zlib, snappy, rapidjson, SDL2, tinyxml2). Prefer existing libs where possible.

## Debugging & Analysis
- **Interactive debug mode:** built-in shell with `run`, `state`, `snapshot` commands.
- **Signal traces:** check `signaltrace.txt` for cycle-by-cycle signal activity.
- **Tools:** `tools/TraceViewer` (visual signal trace), `tools/SnapshotExplainer` (GPU register state dump).
- **Statistics:** per-cycle, per-frame, per-batch CSV files generated in output directory.

## Sensitive Areas & Security
- **Sensitive logic:** GPU register handling (`arch/common/GPUReg.h`) and trace processing — avoid leaking trace data.
- **No credentials:** do not add secrets, API keys, or tokens to the repo.

## When in Doubt
- Inspect nearby code examples before changing style (e.g. `arch/perfmodel/*`, `arch/bhavmodel/*`).
- Run `tools/script/build.*` and `tools/script/regression/*` after changes.
- See `.claude/CLAUDE.md` for detailed architecture deep-dive and development workflows.

---
If anything here is unclear or you'd like me to expand a section (examples, tests, or commit checklist), tell me which part to extend.
