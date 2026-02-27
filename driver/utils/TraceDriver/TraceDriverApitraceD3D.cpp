/**************************************************************************
 * TraceDriverApitraceD3D.cpp
 * 
 * Implementation of apitrace D3D9 trace driver.
 * Converts apitrace D3D9 call events to MetaStream transactions
 * through the CG1 D3D9 GAL layer (AIRoot9 -> AIDirect3DImp9 ->
 * AIDeviceImp9 -> GAL -> HAL).
 */

#include "TraceDriverApitraceD3D.h"
#include "D3DApitraceCallDispatcher.h"
#include "D3D9.h"
#include "D3DInterface.h"
#include "Profiler.h"

using namespace arch;
#include "support.h"
#include <iostream>

TraceDriverApitraceD3D::TraceDriverApitraceD3D(const char* traceFile, HAL* driver, U32 startFrame, U32 maxFrames)
    : driver_(driver), startFrame_(startFrame), currentFrame_(0),
      initialized_(false)
{
    traceTyp = TraceTypD3d;
    maxFrames_ = maxFrames;
    
    if (!parser_.open(traceFile)) {
        std::cerr << "ERROR: Failed to open apitrace D3D trace: " << traceFile << std::endl;
        CG_ASSERT("Failed to open apitrace D3D trace file");
    }
    
    // Initialize D3D9 subsystem
    D3D9::initialize();
    
    // Set up the dispatcher state
    AIRoot9* root = D3DInterface::get_gal_root_9();
    apitrace::d3d9::initDispatcher(dispState_, root);
    
    // Skip to start frame if requested
    if (startFrame_ > 0) {
        U32 frame = 0;
        apitrace::CallEvent evt;
        while (frame < startFrame_ && parser_.readEvent(evt)) {
            const std::string& fn = evt.signature.functionName;
            if (fn.empty()) continue;
            
            // Dispatch the call (we need to build up state)
            apitrace::d3d9::dispatchCall(dispState_, evt);
            
            // Check frame boundary
            if (apitrace::d3d9::isFrameBoundary(fn)) {
                frame++;
            }
        }
    }
    
    initialized_ = true;
}

TraceDriverApitraceD3D::~TraceDriverApitraceD3D() {
    parser_.close();
    D3D9::finalize();
}

int TraceDriverApitraceD3D::startTrace() {
    if (!initialized_) return -1;
    
    // Set preload memory mode for start frame > 0
    if (startFrame_ > 0) {
        driver_->setPreloadMemory(true);
    }
    currentFrame_ = 0;
    return 0;
}

cgoMetaStream* TraceDriverApitraceD3D::nxtMetaStream() {
    if (!initialized_ || parser_.eof())
        return nullptr;
    
    cgoMetaStream* metaStream = nullptr;
    
    // If frame limit was reached, only drain remaining MetaStreams from HAL
    if (frameLimitReached_) {
        metaStream = driver_->nextMetaStream();
        return metaStream;
    }
    
    // Execute trace calls until HAL produces a MetaStream or trace ends
    while (!(metaStream = driver_->nextMetaStream())) {
        apitrace::CallEvent evt;
        if (!parser_.readEvent(evt)) {
            return nullptr;  // End of trace
        }
        
        const std::string& fn = evt.signature.functionName;
        
        // Skip empty / CALL_LEAVE events
        if (fn.empty()) continue;
        
        // Dispatch the D3D9 call through AI*Imp9 -> GAL -> HAL
        TRACING_ENTER_REGION((char*)fn.c_str(), (char*)"D3D9", (char*)"");
        bool ok = false;
        try {
            ok = apitrace::d3d9::dispatchCall(dispState_, evt);
        } catch (const std::exception& e) {
            std::cerr << "EXCEPTION in call #" << evt.callNo
                      << " " << fn << ": " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "EXCEPTION in call #" << evt.callNo
                      << " " << fn << std::endl;
        }
        TRACING_EXIT_REGION();
        
        // Handle frame boundary (Present)
        if (apitrace::d3d9::isFrameBoundary(fn)) {
            if (currentFrame_ >= startFrame_) {
                driver_->signalEvent(GPU_END_OF_FRAME_EVENT, "");
            }
            
            // Disable preload memory before first renderable frame
            if (startFrame_ > 0 && currentFrame_ == (startFrame_ - 1)) {
                driver_->setPreloadMemory(false);
                driver_->signalEvent(GPU_END_OF_FRAME_EVENT, "");
                driver_->signalEvent(GPU_UNNAMED_EVENT, "End of preload phase.  Starting simulation.");
            }
            
            currentFrame_++;
            driver_->printMemoryUsage();
            
            // Check if we've reached the frame limit
            if (maxFrames_ > 0 && currentFrame_ >= (startFrame_ + maxFrames_)) {
                frameLimitReached_ = true;
                metaStream = driver_->nextMetaStream();
                return metaStream;
            }
        }
    }
    
    return metaStream;
}
