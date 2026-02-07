/**************************************************************************
 *
 */

#include "OGLTextureManager.h"
#include <iostream>


using namespace std;
using namespace ogl;

GLTextureManager* GLTextureManager::tm = 0;


// Create singleton 
GLTextureManager& GLTextureManager::instance()
{
    if ( !tm )
        tm = new GLTextureManager(16); // MAX_TEXTURE_UNITS_ARB groups
    return *tm;
}


GLTextureManager::GLTextureManager(GLenum textureUnits) : BaseManager(textureUnits)
{
    for ( GLuint i = 0; i < textureUnits; i++ )
    {
        // Add targets
        selectGroup(i);
        addTarget(new GLTextureTarget(GL_TEXTURE_1D));
        addTarget(new GLTextureTarget(GL_TEXTURE_2D));
        addTarget(new GLTextureTarget(GL_TEXTURE_3D));
        addTarget(new GLTextureTarget(GL_TEXTURE_CUBE_MAP));
        addTarget(new GLTextureTarget(GL_TEXTURE_RECTANGLE));
    }
    selectGroup(0);    
}
