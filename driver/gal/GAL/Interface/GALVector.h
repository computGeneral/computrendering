/**************************************************************************
 *
 */

#ifndef GAL_VECTOR
#define GAL_VECTOR

#include "GALTypes.h"
#include <ostream>
#include <iostream>
#include <cmath>
#include <cstring>

namespace libGAL
{

/**
 * Templatized standard GAL vector class
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @date 03/13/2007
 */
template<class T, gal_uint D> class GALVector
{
public:

    /**
     * Creates an uninitialized vector of dimension D
     */
    inline GALVector();

    /**
     * Creates a vector of dimension D with all all its components initialized to the same value
     *
     * @param initValue Initial value of every vector compoent
     */
    GALVector(T initValue);

    /**
     * Creates a vector of dimension D with the "first" components initialized to a value
     * and the remaining components initialized to a specific default value
     *
     * @param initValue Value of the "first" components
     * @param count Number of components starting from the first one that use the 'initialValue'
     * @param def Default value for the components after the first ones
     */
    GALVector(T initValue, gal_uint count, T def = 0);

    /**
     * Creates a vector of dimension D using an input array to initialize all the vector components
     *
     * @param values An array of D values used to initialize the D components of the vector
     *
     * @note It is assumed that values contains at least D values, otherwise the expected behaviour is
     *       undefined
     */
    GALVector(const T* values);

    /**
     * Creates a vector of dimension D using a number of values from an input array and setting
     * the remaining components (if exists), with a default value
     *
     * @param values An array used to initialize the "first" components of the vector
     * @param count Number of values to use from the array
     * @param def Default value used to initialize the remaining components not initialized
     *        with values coming from the array
     */
    GALVector(const T* values, gal_uint count, T def = 0);

    /**
     * Returns a read/write reference of a vector component
     *
     * @param index component index
     * @returns a read/write reference to the vector component
     *
     * @note Bounds checking is not performed (like c-like arrays)
     * @note This operator is automatically selected in "non-const contexts"
     */
    inline T& operator[](gal_uint index);

    /**
     * Returns a read-only reference of a vector component
     *
     * @param index component index
     * @returns a read-only reference to the vector component
     *
     * @note Bounds checking is not performed (like c-like arrays)
     * @note This operator is automatically selected in "const contexts"
     */
    inline const T& operator[](gal_uint index) const;

    /**
     * Assigns the contents from a vector of dimension D to this vector
     *
     * @param vect A vector of dimension D and type T
     */
    inline GALVector& operator=(const GALVector& vect);

    /**
     * Assigns the contents from an c-like array of dimension D to this vector
     *
     * @param vect A c-like array containing at least D values
     *
     * @note It is assumed that vect contains at least D values, otherwise the expected behaviour is
     *       undefined
     */
    inline GALVector& operator=(const T* vect);

    /**
     * Multiplies each vector component by one scalar value
     *
     * @param scalar The scalar value used to multiply each vector component
     *
     * @returns A new vector with each component equals to the same component in the original vector
     *          multiplied by the scalar value
     */
    GALVector operator*(const T& scalar) const;

    /**
     * Computes the DOT product between the vector and another input vector
     *
     * @param vect The second vector used to compute the DOT product
     *
     * @returns The DOT product of the two input vectors
     */
    T operator*(const GALVector& vect) const;

    /**
     * Perform the addition of the vector with another input vector
     *
     * @param vect The second vector used in the addition operation
     *
     * @returns A vector which is the addition of the two input vectors
     */
    GALVector operator+(const GALVector& vect) const;

    /**
     * Perform the subtract if the vector with a second vector
     *
     * @param the second vector used in the subtrac operation
     *
     * @returns A vector which is the subtract between the two vectors
     */
    GALVector operator-(const GALVector& vect) const;

    /**
     * Applies the possitive sign of type T to each vector component
     *
     * @returns A vector with the possitive sign of type T applied to each vector component
     */
    GALVector operator+() const;

    /**
     * Applies the negative sign of type T to each vector component
     *
     * @returns A vector with the negative sign of type T applied to each vector component
     */
    GALVector operator-() const;

    /**
     * Check if two vectors are equal (all components have the same value)
     *
     * @param v The second vector of the comparison
     *
     * @returns True if the two vector are equal, false otherwise
     */
    inline gal_bool operator==(const GALVector<T,D>& v) const;

    /**
     * Check if two vectors are different (if they have any component different)
     *
     * @param v The second vector of the comparison
     *
     * @returns True if the two vectors are different, false otherwise
     */
    inline gal_bool operator!=(const GALVector<T,D>& v) const;
    /**
     * Returns the vector dimension (D)
     *
     * @returns the vector dimension
     */
    inline gal_uint dim() const;

    /**
     * Returns the internal data managed by this vector
     *
     * @returns the internal data through a pointer to non-const data (used automatically in non-const contexts)
     */
    inline T* c_array();

    /**
     * Returns the internal data managed by this vector
     *
     * @returns the internal data through a pointer to const data (used automatically in const contexts)
     */
    inline const T* c_array() const;

    ///////////////////
    // PRINT METHODS //
    ///////////////////

    friend std::ostream& operator<<(std::ostream& os, const GALVector<T,D>& v)
    {
        v.print(os);
        return os;
    }

    void print(std::ostream& out = std::cout) const;

private:

    T _data[D];
};

/**
 * Convenient matrix definitions frequently used
 */
typedef GALVector<gal_float,4> GALFloatVector4;
typedef GALVector<gal_float,3> GALFloatVector3;

typedef GALVector<gal_uint,4> GALUintVector4;
typedef GALVector<gal_uint,3> GALUintVector3;

typedef GALVector<gal_int,4> GALIntVector4;
typedef GALVector<gal_int,3> GALIntVector3;


/**
 * Helper function to implement simetric scalar product behaviour
 * i.e: 4 * v == v * 4
 */
template<class T, libGAL::gal_uint D>
inline GALVector<T,D> operator*(const T& scalar, const GALVector<T,D>& vect);

////////////////////////////////////////////////////
// EUCLIDIAN OPERATIONS DEFINED FOR FLOAT VECTORS // 
////////////////////////////////////////////////////

/**
 * Returns the length (euclidian norm) of a GALVector<gal_float,D> with D
 * taking an arbitrary number.
 *
 * This is computed as the root square of the sum of the squared components).
 *
 * @param    vect    The input GALVector.
 * @returns The vector length
 */
template<libGAL::gal_uint D>
libGAL::gal_float _length(const GALVector<gal_float,D>& vect);

/**
 * Returns a copy of the equivalent normalized GALVector<gal_float,D> with D
 * taking an arbitrary number.
 *
 * The normalization is computed by dividing all the components by the length.
 *
 * @param    vect    The input GALVector.
 * @returns The normalized vector copy
 */
template<libGAL::gal_uint D>
libGAL::GALVector<gal_float,D> _normalize(const GALVector<gal_float,D>& vect);

} // namespace libGAL


