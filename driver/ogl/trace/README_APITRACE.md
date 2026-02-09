# Apitrace Support in CG1 Simulator

## Status: Experimental (Framework Ready, Parameter Conversion In Progress)

The CG1 GPU simulator now includes initial support for apitrace binary trace format (`.trace` files).

### What Works

- ✅ Apitrace binary format parsing (Snappy decompression, event reading)
- ✅ Call signature extraction and caching
- ✅ `.trace` file detection in CG1SIM
- ✅ Build integration (Snappy library, ApitraceParser, TraceDriverApitrace)

### What's In Progress

- 🚧 **Parameter conversion**: Mapping apitrace typed values to CG1 MetaStream arguments
- 🚧 **Blob handling**: Converting binary buffer data to BufferDescriptor/MemoryRegion
- 🚧 **Full API coverage**: Currently uses GLResolver::getFunctionID() for all 1874+ GL calls

### Architecture

```
apitrace .trace file (binary)
        ↓
ApitraceParser (Snappy decompression)
        ↓
CallEvent (function name + typed arguments)
        ↓
TraceDriverApitrace (APICall mapping)
        ↓
MetaStream → CG1 Simulator
```

### Usage (when complete)

```bash
# Capture with apitrace
apitrace trace --api gl glxgears
# → glxgears.trace

# Simulate with CG1
cd _BUILD_/arch
cp ../../arch/common/params/CG1GPU.csv .
./CG1SIM --trace /path/to/glxgears.trace --frames 10
```

### Implementation Files

| File | Purpose |
|------|---------|
| `driver/utils/ApitraceParser/ApitraceParser.h/cpp` | Binary format parser |
| `driver/utils/TraceDriver/TraceDriverApitrace.h/cpp` | Trace driver adapter |
| `common/thirdparty/snappy-1.1.10/` | Snappy decompression library |

### Next Steps

1. **Complete parameter conversion**:
   - Implement `convertArgument(Value, outData)` for all GL parameter types
   - Handle arrays, blobs, enums, bitmasks
   
2. **Integrate BufferManager**:
   - Convert apitrace blobs to `BufferDescriptor.dat` format
   - Store in HAL memory management system
   
3. **Test with real traces**:
   - Capture simple GL apps (triangle, cube, textured quad)
   - Verify rendered output matches apitrace replay
   
4. **Add regression tests**:
   - Extend `tools/script/regression/` with apitrace test cases

### Differences: CG1 GLInterceptor vs Apitrace

| Feature | GLInterceptor (.txt.gz) | Apitrace (.trace) |
|---------|-------------------------|-------------------|
| Format | Text-based, human-readable | Binary, compact |
| Buffer data | External .dat files | Embedded as blobs |
| Compression | gzip | Snappy (faster) |
| Tools | GLTracePlayer (Windows) | Cross-platform replay |
| Editability | Text editor | apitrace dump/edit |

**Recommendation**: Use GLInterceptor for debugging/development (easy to inspect), apitrace for production/sharing (standard format, better tools).

## Development Guide

### Adding API Call Support

The `mapFunctionName()` now uses `GLResolver::getFunctionID()` for automatic mapping.
Focus on parameter conversion:

```cpp
// In TraceDriverApitrace::nxtMetaStream()
if (apiCall == APICall_glTexImage2D) {
    // Extract: target, level, internalformat, width, height, border, format, type, pixels(blob)
    auto& args = evt.arguments;
    GLenum target = args[0].uintVal;
    GLint level = args[1].intVal;
    // ... extract all params
    
    // Generate MetaStream command
    // (Use existing GLExec call flow as reference)
}
```

### Testing

```bash
# 1. Capture a simple GL trace
apitrace trace glxgears -o test.trace

# 2. Verify parse (standalone test)
./ApitraceParserTest test.trace

# 3. Run through simulator
./CG1SIM --trace test.trace --frames 1
```

### Known Limitations

- OpenGL version: Apitrace supports GL 1.0-4.6, CG1 supports GL 1.4-2.0 subset
- Extensions: Many GL extensions in apitrace traces may not be implemented
- Platform calls: wgl* (Windows), glX* (X11), egl* (embedded) need platform translation

