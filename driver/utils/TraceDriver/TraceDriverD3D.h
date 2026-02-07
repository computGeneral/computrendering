#ifndef _D3DTRACEDRIVER_
#define _D3DTRACEDRIVER_

class D3DTrace;

namespace cg1gpu {
    class cgoMetaStream;
}

/**
 *  Generates MetaStreams from an D3D API trace file.
 */
class TraceDriverD3D : public cgoTraceDriverBase
{
public:
    TraceDriverD3D(char *ProfilingFile, U32 start_frame);
    ~TraceDriverD3D();
    int startTrace();
    cg1gpu::cgoMetaStream* nxtMetaStream();
    U32 getTracePosition();

private:
    D3DTrace* trace; // Equivalent to GLExec
    U32       startFrame;
    U32       currentFrame;
};


#endif
