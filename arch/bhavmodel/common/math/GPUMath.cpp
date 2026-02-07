/**************************************************************************
 *
 * GPU Math libary.
 *
 */

#include "GPUMath.h"
#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cg1gpu;

F64 cg1gpu::CG_LOG2(F64 x)
{
    return std::log(x)/std::log(2.0);
}

F64 cg1gpu::CG_CEIL(F64 x)
{
    return std::ceil((double)x);
}

F64 cg1gpu::CG_CEIL2(F64 x)
{
    return ((x - std::floor(x) > 0)?std::floor(x) + 1: std::floor(x));
}

F64 cg1gpu::GPU_FLOOR(F64 x)
{
    return std::floor(x);
}

F64 cg1gpu::GPU_SQRT(F64 x)
{
    return std::sqrt(x);
}

F64 cg1gpu::GPU_POWER2OF(F64 exp)
{
    return std::pow(F64(2.0),exp);
}


U32 GPUMath::ABS(S32 in)
{
    return abs(F32(in));
}

F32 GPUMath::ABS(F32 in)
{
    return fabs(in);
}

void GPUMath::ABS( F32* vinout )
{
    vinout[0] = ( vinout[0] < 0 ? -vinout[0] : vinout[0] );
    vinout[1] = ( vinout[1] < 0 ? -vinout[1] : vinout[1] );
    vinout[2] = ( vinout[2] < 0 ? -vinout[2] : vinout[2] );
    vinout[3] = ( vinout[3] < 0 ? -vinout[3] : vinout[3] );
}


void GPUMath::ABS( const F32* v, F32* result )
{
    result[0] = ( v[0] < 0 ? -v[0] : v[0] );
    result[1] = ( v[1] < 0 ? -v[1] : v[1] );
    result[2] = ( v[2] < 0 ? -v[2] : v[2] );
    result[3] = ( v[3] < 0 ? -v[3] : v[3] );
}


void GPUMath::ADD( const F32* v1, const F32* v2, F32* result )
{
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
    result[3] = v1[3] + v2[3];
}


void GPUMath::ADD( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
    const F32 x2, const F32 y2, const F32 z2, const F32 w2,
    F32* result )
{
    result[0] = x1 + x2;
    result[1] = y1 + y2;
    result[2] = z1 + z2;
    result[3] = w1 + w2;
}


void GPUMath::ARL( S32* addressRegister, F32* in )
{
    addressRegister[0] = (S32) (GPU_FLOOR(in[0]));
    addressRegister[1] = (S32) (GPU_FLOOR(in[1]));
    addressRegister[2] = (S32) (GPU_FLOOR(in[2]));
    addressRegister[3] = (S32) (GPU_FLOOR(in[3]));
}

void GPUMath::CMP( const F32* v1, const F32* v2, const F32* v3, F32* result )
{
    result[0] = (v1[0] < 0.0f)? v2[0] : v3[0];
    result[1] = (v1[1] < 0.0f)? v2[1] : v3[1];
    result[2] = (v1[2] < 0.0f)? v2[2] : v3[2];
    result[3] = (v1[3] < 0.0f)? v2[3] : v3[3];
}

F32 GPUMath::DP3( const F32* v1, const F32* v2, F32* result )
{
    result[0] = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    result[1] = result[0];
    result[2] = result[0];
    result[3] = result[0];

    return result[0];
}


F32 GPUMath::DP4( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
    const F32 x2, const F32 y2, const F32 z2, const F32 w2 )
{
    return ( x1*x2 + y1*y2 + z1*z2 + w1*w2 );
}


F32 GPUMath::DP4( const F32* v1, const F32* v2, F32* result )
{
    result[0] = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2] + v1[3]*v2[3];
    result[1] = result[0];
    result[2] = result[0];
    result[3] = result[0];

    return result[0];
}


F32 GPUMath::DPH( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
    const F32 x2, const F32 y2, const F32 z2, const F32 w2 )
{
    return ( x1*x2 + y1*y2 + z1*z2 + w2 );
}


F32 GPUMath::DPH( const F32* v1, const F32* v2, F32* result )
{
    result[0] = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2] + v2[3];
    result[1] = result[0];
    result[2] = result[0];
    result[3] = result[0];

    return result[0];
}


void GPUMath::DST( const F32* v1, const F32* v2, F32* result )
{
    result[0] = 1.0;
    result[1] = v1[1] * v2[1];
    result[2] = v1[2];
    result[3] = v2[3];
}


void GPUMath::MAD( const F32* v1, const F32* v2, const F32* v3, F32* result )
{
    result[0] = v1[0] * v2[0] + v3[0];
    result[1] = v1[1] * v2[1] + v3[1];
    result[2] = v1[2] * v2[2] + v3[2];
    result[3] = v1[3] * v2[3] + v3[3];
}


void GPUMath::MAD( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
    const F32 x2, const F32 y2, const F32 z2, const F32 w2,
    const F32 x3, const F32 y3, const F32 z3, const F32 w3,
    F32* result )
{
    result[0] = x1 * x2 + x3;
    result[1] = y1 * y2 + y3;
    result[2] = z1 * z2 + z3;
    result[3] = w1 * w2 + w3;
}


void GPUMath::MAX( const F32* v1, const F32* v2, F32* result )
{
    result[0] = ( v1[0] > v2[0] ? v1[0] : v2[0] );
    result[1] = ( v1[1] > v2[1] ? v1[1] : v2[1] );
    result[2] = ( v1[2] > v2[2] ? v1[2] : v2[2] );
    result[3] = ( v1[3] > v2[3] ? v1[3] : v2[3] );
}


void GPUMath::MAX( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
   const F32 x2, const F32 y2, const F32 z2, const F32 w2,
   F32* result )
{
    result[0] = ( x1 > x2 ? x1 : x2 );
    result[1] = ( y1 > y2 ? y1 : y2 );
    result[2] = ( z1 > z2 ? z1 : z2 );
    result[3] = ( w1 > w2 ? w1 : w2 );
}


void GPUMath::MIN( const F32* v1, const F32* v2, F32* result )
{
    result[0] = ( v1[0] < v2[0] ? v1[0] : v2[0] );
    result[1] = ( v1[1] < v2[1] ? v1[1] : v2[1] );
    result[2] = ( v1[2] < v2[2] ? v1[2] : v2[2] );
    result[3] = ( v1[3] < v2[3] ? v1[3] : v2[3] );
}


void GPUMath::MIN( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
   const F32 x2, const F32 y2, const F32 z2, const F32 w2,
   F32* result )
{
    result[0] = ( x1 < x2 ? x1 : x2 );
    result[1] = ( y1 < y2 ? y1 : y2 );
    result[2] = ( z1 < z2 ? z1 : z2 );
    result[3] = ( w1 < w2 ? w1 : w2 );
}

void GPUMath::MOV( F32* source, F32* destination )
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    destination[3] = source[3];
}

void GPUMath::MUL( const F32* v1, const F32* v2, F32* result )
{
    result[0] = v1[0] * v2[0];
    result[1] = v1[1] * v2[1];
    result[2] = v1[2] * v2[2];
    result[3] = v1[3] * v2[3];
}


void GPUMath::MUL( const F32 x1, const F32 y1, const F32 z1, const F32 w1,
    const F32 x2, const F32 y2, const F32 z2, const F32 w2,
    F32* result )
{
    result[0] = x1 * x2;
    result[1] = y1 * y2;
    result[2] = z1 * z2;
    result[3] = w1 * w2;
}

F32 GPUMath::RCP( const F32 vin, F32* vout )
{
    vout[0] = 1 / vin;
    vout[1] = vout[0];
    vout[2] = vout[0];
    vout[3] = vout[0];
    return vout[0];
}

F32 GPUMath::RCP( F32 value )
{
    return ( 1 / value );
}


F32 GPUMath::RSQ( const F32 vin, F32* vout )
{
    vout[0] = static_cast<F32>(GPU_SQRT( 1 / ( vin < 0 ? -vin : vin ) ));
    vout[1] = vout[0];
    vout[2] = vout[0];
    vout[3] = vout[0];
    return vout[0];
}


F32 GPUMath::RSQ( F32 value )
{
    return static_cast<F32>(GPU_SQRT( 1 / ( value < 0 ? -value : value ) ));
}


void GPUMath::FRC( F32* vin, F32* vout )
{
    vout[0] = vin[0] - static_cast<F32>(GPU_FLOOR(vin[0]));
    vout[1] = vin[1] - static_cast<F32>(GPU_FLOOR(vin[1]));
    vout[2] = vin[2] - static_cast<F32>(GPU_FLOOR(vin[2]));
    vout[3] = vin[3] - static_cast<F32>(GPU_FLOOR(vin[3]));
}


void GPUMath::SAT( F32* vin1, F32* vout )
{
    vout[0] = GPU_CLAMP(vin1[0], 0.0f, 1.0f);
    vout[1] = GPU_CLAMP(vin1[1], 0.0f, 1.0f);
    vout[2] = GPU_CLAMP(vin1[2], 0.0f, 1.0f);
    vout[3] = GPU_CLAMP(vin1[3], 0.0f, 1.0f);
}

void GPUMath::SLT( F32* vin1, F32* vin2, F32* vout )
{
    vout[0] = ( vin1[0] < vin2[0] ? 1.0f : 0.0f );
    vout[1] = ( vin1[1] < vin2[1] ? 1.0f : 0.0f );
    vout[2] = ( vin1[2] < vin2[2] ? 1.0f : 0.0f );
    vout[3] = ( vin1[3] < vin2[3] ? 1.0f : 0.0f );
}


void GPUMath::SGE( F32* vin1, F32* vin2, F32* vout )
{
    vout[0] = ( vin1[0] < vin2[0] ? 0.0f : 1.0f );
    vout[1] = ( vin1[1] < vin2[1] ? 0.0f : 1.0f );
    vout[2] = ( vin1[2] < vin2[2] ? 0.0f : 1.0f );
    vout[3] = ( vin1[3] < vin2[3] ? 0.0f : 1.0f );
}

