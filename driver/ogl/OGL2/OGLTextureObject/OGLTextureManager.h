/**************************************************************************
 *
 */

#ifndef OGL_TEXTUREMANAGER
    #define OGL_TEXTUREMANAGER

#include "gl.h"
#include "glext.h"
#include "OGLBaseManager.h"
#include "OGLTextureTarget.h"
#include "OGLTextureObject.h"

namespace ogl
{

class GLTextureManager: public BaseManager
{

private:

    static GLTextureManager* tm;

public:

    GLTextureManager(GLenum textureUnits = 0);

    GLTextureManager(const GLTextureManager&);

    static GLTextureManager& instance();

    GLTextureTarget& getTarget(GLenum target) const
    {
        return static_cast<GLTextureTarget&>(BaseManager::getTarget(target));
    }
    
    /*GLTextureObject& bindObject(GLenum target, GLuint name)
    {
        return static_cast<GLTextureObject&>(BaseManager::bindObject(target, name));
    }*/
    
    bool removeObjects(GLsizei n, const GLuint* names)
    {
        return BaseManager::removeObjects(n, names);
    }           

}; // class GLTextureManager

} // namespace ogl

#endif // OGL_TEXTUREMANAGER
