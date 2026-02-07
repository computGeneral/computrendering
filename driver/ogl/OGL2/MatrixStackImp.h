/**************************************************************************
 *
 */

#ifndef MATRIX_STACK_IMP
    #define MATRIX_STACK_IMP

#include "MatrixStack.h"
#include "GALxFixedPipelineState.h"
#include "GALxTextCoordGenerationStage.h"
#include <stack>

namespace ogl
{

class MatrixStackProjection : public MatrixStack
{
public:

    MatrixStackProjection(libGAL::GALxTransformAndLightingStage* tl);

    void multiply(const libGAL::GALxFloatMatrix4x4& mat);

    void set(const libGAL::GALxFloatMatrix4x4& mat);

    libGAL::GALxFloatMatrix4x4 get() const;

    void push();

    void pop();

private:

    libGAL::GALxTransformAndLightingStage* _tl;
    std::stack<libGAL::GALxFloatMatrix4x4> _stack;
};
    
class MatrixStackModelview : public MatrixStack
{
public:

    MatrixStackModelview(libGAL::GALxTransformAndLightingStage* tl, libGAL::gal_uint unit);

    void multiply(const libGAL::GALxFloatMatrix4x4& mat);

    void set(const libGAL::GALxFloatMatrix4x4& mat);

    libGAL::GALxFloatMatrix4x4 get() const;

    void push();

    void pop();

private:

    libGAL::GALxTransformAndLightingStage* _tl;
    const libGAL::gal_uint _UNIT;
    std::stack<libGAL::GALxFloatMatrix4x4> _stack;
};


class MatrixStackTextureCoord : public MatrixStack
{
public:

    MatrixStackTextureCoord(libGAL::GALxTextCoordGenerationStage* tl, libGAL::gal_uint textureStage);

    void multiply(const libGAL::GALxFloatMatrix4x4& mat);

    void set(const libGAL::GALxFloatMatrix4x4& mat);

    libGAL::GALxFloatMatrix4x4 get() const;

    void push();

    void pop();

private:

    libGAL::GALxTextCoordGenerationStage* _tl;
    const libGAL::gal_uint _UNIT;
    std::stack<libGAL::GALxFloatMatrix4x4> _stack;
};

} // namespace ogl


#endif // MATRIX_STACK_IMP
