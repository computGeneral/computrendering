/**************************************************************************
 *
 */

#ifndef VSLOADER_H
    #define VSLOADER_H

#include "ProgramObject.h"
#include "HAL.h"
#include "gl.h"
#include "glext.h"
//#include <map> // for mapping shaders 

namespace libgl
{
    
class GLContext;

/**
 * class used for loading vertex shaders in GPU
 *
 * In future it will include all T&L emulations via shaders
 */
class VSLoader
{
   
private:

    HAL* driver; // current driver
    GLContext* ctx;
        
    void resolveBank(ProgramObject& po, RBank<float>& b);
        
public:

    VSLoader(GLContext* ctx, ::HAL* driver);

    //VSLoader(HAL* driver, const glsNS::GLState& gls, TLState& tls, FPState& fps);
    
    void initShader(ProgramObject& po, GLenum target, GLContext* ctx);
    
    // Selects a shader and loads into GPU using driver interface 
    //U32 setupShader(bool useUserShader, GLenum target);
    
    ~VSLoader();
    
};

} // namespace libgl

#endif
