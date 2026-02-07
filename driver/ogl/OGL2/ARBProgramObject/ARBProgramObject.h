/**************************************************************************
 *
 */

#ifndef ARB_PROGRAMOBJECT
    #define ARB_PROGRAMOBJECT


#include "gl.h"

#include "OGLBaseObject.h"
#include "ARBRegisterBank.h"
#include "GALShaderProgram.h"
#include "ARBImplementationLimits.h"

#include "GALDevice.h"
#include "GALx.h"

#include <string>
#include <set>

namespace ogl
{

class ARBProgramTarget;

class ARBProgramObject : public BaseObject
{

public:

    static const GLuint MaxLocalRegisters = 23;//MAX_PROGRAM_LOCAL_PARAMETERS_ARB;

private:

    // Program Object info
    std::string _source; ///< The program string defined through the glProgramString() call.
    GLenum _format; ///< The program string format. GL_PROGRAM_FORMAT_ASCII_ARB only supported.
    
    // The binary CG1 program
    libGAL::GALShaderProgram* _shader;

    // The compiled arb program exposed by the GALx
    libGAL::GALxCompiledProgram* _arbCompiledProgram;

    libGAL::GALDevice* _galDev;

    bool _sourceCompiled;

    ARBRegisterBank _locals;

    friend class ARBProgramTarget;

    ARBProgramObject(GLenum name, GLenum targetName);

public:

    void attachDevice(libGAL::GALDevice* device);

    void restartLocalTracking();
    //const std::vector<GLuint>& getLocalsChanged() const; 
                
    //void setSource(const GLvoid* arbProgram, GLuint arbProgramSize, GLenum format);
    //bool getSource(GLubyte* arbProgram, GLuint& arbProgramSize) const;
    void setSource(const std::string& arbCode, GLenum format);
    const std::string& getSource() const;

    // Access to local params register bank
    ARBRegisterBank& getLocals();
    const ARBRegisterBank& getLocals() const;

    // Force program compilation if the program is not compiled
    libGAL::GALShaderProgram* compileAndResolve(libGAL::GALxFixedPipelineState* fpState);

    // Check if real compilation is required
    bool isCompiled();

    const char* getStringID() const;
        
    ~ARBProgramObject();
};

} // namespace libgl


#endif // ARB_PROGRAMOBJECT
