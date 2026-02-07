/**************************************************************************
 *
 */

#ifndef OGL_BUFFERTARGET_H
    #define OGL_BUFFERTARGET_H

#include "OGLBaseTarget.h"
#include "OGLBufferObject.h"
#include "gl.h"

namespace ogl
{

class BufferTarget : public BaseTarget
{

protected:    

    BufferObject* createObject(GLuint name);

public:

    BufferTarget(GLenum target);
    
    BufferObject& getCurrent() const
    {
        return static_cast<BufferObject&>(BaseTarget::getCurrent());
    }

};

} // namespace libgl


#endif // BUFFERTARGET_H
