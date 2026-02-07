/**************************************************************************
 *
 */

#ifndef OGL_BUFFEROBJECT_H
    #define OGL_BUFFEROBJECT_H

#include "OGLBaseObject.h"
#include "GALBuffer.h"
#include "GALDevice.h"
#include "GPUType.h"
#include "gl.h"

namespace ogl
{

class BufferTarget;

class BufferObject : public BaseObject
{

private:    
          
    libGAL::GALBuffer* _data; // asociated data 

    libGAL::GALDevice* _galDev;
    
public:     

    BufferObject(GLuint name, GLenum targetName);
    
    void attachDevice(libGAL::GALDevice* device);

    GLenum getUsage() const;
    
    libGAL::GALBuffer* getData();

    GLsizei getSize();
    
    void setContents(GLsizei size, const GLubyte* data, GLenum usage);
    
    void setContents(GLsizei size, GLenum usage); // change just size & usage (discard previous data)
   
    void setPartialContents(GLsizei offset, GLsizei size, const void* data);

    libGAL::GAL_USAGE getGALUsage (GLenum usage);
    
    ~BufferObject();
    
};

} // namespace libgl

#endif // BUFFEROBJECT_H
