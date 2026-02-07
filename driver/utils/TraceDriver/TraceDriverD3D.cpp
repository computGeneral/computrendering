/**************************************************************************
 *
 */

#include "TraceDriverBase.h"
#include "TraceDriverD3D.h"
#include "D3DTrace.h"
#include "D3D9.h"
#include "HAL.h"
using namespace cg1gpu;

//  TraceDriver constructor.  
TraceDriverD3D::TraceDriverD3D(char *trace_file, U32 _startFrame)
{
    traceTyp = TraceTypD3d;
    trace = createD3D9Trace(trace_file);
    D3D9::initialize(trace);
    startFrame = _startFrame;
    currentFrame = 0;
}

TraceDriverD3D::~TraceDriverD3D()
{
    D3D9::finalize();
    delete trace;
}

cgoMetaStream* TraceDriverD3D::nxtMetaStream()
{
    cgoMetaStream* metaStream = 0;
    bool end = false;    // Execute trace calls until HAL receives some transaction or end of trace reached.
    while ((metaStream == 0) & !end)    //  Execute D3D trace calls until an MetaStream is generated or the trace ends.
    {
        metaStream = HAL::getHAL()->nextMetaStream(); //  Check if there is a transaction pending in the GPU Driver buffer.
        if (metaStream == NULL)
        {
            //  Execute the next D3D trace call.
            if(trace->next())
            {
                //  Check if the current frame finished.
                if(trace->getFrameEnded())
                {
                    HAL::getHAL()->printMemoryUsage();
                    
                    if (currentFrame < startFrame)
                    {
                        cout << "Skipped frame " << currentFrame << endl;
                    }                       
                    else
                    {
                        cout << "Rendered frame " << currentFrame << endl;
                        
                        //  Clear cycles spent on swap flush and DisplayController.
                        HAL::getHAL()->signalEvent(GPU_END_OF_FRAME_EVENT, "");
                    }
                    
                    // Disable preload memory before first renderable frame
                    if(currentFrame == (startFrame - 1))
                    {
                        HAL::getHAL()->setPreloadMemory(false);
                        cout << "D3DTRACEDRIVER: Disabling preload memory on frame " << (int)currentFrame << endl;
                        
                        //  Clear end of frame event register.
                        HAL::getHAL()->signalEvent(GPU_END_OF_FRAME_EVENT, "");
                        
                        //  Signal start of simulation.
                        HAL::getHAL()->signalEvent(GPU_UNNAMED_EVENT, "End of preload phase.  Starting simulation.");
                    }
                    
                    currentFrame ++;
                }
            }
            else
            {
                //  Reached the end of the D3D trace.
                end = true;
            }
        }
    }

    return metaStream;
}

int TraceDriverD3D::startTrace() {
    //  Set preload memory mode
    if (startFrame > 0) {
        HAL::getHAL()->setPreloadMemory(true);
        cout << "D3DTRACEDRIVER: Enabling preload memory" << endl;
    }
    currentFrame = 0;
    return 0;
}

//  Return the current position inside the trace file.
U32 TraceDriverD3D::getTracePosition()
{
    return trace->getCurrentEventID();
}


