/**************************************************************************
 *
 * The Fixed Point class implements a fixed point type and basic arithmetic operations.
 *
 */

/**
 *
 *  @file FixedPoint.cpp
 *
 *  This file implements the Fixed Point class supporting fixed point data types and basic
 *  fixed point arithmetic operations.
 *
 */

#include "FixedPoint.h"
#include <iostream>
#include <cmath>

using namespace std;

//  Constructor:  32-bit float point version.
FixedPoint::FixedPoint(F32 f, U32 intBits_, U32 fracBits_) :
    intBits(intBits_), fracBits(fracBits_)
{
    U08 exponent;
    U32 mantissa;

    //  Check representation limits.
    if (intBits > MAX_INTEGER_BITS)
    {
        CG_ASSERT("Integral representation exceded");
    }

    //  Check representation limits.
    if (fracBits > MAX_FRACTIONAL_BITS)
    {
        CG_ASSERT("Fractional representation exceded");
    }

//cout << "----  Starting conversion (source float32) --- " << endl << endl;
//cout << "float point value = " << f << endl;

    //  Get components of the 32-bit float point value; sign, mantissa and exponent.
    sign = (((*((U32 *) &f) & 0x80000000) >> 31) == 0) ? 1 : -1;
    mantissa = (*((U32 *) &f) & 0x007fffff);
    exponent = (*((U32 *) &f) & 0x7f800000) >> 23;

//cout << hex;
//cout << "sign = " << S32(sign) << " | mantissa = " << mantissa << " | exponent = " << U32(exponent) << endl;

    //  Check for NaN or infinite.
    nan = (mantissa != 0) && (exponent == 0xff);
    infinite = (mantissa == 0) && (exponent == 0xff);

    //  Check for denormalized 32-bit float point values.
    bool denormal = (mantissa != 0) && (exponent == 0);

    //  Set exponent to the denormalized exponent.
    if (denormal)
    {
//cout << "denormal" << endl;
        exponent = 1;
    }

    //  Check for zero.
    bool zero = (mantissa == 0) && (exponent == 0);

    //  Add to the mantissa the implicit most significative bit if not a denormalized number.
    if (!denormal && !zero)
    {
        mantissa = mantissa | 0x00800000;
    }

    //  Compute the position of the fractional point inside the mantissa.
    S32 pointPosition = 23 - (exponent - 127);

//cout << dec;
//cout << "pointPosition = " << pointPosition << endl;

    //  Set overflow flag.
    overflow = (pointPosition < 0);

    //  Create the mask covering the integral bits of the fixed point value.
    U64 intMask = (U64(1) << intBits) - 1;

//cout << hex;
//cout << "intMask = " << intMask << endl;

    //  Get the integral bits of the value.
    if (std::abs(F32(pointPosition)) > 31)
        intVal = 0;     //  Shifted to zero due to shift value overflow.
    else
        intVal = ((pointPosition >= 0) ? (mantissa >> pointPosition) : (U64(mantissa) << S32(std::abs(F32(pointPosition))))) & intMask;

//cout << "shifted mantissa (+) = " << (mantissa >> pointPosition) << endl;
//cout << "shifted mantissa (-) = " << (mantissa << std::abs(F32(pointPosition))) << endl;

//cout << "intVal = " << intVal << endl;

    //  Create the mask covering the fractional bits of the fixed point value.
    U64 fracMask = U64((U64(1) << fracBits) - 1) << (64 - fracBits);

//cout << "fracBits = " << fracBits << " t = " << (1 << fracBits) << " s = " << ((1 << fracBits) - 1) << " r = " << (64 - fracBits) << endl;
//cout << "fracMask = " << fracMask << endl;

    //  Get the fractional bits of the value.
    S32 fractionalShift = 64 - pointPosition;
    fracVal = (pointPosition <= 0) ? 0 : ((fractionalShift >= 0) ? (U64(mantissa) << fractionalShift) : (U64(mantissa) >> (-fractionalShift)));
    fracVal = fracVal & fracMask;

//cout << "shifted Mantissa = " << (mantissa << (64 - pointPosition)) << endl;
//cout << "fracVal = " << fracVal << endl;

    //  Check for underflow.
    underflow = !overflow && (mantissa != 0) && (intVal == 0) && (fracVal == 0);

    if (overflow)
    {
//cout << "Overflow" << endl;
        intVal = (1 << (intBits -1)) - 1;
        fracVal = (1 << fracBits) - 1;
    }

    if (underflow)
    {
//cout << "Underflow" << endl;
        intVal = 0;
        fracVal = 0;
    }

    if (nan)
    {
//cout << "NaN" << endl;
        intVal  = 0;
        fracVal = 0;
        sign = 0;
    }

    if (infinite)
    {
//cout << "infinite" << endl;
        intVal  = 0xffffffffffffffffULL;
        fracVal = 0xffffffffffffffffULL;
    }

//cout << dec;
//cout << "---- End of conversion ---- " << endl << endl;
}

