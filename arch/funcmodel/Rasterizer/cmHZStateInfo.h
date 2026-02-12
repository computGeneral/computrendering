/**************************************************************************
 *
 * Hierarchical Z State Info definition file.
 *
 */

/**
 *
 *  @file cmHZStateInfo.h
 *
 *  This file defines the HZStateInfo class.
 *
 *  The Hieararchical Z State Info class is used to carry state information
 *  between Hierarchical Z early test and Triangle Traversal.
 *
 */

#ifndef _HZSTATEINFO_
#define _HZSTATEINFO_

#include "DynamicObject.h"
#include "cmHierarchicalZ.h"

namespace arch
{

/**
 *
 *  This class defines a container for the state signals
 *  that the Hierarchical Z mdu sends to the Triangle Traversal mdu.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class HZStateInfo : public DynamicObject
{
private:

    HZState state;    //  The Hiearchical Z early test state.  

public:

    /**
     *
     *  Creates a new HZStateInfo object.
     *
     *  @param state The Hierarchical Z Early test state carried by this
     *  hierarchical Z state info object.
     *
     *  @return A new initialized HZStateInfo object.
     *
     */

    HZStateInfo(HZState state);

    /**
     *
     *  Returns the hiearchical Z early test state carried by the hierarchical z
     *  state info object.
     *
     *  @return The Hierarchical Z state carried in the object.
     *
     */

    HZState getState();
};

} // namespace arch

#endif
