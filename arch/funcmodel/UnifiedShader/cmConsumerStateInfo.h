/**************************************************************************
 *
 * Consumer State Info definition file.
 *
 */


#ifndef _CONSUMERSTATEINFO_
#define _CONSUMERSTATEINFO_

#include "DynamicObject.h"
#include "cmStreamer.h"

namespace arch
{

//*  This describes the shader consumer state signal states.  
enum ConsumerState
{
    CONS_READY,               //  The consumer can receive ouputs from the shaders.  
    CONS_BUSY,                //  The consumer can not receive outputs from the shaders.  

    // Specific to the cmoStreamController Unit. 

    CONS_LAST_VERTEX_COMMIT,  /**<  cmStreamer.has committed the last transformed vertex. 
                                    Hence, no more vertex shading inputs can be expected 
                                    from cmoStreamController Loader.  */
    CONS_FIRST_VERTEX_IN,     /**<  cmoStreamController is processing the first index of the new input stream.
                                    Hence, new vertex shading inputs are expected from
                                    cmoStreamController Loader.  */
};


/**
 *
 *  This class defines a container for the state signals
 *  that the StreamController sends to the Shader.
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class ConsumerStateInfo : public DynamicObject
{
private:

    ConsumerState state;    //  The StreamController (consumer) state.  

public:

    /**
     *
     *  Creates a new ConsumerStateInfo object.
     *
     *  @param state The cmoStreamController (consumer) state carried by this
     *  consumer state info object.
     *
     *  @return A new initialized ConsumerStateInfo object.
     *
     */

    ConsumerStateInfo(ConsumerState state);

    /**
     *
     *  Returns the StreamController (consumer) state carried by the
     *  consumer state info object.
     *
     *  @return The StreamController (consumer) state carried in the object.
     *
     */

    ConsumerState getState();
};

} // namespace arch

#endif
