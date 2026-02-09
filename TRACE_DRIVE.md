# TRACE_DRIVE.md — Trace Driver Architecture & Code Calling Structure

## Overview

The CG1 GPU simulator is **trace-driven**: it replays API call sequences captured from real
applications. Four trace driver paths exist, all converging at the same MetaStream interface
consumed by the simulation models.

```
    ┌───────────────────────────────────────────────────────────────┐
    │                       Input Traces                           │
    │                                                               │
    │  .txt / .txt.gz / .ogl.txt.gz     .PIXRun / .PIXRunz        │
    │  (GLInterceptor format)           (D3D9 PIX format)          │
    │                                                               │
    │  .trace (apitrace binary)         .metaStream.txt.gz          │
    │  (Snappy-compressed)              (Pre-compiled MetaStreams)  │
    └──────────────┬────────────────────────────┬──────────────────┘
                   │                            │
    ┌──────────────▼────────────────────────────▼──────────────────┐
    │                     CG1SIM main()                            │
    │                   (arch/CG1SIM.cpp)                          │
    │                                                               │
    │  Extension detection → TraceDriver instantiation              │
    └──────┬──────────┬──────────┬──────────┬──────────────────────┘
           │          │          │          │
    ┌──────▼───┐ ┌────▼────┐ ┌──▼───┐ ┌────▼──────┐
    │   OGL    │ │   D3D   │ │ Meta │ │ Apitrace  │
    │  Driver  │ │  Driver │ │Driver│ │  Driver   │
    └──────┬───┘ └────┬────┘ └──┬───┘ └────┬──────┘
           │          │         │           │
    ┌──────▼──────────▼─────────│───────────│───────────────────────┐
    │                           │           │                       │
    │   OGL/GAL/HAL Pipeline    │           │  (future)             │
    │                           │           │                       │
    │   GLExec → OGL2 →        │           │                       │
    │   GAL → HAL              │           │                       │
    │         │                 │           │                       │
    │   ┌─────▼─────────────────▼───────────▼──────────────────┐   │
    │   │              MetaStream Buffer                        │   │
    │   │   (HAL::metaStreamBuffer[] — circular queue)          │   │
    │   └─────────────────────────┬────────────────────────────┘   │
    └─────────────────────────────┼────────────────────────────────┘
                                  │
    ┌─────────────────────────────▼────────────────────────────────┐
    │                    Simulation Models                         │
    │                                                               │
    │  CG1BMDL (bhavmodel)          CG1CMDL (funcmodel)           │
    │  - Fast emulation              - Cycle-accurate simulation   │
    │  - emulateCommandProcessor()   - clock() all MDUs per cycle  │
    │                                                               │
    │  GPU Pipeline: CommandProcessor → Streamer → Clipper →       │
    │  Rasterizer → UnifiedShader → Z/Stencil → ColorWrite → DAC  │
    └──────────────────────────────────────────────────────────────┘
```

---

## 1. OpenGL Trace Path (GLInterceptor)

**Extensions**: `.txt`, `.txt.gz`, `.ogl.txt.gz`
**Platform**: Cross-platform (capture: Windows only)
**Status**: ✅ Full support

### Call Flow

