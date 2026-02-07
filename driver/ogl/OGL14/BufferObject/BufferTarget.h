/**************************************************************************
 *
 */

#ifndef BUFFERTARGET_H
    #define BUFFERTARGET_H

#include "BaseTarget.h"
#include "BufferObject.h"
#include "gl.h"

namespace libgl
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
