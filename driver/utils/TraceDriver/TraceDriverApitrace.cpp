/**************************************************************************
 * TraceDriverApitrace.cpp
 * 
 * Implementation of apitrace trace driver.
 */

#include "TraceDriverApitrace.h"
#include "GLResolver.h"
#include "ApitraceCallDispatcher.h"
#include "OGL.h"
#include "OGLEntryPoints.h"

using namespace arch;
#include "support.h"
#include <iostream>
#include <cstring>
#include <set>

TraceDriverApitrace::TraceDriverApitrace(const char* traceFile, HAL* driver, unsigned int startFrame)
    : driver_(driver), startFrame_(startFrame), currentFrame_(0), 
      initialized_(false), nextBlobId_(1000)
{
    if (!parser_.open(traceFile)) {
        std::cerr << "ERROR: Failed to open apitrace file: " << traceFile << std::endl;
        CG_ASSERT("Failed to open apitrace file");
    }
    
    std::cout << "Apitrace trace opened (version " << parser_.getVersion() << ")" << std::endl;
    
    // Skip to start frame if needed
    if (startFrame_ > 0) {
        std::cout << "Skipping to frame " << startFrame_ << "..." << std::endl;
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

APICall TraceDriverApitrace::mapFunctionName(const std::string& name) {
    // Use GLResolver to dynamically map function name to APICall enum
    APICall apiCall = GLResolver::getFunctionID(name.c_str());
    
    if (apiCall == APICall_UNDECLARED) {
        // Log unmapped calls for debugging
        static std::set<std::string> warned;
        if (warned.find(name) == warned.end()) {
            std::cerr << "[TraceDriverApitrace] Unmapped call: " << name << std::endl;
            warned.insert(name);
        }
    }
    
    return apiCall;
}

unsigned int TraceDriverApitrace::storeBlob(const apitrace::Value& val) {
    if (!val.isBlob()) return 0;
    
    unsigned int blobId = nextBlobId_++;
    blobCache_[blobId] = val.blobVal;
    return blobId;
}

arch::cgoMetaStream* TraceDriverApitrace::nxtMetaStream() {
    if (!initialized_ || parser_.eof()) 
        return nullptr;
    
    arch::cgoMetaStream* agpt = nullptr;

    // Try to drain a MetaStream from HAL buffer first
    while (!(agpt = driver_->nextMetaStream())) {
        apitrace::CallEvent evt;
        if (!parser_.readEvent(evt)) 
            return nullptr;  // End of trace
        
        const std::string& fn = evt.signature.functionName;
        
        // Skip CALL_LEAVE events (we only process CALL_ENTER)
        if (fn.empty()) continue;

        // Dispatch the GL call through OGL entry points → GAL → HAL → MetaStream
        apitrace::dispatchCall(evt);
        
        // Handle SwapBuffers (frame boundary)
        if (fn.find("SwapBuffers") != std::string::npos) {
            std::cout << "TraceDriverApitrace: Found SwapBuffers: " << fn << std::endl;
            if (currentFrame_ >= startFrame_) {
                std::cout << "Dumping frame " << currentFrame_ << std::endl;
                ogl::swapBuffers();
                driver_->signalEvent(arch::GPU_END_OF_FRAME_EVENT, "");
            }
            currentFrame_++;
            driver_->printMemoryUsage();
        }
    }

    return agpt;
}

