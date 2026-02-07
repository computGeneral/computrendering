/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERPROXY_H
    #define MEMORYCONTROLLERPROXY_H

#include "MemoryControllerV2.h"
#include "MemoryController.h"
#include <string>

namespace cg1gpu
{

// Simplified memory controller interface to query memory controllers state
class MemoryControllerProxy
{
private:

    const cmoMduBase* mc;

public:

    MemoryControllerProxy(const cmoMduBase* mc);

    std::string getDebugInfo() const;

};

}

#endif // MEMORYCONTROLLERPROXY_H
