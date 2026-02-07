/**************************************************************************
 *
 * Rasterizer State Info definition file.
 *
 */


#ifndef _RASTERIZERSTATEINFO_
#define _RASTERIZERSTATEINFO_

#include "DynamicObject.h"
#include "cmRasterizer.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the state signals
 *  that the rasterizer sends to the cmoStreamController.
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class RasterizerStateInfo : public DynamicObject
{
private:
    
    RasterizerState state;    //  The rasterizer state.  

public:

    /**
     *
     *  Creates a new RasterizerStateInfo object.
     *
     *  @param state The rasterizer state carried by this
     *  rasterizer state info object.
     *
     *  @return A new initialized RasterizerStateInfo object.
     *
     */
     
    RasterizerStateInfo(RasterizerState state);
    
    /**
     *
     *  Returns the rasterizer state carried by the rasterizer
     *  state info object.
     *
     *  @return The rasterizer state carried in the object.
     *
     */
     
    RasterizerState getState();
};

} // namespace cg1gpu

#endif
