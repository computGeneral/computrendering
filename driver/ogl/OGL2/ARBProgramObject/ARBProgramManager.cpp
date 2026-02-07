/**************************************************************************
 *
 */

#include "ARBProgramManager.h"
#include "support.h"
#include "glext.h"

using namespace ogl;


ARBProgramManager::ARBProgramManager() 
{
    // Program objects define one group with two targets
    addTarget(new ARBProgramTarget(GL_VERTEX_PROGRAM_ARB));
    addTarget(new ARBProgramTarget(GL_FRAGMENT_PROGRAM_ARB));
}

ARBProgramTarget& ARBProgramManager::target(GLenum target) 
{ 
    return static_cast<ARBProgramTarget&>(getTarget(target));
}

void ARBProgramManager::programString(GLenum targetName, GLenum format, GLsizei len, const void* program)
{
    target(targetName).getCurrent().setSource(string((const char*)program, len), format);
}

