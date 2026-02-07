/**************************************************************************
 *
 */

#include "GALMatrix.h"
#include "GALMath.h"
#include <cmath>

using namespace libGAL;

//#define PI = 3.1415926536;
#define DEG2RAD (3.1415926536 / 180.0)

/*
 * Identity 4x4 Matrix
 */
static gal_float IDENTITY[] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void _aux_mult_matrixf( gal_float m[], const gal_float a[], const gal_float b[] )
{

#define A(row,col) a[(col<<2) + row]
#define B(row,col) b[(col<<2) + row]
#define M(row,col) m[(col<<2) + row]

    for ( int i = 0; i < 4; i++ ) // current row 
    {
        // a == m allowed, b == m not allowed 
        const gal_float ai0 = A(i,0);
        const gal_float ai1 = A(i,1);
        const gal_float ai2 = A(i,2);
        const gal_float ai3 = A(i,3);

        M(i,0) = ai0*B(0,0) + ai1*B(1,0) + ai2*B(2,0) + ai3*B(3,0);
        M(i,1) = ai0*B(0,1) + ai1*B(1,1) + ai2*B(2,1) + ai3*B(3,1);
        M(i,2) = ai0*B(0,2) + ai1*B(1,2) + ai2*B(2,2) + ai3*B(3,2);
        M(i,3) = ai0*B(0,3) + ai1*B(1,3) + ai2*B(2,3) + ai3*B(3,3);
    }

#undef A
#undef B
#undef M

}

void libGAL::_mult_mat_vect( libGAL::GALVector<libGAL::gal_float,4>& vect, const libGAL::GALMatrix<libGAL::gal_float,4,4>& mat)
{
    gal_float x = vect[0];
    gal_float y = vect[1];
    gal_float z = vect[2];
    gal_float w = vect[3];

    #define __DP4(i) vect[i] = x*mat(0,i) + y*mat(1,i) + z*mat(2,i) + w*mat(3,i);

    __DP4(0);
    __DP4(1);
    __DP4(2);
    __DP4(3);

    #undef __DP4
}

#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[r][c]

libGAL::gal_bool libGAL::_translate(libGAL::GALMatrix<libGAL::gal_float,4,4>& mat, 
                            gal_float x, gal_float y, gal_float z)
{
    mat(0,3) = mat(0,0) * x + mat(0,1) * y + mat(0,2) * z + mat(0,3);
    mat(1,3) = mat(1,0) * x + mat(1,1) * y + mat(1,2) * z + mat(1,3);
    mat(2,3) = mat(2,0) * x + mat(2,1) * y + mat(2,2) * z + mat(2,3);
    mat(3,3) = mat(3,0) * x + mat(3,1) * y + mat(3,2) * z + mat(3,3);

    return true;
}

void libGAL::_scale(libGAL::GALMatrix<libGAL::gal_float,4,4>& inOutMat,
                    gal_float x, gal_float y, gal_float z)
{
    gal_float mat[16]; // input matrix copy 
    for ( gal_uint i = 0; i < 4; ++i ) { // iterate over cols
        for ( gal_uint j = 0; j < 4; ++j ) // iterate over rows
            mat[i*4 + j] = inOutMat(j,i);
    }

    gal_float m[16]; // scale matrix 
    
    memcpy(m, IDENTITY, sizeof(gal_float)*16);

    m[0] *= x;
    m[5] *= y;
    m[10] *= z;

    _aux_mult_matrixf(mat, mat, m);

    for ( gal_uint i = 0; i < 4; ++i ) { // iterate over cols
        for ( gal_uint j = 0; j < 4; ++j ) // iterate over rows
            inOutMat(j,i) = mat[i*4 + j];
    }
}