void GPUMath::EXP( F32 vin, F32* vout )
{
    F32 floorOfComponent = static_cast<F32>(GPU_FLOOR( vin ));
    vout[0] = static_cast<F32>(GPU_POWER2OF( floorOfComponent ));
    vout[1] = vin - floorOfComponent;
    // It is an aproximation in hardware implementation ( Accurate to 11 bit )
    vout[2] = static_cast<F32>(GPU_POWER2OF( vin ));
    vout[3] = 1;
}

void GPUMath::EX2( F32 vin, F32* vout )
{
    vout[0] = static_cast<F32>(GPU_POWER2OF( vin ));
    vout[1] = static_cast<F32>(GPU_POWER2OF( vin ));
    vout[2] = static_cast<F32>(GPU_POWER2OF( vin ));
    vout[3] = static_cast<F32>(GPU_POWER2OF( vin ));
}

void GPUMath::LOG( F32 vin, F32* vout )
{
    vout[0] = static_cast<F32>(GPU_FLOOR(CG_LOG2(GPU_ABS( vin ))));
    vout[1] = vin / static_cast<F32>(GPU_POWER2OF(GPU_FLOOR( CG_LOG2( vin ))));
    // In hardware this is an aproximation good for 11 bits
    vout[2] = static_cast<F32>(CG_LOG2(vin));
    vout[3] = 1;
}

void GPUMath::LG2( F32 vin, F32* vout )
{
    vout[0] = static_cast<F32>(CG_LOG2( vin ));
    vout[1] = static_cast<F32>(CG_LOG2( vin ));
    vout[2] = static_cast<F32>(CG_LOG2( vin ));
    vout[3] = static_cast<F32>(CG_LOG2( vin ));
}

void GPUMath::LIT( F32* vin, F32* vout )
{
    float x, y, w;

    x = (vin[0] < 0.0f) ? 0.0f: vin[0];
    y = (vin[1] < 0.0f) ? 0.0f: vin[1];
    w = GPU_CLAMP(vin[3], -128.0f, 128.0f);

    vout[0] = 1.0f;
    vout[1] = x;
    vout[2] = (x > 0.0f) ? static_cast<F32>(GPU_POWER(y, w)) : 0.0f;
    vout[3] = 1.0f;

}

void GPUMath::SETPEQ(F32 a, F32 b, bool &out)
{
    out = (a == b);
}

void GPUMath::SETPGT(F32 a, F32 b, bool &out)
{
    out = (a > b);
}

void GPUMath::SETPLT(F32 a, F32 b, bool &out)
{
    out = (a < b);
}

void GPUMath::ANDP(bool a, bool b, bool &out)
{
    out = (a && b);
}

void GPUMath::COS( F32 vin, F32* vout )
{
    vout[0] = 
    vout[1] = 
    vout[2] = 
    vout[3] = static_cast<F32>(cos(vin));
}

void GPUMath::SIN( F32 vin, F32* vout )
{
    vout[0] = 
    vout[1] = 
    vout[2] = 
    vout[3] = static_cast<F32>(sin(vin));
}

void GPUMath::derivX(Vec4FP32 *input, Vec4FP32 *derivatives)
{
    for(U32 c = 0; c < 4; c++)
    {
        derivatives[0][c] =
        derivatives[1][c] = input[1][c] - input[0][c];
        derivatives[2][c] =
        derivatives[3][c] = input[3][c] - input[2][c];
    }
}

void GPUMath::derivY(Vec4FP32 *input, Vec4FP32 *derivatives)
{
    for(U32 c = 0; c < 4; c++)
    {
        derivatives[0][c] =
        derivatives[2][c] = input[2][c] - input[0][c];
        derivatives[1][c] =
        derivatives[3][c] = input[3][c] - input[1][c];
    }
}

void GPUMath::ADDI(const S32 *v1, const S32 *v2, S32 *result)
{
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
    result[3] = v1[3] + v2[3];
}

void GPUMath::MULI(const S32 *v1, const S32 *v2, S32 *result)
{
    result[0] = v1[0] * v2[0];
    result[1] = v1[1] * v2[1];
    result[2] = v1[2] * v2[2];
    result[3] = v1[3] * v2[3];
}

void GPUMath::STPEQI(S32 a, S32 b, bool &out)
{
    out = (a == b);
}

void GPUMath::STPGTI(S32 a, S32 b, bool &out)
{
    out = (a > b);
}

void GPUMath::STPLTI(S32 a, S32 b, bool &out)
{
    out = (a < b);
}

//  Calculates the triangle setup matrix.  
void GPUMath::setupMatrix(Vec4FP32 vertex1, Vec4FP32 vertex2,
    Vec4FP32 vertex3, F64 *edge1, F64 *edge2, F64 *edge3)
{
    //F64 det;

    /*  Calculate the adjoint matrix for the [X Y W] matrix (stored
        as rows) (three vertices 2DH coordinate matrix) and store it
        in A, B and C vectors (adjoint matrix columns).  */

    /*  The adjoint matrix is calculated this way
        (using a shader program):

          mul rC.xyz, rX.zxy, rY.yzx
          mul rB.xyz, rX.yzx, rW.zxy
          mul rA.xyz, rY.zxy, rW.yzx
          mad rC.xyz, rX.yzx, rY.zxy, -rC
          mad rB.xyz, rX.zxy, rW.yzx, -rB
          mad rA.xyz, rY.yzx, rW.zxy, -rA

     */

    /*  Calculate the adjoint vertex matrix (setup matrix) and
        store it as the triangle edge equations.

        Vertex Matrix M:

            M = [ w0x0 w1x1 w2x2 ]
                [ w0y0 w1y1 w2y2 ]
                [  w0   w1   w2  ]

        Adjoint Matrix (Setup Matrix): A

            A = adj(M) = [ a0 b0 c0]
                        [ a1 b1 c1]
                        [ a2 b2 c2]


            a0 = w1y1 * w2 - w2y2 * w1
            a1 = w2y2 * w0 - w0y0 * w0
            a2 = w0y0 * w1 - w1y1 * w0
            b0 = w2x2 * w1 - w1x1 * w2
            b1 = w0x0 * w2 - w2x2 * w0
            b2 = w1x1 * w0 - w0x0 * w1
            c0 = w1x1 * w2y2 - w2x2 * w1y1
            c1 = w2x2 * w0y0 - w0x0 - w2y2
            c2 = w0x0 * w1y1 - w1x1 * w0y0

        Vertex Matrix Inverse:

            inv(M) = adj(M)/det(M)

        Vertex Matrix Determinant:

            det(M) = c0 * w0 + c1 * w1 + c2 * w2;

        Edge equations:

            E0 = [a0, b0, c0]
            E1 = [a1, b1, c1]
            E2 = [a2, b2, c2]

    */

    //  Calculate first edge equation.
    edge1[0] = F64(vertex2[1]) * F64(vertex3[3]) - F64(vertex3[1]) * F64(vertex2[3]);
    edge1[1] = F64(vertex3[0]) * F64(vertex2[3]) - F64(vertex2[0]) * F64(vertex3[3]);
    edge1[2] = F64(vertex2[0]) * F64(vertex3[1]) - F64(vertex3[0]) * F64(vertex2[1]);

    //  Calculate second edge equation.
    edge2[0] = F64(vertex3[1]) * F64(vertex1[3]) - F64(vertex1[1]) * F64(vertex3[3]);
    edge2[1] = F64(vertex1[0]) * F64(vertex3[3]) - F64(vertex3[0]) * F64(vertex1[3]);
    edge2[2] = F64(vertex3[0]) * F64(vertex1[1]) - F64(vertex1[0]) * F64(vertex3[1]);

    //  Calculate third edge equation.
    edge3[0] = F64(vertex1[1]) * F64(vertex2[3]) - F64(vertex2[1]) * F64(vertex1[3]);
    edge3[1] = F64(vertex2[0]) * F64(vertex1[3]) - F64(vertex1[0]) * F64(vertex2[3]);
    edge3[2] = F64(vertex1[0]) * F64(vertex2[1]) - F64(vertex2[0]) * F64(vertex1[1]);

    //  Calculate the vertex matrix determinant.  
    //det = edge1[2] * vertex1[3] + edge2[2] * vertex2[3] +
    //    edge3[2] * vertex3[3];

    //  Divide matrix by the matrix determinant.  
    //edge1[0] = edge1[0] / det;
    //edge1[1] = edge1[1] / det;
    //edge1[2] = edge1[2] / det;

    //edge2[0] = edge2[0] / det;
    //edge2[1] = edge2[1] / det;
    //edge2[2] = edge2[2] / det;

    //edge3[0] = edge3[0] / det;
    //edge3[1] = edge3[1] / det;
    //edge3[2] = edge3[2] / det;
}

/*  Adjusts the triangle edge equations and z interpolation equation to
    the viewport.  */
