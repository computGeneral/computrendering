/**************************************************************************
 *
 * Quad Int class implementation file.
 *
 */


#include "Vec4Int.h"
#include <iostream>

using namespace cg1gpu;
using namespace std;

Vec4Int::Vec4Int( S32 x, S32 y, S32 z, S32 w ) {

    component[0] = x;
    component[1] = y;
    component[2] = z;
    component[3] = w;
}

S32& Vec4Int::operator[]( U32 index ) {
    GPU_ERROR_CHECK(
        if ( index < 0 || index > 3 ) {
            cout << "Error. Vec4FP32 Index out of bounds, index value: " << index << endl; //
            // REPORT_ERROR
        }
    );
    // Returning a reference allow the expression to be "left hand"
    return component[index];
}


void Vec4Int::setComponents( S32 x, S32 y, S32 z, S32 w ) {

    component[0] = x;
    component[1] = y;
    component[2] = z;
    component[3] = w;

}

void Vec4Int::getComponents( S32& x, S32& y, S32& z, S32& w ) {
    x = component[0];
    y = component[1];
    z = component[2];
    w = component[3];
}

S32 *Vec4Int::getVector()
{
    return component;
}

Vec4Int& Vec4Int::operator=(S32 *source)
{
    component[0] = source[0];
    component[1] = source[1];
    component[2] = source[2];
    component[3] = source[3];

    return *this;
}


