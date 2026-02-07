/**************************************************************************
 *
 */

#ifndef STATE_COMPONENT
    #define STATE_COMPONENT

#include <string>

namespace libGAL
{

/**
 * Interface for all the subcomponents that owns GAL/GPU state and required synchronization with the GPU
 */
class StateComponent
{
public:

    virtual std::string getInternalState() const = 0;

    virtual void sync() = 0;

    virtual void forceSync() = 0;
};

}

#endif // STATE_COMPONENT
