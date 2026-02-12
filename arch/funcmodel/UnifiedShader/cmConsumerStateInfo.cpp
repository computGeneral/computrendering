/**************************************************************************
 *
 * Consumer State Info implementation file.
 *
 */

#include "cmConsumerStateInfo.h"

using namespace arch;

//  Creates a new ConsumerStateInfo object.  
ConsumerStateInfo::ConsumerStateInfo(ConsumerState newState) :
//  Set state.  
state(newState)
{
    //  Set object color for tracing.  
    setColor(state);

    setTag("ConStIn");
}


//  Returns the StreamController (consumer) state carried by the object.  
ConsumerState ConsumerStateInfo::getState()
{
    return state;
}
