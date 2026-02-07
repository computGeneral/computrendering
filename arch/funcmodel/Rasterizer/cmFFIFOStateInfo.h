/**************************************************************************
 *
 * Fragment FIFO State Info definition file.
 *
 */

/**
 *
 *  @file cmFFIFOStateInfo.h
 *
 *  This file defines the FFIFOStateInfo class.
 *
 *  The Fragment FIFO State Info class is used to carry state information
 *  from the Fragment FIFO to the Hierarchical/Early Z unit.
 *
 */

#ifndef _FFIFOSTATEINFO_
#define _FFIFOSTATEINFO_

#include "DynamicObject.h"
#include "cmFragmentFIFOState.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the state signals
 *  that the Fragment FIFO mdu sends to the Hierarchical/Early Z.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class FFIFOStateInfo : public DynamicObject
{
private:

    FFIFOState state;       //  The Fragment FIFO state.  

public:

    /**
     *
     *  Creates a new FFIFOStateInfo object.
     *
     *  @param state The Fragment FIFO state carried by this fragment FIFO state info object.
     *
     *  @return A new initialized FFIFOStateInfo object.
     *
     */

    FFIFOStateInfo(FFIFOState state);

    /**
     *
     *  Returns the Fragment FIFO state carried by the Fragment FIFO
     *  state info object.
     *
     *  @return The Fragment FIFO carried in the object.
     *
     */

    FFIFOState getState();
};

} // namespace cg1gpu

#endif
