/**************************************************************************
 *
 * cmoStreamController State Info definition file.
 *
 */


#ifndef _STREAMERSTATEINFO_
#define _STREAMERSTATEINFO_

#include "DynamicObject.h"
#include "cmStreamer.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the state signals
 *  that the StreamController sends to the Command Processor.
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class StreamerStateInfo : public DynamicObject
{
private:
    
    StreamerState state;    //  The StreamController state.  

public:

    /**
     *
     *  Creates a new StreamerStateInfo object.
     *
     *  @param state The cmoStreamController state carried by this
     *  StreamController state info object.
     *
     *  @return A new initialized StreamerStateInfo object.
     *
     */
     
    StreamerStateInfo(StreamerState state);
    
    /**
     *
     *  Returns the StreamController state carried by the stream
     *  state info object.
     *
     *  @return The StreamController state carried in the object.
     *
     */
     
    StreamerState getState();
};

} // namespace cg1gpu

#endif
