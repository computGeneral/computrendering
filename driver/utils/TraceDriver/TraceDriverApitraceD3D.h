/**************************************************************************
 * TraceDriverApitraceD3D.h
 * 
 * Trace driver for apitrace binary format (.trace files) containing
 * D3D9 API calls.  Converts apitrace call events to MetaStream
 * transactions through the CG1 D3D9 GAL layer.
 */

#ifndef TRACEDRIVERAPITRACED3D_H
#define TRACEDRIVERAPITRACED3D_H

#include "GPUType.h"
#include "MetaStream.h"
#include "TraceDriverBase.h"
#include "ApitraceParser.h"
#include "D3DApitraceCallDispatcher.h"
#include "HAL.h"

class AIRoot9;

class TraceDriverApitraceD3D : public cgoTraceDriverBase {
public:
    /**
     * Constructor.
     * @param traceFile Path to .trace file containing D3D9 calls
     * @param driver HAL driver instance
     * @param startFrame Frame to start simulation from
     */
    TraceDriverApitraceD3D(const char* traceFile, HAL* driver, U32 startFrame = 0, U32 maxFrames = 0);
    ~TraceDriverApitraceD3D();
    
    int startTrace() override;
    arch::cgoMetaStream* nxtMetaStream() override;
    U32 getTracePosition() override { return currentFrame_; }

private:
    apitrace::ApitraceParser parser_;
    HAL* driver_;
    U32 startFrame_;
    U32 currentFrame_;
    bool initialized_;
    
    //! D3D9 dispatcher state (object tracker, root, device)
    apitrace::d3d9::D3D9DispatcherState dispState_;
};

#endif // TRACEDRIVERAPITRACED3D_H
