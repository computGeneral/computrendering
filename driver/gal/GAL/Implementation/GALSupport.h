/**************************************************************************
 *
 */

#ifndef GAL_SUPPORT
    #define GAL_SUPPORT

#include "GALTypes.h"

namespace libGAL
{

namespace galsupport
{
    gal_float clamp(gal_float value);

    void clamp_vect4(gal_float* vect4);
}

}

#endif // GAL_SUPPORT
