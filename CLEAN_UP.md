# Project Cleanup & Migration Status

This document tracks the removal of legacy modules as the project migrated to **apitrace** for trace capture and playback.

## 1. Completed Removals

The following legacy modules have been successfully removed from the codebase:

*   **`driver/ogl/trace/GLInterceptor`**
    *   **Description:** Source code for the custom OpenGL interception DLL (Windows-only).
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by cross-platform `apitrace` CLI. Essential headers migrated to `driver/utils/OGLApiCodeGen`.

*   **`driver/ogl/trace/GLTracePlayer`**
    *   **Description:** Standalone Windows GUI tool for replaying GLInterceptor traces.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by `apitrace replay` and the internal simulator player.

*   **`driver/ogl/trace/GLInstrument` & `driver/ogl/trace/GLInstrumentTool`**
    *   **Description:** Legacy Windows-specific OpenGL instrumentation framework and tools.
    *   **Status:** **Removed**.
    *   **Resolution:** Obsolete in the Apitrace workflow.

*   **`driver/utils/TraceReader` & `driver/utils/TraceDriver/TraceDriverOGL`**
    *   **Description:** Reader and driver for `.ogl.txt.gz` GLInterceptor traces.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by `ApitraceParser` and `TraceDriverApitrace`.

*   **`driver/d3d/trace/` (D3DTraceCore, D3DTracePlayer, D3DTraceStat)**
    *   **Description:** Legacy Microsoft PIX trace reader library, player, and statistics tool.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by `ApitraceParser` and `TraceDriverApitraceD3D` for D3D9 trace playback.

*   **`driver/utils/TraceDriver/TraceDriverD3D`**
    *   **Description:** Legacy trace driver for `.PIXRun` / `.PIXRunz` D3D9 PIX traces.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by `TraceDriverApitraceD3D` using apitrace `.trace` format.

*   **PIX test traces (`.PIXRun` files)**
    *   **Description:** Legacy D3D9 PIX trace test files under `tests/d3d/`.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by apitrace `.trace` files under `tests/d3d/trace/`.

*   **`driver/utils/MetaTraceGenerator/traceTranslator`**
    *   **Description:** Tool to convert API-level traces to MetaStream binary format (`Api2MetaTranslator`).
    *   **Status:** **Removed**.
    *   **Resolution:** Depended on removed `TraceReader`/`TraceDriverOGL`. Was already disabled in CMake. The parent `MetaTraceGenerator/` directory is retained for `MetaStreamTrace.h` and `traceExtractor/` which are still actively used.

*   **`driver/utils/D3DApiCodeGen/`**
    *   **Description:** Code generator for D3D9 PIX trace dispatch code (`.gen` files consumed by removed `D3DTraceCore`).
    *   **Status:** **Removed**.
    *   **Resolution:** Output was only consumed by `D3DTraceCore` (also removed). Removed from CMake build and deleted entirely.

*   **`driver/d3d/trace/D3DTraceCore/` (stub files)**
    *   **Description:** Leftover `.gen` stub files from the removed D3DTraceCore library.
    *   **Status:** **Removed**.
    *   **Resolution:** Stub files and empty parent directory `driver/d3d/trace/` deleted.

## 2. Current Trace Architecture

All trace playback now uses the apitrace-based pipeline:

| Path | Driver | Dispatcher | Format |
|------|--------|------------|--------|
| OpenGL | `TraceDriverApitrace` | `ApitraceCallDispatcherOGL` | `.trace` (apitrace) |
| D3D9 | `TraceDriverApitraceD3D` | `ApitraceCallDispatcherD3D` | `.trace` (apitrace) |
| MetaStream | `TraceDriverMeta` | N/A (direct) | `.tracefile.gz` |

## 3. Future Removal Candidates

No pending removal candidates. All legacy trace modules have been cleaned up.
