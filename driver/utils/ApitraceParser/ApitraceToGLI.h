/**************************************************************************
 * ApitraceToGLI.h
 * 
 * Converts apitrace binary format to GLI text format for reuse of
 * existing TraceDriverOGL/GLExec infrastructure.
 */

#ifndef APITRACETOGLI_H
#define APITRACETOGLI_H

#include "ApitraceParser.h"
#include <string>
#include <sstream>

namespace apitrace {

class ApitraceToGLI {
public:
    /**
     * Convert an apitrace CallEvent to GLI text format.
     * Example: glClear(GL_COLOR_BUFFER_BIT) with args as apitrace Values
     * Output:  "glClear(16384)"
     */
    static std::string convertCall(const CallEvent& evt);
    
private:
    static std::string valueToString(const Value& val);
    static std::string formatArguments(const CallEvent& evt);
};

}  // namespace apitrace

#endif
