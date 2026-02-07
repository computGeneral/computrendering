/**************************************************************************
 *
 */

#include "OGLBufferObject.h"
#include "OGLBufferTarget.h"
#include "glext.h"
#include <iostream>

using namespace ogl;
using namespace std;

void BufferObject::attachDevice(libGAL::GALDevice* device)
{
    if (_galDev == 0)
    {
        _galDev = device;
        _data = _galDev->createBuffer();
    }

}

BufferObject::BufferObject(GLuint name, GLenum targetName) : BaseObject(name, targetName)
{
    _galDev = 0;
}

GLenum BufferObject::getUsage() const
{
    return _data->getUsage();
}

GLsizei BufferObject::getSize()
{
    return _data->getSize();
}

void BufferObject::setContents(GLsizei size, const GLubyte* data, GLenum usage)
{
    if ( size == 0 )
    {
        CG_ASSERT("Size cannot be 0");
        return ;
    }

    _data->clear();

    _data->setUsage(getGALUsage(usage));

   
    if (data == 0) _data->resize(size, true);
    else 
    {
        _data->clear();
        _data->pushData(data, size);
    }   
}

void BufferObject::setContents(GLsizei size, GLenum usage)
{
    setContents(size, 0, usage);
}

void BufferObject::setPartialContents(GLsizei offset, GLsizei subSize, const void* subData)
{

    if ( _data->getSize() == 0 )
    {
        _galDev->destroy(_data);
        _data = _galDev->createBuffer(subSize + offset, 0);
        //CG_ASSERT("Buffer Object undefined yet");
    }


    if ( subSize + offset > _data->getSize())
    {
        _data->resize(subSize+offset, true);
        //CG_ASSERT("size + offset out of range!");
    }

     _data->updateData(offset, subSize, (const libGAL::gal_ubyte*) subData);
}


libGAL::GALBuffer* BufferObject::getData()
{
    return _data;
}

libGAL::GAL_USAGE BufferObject::getGALUsage (GLenum usage)
{
    switch(usage)
    {
        case (GL_STATIC_DRAW_ARB):
        case (GL_STATIC_READ_ARB):
        case (GL_STATIC_COPY_ARB):
            return libGAL::GAL_USAGE_STATIC;
            break;

        case (GL_DYNAMIC_DRAW_ARB):
        case (GL_DYNAMIC_READ_ARB):
        case (GL_DYNAMIC_COPY_ARB):
        case (GL_STREAM_DRAW_ARB):
        case (GL_STREAM_READ_ARB):
        case (GL_STREAM_COPY_ARB):
            return libGAL::GAL_USAGE_DYNAMIC;
            break;

        default:
            CG_ASSERT("Type not supported yet");
            return static_cast<libGAL::GAL_USAGE>(0);
    }
}

BufferObject::~BufferObject()
{

}
