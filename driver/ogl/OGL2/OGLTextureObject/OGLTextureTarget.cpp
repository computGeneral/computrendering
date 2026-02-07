/**************************************************************************
 *
 */

#include "OGLTextureTarget.h"

using namespace ogl;

GLTextureTarget::GLTextureTarget(GLuint target) : BaseTarget(target)
{    
    if ( target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && 
         target != GL_TEXTURE_CUBE_MAP && target != GL_TEXTURE_RECTANGLE )
         CG_ASSERT("Unexpected texture target");
         
    GLTextureObject* defTex = new GLTextureObject(0, target); // create a default object
    defTex->setTarget(*this);
    setCurrent(*defTex);
    setDefault(defTex);
    
    /*****************************************
     * CONFIGURE DEFAULT TEXTURE OBJECT HERE *
     *****************************************/
}


GLTextureObject* GLTextureTarget::createObject(GLuint name)
{
    
    GLTextureObject* to = new GLTextureObject(name, getName());    
    to->setTarget(*this);
    return to;
}

