# Apitrace Support - Remaining Work

## Current Status (2026-02-09)

✅ **Framework Complete**: Binary parser, Snappy decompression, trace driver skeleton  
🚧 **Parameter Conversion**: Needs full implementation (see below)  
📝 **Documented**: See `driver/ogl/trace/README_APITRACE.md`

## Phase 1: Parameter Conversion (Estimated: 1-2 weeks)

### Core Challenge

Convert apitrace typed `Value` objects to GL parameter types and generate MetaStreams.

**Approach 1: Direct MetaStream Generation** (Complex)
- For each mapped APICall, manually extract arguments from `evt.arguments`
- Call HAL functions to generate MetaStreams
- Requires understanding MetaStream format for each GL call

**Approach 2: Reuse GLExec** (Recommended)
- Convert apitrace CallEvent to GLI text format
- Feed to existing `GLExec` class (used by TraceDriverOGL)
- Leverage existing parameter parsing logic

### Implementation Steps (Approach 2)

1. **Create `ApitraceToGLI` converter**:
```cpp
// driver/utils/ApitraceParser/ApitraceToGLI.cpp
std::string ApitraceToGLI::convertCall(const CallEvent& evt) {
    std::stringstream ss;
    ss << evt.signature.functionName << "(";
    
    for (size_t i = 0; i < evt.signature.argNames.size(); ++i) {
        if (i > 0) ss << ",";
        ss << valueToGLIString(evt.arguments[i]);
    }
    ss << ")";
    
    if (evt.hasReturn) {
        ss << " = " << valueToGLIString(evt.returnValue);
    }
    
    return ss.str();
}
```

2. **Value to GLI string conversion**:
```cpp
std::string valueToGLIString(const Value& val) {
    switch (val.type) {
        case VALUE_UINT: return std::to_string(val.uintVal);
        case VALUE_FLOAT: return std::to_string(val.floatVal);
        case VALUE_STRING: return val.strVal;
        case VALUE_BLOB: return "*" + std::to_string(storeBlobAndGetID(val));
        case VALUE_ARRAY: // format as {elem1,elem2,...}
        case VALUE_ENUM: // resolve enum name or use numeric value
        // ...
    }
}
```

3. **Stream to GLExec**:
```cpp
// In TraceDriverApitrace:
GLExec glexec_;
std::stringstream gliStream_;

// In constructor:
glexec_.initFromStream(gliStream_);

// In nxtMetaStream():
std::string gliCall = ApitraceToGLI::convertCall(evt);
gliStream_ << gliCall << "\\n";
return glexec_.nextMetaStream();  // GLExec parses and generates MetaStream
```

## Phase 2: Buffer/Blob Integration (Estimated: 3-5 days)

### Blob Handling Strategy

Apitrace stores buffer data as inline blobs. CG1 uses separate `.dat` files.

**Solution**: Runtime BufferManager population
```cpp
// When VALUE_BLOB encountered:
U32 bufID = driver_->getBufferManager().allocate(val.blobVal.size());
driver_->writeGPUMemory(bufID, val.blobVal.data(), val.blobVal.size());
// Replace blob with buffer ID in converted call
```

## Phase 3: Testing (Estimated: 2-3 days)

### Test Cases

1. **Simple geometric primitives**:
```bash
# Capture glxgears
apitrace trace glxgears -o tests/ogl/trace/apitrace_glxgears/glxgears.trace

# Simulate
./CG1SIM --trace glxgears.trace --frames 10

# Compare with apitrace replay
apitrace dump-images glxgears.trace
diff frame0000.ppm glxgears-0000.png
```

2. **Textured scenes**:
   - Capture texture upload (glTexImage2D with blob data)
   - Verify texture rendering matches reference

3. **Shader programs**:
   - ARB vertex/fragment programs
   - GLSL shaders (if CG1 supports)

### Validation

- ✅ Parser opens .trace files without crash
- ✅ Reads call sequence correctly
- 🚧 Generates valid MetaStreams
- 🚧 Renders identical output to apitrace replay

## Phase 4: Documentation & Polish (Estimated: 1 day)

- [ ] Update `driver/ogl/trace/README.md` with apitrace workflow
- [ ] Add example traces to `tests/ogl/trace/apitrace_samples/`
- [ ] Document known limitations (GL version, extensions)
- [ ] Add to regression test suite

## Alternative: Apitrace-to-GLI Converter Tool

If direct integration is too complex, create a standalone converter:

```bash
# Standalone tool
./apitrace2gli input.trace > output.txt
./CG1SIM --trace output.txt  # Use existing GLInterceptor path
```

Benefits:
- Reuses all existing GLExec parsing logic
- Easier debugging (inspect converted text)
- Gradual migration (can improve converter incrementally)

