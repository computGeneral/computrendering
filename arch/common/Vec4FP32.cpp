/**************************************************************************
 *
 * Quad Float class implementation file. 
 *
 */


#include "Vec4FP32.h"
#include <iostream>

using namespace arch;
using namespace std;

Vec4FP32::Vec4FP32( F32 x, F32 y, F32 z, F32 w ) {
    
    component[0] = x;
    component[1] = y;
    component[2] = z;
    component[3] = w;
}

F32& Vec4FP32::operator[]( U32 index ) {
    GPU_ERROR_CHECK(
        if ( index < 0 || index > 3 ) {
            cout << "Error. Vec4FP32 Index out of bounds, index value: " << index << endl; //
            // REPORT_ERROR
        }
    );
    // Returning a reference allow the expression to be "left hand"
    return component[index];
}


void Vec4FP32::setComponents( F32 x, F32 y, F32 z,
                                F32 w ) {

    component[0] = x;
    component[1] = y;
    component[2] = z;
    component[3] = w;    

}

void Vec4FP32::getComponents( F32& x, F32& y, F32& z, F32& w ) {
    x = component[0];
    y = component[1];
    z = component[2];
    w = component[3];
}

F32 *Vec4FP32::getVector()
{
    return component;
}

Vec4FP32& Vec4FP32::operator=(F32 *source)
{
    component[0] = source[0];
    component[1] = source[1];
    component[2] = source[2];
    component[3] = source[3];

    return *this;
}

