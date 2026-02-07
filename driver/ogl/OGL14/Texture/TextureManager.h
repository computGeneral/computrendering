/**************************************************************************
 *
 */

#ifndef TEXTUREMANAGER_H
    #define TEXTUREMANAGER_H

#include "BaseManager.h"
#include "TextureObject.h"
#include <map>
#include "gl.h"
#include "TextureTarget.h"

namespace libgl
{

class TextureManager : public BaseManager
{

private:

    TextureManager(GLenum textureUnits);
    TextureManager(const TextureManager&);    
    
    static TextureManager* tm;


public:

    // Wraps cast
    TextureTarget& getTarget(GLenum target) const
    {
        return static_cast<TextureTarget&>(BaseManager::getTarget(target));
    }
    
    // Wraps cast
    TextureObject& bindObject(GLenum target, GLuint name)
    {
        return static_cast<TextureObject&>(BaseManager::bindObject(target, name));
    }
    
    // Wraps cast
    bool removeObjects(GLsizei n, const GLuint* names)
    {
        return BaseManager::removeObjects(n, names);
    }
    
    static TextureManager& instance();
                    
    void texParameteri( GLenum target, GLenum pname, GLint param);


};

} // namespace libgl

#endif // TEXTUREMANAGER_H
