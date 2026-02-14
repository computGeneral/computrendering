/**************************************************************************
 * ApitraceCallDispatcher.h
 * 
 * Dispatches apitrace CallEvents to CG1 OGL entry point functions.
 * Extracts typed arguments from apitrace::Value objects and calls
 * the appropriate OGL_gl* functions directly.
 */

#ifndef APITRACECALLDISPATCHER_H
#define APITRACECALLDISPATCHER_H

#include "ApitraceParser.h"
#include "GLResolver.h"
#include "glAll.h"
#include <map>
#include <functional>

namespace apitrace {

// ---- Helpers: extract GL types from apitrace::Value ----

inline GLuint   asUInt(const Value& v)   { return (GLuint)v.uintVal; }
inline GLint    asInt(const Value& v)    { return (v.type == VALUE_SINT) ? (GLint)v.intVal : (GLint)v.uintVal; }
inline GLenum   asEnum(const Value& v)   { return (GLenum)v.uintVal; }
inline GLfloat  asFloat(const Value& v)  { return (v.type == VALUE_FLOAT) ? v.floatVal : (GLfloat)v.uintVal; }
inline GLdouble asDouble(const Value& v) { return (v.type == VALUE_DOUBLE) ? v.doubleVal : (GLdouble)asFloat(v); }
inline GLboolean asBool(const Value& v)  { return (v.type == VALUE_TRUE) ? GL_TRUE : GL_FALSE; }
inline GLclampf asClampf(const Value& v) { return (GLclampf)asFloat(v); }
inline GLclampd asClampd(const Value& v) { return (GLclampd)asDouble(v); }

inline std::string asString(const Value& v) { return (v.type == VALUE_STRING) ? v.strVal : ""; }

inline const GLfloat* asFloatPtr(const Value& v) {
    if (v.type == VALUE_BLOB) return (const GLfloat*)v.blobVal.data();
    if (v.type == VALUE_ARRAY && !v.arrayVal.empty()) {
        // Caller must ensure lifetime — using thread-local static buffer
        static thread_local GLfloat buf[64];
        for (size_t i = 0; i < v.arrayVal.size() && i < 64; ++i)
            buf[i] = asFloat(v.arrayVal[i]);
        return buf;
    }
    return nullptr;
}

inline const GLdouble* asDoublePtr(const Value& v) {
    if (v.type == VALUE_BLOB) return (const GLdouble*)v.blobVal.data();
    if (v.type == VALUE_ARRAY && !v.arrayVal.empty()) {
        static thread_local GLdouble buf[64];
        for (size_t i = 0; i < v.arrayVal.size() && i < 64; ++i)
            buf[i] = asDouble(v.arrayVal[i]);
        return buf;
    }
    return nullptr;
}

inline const GLvoid* asVoidPtr(const Value& v) {
    if (v.type == VALUE_BLOB) return (const GLvoid*)v.blobVal.data();
    if (v.type == VALUE_OPAQUE) return (const GLvoid*)(uintptr_t)v.uintVal;
    if (v.type == VALUE_NULL) return nullptr;
    return nullptr;
}

inline const GLubyte* asUBytePtr(const Value& v) {
    if (v.type == VALUE_BLOB) return (const GLubyte*)v.blobVal.data();
    if (v.type == VALUE_STRING) return (const GLubyte*)v.strVal.c_str();
    return nullptr;
}

// ---- Convenience: get arg by index with default ----
inline const Value& arg(const CallEvent& evt, uint32_t idx) {
    static const Value nullVal;
    auto it = evt.arguments.find(idx);
    return (it != evt.arguments.end()) ? it->second : nullVal;
}

/**
 * Dispatch an apitrace CallEvent to the corresponding OGL_gl* entry point.
 * Returns true if the call was dispatched, false if unmapped/skipped.
 */
bool dispatchCall(const CallEvent& evt);

}  // namespace apitrace

#endif // APITRACECALLDISPATCHER_H
