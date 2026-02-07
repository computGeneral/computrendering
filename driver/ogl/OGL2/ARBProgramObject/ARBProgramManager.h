/**************************************************************************
 *
 */

#ifndef ARB_PROGRAMMANAGER
    #define ARB_PROGRAMMANAGER

#include "gl.h"
#include "OGLBaseManager.h"
#include "ARBProgramTarget.h"
#include "ARBProgramObject.h"

namespace ogl
{

class ARBProgramManager : public BaseManager
{
private:

    ARBProgramManager(const ARBProgramManager&);
    ARBProgramManager& operator=(const ARBProgramManager&);

public:

    ARBProgramManager();

    // Helper function to set the source code of the current program of the specified target
    void programString(GLenum target, GLenum format, GLsizei len, const void* program);

    ARBProgramTarget& target(GLenum target);

};

} // namespace ogl

#endif // ARB_PROGRAMMANAGER
