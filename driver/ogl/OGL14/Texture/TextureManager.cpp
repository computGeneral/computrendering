/**************************************************************************
 *
 */

#include "TextureManager.h"
#include "support.h"
#include "ImplementationLimits.h"
#include <iostream>


using namespace std;
using namespace libgl;

TextureManager* TextureManager::tm = 0;


// Create singleton 
TextureManager& TextureManager::instance()
{
    if ( !tm )
        tm = new TextureManager(MAX_TEXTURE_UNITS_ARB); // MAX_TEXTURE_UNITS_ARB groups
    return *tm;
}


TextureManager::TextureManager(GLenum textureUnits) : BaseManager(textureUnits)
{
    for ( GLuint i = 0; i < textureUnits; i++ )
    {
        // Add targets
        selectGroup(i);
        addTarget(new TextureTarget(GL_TEXTURE_1D));
        addTarget(new TextureTarget(GL_TEXTURE_2D));
        addTarget(new TextureTarget(GL_TEXTURE_3D));
        addTarget(new TextureTarget(GL_TEXTURE_CUBE_MAP));
    }
    selectGroup(0);    
}

