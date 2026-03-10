# driver/utils — Driver Utilities

Utilities shared by the driver/simulator bridge for trace replay, compressed I/O,
OpenGL call resolution, and MetaStream integration.

## Current Layout

```
driver/utils/
├── ApitraceParser/      # Apitrace parser library + OGL/D3D dispatchers
├── generated/           # Checked-in generated OpenGL mapping tables (.gen)
├── MetaTraceGenerator/  # MetaStream format and trace extraction helpers
├── TraceDriver/         # Trace driver interfaces and implementations
├── misc/                # Shared utility sources/headers (moved from root + log/)
└── README.md
```

## What Moved / Removed

- Moved into `misc/`:
  - `BufferDescriptor.*`, `MemoryRegion.*`, `DArray.*`
  - `GLJumpTable.*`, `GLResolver.*`
  - `zfstream.*`, `zlib.h`, `zconf.h`
  - `IncludeLog.*`, `LogObject.*` (from old `log/`)
- Removed from `driver/utils/`:
  - `OGLApiCodeGen/`, `Parsers/`, `GALTester/`, `UserCallTable.*`
- OpenGL headers are now canonical under `driver/ogl/inc/GL/`
  (root `driver/ogl/inc/` duplicates were removed).

## Component Notes

### TraceDriver/
Actively used trace entry points:
- `TraceDriverBase.h`
- `TraceDriverApitraceOGL.*`
- `TraceDriverApitraceD3D.*`
- `TraceDriverMeta.*`

Used by `arch/computrender.cpp`, `BhavModel`, and `PerfModel` startup paths.

### ApitraceParser/
CMake library target `ApitraceParser`:
- Parses `.trace` files (Snappy-backed format)
- Dispatches OGL/D3D calls into simulator driver stack
- Key files: `ApitraceParser.*`, `ApitraceCallDispatcherOGL.*`,
  `ApitraceCallDispatcherD3D.*`

### generated/
Checked-in generated tables consumed by runtime code:
- `APICall.gen`
- `GLFunctionNames.gen`
- `GLConstantNames.gen`
- `GLJumpTableFields.gen`
- `GLJumpTableSL.gen`
- `JumpTableWrapper.gen`
- `PlayableCalls.gen`
- `SwitchBranches.gen`

No codegen tool is required in normal build flow.

### misc/
Core shared utilities:
- Buffer/memory helpers: `BufferDescriptor.*`, `MemoryRegion.*`, `DArray.*`
- GL function mapping: `GLResolver.*`, `GLJumpTable.*`
- Logging: `IncludeLog.*`, `LogObject.*`
- Gzip streams: `zfstream.*` (+ local zlib headers)

`zfstream` is used broadly across `arch/common`, `perfmodel`, trace drivers, and tests.

### MetaTraceGenerator/
Defines MetaStream format and related extraction helpers.
`traceExtractor/RegisterWriteBufferMeta.*` is consumed by PerfModel build.

## Build Integration (main build)

Integrated through:
1. Direct source inclusion in `arch/CMakeLists.txt` and `arch/perfmodel/CMakeLists.txt`
2. `ApitraceParser` library target from `driver/utils/ApitraceParser/CMakeLists.txt`
3. Header include paths from:
   - `driver/utils/misc`
   - `driver/utils/generated`
   - `driver/ogl/inc/GL`

## Notes

- Keep `.gen` files committed and treated as build inputs.
- If adding new shared utility files, prefer placing them under `driver/utils/misc/`.