void libGAL::_rotate(libGAL::GALMatrix<libGAL::gal_float,4,4>& inOutMat, 
                            gal_float angle, gal_float x, gal_float y, gal_float z)
{

    gal_float mat[16];
    for ( gal_uint i = 0; i < 4; ++i ) { // iterate over cols
        for ( gal_uint j = 0; j < 4; ++j ) // iterate over rows
            mat[i*4 + j] = inOutMat(j,i);
    }


   gal_float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c, s, c;
   gal_float m[16]; // rotation matrix 
   bool optimized;

   s = (gal_float)std::sin(angle * DEG2RAD );
   c = (gal_float)std::cos( angle * DEG2RAD );

   memcpy(m, IDENTITY, sizeof(gal_float)*16);
   optimized = false;

#define M(row,col)  m[col*4+row]

   if (x == 0.0F) {
      if (y == 0.0F) {
         if (z != 0.0F) {
            optimized = true;
            // rotate only around z-axis 
            M(0,0) = c;
            M(1,1) = c;
            if (z < 0.0F) {
               M(0,1) = s;
               M(1,0) = -s;
            }
            else {
               M(0,1) = -s;
               M(1,0) = s;
            }
         }
      }
      else if (z == 0.0F) {
         optimized = true;
         // rotate only around y-axis 
         M(0,0) = c;
         M(2,2) = c;
         if (y < 0.0F) {
            M(0,2) = -s;
            M(2,0) = s;
         }
         else {
            M(0,2) = s;
            M(2,0) = -s;
         }
      }
   }
   else if (y == 0.0F) {
      if (z == 0.0F) {
         optimized = true;
         // rotate only around x-axis 
         M(1,1) = c;
         M(2,2) = c;
         if (y < 0.0F) {
            M(1,2) = s;
            M(2,1) = -s;
         }
         else {
            M(1,2) = -s;
            M(2,1) = s;
         }
      }
   }

   if (!optimized) {
      const gal_float mag = (gal_float) std::sqrt(x * x + y * y + z * z);

      if (mag <= 1.0e-4) {
         // no rotation, leave mat as-is 
         return;
      }

      x /= mag;
      y /= mag;
      z /= mag;


      /*
       * Arbitrary axis rotation matrix.
       *
       * Version extracted from Mesa 5.0
       * File: src\math\m_matrix.c
       * Function: _math_matrix_rotate()
       */

      xx = x * x;
      yy = y * y;
      zz = z * z;
      xy = x * y;
      yz = y * z;
      zx = z * x;
      xs = x * s;
      ys = y * s;
      zs = z * s;
      one_c = 1.0F - c;

      // We already hold the identity-matrix so we can skip some statements 
      M(0,0) = (one_c * xx) + c;
      M(0,1) = (one_c * xy) - zs;
      M(0,2) = (one_c * zx) + ys;
//    M(0,3) = 0.0F; 

      M(1,0) = (one_c * xy) + zs;
      M(1,1) = (one_c * yy) + c;
      M(1,2) = (one_c * yz) - xs;
//    M(1,3) = 0.0F; 

      M(2,0) = (one_c * zx) - ys;
      M(2,1) = (one_c * yz) + xs;
      M(2,2) = (one_c * zz) + c;
//    M(2,3) = 0.0F; 

/*
      M(3,0) = 0.0F;
      M(3,1) = 0.0F;
      M(3,2) = 0.0F;
      M(3,3) = 1.0F;
*/
   }
#undef M

    _aux_mult_matrixf(mat, mat, m);

    for ( gal_uint i = 0; i < 4; ++i ) { // iterate over cols
        for ( gal_uint j = 0; j < 4; ++j ) // iterate over rows
            inOutMat(j,i) = mat[i*4 + j];
    }
}

gal_bool libGAL::_inverse(GALMatrix<gal_float,4,4>& outMat, const GALMatrix<gal_float,4,4>& inMat)
{
   gal_float wtmp[4][8];
   gal_float m0, m1, m2, m3, s;
   gal_float *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(inMat,0,0), r0[1] = MAT(inMat,0,1),
   r0[2] = MAT(inMat,0,2), r0[3] = MAT(inMat,0,3),
   r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

   r1[0] = MAT(inMat,1,0), r1[1] = MAT(inMat,1,1),
   r1[2] = MAT(inMat,1,2), r1[3] = MAT(inMat,1,3),
   r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

   r2[0] = MAT(inMat,2,0), r2[1] = MAT(inMat,2,1),
   r2[2] = MAT(inMat,2,2), r2[3] = MAT(inMat,2,3),
   r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

   r3[0] = MAT(inMat,3,0), r3[1] = MAT(inMat,3,1),
   r3[2] = MAT(inMat,3,2), r3[3] = MAT(inMat,3,3),
   r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   // choose pivot - or die 
   if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
   if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
   if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
   if (0.0 == r0[0]) return false;

   // eliminate first variable     
   m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
   s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
   s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
   s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r0[5];
   if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r0[6];
   if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r0[7];
   if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

   // choose pivot - or die 
   if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
   if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
   if (0.0 == r1[1]) return false;

   // eliminate second variable 
   m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
   r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
   s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

   // choose pivot - or die 
   if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
   if (0.0 == r2[2]) return false;

   // eliminate third variable 
   m3 = r3[2]/r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
   r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
   r3[7] -= m3 * r2[7];

   // last check 
   if (0.0 == r3[3]) return false;

   s = 1.0F/r3[3];             // now back substitute row 3 
   r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

   m2 = r2[3];                 // now back substitute row 2 
   s  = 1.0F/r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
   r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
   r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
   r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];                 // now back substitute row 1 
   s  = 1.0F/r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
   r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
   r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];                 // now back substitute row 0 
   s  = 1.0F/r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
   r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(outMat,0,0) = r0[4]; MAT(outMat,0,1) = r0[5],
   MAT(outMat,0,2) = r0[6]; MAT(outMat,0,3) = r0[7],
   MAT(outMat,1,0) = r1[4]; MAT(outMat,1,1) = r1[5],
   MAT(outMat,1,2) = r1[6]; MAT(outMat,1,3) = r1[7],
   MAT(outMat,2,0) = r2[4]; MAT(outMat,2,1) = r2[5],
   MAT(outMat,2,2) = r2[6]; MAT(outMat,2,3) = r2[7],
   MAT(outMat,3,0) = r3[4]; MAT(outMat,3,1) = r3[5],
   MAT(outMat,3,2) = r3[6]; MAT(outMat,3,3) = r3[7];
   
   return true;
}

#undef SWAP_ROWS
#undef MAT