//  Constructor:  64-bit float point version.
FixedPoint::FixedPoint(F64 f, U32 intBits_, U32 fracBits_) :
    intBits(intBits_), fracBits(fracBits_)
{
    U16 exponent;
    U64 mantissa;

    //  Check representation limits.
    if (intBits > MAX_INTEGER_BITS)
    {
        CG_ASSERT("Integral representation exceded");
    }

    //  Check representation limits.
    if (fracBits > MAX_FRACTIONAL_BITS)
    {
        CG_ASSERT("Fractional representation exceded");
    }

//cout << "----  Starting conversion (source float64) --- " << endl << endl;
//cout << "float point value = " << f << endl;

    //  Get components of the 64-bit float point value; sign, mantissa and exponent.
    sign = ((*((U64 *) &f) & 0x8000000000000000ULL) >> 63) == 0 ? 1 : -1;
    mantissa = (*((U64 *) &f) & 0x000fffffffffffffULL);
    exponent = (*((U64 *) &f) & 0x7ff0000000000000ULL) >> 52;

//cout << hex;
//cout << "sign = " << S64(sign) << " | mantissa = " << mantissa << " | exponent = " << U64(exponent) << endl;

    //  Check for NaN or infinite.
    nan = (mantissa != 0) && (exponent == 0x7ff);
    infinite = (mantissa == 0) && (exponent == 0x7ff);

    //  Check for denormalized 64-bit float point values.
    bool denormal = (mantissa != 0) && (exponent == 0);

    //  Set exponent to the denormalized exponent.
    if (denormal)
    {
//cout << "denormal" << endl;
        exponent = 1;
    }

    //  Add to the mantissa the implicit most significative bit if not a denormalized number.
    if (!denormal)
    {
        mantissa = mantissa | 0x0010000000000000ULL;
    }

    //  Compute the position of the fractional point inside the mantissa.
    S32 pointPosition = 52 - (exponent - 1023);

//cout << dec;
//cout << "pointPosition = " << pointPosition << endl;

    //  Set overflow flag.
    overflow = (pointPosition < 0);

    //  Create the mask covering the integral bits of the fixed point value.
    U64 intMask = (U64(1) << intBits) - 1;

//cout << hex;
//cout << "intMask = " << intMask << endl;

    //  Get the integral bits of the value.
    if (std::abs(F32(pointPosition)) > 63)
        intVal = 0;     //  Shifted to zero due to shift value overflow.
    else
        intVal = ((pointPosition >= 0) ? (mantissa >> pointPosition) : (mantissa << S32(std::abs(F32(pointPosition))))) & intMask;

//cout << "shifted mantissa (+) = " << (mantissa >> pointPosition) << endl;
//cout << "shifted mantissa (-) = " << (mantissa << std::abs(F32(pointPosition))) << endl;

//cout << "intVal = " << intVal << endl;

    //  Create the mask covering the fractional bits of the fixed point value.
    U64 fracMask = U64((U64(1) << fracBits) - 1) << (64 - fracBits);

//cout << "fracBits = " << fracBits << " t = " << (1 << fracBits) << " s = " << ((1 << fracBits) - 1) << " r = " << (64 - fracBits) << endl;
//cout << "fracMask = " << fracMask << endl;

    //  Get the fractional bits of the value.
    S32 fractionalShift = 64 - pointPosition;
    fracVal = (pointPosition <= 0) ? 0 : ((fractionalShift >= 0) ? (U64(mantissa) << fractionalShift) : (U64(mantissa) >> (-fractionalShift)));
    fracVal = fracVal & fracMask;

//cout << "shifted Mantissa = " << (mantissa << (64 - pointPosition)) << endl;
//cout << "fracVal = " << fracVal << endl;

    //  Check for underflow.
    underflow = !overflow && (mantissa != 0) && (intVal == 0) && (fracVal == 0);

    if (overflow)
    {
//cout << "Overflow" << endl;
        intVal = (1 << (intBits -1)) - 1;
        fracVal = (1 << fracBits) - 1;
    }

    if (underflow)
    {
//cout << "Underflow" << endl;
        intVal = 0;
        fracVal = 0;
    }

    if (nan)
    {
//cout << "NaN" << endl;
        intVal = 0;
        fracVal = 0;
        sign = 0;
    }

    if (infinite)
    {
//cout << "infinite" << endl;
        intVal  = 0xffffffffffffffffULL;
        fracVal = 0xffffffffffffffffULL;
    }

//cout << dec;
//cout << "---- End of conversion ---- " << endl << endl;

}