F64 GPUMath::viewport(Vec4FP32 vertex1, Vec4FP32 vertex2,
    Vec4FP32 vertex3, F64 *edge1, F64 *edge2, F64 *edge3,
    F64 *zeq, S32 x0, S32 y0, U32 w, U32 h)
{
    F64 area;

    /*  Adjust the three edge equations to the viewport:

        a' = a * 2/w
        b' = b * 2/h
        c' = c - a * ((x0 * 2)/w + 1) - b * ((y0 * 2)/h + 1)

     */

    //  Calculate triangle area (or an approximation).
    area = edge1[2] * F64(vertex1[3]) + edge2[2] * F64(vertex2[3]) +
        edge3[2] * F64(vertex3[3]);

    //  Adjust the first edge equation.
    edge1[2] = edge1[2] - edge1[0] * ((F64(x0) * 2.0f) / F64(w) + 1.0f) -
        edge1[1] * ((F64(y0) * 2.0f) / F64(h) + 1.0f);
    edge1[0] = edge1[0] * 2.0f / F64(w);
    edge1[1] = edge1[1] * 2.0f / F64(h);

    //  Adjust the second edge equation.
    edge2[2] = edge2[2] - edge2[0] * ((F64(x0) * 2.0f) / F64(w) + 1.0f) -
        edge2[1] * ((F64(y0) * 2.0f) / F64(h) + 1.0f);
    edge2[0] = edge2[0] * 2.0f / F64(w);
    edge2[1] = edge2[1] * 2.0f / F64(h);

    //  Adjust the third edge equation.
    edge3[2] = edge3[2] - edge3[0] * ((F64(x0) * 2.0f) / F64(w) + 1.0f) -
        edge3[1] * ((F64(y0) * 2.0f) / F64(h) + 1.0f);
    edge3[0] = edge3[0] * 2.0f / F64(w);
    edge3[1] = edge3[1] * 2.0f / F64(h);

    //  Adjust the z interpolation equation.
    zeq[2] = zeq[2] - zeq[0] * ((F64(x0) * 2.0f) / F64(w) + 1.0f) -
        zeq[1] * ((F64(y0) * 2.0f) / F64(h) + 1.0f);
    zeq[0] = zeq[0] * 2.0f / F64(w);
    zeq[1] = zeq[1] * 2.0f / F64(h);

    //  Normalize Z with the calculated area.
    zeq[0] = zeq[0] / area;
    zeq[1] = zeq[1] / area;
    zeq[2] = zeq[2] / area;

    //  Return the vertex matrix determinant.
    return area;
}

//  Evaluates an edge equation against a 2D point.  
F64 GPUMath::evaluateEquation(F64 *e, F64 *p)
{
    return e[0] * p[0] + e[1] * p[1] + e[2];
}

/*  Returns if a point is inside the triangle defined by the three
    given edge equations.  */
bool GPUMath::isInside(F64 *e1, F64 *e2, F64 *e3, F64 *p)
{
    F64 et1, et2, et3;

    //  Evaluate edge equations at the point at get 1/w.  
    et1 = evaluateEquation(e1, p);
    et2 = evaluateEquation(e2, p);
    et3 = evaluateEquation(e3, p);

    //  Evaluate if the point is a corner of the rasterized triangle.  
    return ((GPU_IS_ZERO(et1) || GPU_IS_POSITIVE(et1)) &&
        (GPU_IS_ZERO(et2) || GPU_IS_POSITIVE(et2)) &&
        (GPU_IS_ZERO(et3) || GPU_IS_POSITIVE(et3)));

}

/*  Calculate bounding mdu from triangle homogeneous coordinates.

    Source: Blinn - Calculating Screen Coverage).

    XL = - 1, XR = 1
    YB = - 1, YT = 1
    ZN = - 1, ZF = 1

 */

void GPUMath::boundingBox(Vec4FP32 v1, Vec4FP32 v2, Vec4FP32 v3,
    S32 x0, S32 y0, U32 width, U32 height, S32 &xMin, S32 &xMax,
    S32 &yMin, S32 &yMax, S32 &zMin, S32 &zMax)
{
    U32 outcodes[3];
    bool anyVis[3];
    U32 accept;
    U32 reject;
    F32 minX, maxX, minY, maxY, minZ, maxZ;

#define BOUNDCODES(c, w, a, b, co0, co1)    \
    (((((c) - (a) * (w)) < 0.0f)?(co0):0x00) | (((-(c) + (b) * (w)) < 0.0f)?(co1):0x00))

    //  Calculate the horizontal outcodes for the three vertices.  
    outcodes[0] = BOUNDCODES(v1[0], v1[3], -1.0f, 1.0f, 0x01, 0x02);
    outcodes[1] = BOUNDCODES(v2[0], v2[3], -1.0f, 1.0f, 0x01, 0x02);
    outcodes[2] = BOUNDCODES(v3[0], v3[3], -1.0f, 1.0f, 0x01, 0x02);

    //  Calculate the vertical outcodes for the three vertices.  
    outcodes[0] |= BOUNDCODES(v1[1], v1[3], -1.0f, 1.0f, 0x04, 0x08);
    outcodes[1] |= BOUNDCODES(v2[1], v2[3], -1.0f, 1.0f, 0x04, 0x08);
    outcodes[2] |= BOUNDCODES(v3[1], v3[3], -1.0f, 1.0f, 0x04, 0x08);

    //  Calculate the depth outcodes for the three vertices.  
    outcodes[0] |= BOUNDCODES(v1[2], v1[3], -1.0f, 1.0f, 0x10, 0x20);
    outcodes[1] |= BOUNDCODES(v2[2], v2[3], -1.0f, 1.0f, 0x10, 0x20);
    outcodes[2] |= BOUNDCODES(v3[2], v3[3], -1.0f, 1.0f, 0x10, 0x20);

#undef BOUNDCODES

    //  Calculate trivial acception.  
    accept = outcodes[0] | outcodes[1] | outcodes[2];

    //  Calculate trivial rejection.  
    reject = outcodes[0] & outcodes[1] & outcodes[2];

//printf("outcodes[0] = %04x outcodes[1] = %04x outcodes[2] = %04x\n",
//outcodes[0], outcodes[1], outcodes[2]);

    //  Check trivial rejection.  
    CG_ASSERT_COND(!(reject != 0), "Triangle is trivially rejected (shouldn't happen here!).");
    //  Set initial bounding mdu limits.  
    minX = minY = minZ = 1.0f;
    maxX = maxY = maxZ = -1.0f;

    //  Set visibility flags.  
    anyVis[0] = anyVis[1] = anyVis[2] = FALSE;

#define BOUNDRANGE(outcode, mask, c, w, min, max, vis)\
    if (((outcode) & (mask)) == 0)                  \
    {                                               \
        vis = TRUE;                                 \
                                                    \
        if (((c) - (min) * (w)) < 0.0f)             \
            (min) = (c) / (w);                      \
        if (((c) - (max) * (w)) > 0.0f)             \
            (max) = (c) / (w);                      \
    }

    //  Calculate horizontal range.  
    BOUNDRANGE(outcodes[0], 0x03, v1[0], v1[3], minX, maxX, anyVis[0])
    BOUNDRANGE(outcodes[1], 0x03, v2[0], v2[3], minX, maxX, anyVis[0])
    BOUNDRANGE(outcodes[2], 0x03, v3[0], v3[3], minX, maxX, anyVis[0])

    //  Calculate vertical range.  
    BOUNDRANGE(outcodes[0], 0x0c, v1[1], v1[3], minY, maxY, anyVis[1])
    BOUNDRANGE(outcodes[1], 0x0c, v2[1], v2[3], minY, maxY, anyVis[1])
    BOUNDRANGE(outcodes[2], 0x0c, v3[1], v3[3], minY, maxY, anyVis[1])

    //  Calculate depth range.  
    BOUNDRANGE(outcodes[0], 0x30, v1[2], v1[3], minZ, maxZ, anyVis[2])
    BOUNDRANGE(outcodes[1], 0x30, v2[2], v2[3], minZ, maxZ, anyVis[2])
    BOUNDRANGE(outcodes[2], 0x30, v3[2], v3[3], minZ, maxZ, anyVis[2])

#undef BOUNDRANGE

//printf("First pass >> %f %f %f %f\n", minX, maxX, minY, maxY);

#define BOUNDRANGE2(outcode, mask0, mask1, c, w, min, max, a, b)        \
    if ((((outcode) & (mask0)) != 0) && (((c) - (min) * (w)) < 0.0f))   \
        (min) = (a);                                                    \
    if ((((outcode) & (mask1)) != 0) && (((c) - (max) * (w)) > 0.0f))   \
        (max) = (b);

    //  Check if all the vertices are visible (trivial accept).  
    if (accept != 0)
    {
        //  No trivial accept, second pass required.  

        //  Reset boundaries if none of the triangle points are visible.  
        if (!anyVis[0])
        {
            minX = -1.0f;
            maxX = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x01, 0x02, v1[0], v1[3], minX, maxX, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x01, 0x02, v2[0], v2[3], minX, maxX, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x01, 0x02, v3[0], v3[3], minX, maxX, -1.0f, 1.0f);
        }

        if (!anyVis[1])
        {
            minY = -1.0f;
            maxY = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x04, 0x08, v1[1], v1[3], minY, maxY, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x04, 0x08, v2[1], v2[3], minY, maxY, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x04, 0x08, v3[1], v3[3], minY, maxY, -1.0f, 1.0f);
        }

        if (!anyVis[2])
        {
            minZ = -1.0f;
            maxZ = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x10, 0x20, v1[2], v1[3], minZ, maxZ, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x10, 0x20, v2[2], v2[3], minZ, maxZ, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x10, 0x20, v3[2], v3[3], minZ, maxZ, -1.0f, 1.0f);
        }
    }

#undef BOUNDRANGE2

//printf("Second pass >> %f %f %f %f\n", minX, maxX, minY, maxY);

    //  Convert to viewport coordinates.  
    xMin = GPU_MAX(S32(GPU_FLOOR(minX * (F32(width) * 0.5f) + F32(x0) + (F32(width) * 0.5f))) - 1, S32(x0));
    xMax = GPU_MIN(S32(GPU_FLOOR(maxX * (F32(width) * 0.5f) + F32(x0) + (F32(width) * 0.5f))) + 1, S32(x0 + width));
    yMin = GPU_MAX(S32(GPU_FLOOR(minY * (F32(height) * 0.5f) + F32(y0) + (F32(height) * 0.5f))) - 1, S32(y0));
    yMax = GPU_MIN(S32(GPU_FLOOR(maxY * (F32(height) * 0.5f) + F32(y0) + (F32(height) * 0.5f))) + 1, S32(y0 + height));
//printf("Viewport coords: %d %d %d %d\n", xMin, xMax, yMin, yMax);
}

