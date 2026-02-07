/**************************************************************************
 *
 */

#include "OGLBaseObject.h"
#include "OGLBaseTarget.h"
#include "support.h"

#include <sstream>
#include <iostream>

using namespace ogl;
using namespace std;

BaseObject::BaseObject(GLuint name, GLenum targetName) : _name(name), _target(0), 
_targetName(targetName)
{}

GLuint BaseObject::getName() const 
{
    return _name;
}

GLenum BaseObject::getTargetName() const
{
    return _targetName;
}

void BaseObject::setTarget(BaseTarget& bt)
{
    if ( bt.getName() != _targetName )
        CG_ASSERT("Target not compatible");
        
    _target = &bt;
}

BaseTarget& BaseObject::getTarget() const
{    
    if ( _target == 0 )
    {
        stringstream ss;
        ss << "Object " << _name << " of type " << getStringID() << " not attached to any target yet";
        CG_ASSERT(ss.str().c_str());
    }
        
    return *_target;
}


BaseObject::~BaseObject()
{}



const char* BaseObject::getStringID() const
{
    return "UNKNOWN Base object";
}

string BaseObject::toString() const
{
    stringstream ss;
    ss << "BASE_OBJECT. Name = " << _name << std::hex << _targetName << std::dec;
    return ss.str();
}
