/**************************************************************************
 *
 * Multi Clock cmoMduBase class definition file.
 *
 */

#ifndef __MULTICLOCKBOX__
   #define __MULTICLOCKBOX__

#include "cmMduBase.h"

namespace cg1gpu
{

/**
 *  Specialization of the cmoMduBase class for boxes that require multiple clock domains.
 */
 
class cmoMduMultiClk : public cmoMduBase
{
private:

public:
    /**
     *  Multi Clock cmoMduBase constructor.
     *  Current implementation just calls cmoMduBase constructor.
     *  @param name Name of the multi clocked mdu.
     *  @param parent Pointer to the parent mdu of the multi clocked mdu.
     *  @return A new cmoMduMultiClk instance.
     */
    cmoMduMultiClk(const char *name, cmoMduBase *parent);
     
    /**
     *  Pure virtual clock function for a single clock domain.
     *  Updates the state of one of the single clock domain implemented in the mdu.
     *  @param cycle Current simulation cycle for the clock domain to update.
     */
    virtual void clock(U64 cycle) = 0;

    /**
     *  Pure virtual clock function for multiple clock domains.
     *  Updates the state of one of the clock domains implemented in the mdu.
     *  @param domain Clock domain from the mdu to update.
     *  @param cycle Current simulation cycle for the clock domain to update.
     */
    virtual void clock(U32 domain, U64 cycle) = 0;
};

} // namespace cg1gpu

#endif
