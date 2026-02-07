/**************************************************************************
 *
 */

#ifndef BUFFERMANAGER_H
    #define BUFFERMANAGER_H

#include "BaseManager.h"
#include "gl.h"
#include "BufferTarget.h"

namespace libgl
{

class BufferManager : public BaseManager
{

private:

    BufferManager();
    BufferManager(const BufferManager&);
    
    static BufferManager* bom;
    
public:

    BufferObject& getBuffer(GLenum target, GLenum name);

    BufferTarget& getTarget(GLenum target) const
    {
        return static_cast<BufferTarget&>(BaseManager::getTarget(target));
    }    
    
    static BufferManager& instance();
    
    bool removeObjects(GLsizei n, const GLuint* names)
    {
        return BaseManager::removeObjects(n, names);
    }
    
};

} // namespace libgl

#endif // BUFFERMANAGER_H
