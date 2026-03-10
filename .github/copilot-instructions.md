# Project Guidelines — computrenderender (AI agent brief)

This file gives concise, actionable guidance for AI coding agents working in this repository.

## Code Style
- Language: C++ (C++11 / C++14). Follow existing code in `arch/`.
- Indentation: 4 spaces (no tabs). Line endings: LF. Column ~80-100.
- Headers: use include guards (`#ifndef __HEADER__` / `#define`) — do not use `#pragma once`.
- Naming: Classes PascalCase, methods camelCase, variables camelCase, macros UPPER_CASE.
- Types: prefer project types in `arch/common/GPUType.h` (e.g. `U32`, `F32`).

## Architecture (quick map)
- Drivers: `driver/` contains `ogl`, `d3d`, `hal`, `gal` trace drivers.
- Simulator cores: `arch/bhavmodel` (fast functional), `arch/perfmodel` (cycle-accurate).
- Key modules live under `arch/perfmodel` (MDUs) and are wired in `cmoGpuTop`.

## Build & Test
- Preferred helper scripts: `tools/script/build.bat` (Windows) or `tools/script/build.sh` (POSIX).
- Typical build: configure with CMake and then `cmake --build _BUILD_ --config Debug --target computrender`.
- Regression: `tools/script/regression/regression.sh` or `regression.ps1` (Windows).
- When modifying modules, update corresponding `CMakeLists.txt` under `arch/`.

## Project Conventions (practical rules)
- New perfmodel module: add `arch/perfmodel/YourModule/cmYourModule.{h,cpp}`, inherit `cmoMduBase`, implement `clock()`.
- New bhavmodel module: same pattern under `arch/bhavmodel` using `bmoMduBase`.
- Follow existing include order: local header, project headers, system headers.
- Use Doxygen-style comments for public APIs (`//!` / `/*! ... */`).

## Integration Points
- Trace inputs: `apitrace` traces (OpenGL/D3D) and MetaStream traces; see `tools/apitrace` and README.
- MetaStream is the driver–simulator protocol (REG_READ/WRITE, READ/WRITE, COMMAND, EVENT).
- Third-party libs are in `thirdparty/` (zlib, snappy, rapidjson). Prefer existing libs where possible.

## Sensitive Areas & Security
- Sensitive logic: GPU register handling (`arch/common/GPUReg.h`) and trace processing — avoid leaking trace data.
- Do not add credentials or secrets to the repo. Keep any API keys or tokens out of source.

## When in Doubt
- Inspect nearby code examples before changing style (e.g. `arch/perfmodel/*`, `arch/bhavmodel/*`).
- Run `tools/script/build.*` and `tools/script/regression/*` after changes.

---
If anything here is unclear or you'd like me to expand a section (examples, tests, or commit checklist), tell me which part to extend.
