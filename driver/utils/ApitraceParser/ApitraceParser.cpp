/**************************************************************************
 * ApitraceParser.cpp
 *  
 * Apitrace binary format parser implementation.
 */

#include "ApitraceParser.h"
#include "support.h"
#include <snappy.h>
#include <cstring>
#include <iostream>

using namespace apitrace;


// ============ SnappyStream Implementation ============

SnappyStream::SnappyStream() : bufferPos_(0) {}

SnappyStream::~SnappyStream() {
    close();
}

bool SnappyStream::open(const char* filename) {
    file_.open(filename, std::ios::binary);
    if (!file_.is_open()) return false;
    
    // Read and verify header: 'a' 't'
    char header[2];
    file_.read(header, 2);
    if (!file_ || header[0] != 'a' || header[1] != 't') {
        std::cerr << "Invalid apitrace file header" << std::endl;
        return false;
    }
    
    decompressed_.reserve(1024 * 1024); // 1MB buffer
    bufferPos_ = 0;
    return true;
}

void SnappyStream::close() {
    if (file_.is_open()) file_.close();
    decompressed_.clear();
}

bool SnappyStream::eof() const {
    return file_.eof() && bufferPos_ >= decompressed_.size();
}

bool SnappyStream::fillBuffer() {
    if (file_.eof()) return false;
    
    // Read chunk: compressed_length (uint32 LE) + compressed_data
    uint32_t compressedLen;
    file_.read(reinterpret_cast<char*>(&compressedLen), 4);
    if (!file_ || compressedLen == 0 || compressedLen > 100*1024*1024) 
        return false;
    
    std::vector<uint8_t> compressed(compressedLen);
    file_.read(reinterpret_cast<char*>(compressed.data()), compressedLen);
    if (!file_) return false;
    
    // Decompress with Snappy
    size_t uncompressedLen;
    if (!snappy::GetUncompressedLength(reinterpret_cast<const char*>(compressed.data()), 
                                       compressedLen, &uncompressedLen)) 
        return false;
    
    decompressed_.resize(uncompressedLen);
    if (!snappy::RawUncompress(reinterpret_cast<const char*>(compressed.data()), compressedLen,
                                reinterpret_cast<char*>(decompressed_.data())))
        return false;
    
    bufferPos_ = 0;
    return true;
}

bool SnappyStream::read(void* buffer, size_t count) {
    uint8_t* dest = static_cast<uint8_t*>(buffer);
    size_t remaining = count;
    
    while (remaining > 0) {
        // Refill buffer if empty
        if (bufferPos_ >= decompressed_.size()) {
            if (!fillBuffer()) return false;
        }
        
        size_t available = decompressed_.size() - bufferPos_;
        size_t toCopy = (remaining < available) ? remaining : available;
        
        std::memcpy(dest, decompressed_.data() + bufferPos_, toCopy);
        dest += toCopy;
        bufferPos_ += toCopy;
        remaining -= toCopy;
    }
    
    return true;
}

// ============ ApitraceParser Implementation ============

ApitraceParser::ApitraceParser() 
    : stream_(nullptr), version_(0), semanticVersion_(0), nextCallNo_(0),
      apiTypeDetected_(false), hasFirstEvent_(false) {}

ApitraceParser::~ApitraceParser() {
    close();
}

bool ApitraceParser::open(const char* traceFile) {
    stream_ = new SnappyStream();
    if (!stream_->open(traceFile)) {
        delete stream_;
        stream_ = nullptr;
        return false;
    }
    
    // Read version
    if (!readVarUInt(reinterpret_cast<uint64_t&>(version_)))
        return false;
    
    // Read semantic version (if version >= 6)
    if (version_ >= 6) {
        if (!readVarUInt(reinterpret_cast<uint64_t&>(semanticVersion_)))
            return false;
        
        // Read properties
        while (true) {
            std::string name, value;
            if (!readString(name)) return false;
            if (name.empty()) break;  // terminator
            if (!readString(value)) return false;
            properties_[name] = value;
        }
    }
    
    nextCallNo_ = 0;
    return true;
}

void ApitraceParser::close() {
    if (stream_) {
        stream_->close();
        delete stream_;
        stream_ = nullptr;
    }
}

bool ApitraceParser::eof() const {
    return stream_ ? stream_->eof() : true;
}

bool ApitraceParser::readVarUInt(uint64_t& value) {
    value = 0;
    int shift = 0;
    uint8_t b;
    
    do {
        if (!readByte(b)) return false;
        value |= static_cast<uint64_t>(b & 0x7F) << shift;
        shift += 7;
    } while (b & 0x80);
    
    return true;
}

