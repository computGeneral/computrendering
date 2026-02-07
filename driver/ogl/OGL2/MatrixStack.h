/**************************************************************************
 *
 */

#ifndef MATRIX_STACK
    #define MATRIX_STACK

#include "GALxGlobalTypeDefinitions.h"

namespace ogl
{

class MatrixStack
{
public:

    virtual void multiply(const libGAL::GALxFloatMatrix4x4& mat) = 0;

    virtual void set(const libGAL::GALxFloatMatrix4x4& mat) = 0;

    virtual libGAL::GALxFloatMatrix4x4 get() const = 0;

    virtual void push() = 0;

    virtual void pop() = 0;
};

}

#endif // MATRIX_STACK
