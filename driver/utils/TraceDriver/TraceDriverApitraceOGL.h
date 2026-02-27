/**************************************************************************
 * TraceDriverApitraceOGL.h
 * 
 * Trace driver for apitrace binary format (.trace files).
 * Converts apitrace call events to MetaStream transactions.
 */

#ifndef TRACEDRIVERAPITRACEOGL_H
#define TRACEDRIVERAPITRACEOGL_H

#include "GPUType.h"
#include "MetaStream.h"
#include "TraceDriverBase.h"
#include "ApitraceParser.h"
#include "HAL.h"

class TraceDriverApitraceOGL : public cgoTraceDriverBase {
public:
    /**
     * Constructor.
     * @param traceFile Path to .trace file
     * @param driver HAL driver instance
     * @param startFrame Frame to start simulation from
     */
    TraceDriverApitraceOGL(const char* traceFile, HAL* driver, U32 startFrame = 0, U32 maxFrames = 0);
    ~TraceDriverApitraceOGL();
    
    int startTrace() override;
    arch::cgoMetaStream* nxtMetaStream() override;
    U32 getTracePosition() override { return currentFrame_; }

private:
    apitrace::ApitraceParser parser_;
    HAL* driver_;
    U32 startFrame_;
    U32 currentFrame_;
    bool initialized_;
};

#endif // TRACEDRIVERAPITRACEOGL_H
