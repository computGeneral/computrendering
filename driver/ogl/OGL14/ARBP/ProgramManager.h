/**************************************************************************
 *
 */

#ifndef PROGRAMMANAGER_H
    #define PROGRAMMANAGER_H

#include "gl.h"
#include "BaseManager.h"
#include "ProgramTarget.h"

namespace libgl
{

class ProgramManager : public BaseManager
{

private:

    ProgramManager();
    ProgramManager(const ProgramManager&);

    static ProgramManager* pm;

public:

    static ProgramManager& instance();

    bool programString(GLenum target, GLenum format, GLsizei len, const void* program);

    // Wraps cast
    ProgramTarget& getTarget(GLenum target) const
    {
        return static_cast<ProgramTarget&>(BaseManager::getTarget(target));
    }
};

} // namespace libgl

#endif // PROGRAMMANAGER_H