//////////////////////////////////////
//// GAL Template code definition ////
//////////////////////////////////////

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>::GALVector(T initValue)
{
    for ( gal_uint i = 0; i < D; ++i )
        _data[i] = initValue;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>::GALVector(const T* values, gal_uint count, T def)
{
    gal_uint i = 0;
    for ( ; i < D && i < count; ++i )
        _data[i] = values[i];
    for ( ; i < D; ++i )
        _data[i] = def;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>::GALVector(T initValue, gal_uint count, T def)
{
    gal_uint i = 0;
    for ( ; i < D && i < count; ++i )
        _data[i] = initValue;
    for ( ; i < D; ++i )
        _data[i] = def;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>::GALVector(const T* vect) { memcpy(_data, vect, D*sizeof(T)); }

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>::GALVector() {}

template<class T, libGAL::gal_uint D>
libGAL::gal_uint libGAL::GALVector<T,D>::dim() const { return D; }

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>& libGAL::GALVector<T,D>::operator=(const GALVector<T,D>& vect)
{
    memcpy(_data, vect._data, D*sizeof(T));
    return *this;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D>& libGAL::GALVector<T,D>::operator=(const T* vect)
{
    memcpy(_data, vect, D*sizeof(T));
    return *this;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D> libGAL::GALVector<T,D>::operator*(const T& scalar) const
{
    GALVector<T,D> result;
    const T* in = _data;
    T* out = result._data;
    for ( gal_uint i = 0; i < D; ++i )
        *out++ = *in++ * scalar;

    return result;
}

template<class T, libGAL::gal_uint D>
T libGAL::GALVector<T,D>::operator*(const libGAL::GALVector<T,D>& vect) const
{
    T accum(0);

    const T* a = _data;
    const T* b = vect._data;
    for ( gal_uint i = 0; i < D; ++i )
        accum += *a++ * *b++;
    
    return accum;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D> libGAL::GALVector<T,D>::operator+(const GALVector& vect) const
{
    GALVector<T,D> result;
    const T* a = _data;
    const T* b = vect._data;
    T* out = result._data;
    for ( gal_uint i = 0; i < D; ++i )
        *out++ = *a++ + *b++;
    return result;
}

template<class T, libGAL::gal_uint D>
const T& libGAL::GALVector<T,D>::operator[](libGAL::gal_uint index) const 
{
    // no bounds checking performed!
    return _data[index]; 
}

template<class T, libGAL::gal_uint D>
T& libGAL::GALVector<T,D>::operator[](libGAL::gal_uint index)
{
    // no bounds checking performed!
    return _data[index];
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D> libGAL::GALVector<T,D>::operator+() const
{
    return *this;
}

template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D> libGAL::GALVector<T,D>::operator-() const
{
    GALVector<T,D> result;
    const T* in = _data;
    T* out = result._data;
    for ( gal_uint i = 0; i < D; ++i )
        *out++ = - *in++;
    return result;
}

template<class T, libGAL::gal_uint D>
libGAL::gal_bool libGAL::GALVector<T,D>::operator==(const libGAL::GALVector<T,D>& v) const
{
    return (memcmp(_data, v._data, D * sizeof(T)) == 0);
}

template<class T, libGAL::gal_uint D>
libGAL::gal_bool libGAL::GALVector<T,D>::operator!=(const libGAL::GALVector<T,D>& v) const
{
    return (memcmp(_data, v._data, D * sizeof(T)) != 0);
}



template<class T, libGAL::gal_uint D>
T* libGAL::GALVector<T,D>::c_array() { return _data; }

template<class T, libGAL::gal_uint D>
const T* libGAL::GALVector<T,D>::c_array() const { return _data; }


template<class T, libGAL::gal_uint D>
libGAL::GALVector<T,D> libGAL::operator*(const T& scalar, const libGAL::GALVector<T,D>& vect)
{
    return vect * scalar;
}

template<libGAL::gal_uint D>
libGAL::gal_float libGAL::_length(const libGAL::GALVector<gal_float,D>& v)
{
    gal_float accum = gal_float(0);

    for (gal_uint i=0; i<D; i++)
        accum += v[i] * v[i];

    return gal_float(sqrt(accum));
}

template<libGAL::gal_uint D>
libGAL::GALVector<libGAL::gal_float,D> libGAL::_normalize(const libGAL::GALVector<libGAL::gal_float,D>& v)
{
    GALVector<gal_float,D> ret;
    
    for (gal_uint i=0; i<D; i++)
    {
        ret[i] = v[i] / _length(v);
    }
    
    return ret;
}

template<class T, libGAL::gal_uint D>
void libGAL::GALVector<T,D>::print(std::ostream& out) const
{
    //const T* v = _data;

    out << "{";

    for ( gal_uint i = 0; i < D; i++ )
    {
        out << _data[i];
        if ( i != D - 1 ) out << ",";
    }

    out << "}";

    out.flush();
}


#endif // GAL_VECTOR