void GPUMath::subPixelBoundingBox(Vec4FP32 v1, Vec4FP32 v2, Vec4FP32 v3,
    S32 x0, S32 y0, U32 width, U32 height, FixedPoint &xMin, FixedPoint &xMax,
    FixedPoint &yMin, FixedPoint &yMax, S32 &zMin, S32 &zMax, U32 subPixelPrecision)
{
    U32 outcodes[3];
    bool anyVis[3];
    U32 accept;
    U32 reject;
    F32 minX, maxX, minY, maxY, minZ, maxZ;
    F32 widthDiv2, heightDiv2;
    U32 subPixelScaleMask;
    U32 subPixelScaleRange;

#define BOUNDCODES(c, w, a, b, co0, co1)    \
    (((((c) - (a) * (w)) < 0.0f)?(co0):0x00) | (((-(c) + (b) * (w)) < 0.0f)?(co1):0x00))

    //  Calculate the horizontal outcodes for the three vertices.  
    outcodes[0] = BOUNDCODES(v1[0], v1[3], -1.0f, 1.0f, 0x01, 0x02);
    outcodes[1] = BOUNDCODES(v2[0], v2[3], -1.0f, 1.0f, 0x01, 0x02);
    outcodes[2] = BOUNDCODES(v3[0], v3[3], -1.0f, 1.0f, 0x01, 0x02);

    //  Calculate the vertical outcodes for the three vertices.  
    outcodes[0] |= BOUNDCODES(v1[1], v1[3], -1.0f, 1.0f, 0x04, 0x08);
    outcodes[1] |= BOUNDCODES(v2[1], v2[3], -1.0f, 1.0f, 0x04, 0x08);
    outcodes[2] |= BOUNDCODES(v3[1], v3[3], -1.0f, 1.0f, 0x04, 0x08);

    //  Calculate the depth outcodes for the three vertices.  
    outcodes[0] |= BOUNDCODES(v1[2], v1[3], -1.0f, 1.0f, 0x10, 0x20);
    outcodes[1] |= BOUNDCODES(v2[2], v2[3], -1.0f, 1.0f, 0x10, 0x20);
    outcodes[2] |= BOUNDCODES(v3[2], v3[3], -1.0f, 1.0f, 0x10, 0x20);

#undef BOUNDCODES

    //  Calculate trivial acception.  
    accept = outcodes[0] | outcodes[1] | outcodes[2];

    //  Calculate trivial rejection.  
    reject = outcodes[0] & outcodes[1] & outcodes[2];

    //  Check trivial rejection.  
    CG_ASSERT_COND(!(reject != 0), "Triangle is trivially rejected (shouldn't happen here!).");
    //  Set initial bounding mdu limits.  
    minX = minY = minZ = 1.0f;
    maxX = maxY = maxZ = -1.0f;

    //  Set visibility flags.  
    anyVis[0] = anyVis[1] = anyVis[2] = FALSE;

#define BOUNDRANGE(outcode, mask, c, w, min, max, vis)\
    if (((outcode) & (mask)) == 0)                  \
    {                                               \
        vis = TRUE;                                 \
                                                    \
        if (((c) - (min) * (w)) < 0.0f)             \
            (min) = (c) / (w);                      \
        if (((c) - (max) * (w)) > 0.0f)             \
            (max) = (c) / (w);                      \
    }

    //  Calculate horizontal range.  
    BOUNDRANGE(outcodes[0], 0x03, v1[0], v1[3], minX, maxX, anyVis[0])
    BOUNDRANGE(outcodes[1], 0x03, v2[0], v2[3], minX, maxX, anyVis[0])
    BOUNDRANGE(outcodes[2], 0x03, v3[0], v3[3], minX, maxX, anyVis[0])

    //  Calculate vertical range.  
    BOUNDRANGE(outcodes[0], 0x0c, v1[1], v1[3], minY, maxY, anyVis[1])
    BOUNDRANGE(outcodes[1], 0x0c, v2[1], v2[3], minY, maxY, anyVis[1])
    BOUNDRANGE(outcodes[2], 0x0c, v3[1], v3[3], minY, maxY, anyVis[1])

    //  Calculate depth range.  
    BOUNDRANGE(outcodes[0], 0x30, v1[2], v1[3], minZ, maxZ, anyVis[2])
    BOUNDRANGE(outcodes[1], 0x30, v2[2], v2[3], minZ, maxZ, anyVis[2])
    BOUNDRANGE(outcodes[2], 0x30, v3[2], v3[3], minZ, maxZ, anyVis[2])

#undef BOUNDRANGE

//printf("First pass >> %f %f %f %f\n", minX, maxX, minY, maxY);

#define BOUNDRANGE2(outcode, mask0, mask1, c, w, min, max, a, b)        \
    if ((((outcode) & (mask0)) != 0) && (((c) - (min) * (w)) < 0.0f))   \
        (min) = (a);                                                    \
    if ((((outcode) & (mask1)) != 0) && (((c) - (max) * (w)) > 0.0f))   \
        (max) = (b);

    //  Check if all the vertices are visible (trivial accept).  
    if (accept != 0)
    {
        //  No trivial accept, second pass required.  

        //  Reset boundaries if none of the triangle points are visible.  
        if (!anyVis[0])
        {
            minX = -1.0f;
            maxX = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x01, 0x02, v1[0], v1[3], minX, maxX, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x01, 0x02, v2[0], v2[3], minX, maxX, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x01, 0x02, v3[0], v3[3], minX, maxX, -1.0f, 1.0f);
        }

        if (!anyVis[1])
        {
            minY = -1.0f;
            maxY = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x04, 0x08, v1[1], v1[3], minY, maxY, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x04, 0x08, v2[1], v2[3], minY, maxY, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x04, 0x08, v3[1], v3[3], minY, maxY, -1.0f, 1.0f);
        }

        if (!anyVis[2])
        {
            minZ = -1.0f;
            maxZ = 1.0f;
        }
        else
        {
            //  Second pass.  
            BOUNDRANGE2(outcodes[0], 0x10, 0x20, v1[2], v1[3], minZ, maxZ, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[1], 0x10, 0x20, v2[2], v2[3], minZ, maxZ, -1.0f, 1.0f);
            BOUNDRANGE2(outcodes[2], 0x10, 0x20, v3[2], v3[3], minZ, maxZ, -1.0f, 1.0f);
        }
    }

#undef BOUNDRANGE2

    widthDiv2 = F32(width) * 0.5f;
    heightDiv2 = F32(height) * 0.5f;

    //  Compute unbound floating point viewport coordinates.  
    minX = (minX + 1.0f) * widthDiv2 + F32(x0);
    maxX = (maxX + 1.0f) * widthDiv2 + F32(x0);
    minY = (minY + 1.0f) * heightDiv2 + F32(y0);
    maxY = (maxY + 1.0f) * heightDiv2 + F32(y0);

    xMin = FixedPoint(minX, 16, subPixelPrecision);
    xMax = FixedPoint(maxX, 16, subPixelPrecision);
    yMin = FixedPoint(minY, 16, subPixelPrecision);
    yMax = FixedPoint(maxY, 16, subPixelPrecision);

//printf("Float point viewport BB coordinates: (%f,%f)->(%f,%f)\n", minX, minY, maxX, maxY); 

/*    
    //  Compute integer part of fixed-point precision subpixel viewport coordinates.
    xMin = GPU_MAX(S32(GPU_FLOOR(minX)), S32(x0)) << subPixelScaleFactor;
    xMax = GPU_MIN(S32(GPU_FLOOR(maxX)), S32(x0 + width)) << subPixelScaleFactor;
    yMin = GPU_MAX(S32(GPU_FLOOR(minY)), S32(y0)) << subPixelScaleFactor;
    yMax = GPU_MIN(S32(GPU_FLOOR(maxY)), S32(y0 + height)) << subPixelScaleFactor;

    subPixelScaleRange = ( 0x01 << subPixelScaleFactor );
    subPixelScaleMask = ( 0x01 << subPixelScaleFactor ) - 1;

    xMin = xMin | U32(GPU_FLOOR((minX - GPU_FLOOR(minX)) * F32(subPixelScaleRange)));
    xMax = xMax | U32(CG_CEIL((maxX - GPU_FLOOR(maxX)) * F32(subPixelScaleRange)));
    yMin = yMin | U32(GPU_FLOOR((minY - GPU_FLOOR(minY)) * F32(subPixelScaleRange)));
    yMax = yMax | U32(CG_CEIL((maxY - GPU_FLOOR(maxY)) * F32(subPixelScaleRange)));

//printf("SubPixel BB: (%f, %f)->(%f, %f)\n", F32(xMin >> subPixelScaleFactor) + F32(xMin & subPixelScaleMask)/F32(subPixelScaleRange),
//                                            F32(yMin >> subPixelScaleFactor) + F32(yMin & subPixelScaleMask)/F32(subPixelScaleRange),
//                                            F32(xMax >> subPixelScaleFactor) + F32(xMax & subPixelScaleMask)/F32(subPixelScaleRange),
//                                            F32(yMax >> subPixelScaleFactor) + F32(yMax & subPixelScaleMask)/F32(subPixelScaleRange));
*/
}

/*  Calculates the rasterization start position for a triangle from
    its three edge equations (top right most position).  */
