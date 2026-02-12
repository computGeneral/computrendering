/**************************************************************************
 *
 * Texture Request definition file.
 *
 */

/**
 *
 *  @file cmTextureRequest.h
 *
 *  Defines the Texture Request class.  This class describes a request from
 *  the Shader Decode Execute unit to the Texture Unit.
 *
 */

#include "GPUType.h"
#include "DynamicObject.h"
#include "bmTextureProcessor.h"

#ifndef _TEXTUREREQUEST_

#define _TEXTUREREQUEST_

namespace arch
{

/**
 *
 *  Defines a texture access request from the Shader Decode Execute Unit
 *  to the Texture Unit.
 *
 *  This class inherits from the DynamicObject class which offers
 *  dynamic memory management and tracing support.
 *
 *
 */


class TextureRequest : public DynamicObject
{

private:

    TextureAccess *textAccess;      //  Pointer to the texture access carried by the object.  

public:

    /**
     *
     *  Class constructor.
     *
     *  Creates and initializes a new Texture Request object.
     *
     *  @param txAccess Pointer to a Texture Access to be carried by the object from
     *  the shader to the texture unit.
     *
     *  @return A new Texture Request object.
     *
     */

    TextureRequest(TextureAccess *txAccess);

    /**
     *
     *  Returns the texture access being carried by the object.
     *
     *  @return A pointer to the texture access being carried by the object.
     *
     */

    TextureAccess *getTextAccess();

};

} // namespace arch

#endif