bool ApitraceParser::readByte(uint8_t& value) {
    return stream_->read(&value, 1);
}

bool ApitraceParser::readBytes(void* buffer, size_t count) {
    return stream_->read(buffer, count);
}

bool ApitraceParser::readFloat(float& value) {
    return readBytes(&value, sizeof(float));
}

bool ApitraceParser::readDouble(double& value) {
    return readBytes(&value, sizeof(double));
}

bool ApitraceParser::readString(std::string& str) {
    uint64_t len;
    if (!readVarUInt(len)) return false;
    if (len == 0) {
        str.clear();
        return true;
    }
    if (len > 1024*1024) return false; // sanity check
    
    str.resize(len);
    if (!readBytes(&str[0], len)) return false;
    // Strip trailing null if present (some formats might include it)
    if (!str.empty() && str.back() == '\0') {
        str.pop_back();
    }
    return true;
}

bool ApitraceParser::readValue(Value& val) {
    uint8_t typeTag;
    if (!readByte(typeTag)) return false;
    
    val.type = static_cast<ValueType>(typeTag);
    
    switch (val.type) {
        case VALUE_NULL:
        case VALUE_FALSE:
        case VALUE_TRUE:
            return true;
        
        case VALUE_SINT: {
            uint64_t u;
            if (!readVarUInt(u)) return false;
            val.intVal = -static_cast<int64_t>(u);
            return true;
        }
        
        case VALUE_UINT:
            return readVarUInt(val.uintVal);
        
        case VALUE_FLOAT:
            return readFloat(val.floatVal);
        
        case VALUE_DOUBLE:
            return readDouble(val.doubleVal);
        
        case VALUE_STRING:
        case VALUE_WSTRING:
            return readString(val.strVal);
        
        case VALUE_BLOB: {
            uint64_t len;
            if (!readVarUInt(len)) return false;
            if (len > 100*1024*1024) return false; // 100MB limit
            val.blobVal.resize(len);
            if (len > 0 && !readBytes(val.blobVal.data(), len)) return false;
            return true;
        }
        
        case VALUE_ENUM: {
            uint64_t sigId;
            if (!readVarUInt(sigId)) return false;
            EnumSignature sig;
            if (!readEnumSignature(sigId, sig)) return false;
            // Enum signature parsing handles caching.
            // Now read the enum value (SINT)
            return readValue(val);
        }

        case VALUE_BITMASK: {
            uint64_t sigId;
            if (!readVarUInt(sigId)) return false;
            BitmaskSignature sig;
            if (!readBitmaskSignature(sigId, sig)) return false;
            // Bitmask value is UINT
            return readVarUInt(val.uintVal);
        }

        case VALUE_STRUCT: {
             uint64_t sigId;
             if (!readVarUInt(sigId)) return false;
             StructSignature sig;
             if (!readStructSignature(sigId, sig)) return false;
             
             val.arrayVal.resize(sig.memberNames.size());
             for(size_t i=0; i<sig.memberNames.size(); ++i) {
                  if (!readValue(val.arrayVal[i])) return false;
             }
             return true;
        }

        case VALUE_REPR: {
            if (!readValue(val)) return false; // First sub-value
            Value repr;
            if (!readValue(repr)) return false; // Second sub-value
            // If first is text (human-readable repr) and second is blob (original),
            // prefer the blob.  Some apitrace versions write repr first.
            if (repr.type == VALUE_BLOB && !repr.blobVal.empty()) {
                val = std::move(repr);
            }
            return true;
        }
        
        case VALUE_ARRAY: {
            uint64_t count;
            if (!readVarUInt(count)) return false;
            val.arrayVal.resize(count);
            for (uint64_t i = 0; i < count; ++i) {
                if (!readValue(val.arrayVal[i])) return false;
            }
            return true;
        }
        
        case VALUE_OPAQUE:
            return readVarUInt(val.ptrVal);
        
        default:
            // Unsupported type — skip for now
            std::cerr << "Unsupported value type: 0x" << std::hex << (int)typeTag 
                      << " at callNo=" << std::dec << nextCallNo_ << std::endl;
            return false;
    }
}

bool ApitraceParser::readCallSignature(uint32_t sigId, CallSignature& sig) {
    auto it = signatureCache_.find(sigId);
    if (it != signatureCache_.end()) {
        sig = it->second;
        return true;
    }
    
    // First occurrence: read full signature
    sig.id = sigId;
    if (!readString(sig.functionName)) return false;
    
    uint64_t argCount;
    if (!readVarUInt(argCount)) return false;
    
    sig.argNames.resize(argCount);
    for (uint64_t i = 0; i < argCount; ++i) {
        if (!readString(sig.argNames[i])) return false;
    }
    
    signatureCache_[sigId] = sig;
    return true;
}


