/**************************************************************************
 *
 */

#include "TextureTarget.h"

using namespace libgl;

TextureTarget::TextureTarget(GLuint target) : BaseTarget(target)
{    
    if ( target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && 
         target != GL_TEXTURE_CUBE_MAP )
         CG_ASSERT("Unexpected texture target");
         
    TextureObject* defTex = new TextureObject(0, target); // create a default object
    defTex->setTarget(*this);
    setCurrent(*defTex);
    setDefault(defTex);
    
    /*****************************************
     * CONFIGURE DEFAULT TEXTURE OBJECT HERE *
     *****************************************/
}


TextureObject* TextureTarget::createObject(GLuint name)
{
    
    TextureObject* to = new TextureObject(name, getName());    
    to->setTarget(*this);
    return to;
}