```
CG1SIM::main()
  │
  ├─ fileExtensionTester("ogl.txt.gz" | "txt.gz" | "txt")
  │
  ▼
TraceDriverOGL(inputFile, HAL, shadedSetup, startFrame)
  │
  │  Constructor:
  │  ├─ GLExec::init(traceFile, "BufferDescriptors.dat", "MemoryRegions.dat")
  │  │   └─ TraceReader::open(traceFile)  // opens .txt.gz
  │  │   └─ BufferManager::open(bufferFile, memFile)  // loads buffer/memory data
  │  ├─ _setOGLFunctions()   // sets GLJumpTable → OGL2 entry points
  │  └─ ogl::initOGL(driver, startFrame)   // init OGL state machine
  │
  │  nxtMetaStream():
  │  ├─ HAL::nextMetaStream()  →  if MetaStream available, return it
  │  │
  │  ├─ (no MetaStream pending) → execute next GL call:
  │  │   ├─ GLExec::getCurrentCall()     // get APICall enum
  │  │   ├─ GLExec::executeCurrentCall() // dispatch via SwitchBranches.gen
  │  │   │   │
  │  │   │   ▼
  │  │   │  StubApiCalls.cpp::STUB_glClear(TraceReader, JumpTable, ...)
  │  │   │   ├─ Read args from trace:  mask = TR.readUInt()
  │  │   │   └─ Call jump table:       JT.glClear(mask)
  │  │   │       │
  │  │   │       ▼
  │  │   │  OGLEntryPoints.cpp::OGL_glClear(mask)
  │  │   │   ├─ Translate to GAL:  _ctx->gal().clear(...)
  │  │   │   │   │
  │  │   │   │   ▼
  │  │   │   │  GALDeviceImp::clearColorBuffer() / clearZStencilBuffer()
  │  │   │   │   ├─ _driver->writeGPURegister(GPU_CLEAR_COLOR, ...)
  │  │   │   │   └─ _driver->sendCommand(GPU_CLEARCOLORBUFFER)
  │  │   │   │       │
  │  │   │   │       ▼
  │  │   │   │  HAL::writeGPURegister(regId, idx, data)
  │  │   │   │   └─ registerWriteBuffer.add(regId, idx, data)
  │  │   │   │
  │  │   │   │  HAL::sendCommand(command)
  │  │   │   │   └─ registerWriteBuffer.flush()
  │  │   │   │       ├─ for each buffered write:
  │  │   │   │       │   _sendcgoMetaStream(new cgoMetaStream(REG_WRITE, ...))
  │  │   │   │       └─ _sendcgoMetaStream(new cgoMetaStream(COMMAND, ...))
  │  │   │   │           └─ metaStreamBuffer[writeIdx++] = ms  // enqueue
  │  │   │   │
  │  │   │   └─ return HAL::nextMetaStream()  // dequeue and return
  │  │   │
  │  │   └─ wglSwapBuffers → ogl::swapBuffers() → end-of-frame event
  │  │
  │  └─ return MetaStream to simulation
  │
  ▼
CG1BMDL::simulationLoop()  /  CG1CMDL::simulationLoop()
  └─ Consumes MetaStream → GPU pipeline execution
```

### Key Files

| File | Role |
|------|------|
| `driver/utils/TraceDriver/TraceDriverOGL.h/cpp` | OGL trace driver (top level) |
| `driver/utils/TraceReader/GLExec.h/cpp` | Trace file reader + call dispatcher |
| `driver/utils/TraceReader/StubApiCalls.cpp` | Auto-generated GL call stubs |
| `driver/utils/TraceReader/SwitchBranches.gen` | Auto-generated dispatch switch |
| `driver/ogl/OGL2/OGLEntryPoints.cpp` | OGL→GAL translation layer |
| `driver/gal/GAL/Implementation/GALDeviceImp.cpp` | GAL→HAL GPU register writes |
| `driver/hal/HAL.cpp` | MetaStream generation & buffer |
| `driver/hal/RegisterWriteBuffer.cpp` | Batch register writes into MetaStreams |

### Trace File Format

```
GLI0.t                                          # Header: "GLI0" + mode (t=text, h=hex)
# Trace generated with GLInterceptor 0.1
# Tue Mar 01 12:39:18 2005
wglChoosePixelFormat(201393414,*0)=7            # wgl* = Windows GL context setup
glEnable(GL_DEPTH_TEST)                         # GL calls with resolved enum names
glLightfv(GL_LIGHT0,GL_POSITION,{0.5,0.5,3,0}) # Arrays in {} notation
glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT)  # Bitmask with | operator
glBegin(GL_TRIANGLE_FAN)
glVertex3f(0,0,0.4)                             # Vertex data inline
glEnd()
wglSwapBuffers(0x00000000) = TRUE               # Frame boundary
```

### Auxiliary Files

| File | Contents | Required |
|------|----------|----------|
| `BufferDescriptors.dat` | VBO/texture buffer metadata | Yes (if trace uses buffers) |
| `MemoryRegions.dat` | Pre-allocated memory regions | Yes (if trace uses buffers) |

---

## 2. D3D9 PIX Trace Path

**Extensions**: `.PIXRun`, `.PIXRunz`, `.wpix`
**Platform**: Windows only
**Status**: ✅ Full support (Windows build)

### Call Flow