void GPUMath::startPosition(
    Vec4FP32 vertex1, Vec4FP32 vertex2, Vec4FP32 vertex3,
    F64 *e1, F64 *e2, F64 *e3,
    S32 x0, S32 y0, U32 w, U32 h,
    S32 scissorX0, S32 scissorY0, U32 scissorW, U32 scissorH,
    S32 boxMinX, S32 boxMinY, S32 boxMaxX, S32 boxMaxY,
    S32 &startX, S32 &startY, RasterDirection &dir)
{
    Vec4FP32 vp[3];
    U32 topVertexId;
    Vec4FP32 topVertex;
    bool corner[4];
    F64 p[2];

    /*
        First check if the three vertices have positive w.

        If all the vertices have a positive w then project them and
        find the top right most vertex.  Use DOWN rasterization
        start direction for the traversal algorithm.

        If any of the vertices has a negative or zero w then test
        if any of the triangle bounding mdu corners are inside the triangle.

        If no corners are inside the triangle project one of the
        non vertices with non negative w and choice it as start
        rasterization position.  Use CENTER rasterization start direction
        for the traversal algorithm.

        If any of the corners is inside the triangle set DOWN rasterization
        start direction if any of the top corners are inside, choice
        top right versus top left as start position if both are inside.
        If none of the top corners are inside choice one of the down corners,
        down right over down left and use TOP rasterization start direction
        for the traversal algorithm.

     */

    //  Check if the bounding mdu has been clamped to the scissor mdu.  
    if ((boxMinX == scissorX0) || (boxMinY == scissorY0) ||
        (boxMaxX == (scissorX0 + S32(scissorW))) || (boxMaxY == (scissorY0 + S32(scissorH))))
    {
        //  Check if the bounding mdu center is inside the triangle.  
        p[0] = F64(boxMinX + (boxMaxX - boxMinX)/2); p[1] = F64(boxMinY + (boxMaxY - boxMinY)/2);

        if (isInside(e1, e2, e3, p))
        {
            startX = boxMinX + (boxMaxX - boxMinX)/2;
            startY = boxMinY + (boxMaxY - boxMinY)/2;
            dir = CENTER_DIR;

            return;
        }
        else
        {
            //  Check if bounding mdu corners are inside the triangle.   

            //  Bottom left corner.  
            p[0] = F64(boxMinX); p[1] = F64(boxMinY);
            corner[0] = isInside(e1, e2, e3, p);

            //  Bottom left corner.  
            p[0] = F64(boxMinX); p[1] = F64(boxMinY);
            corner[0] = isInside(e1, e2, e3, p);

            //  Bottom right corner.  
            p[0] = F64(boxMaxX - 1); p[1] = F64(boxMinY);
            corner[1] = isInside(e1, e2, e3, p);

            //  Top left corner.  
            p[0] = F64(boxMinX); p[1] = F64(boxMaxY - 1);
            corner[2] = isInside(e1, e2, e3, p);

            //  Top right corner.  
            p[0] = F64(boxMaxX - 1); p[1] = F64(boxMaxY - 1);
            corner[3] = isInside(e1, e2, e3, p);

            //  Check if any of the triangle corners corner is inside the triangle.  
            if (corner[0] || corner[1] || corner[2] || corner[3])
            {
                //  Select one of the corners as the start vertex.  
                if (corner[3])
                {
                    //  Top right selected.  
                    startX = boxMaxX - 1;
                    startY = boxMaxY - 1;

                    //  Set rasterization direction to down.  
                    dir = DOWN_DIR;
                }
                else if (corner[2])
                {
                    //  Top left corner selected.  
                    startX = boxMinX;
                    startY = boxMaxY - 1;

                    //  Set rasterization direction to down.  
                    dir = DOWN_DIR;
                }
                else if (corner[1])
                {
                    //  Bottom right corner selected.  
                    startX = boxMaxX - 1;
                    startY = boxMinY;

                    //  Set rasterization direction to up.  
                    dir = UP_DIR;
                }
                else
                {
                    //  Bottom left corner selected.  
                    startX = boxMinX;
                    startY = boxMinY;

                    //  Set rasterization direction to up.  
                    dir = UP_DIR;
                }

                return;
            }
            else
            {
                //  Left top corner selected.  
                startX = boxMaxX;
                startY = boxMaxY;

                //  Set rasterization direction to top border.  
                dir = TOP_BORDER_DIR;

                //  Project triangle vertices.  
                //vp[0][0] = vertex1[0]/vertex1[3];
                //vp[0][1] = vertex1[1]/vertex1[3];
                //vp[1][0] = vertex2[0]/vertex2[3];
                //vp[1][1] = vertex2[1]/vertex2[3];
                //vp[2][0] = vertex3[0]/vertex3[3];
                //vp[2][1] = vertex3[1]/vertex3[3];

                //  Viewport transformation.  
                //vp[0][0] = vp[0][0] * F32(w) * 0.5 +  F32(x0) + F32(w) * 0.5;
                //vp[0][1] = vp[0][1] * F32(h) * 0.5 +  F32(y0) + F32(h) * 0.5;
                //vp[1][0] = vp[1][0] * F32(w) * 0.5 +  F32(x0) + F32(w) * 0.5;
                //vp[1][1] = vp[1][1] * F32(h) * 0.5 +  F32(y0) + F32(h) * 0.5;
                //vp[2][0] = vp[2][0] * F32(w) * 0.5 +  F32(x0) + F32(w) * 0.5;
                //vp[2][1] = vp[2][1] * F32(h) * 0.5 +  F32(y0) + F32(h) * 0.5;

                //printf("Bounding mdu: %d %d x %d %d\n", boxMinX, boxMinY, boxMaxX, boxMaxY);
                //printf("Vertices: {%f, %f}, {%f, %f}, {%f, %f}\n", vp[0][0], vp[0][1],
                //    vp[1][0], vp[1][1], vp[2][0], vp[2][1]);

                return;

                //CG_ASSERT("Current algorithm couldn't find a start position.");
            }
        }
    }
    else
    {
        //  Use the center of the bounding mdu as start point.  
        startX = boxMinX + (boxMaxX - boxMinX)/2;
        startY = boxMinY + (boxMaxY - boxMinY)/2;
        dir = CENTER_DIR;

        return;
    }

    /*  NOTE: OLD CODE AHEAD.  THE CODE BELOW SHOULDN'T BE REACHABLE AND ISN'T CURRENTLY IN USE.
        KEPT AS REFERENCE UNTIL A FINAL ALGORITHM TO FIND THE START SCAN LINE RASTERIZATION POSITION
        IS IMPLEMENTED.   */

    //  Check triangle vertices w.  
    if ((vertex1[3] <= 0) || (vertex2[3] <= 0) || (vertex3[3] <= 0))
    {
        //  Check if there is a vertex with positive w.  
        if ((vertex1[3] > 0) || (vertex2[3] > 0) || (vertex3[3] > 0))
        {
            //  Select one of the vertex with positive w as start point.  
            if (vertex1[3] > 0)
            {
                //  Project the selected vertex.  
                vp[0][0] = vertex1[0]/vertex1[3];
                vp[0][1] = vertex1[1]/vertex1[3];
            }
            else if (vertex2[3] > 0)
            {
                //  Project the selected vertex.  
                vp[0][0] = vertex2[0]/vertex2[3];
                vp[0][1] = vertex2[1]/vertex2[3];
            }
            else
            {
                //  Project selected vertex.  
                vp[0][0] = vertex3[0]/vertex3[3];
                vp[0][1] = vertex3[1]/vertex3[3];
            }

            //  Viewport transform.  
            startX = (S32) floor(vp[0][0] * (F32) w * 0.5 +
                (F32) x0 + (F32) w * 0.5);
            startY = (S32) floor(vp[0][1] * (F32) h * 0.5 +
                (F32) y0 + (F32) h * 0.5);

            //  Set rasterization direction to center.  
            dir = CENTER_DIR;
        }
        else
        {
            /*  This triangle should have been trivially rejected
                at clipping.  */
            char buffer[64];

            sprintf(buffer, "Unclipped triangle w v1 %e v2 %e v3 %e\n", vertex1[3], vertex2[3], vertex3[3]);
            CG_ASSERT(buffer);
        }
    }
    else
    {
        //  Project triangle vertices.  
        vp[0][0] = vertex1[0]/vertex1[3];
        vp[0][1] = vertex1[1]/vertex1[3];
        vp[1][0] = vertex2[0]/vertex2[3];
        vp[1][1] = vertex2[1]/vertex2[3];
        vp[2][0] = vertex3[0]/vertex3[3];
        vp[2][1] = vertex3[1]/vertex3[3];

        //  Calculate top most vertex.  
        if ((vp[0][1] > vp[1][1]) && (vp[0][1] > vp[2][1]))
            topVertexId = 0;
        else if ((vp[1][1] > vp[0][1]) && (vp[1][1] > vp[2][1]))
            topVertexId = 1;
        else if ((vp[2][1] > vp[0][1]) && (vp[2][1] > vp[1][1]))
            topVertexId = 2;
        else if ((vp[0][1] > vp[1][1]) && (vp[0][1] == vp[2][1]) && (vp[0][0] >= vp[2][0]))
            topVertexId = 0;
        else if ((vp[0][1] > vp[1][1]) && (vp[0][1] == vp[2][1]) && (vp[0][0] < vp[2][0]))
            topVertexId = 2;
        else if ((vp[0][1] == vp[1][1]) && (vp[0][1] > vp[2][1]) && (vp[0][0] >= vp[1][0]))
            topVertexId = 0;
        else if ((vp[0][1] == vp[1][1]) && (vp[0][1] > vp[2][1]) && (vp[0][0] < vp[1][0]))
            topVertexId = 1;
        else if ((vp[1][1] > vp[0][1]) && (vp[1][1] == vp[2][1]) && (vp[1][0] >= vp[2][0]))
            topVertexId = 1;
        else if ((vp[1][1] > vp[0][1]) && (vp[1][1] == vp[2][1]) && (vp[1][0] < vp[2][0]))
            topVertexId = 2;
        else
            topVertexId = 1;  //  It's not a correct triangle!!!  

        //  Set top vertex.  
        topVertex = vp[topVertexId];

        //  Viewport transform.  
        startX = (S32) floor(topVertex[0] * (F32) w * 0.5 +
            (F32) x0 + (F32) w * 0.5);
        startY = (S32) floor(topVertex[1] * (F32) h * 0.5 +
           (F32) y0 + (F32) h * 0.5);

        //  Set rasterization direction to down.  
        dir = DOWN_DIR;
    }
}

