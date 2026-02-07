/**************************************************************************
 *
 */

#ifndef TEXTURETARGET_H
    #define TEXTURETARGET_H

#include "BaseTarget.h"
#include "TextureObject.h"
#include <map>
#include "gl.h"

namespace libgl
{

class TextureTarget : public BaseTarget
{
private:

    GLenum envMode;
    
protected:
    
    TextureObject* createObject(GLuint name);
    
public:

    TextureTarget(GLuint target);    
    
    // Wraps cast
    TextureObject& getCurrent() const
    {
        return static_cast<TextureObject&>(BaseTarget::getCurrent());
    }
            
};

} // namespace libgl

#endif // TEXTURETARGET_H
