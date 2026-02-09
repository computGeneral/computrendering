# Apitrace Support — Status & Remaining Work

## Current Status (2026-02-09)

✅ **Phase 1 Complete**: Binary parser, Snappy decompression, call dispatcher, full simulation  
✅ **Verified**: apitrace triangle.trace → byte-identical PPM output vs GLInterceptor reference  
📝 **Documented**: See `driver/ogl/trace/README_APITRACE.md`, `TRACE_DRIVE.md`

### What Works

- ✅ Apitrace binary format parsing (Snappy, varuint, typed Values, signature caching)
- ✅ 111 GL calls dispatched directly to OGL_gl* entry points
- ✅ Full OGL→GAL→HAL→MetaStream pipeline from .trace files
- ✅ Both bhavmodel and funcmodel simulation from .trace input
- ✅ Frame boundary detection (SwapBuffers variants)
- ✅ Simple geometry rendering (triangle test — byte-identical output)

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
| **Total** | **111** | |

---

## Phase 2: Extended Trace Support (Estimated: 3-5 days)

### 2.1 Complex Trace Testing

Test with traces beyond simple triangles:

```bash
# Capture with apitrace (requires apitrace installed + GL app)
apitrace trace --api gl glxgears
apitrace trace --api gl <your_opengl_app>

# Simulate
cd _BUILD_/arch
./CG1SIM --trace /path/to/app.trace --frames 5

# Compare output
apitrace dump-images app.trace  # generates reference PNGs
```

**Expected issues with complex traces:**
- Missing GL calls (check `[ApitraceDispatch] Unhandled GL call:` warnings)
- Blob data for textures/VBOs may need BufferManager integration
- Enum resolution: apitrace uses enum_sig which we currently skip

### 2.2 Enum Signature Handling

Currently `VALUE_ENUM` just reads the inner value. Full implementation:

```cpp
case VALUE_ENUM: {
    uint64_t sigId;
    readVarUInt(sigId);
    // First occurrence: read enum name/value pairs
    // Follow-on: use cached enum_sig
    // Then read the actual value
    return readValue(val);  // current: works but loses enum name info
}
```

### 2.3 Bitmask Signature Handling

Similar to enums — `VALUE_BITMASK` currently falls through. Needs:
```cpp
case VALUE_BITMASK: {
    uint64_t sigId;
    readVarUInt(sigId);
    // Read bitmask signature on first occurrence
    return readValue(val);
}
```

### 2.4 Struct Value Handling

`VALUE_STRUCT` for complex parameters (e.g., pixel format descriptors):
```cpp
case VALUE_STRUCT: {
    uint64_t sigId;
    readVarUInt(sigId);
    // Read struct signature on first occurrence
    // Read member values
}
```

---

## Phase 3: Buffer/Blob Integration (Estimated: 2-3 days)

### Current State

Blob data (textures, vertex buffers) is passed directly via pointer from
`Value::blobVal.data()`. This works for immediate-mode data that's consumed
in the same call (e.g., glTexImage2D pixel data).

### Remaining Work

For VBO/buffer object data that persists:
```cpp
// In ApitraceCallDispatcher, when handling glBufferData:
if (fn == "glBufferData") {
    // A(2) is the blob data — passed directly as void*
    // Works because OGL_glBufferDataARB copies the data internally
    OGL_glBufferDataARB(asEnum(A(0)), A(1).uintVal, asVoidPtr(A(2)), asEnum(A(3)));
}
```

**Already handled** — `asVoidPtr()` returns `blobVal.data()` directly, and OGL
functions copy the data internally. No explicit BufferManager needed for basic cases.

---

## Phase 4: Regression Test Integration (Estimated: 1 day)

### Add apitrace triangle to regression suite

Update `tools/script/regression/regression_list`:
```
ogl/apitrace_triangle, CG1GPU.ini, triangle.trace, 1, 0, 90
```

Update `tools/script/regression/regression.sh`:
- Detect `.trace` extension in trace file column
- No need to copy BufferDescriptors.dat (not used by apitrace path)

### Generate more test traces

Create Python script to generate test traces:
```bash
python3 tools/script/gen_apitrace.py --scene textured_quad -o tests/ogl/trace/apitrace_texquad/texquad.trace
python3 tools/script/gen_apitrace.py --scene lit_sphere -o tests/ogl/trace/apitrace_sphere/sphere.trace
```

---

## Phase 5: Documentation & Polish (Estimated: 1 day)

- [x] `driver/ogl/trace/README_APITRACE.md` — architecture and usage
- [x] `TRACE_DRIVE.md` — complete call flow documentation
- [x] `README.md` — supported trace formats table
- [ ] Update `CLAUDE.md` with apitrace section
- [ ] Add `tools/script/gen_apitrace.py` — trace generator for testing
- [ ] Document known GL call coverage gaps

---

## Known Limitations

| Limitation | Impact | Mitigation |
|-----------|--------|------------|
| OpenGL version | CG1 supports GL 1.4-2.0 subset | Traces using GL 3.0+ features will fail |
| 111/1874 calls mapped | Complex traces may have unmapped calls | Warnings printed, calls skipped |
| Platform calls skipped | wgl/glX/egl context management ignored | OGL subsystem pre-initialized |
| No GLSL shaders | Only ARB vertex/fragment programs | Limit to ARB shader traces |
| Enum signatures | Enum names not preserved from apitrace | Numeric values used directly |
| Thread support | Single-threaded only | Multi-threaded traces may misbehave |

---

## Architecture Reference

```
.trace file (Snappy compressed binary)
        ↓
ApitraceParser::readEvent() → CallEvent{functionName, arguments[Value]}
        ↓
ApitraceCallDispatcher::dispatchCall(evt)
        ↓
    ┌─ asUInt/asFloat/asEnum/asVoidPtr... (type extraction)
    └─ OGL_gl*(args...)  (direct entry point call)
        ↓
    GALDeviceImp → HAL::writeGPURegister() → RegisterWriteBuffer::flush()
        ↓
    MetaStream → CG1BMDL/CG1CMDL simulation
```
