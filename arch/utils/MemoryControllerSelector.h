/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERSELECTOR_H
    #define MEMORYCONTROLLERSELECTOR_H

#include "cmMduBase.h"
#include "ArchConfig.h"

namespace arch
{

cmoMduBase* createMemoryController(cgsArchConfig& arch, 
                            const char** tuPrefixes, 
                            const char** suPrefix,
                            const char** slPrefixes,
                            const char* memoryControllerName,
                            cmoMduBase* parentBox);

}

#endif // MEMORYCONTROLLERSELECTOR_H