//  Creates the interpolation equation for triangle vertex parameter.  
void GPUMath::interpolationEquation(F64 *edge1, F64 *edge2, F64 *edge3,
    F32 u0, F32 u1, F32 u2, F64 *intEq)
{

    /*  The interpolation equation for a vertex parameters using
        2DH rasterization is calculated using this formula:

                                [ a0 b0 c0 ]
        [a b c ] = [u0 u1 u2] * [ a1 b1 c1 ]
                                [ a2 b2 c2 ]

        Where (a, b, c) are the coefficients of the interpolation equation,
        (u0, u1, u2) are the parameter values at the three triangle vertices
        and (a0, b0, c0), (a1, b1, c1) and (a2, b2, c2) are three edge
        equations of the triangle (the rasterization matrix rows).

     */

     //  Calculate the interpolation equation coefficients.  
     intEq[0] = edge1[0] * F64(u0) + edge2[0] * F64(u1) + edge3[0] * F64(u2);
     intEq[1] = edge1[1] * F64(u0) + edge2[1] * F64(u1) + edge3[1] * F64(u2);
     intEq[2] = edge1[2] * F64(u0) + edge2[2] * F64(u1) + edge3[2] * F64(u2);
}

//  Computes the triangle size based on the screen area of non-homogeneous projected screen coordinates.  
F64 GPUMath::computeTriangleScreenArea(Vec4FP32 vPos1, Vec4FP32 vPos2, Vec4FP32 vPos3, Vec4FP32& nH_vPos1, Vec4FP32& nH_vPos2, Vec4FP32& nH_vPos3)
{
    F64 vtx1_inv_w;
    F64 vtx2_inv_w;
    F64 vtx3_inv_w;
    F64 nH_vtx1_x;
    F64 nH_vtx1_y;
    F64 nH_vtx1_z;
    F64 nH_vtx2_x;
    F64 nH_vtx2_y;
    F64 nH_vtx2_z;
    F64 nH_vtx3_x;
    F64 nH_vtx3_y;
    F64 nH_vtx3_z;

    //  Compute vertex�s 1/w.  
    vtx1_inv_w = 1.0 / vPos1[3];
    vtx2_inv_w = 1.0 / vPos2[3];
    vtx3_inv_w = 1.0 / vPos3[3];

    //  Project the vertices screen coordinates into non-homogeneous coordinates. 
    nH_vtx1_x = vPos1[0] * vtx1_inv_w;
    nH_vtx1_y = vPos1[1] * vtx1_inv_w;
    nH_vtx1_z = vPos1[2] * vtx1_inv_w;
    nH_vtx2_x = vPos2[0] * vtx2_inv_w;
    nH_vtx2_y = vPos2[1] * vtx2_inv_w;
    nH_vtx2_z = vPos2[2] * vtx2_inv_w;
    nH_vtx3_x = vPos3[0] * vtx3_inv_w;
    nH_vtx3_y = vPos3[1] * vtx3_inv_w;
    nH_vtx3_z = vPos3[2] * vtx3_inv_w;

    //  Return non-homogeneous coordinates in output parameters.  
    nH_vPos1.setComponents(F32(nH_vtx1_x), F32(nH_vtx1_y), F32(nH_vtx1_z), F32(vtx1_inv_w));
    nH_vPos2.setComponents(F32(nH_vtx2_x), F32(nH_vtx2_y), F32(nH_vtx2_z), F32(vtx2_inv_w));
    nH_vPos3.setComponents(F32(nH_vtx3_x), F32(nH_vtx3_y), F32(nH_vtx3_z), F32(vtx3_inv_w));

    /*
     *  Compute the triangle area based on the cross product of two edges of the triangle.
     *  Computation is as follows:
     *
     *         v1
     *         |\
     *         | \
     *         |  \
     *         |   \
     *         |    \
     *         v2____v3
     *
     *  Area(v1,v2,v3) = (v1v2) x (v1v3) / 2 = [(v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)] / 2
     *
     *  Each coordinate is in the range [-1,1], so total range for Area is [-4,4]. Normalization to [-1,1] requires
     *  additionally divide by 4.0.
     *
     */

    return (( nH_vtx2_x - nH_vtx1_x )*( nH_vtx3_y - nH_vtx1_y ) -
            ( nH_vtx3_x - nH_vtx1_x )*( nH_vtx2_y - nH_vtx1_y ))/ 8.0;
}

//  Vector addition of two 4-component 64bit vectors.  
void GPUMath::ADD64(F64* res, F64* a, F64* b)
{
    res[0] = a[0] + b[0];
    res[1] = a[1] + b[1];
    res[2] = a[2] + b[2];
    res[3] = a[3] + b[3];
}

/*  Evaluate if a sample is in the negative or positive half space
    defined by the triangle three edge equations.  */
U32 GPUMath::evaluateSample(F64 *s)
{
    U32 outcode;

    /*  Calculate the outcodes for the sample point.

        outcode

            bit 0 -> 1: sample in negative half plane of the triangle first
                        edge equation.
                     0: sample in positive half plane of the triangle first
                        edge equation.
            bit 1 -> 1: sample in negative half plane of the triangle second
                        edge equation.
                     0: sample in positive half plane of the triangle second
                        edge equation.
            bit 2 -> 1: sample in negative half plane of the triangle third
                        edge equation.
                     0: sample in positive half plane of the triangle third
                        edge equation.

        The sample is inside the triangle if it is 0.  It is outside
        if not zero.

      */

    //  Calculate first equation outcode.  
    outcode = (s[0] < 0)?0x01:0x00;

    //  Calculate second equation outcode.  
    outcode |= (s[1] < 0)?0x02:0x00;

    //  Calculate third equation outcode.  
    outcode |= (s[2] < 0)?0x04:0x00;

    return outcode;
}

/*  Evaluate the tile 4 sample points and determine if the tile has
    triangle fragments inside.  */
bool GPUMath::evaluateTile(F64 *s0, F64 *s1, F64 *s2, F64 *s3)
{
    bool pass;
    U32 outcode[4];

    //  Evaluate the four sample points defining the tile.  
    outcode[0] = evaluateSample(s0);
    outcode[1] = evaluateSample(s1);
    outcode[2] = evaluateSample(s2);
    outcode[3] = evaluateSample(s3);

    /*  Evaluate the tile.

        A tile doesn't have triangle fragments inside if all the
        sample points are in the negative half space of the triangle
        three edge equations.
        That can be calculated using the sample outcodes, if the logical
        and of the three sample outcodes is not zero the tile doesn't
        have any triangle fragment inside.  If the logical and of the
        three sample point outcodes is zero there may be triangle
        fragments inside the tile.

     */

    pass = ((outcode[0] & outcode[1] & outcode[2] & outcode[3]) == 0);

    return pass;
}


/*  Converts an input 32 bit unsigned integer value into a bit shift
    value that covers/removes/adds at least as many values as the
    input value.  */
U32 GPUMath::calculateShift(U32 n)
{
    U32 l;
    U32 m;

    //  Value 0 has no shift.  
    if (n == 0)
        return 0;
    //  Value 1 translates to 1 bit shift.  
    else if (n == 1)
        return 1;
    else
    {
        //  Calculate shift length for the value.  
        for (l = 0, m = 1; (l < 32) && (n > m); l++, m = m << 1);

        return l;
    }
}


/*  Converts an input 32 bit unsigned integer value into a mask that
    covers at least as many values as the input value.  */
U32 GPUMath::buildMask(U32 n)
{
    U32 l;
    U32 m;

    //  Value 0 has no mask.  
    if (n == 0)
        return 0;
    //  Value 1 translates to mask 0x01.  
    else if (n == 1)
        return 0x01;
    else
    {
        //  Calculate the shift for the value.  
        l = calculateShift(n);

        //  Build the start mask.  
        m = (unsigned int) -1;

        //  Adjust the mask.  
        m = m >> (32 - l);
    }

    return m;
}

