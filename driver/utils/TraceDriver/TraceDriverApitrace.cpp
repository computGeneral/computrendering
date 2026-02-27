/**************************************************************************
 * TraceDriverApitrace.cpp
 * 
 * Implementation of apitrace trace driver.
 */

#include "TraceDriverApitrace.h"
#include "ApitraceCallDispatcher.h"
#include "OGL.h"
#include "OGLEntryPoints.h"
#include "Profiler.h"

using namespace arch;
#include "support.h"
#include <iostream>

TraceDriverApitrace::TraceDriverApitrace(const char* traceFile, HAL* driver, U32 startFrame, U32 maxFrames)
    : driver_(driver), startFrame_(startFrame), currentFrame_(0), 
      initialized_(false)
{
    maxFrames_ = maxFrames;
    if (!parser_.open(traceFile)) {
        std::cerr << "ERROR: Failed to open apitrace file: " << traceFile << std::endl;
        CG_ASSERT("Failed to open apitrace file");
    }
    
    // Skip to start frame if needed
    if (startFrame_ > 0) {
        unsigned int frame = 0;
        apitrace::CallEvent evt;
        while (frame < startFrame_ && parser_.readEvent(evt)) {
            if (evt.signature.functionName.find("SwapBuffers") != std::string::npos ||
                evt.signature.functionName == "glXSwapBuffers" ||
                evt.signature.functionName == "eglSwapBuffers") {
                frame++;
            }
        }
    }
    
    // Initialize the OGL/GAL subsystem (required for OGL_gl* entry points)
    ogl::initOGL(driver, startFrame);
    
    initialized_ = true;
}

TraceDriverApitrace::~TraceDriverApitrace() {
    parser_.close();
}

int TraceDriverApitrace::startTrace() {
    return initialized_ ? 0 : -1;
}

arch::cgoMetaStream* TraceDriverApitrace::nxtMetaStream() {
    if (!initialized_ || parser_.eof()) 
        return nullptr;
    
    arch::cgoMetaStream* agpt = nullptr;

    // If frame limit was reached, only drain remaining MetaStreams from HAL
    if (frameLimitReached_) {
        agpt = driver_->nextMetaStream();
        return agpt;  // nullptr means simulation loop will stop
    }

    // Try to drain a MetaStream from HAL buffer first
    while (!(agpt = driver_->nextMetaStream())) {
        apitrace::CallEvent evt;
        if (!parser_.readEvent(evt)) 
            return nullptr;  // End of trace
        
        const std::string& fn = evt.signature.functionName;
        
        // Skip CALL_LEAVE events (we only process CALL_ENTER)
        if (fn.empty()) continue;

        // Dispatch the GL call through OGL entry points → GAL → HAL → MetaStream
        TRACING_ENTER_REGION((char*)fn.c_str(), (char*)"OpenGL", (char*)"");
        apitrace::dispatchCall(evt);
        TRACING_EXIT_REGION();
        
        // Handle SwapBuffers (frame boundary)
        if (fn.find("SwapBuffers") != std::string::npos) {
            if (currentFrame_ >= startFrame_) {
                ogl::swapBuffers();
                driver_->signalEvent(arch::GPU_END_OF_FRAME_EVENT, "");
            }
            currentFrame_++;
            driver_->printMemoryUsage();
            
            // Check if we've reached the frame limit
            if (maxFrames_ > 0 && currentFrame_ >= (startFrame_ + maxFrames_)) {
                frameLimitReached_ = true;
                agpt = driver_->nextMetaStream();
                return agpt;
            }
        }
    }

    return agpt;
}

