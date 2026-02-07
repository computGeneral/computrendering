/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERSELECTOR_H
    #define MEMORYCONTROLLERSELECTOR_H

#include "cmMduBase.h"
#include "ConfigLoader.h"

namespace cg1gpu
{

cmoMduBase* createMemoryController(cgsArchConfig& arch, 
                            const char** tuPrefixes, 
                            const char** suPrefix,
                            const char** slPrefixes,
                            const char* memoryControllerName,
                            cmoMduBase* parentBox);

}

#endif // MEMORYCONTROLLERSELECTOR_H
