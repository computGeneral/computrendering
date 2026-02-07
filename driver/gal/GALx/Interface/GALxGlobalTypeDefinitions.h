/**************************************************************************
 *
 */

#ifndef GALx_GLOBAL_TYPE_DEFINITIONS_H
    #define GALx_GLOBAL_TYPE_DEFINITIONS_H

#include "GALTypes.h"
#include "GALVector.h"
#include "GALMatrix.h"

namespace libGAL
{
///////////////////////
//     Vectors       //
///////////////////////

/**
 * Vector of 4 gal_float components defined from the GALVector class
 */
typedef GALVector<gal_float,4> GALxFloatVector4;

/**
 * Vector of 4 gal_float components defined from the GALVector class
 */
typedef GALVector<gal_float,3> GALxFloatVector3;

//////////////////////
//    Matrices      //
//////////////////////
/**
 * Matrix of 4x4 gal_float components defined from the GALMatrix class
 */
typedef GALMatrix<gal_float,4,4> GALxFloatMatrix4x4;

} // namespace libGAL

#endif // GALx_GLOBAL_TYPE_DEFINITIONS_H
