/**************************************************************************
 *
 * Texture Request implementation file.
 *
 */


/**
 *
 *  @file TextureRequest.cpp
 *
 *  This file implements the Texture Request class.
 *
 *  The Texture Request class carries texture access requests from
 *  the Shader Decode Execute unit to the Texture Unit.
 *
 */

#include "cmTextureRequest.h"

using namespace arch;

//  Texture Request constructor.  
TextureRequest::TextureRequest(TextureAccess *txAccess)
{
    //  Initialize Texture Request attributes.  
    textAccess = txAccess;

    setTag("TexReq");
}


//  Gets the texture coordinates.  
TextureAccess *TextureRequest::getTextAccess()
{
    return textAccess;
}