//  Constructor:  32-bit integer point version.
FixedPoint::FixedPoint(S08 signP, U32 intValP, U32 fracValP, U32 intBitsP, U32 fracBitsP)
{
    sign = signP;
    intVal = intValP;
    fracVal = U64(fracValP) << 32;
    intBits = intBitsP;
    fracBits = fracBitsP;

    nan = false;
    infinite = false;
    overflow = false;
    underflow = false;
}

//  Constructor:  64-bit integer point version.
FixedPoint::FixedPoint(S08 signP, U64 intValP, U64 fracValP, U32 intBitsP, U32 fracBitsP)
{
    sign = signP;
    intVal = intValP;
    fracVal = fracValP;
    intBits = intBitsP;
    fracBits = fracBitsP;

    nan = false;
    infinite = false;
    overflow = false;
    underflow = false;
}

FixedPoint FixedPoint::operator+(const FixedPoint &b)
{
    U32 resIntBits = (intBits > b.intBits) ? intBits : b.intBits;
    U32 resFracBits = (fracBits > b.fracBits) ? fracBits : b.fracBits;

    U64 intMask = (U64(1) << resIntBits) - 1;
    U64 fracMask = (U64(U64(1) << resFracBits) - 1) << (64 - resFracBits);

    S08 resSign;
    U64 resFracVal;
    U64 resIntVal;

    if (sign == b.sign)
    {
        //  Shift fractional portion to the right by one bit to compute carry bit for
        //  the integral portion and then add the two fractional portions.
        resFracVal = (fracVal >> 1) + (b.fracVal >> 1);

        //  Add the integral portions with the carry from the fractional portions.
        resIntVal = (intVal + b.intVal + (resFracVal >> 63)) & intMask;

        //  Shift fractional portion to the left and bit apply mask.
        resFracVal = (resFracVal << 1) & fracMask;

        //  Both signs are the same.
        resSign = sign;
    }
    else
    {
        //  Determine the largest operand.
        if ((intVal > b.intVal) || ((intVal == b.intVal) && (fracVal >= b.fracVal)))
        {
            //  Shift fractional portion to the right by one bit to compute borrow bit for
            //  the integral portion and then substract the two fractional portions.
            resFracVal = (fracVal >> 1) - (b.fracVal >> 1);

            //  Substract the integral portions with the borrow from the fractional portions.
            resIntVal = (intVal -  b.intVal - (resFracVal >> 63)) & intMask;

            //  Shift fractional portion to the left and bit apply mask.
            resFracVal = (resFracVal << 1) & fracMask;

            //  The result has the sign of the largest operand.
            resSign = sign;
        }
        else
        {
            //  Shift fractional portion to the right by one bit to compute borrow bit for
            //  the integral portion and then substract the two fractional portions.
            resFracVal = (b.fracVal >> 1) - (fracVal >> 1);

            //  Substract the integral portions with the borrow from the fractional portions.
            resIntVal = (b.intVal -  intVal - (resFracVal >> 63)) & intMask;

            //  Shift fractional portion to the left and bit apply mask.
            resFracVal = (resFracVal << 1) & fracMask;

            //  The result has the sign of the largest operand.
            resSign = b.sign;
        }
    }

    return FixedPoint(resSign, resIntVal, resFracVal, resIntBits, resFracBits);
}

