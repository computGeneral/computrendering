/**************************************************************************
 *
 * Quad Int class definition file. 
 *
 */


#ifndef __QUADINT__
    #define __QUADINT__

#include "GPUType.h"
#include <ostream>

namespace arch
{

//! Class Vec4Int defines a 4-component 32 bit integer vector
class Vec4Int {

private:
    
    S32 component[4]; //!< Data

public:
    
    //! Constructor
    Vec4Int( S32 x = 0, S32 y = 0, S32 z = 0, S32 w = 0 );

    //!Allows indexed access & LH position ( implements set & get ) to individual components
    S32& operator[]( U32 index );
    
    /*!
     * Modifies ALL components with new values, components without values are set to 0
     * i.e: qi.setComponents( 1, 3 );
     * The Vec4Int resultant is ( 1, 3, 0, 0 )
     */
    void setComponents( S32 x = 0, S32 y = 0, S32 z = 0, S32 w = 0 );

    /*!
     * Gets all the Vec4Int components
     */
    void getComponents( S32& x, S32& y, S32& z, S32& w );


    /**
     *  Get the pointer to the S32 vector for Vec4Int.    
     *
     */
     
    S32 *getVector();
    
    /**
     *
     *  Assignment operator.  Copy values from an array of integer values.
     *
     */
     
    Vec4Int& operator=(S32 *source);
         
     
    //! Must be declared and defined inside the class
    friend std::ostream& operator<<( std::ostream& theStream, const Vec4Int& qi ) {
            
        theStream << "(" << qi.component[0] << "," << qi.component[1] << "," 
            << qi.component[2] << "," << qi.component[3] << ")";
    
        return theStream;
    }

};

} // namespace arch

#endif
