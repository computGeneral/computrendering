/**************************************************************************
 *
 */

#include "OGLBufferTarget.h"

using namespace ogl;

BufferTarget::BufferTarget(GLuint target) : BaseTarget(target)
{
    resetCurrent(); // no current (unbound)
}

BufferObject* BufferTarget::createObject(GLuint name)
{

    BufferObject* bo = new BufferObject(name, getName());
    bo->setTarget(*this);

    return bo;
}

