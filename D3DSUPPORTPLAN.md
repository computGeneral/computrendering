# D3D Apitrace Support — Completed Status

## Scope Completed

This document closes the original D3D support plan with the current agreed scope:

- D3D9 `.trace` (apitrace binary) playback is implemented.
- Sample D3D trace playback is considered **pass**.
- Legacy PIX pipeline (`driver/d3d/trace/*`) is removed from source and build wiring.
- Frame limiting (`--frames`) is enforced in trace drivers, including D3D/OGL apitrace paths.

Rendering correctness improvements (image quality / black frames / output parity) are intentionally deferred to follow-up work.

---

## Implemented Architecture

### Active D3D path

`.trace` (apitrace) → `ApitraceParser` → `D3DApitraceCallDispatcher` → D3D9 GAL (`AIDeviceImp9` et al.) → HAL → MetaStream → simulator

### Active OGL path

`.trace` (apitrace) → `ApitraceParser` → `ApitraceCallDispatcher` → OGL entry points/GAL → HAL → MetaStream → simulator

---

## Legacy PIX Removal (Done)

Removed / disabled legacy PIX code and references:

- Deleted source tree: `driver/d3d/trace/*`
- Deleted legacy trace driver files:
  - `driver/utils/TraceDriver/TraceDriverD3D.h`
  - `driver/utils/TraceDriver/TraceDriverD3D.cpp`
- Removed CG1 runtime selection for `.pixrun/.pixrunz/.wpix`
- Removed CMake integration for `D3DTraceCore`
- Removed stale `MetaTraceGenerator/traceTranslator` references to `TraceDriverD3D` and `D3DTraceCore`

---

## Build/Config Updates (Done)

- `CMakeLists.txt`: no longer adds `driver/d3d/trace/D3DTraceCore`
- `arch/CMakeLists.txt`:
  - removes `TraceDriverD3D.cpp` from D3D sources
  - removes `D3DTraceCore` from linked driver libs
- `driver/d3d/D3D9/CMakeLists.txt`:
  - removes include directory dependency on `driver/d3d/trace/D3DTraceCore`
- `arch/CG1SIM.cpp`:
  - removes `TraceDriverD3D` include and PIX extension branch
  - keeps D3D apitrace detection/dispatch path

---

## Runtime Behavior (Current)

- `.trace` input auto-detects API type (`d3d9` or `gl`) and chooses the apitrace driver.
- `--frames` limit is passed to trace drivers and stops execution after requested frame count.

---

## Deferred Work (Intentional)

To be handled in follow-up tasks:

- Rendering result validation/fixes (pixel correctness, format handling, parity checks)
- Remaining unhandled D3D calls and coverage expansion
- Performance tuning and large-trace robustness

---

## Acceptance

This plan is marked **completed** for the agreed milestone:

- **D3D apitrace playback support established**
- **Legacy PIX trace pipeline removed**
- **Rendering-fidelity work deferred**
