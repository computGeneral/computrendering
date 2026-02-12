/**************************************************************************
 *
 * Texture Unit State Info definition file.
 *
 */


#ifndef _TEXTUREUNITSTATEINFO_
#define _TEXTUREUNITSTATEINFO_

#include "DynamicObject.h"
#include "cmTextureProcessor.h"

namespace arch
{

/**
 *
 *  @file cmTextureUnitStateInfo.h
 *
 *  This file defines the TextureUnitStateInfo class that is used
 *  to carry state information from the Texture Unit to the Shader.
 *
 */

/**
 *
 *  This class defines a container for the state signals that the Texture Unit sends to the Shader.
 *  Inherits from Dynamic Object that offers dynamic memory support and tracing capabilities.
 *
 */


class TextureUnitStateInfo : public DynamicObject
{
private:

    TextureUnitState state;    //  The Texture Unit state.  

public:

    /**
     *
     *  Creates a new TextureUnitStateInfo object.
     *
     *  @param state The Texture Unit state carried by this  texture state info object.
     *
     *  @return A new initialized Texture Unit StateInfo object.
     *
     */

    TextureUnitStateInfo(TextureUnitState state);

    /**
     *
     *  Returns the texture unit state carried by the texture unit state info object.
     *
     *  @return The texture unit state carried in the object.
     *
     */

    TextureUnitState getState();
};

} // namespace arch

#endif
