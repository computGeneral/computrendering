/**************************************************************************
 *
 * Texture Result definition file.
 *
 */

/**
 *
 *  @file cmTextureResult.h
 *
 *  Defines the Texture Result class.  This class carries texture access sample results from
 *  the Texture Unit to the Shader Decode Execute unit.
 *
 */

#include "GPUType.h"
#include "DynamicObject.h"

#ifndef _TEXTURERESULT_

#define _TEXTURERESULT_

namespace arch
{

/**
 *
 *  Defines a texture access sample result from the Texture Unit to the Shader Decode Execute Unit.
 *
 *  This class inherits from the DynamicObject class which offers dynamic memory
 *  management and tracing support.
 *
 *
 */


class TextureResult : public DynamicObject
{

private:

    U32 id;                  //  Texture Access identifier.  
    Vec4FP32 *textSamples;     //  The result texture samples.  
    U64 startCycle;          //  Cycle at which the texture access was issued to the Texture Unit.  

public:

    /**
     *
     *  Class constructor.
     *
     *  Creates and initializes a new Texture Result object.
     *
     *  @param id Identifier of the Texture Access.
     *  @param samples Pointer to a Vec4FP32 array with the result texture samples
     *  for the fragments in a stamp.
     *  @param cycle Cycle at which the texture access that produced the texture result was
     *  issued to the Texture Unit.
     *
     *  @return A new Texture Result object.
     *
     */

    TextureResult(U32 id, Vec4FP32 *sample, U32 stampFrags, U64 cycle);

    /**
     *
     *  Class destructor.
     *
     */

    ~TextureResult();

    /**
     *
     *  Returns the texture access identifier.
     *
     *  @return The texture access id.
     *
     */

    U32 getTextAccessID();

    /**
     *
     *  Returns the result texture samples.
     *
     *  @return A pointer to the texture samples for the fragments in a stamp.
     *
     */

    Vec4FP32 *getTextSamples();

    /**
     *
     *  Returns the cycle at which the texture access was started.
     *
     *  @return The cycle at which the texture access was issued to the Texture Unit.
     *
     */

    U64 getStartCycle();


};

} // namespace arch

#endif