bool ApitraceParser::readEnumSignature(uint32_t sigId, EnumSignature& sig) {
    if (enumSignatureCache_.count(sigId)) {
        sig = enumSignatureCache_[sigId];
        return true;
    }
    
    sig.id = sigId;
    // In V6, Enums don't have a name in the signature
    // if (!readString(sig.name)) return false; 
    
    uint64_t num_values;
    if (!readVarUInt(num_values)) return false;
    
    if (num_values > 10000) {
        return false;
    }

    sig.values.resize(num_values);
    for (uint64_t i = 0; i < num_values; ++i) {
        if (!readString(sig.values[i].first)) return false;
        // Enum value is read as SINT (TypeTag + VarUInt)
        Value v;
        if (!readValue(v)) return false;
        sig.values[i].second = v.intVal;
    }
    
    enumSignatureCache_[sigId] = sig;
    return true;
}

bool ApitraceParser::readBitmaskSignature(uint32_t sigId, BitmaskSignature& sig) {
    if (bitmaskSignatureCache_.count(sigId)) {
        sig = bitmaskSignatureCache_[sigId];
        return true;
    }
    
    sig.id = sigId;
    uint64_t count;
    if (!readVarUInt(count)) return false;
    
    if (count > 10000) {
         return false;
    }

    sig.flags.resize(count);
    for (uint64_t i = 0; i < count; ++i) {
        if (!readString(sig.flags[i].first)) return false;
        if (!readVarUInt(sig.flags[i].second)) return false;
    }
    
    bitmaskSignatureCache_[sigId] = sig;
    return true;
}

bool ApitraceParser::readStructSignature(uint32_t sigId, StructSignature& sig) {
    if (structSignatureCache_.count(sigId)) {
        sig = structSignatureCache_[sigId];
        return true;
    }
    
    sig.id = sigId;
    if (!readString(sig.name)) return false;
    uint64_t count;
    if (!readVarUInt(count)) return false;
    
    sig.memberNames.resize(count);
    for (uint64_t i = 0; i < count; ++i) {
        if (!readString(sig.memberNames[i])) return false;
    }
    
    structSignatureCache_[sigId] = sig;
    return true;
}

bool ApitraceParser::readCallDetails(CallEvent& event) {
    while (true) {
        uint8_t detailType;
        if (!readByte(detailType)) return false;
        
        if (detailType == DETAIL_END) break;
        
        switch (detailType) {
            case DETAIL_ARG: {
                uint64_t argNo;
                if (!readVarUInt(argNo)) return false;
                if (!readValue(event.arguments[argNo])) return false;
                break;
            }
            
            case DETAIL_RETURN:
                if (!readValue(event.returnValue)) return false;
                event.hasReturn = true;
                break;
            
            case DETAIL_THREAD: {
                uint64_t threadNo;
                if (!readVarUInt(threadNo)) return false;
                event.threadNo = threadNo;
                break;
            }

            case DETAIL_BACKTRACE: {
                 Value v;
                 if (!readValue(v)) return false;
                 break;
            }

            case DETAIL_FLAG: {
                 uint64_t flags;
                 if (!readVarUInt(flags)) return false;
                 break;
            }
            
            default:
                // Skip unsupported details
                std::cerr << "Unsupported detail type: 0x" << std::hex << (int)detailType << " at callNo=" << std::dec << nextCallNo_ << std::endl;
                return false; // Can't recover
        }
    }
    return true;
}

