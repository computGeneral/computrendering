#ifndef _TRACE_DRIVER_INTERFACE_
#define _TRACE_DRIVER_INTERFACE_

#include "MetaStreamTrace.h"
#include "HAL.h"
enum TraceTyp
{
    TraceTypD3d, //  Flag that stores if the input trace is a D3D9 API trace (PIX).  */
    TraceTypOgl, //  Flag that stores if the input trace is a OGL API trace.  */
    TraceTypCgp, //  Flag that stores if the input trace is an MetaStream trace.  */
    TraceTypVlk,
};

/**
 *  Trace Driver class. Generates MetaStreams to the GPU from API traces.
 */
class cgoTraceDriverBase
{
private:
protected:
    TraceTyp traceTyp;
    U32 maxFrames_;          //!< Maximum frames to simulate (0 = unlimited)
    bool frameLimitReached_; //!< Set when frame limit has been reached
public:
    cgoTraceDriverBase() : maxFrames_(0), frameLimitReached_(false) {}
    virtual ~cgoTraceDriverBase() {}
    TraceTyp getTraceTyp() { return traceTyp; }
    void setMaxFrames(U32 maxFrames) { maxFrames_ = maxFrames; }
    U32 getMaxFrames() const { return maxFrames_; }
    bool isFrameLimitReached() const { return frameLimitReached_; }
    /**
     * Starts the trace driver.
     * Verifies if a TraceDriver object is correctly created and available for use
     * @return  0 if all is ok.
     *         -1 opengl functions cannot be loaded
     *         -2 if ProfilingFile could not be opened
     *         -3 if bufferFile could not be opened
     */
    virtual int startTrace() = 0;

    /**
     *  Generates the next MetaStream from the API trace file.
     *  @return A pointer to the new MetaStream, NULL if
     *  there are no more MetaStreams.
     */
    virtual arch::cgoMetaStream* nxtMetaStream() = 0;
    
    /**
     *
     *  Obtain the current position inside the trace.
     *  @return The current position inside the trace.
     *
     */
     
    virtual U32 getTracePosition() = 0;
};

extern "C" cgoTraceDriverBase *createTraceDriver(char *ProfilingFile, HAL* driver, U32 startFrame);
extern "C" void destroyTraceDriver(cgoTraceDriverBase *TraceDriver);

#endif