FixedPoint FixedPoint::operator-(const FixedPoint &b)
{
    U32 resIntBits = (intBits > b.intBits) ? intBits : b.intBits;
    U32 resFracBits = (fracBits > b.fracBits) ? fracBits : b.fracBits;

    S64 intMask = (U64(1) << resIntBits) - 1;
    U64 fracMask = (U64(U64(1) << resFracBits) - 1) << (64 - resFracBits);

    S08 resSign;
    U64 resFracVal;
    U64 resIntVal;

    if (sign != b.sign)
    {
        //  Shift fractional portion to the right by one bit to compute carry bit for
        //  the integral portion and then add the two fractional portions.
        resFracVal = (fracVal >> 1) + (b.fracVal >> 1);

        //  Add the integral portions with the carry from the fractional portions.
        resIntVal = (intVal + b.intVal + (resFracVal >> 63)) & intMask;

        //  Shift fractional portion to the left and bit apply mask.
        resFracVal = (resFracVal << 1) & fracMask;

        //  Both signs are the same.
        resSign = sign;
    }
    else
    {
        //  Determine the largest operand.
        if ((intVal > b.intVal) || ((intVal == b.intVal) && (fracVal >= b.fracVal)))
        {
            //  Shift fractional portion to the right by one bit to compute borrow bit for
            //  the integral portion and then substract the two fractional portions.
            U64 resFracVal = (fracVal >> 1) - (b.fracVal >> 1);

            //  Substract the integral portions with the borrow from the fractional portions.
            S64 resIntVal = (intVal - b.intVal - (resFracVal >> 63)) & intMask;

            //  Shift fractional portion to the left and bit apply mask.
            resFracVal = (resFracVal << 1) & fracMask;

            //  The result has the sign of the largest operand.
            resSign = sign;
        }
        else
        {
            //  Shift fractional portion to the right by one bit to compute borrow bit for
            //  the integral portion and then substract the two fractional portions.
            resFracVal = (b.fracVal >> 1) - (fracVal >> 1);

            //  Substract the integral portions with the borrow from the fractional portions.
            resIntVal = (b.intVal -  intVal - (resFracVal >> 63)) & intMask;

            //  Shift fractional portion to the left and bit apply mask.
            resFracVal = (resFracVal << 1) & fracMask;

            //  The result has the inverted sign of the largest operand.
            resSign = -b.sign;
        }
    }

    return FixedPoint(resSign, resIntVal, resFracVal, resIntBits, resFracBits);
}

FixedPoint FixedPoint::operator-()
{
    return FixedPoint(-sign, intVal, fracVal, intBits, fracBits);
}

FixedPoint FixedPoint::operator*(const FixedPoint &b)
{
//cout << "Mult A = ";
//this->dump();
//cout << " x B = ";
//b.dump();
//cout << endl;

//cout << hex;

//cout << "this.IntBits = " << intBits << " | this.fracBits = " << fracBits << endl;
//cout << "b.IntBits = " << b.intBits << " | b.fracBits = " << b.fracBits << endl;

    U32 resIntBits = (intBits > b.intBits) ? intBits : b.intBits;
    U32 resFracBits = (fracBits > b.fracBits) ? fracBits : b.fracBits;

//cout << "resIntBits = " << resIntBits << " | resFracBits = " << resFracBits << endl;

    S64 intMask = (U64(1) << resIntBits) - 1;
    U64 fracMask = (U64((U64(1) << resFracBits) - 1)) << (64 - resFracBits);

//cout << "intMask = " << intMask << " | fracMask = " << fracMask << endl;

    U64 multFrac0;
    U64 multFrac1;
    U64 multFrac2;
    U64 multFrac3;
    U64 multFrac4;
    U64 multInt1;
    U64 multInt2;
    U64 multInt3;
    U64 multInt4;

    mult64bit(fracVal, b.fracVal, multFrac1, multFrac0);
    mult64bit(fracVal, b.intVal, multInt2, multFrac2);
    mult64bit(intVal, b.fracVal, multInt3, multFrac3);
    mult64bit(intVal, b.intVal, multInt4, multInt1);

    multFrac4 = (multFrac1 >> 2) + (multFrac2 >> 2) + (multFrac3 >> 2);
    U64 multFracFinal = multFrac4 << 2;
    S64 multIntFinal = multInt1 + multInt2 + multInt3 + (multFrac4 >> 62);

//cout << "multFrac1 = " << multFrac1 << " | multFrac2 = " << multFrac2 << " | multFrac3 = " << multFrac3 << " | multFrac4 = " << multFrac4 << endl;
//cout << "multInt1 = " << multInt1 << " | multInt2 = " << multInt3 << " | multInt3 = " << multFrac3 << endl;
//cout << "multFrac = " << multFracFinal << " | multInt = " << multIntFinal << endl;

    U64 resFracVal = multFracFinal & fracMask;
    S64 resIntVal = multIntFinal & intMask;

    //  Multiply signs.
    S08 resSign = sign * b.sign;

//cout << "resFracVal = " << resFracVal << " | resIntVal = " << resIntVal << endl;

//cout << dec;

    return FixedPoint(resSign, resIntVal, resFracVal, resIntBits, resFracBits);
}

