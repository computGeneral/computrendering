# Project Cleanup & Migration Status

This document tracks the removal of legacy modules as the project migrates to **apitrace** for trace capture and playback.

## 1. Completed Removals (Phase 1)

The following legacy modules have been successfully removed from the codebase:

*   **`driver/ogl/trace/GLInterceptor`**
    *   **Description:** Source code for the custom OpenGL interception DLL (Windows-only).
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by the cross-platform `apitrace` CLI. Essential headers (`Specific.h`, `SpecificStats.h`) were migrated to `driver/utils/OGLApiCodeGen` to preserve build integrity.

*   **`driver/ogl/trace/GLTracePlayer`**
    *   **Description:** Standalone Windows GUI tool for replaying GLInterceptor traces.
    *   **Status:** **Removed**.
    *   **Resolution:** Replaced by `apitrace replay` (glretrace) and the internal simulator player.

*   **`driver/ogl/trace/GLInstrument` & `driver/ogl/trace/GLInstrumentTool`**
    *   **Description:** Legacy Windows-specific OpenGL instrumentation framework and tools.
    *   **Status:** **Removed**.
    *   **Resolution:** Obsolete in the new Apitrace workflow; unused in Linux build.

*   **`driver/utils/TraceReader` & `driver/utils/TraceDriver/TraceDriverOGL`**
    *   **Description:** Reader and driver for `.ogl.txt.gz` GLInterceptor traces.
    *   **Status:** **Removed**.
    *   **Resolution:** Switch to Apitrace as the primary OGL trace source. Legacy traces removed.

## 2. Future Removal Candidates

The following modules are legacy artifacts but are currently **required** for dependencies or missing features. They can be removed in future phases.

### C. D3D9 PIX Support
*   **Modules:** `driver/d3d/trace` (D3DTraceCore, D3DTracePlayer)
*   **Description:** Support for legacy Microsoft PIX traces.
*   **Blocker:** `TraceDriverApitrace` currently **only supports OpenGL**. Removing this would drop D3D simulation support.
*   **Action Required:** Implement Direct3D call mapping in `TraceDriverApitrace`.

### D. MetaStream Translator
*   **Modules:** `driver/utils/MetaTraceGenerator/traceTranslator`
*   **Description:** Converts traces to optimized MetaStream format.
*   **Blocker:** Currently depends on `TraceReader`.
*   **Action Required:** Update tool to use `ApitraceParser` as input.

## 3. Migration Roadmap (Phase 2)

To enable further cleanup, the following engineering tasks are required:

1.  **Convert Regression Suite:** Capture or convert existing tests (`triangle`, `bunny`, etc.) to `.trace` format and update `tools/script/regression/regression_list`.
2.  **Update Translator:** Port `traceTranslator` to read from `ApitraceParser` instead of `TraceReader`.
3.  **D3D Support:** Extend `TraceDriverApitrace` to handle D3D9 calls.
