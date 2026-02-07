#ifndef D3D_TRACE
#define D3D_TRACE

#include "GPUType.h"

class D3DTrace
{
public:
    virtual bool next() = 0;
    virtual bool getFrameEnded() = 0;
    virtual bool isPreloadCall() = 0;
    virtual U32  getCurrentEventID() = 0;
};

D3DTrace* createD3D9Trace(char *file);

#endif
