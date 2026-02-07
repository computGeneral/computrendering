/**************************************************************************
 *
 */

#ifndef SCHEDULERSELECTOR_H
    #define SCHEDULERSELECTOR_H

#include "cmChannelScheduler.h"
#include "cmMemoryControllerV2.h"

namespace cg1gpu
{
namespace memorycontroller
{

ChannelScheduler* createChannelScheduler(const char* name, 
                                         const char* prefix, 
                                         const MemoryControllerParameters& params,
                                         cmoMduBase* parent);

} // namespace memorycontroller
} // namespace cg1gpu


#endif // SCHEDULERSELECTOR_H
