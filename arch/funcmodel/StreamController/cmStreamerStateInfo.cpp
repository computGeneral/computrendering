/**************************************************************************
 *
 * cmoStreamController State Info implementation file.
 *
 */

#include "cmStreamerStateInfo.h"

using namespace arch;

//  Creates a new StreamerStateInfo object.  
StreamerStateInfo::StreamerStateInfo(StreamerState newState) : state(newState)
{
    //  Set object color for tracing.  
    setColor(state);

    setTag("stStIn");
}


//  Returns the StreamController state carried by the object.  
StreamerState StreamerStateInfo::getState()
{
    return state;
}
