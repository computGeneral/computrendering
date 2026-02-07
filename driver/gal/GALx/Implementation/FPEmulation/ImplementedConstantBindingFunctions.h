/**************************************************************************
 *
 */

#ifndef IMPLEMENTED_CONSTANT_BINDING_FUNCTIONS_H
    #define IMPLEMENTED_CONSTANT_BINDINGS_FUNCTIONS_H

#include "GALxConstantBinding.h"
#include <cmath>

namespace libGAL
{

/**
 * Function for rescaling factor computing
 */
class ScaleFactorFunction: public libGAL::GALxBindingFunction
{
public:

    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        libGAL::GALxFloatMatrix4x4 mat, invMat;
        
        mat[0] = vState[0];    mat[1] = vState[1];    mat[2] = vState[2];    mat[3] = vState[3];

        libGAL::_inverse(invMat,mat);
        
        constant = 1 / libGAL::_length(invMat[2]); // Without indexing assigns the value to all the components
    };
};

/**
 * Function for light position vector normalization
 */
class LightPosNormalizeFunction: public libGAL::GALxBindingFunction
{
public:

    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        vState[0][3] = 0.0f;
        constant = libGAL::_normalize(vState[0]);
        constant[3] = 0.0f;
    };

};

/**
 * Function for linear fog parameters computation
 *
 * @note The constant vector needed c = {-1/(END-START), END/(END-START), NOT USED, NOT USED};
 */
class LinearFogParamsFunction: public libGAL::GALxBindingFunction
{
public:
    
    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        constant[0] = libGAL::gal_float(-1) / (vState[1][0] - vState[0][0]);
        constant[1] = vState[1][0]/(vState[1][0] - vState[0][0]); //constant[0] * vState[1][0];
    };
};

/**
 * Function for exponential fog parameters computation
 *
 * @note The constant vector needed c = {DENSITY/LN(2), NOT USED, NOT USED, NOT USED};
 */
class ExponentialFogParamsFunction: public libGAL::GALxBindingFunction
{
public:
    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        constant[0] =  vState[0][0] / libGAL::gal_float(std::log(2.0));
    };
};

/**
 * Function for second order exponential fog parameters computation
 *
 * @note The constant vector needed c = {DENSITY/SQRT(LN(2)), NOT USED, NOT USED, NOT USED};
 */
class SecondOrderExponentialFogParamsFunction: public libGAL::GALxBindingFunction
{
public:
    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        constant[0] =  vState[0][0] / libGAL::gal_float(std::sqrt(std::log(2.0)));
    };
};

/**
 * Function to copy single matrix rows to 
 */
class CopyMatrixRowFunction: public libGAL::GALxBindingFunction
{
private:

    libGAL::gal_uint _row;

public:

    CopyMatrixRowFunction(libGAL::gal_uint row): _row(row) {};

    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        constant = vState[_row];
    };
};

/**
 * Function to replicate the Alpha test reference value to all the
 * vector components.
 */
class AlphaRefValueFunction: public libGAL::GALxBindingFunction
{
public:

    virtual void function(libGAL::GALxFloatVector4& constant, 
                          std::vector<libGAL::GALxFloatVector4> vState,
                          const libGAL::GALxFloatVector4&) const
    {
        constant[0] = constant[1] = constant[2] = constant[3] = vState[0][0];
    };

};

} // namespace libGAL

#endif // IMPLEMENTED_CONSTANT_BINDINGS_FUNCTIONS_H
