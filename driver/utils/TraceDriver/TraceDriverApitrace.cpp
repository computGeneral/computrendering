/**************************************************************************
 * TraceDriverApitrace.cpp
 * 
 * Implementation of apitrace trace driver.
 */

#include "TraceDriverApitrace.h"
#include "GLResolver.h"

using namespace cg1gpu;
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

cg1gpu::cgoMetaStream* TraceDriverApitrace::nxtMetaStream() {
    if (!initialized_ || parser_.eof()) 
        return nullptr;
    
    apitrace::CallEvent evt;
    if (!parser_.readEvent(evt)) 
        return nullptr;
    
    // Map to CG1 APICall
    APICall apiCall = this->mapFunctionName(evt.signature.functionName);
    
    // Track frames (SwapBuffers variants)
    if (evt.signature.functionName.find("SwapBuffers") != std::string::npos ||
        evt.signature.functionName == "glXSwapBuffers" ||
        evt.signature.functionName == "eglSwapBuffers") {
        currentFrame_++;
    }
    
    // TODO: Full MetaStream generation from apitrace arguments
    // Current implementation: framework is ready, parameter conversion needs completion.
    // For simple calls, we would:
    //   1. Extract arguments from evt.arguments map
    //   2. Convert apitrace::Value types to GL types  
    //   3. Generate MetaStream with driver_->writeGPURegister() etc.
    //
    // Example for glClear(GLbitfield mask):
    //   if (apiCall == APICall_glClear) {
    //       GLbitfield mask = evt.arguments[0].uintVal;
    //       // Call OGL library: OGL_glClear(mask);
    //       // OGL lib generates MetaStream internally
    //   }
    //
    // For now, skip all calls (returns nullptr, ends trace immediately)
    // Proper implementation requires calling the OGL/GAL library functions
    // with converted parameters to generate MetaStreams.
    
    std::cerr << "[TraceDriverApitrace] WARNING: Parameter conversion not implemented. "
              << "Trace will not produce simulation output." << std::endl;
    return nullptr;
}

