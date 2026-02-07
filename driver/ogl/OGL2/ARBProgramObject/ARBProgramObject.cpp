/**************************************************************************
 *
 */

#include "ARBProgramObject.h"
#include "ARBProgramTarget.h"
#include "GALShaderProgram.h" // Include the GAL interface for shader programs

#include "support.h"

//#define ENABLE_GLOBAL_PROFILER
#include "GlobalProfiler.h"

#include <cstring>
#include "glext.h"

using namespace ogl;
using namespace std;
using namespace libGAL;

ARBProgramObject::ARBProgramObject(GLenum name, GLenum targetName) : BaseObject(name, targetName),
    _shader(0), _arbCompiledProgram(0), _format(0), _locals(MaxLocalRegisters), _sourceCompiled(false),
    _galDev(0)
{}

void ARBProgramObject::setSource(const string& arbCode, GLenum format)
{
    if ( arbCode == _source ) // ignore updates with the same code
        return ;

    _source = arbCode;
    _sourceCompiled = false; // New source requiring compilation
}

const string& ARBProgramObject::getSource() const // new version
{
    return _source;
}


void ARBProgramObject::attachDevice(GALDevice* device)
{
    _galDev = device;
}


GALShaderProgram* ARBProgramObject::compileAndResolve(libGAL::GALxFixedPipelineState* fpState)
{
    // Compilation process
    if ( !_sourceCompiled )
    {
        GALxDestroyCompiledProgram(_arbCompiledProgram); // Discard previous compilation
        GLOBAL_PROFILER_ENTER_REGION("compile ARB", "", "")
        _arbCompiledProgram = GALxCompileProgram(_source); // Generate the new compilation
        GLOBAL_PROFILER_EXIT_REGION()
        _sourceCompiled = true; // Source and binary are synchronized
    }

    // Create a shader program if it does not exist yet
    if ( !_shader ) {
        if ( !_galDev )
            CG_ASSERT("No GALDevice attached. GALShaderProgram object cannot be created");
        _shader = _galDev->createShaderProgram();
    }

    
    std::vector<GALx_STORED_FP_ITEM_ID> emptyList;
    GALxConstantBindingList cbList;
    
    // Resolve local params
    vector<gal_uint> params = _locals.getModified();
    GALxDirectSrcCopyBindFunction directCopyFunction;
    for ( gal_uint i = 0; i < params.size(); ++i ) {
        GALxFloatVector4 directSource;
        _locals.get(params[i], directSource[0], directSource[1], directSource[2], directSource[3]);        
        cbList.push_back( GALxCreateConstantBinding( GALx_BINDING_TARGET_LOCAL, 
                                       params[i], 
                                       emptyList,
                                       &directCopyFunction, 
                                       directSource) );
    }

    // Resolve environment params
    ARBRegisterBank& envs = static_cast<ARBProgramTarget&>(getTarget()).getEnv();
    params = envs.getModified();
    for ( gal_uint i = 0; i < params.size(); ++i ) {
        GALFloatVector4 directSource;
        envs.get(params[i], directSource[0], directSource[1], directSource[2], directSource[3]);
        cbList.push_back( GALxCreateConstantBinding( GALx_BINDING_TARGET_ENVIRONMENT, 
                                       params[i], 
                                       emptyList,
                                       &directCopyFunction, 
                                       directSource) );
    }

    // Resolve process
    GALxResolveProgram(fpState, _arbCompiledProgram, &cbList, _shader);

    GALxConstantBindingList::iterator it;
    it = cbList.begin();
    while (it != cbList.end())
    {
        GALxDestroyConstantBinding(*it);
        it++;
    }
    cbList.clear();
    
    // Return result shader
    return _shader;
}


ARBProgramObject::~ARBProgramObject()
{
    if ( _galDev ) {
        if ( _shader )
            _galDev->destroy(_shader);
    }
    if ( _arbCompiledProgram )
        GALxDestroyCompiledProgram(_arbCompiledProgram);
}

const char* ARBProgramObject::getStringID() const
{
    GLenum tname = getTargetName();
    
    if ( tname == GL_VERTEX_PROGRAM_ARB )
        return "VERTEX_PROGRAM";
    else if ( tname == GL_FRAGMENT_PROGRAM_ARB )
        return "FRAGMENT_PROGRAM";
    else
        return BaseObject::getStringID();
}

ARBRegisterBank& ARBProgramObject::getLocals()
{
    return _locals;
}

const ARBRegisterBank& ARBProgramObject::getLocals() const
{
    return _locals;
}
