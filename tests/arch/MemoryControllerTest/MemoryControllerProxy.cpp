/**************************************************************************
 *
 */

#include "MemoryControllerProxy.h"

using namespace arch;

MemoryControllerProxy::MemoryControllerProxy(const cmoMduBase* mc) : mc(mc)
{}

string MemoryControllerProxy::getDebugInfo() const
{
    string debugInfo;
    mc->getDebugInfo(debugInfo);
    return debugInfo;
}

