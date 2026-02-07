/**************************************************************************
 *
 */

#include "GAL.h"
#include "GALBuffer.h"
#include "GALVector.h"
#include "GALMatrix.h"
#include <iostream>

using namespace libGAL;
using namespace std;

template<class T,gal_uint D>
ostream& operator<<(ostream& os, const GALVector<T,D>& vect)
{
    gal_uint i = 0;
    os << "{";
    for ( ; i < vect.dim() - 1; ++i )
        os << vect[i] << ",";
    os << vect[i] << "}";
    return os;
}

template<class T,gal_uint R, gal_uint C>
ostream& operator<<(ostream& os, const GALMatrix<T,R,C>& mat)
{
    os << "{";
    for ( gal_uint i = 0; i < R; ++i ) {
        os << "{";
        for ( gal_uint j = 0; j < C; ++j ) {
            os << mat(i,j);
            if ( j < C - 1)
                os << ",";
            else
                os << "}\n";
        }
        if ( i < R - 1 ) os << ",";
        else os << "}\n";
    }
    return os;
}

/*
void matrixTest()
{
    gal_uint data[] = {
        1,2,3,4,
        5,6,7,8,
        9,10,11,12 };

    GALMatrix<gal_float,3,4> a(0);

    a(0,1) = 1;

    GALMatrix<gal_float,4,3> b;
    
    b = a.transpose();

    GALMatrix<gal_int,3,4> A(data, true);    
    GALMatrix<gal_int,4,3> B(data, false);
    B(0,2) = 10;
    B(1,2) = 11;
    B(2,2) = 12;
    B(3,2) = 13;
    GALMatrix<gal_int,3,3> C;

    C = A * B;

    cout << "Matrix A = \n" << A << "\n";
    cout << "Matrix B = \n" << B << "\n";
    cout << "Matrix C = \n" << C << "\n";
}
*/

void main()
{
    using namespace libGAL;
    GALDevice* galDev = libGAL::createDevice(0);
    cout << "Available streams: " << galDev->availableStreams() << "\n";
    /*
    MortonTransformation mt;

    mt.setTilingParameters(3,3);
    */

/*
    gal_int a_data[] = {1,2,1,2};
    gal_int b_data[] = {10,-10, 10, -10};

    GALVector<gal_int, 4> a(-1);
    GALVector<gal_int,4> b(b_data);
    gal_int s;

    cout << "a: " << a << "\n";
    cout << "b: " << b << "\n";
    cout << "result: " << s << "\n";



    GALDevice* galDev = libGAL::createDevice();

    GALBuffer* buffer = galDev->createBuffer();

    buffer->resize(1, false);

    cout << "GALTest finished!" << endl;

    GAL_RT_DESC rtDesc = { GAL_FORMAT_UNKNOWN, GAL_RT_DIMENSION_BUFFER, { 48, 24 } };

    GALRenderTarget* rt = galDev->createRenderTarget(buffer, &rtDesc);

    galDev->setRenderTargets(1, &rt, 0);
    
*/

}
