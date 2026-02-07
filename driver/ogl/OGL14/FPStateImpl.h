/**************************************************************************
 *
 */
 
#ifndef FPSTATEIMPL_H
    #define FPSTATEIMPL_H

#include "FPState.h"

namespace libgl
{

class FPStateImpl : public FPState
{

public:
    FPStateImpl();
    virtual ~FPStateImpl();

    bool fogEnabled();
    bool separateSpecular();
    bool anyTextureUnitEnabled();
    bool isTextureUnitEnabled(GLuint unit);
    TextureUnit getTextureUnit(GLuint unit);
    int maxTextureUnits();

};

} // namespace libgl

#endif // FPSTATEIMPL_H

