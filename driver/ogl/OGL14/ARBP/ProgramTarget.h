/**************************************************************************
 *
 */

#ifndef PROGRAMTARGET_H
    #define PROGRAMTARGET_H

#include "BaseTarget.h"
#include "ProgramObject.h"
#include <map>
#include <string>

namespace libgl
{

class ProgramTarget : public BaseTarget
{
    
private:
    
    U32 fetchRate;
    
    // provides a default (0)
    ProgramObject* def;

    /**
     * Environment parameters
     */
    RBank<float> envParams;

    const char* programErrorString;

    GLint programErrorPosition; // -1 -> no errors 
    
protected:

    ProgramObject* createObject(GLenum name);

public:

    ProgramTarget(GLenum target);

    RBank<float>& getEnvironmentParameters();
    
    ProgramObject& getCurrent() const
    {
        return static_cast<ProgramObject&>(BaseTarget::getCurrent());
    }
        
};


} // namespace libgl

#endif // PROGRAMTARGET_H
