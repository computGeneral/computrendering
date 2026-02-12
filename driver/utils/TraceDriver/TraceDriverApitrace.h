/**************************************************************************
 * TraceDriverApitrace.h
 * 
 * Trace driver for apitrace binary format (.trace files).
 * Converts apitrace call events to MetaStream transactions.
 */

#ifndef TRACEDRIVERAPITRACE_H
#define TRACEDRIVERAPITRACE_H

#include "GPUType.h"
#include "MetaStream.h"
#include "TraceDriverBase.h"
#include "GLResolver.h"
#include "ApitraceParser.h"
#include "HAL.h"
#include <map>

class TraceDriverApitrace : public cgoTraceDriverBase {
public:
    /**
     * Constructor.
     * @param traceFile Path to .trace file
     * @param driver HAL driver instance
     * @param startFrame Frame to start simulation from
     */
    TraceDriverApitrace(const char* traceFile, HAL* driver, U32 startFrame = 0);
    ~TraceDriverApitrace();
    
    int startTrace() override;
    arch::cgoMetaStream* nxtMetaStream() override;
    U32 getTracePosition() override { return currentFrame_; }

private:
    // Map apitrace function name to CG1's APICall enum
    APICall mapFunctionName(const std::string& name);
    
    // Convert apitrace Value to CG1 parameter representation
    void convertArgument(const apitrace::Value& val, std::vector<unsigned char>& outData);
    
    // Handle buffer/blob values
    unsigned int storeBlob(const apitrace::Value& val);
    
    apitrace::ApitraceParser parser_;
    HAL* driver_;
    unsigned int startFrame_;
    unsigned int currentFrame_;
    bool initialized_;
    
    // Buffer manager for blob data
    std::map<unsigned int, std::vector<unsigned char>> blobCache_;
    unsigned int nextBlobId_;
};

#endif // TRACEDRIVERAPITRACE_H
