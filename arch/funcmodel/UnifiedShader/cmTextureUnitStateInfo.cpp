/**************************************************************************
 *
 * Texture Unit State Info implementation file.
 *
 */

/**
 *
 *  @file TextureUnitStateInfo.cpp
 *
 *  This file implements the Texture Unit State Info class.
 *  This class carries state information from the Texture Unit
 *  to the shader to which it is attached.
 *
 */

#include "cmTextureUnitStateInfo.h"

using namespace cg1gpu;

//  Creates a new TextureUnit State Info object.  
TextureUnitStateInfo::TextureUnitStateInfo(TextureUnitState newState) : state(newState)
{
    //  Set object color for tracing.  
    setColor(state);

    setTag("TexStIn");
}


//  Returns the texture unit state carried by the object.  
TextureUnitState TextureUnitStateInfo::getState()
{
    return state;
}
