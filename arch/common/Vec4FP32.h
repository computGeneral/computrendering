/**************************************************************************
 *
 * Quad Fload class definition file. 
 *
 */


#ifndef __QUAD_FLOAT__
    #define __QUAD_FLOAT__

#include "support.h"
#include "GPUType.h"
#include <ostream>

namespace cg1gpu
{

//! Class Vec4FP32 defines a 4-component vector of floats
class Vec4FP32 {

private:
    
    F32 component[4]; //!< Data

public:
    
    //! Constructor
    Vec4FP32( F32 x = 0, F32 y = 0, F32 z = 0, F32 w = 0 );

    //!Allows indexed access & LH position ( implements set & get ) to individual components
    F32& operator[]( U32 index );
    
    /*!
     * Modifies ALL components with new values, components without values are set to 0
     * i.e: qf.setComponents( 1.0, 3.0 );
     * The Vec4FP32 resultant is ( 1.0, 3.0, 0.0, 0.0 )
     */
    void setComponents( F32 x = 0, F32 y = 0, F32 z = 0, F32 w = 0 );

    /*!
     * Gets all the Vec4FP32 components
     */
    void getComponents( F32& x, F32& y, F32& z, F32& w );

    
    /**
     *  Returns the pointer to the vector of F32 components
     *  for the Vec4FP32.
     *
     */

    F32 *getVector();

    /**
     *
     *  Assignment operator.  Copy from an array of float point values.
     *
     */
     
    Vec4FP32& operator=(F32 *source);
    
    //! Must be declared and defined inside the class
    friend std::ostream& operator<<( std::ostream& theStream, const Vec4FP32& qf ) {
            
        theStream << "(" << qf.component[0] << "," << qf.component[1] << "," 
                         << qf.component[2] << "," << qf.component[3] << ")";
        return theStream;
    }
    
};

} // namespace cg1gpu

#endif
