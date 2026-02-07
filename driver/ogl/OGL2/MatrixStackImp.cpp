/**************************************************************************
 *
 */

#include "MatrixStackImp.h"
#include "support.h"

using namespace ogl;
using namespace libGAL;

MatrixStackProjection::MatrixStackProjection(GALxTransformAndLightingStage* tl) : _tl(tl)
{}

void MatrixStackProjection::multiply(const libGAL::GALxFloatMatrix4x4& mat)
{
    GALxFloatMatrix4x4 newMatrix = _tl->getProjectionMatrix() * mat;
    _tl->setProjectionMatrix(newMatrix);
}

void MatrixStackProjection::set(const libGAL::GALxFloatMatrix4x4& mat)
{
    _tl->setProjectionMatrix(mat);
}

GALxFloatMatrix4x4 MatrixStackProjection::get() const
{
    return _tl->getProjectionMatrix();
}

void MatrixStackProjection::push()
{
    _stack.push(_tl->getProjectionMatrix());
}

void MatrixStackProjection::pop()
{
    if ( _stack.empty() )
        CG_ASSERT("Stack pop method failed. Stack is empty");

    _tl->setProjectionMatrix(_stack.top());
    _stack.pop(); // discard top
}


MatrixStackModelview::MatrixStackModelview(GALxTransformAndLightingStage* tl, gal_uint unit) : _tl(tl), _UNIT(unit)
{}

void MatrixStackModelview::multiply(const libGAL::GALxFloatMatrix4x4& mat)
{
    GALxFloatMatrix4x4 newMatrix = _tl->getModelviewMatrix(_UNIT) * mat;
    _tl->setModelviewMatrix(_UNIT, newMatrix);
}

void MatrixStackModelview::set(const libGAL::GALxFloatMatrix4x4& mat)
{
    _tl->setModelviewMatrix(_UNIT, mat);
}

GALxFloatMatrix4x4 MatrixStackModelview::get() const
{
    return _tl->getModelviewMatrix(_UNIT);
}

void MatrixStackModelview::push()
{
    _stack.push(_tl->getModelviewMatrix(_UNIT));
}

void MatrixStackModelview::pop()
{
    if ( _stack.empty() )
        CG_ASSERT("Stack pop method failed. Stack is empty");

    _tl->setModelviewMatrix(_UNIT, _stack.top());
    _stack.pop(); // discard top
}


MatrixStackTextureCoord::MatrixStackTextureCoord(GALxTextCoordGenerationStage* tl, gal_uint unit) : _tl(tl), _UNIT(unit)
{}

void MatrixStackTextureCoord::multiply(const libGAL::GALxFloatMatrix4x4& mat)
{
    GALxFloatMatrix4x4 newMatrix = _tl->getTextureCoordMatrix(_UNIT) * mat;
    _tl->setTextureCoordMatrix(_UNIT, newMatrix);
}

void MatrixStackTextureCoord::set(const libGAL::GALxFloatMatrix4x4& mat)
{
    _tl->setTextureCoordMatrix(_UNIT, mat);
}

GALxFloatMatrix4x4 MatrixStackTextureCoord::get() const
{
    return _tl->getTextureCoordMatrix(_UNIT);
}

void MatrixStackTextureCoord::push() 
{ 
    _stack.push(_tl->getTextureCoordMatrix(_UNIT));
}

void MatrixStackTextureCoord::pop()
{
    if ( _stack.empty() )
        CG_ASSERT("Stack pop method failed. Stack is empty");

    _tl->setTextureCoordMatrix(_UNIT, _stack.top());
    _stack.pop(); // discard top
}
