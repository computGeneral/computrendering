# Apitrace Support — Status & Remaining Work

## Current Status (2026-02-27)

✅ **OGL Support Complete**: Binary parser, Snappy decompression, 111 GL call dispatcher, full simulation  
✅ **D3D9 Support Complete**: 80+ D3D9 API calls dispatched via D3DApitraceCallDispatcher  
✅ **Legacy PIX Removed**: TraceDriverD3D, D3DTraceCore, D3DTracePlayer all removed  
✅ **Legacy GLInterceptor Removed**: TraceDriverOGL, TraceReader, GLExec all removed  
✅ **Verified**: glxgears.trace → byte-identical PPM output vs reference  
✅ **D3D9 Verified**: FruitNinja.trace → functional rendering  
📝 **Documented**: See `README_APITRACE.md`, `TRACE_DRIVE.md`, `APITRACE_DEV_SKILL.md`

### Completed Milestones

- ✅ Apitrace binary format parsing (Snappy, varuint, typed Values, signature caching)
- ✅ 111 GL calls dispatched directly to OGL_gl* entry points
- ✅ 80+ D3D9 calls dispatched to AIDeviceImp9 via D3DApitraceCallDispatcher
- ✅ Full OGL→GAL→HAL→MetaStream pipeline from .trace files
- ✅ Full D3D9→GAL→HAL→MetaStream pipeline from .trace files
- ✅ API auto-detection (OGL vs D3D9) from trace header
- ✅ Both bhavmodel and funcmodel simulation from .trace input
- ✅ Frame boundary detection (SwapBuffers / Present variants)
- ✅ Frame limiting (`--frames N`) in all trace drivers (base class)
- ✅ VALUE_REPR parsing for D3D9 shader bytecode
- ✅ Lock/Unlock data transfer (memcpy handler)
- ✅ OpaquePointerTracker for COM object mapping
- ✅ CreateVertexDeclaration VALUE_ARRAY parsing
- ✅ Regression test infrastructure (regression.ps1, regression.sh, regression_list)
- ✅ Debug diagnostics cleaned, code review issues fixed

### Implemented GL Call Categories

| Category | Calls | Examples |
|----------|-------|---------|
| State management | 7 | glEnable, glDisable, glShadeModel, glCullFace |
| Clear | 4 | glClear, glClearColor, glClearDepth, glClearStencil |
| Viewport/Scissor | 3 | glViewport, glScissor, glDepthRange |
| Matrix | 13 | glMatrixMode, glLoadIdentity, glOrtho, glTranslatef, ... |
| Immediate mode | 14 | glBegin/glEnd, glVertex*, glColor*, glNormal*, glTexCoord* |
| Vertex arrays | 8 | glEnableClientState, glVertexPointer, glDrawArrays/Elements |
| VBO (ARB) | 6 | glBindBuffer, glBufferData, glVertexAttribPointer |
| Texture | 18 | glBindTexture, glTexImage2D, glTexParameter*, glTexEnv* |
| Lighting | 8 | glLightfv, glMaterialfv, glColorMaterial |
| Depth/Stencil/Blend | 11 | glDepthFunc, glStencilOp, glBlendFunc, glAlphaFunc |
| Fog | 4 | glFogf, glFogi, glFogfv, glFogiv |
| ARB Programs | 5 | glBindProgramARB, glProgramStringARB, glProgramEnvParameter* |
| Push/Pop | 2 | glPushAttrib, glPopAttrib |
| Misc | 2 | glFlush, glGetString |
| Display Lists | 3 | glNewList, glEndList, glCallList |
| **Total** | **111** | |

---

## Remaining Work

### Rendering Improvements (Deferred)

- [ ] Fix `bytesPixel` for `GPU_RGBA32F` in bhavmodel `dumpFrame()` — D3D frames render black
- [ ] Multiple simultaneous Lock/Unlock support (current memcpy uses first-pending heuristic)
- [ ] Image quality validation for D3D9 traces (pixel correctness, format handling)

### Coverage Expansion

- [ ] Additional D3D9 calls: GetFunction, GetDevice, BeginStateBlock, EndStateBlock, Capture, Apply
- [ ] Additional GL calls for complex traces (GL 2.0 GLSL shaders, FBO, MRT)
- [ ] Enum signature name preservation from apitrace (currently numeric-only)

### Regression Tests

- [ ] Add D3D9 regression test entries to regression_list
- [ ] Convert more legacy GLInterceptor test traces to apitrace format
- [ ] Add tolerance-based image comparison for D3D9 tests

### Performance & Robustness

- [ ] 64-bit build support for large traces
- [ ] Performance tuning for large trace files
- [ ] Better error reporting for unsupported API calls

---

## Architecture Reference

```
.trace file (Snappy compressed binary)
        ↓
ApitraceParser::readEvent() → CallEvent{functionName, arguments[Value]}
        ↓
    ┌── OGL ──────────────────────────┐   ┌── D3D9 ──────────────────────────┐
    │ ApitraceCallDispatcher          │   │ D3DApitraceCallDispatcher        │
    │   asUInt/asFloat/asVoidPtr...   │   │   MA()/asOpaquePtr/asDWORD...   │
    │   OGL_gl*(args...)              │   │   dev->Method(args...)           │
    └────────────┬────────────────────┘   └────────────┬──────────────────────┘
                 ↓                                      ↓
    GALDeviceImp → HAL::writeGPURegister() → RegisterWriteBuffer::flush()
                 ↓
    MetaStream → CG1BMDL/CG1CMDL simulation
```
