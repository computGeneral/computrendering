/**************************************************************************
 *
 * Texture Result implementation file.
 *
 */


/**
 *
 *  @file TextureResult.cpp
 *
 *  This file implements the Texture Result class.
 *
 *  The Texture Result class carries texture sample results from the Texture Unit to
 *  the Shader Decode Execute Unit.
 *
 */

#include "cmTextureResult.h"

using namespace arch;

//  Texture Result constructor.  
TextureResult::TextureResult(U32 ident, Vec4FP32 *samples, U32 stampFrags, U64 cycle)
{
    U32 i;

    //  Set texture access identifier.  
    id = ident;

    //  Set texture access start cycle.  
    startCycle = cycle;

    //  Allocate space for the result.  
    textSamples = new Vec4FP32[stampFrags];

    //  Copy result texture samples.  
    for(i = 0; i < stampFrags; i++)
        textSamples[i] = samples[i];

    setTag("TexRes");
}

//  Gets the texture access id for the result.  
U32 TextureResult::getTextAccessID()
{
    return id;
}

//  Gets the texture samples.  
Vec4FP32 *TextureResult::getTextSamples()
{
    return textSamples;
}

//  Texture Result destructor.  
TextureResult::~TextureResult()
{
    delete[] textSamples;
}

//  Returns the texture access start cycle for the texture result.  
U64 TextureResult::getStartCycle()
{
        return startCycle;
}
