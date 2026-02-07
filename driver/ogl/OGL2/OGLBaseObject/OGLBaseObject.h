/**************************************************************************
 *
 */

#ifndef OGLBASEOBJECT
    #define OGLBASEOBJECT

#include "gl.h"
#include <vector>
#include <string>

namespace ogl
{

class BaseTarget;
    
class BaseObject
{
private:

    GLuint _name;
    BaseTarget* _target;
    const GLenum _targetName; // target type supported (cannot be changed)
   
    friend class BaseTarget;
    
protected:

    /**
     * Creates a BaseObject with a name and a targetName (type of target supported)
     *
     * This object can change its target to a new target with getName() == targetName
     */
    BaseObject(GLuint name, GLenum targetName);

public:

    //virtual bool AmIDefault() { return name == 0; }
    
    /**
     * Returns the BaseTarget type/name supported by this BaseObject
     */ 
    virtual GLenum getTargetName() const;
    
    /**
     * Returns the BaseObject name
     */
    virtual GLuint getName() const;

    /**
     * Returns the current BaseTarget assigned to this BaseObject
     */    
    virtual BaseTarget& getTarget() const;    
    
    /**
     * Sets a new BaseTarget that must have the same targetName than the previous BaseTarget
     * assigned to this BaseObject
     */
    virtual void setTarget(BaseTarget& bt);

    virtual ~BaseObject() = 0;

    virtual const char* getStringID() const;
    
    virtual std::string toString() const;
};    

} // namespace ogl
    
#endif // OGLBASEOBJECT
