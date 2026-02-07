/**************************************************************************
 *
 */

#include "GALSupport.h"

using namespace libGAL;

gal_float galsupport::clamp(gal_float v)
{
    return ((v)>1.0f?1.0f:((v)<0.0f?0.0f:(v)));
}

void galsupport::clamp_vect4(gal_float* vect4)
{
    vect4[0] = libGAL::galsupport::clamp(vect4[0]);
    vect4[1] = libGAL::galsupport::clamp(vect4[1]);
    vect4[2] = libGAL::galsupport::clamp(vect4[2]);
    vect4[3] = libGAL::galsupport::clamp(vect4[3]);
}