bool FixedPoint::operator>(const FixedPoint &b)
{
    //  Get the largest number of bits for integer and fractional values in both operands.
    U32 cmpFracBits = (fracBits > b.fracBits) ? fracBits : b.fracBits;
    U32 cmpIntBits = (intBits > b.intBits) ? intBits : b.intBits;

    //  Compare integer values (sign matters).
    S64 intMask = (U64(1) << cmpIntBits) - 1;

    if ( (S64(sign) * (S64(intVal) & intMask)) > (S64(b.sign) * (S64(b.intVal) & intMask)) )
        return true;

    //  Check if equal integer parts.
    if ( (sign == b.sign) && (S64(intVal) == S64(b.intVal)) )
    {
        //  Compare fractional values.

        //  Required shiftings for fractional values;
        U32 shiftFracA = cmpFracBits - fracBits;
        U32 shiftFracB = cmpFracBits - b.fracBits;

        if ( (U64(fracVal) << shiftFracA) > (U64(b.fracVal) << shiftFracB) )
            return true;
        else
            return false;
    }

    return false;
}

#define FP_FINFINITE 0x7F800000
#define FP_FNAN 0x7FD00000

F32 FixedPoint::toFloat32()
{
    F32 val;

    /*if (overflow)
        cout << "FixedPoint => toFloat32 -> overflow found" << endl;
    if (underflow)
        cout << "FixedPoint => toFloat32 -> underflow found" << endl;
    if (infinite)
        cout << "FixedPoint => toFloat32 -> infinite found" << endl;
    if (nan)
        cout << "FixedPoint => toFloat32 -> nan found" << endl;*/

    if ((overflow || underflow) || infinite)
        *((U32 *) &val) = FP_FINFINITE;
    else if (nan)
        *((U32 *) &val) = FP_FNAN;

    val = F32(sign) * F32(intVal) + F32(sign) * (F32(fracVal) / std::pow(2.0f, 64.0f));

    return val;
}

F64 FixedPoint::toFloat64()
{
    F64 val;

    val = F64(sign) * F64(intVal) + F64(sign) * (F64(fracVal) / std::pow(2.0f, 64.0f));

    return val;
}

FixedPoint FixedPoint::integer() const
{
    return FixedPoint(sign, intVal, 0, intBits, fracBits);
}

FixedPoint FixedPoint::fractional() const
{
    return FixedPoint(1, 0, fracVal, intBits, fracBits);
}

void FixedPoint::print() const
{
    cout << hex << intVal << "." << fracVal << dec;
}

void FixedPoint::dump() const
{
    cout << "-------" << endl;
    cout << hex << "intVal = " << intVal << endl;
    cout << "sign = " << S32(sign) << endl;
    cout << "fracVal = " << fracVal << endl;
    cout << "intBits = " << intBits << endl;
    cout << "fracBits = " << fracBits << endl;
    cout << "nan = " << nan << endl;
    cout << "infinite = " << infinite << endl;
    cout << "overflow = " << overflow << endl;
    cout << "underflow = " << underflow << endl;
    cout << "-------" << endl;
}

void FixedPoint::mult64bit(const U64 a, const U64 b, U64 &resHI, U64 &resLO)
{

//cout << " mult64bit -> input A = " << a << " input B = " << b << endl;

    U64 multRes1 = (a & 0x00000000FFFFFFFFULL) * (b & 0x00000000FFFFFFFFULL);
    U64 multRes2 = (a & 0x00000000FFFFFFFFULL) * (b >> 32);
    U64 multRes3 = (a >> 32) * (b & 0x00000000FFFFFFFFULL);
    U64 multRes4 = (a >> 32) * (b >> 32);

    U64 resLOWithCarry = (multRes1 >> 2) + ((multRes2 << 32) >> 2) + ((multRes3 << 32) >> 2);
    resLO = (resLOWithCarry << 2) + (multRes1 & 0x03);

    resHI = multRes4 + (multRes2 >> 32) + (multRes3 >> 32) + (resLOWithCarry >> 62);

//cout << "mult64bit -> output HI = " << resHI << " output LO = " << resLO << endl;
}



