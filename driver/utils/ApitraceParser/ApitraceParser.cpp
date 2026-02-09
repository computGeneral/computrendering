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
    : stream_(nullptr), version_(0), semanticVersion_(0), nextCallNo_(0) {}

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
    return readBytes(&str[0], len);
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
            // For now, just read the value part (skip enum signature caching)
            return readValue(val);  // recursive read of the actual enum value
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
            std::cerr << "Unsupported value type: 0x" << std::hex << (int)typeTag << std::endl;
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
            
            default:
                // Skip unsupported details
                break;
        }
    }
    return true;
}

bool ApitraceParser::readEvent(CallEvent& event) {
    uint8_t eventType;
    if (!readByte(eventType)) return false;
    
    if (eventType == EVENT_CALL_ENTER) {
        event.callNo = nextCallNo_++;
        
        // Read thread number (version >= 4)
        if (version_ >= 4) {
            uint64_t threadNo;
            if (!readVarUInt(threadNo)) return false;
            event.threadNo = threadNo;
        }
        
        // Read call signature
        uint64_t sigId;
        if (!readVarUInt(sigId)) return false;
        if (!readCallSignature(sigId, event.signature)) return false;
        
        // Read call details
        return readCallDetails(event);
    }
    else if (eventType == EVENT_CALL_LEAVE) {
        // For LEAVE events, just read call_no and any details
        uint64_t callNo;
        if (!readVarUInt(callNo)) return false;
        event.callNo = callNo;
        return readCallDetails(event);
    }
    
    return false;
}

