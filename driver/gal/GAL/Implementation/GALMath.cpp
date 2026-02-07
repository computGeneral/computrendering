/**************************************************************************
 *
 */

#include "GALMath.h"
#include <cmath>

using namespace libGAL; // to use types gal_*

gal_double libGAL::ceil(gal_double x)
{
    return (x - std::floor(x) > 0)?std::floor(x) + 1:std::floor(x);
}

gal_double libGAL::logTwo(gal_double x)
{
    return std::log(x)/std::log(2.0);
}

gal_int libGAL::abs(gal_int a)
{
    return (gal_int) std::abs((gal_float) a);
}