bool ApitraceParser::readEvent(CallEvent& event) {
    // If we have a cached first event from detectApiType(), return it first
    if (hasFirstEvent_) {
        event = firstEvent_;
        hasFirstEvent_ = false;
        return true;
    }
    
    // Read events, merging ENTER+LEAVE pairs automatically.
    // ENTER events are pushed to a stack. When a matching LEAVE
    // arrives, we merge its args/return into the ENTER and return.
    while (true) {
        uint8_t eventType;
        if (!readByte(eventType)) {
            // EOF — flush any remaining ENTER events from the stack
            if (!pendingEnterStack_.empty()) {
                event = pendingEnterStack_.top();
                pendingEnterStack_.pop();
                return true;
            }
            return false;
        }
        
        if (eventType == EVENT_CALL_ENTER) {
            CallEvent enterEvt;
            enterEvt.callNo = nextCallNo_++;
            
            // Read thread number (version >= 4)
            if (version_ >= 4) {
                uint64_t threadNo;
                if (!readVarUInt(threadNo)) return false;
                enterEvt.threadNo = threadNo;
            }
            
            // Read call signature
            uint64_t sigId;
            if (!readVarUInt(sigId)) return false;
            if (!readCallSignature(sigId, enterEvt.signature)) return false;
            
            // Read call details (input args)
            if (!readCallDetails(enterEvt)) return false;
            
            // Push to stack and continue reading for matching LEAVE
            pendingEnterStack_.push(enterEvt);
        }
        else if (eventType == EVENT_CALL_LEAVE) {
            // Read LEAVE call number and details
            uint64_t callNo;
            if (!readVarUInt(callNo)) return false;
            
            CallEvent leaveEvt;
            if (!readCallDetails(leaveEvt)) return false;
            
            if (!pendingEnterStack_.empty()) {
                // Merge LEAVE data into the matching ENTER event
                event = pendingEnterStack_.top();
                pendingEnterStack_.pop();
                
                // Merge arguments: LEAVE args override/supplement ENTER args
                for (auto& kv : leaveEvt.arguments) {
                    event.arguments[kv.first] = kv.second;
                }
                
                // Merge return value
                if (leaveEvt.hasReturn) {
                    event.returnValue = leaveEvt.returnValue;
                    event.hasReturn = true;
                }
                
                return true;
            }
            // No matching ENTER — skip orphan LEAVE
        }
        else {
            // Unknown event type — try to continue
            return false;
        }
    }
}

std::string ApitraceParser::detectApiType() {
    if (apiTypeDetected_) return detectedApiType_;
    apiTypeDetected_ = true;
    
    // Try to determine API from trace properties first
    auto it = properties_.find("API");
    if (it != properties_.end()) {
        std::string api = it->second;
        // Normalize specific API strings
        if (api.find("GL") != std::string::npos || api.find("gl") != std::string::npos) {
            detectedApiType_ = "gl";
            return detectedApiType_;
        }
        else if (api.find("D3D9") != std::string::npos || api.find("d3d9") != std::string::npos ||
                 api == "Direct3D 9") {
            detectedApiType_ = "d3d9";
            return detectedApiType_;
        }
        else if (api.find("D3D11") != std::string::npos || api.find("d3d11") != std::string::npos ||
                 api == "Direct3D 11") {
            detectedApiType_ = "d3d11";
            return detectedApiType_;
        }
        else if (api.find("D3D12") != std::string::npos || api.find("d3d12") != std::string::npos) {
            detectedApiType_ = "d3d12";
            return detectedApiType_;
        }
        // "DirectX" or other generic D3D strings — need to check first call
        // to determine the exact D3D version. Fall through below.
    }
    
    // Peek at the first call event to determine API type from the function names
    CallEvent evt;
    // Read events until we find a CALL_ENTER with a non-empty function name
    while (readEvent(evt)) {
        const std::string& fn = evt.signature.functionName;
        if (fn.empty()) continue;
        
        // Cache this event to be returned by next readEvent() call
        firstEvent_ = evt;
        hasFirstEvent_ = true;
        
        // Check for D3D9 patterns
        if (fn.find("IDirect3D") != std::string::npos ||
            fn.find("Direct3DCreate9") != std::string::npos ||
            fn.find("DirectDraw") != std::string::npos ||
            fn.find("D3DPERF_") != std::string::npos) {
            detectedApiType_ = "d3d9";
        }
        // Check for D3D11 patterns
        else if (fn.find("ID3D11") != std::string::npos ||
                 fn.find("IDXGIFactory") != std::string::npos ||
                 fn.find("IDXGISwapChain") != std::string::npos ||
                 fn.find("D3D11CreateDevice") != std::string::npos ||
                 fn.find("CreateDXGIFactory") != std::string::npos) {
            detectedApiType_ = "d3d11";
        }
        // Check for D3D10/12 patterns
        else if (fn.find("ID3D10") != std::string::npos ||
                 fn.find("D3D10CreateDevice") != std::string::npos) {
            detectedApiType_ = "d3d10";
        }
        else if (fn.find("ID3D12") != std::string::npos ||
                 fn.find("D3D12CreateDevice") != std::string::npos) {
            detectedApiType_ = "d3d12";
        }
        // Check for OGL patterns
        else if (fn.find("gl") == 0 || fn.find("wgl") == 0 ||
                 fn.find("glX") == 0 || fn.find("egl") == 0) {
            detectedApiType_ = "gl";
        }
        else {
            detectedApiType_ = "";
        }
        
        return detectedApiType_;
    }
    
    return detectedApiType_;
}
