/**************************************************************************
 *
 */

#ifndef GALx_MATRIXF_H
    #define GALx_MATRIXF_H

#include <cstring>
#include <iostream>

namespace libGAL
{
//#define MATRIX_TRACE_CODE 

#ifdef MATRIX_TRACE_CODE
    #define MATRIXF_TC(x) x
#else
    #define MATRIXF_TC(x)
#endif


class GALxMatrixf
{

private:
    
    float rows[16];    

    // a == m allowed, b == m not allowed 
    static void _mult_matrixf(float m[], const float a[], const float b[]);
    static void _transpose_matrixf(float m[]);
    static bool _invert_matrixf(const float* m, float* out);

public:

    GALxMatrixf();
    GALxMatrixf(const GALxMatrixf& m);    
    
    /**
     * Creates a GALxMatrixf object using a row or column representation array
     *
     * colRep: true means 'rows' pointer points to a matrix stored in columns
     *         false means matrix stored in rows
     */
    GALxMatrixf(const float* rows, bool colRep = false);    
    GALxMatrixf(const double* rows, bool colRep = false); // truncates doubles to floats 



    GALxMatrixf& operator=(const GALxMatrixf& m);
    
    GALxMatrixf& identityMe();

    // gets the identity matrix 
    static GALxMatrixf identity();

    // if u want to get the columns of a matrix 'm' do: m.transpose().getRows() 
    // @note: Allow direct access to internal representation in rows 
    float* getRows() const;

    GALxMatrixf operator*(const GALxMatrixf& m) const;

    // A *= B is equivalent to A = A * B (not B*A) but more efficient; 
    GALxMatrixf& operator*=(const GALxMatrixf& m);

    // gets the inverse of this matrix 
    GALxMatrixf inverse() const;        
    GALxMatrixf& inverseMe();

    // gets the transpose of this matrix 
    GALxMatrixf transpose() const;
    GALxMatrixf& transposeMe();

    /* 
     * gets the transpose inverse of this matrix 
     * equivalent to: m.inverse().transpose()
     */
    GALxMatrixf invtrans() const;
    GALxMatrixf& invtransMe();

    // allows indexing in a matrix like in a two dimensional array. Example: m[3][2] = 12.3; 
    float* operator[](int row)
    {
        return &(rows[row*4]);
    }

    /* 
     * Returns a 16 floats array containing the matrix representation in rows 
     * equivalent to getRows().
     */
    const float* operator()()
    {
        return rows;
    }

    /*
     * Compare two matrices
     */
    bool operator==(const GALxMatrixf& m) const;
    
    friend std::ostream& operator<<(std::ostream& os, const GALxMatrixf& m)
    {
        m.dump(os);
        return os;
    }


    void dump(std::ostream& out = std::cout) const;
    
};

} // namespace libGAL

#endif // GALx_MATRIXF_H
