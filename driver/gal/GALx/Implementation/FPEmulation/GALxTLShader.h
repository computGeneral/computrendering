/**************************************************************************
 *
 */

#ifndef GALx_TLSHADER_H
    #define GALx_TLSHADER_H

#include <string>
#include <list>
#include "gl.h"
#include "glext.h"
#include "GALx.h"
#include <iostream>

namespace libGAL
{

/**
 * Interface for getting the program generation results.
 *
 * The implementor of this class is assumed to provided 
 */
class GALxTLShader
{
    std::string code;
    GLenum format;

    std::list<GALxConstantBinding*> constantBindings;

public:

    void setCode(const std::string& code_, GLenum format_ = GL_PROGRAM_FORMAT_ASCII_ARB)
    {
        code = code_;
        format = format_;
    }

    std::string getCode() { return code; }

    void addConstantBinding(libGAL::GALxConstantBinding* cb)    { constantBindings.push_back(cb); }

    std::list<GALxConstantBinding*> getConstantBindingList() {    return constantBindings;    }

    ~GALxTLShader()
    {
        std::list<GALxConstantBinding*>::iterator iter = constantBindings.begin();
    
        while ( iter != constantBindings.end() )
        {
            GALxDestroyConstantBinding(*iter);
            iter++;
        }
    }
};

} // namespace libGAL

#endif // GALx_TLSHADER_H
