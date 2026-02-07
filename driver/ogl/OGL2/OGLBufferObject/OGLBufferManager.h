/**************************************************************************
 *
 */

#ifndef OGL_BUFFERMANAGER_H
    #define OGL_BUFFERMANAGER_H

#include "OGLBaseManager.h"
#include "OGLBufferTarget.h"
#include "gl.h"

namespace ogl
{

class BufferManager : public BaseManager
{

private:

    
    
    static BufferManager* bom;
    
public:

    BufferManager();
    BufferManager(const BufferManager&);

    BufferObject& getBuffer(GLenum target, GLenum name);

    BufferTarget& target(GLenum target) const
    {
        return static_cast<BufferTarget&>(BaseManager::getTarget(target));
    }    
    
    static BufferManager& instance();
    
    bool removeObjects(GLsizei n, const GLuint* names)
    {
        return BaseManager::removeObjects(n, names);
    }
    
};

}

#endif // BUFFERMANAGER_H