```
CG1SIM::main()
  │
  ├─ fileExtensionTester("pixrun" | "pixrunz" | "wpix")
  │
  ▼
TraceDriverD3D(inputFile, startFrame)
  │
  │  Constructor:
  │  ├─ createD3D9Trace()  // creates D3D9PixRunPlayer
  │  └─ D3D9::initialize(trace)
  │
  │  nxtMetaStream():
  │  ├─ HAL::nextMetaStream()  → if MetaStream available, return it
  │  │
  │  ├─ (no MetaStream pending) → execute next D3D call:
  │  │   ├─ trace->next()    // D3D9PixRunPlayer reads next PIX event
  │  │   │   │
  │  │   │   ▼
  │  │   │  D3D9PixRunPlayer::execute()
  │  │   │   ├─ Parse PIX binary event
  │  │   │   ├─ D3DInterface::SetRenderState(...)
  │  │   │   │   │
  │  │   │   │   ▼
  │  │   │   │  D3D9 → GAL → HAL → MetaStream
  │  │   │   │  (same GAL/HAL path as OGL)
  │  │   │   │
  │  │   │   └─ D3DInterface::DrawPrimitive(...)
  │  │   │       └─ GAL::draw() → HAL::sendCommand() → MetaStream
  │  │   │
  │  │   └─ Present() → end-of-frame event
  │  │
  │  └─ return MetaStream to simulation
  │
  ▼
Simulation (same as OGL path)
```

### Key Files

| File | Role |
|------|------|
| `driver/utils/TraceDriver/TraceDriverD3D.h/cpp` | D3D trace driver |
| `driver/d3d/trace/D3DTraceCore/` | PIX trace reader library |
| `driver/d3d/D3D9/` | D3D9→GAL translation |
| `driver/gal/GAL/Implementation/` | GAL→HAL (shared with OGL) |

---

## 3. MetaStream Trace Path (Pre-compiled)

**Extensions**: `.metaStream.txt.gz`, `.tracefile.gz`
**Platform**: Cross-platform
**Status**: ✅ Full support

### Call Flow

```
CG1SIM::main()
  │
  ├─ (else clause — not OGL/D3D/apitrace extension)
  ├─ Open file, read MetaStreamHeader, verify signature
  │
  ▼
TraceDriverMeta(ProfilingFile, startFrame, headerStartFrame)
  │
  │  Constructor:
  │  └─ Opens gzip stream, sets initial phase
  │
  │  nxtMetaStream():
  │  ├─ Phase: PREINIT → load initial GPU register state
  │  ├─ Phase: LOAD_REGS → load register writes from header
  │  ├─ Phase: LOAD_SHADERS → load shader programs
  │  ├─ Phase: SIMULATION → read MetaStreams from file
  │  │   │
  │  │   ▼
  │  │  cgoMetaStream* ms = new cgoMetaStream(MetaTraceFile)
  │  │   ├─ Reads binary MetaStream directly from .gz file
  │  │   ├─ Contains: REG_WRITE, COMMAND, MEM_WRITE, EVENT
  │  │   └─ No API translation needed (pre-compiled)
  │  │
  │  └─ return MetaStream to simulation
  │
  ▼
Simulation (same consumption path)
```

### Key Advantage

**No API overhead**: MetaStream traces bypass the entire OGL/D3D/GAL/HAL stack.
Pre-compiled by `traceTranslator` tool from GLInterceptor traces.

### Key Files

| File | Role |
|------|------|
| `driver/utils/TraceDriver/TraceDriverMeta.h/cpp` | MetaStream trace driver |
| `arch/funcmodel/CommandProcessor/MetaStream.h` | MetaStream format definition |
| `driver/utils/MetaTraceGenerator/traceTranslator/` | GLI→MetaStream converter tool |

---

## 4. Apitrace Path

**Extensions**: `.trace`
**Platform**: Cross-platform
**Status**: ✅ Functional (111 GL calls, verified byte-identical output)

### Call Flow

