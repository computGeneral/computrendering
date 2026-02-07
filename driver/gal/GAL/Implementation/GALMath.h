/**************************************************************************
 *
 */

#ifndef GAL_MATH
    #define GAL_MATH

#include "GALTypes.h"

namespace libGAL
{
    template<class T> T max(T a, T b) { return (a>b?a:b); } 
    template<class T> T min(T a, T b) { return (a<b?a:b); }
    gal_double ceil(gal_double a);
    gal_double logTwo(gal_double a);
    gal_int abs(gal_int a);
}

#endif // GAL_MATH
