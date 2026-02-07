/**************************************************************************
 *
 */

#ifndef OGL_TEXTURETARGET
    #define OGL_TEXTURETARGET

#include "gl.h"
#include "glext.h"
#include "OGLBaseTarget.h"
#include "OGLTextureObject.h"
#include "support.h"

namespace ogl
{

class GLTextureTarget : public BaseTarget
{
private:

    GLenum envMode;
    
protected:
    
    GLTextureObject* createObject(GLuint name);
    
public:

    GLTextureTarget(GLuint target);    
    
    GLTextureObject& getCurrent() const
    {
        return static_cast<GLTextureObject&>(BaseTarget::getCurrent());
    }
            
};

} // namespace libgl

#endif // OGL_TEXTURETARGET
