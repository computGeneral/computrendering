/**************************************************************************
 *
 * OpenGL API library internal implementation.
 *
 */

#include "ProgramManager.h"
#include "GPULibInternals.h"
#include "VSLoader.h"
#include "Matrixf.h"

// Don't open namespace (it clashes with other enums used here)
//using namespace glsNS;

HAL* driver = 0;
libgl::GLContext* ctx = 0;
bool supportedMode = true;
// ClientDescriptorManager* libClientDescriptors = ClientDescriptorManager::instance();
ClientDescriptorManager* libClientDescriptors = 0;

ClientDescriptorManager::ClientDescriptorManager()
{}

ClientDescriptorManager* ClientDescriptorManager::instance()
{
    static ClientDescriptorManager gmdm;
    return &gmdm;
}

void ClientDescriptorManager::save(const std::vector<U32>& memDescs)
{
    descs = memDescs;
}

void ClientDescriptorManager::release()
{
    std::vector<U32>::iterator it = descs.begin();
    for ( ; it != descs.end(); it++ )
        driver->releaseMemory(*it);
    descs.clear(); // throw away descriptors
}

// legncy code void initLibOGL14(HAL* d, bool triangleSetupOnShader)
// legncy code {
// legncy code     driver = d;
// legncy code     ctx = new libgl::GLContext(driver, triangleSetupOnShader);
// legncy code     libClientDescriptors = ClientDescriptorManager::instance();
// legncy code }


//  Our own swap buffer function.
void privateSwapBuffers()
{
//if (frame >= START_FRAME)
    //  Request a framebuffer swap to the driver.
    driver->sendCommand(arch::GPU_SWAPBUFFERS);
//else
//printf("LIB >> Skipping frame %d\n", frame);

//frame++;
}
