/**************************************************************************
 *
 */

#ifndef GALx_H
    #define GALx_H

#include "GALDevice.h"
#include "GALShaderProgram.h"

#include "GALxGlobalTypeDefinitions.h"
#include "GALxFixedPipelineState.h"
#include "GALxFixedPipelineSettings.h"
#include "GALxConstantBinding.h"

#include <string>

/**
 * The libGAL namespace containing all the GAL interface classes and methods.
 */
namespace libGAL
{

/**
 * Define the vertex input semantic of the automatically generated vertex shaders
 */
enum GALx_VERTEX_ATTRIBUTE_MAP
{
    GALx_VAM_VERTEX = 0,
    GALx_VAM_WEIGHT = 1,
    GALx_VAM_NORMAL = 2,
    GALx_VAM_COLOR = 3,
    GALx_VAM_SEC_COLOR = 4,
    GALx_VAM_FOG = 5,
    GALx_VAM_SEC_WEIGHT = 6,
    GALx_VAM_THIRD_WEIGHT = 7,
    GALx_VAM_TEXTURE_0 = 8,
    GALx_VAM_TEXTURE_1 = 9,
    GALx_VAM_TEXTURE_2 = 10,
    GALx_VAM_TEXTURE_3 = 11,
    GALx_VAM_TEXTURE_4 = 12,
    GALx_VAM_TEXTURE_5 = 13,
    GALx_VAM_TEXTURE_6 = 14,
    GALx_VAM_TEXTURE_7 = 15,
};

/**
 * GALx utility/extension library
 * 
 * Utility library build on top of the GAL library that provides some
 * useful extended functions.
 *
 * This library provides methods and interfaces for:
 *
 *    1. Fixed Pipeline State Management. Compatible with both OpenGL큦 and 
 *       D3D9큦 Fixed Function.
 *
 *    2. Fixed Pipeline emulation through state-based generated 
 *       shader programs. Compatible with both OpenGL큦 and D3D9큦 
 *       Fixed Function.
 *
 *    3. Shader program compilation and constant resolving. Compatible
 *       with the OpenGL ARB vertex/fragment program 1.0 specification.
 *
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @version 1.0
 * @date 04/10/2007
 */

///////////////////////////////////////////////
// Fixed Pipeline State Management functions //
///////////////////////////////////////////////

/**
 * Creates a fixed pipeline state interface.
 *
 * @param gal_device The GAL rendering device.
 * @returns             The new created Fixed Pipeline state.
 */
GALxFixedPipelineState* GALxCreateFixedPipelineState(const GALDevice* gal_device);

/**
 * Releases a GALxFixedPipelineState interface object.
 *
 * @param fpState The GALxFixedPipelineState interface to release.
 */
void GALxDestroyFixedPipelineState(GALxFixedPipelineState* fpState);


/////////////////////////////////////
// Fixed Pipeline Shader Emulation //
/////////////////////////////////////

/**
 * Initializes a GALx_FIXED_PIPELINE_SETTINGS struct with the default
 * values, as follows:
 *
 * 1. Lighting disabled.
 *
 * 2. Local viewer is disabled (infinite viewer).
 *
 * 3. The normal normalization mode is "no normalize".
 *
 * 4. Separate specular color is disabled.
 *
 * 5. Lighting of both faces mode is disabled.
 *
 * 6. All the lights are disabled and the default type is directional.
 *
 * 7. Back face culling is enabled.
 *
 * 8. Color Material is disabled and the default mode is emission.
 *
 * 9. FOG is disabled, the default coordinate source is fragment depth and the
 *    default FOG mode is linear.
 *
 * 10. All the texture stages have a identity texture coordinate generation matrix
 *     and all the texture generate modes are set to vertex attrib.
 *
 * 11. All the texture stages are disabled, the active texture target is 2D, the
 *     default texture stage function is modulate and the base internal format is RGB.
 *       For the combine function mode the default parameters are:
 *       11.1 RGB and ALPHA combine function are "modulate".
 *     11.2 [COMPLETE]
 *         
 */
void GALxLoadFPDefaultSettings(GALx_FIXED_PIPELINE_SETTINGS& fpSettings);


/**
 * Generates the compiled vertex program that emulate the Fixed 
 * Pipeline based on the input struct and resolves the constant 
 * parameters according to the Fixed Pipeline state.
 *
 * @param  fpState           The Fixed Pipeline state interface.
 * @param  fpSettings       The input struct with the Fixed Pipeline settings.
 * @retval vertexProgram   Pointer to the result vertex program.
 *
 * @code
 *
 *  // Get the GAL device
 *  GALDevice* gal_device = getGALDevice();
 *
 *  // Get the shader programs
 *  GALShaderProgram* vertexProgram = gal_device->createShaderProgram();
 *
 *  GALxFixedPipelineState* fpState = GALxCreateFixedPipelineState(gal_device);
 *
 *  GALx_FIXED_PIPELINE_SETTINGS fpSettings;
 *
 *  GALxLoadFPDefaultSettings(fpSettings);
 *
 *  // Set FP State configuration
 *
 *  fpSettings.alphaTestEnabled = true;
 *  fpSettings.lights[0].enabled = true;
 *
 *  // ....
 *
 *  GALxGenerateVertexProgram(fpState, fpSettings, vertexProgram);
 *
 *  gal_device->setVertexShader(vertexProgram);
 *
 * @endcode
 */
void GALxGenerateVertexProgram(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &vertexProgram);


/**
 * Generates the compiled fragment program that emulate the Fixed 
 * Pipeline based on the input struct and resolves the constant 
 * parameters according to the Fixed Pipeline state.
 *
 * @param  fpState           The Fixed Pipeline state interface.
 * @param  fpSettings       The input struct with the Fixed Pipeline settings.
 * @retval fragmentProgram   Pointer to the result vertex program.
 *
 * @code
 *
 *  // Get the GAL device
 *  GALDevice* gal_device = getGALDevice();
 *
 *  // Get the shader programs
 *  GALShaderProgram* vertexProgram = gal_device->createShaderProgram();
 *
 *  GALxFixedPipelineState* fpState = GALxCreateFixedPipelineState(gal_device);
 *
 *  GALx_FIXED_PIPELINE_SETTINGS fpSettings;
 *
 *  GALxLoadFPDefaultSettings(fpSettings);
 *
 *  // Set FP State configuration
 *
 *  fpSettings.alphaTestEnabled = true;
 *  fpSettings.lights[0].enabled = true;
 *
 *  // ....
 *
 *  GALxGeneratePrograms(fpState, fpSettings, fragmentProgram);
 *
 *  gal_device->setFragmentShader(fragmentProgram);
 *
 * @endcode
 */
void GALxGenerateFragmentProgram(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &fragmentProgram);


/**
 * Generates the compiled shader programs that emulate the Fixed 
 * Pipeline based on the input struct and resolves the constant 
 * parameters according to the Fixed Pipeline state.
 *
 * @param  fpState           The Fixed Pipeline state interface.
 * @param  fpSettings       The input struct with the Fixed Pipeline settings.
 * @retval vertexProgram   Pointer to the result vertex program.
 * @retval fragmentProgram Pointer to the result fragment program.
 *
 * @code
 *
 *  // Get the GAL device
 *  GALDevice* gal_device = getGALDevice();
 *
 *  // Get the shader programs
 *  GALShaderProgram* vertexProgram = gal_device->createShaderProgram();
 *  GALShaderProgram* fragmentProgram = gal_device->createShaderProgram();
 *
 *  GALxFixedPipelineState* fpState = GALxCreateFixedPipelineState(gal_device);
 *
 *  GALx_FIXED_PIPELINE_SETTINGS fpSettings;
 *
 *  GALxLoadFPDefaultSettings(fpSettings);
 *
 *  // Set FP State configuration
 *
 *  fpSettings.alphaTestEnabled = true;
 *  fpSettings.lights[0].enabled = true;
 *
 *  // ....
 *
 *  GALxGeneratePrograms(fpState, fpSettings, vertexProgram, fragmentProgram);
 *
 *  gal_device->setVertexShader(vertexProgram);
 *  gal_device->setFragmentShader(fragmentProgram);
 *
 * @endcode
 */
void GALxGeneratePrograms(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &vertexProgram, GALShaderProgram* &fragmentProgram);


////////////////////////////////
// Shader program compilation //
////////////////////////////////

/**
 * Creates and defines a new GALxConstantBinding interface object given a single state Id.
 *
 * @param target        The way/target the constant is refered in the program.
 * @param constantIndex    The index of the refered constant in the program.
 * @param stateId        The id for the requested state to be used in the function
 * @param function        The pointer to the binding function interface
 * @param directSource  The optional/complementary source value to be used in the function
 * @returns                The new created constant binding.
 */
GALxConstantBinding* GALxCreateConstantBinding(GALx_BINDING_TARGET target, 
                                               gal_uint constantIndex, 
                                               GALx_STORED_FP_ITEM_ID stateId,
                                               const GALxBindingFunction* function,
                                               GALxFloatVector4 directSource = GALxFloatVector4(gal_float(0)));

/**
 * Creates and defines a new GALxConstantBinding interface object given a vector of state Ids.
 *
 * @param target        The way/target the constant is refered in the program.
 * @param constantIndex    The index of the refered constant in the program.
 * @param vStateId        The vector of ids for the requested states to be used in the function
 * @param function        The pointer to the binding function interface
 * @param directSource  The optional/complementary source value to be used in the function
 * @returns                The new created constant binding.
 */
GALxConstantBinding* GALxCreateConstantBinding(GALx_BINDING_TARGET target, 
                                               gal_uint constantIndex, 
                                               std::vector<GALx_STORED_FP_ITEM_ID> vStateId, 
                                               const GALxBindingFunction* function,
                                               GALxFloatVector4 directSource = GALxFloatVector4(gal_float(0)));


/**
 * Releases a GALxConstantBinding interface object.
 *
 * @param constantBinding The GALxConstantBinding interface to release.
 */
void GALxDestroyConstantBinding(GALxConstantBinding* constantBinding);

/**
 * The GALxCompiledProgram interface is empty of public methods
 * as long as it represents a closed object containing a compiled program
 * information only interpretable by the internal GALx implementation.
 */
class GALxCompiledProgram
{
};

/**
 * The GALxCompileProgram functions compiles an ARB shader program source and
 * returns a GALxCompiledProgram interface object that can be further resolved
 * (the corresponding constant bindings can be resolved) using the GALxResolveProgram() 
 * function.
 *
 * @param  code        The program code string.
 * @returns            The compiled program object.
 */
GALxCompiledProgram* GALxCompileProgram(const std::string& code);

/**
 * The GALxResolveProgram function resolves the compiled program updating a
 * GALShaderProgram code and constant parameters accordingly to the
 * the Fixed Pipeline state.
 *
 * @param  fpState          The Fixed Pipeline state interface.
 * @param  cProgram          The compiled program object.
 * @param  constantList   The list of optional constant bindings.
 * @retval program          A valid/initialized GALShaderProgram where to 
 *                          output the compiled code and constants.
 *
 * @code 
 *  
 *  // The following example creates a constant binding for the example program
 *  // at the program local 41 location. The constant binding consists in the
 *  // normalization of the light 0 direction.
 *  
 *  string example_program = 
 *  "!!ARBvp1.0
 *  #
 *  # Shade vertex color using the normalized light direction
 *  #
 *  PARAM mvp[4]    = { state.matrix.mvp };
 *  PARAM lightDir = { program.local[41] };
 *
 *  # This just does out vertex transform by the modelview projection matrix
 *  DP4 oPos.x, mvp[0], vertex.position;
 *  DP4 oPos.y, mvp[1], vertex.position;
 *  DP4 oPos.z, mvp[2], vertex.position;
 *  DP4 oPos.w, mvp[3], vertex.position;
 *  # Modulate vertex color with light direction
 *  MUL result.color, vertex.color, lightDir;
 *  END";
 *
 *  // Define the normalize binding function for the light direction
 *    class LightDirNormalizeFunction: public libGAL::GALxBindingFunction
 *    {
 *    public:
 *
 *        virtual void function(libGAL::GALxFloatVector4& constant, 
 *                              std::vector<libGAL::GALxFloatVector4> vState,
 *                              const libGAL::GALxFloatVector4& directSource) const
 *        {
 *            // Take the direction from the first STL vector element
 *            constant = libGAL::_normalize(vState[0]); 
 *        };
 *    };
 *
 *  // Get the GAL device
 *  GALDevice* gal_device = getGALDevice();
 *
 *  // Get the shader programs
 *  GALShaderProgram* vertexProgram = gal_device->createShaderProgram();
 *  GALShaderProgram* fragmentProgram = gal_device->createShaderProgram();
 *
 *  GALxFixedPipelineState* fpState = GALxCreateFixedPipelineState(gal_device);
 *
 *  // The vector for required states.
 *  std::vector<GALx_STORED_FP_ITEM_ID> vState;
 *    
 *  // Add the light 0 direction state
 *    vState.push_back(GALx_LIGHT_DIRECTION);  
 *
 *  // Create the constant binding object
 *    GALxConstantBinding* cb = GALxCreateConstantBinding(GALx_BINDING_TARGET_LOCAL, 
 *                                                        41, vState, 
 *                                                        new LightPosNormalizeFunction);
 *
 *  // Create the constant binding list
 *  GALxConstantBindingList* cbl = new GALxConstantBindingList();
 *  cbl->push_back(cb);
 *
 *  // Make the proper state changes
 *  GALxLight& light = fpState->tl().light(0).setLightDirection(22.0f,30.0f,2.0f);
 *
 *  // Compile the program
 *    GALxCompiledProgram* cProgram = GALxCompileProgram(example_program);
 *
 *  // Resolve the program
 *  GALxResolveProgram(fpState, cProgram, cbl, vertexProgram);
 *
 *  GALxDestroyConstantBinding(cb);       // Release the constant binding object
 *  GALxDestroyCompiledProgram(cProgram); // Release the compiled program object
 *  delete cbl;                              // Release the constant binding list
 *
 * @endcode
 */
GALxConstantBindingList* GALxResolveProgram(GALxFixedPipelineState* fpState, const GALxCompiledProgram* cProgram, const GALxConstantBindingList* constantList, GALShaderProgram* program);

/**
 * The GALxDestroyCompiledProgram releases a GALxCompiledProgram object
 *
 * @param cProgram The compiled program to destroy
 */
void GALxDestroyCompiledProgram(GALxCompiledProgram* cProgram);

/**
 * This struct represents a program compilation/resolving log.
 */
struct GALx_COMPILATION_LOG
{
    /////////////////////////////////////
    // Vertex program compilation info //
    /////////////////////////////////////

    std::string vpSource;            ///< Source ARB code of the last compiled vertex program
    gal_uint vpNumInstructions;        ///< Number of ARB instructions of the last compiled vertex program
    gal_uint vpParameterRegisters;  ///< Number of used different parameter registers (see at ARB specifications) of the last compiled vertex program

    ///////////////////////////////////////
    // Fragment program compilation info //
    ///////////////////////////////////////

    std::string fpSource;            ///< Source ARB code of the last compiled fragment program
    gal_uint fpNumInstructions;        ///< Number of ARB instructions of the last compiled fragment program
    gal_uint fpParameterRegisters;    ///< Number of used different parameter registers (see at ARB specifications) of the last compiled fragment program
};

/**
 * The GALxGetCompilationLog returns the log of the last compiled program 
 * by the GALx library.
 *
 * @retval compileLog The compilation log structure to be filled out.
 */
void GALxGetCompilationLog(GALx_COMPILATION_LOG& compileLog);


////////////////////////////////
// Texture format conversions //
////////////////////////////////
// TO DO ... //

} // namespace libGAL

#endif // GALx_H