```
CG1SIM::main()
  │
  ├─ fileExtensionTester("trace")
  │
  ▼
TraceDriverApitrace(inputFile, HAL, startFrame)
  │
  │  Constructor:
  │  ├─ ApitraceParser::open(traceFile)
  │  │   ├─ SnappyStream::open()           // verify 'at' header, init chunk reader
  │  │   ├─ readVarUInt(version)            // trace format version (typically 4-6)
  │  │   ├─ readProperties()               // if version >= 6: OS, driver info
  │  │   └─ Signature cache initialized     // for efficient repeated call lookup
  │  │
  │  ├─ ogl::initOGL(driver, startFrame)   // init OGL/GAL subsystem
  │  └─ Skip to startFrame (count SwapBuffers events)
  │
  │  nxtMetaStream():
  │  ├─ HAL::nextMetaStream() → if MetaStream available, return it
  │  │
  │  ├─ (no MetaStream pending) → read and dispatch next call:
  │  │   ├─ ApitraceParser::readEvent(evt)
  │  │   │   ├─ Read event type (CALL_ENTER / CALL_LEAVE)
  │  │   │   ├─ Read/cache call signature (function name + arg names)
  │  │   │   └─ Read call details: typed arguments (uint/float/blob/enum/array)
  │  │   │
  │  │   ├─ ApitraceCallDispatcher::dispatchCall(evt)
  │  │   │   ├─ Extract typed args: asUInt(A(0)), asFloat(A(1)), asVoidPtr(A(2))...
  │  │   │   └─ Call OGL entry point: OGL_glClear(mask), OGL_glVertex3f(x,y,z), etc.
  │  │   │       │
  │  │   │       ▼  (same path as OGL TraceDriver from here)
  │  │   │   OGLEntryPoints → GALDeviceImp → HAL::writeGPURegister()
  │  │   │       └─ RegisterWriteBuffer::flush() → MetaStream enqueued
  │  │   │
  │  │   └─ SwapBuffers → ogl::swapBuffers() → end-of-frame event
  │  │
  │  └─ return MetaStream to simulation
  │
  ▼
CG1BMDL / CG1CMDL simulation (identical to OGL path)
```

### Apitrace Binary Format

```
file = 'a' 't' chunk*                // Snappy-compressed chunks

chunk = compressed_length(uint32_LE) compressed_data(bytes)

trace = version_no event*            // after decompression

event = 0x00 [thread_no] call_sig call_detail*    // CALL_ENTER
      | 0x01 call_no call_detail*                  // CALL_LEAVE

call_sig = id function_name count arg_name*        // first occurrence
         | id                                       // follow-on (cached)

call_detail = 0x00                                  // end
            | 0x01 arg_no value                     // argument
            | 0x02 value                            // return value

value = 0x00         // null
      | 0x01         // false
      | 0x02         // true
      | 0x03 uint    // negative int
      | 0x04 uint    // positive int
      | 0x05 float32 // float
      | 0x07 string  // string (length-prefixed)
      | 0x08 blob    // binary blob (length-prefixed)
      | 0x09 enum_sig value  // enum
      | 0x0b count value*    // array
```

### Key Files

| File | Role |
|------|------|
| `driver/utils/ApitraceParser/ApitraceParser.h/cpp` | Binary format parser (Snappy, varuint, Values) |
| `driver/utils/ApitraceParser/ApitraceCallDispatcher.h/cpp` | 111 GL calls → OGL_gl* entry point dispatch |
| `driver/utils/TraceDriver/TraceDriverApitrace.h/cpp` | Trace driver (HAL drain → dispatch → repeat) |
| `common/thirdparty/snappy-1.1.10/` | Snappy decompression library |
| `tests/ogl/trace/apitrace_triangle/triangle.trace` | Test trace (verified byte-identical output) |

---

## 5. Simulation Consumption Path

All trace drivers produce `cgoMetaStream` objects consumed by the simulation models.

### Bhavmodel (Fast Emulation)

```
CG1BMDL::simulationLoop()
  │
  ├─ TraceDriver->startTrace()
  │
  └─ Loop until traceEnd or frameLimit:
      ├─ TraceDriver->nxtMetaStream()  →  cgoMetaStream* ms
      │
      ├─ if (ms != NULL):
      │   └─ GpuBMdl.emulateCommandProcessor(ms)
      │       ├─ Decode MetaStream type (REG_WRITE, COMMAND, ...)
      │       ├─ Update GPU state registers
      │       ├─ On DRAW → emulate full pipeline:
      │       │   Streamer → PrimAssembly → Clipper → Rasterizer →
      │       │   Shader → TextureProcessor → FragmentOp → Output
      │       └─ On SWAPBUFFERS → dump frame, increment counter
      │
      └─ if (ms == NULL): traceEnd = true
```

