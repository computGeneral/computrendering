/**************************************************************************
 *
 * Filter Operation definition file.
 *
 */


#ifndef _FILTEROPERATION_
#define _FILTEROPERATION_

#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  @file cmFilterOperation.h
 *
 *  This file defines the FilterOperation class that carrier information
 *  about the current operation in the texture unit filter pipeline.
 *
 */

/**
 *
 *  This class defines a container for filtering operations that circulate through the
 *  filter pipeline in a Texture Unit.
 *  Inherits from Dynamic Object that offers dynamic memory support and tracing capabilities.
 *
 */


class FilterOperation : public DynamicObject
{
private:

    U32 filterEntry;     //  The entry in the filter queue being processed.  

public:

    /**
     *
     *  Creates a new FilterOperation object.
     *
     *  @param filterEntry The queue entry for the filter operation being
     *  carried by this texture state info object.
     *
     *  @return A new initialized FilterOperation object.
     *
     */

    FilterOperation(U32 filterEntry);

    /**
     *
     *  Returns the queue entry for the filter operation being carried by this
     *  FilterOperation object.
     *
     *  @return The filter queue entry for this operation.
     *
     */

    U32 getFilterEntry();
};

} // namespace cg1gpu

#endif
