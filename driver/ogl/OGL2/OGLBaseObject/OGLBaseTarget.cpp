/**************************************************************************
 *
 */

#include "OGLBaseTarget.h"
#include "support.h"

using namespace ogl;

BaseTarget::BaseTarget(GLuint target) : _target(target), _current(0), _def(0)
{
    // empty
}

GLuint BaseTarget::getName() const
{
    return _target;
}

BaseObject& BaseTarget::getCurrent() const
{
    if ( _current == 0 )
        CG_ASSERT("Current does not exist");

    return *_current;
}

bool BaseTarget::setCurrent(BaseObject& bo)
{
    _current = &bo;
    return true;
}

bool BaseTarget::resetCurrent()
{
    _current = 0;
    return true;
}

bool BaseTarget::hasCurrent() const
{
    return (_current != 0);
}

BaseTarget::~BaseTarget()
{
    delete _def;
}

void BaseTarget::setDefault(BaseObject* d)
{
    if ( d->getName() != 0 )
        CG_ASSERT("Default object must have name equal to zero");
    if ( _def != 0 )
        CG_ASSERT("this method cannot be called more than once");
    _def = d;
}
