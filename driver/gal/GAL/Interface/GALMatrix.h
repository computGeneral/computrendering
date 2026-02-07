/**************************************************************************
 *
 */

#ifndef GAL_MATRIX
    #define GAL_MATRIX

#include "GALTypes.h"
#include "GALVector.h"
#include <iostream>
#include <cmath>

using namespace std;

namespace libGAL
{

template<class T, gal_uint R, gal_uint C>
class GALMatrix
{
public:

    GALMatrix() {};
    GALMatrix(const T& initValue);

    GALMatrix(const T* values, gal_bool rowMajorOrder);

    T& operator()(gal_uint i, gal_uint j);

    GALVector<T,C>& operator[](gal_uint row);
    const GALVector<T,C>& operator[](gal_uint row) const;
    
    const T& operator()(gal_uint i, gal_uint j) const;

    gal_bool operator==(const GALMatrix<T,R,C>& m) const
    {
        for ( gal_uint i = 0; i < R; ++i ) {
            if ( _rows[i] != m._rows[i] )
                return false;
        }
        return true;
    }

    gal_bool operator!=(const GALMatrix<T,R,C>& m) const
    {
        for ( gal_uint i = 0; i < R; ++i ) {
            if ( _rows[i] != m._rows[i] )
                return true;
        }
        return false;
    }

    GALVector<T,R> multVector(GALVector<T,C>& v) const
    {
        GALVector<T,R> result;
        for ( gal_uint i = 0; i < R; ++i ) {
            for ( gal_uint j = 0; j < C; ++j )
                result[i] = v[j] * _rows[i][j];
        }
        return result;
    }


    GALVector<T,C> row(gal_uint r) const;

    GALVector<T,R> col(gal_uint c) const;

    GALMatrix<T,C,R> transpose() const;

    template<gal_uint C2>
    GALMatrix<T,R,C2> operator*(const GALMatrix<T,C,C2>& mat) const
    {
        GALMatrix<T,R,C2> result;
        for ( gal_uint i = 0; i < R; ++i ) {
            for ( gal_uint j = 0; j < C2; ++j ) {
                T accum = 0;
                for ( gal_uint k = 0; k < C; ++k )
                    accum += (_rows[i][k] * mat(k,j));
                result(i,j) = accum;
            }
        }
        return result;
    }

    ///////////////////
    // PRINT METHODS //
    ///////////////////

    friend std::ostream& operator<<(std::ostream& os, const GALMatrix<T,R,C>& m)
    {
        m.print(os);
        return os;
    }

    void print(std::ostream& out = cout) const;


private:

    GALVector<T,C> _rows[R];

};

////////////////////////////////////////////////////
// INVERSE OPERATION DEFINED FOR 4x4 FLOAT MATRIX // 
////////////////////////////////////////////////////

template<class T,gal_uint RC>
void identityMatrix(GALMatrix<T,RC,RC>& m)
{
    for ( gal_uint i = 0; i < RC; ++i ) {
        for ( gal_uint j = 0; j < RC; ++j )
            m(i,j) = static_cast<T>(0);
    }

    for ( gal_uint i = 0; i < RC; ++i )
        m(i,i) = static_cast<T>(1);
}

/**
 * Computes the inverse of the input 4x4 matrix
 *
 * Code contributed by Jacques Leroy jle@star.be. 
 *
 * @param    outMat    The output 4x4 gal_float inverse matrix.
 * @param    inMat    The input 4x4 gal_float matrix.
 * @returns True for success, false for failure (singular matrix).
 */
libGAL::gal_bool _inverse(libGAL::GALMatrix<libGAL::gal_float,4,4>& outMat, const libGAL::GALMatrix<libGAL::gal_float,4,4>& inMat);

libGAL::gal_bool _translate(libGAL::GALMatrix<libGAL::gal_float,4,4>& inOutMat, 
                            gal_float x, gal_float y, gal_float z);

void _scale(libGAL::GALMatrix<libGAL::gal_float,4,4>& inOutMat,
                    gal_float x, gal_float y, gal_float z);

void _rotate(libGAL::GALMatrix<libGAL::gal_float,4,4>& inOutMat, 
                            gal_float angle, gal_float x, gal_float y, gal_float z);

void _mult_mat_vect( libGAL::GALVector<libGAL::gal_float,4>& vect, const libGAL::GALMatrix<libGAL::gal_float,4,4>& mat);
// Convenient matrix definition
typedef GALMatrix<gal_float,4,4> GALFloatMatrix4x4;


} // namespace libGAL

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALMatrix<T,R,C>::GALMatrix(const T& initValue)
{
    for ( gal_uint i = 0; i < R; ++i ) {
        for ( gal_uint j = 0; j < C; ++j ) {
            _rows[i][j] = initValue;
        }
    }
}

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALMatrix<T,R,C>::GALMatrix(const T* values, libGAL::gal_bool rows) 
{
    gal_uint k = 0;
    if ( rows ) {
        for ( gal_uint i = 0; i < R; i++ ) {
            for ( gal_uint j = 0; j < C; ++j ) {
                _rows[i][j] = values[k++];
            }
        }
    }
    else {
        for ( gal_uint i = 0; i < C; i++ ) {
            for ( gal_uint j = 0; j < R; ++j ) {
                _rows[j][i] = values[k++];
            }
        }
    }
}

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALVector<T,C>& libGAL::GALMatrix<T,R,C>::operator[](libGAL::gal_uint row)
{
    return _rows[row];
}

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
const libGAL::GALVector<T,C>& libGAL::GALMatrix<T,R,C>::operator[](libGAL::gal_uint row) const
{
    return _rows[row];
}


template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
T& libGAL::GALMatrix<T,R,C>::operator()(libGAL::gal_uint i, libGAL::gal_uint j) { return _rows[i][j]; }

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
const T& libGAL::GALMatrix<T,R,C>::operator()(libGAL::gal_uint i, libGAL::gal_uint j) const { return _rows[i][j]; }

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALVector<T,C> libGAL::GALMatrix<T,R,C>::row(libGAL::gal_uint r) const { return _rows[r]; }

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALVector<T,R> libGAL::GALMatrix<T,R,C>::col(libGAL::gal_uint c) const
{
    GALVector<T,R> column;
    for ( gal_uint i = 0; i < R; ++i )
        column[i] = _rows[i][c];
    return column;
}

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
libGAL::GALMatrix<T,C,R> libGAL::GALMatrix<T,R,C>::transpose() const
{
    GALMatrix<T,C,R> m;
    for ( gal_uint i = 0; i < R; ++i ) {
        for ( gal_uint j = 0; j < C; ++j )
            m(j,i) = _rows[i][j];
    }
    return m;
}

template<class T, libGAL::gal_uint R, libGAL::gal_uint C>
void libGAL::GALMatrix<T,R,C>::print(std::ostream& out) const
{
    for ( libGAL::gal_uint i = 0; i < R; ++i ) 
    {
        out << "{";

        for ( libGAL::gal_uint j = 0; j < C; ++j )
        {
            out << _rows[i][j];
            if ( j != C - 1 ) out << ",";
        }

        out << "}";

        if ( i != R - 1 ) out << ",";
    }

    out.flush();
}



#endif // GAL_MATRIX