//  Translates a pixel address to a memory address in the framebuffer.  
U32 GPUMath::pixel2Memory(S32 x, S32 y, S32 x0, S32 y0,
    U32 w, U32 h, U32 xRes, U32 yRes, U32 stampW, U32 stampH,
    U32 genW, U32 genH, U32 scanW, U32 scanH, U32 overW, U32 overH,
    U32 samples, U32 bytesSample, bool log)
{
    /*
        Memory layout:

        Over scan tiles:

            device

            <--- xRes ---->
            O0  O1  O2  O3  ^
            O4  O5  O6  O7  | yRes
            O8  O9  O10 O11 |
            O12 O13 O14 O15 v

            memory (rows)

            O0 O1 O2 O3 O4 O5 O6 O7 O8 O9 O10 O11 O12 O13 O14 O15

        Scan tiles (inside over scan tile):

        For 1 sample:
              
            device

            <--- overW ---->
            T0  T1  T2  T3  ^
            T4  T5  T6  T7  | overH
            T8  T9  T10 T11 |
            T12 T13 T14 T15 v

            memory (rows)

            T0 T1 T2 T3 T4 T5 T6 T7 T8 T9 T10 T11 T12 T13 T14 T15

            memory (morton) <- current!!

            T0 T1 T4 T5  T2 T3 T6 T7  T8 T9 T12 T13  T10 T11 T14 T15

        When the number of samples is > 1 the scan tile is subdivided in as many subtiles as
        the number of samples per pixel.  Subtiles from the different scan tiles are then
        interleaved.
        
        For 2 samples:
        
            <--------------------- overW * 2 -------------------->
            T0S0   T0S1  T1S0   T1S1   T2S0   T2S1   T3S0   T3S1  ^
            T4S0   T4S1  T5S0   T5S1   T6S0   T6S0   T7S0   T7S1  | overH
            T8S0   T8S1  T9S0   T9S1   T10S0  T10S1  T11S0  T11S1 |
            T12S0  T12S0 T13S0  T13S1  T14S0  T14S1  T15S0  T15S1 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1

        For 4 samples:
        
            <--------------------- overW * 2 -------------------->
            T0S0   T0S1  T1S0   T1S1   T2S0   T2S1   T3S0   T3S1  ^
            T0S2   T0S3  T1S2   T1S3   T2S2   T2S3   T3S2   T3S3  |
            T4S0   T4S1  T5S0   T5S1   T6S0   T6S0   T7S0   T7S1  |
            T4S2   T4S3  T5S2   T5S3   T6S2   T6S3   T7S2   T7S3  | overH * 2
            T8S0   T8S1  T9S0   T9S1   T10S0  T10S1  T11S0  T11S1 |
            T8S2   T8S3  T9S2   T9S3   T10S2  T10S3  T11S2  T11S3 |
            T12S0  T12S0 T13S0  T13S1  T14S0  T14S1  T15S0  T15S1 |
            T12S2  T12S3 T13S2  T13S3  T14S2  T14S3  T15S2  T15S3 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
            T0S2 T1S2 T2S2 T3S2 T4S2 T5S2 T6S2 T7S2 T8S2 T9S2 T10S2 T11S2 T12S2 T13S2 T14S2 T15S2
            T0S3 T1S3 T2S3 T3S3 T4S3 T5S3 T6S3 T7S3 T8S3 T9S3 T10S3 T11S3 T12S3 T13S3 T14S3 T15S3
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1
            T0S2 T1S2 T4S2 T5S2  T2S2 T3S2 T6S2 T7S2  T8S2 T9S2 T12S2 T13S2  T10S2 T11S2 T14S2 T15S2
            T0S3 T1S3 T4S3 T5S3  T2S3 T3S3 T6S3 T7S3  T8S3 T9S3 T12S3 T13S3  T10S3 T11S3 T14S3 T15S3

        For 8 samples:
        
            <------------------------------------------------- overW * 4 ----------------------------------------------->
            T0S0   T0S1  T0S2   T0S3   T1S0   T1S1   T1S2   T1S3   T2S0   T2S1   T2S2   T2S3   T3S0   T3S1   T3S2   T3S3  ^
            T0S4   T0S5  T0S6   T0S7   T1S4   T1S5   T1S6   T1S7   T2S4   T2S5   T2S6   T2S7   T3S4   T3S5   T3S6   T3S7  |
            T4S0   T4S1  T0S2   T4S3   T5S0   T5S1   T5S2   T5S3   T6S0   T6S1   T6S2   T6S3   T7S0   T7S1   T7S2   T7S3  |
            T4S4   T4S5  T0S6   T4S7   T5S4   T5S5   T5S6   T5S7   T6S4   T6S5   T6S6   T6S7   T7S4   T7S5   T7S6   T7S7  |
            T8S0   T8S1  T8S2   T8S3   T9S0   T9S1   T9S2   T9S3   T10S0  T10S1  T10S2  T10S3  T11S0  T11S1  T11S2  T11S3 |  overH * 2
            T8S4   T8S5  T8S6   T8S7   T9S4   T9S5   T9S6   T9S7   T10S4  T10S5  T10S6  T10S7  T11S4  T11S5  T11S6  T11S7 |
            T12S0  T12S1 T12S2  T12S3  T13S0  T13S1  T13S2  T13S3  T14S0  T14S1  T14S2  T14S3  T15S0  T51S1  T51S2  T51S3 |
            T12S4  T12S5 T12S6  T12S7  T13S4  T13S5  T13S6  T13S7  T14S4  T14S5  T14S6  T14S7  T15S4  T51S5  T51S6  T51S7 v

            memory (rows)
            
            T0S0 T1S0 T2S0 T3S0 T4S0 T5S0 T6S0 T7S0 T8S0 T9S0 T10S0 T11S0 T12S0 T13S0 T14S0 T15S0
            T0S1 T1S1 T2S1 T3S1 T4S1 T5S1 T6S1 T7S1 T8S1 T9S1 T10S1 T11S1 T12S1 T13S1 T14S1 T15S1
            T0S2 T1S2 T2S2 T3S2 T4S2 T5S2 T6S2 T7S2 T8S2 T9S2 T10S2 T11S2 T12S2 T13S2 T14S2 T15S2
            T0S3 T1S3 T2S3 T3S3 T4S3 T5S3 T6S3 T7S3 T8S3 T9S3 T10S3 T11S3 T12S3 T13S3 T14S3 T15S3
            T0S4 T1S4 T2S4 T3S4 T4S4 T5S4 T6S4 T7S4 T8S4 T9S4 T10S4 T11S4 T12S4 T13S4 T14S4 T15S4
            T0S5 T1S5 T2S5 T3S5 T4S5 T5S5 T6S5 T7S5 T8S5 T9S5 T10S5 T11S5 T12S5 T13S5 T14S5 T15S5
            T0S6 T1S6 T2S6 T3S6 T4S6 T5S6 T6S6 T7S6 T8S6 T9S6 T10S6 T11S6 T12S6 T13S6 T14S6 T15S6
            T0S7 T1S7 T2S7 T3S7 T4S7 T5S7 T6S7 T7S7 T8S7 T9S7 T10S7 T11S7 T12S7 T13S7 T14S7 T15S7
                    
            memory (morton) <- current!!
                       
            T0S0 T1S0 T4S0 T5S0  T2S0 T3S0 T6S0 T7S0  T8S0 T9S0 T12S0 T13S0  T10S0 T11S0 T14S0 T15S0
            T0S1 T1S1 T4S1 T5S1  T2S1 T3S1 T6S1 T7S1  T8S1 T9S1 T12S1 T13S1  T10S1 T11S1 T14S1 T15S1
            T0S2 T1S2 T4S2 T5S2  T2S2 T3S2 T6S2 T7S2  T8S2 T9S2 T12S2 T13S2  T10S2 T11S2 T14S2 T15S2
            T0S3 T1S3 T4S3 T5S3  T2S3 T3S3 T6S3 T7S3  T8S3 T9S3 T12S3 T13S3  T10S3 T11S3 T14S3 T15S3
            T0S4 T1S4 T4S4 T5S4  T2S4 T3S4 T6S4 T7S4  T8S4 T9S4 T12S4 T13S4  T10S4 T11S4 T14S4 T15S4
            T0S5 T1S5 T4S5 T5S5  T2S5 T3S5 T6S5 T7S5  T8S5 T9S5 T12S5 T13S5  T10S5 T11S5 T14S5 T15S5
            T0S6 T1S6 T4S6 T5S6  T2S6 T3S6 T6S6 T7S6  T8S6 T9S6 T12S6 T13S6  T10S6 T11S6 T14S6 T15S6
            T0S7 T1S7 T4S7 T5S7  T2S7 T3S7 T6S7 T7S7  T8S7 T9S7 T12S7 T13S7  T10S7 T11S7 T14S7 T15S7

        Generation tiles (inside scan tile):

            device

            <--- scanW ---->
            G0  G1  G2  G3  ^
            G4  G5  G6  G7  | scanH
            G8  G9  G10 G11 |
            G12 G13 G14 G15 v

            memory (rows)

            G0 G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 G11 G12 G13 G14 G15              
        
        Stamps (inside generation tile):        

            <---- genW ---->
            S0  S1  S2  S3  ^
            S4  S5  S6  S7  | genH
            S8  S9  S10 S11 |
            S12 S13 S14 S15 v

            memory

            S0 S1 S2 S3 S4 S5 S6 S7 S8 S9 S10 S11 S12 S13 S14 S15

        Pixels (inside a stamp):

            <--- stampW ---->
            P0  P1  P2  P3  ^
            P4  P5  P6  P7  | stampH
            P8  P9  P10 P11 |
            P12 P13 P14 P15 v

            memory

            P0 P1 P2 P3 P4 P5 P6 P7 P8 P9 P10 P11 P12 P13 P14 P15

        Samples (inside a pixel)
        
            memory 
            
            S0 S1 S2 S3             
       
        Bytes (inside a pixel):

            memory

            R G B A


        Other organizations could be used:
            - Hilbert order.
            - Morton order.
            - ...

     */

    //  Position already in device coordinates.
    U32 xDevice = x;
    U32 yDevice = y;

    ///  Removed, as clipping is done currently at color write (should change!).
    //GPU_ASSERT(
    //    if ((xDevice >= xRes) || (yDevice >= yRes))
    //        CG_ASSERT("Pixel outside the device/framebuffer.");
    //)

    //  Get over scan tile coordinates inside the device.
    U32 xOverTile = xDevice / (overW * scanW * genW * stampW);
    U32 yOverTile = yDevice / (overH * scanH * genH * stampH);

    //  Calculate address for the over scan tile.
    U32 overTileColumns = (U32) ceil((F32) xRes / (F32) (overW * scanW * genW * stampW));
    U32 address = yOverTile * overTileColumns + xOverTile;

if (log)
printf("GPUMath::pixel2Memory -> address (over tile) = %d\n", address);

    //  Get scan tile coordinates inside the over scan tile.
    U32 xScanTile = GPU_MOD(xDevice / (scanW * genW * stampW), overW);
    U32 yScanTile = GPU_MOD(yDevice / (scanH * genH * stampH), overH);

    U32 scanSubH = 1;
    U32 scanSubW = 1;
    
    //  Generate sub scan tile parameters.
    if (samples == 1)
    { 
        //  Nothing to do.
    }    
    //  Adjust scan and gen tile sizes to the multisampling level.
    else if (samples == 2)
    {
        scanSubW = 2;
        
        if (genW > 1)
            genW = genW / 2;
        else
            CG_ASSERT("For multisampling 2x gen tiles must be at least 2 stamp wide.");   
    }
    else if (samples == 4)
    {
        scanSubW = 2;
        scanSubH = 2;
        
        if (genW > 1)
            genW = genW / 2;
        else
            CG_ASSERT("For multisampling 4x gen tiles must be at least 2 stamp wide.");

        if (genH > 1)
            genH = genH / 2;
        else
            CG_ASSERT("For multisampling 4x gen tiles must be at least 2 stamp high.");
    }
    else if (samples == 8)
    {
        scanSubW = 4;
        scanSubH = 2;
        
        if (genW > 3)
            genW = genW / 4;
        else
            CG_ASSERT("For multisampling 8x gen tiles must be at least 4 stamp wide.");

        if (genH > 1)
            genH = genH / 2;
        else
            CG_ASSERT("For multisampling 8x gen tiles must be at least 2 stamp high.");
    }    
    else
        CG_ASSERT("Multisampling level not supported.");
    
if (log)
printf("GPUMath::pixel2Memory -> Adjusted (multisampling): genW = %d | genH = %d\n", genW, genH);

    if (bytesSample == 8)
    {
        scanSubH = scanSubH * 2;
        if (genH > 1)
            genH = genH / 2;
        else
            CG_ASSERT("For FP16 color gen tiles must be at least 2 stamp high.");
    }

if (log)
printf("GPUMath::pixel2Memory -> Adjusted (FP16): genW = %d | genH = %d\n", genW, genH);

    //  Get coordinates of the scan sub tile.
    U32 xScanSubTile = GPU_MOD(xDevice / (scanW * genW * stampW), scanSubW);
    U32 yScanSubTile = GPU_MOD(yDevice / (scanH * genH * stampH), scanSubH);
    
    //  Calculate address of the scan sub tile.    
    address = address * scanSubH * scanSubW + yScanSubTile * scanSubW + xScanSubTile;

if (log)
printf("GPUMath::pixel2Memory -> address (scan sub tile) = %d\n", address);

    //  NOTE:  Replaced with morton order for scan tiles.
    //  Calculate address for the scan tile (row major).
    //address = address * overW * overH + yScanTile * overW + yScanTile;

    //  Calculate address for the scan tile (morton).
    address = address * overW * overH + morton(overW, xScanTile, yScanTile);

if (log)
printf("GPUMath::pixel2Memory -> address (scan tile) = %d\n", address);

    //  Get generation tile coordinates inside the scan tile .
    U32 xGenTile = GPU_MOD(xDevice / (genW * stampW), scanW);
    U32 yGenTile = GPU_MOD(yDevice / (genH * stampH), scanH);

    //  Calculate address for the scan tile.
    address = address * scanW * scanH  + yGenTile * scanW + xGenTile;

if (log)
printf("GPUMath::pixel2Memory -> address (gen tile) = %d\n", address);

    //  Get coordinates stamp coordinates inside the generation tile.
    U32 xStamp = GPU_MOD(xDevice / stampW, genW);
    U32 yStamp = GPU_MOD(yDevice / stampH, genH);

    //  Calculate address for the stamp.
    address = address * genW * genH + yStamp * genW + xStamp;

if (log)
printf("GPUMath::pixel2Memory -> address (stamp tile) = %d\n", address);

    //  Get pixel coordinates inside the stamp.
    U32 xPixel = GPU_MOD(xDevice, stampW);
    U32 yPixel = GPU_MOD(yDevice, stampH);

if (log)
{
printf("GPUMath::pixel2Memory -> xDev = %d | yDev = %d | xOverTile = %d | yOverTile = %d\n",
xDevice, yDevice, xOverTile, yOverTile);
printf("GPUMath::pixel2Memory -> xScanTile = %d | yScanTile = %d | xScanSubTile = %d | yScanSubTile = %d\n",
xScanTile, yScanTile, xScanSubTile, yScanSubTile);
printf("GPUMath::pixel2Memory -> xGenTile = %d | yGenTile = %d | xStamp = %d | yStamp = %d | xPixel = %d | yPixel = %d\n",
xGenTile, yGenTile, xStamp, yStamp, xPixel, yPixel);
printf("GPUMath::pixel2Memory -> samples = %d | bytesSample = %d\n", samples, bytesSample);
printf("GPUMath::pixel2Memory -> mem page = %d | rop = %d\n", GPU_MOD(address, 1024) & 0x01, ((xScanTile + yScanTile) & 0x01));
}

    //  Get address for the pixel (which pixel).
    address = address * stampW * stampH + yPixel * stampW + xPixel;
if (log)
printf("GPUMath::pixel2Memory -> address (pixel) = %d\n", address);

    //  Get start address.
    address = address * samples * bytesSample;

if (log)
printf("GPUMath::pixel2Memory -> address (byte) = %d\n", address);

    return address;
}