### Funcmodel (Cycle-Accurate)

```
CG1CMDL::simulationLoop()
  │
  ├─ Cycle loop:
  │   ├─ Clock all MDUs:
  │   │   CommandProcessor.clock(cycle)
  │   │   ├─ Internally calls TraceDriver->nxtMetaStream()
  │   │   └─ Feeds MetaStreams to GPU register file
  │   │
  │   │   Streamer.clock(cycle)
  │   │   PrimitiveAssembler.clock(cycle)
  │   │   Clipper.clock(cycle)
  │   │   Rasterizer.clock(cycle)
  │   │   UnifiedShader[0..N].clock(cycle)
  │   │   TextureProcessor[0..N].clock(cycle)
  │   │   ZStencilTest[0..N].clock(cycle)
  │   │   ColorWrite[0..N].clock(cycle)
  │   │   MemoryController.clock(cycle)
  │   │   DisplayController.clock(cycle)
  │   │
  │   ├─ Update signals between MDUs
  │   ├─ Check statistics / stalls
  │   └─ Increment cycle
  │
  └─ End when simCycles or simFrames reached
```

---

## 6. Convergence Point: MetaStream

All paths converge at the `cgoMetaStream` interface:

```cpp
struct cgoMetaStream {
    enum Type { REG_WRITE, REG_READ, MEM_WRITE, MEM_READ, COMMAND, EVENT };
    Type type;
    U32  regId;        // GPU register ID (from GPUReg.h)
    U32  subRegId;     // Sub-register index
    GPURegData data;   // Register data (union of U32/F32/bool/etc.)
};
```

### MetaStream Types

| Type | Meaning | Example |
|------|---------|---------|
| `REG_WRITE` | Write GPU register | Set blend mode, shader program, texture params |
| `MEM_WRITE` | Write GPU memory | Upload texture data, vertex buffers |
| `COMMAND` | Execute GPU command | `GPU_DRAW`, `GPU_SWAPBUFFERS`, `GPU_CLEARCOLORBUFFER` |
| `EVENT` | Synchronization | Frame boundary, fence |

---

## 7. Format Comparison

| Feature | GLInterceptor | D3D PIX | MetaStream | Apitrace |
|---------|--------------|---------|------------|----------|
| **Format** | Text | Binary | Binary | Binary (Snappy) |
| **Compression** | gzip | None/LZMA | gzip | Snappy/Brotli |
| **Buffer data** | External `.dat` | Embedded | Embedded | Embedded blobs |
| **API level** | OpenGL calls | D3D9 calls | GPU commands | OpenGL calls |
| **Translation** | Full stack | Full stack | None (direct) | Full stack (planned) |
| **Speed** | Slow (text parse) | Medium | Fast (no translation) | Medium (planned) |
| **Portability** | Windows capture | Windows only | Cross-platform | Cross-platform |
| **Standard** | CG1-specific | Microsoft | CG1-specific | Community standard |
| **Tools** | GLTracePlayer | PIX | traceTranslator | apitrace CLI |

---

## 8. Adding a New Trace Format

To add support for a new trace format:

1. **Create parser** in `driver/utils/NewParser/`:
   - Read the file format, extract API calls and arguments

2. **Create trace driver** in `driver/utils/TraceDriver/TraceDriverNew.h/cpp`:
   - Inherit from `cgoTraceDriverBase`
   - Implement `startTrace()`, `nxtMetaStream()`, `getTracePosition()`
   - Either:
     - **(A)** Call OGL/D3D entry points directly → GAL → HAL → MetaStream
     - **(B)** Generate MetaStreams directly (requires MetaStream format knowledge)
     - **(C)** Convert to GLI text format → reuse GLExec path

3. **Register in CG1SIM.cpp**:
   - Add `#include "TraceDriverNew.h"`
   - Add `fileExtensionTester()` check
   - Instantiate `TraceDriverNew`

4. **Update build system**:
   - Add source to `arch/CMakeLists.txt` (OGLSRC)
   - Add include paths
   - Link dependencies
