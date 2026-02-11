/**************************************************************************
 * ApitraceParser.h
 * 
 * Parser for apitrace binary trace format (.trace files).
 * Implements Snappy decompression and apitrace binary format spec.
 */

#ifndef APITRACEPARSER_H
#define APITRACEPARSER_H

#include "GPUType.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstdint>

namespace apitrace {

class SnappyStream;

enum EventType {
    EVENT_CALL_ENTER = 0x00,
    EVENT_CALL_LEAVE = 0x01
};

enum CallDetailType {
    DETAIL_END          = 0x00,
    DETAIL_ARG          = 0x01,
    DETAIL_RETURN       = 0x02,
    DETAIL_THREAD       = 0x03,
    DETAIL_BACKTRACE    = 0x04,
    DETAIL_FLAG         = 0x05
};

enum ValueType {
    VALUE_NULL = 0x00, VALUE_FALSE = 0x01, VALUE_TRUE = 0x02,
    VALUE_SINT = 0x03, VALUE_UINT = 0x04, VALUE_FLOAT = 0x05,
    VALUE_DOUBLE = 0x06, VALUE_STRING = 0x07, VALUE_BLOB = 0x08,
    VALUE_ENUM = 0x09, VALUE_BITMASK = 0x0a, VALUE_ARRAY = 0x0b,
    VALUE_STRUCT = 0x0c, VALUE_OPAQUE = 0x0d, VALUE_REPR = 0x0e,
    VALUE_WSTRING = 0x0f
};

struct Value {
    ValueType type;
    union { bool boolVal; int64_t intVal; uint64_t uintVal; float floatVal; double doubleVal; uint64_t ptrVal; };
    std::string strVal;
    std::vector<uint8_t> blobVal;
    std::vector<Value> arrayVal;
    Value() : type(VALUE_NULL), uintVal(0) {}
    bool isNull() const { return type == VALUE_NULL; }
    bool isBlob() const { return type == VALUE_BLOB; }
};

struct CallSignature {
    uint32_t id;
    std::string functionName;
    std::vector<std::string> argNames;
};

struct EnumSignature {
    uint32_t id;
    std::string name; // Name of the enum type (e.g. "GLenum")
    std::vector<std::pair<std::string, int64_t>> values; // Name/Value pairs
};

struct BitmaskSignature {
    uint32_t id;
    std::vector<std::pair<std::string, uint64_t>> flags;
};

struct StructSignature {
    uint32_t id;
    std::string name;
    std::vector<std::string> memberNames;
};

struct CallEvent {
    uint32_t callNo, threadNo;
    CallSignature signature;
    std::map<uint32_t, Value> arguments;
    Value returnValue;
    bool hasReturn;
    CallEvent() : callNo(0), threadNo(0), hasReturn(false) {}
};

class ApitraceParser {
public:
    ApitraceParser();
    ~ApitraceParser();
    bool open(const char* traceFile);
    void close();
    bool readEvent(CallEvent& event);
    bool eof() const;
    uint32_t getVersion() const { return version_; }

private:
    bool readVarUInt(uint64_t& value);
    bool readByte(uint8_t& value);
    bool readBytes(void* buffer, size_t count);
    bool readString(std::string& str);
    bool readFloat(float& value);
    bool readDouble(double& value);
    bool readValue(Value& value);
    bool readCallSignature(uint32_t sigId, CallSignature& sig);
    bool readEnumSignature(uint32_t sigId, EnumSignature& sig);
    bool readBitmaskSignature(uint32_t sigId, BitmaskSignature& sig);
    bool readStructSignature(uint32_t sigId, StructSignature& sig);
    bool readCallDetails(CallEvent& event);
    
    SnappyStream* stream_;
    uint32_t version_;
    uint32_t semanticVersion_;
    std::map<std::string, std::string> properties_;
    std::map<uint32_t, CallSignature> signatureCache_;
    std::map<uint32_t, EnumSignature> enumSignatureCache_;
    std::map<uint32_t, BitmaskSignature> bitmaskSignatureCache_;
    std::map<uint32_t, StructSignature> structSignatureCache_;
    uint32_t nextCallNo_;
};

class SnappyStream {
public:
    SnappyStream();
    ~SnappyStream();
    bool open(const char* filename);
    void close();
    bool read(void* buffer, size_t count);
    bool eof() const;
private:
    bool fillBuffer();
    std::ifstream file_;
    std::vector<uint8_t> decompressed_;
    size_t bufferPos_;
};

}  // namespace apitrace

#endif