bool GPUMath::mortonTableInitialized = false;
U08 GPUMath::mortonTable[256];

void GPUMath::buildMortonTable()
{
    for(U32 i = 0; i < 256; i++)
    {
        U32 t1 = i & 0x0F;
        U32 t2 = (i >> 4) & 0x0F;
        U32 m = 0;

        for(U32 nextBit = 0; nextBit < 4; nextBit++)
        {
            m += ((t1 & 0x01) << (2 * nextBit)) + ((t2 & 0x01) << (2 * nextBit + 1));

            t1 = t1 >> 1;
            t2 = t2 >> 1;
        }

        mortonTable[i] = m;
    }

    mortonTableInitialized = true;
}


//  Calculates the address in morton order of an element in a 2^size x 2^size matrix.  
U32 GPUMath::morton(U32 size, U32 i, U32 j)
{
    // Check if morton table was initialized
    if (!mortonTableInitialized)
        buildMortonTable();

    U32 address;
    U32 t1, t2;

    t1 = i;
    t2 = j;

    switch(size)
    {
        case 0:

            address = 0;

            return address;

            break;

        case 1:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x03;

            return address;

            break;

        case 2:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x0F;

            return address;

            break;

        case 3:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x3F;

            return address;

            break;

        case 4:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];

            return address;

            break;

        case 5:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x03) << 8;

            return address;

            break;

        case 6:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x0F) << 8;

            return address;

            break;

        case 7:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x3F) << 8;

            return address;

            break;

        case 8:

            address = mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] << 8;

            return address;

            break;

        default:
            CG_ASSERT("Fast morton not supported for this tile size");

            return 0;
            break;
    }

    /*

    U32 address;
    U32 nextBit;
    U32 t1, t2;

    t1 = i;
    t2 = j;
    address = 0;

    //  Get a bit from each coordinate each time.
    for(nextBit = 0; nextBit < size; nextBit++)
    {
        address += ((t1 & 0x01) << (2 * nextBit)) + ((t2 & 0x01) << (2 * nextBit + 1));
        t1 = t1 >> 1;
        t2 = t2 >> 1;
    }

    return address;

    */
}


//  Convert a 16-bit float point value to a 32-bit float point value.
F32 GPUMath::convertFP16ToFP32(f16bit input)
{
    U08 sign;
    U32 mantissa;
    S32 exponent;
    U32 output;
    U32 normbits;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Disassemble float16 value in sign, exponent and mantissa.
    //  Read float16 sign.
    sign = ((input & 0x8000) == 0) ? 0 : 1;

    //  Read the float16 exponent.
    exponent = ((input & 0x7c00) >> 10);

    //  Read the float16 mantissa (explicit bits).
    mantissa = (input & 0x03ff);

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0x1f) && (mantissa == 0);
    nan = (exponent == 0x1f) && (mantissa != 0);

    //  Return zeros.
    if (zero)
       return (sign == 0) ? 0.0f : -0.0f;

    //  Convert exponent and mantissa to float32.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32.  Mantissa goes from 10+1 bits to 23+1 bits.
    if (infinite || nan)
        mantissa = mantissa;
    else
        mantissa = (mantissa << 13);

    //  Check for denormalized float16 values.
    if (denorm)
    {
        //  Renormalize mantissa for denormalized float16 values.
        if      ((mantissa & 0x00400000) != 0)
            normbits = 1;
        else if ((mantissa & 0x00200000) != 0)
            normbits = 2;
        else if ((mantissa & 0x00100000) != 0)
            normbits = 3;
        else if ((mantissa & 0x00080000) != 0)
            normbits = 4;
        else if ((mantissa & 0x00040000) != 0)
            normbits = 5;
        else if ((mantissa & 0x00020000) != 0)
            normbits = 6;
        else if ((mantissa & 0x00010000) != 0)
            normbits = 7;
        else if ((mantissa & 0x00008000) != 0)
            normbits = 8;
        else if ((mantissa & 0x00004000) != 0)
            normbits = 9;
        else if ((mantissa & 0x00002000) != 0)
            normbits = 10;
        else
            CG_ASSERT("Error normalizing denormalized float16 mantissa.");

        //  Decrease exponent 1 bit.
        exponent = exponent - normbits;

        //  Shift exponent 1 bit and remove implicit bit;
        mantissa = ((mantissa << normbits) & 0x007fffff);
    }

    //  Assemble the float32 value with the obtained sign, exponent and mantissa.
    output = (sign << 31) | (U32(exponent & 0xff) << 23) | mantissa;

    return *((F32 *) &output);
}

//  Convert a float32 value to a float16 value.
f16bit GPUMath::convertFP32ToFP16(F32 input)
{
    U08 sign;
    U32 mantissa;
    S32 exponent;
    U32 inputAux;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    inputAux = *((U32 *) &input);

    //  Disassemble float32 value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((inputAux & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (inputAux >> 23) & 0xff;

    //  Extract mantissa.
    mantissa = inputAux & 0x007fffff;

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0xff) && (mantissa == 0);
    nan = (exponent == 0xff) && (mantissa != 0);

    //  Flush float32 denorms to 0
    if (denorm || zero)
        return (sign == 0) ? 0x0000 : 0x8000;

    //  Flush numbers with an exponent not representable in float16 to infinite.
    if (exponent > (127 + 15))
        return ((sign == 0) ? 0x7c00 : 0xfc00);
    else
    {
        //  Convert exponent to float16.  Excess 127 to excess 15.
        exponent = exponent - 127 + 15;

        //  Convert mantissa from to float16.  Mantissa 23+1 to mantissa 10+1.
        mantissa = (mantissa >> 13) & 0x03ff;

        //  Check for denormalized float16 number.
        if (exponent <= 0)
        {
            //  Add implicit float32 bit to the mantissa.
            mantissa = mantissa + 0x0400;

            //  Denormalize mantissa.
            mantissa = mantissa >> (-exponent + 1);
            //  Set denormalized exponent.
            exponent = 0;
        }

        //  Assemble the float16 value.
        return (sign << 15) | (U32(exponent & 0x3f) << 10) | mantissa;
    }
}




