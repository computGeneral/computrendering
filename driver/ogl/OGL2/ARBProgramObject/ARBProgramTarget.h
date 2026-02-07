/**************************************************************************
 *
 */

#ifndef ARB_PROGRAMTARGET
    #define ARB_PROGRAMTARGET

#include "OGLBaseTarget.h"
#include "ARBProgramObject.h"

#include <map>
#include <string>

namespace ogl
{

class ARBProgramTarget : public BaseTarget
{
public:

    static const GLuint MaxEnvRegisters = MAX_PROGRAM_ENV_PARAMETERS_ARB;

private:
        
    // provides a default (0)
    ARBProgramObject* _def;

    ARBRegisterBank _envs;

    friend class ARBProgramManager; // Only program managers can create program targets
    ARBProgramTarget(GLenum target);

protected:

    ARBProgramObject* createObject(GLenum name);

public:



    ARBRegisterBank& getEnv();
    const ARBRegisterBank& getEnv() const;

    ARBProgramObject& getCurrent() const { return static_cast<ARBProgramObject&>(BaseTarget::getCurrent()); }
        
};


} // namespace libgl

#endif // ARB_PROGRAMTARGET
