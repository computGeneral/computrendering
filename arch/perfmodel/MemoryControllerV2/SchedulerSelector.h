/**************************************************************************
 *
 */

#ifndef SCHEDULERSELECTOR_H
    #define SCHEDULERSELECTOR_H

#include "ChannelScheduler.h"
#include "MemoryControllerV2.h"

namespace arch
{

namespace memorycontroller
{

ChannelScheduler* createChannelScheduler(const char* name, 
                                         const char* prefix, 
                                         const MemoryControllerParameters& params,
                                         cmoMduBase* parent);

} // namespace memorycontroller

} // namespace arch


#endif // SCHEDULERSELECTOR_H