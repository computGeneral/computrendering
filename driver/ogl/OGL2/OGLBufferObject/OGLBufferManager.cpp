/**************************************************************************
 *
 */

#include "OGLBufferManager.h"
#include "support.h"
#include "glext.h"

using namespace ogl;

BufferManager* BufferManager::bom = 0;

// Create singleton 
BufferManager& BufferManager::instance()
{
    if ( !bom )
        bom = new BufferManager;
    return *bom;
}

BufferManager::BufferManager()
{
    // Create targets
    addTarget(new BufferTarget(GL_ARRAY_BUFFER));
    addTarget(new BufferTarget(GL_ELEMENT_ARRAY_BUFFER));
}

BufferObject& BufferManager::getBuffer(GLenum target, GLenum name)
{
    BufferObject* bo = static_cast<BufferObject*>(findObject(target, name));
    if ( !bo )
        CG_ASSERT("Buffer object not found");
    if ( bo->getTargetName() != target )
        CG_ASSERT("target does not match with target name of buffer object looked for");
    
    return *bo;
}
