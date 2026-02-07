/**************************************************************************
 *
 */

#include "GALx.h"
#include "GALxFixedPipelineStateImp.h"
#include "GALxConstantBindingImp.h"
#include "GALxCompiledProgramImp.h"
#include "InternalConstantBinding.h"
#include "GALxProgramExecutionEnvironment.h"
#include "GALxGLState.h"
#include "support.h"
#include "SettingsAdapter.h"
#include "ClusterBankAdapter.h"
#include "GALxFPFactory.h"
#include "GALxTLFactory.h"
#include "GALxTLShader.h"
#include "GALxShaderCache.h"
#include <list>

//#define ENABLE_GLOBAL_PROFILER
#include "GlobalProfiler.h"

using namespace libGAL;
using namespace std;

GALxShaderCache vShaderCache;
GALxShaderCache fShaderCache;

// Creates a fixed pipeline state interface. 
GALxFixedPipelineState* libGAL::GALxCreateFixedPipelineState(const GALDevice*)
{
    return new GALxFixedPipelineStateImp(new GALxGLState());
}

// Releases a GALxFixedPipelineState interface.  
void libGAL::GALxDestroyFixedPipelineState(GALxFixedPipelineState* fpState)
{
    GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);

    delete fpStateImp->getGLState();
    
    delete fpStateImp;
}

// Initializes a GALx_FIXED_PIPELINE_SETTINGS struct with the default values 
void libGAL::GALxLoadFPDefaultSettings(GALx_FIXED_PIPELINE_SETTINGS& fpSettings)
{
    GLOBAL_PROFILER_ENTER_REGION("GALx", "", "")
    
    fpSettings.lightingEnabled = false;
    fpSettings.localViewer = false;
    fpSettings.normalizeNormals = GALx_FIXED_PIPELINE_SETTINGS::GALx_NO_NORMALIZE;
    fpSettings.separateSpecular = false;
    fpSettings.twoSidedLighting = false;
    
    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS; i++)
    {
        fpSettings.lights[i].enabled = false;
        fpSettings.lights[i].lightType = GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_DIRECTIONAL;
    };

    fpSettings.cullEnabled = true;

    fpSettings.cullMode = GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_BACK;

    fpSettings.colorMaterialMode = GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_EMISSION;

    fpSettings.colorMaterialEnabledFace = GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_NONE;

    fpSettings.fogEnabled = false;

    fpSettings.fogCoordinateSource = GALx_FIXED_PIPELINE_SETTINGS::GALx_FRAGMENT_DEPTH;

    fpSettings.fogMode = GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP;

    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++)
    {
        fpSettings.textureCoordinates[i].coordS = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB;
        fpSettings.textureCoordinates[i].coordT = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB;
        fpSettings.textureCoordinates[i].coordR = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB;
        fpSettings.textureCoordinates[i].coordQ = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB;
        
        fpSettings.textureCoordinates[i].textureMatrixIsIdentity = true;
    };

    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++)
    {
        fpSettings.textureStages[i].enabled = false;
        fpSettings.textureStages[i].activeTextureTarget = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_2D;
        fpSettings.textureStages[i].textureStageFunction = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_MODULATE;
        fpSettings.textureStages[i].baseInternalFormat = GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGBA;

        fpSettings.textureStages[i].combineSettings.RGBFunction = fpSettings.textureStages[i].combineSettings.ALPHAFunction = GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE;

        fpSettings.textureStages[i].combineSettings.srcRGB[0] = fpSettings.textureStages[i].combineSettings.srcALPHA[0] = GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE; 
        fpSettings.textureStages[i].combineSettings.srcRGB[1] = fpSettings.textureStages[i].combineSettings.srcALPHA[1] = GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS;

        fpSettings.textureStages[i].combineSettings.srcRGB[2] = fpSettings.textureStages[i].combineSettings.srcALPHA[2] = GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT;
        fpSettings.textureStages[i].combineSettings.srcRGB[3] = fpSettings.textureStages[i].combineSettings.srcALPHA[3] = GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO;

        fpSettings.textureStages[i].combineSettings.operandRGB[0] = fpSettings.textureStages[i].combineSettings.operandRGB[1] = GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR;
        fpSettings.textureStages[i].combineSettings.operandRGB[2] = GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA;
    
        fpSettings.textureStages[i].combineSettings.operandALPHA[0] = fpSettings.textureStages[i].combineSettings.operandALPHA[1] = fpSettings.textureStages[i].combineSettings.operandALPHA[2] = GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA;
        fpSettings.textureStages[i].combineSettings.operandRGB[3] = GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR;
        fpSettings.textureStages[i].combineSettings.operandALPHA[3] = GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA;

        fpSettings.textureStages[i].combineSettings.srcRGB_texCrossbarReference[0] = fpSettings.textureStages[i].combineSettings.srcRGB_texCrossbarReference[1] = fpSettings.textureStages[i].combineSettings.srcRGB_texCrossbarReference[2] = fpSettings.textureStages[i].combineSettings.srcRGB_texCrossbarReference[3] = i;
        fpSettings.textureStages[i].combineSettings.srcALPHA_texCrossbarReference[0] = fpSettings.textureStages[i].combineSettings.srcALPHA_texCrossbarReference[1] = fpSettings.textureStages[i].combineSettings.srcALPHA_texCrossbarReference[2] = fpSettings.textureStages[i].combineSettings.srcALPHA_texCrossbarReference[3] = i;
        fpSettings.textureStages[i].combineSettings.RGBScale = 1;
        fpSettings.textureStages[i].combineSettings.ALPHAScale = 1;
    };

    fpSettings.alphaTestEnabled = false;
    fpSettings.alphaTestFunction = GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_ALWAYS;

    GLOBAL_PROFILER_EXIT_REGION()
}



// Private auxiliar function 
void checkValues(const GALx_FIXED_PIPELINE_SETTINGS& fpSettings)
{
    GLOBAL_PROFILER_ENTER_REGION("GALx", "", "")
    switch(fpSettings.normalizeNormals)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_NO_NORMALIZE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_RESCALE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_NORMALIZE:
            break;
        default:
            CG_ASSERT("Unexpected normalize mode value");
    }

    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS; i++)
    {
        switch(fpSettings.lights[i].lightType)
        {
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_DIRECTIONAL:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_POINT:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_SPOT:
                break;
            default:
                CG_ASSERT("Unexpected light type");
        }
    };

    switch(fpSettings.cullMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_NONE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_FRONT:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_BACK:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_FRONT_AND_BACK:
            break;
        default:
            CG_ASSERT("Unexpected cull mode");
    }

    switch(fpSettings.colorMaterialMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_EMISSION:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_DIFFUSE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_SPECULAR:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT_AND_DIFFUSE:
            break;
        default:
            CG_ASSERT("Unexpected color material mode");
    }

    switch(fpSettings.colorMaterialMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_EMISSION:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_DIFFUSE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_SPECULAR:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT_AND_DIFFUSE:
            break;
        default:
            CG_ASSERT("Unexpected color material mode");
    }

    switch(fpSettings.colorMaterialEnabledFace)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_NONE:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_BACK:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT_AND_BACK:
            break;
        default:
            CG_ASSERT("Unexpected color material face");
    }

    switch(fpSettings.fogCoordinateSource)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FRAGMENT_DEPTH:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_COORD:
            break;
        default:
            CG_ASSERT("Unexpected FOG coordinate source");
    }

    switch(fpSettings.fogMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_LINEAR:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP2:
            break;
        default:
            CG_ASSERT("Unexpected FOG Mode");
    }

    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++)
    {
        switch(fpSettings.textureCoordinates[i].coordS)
        {
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP:
                break;
            default:
                CG_ASSERT("Unexpected texture coordinate mode");
        }
    };

    for (unsigned int i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++)
    {
        switch(fpSettings.textureStages[i].activeTextureTarget)
        {
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_1D:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_2D:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_3D:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_CUBE_MAP:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_RECT:
                break;
            default:
                CG_ASSERT("Unexpected active texture target");
        }
    
        switch(fpSettings.textureStages[i].textureStageFunction)
        {
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_REPLACE:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_MODULATE:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DECAL:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_BLEND:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ADD:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE_NV:
                break;
            default:
                CG_ASSERT("Unexpected texture stage function");
        }

        switch(fpSettings.textureStages[i].baseInternalFormat)
        {
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ALPHA:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE_ALPHA:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DEPTH:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_INTENSITY:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGB:
            case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGBA:
                break;
            default:
                CG_ASSERT("Unexpected base internal format");
        }
        
        switch(fpSettings.textureStages[i].combineSettings.RGBFunction)
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_REPLACE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD_SIGNED:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_INTERPOLATE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SUBTRACT:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGB:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGBA:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SIGNED_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT:
                break;
            default:
                CG_ASSERT("Unexpected combine RGB function");
        }

        switch(fpSettings.textureStages[i].combineSettings.ALPHAFunction)
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_REPLACE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD_SIGNED:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_INTERPOLATE:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SUBTRACT:
            //case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGB:
            //case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGBA:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SIGNED_ADD:
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT:
                break;
            default:
                CG_ASSERT("Unexpected combine ALPHA function");
        }

        for (int j=0; j<4; j++)
        {
            switch(fpSettings.textureStages[i].combineSettings.srcRGB[j])
            {
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTUREn:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_PRIMARY_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS:
                    break;
                default:
                    CG_ASSERT("Unexpected combine RGB source");
            }

            switch(fpSettings.textureStages[i].combineSettings.srcALPHA[j])
            {
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTUREn:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_PRIMARY_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS:
                    break;
                default:
                    CG_ASSERT("Unexpected combine ALPHA source");
            }

            switch(fpSettings.textureStages[i].combineSettings.operandRGB[j])
            {
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA:
                    break;
                default:
                    CG_ASSERT("Unexpected combine RGB operand");
            }

            switch(fpSettings.textureStages[i].combineSettings.operandALPHA[j])
            {
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA:
                case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA:
                    break;
                default:
                    CG_ASSERT("Unexpected combine ALPHA operand");
            }

            if (fpSettings.textureStages[i].combineSettings.srcRGB_texCrossbarReference[j] >=
                GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
                CG_ASSERT("Unexpected TEXTUREn RGB source with 'n' greater than max texture stages");

            if (fpSettings.textureStages[i].combineSettings.srcALPHA_texCrossbarReference[j] >=
                GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
                CG_ASSERT("Unexpected TEXTUREn ALPHA source with 'n' greater than max texture stages");
            
            if ( (fpSettings.textureStages[i].combineSettings.RGBScale != 1) &&
                 (fpSettings.textureStages[i].combineSettings.RGBScale != 2) &&
                 (fpSettings.textureStages[i].combineSettings.RGBScale != 4) )
                 CG_ASSERT("Unexpected RGB scale factor <> {1,2,4}");

            if ( (fpSettings.textureStages[i].combineSettings.ALPHAScale != 1) &&
                 (fpSettings.textureStages[i].combineSettings.ALPHAScale != 2) &&
                 (fpSettings.textureStages[i].combineSettings.ALPHAScale != 4) )
                 CG_ASSERT("Unexpected ALPHA scale factor <> {1,2,4}");
        }    

    };

    switch(fpSettings.alphaTestFunction)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_NEVER:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_ALWAYS:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LESS:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LEQUAL:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_EQUAL:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GEQUAL:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GREATER:
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_NOTEQUAL:
            break;
        default:
            CG_ASSERT("Unexpected alpha test function");
    }


    GLOBAL_PROFILER_EXIT_REGION()
};

GALxConstantBinding* libGAL::GALxCreateConstantBinding(GALx_BINDING_TARGET target, 
                                                       gal_uint constantIndex, 
                                                       GALx_STORED_FP_ITEM_ID stateId,
                                                       const GALxBindingFunction* function,
                                                       GALxFloatVector4 directSource)
{
    std::vector<GALx_STORED_FP_ITEM_ID> vStateIds;
    
    vStateIds.push_back(stateId);

    return new GALxConstantBindingImp(target, constantIndex, vStateIds, function, directSource);
}

GALxConstantBinding* libGAL::GALxCreateConstantBinding(GALx_BINDING_TARGET target, 
                                                       gal_uint constantIndex, 
                                                       std::vector<GALx_STORED_FP_ITEM_ID> vStateIds, 
                                                       const GALxBindingFunction* function,
                                                       GALxFloatVector4 directSource)
{
    return new GALxConstantBindingImp(target, constantIndex, vStateIds, function, directSource);
}


void libGAL::GALxDestroyConstantBinding(GALxConstantBinding* constantBinding)
{
    GALxConstantBindingImp* cbi = static_cast<GALxConstantBindingImp*>(constantBinding);
    delete cbi;
}

// Generates the compiled vertex program that emulate the Fixed Pipeline 
void libGAL::GALxGenerateVertexProgram(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &vertexProgram)
{
    GLOBAL_PROFILER_ENTER_REGION("GALxGenerateVertexProgram", "", "")

    GALxConstantBindingList vpcbList;
    GALxTLShader* vtlsh;
    GALxConstantBindingList* finalBindingList;

    // Check all the settings values in the input struct to be proper values. 
    checkValues(fpSettings);

    // Generate the settings adapter to get GALxTLState and GALxFPState interfaces. 
    SettingsAdapter sa(fpSettings);

    if ( !vShaderCache.isCached(fpSettings, vertexProgram, finalBindingList) )
    {
        //cout << "VERTEX SHADER MISSSSSSSSSSSSSS " << endl;

        ////////////////////////////////////
        // Construct the fragment program //
        ////////////////////////////////////

        vtlsh = GALxTLFactory::constructVertexProgram(sa.getTLState());

        vpcbList = vtlsh->getConstantBindingList();

        ///////////////////////////////////////////////////////////////////////////
        // Compile and resolve the vertex program in case it is not in the cache //
        ///////////////////////////////////////////////////////////////////////////

        GLOBAL_PROFILER_ENTER_REGION("compile FX VSH", "", "")
        GALxCompiledProgram* cVertexProgram = GALxCompileProgram(vtlsh->getCode());
        GLOBAL_PROFILER_EXIT_REGION()

        finalBindingList = GALxResolveProgram(fpState, cVertexProgram, &vpcbList, vertexProgram);

        GALxDestroyCompiledProgram(cVertexProgram);

        //printf("Created Vertex Shader :\n");
        //vertexProgram->printASM(cout);
        //printf("----------------------\n");
        
        vShaderCache.addCacheEntry(fpSettings, vertexProgram, finalBindingList);   

        delete vtlsh;
    }
    else
    {
        //cout << "VERTEX SHADER HITTTTTTTTTTTT " << endl;

        //printf("Cached Vertex Shader :\n");
        //vertexProgram->printASM(cout);
        //printf("----------------------\n");     

        // Synchronize GALxFixedPipelineState states to GALxGLState
        GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);
        fpStateImp->sync();

        GALxConstantBindingList::iterator iter = finalBindingList->begin();

        while (iter != finalBindingList->end())
        {
            GALxFloatVector4 constant;
            
            // Resolve the constant
            (*iter)->resolveConstant(fpStateImp, constant);

            // Put the constant value in the GALShaderProgram object
            vertexProgram->setConstant((*iter)->getConstantIndex(), constant.c_array());
            
            iter++;
        }

    }

    
    GLOBAL_PROFILER_EXIT_REGION()   
}

// Generates the compiled shader programs that emulate the Fixed Pipeline 
void libGAL::GALxGenerateFragmentProgram(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &fragmentProgram)
{
    GLOBAL_PROFILER_ENTER_REGION("GALxGenerateFragmentProgram", "", "")

    GALxConstantBindingList vpcbList;
    GALxConstantBindingList* finalBindingList;

    // Check all the settings values in the input struct to be proper values. 
    checkValues(fpSettings);

    // Generate the settings adapter to get GALxTLState and GALxFPState interfaces. 
    SettingsAdapter sa(fpSettings);
    
    if ( !fShaderCache.isCached(fpSettings, fragmentProgram, finalBindingList) )
    {
        //cout << "FRAGMENT SHADER MISSSSSSSSSSSSSS " << endl;
          
        ////////////////////////////////////
        // Construct the fragment program //
        ////////////////////////////////////

        GALxTLShader* ftlsh = GALxFPFactory::constructFragmentProgram(sa.getFPState());
        
        GALxConstantBindingList fpcbList = ftlsh->getConstantBindingList();

        //////////////////////////////////////////////
        // Compile and resolve the fragment program //
        //////////////////////////////////////////////

        GLOBAL_PROFILER_ENTER_REGION("compile FX FSH", "", "")
        GALxCompiledProgram* cFragmentProgram = GALxCompileProgram(ftlsh->getCode());
        GLOBAL_PROFILER_EXIT_REGION()

        finalBindingList = GALxResolveProgram(fpState, cFragmentProgram, &fpcbList, fragmentProgram);

        GALxDestroyCompiledProgram(cFragmentProgram);

        //printf("Created Fragment Shader :\n");
        //fragmentProgram->printASM(cout);
        //printf("----------------------\n");

        fShaderCache.addCacheEntry(fpSettings, fragmentProgram, finalBindingList);   

        delete ftlsh;
            
        GLOBAL_PROFILER_EXIT_REGION()
    }
    else
    {
        //printf("Cached Fragment Shader :\n");
        //fragmentProgram->printASM(cout);
        //printf("----------------------\n");

        // Synchronize GALxFixedPipelineState states to GALxGLState
        GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);
        fpStateImp->sync();

        GALxConstantBindingList::iterator iter = finalBindingList->begin();

        while (iter != finalBindingList->end())
        {
            GALxFloatVector4 constant;
            
            // Resolve the constant
            (*iter)->resolveConstant(fpStateImp, constant);

            // Put the constant value in the GALShaderProgram object
            fragmentProgram->setConstant((*iter)->getConstantIndex(), constant.c_array());

            iter++;
        }

    }

    GLOBAL_PROFILER_EXIT_REGION()   
}

// Generates the compiled shader programs that emulate the Fixed Pipeline 
void libGAL::GALxGeneratePrograms(GALxFixedPipelineState* fpState, const GALx_FIXED_PIPELINE_SETTINGS& fpSettings, GALShaderProgram* &vertexProgram, GALShaderProgram* &fragmentProgram)
{
    GLOBAL_PROFILER_ENTER_REGION("GALxGeneratePrograms", "", "")

    // Check all the settings values in the input struct to be proper values. 
    checkValues(fpSettings);

    // Generate the settings adapter to get GALxTLState and GALxFPState interfaces. 
    SettingsAdapter sa(fpSettings);

    //////////////////////////////////
    // Construct the vertex program //
    //////////////////////////////////

    GALxConstantBindingList vpcbList;
    GALxTLShader* vtlsh;
    GALxConstantBindingList* finalBindingList;

    if ( !vShaderCache.isCached(fpSettings, vertexProgram, finalBindingList) )
    {
        //cout << "VERTEX SHADER MISSSSSSSSSSSSSS " << endl;

        ////////////////////////////////////
        // Construct the fragment program //
        ////////////////////////////////////

        vtlsh = GALxTLFactory::constructVertexProgram(sa.getTLState());

        vpcbList = vtlsh->getConstantBindingList();

        ///////////////////////////////////////////////////////////////////////////
        // Compile and resolve the vertex program in case it is not in the cache //
        ///////////////////////////////////////////////////////////////////////////

        GLOBAL_PROFILER_ENTER_REGION("compile FX VSH", "", "")
        GALxCompiledProgram* cVertexProgram = GALxCompileProgram(vtlsh->getCode());
        GLOBAL_PROFILER_EXIT_REGION()

        finalBindingList = GALxResolveProgram(fpState, cVertexProgram, &vpcbList, vertexProgram);

        GALxDestroyCompiledProgram(cVertexProgram);

        //printf("Created Vertex Shader :\n");
        //vertexProgram->printASM(cout);
        //printf("----------------------\n");
        
        vShaderCache.addCacheEntry(fpSettings, vertexProgram, finalBindingList);   

        delete vtlsh;
    }
    else
    {
        //cout << "VERTEX SHADER HITTTTTTTTTTTT " << endl;

        //printf("Cached Vertex Shader :\n");
        //vertexProgram->printASM(cout);
        //printf("----------------------\n");

        // Synchronize GALxFixedPipelineState states to GALxGLState
        GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);
        fpStateImp->sync();

        GALxConstantBindingList::iterator iter = finalBindingList->begin();

        while (iter != finalBindingList->end())
        {
            GALxFloatVector4 constant;
            
            // Resolve the constant
            (*iter)->resolveConstant(fpStateImp, constant);

            // Put the constant value in the GALShaderProgram object
            vertexProgram->setConstant((*iter)->getConstantIndex(), constant.c_array());
            
            iter++;
        }

    }
    
    if ( !fShaderCache.isCached(fpSettings, fragmentProgram, finalBindingList) )
    {
        //cout << "FRAGMENT SHADER MISSSSSSSSSSSSSS " << endl;
          
        ////////////////////////////////////
        // Construct the fragment program //
        ////////////////////////////////////

        GALxTLShader* ftlsh = GALxFPFactory::constructFragmentProgram(sa.getFPState());
        
        GALxConstantBindingList fpcbList = ftlsh->getConstantBindingList();

        //////////////////////////////////////////////
        // Compile and resolve the fragment program //
        //////////////////////////////////////////////

        GLOBAL_PROFILER_ENTER_REGION("compile FX FSH", "", "")
        GALxCompiledProgram* cFragmentProgram = GALxCompileProgram(ftlsh->getCode());
        GLOBAL_PROFILER_EXIT_REGION()

        finalBindingList = GALxResolveProgram(fpState, cFragmentProgram, &fpcbList, fragmentProgram);

        GALxDestroyCompiledProgram(cFragmentProgram);

        //printf("Created Fragment Shader :\n");
        //fragmentProgram->printASM(cout);
        //printf("----------------------\n");

        fShaderCache.addCacheEntry(fpSettings, fragmentProgram, finalBindingList);   

        delete ftlsh;
            
        GLOBAL_PROFILER_EXIT_REGION()
    }
    else
    {
        //cout << "FRAGMENT SHADER HITTTTTTTTTTTT " << endl;

        //printf("Cached Fragment Shader :\n");
        //fragmentProgram->printASM(cout);
        //printf("----------------------\n");

        // Synchronize GALxFixedPipelineState states to GALxGLState
        GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);
        fpStateImp->sync();

        GALxConstantBindingList::iterator iter = finalBindingList->begin();

        while (iter != finalBindingList->end())
        {
            GALxFloatVector4 constant;
            
            // Resolve the constant
            (*iter)->resolveConstant(fpStateImp, constant);

            // Put the constant value in the GALShaderProgram object
            fragmentProgram->setConstant((*iter)->getConstantIndex(), constant.c_array());

            iter++;
        }

    }
}

static libGAL::GALx_COMPILATION_LOG lastResults;


GALxCompiledProgram* libGAL::GALxCompileProgram(const std::string& code)
{
    GLOBAL_PROFILER_ENTER_REGION("GALxCompileProgram", "", "")
    GALxCompiledProgramImp* cProgramImp = new GALxCompiledProgramImp;

    // Initialize compilers for the first time (using a static pointer)
    static GALxProgramExecutionEnvironment* vpee = new GALxVP1ExecEnvironment;
    static GALxProgramExecutionEnvironment* fpee = new GALxFP1ExecEnvironment;

    U32 size_binary;
    GALxRBank<float> clusterBank(250,"clusterBank");

    if (!code.compare( 0, 10, "!!ARBvp1.0" ))
    {    
        // It�s an ARB vertex program 1.0
        
        // Compile
        ResourceUsage resourceUsage;

        const gal_ubyte* compiledCode = (const gal_ubyte*)vpee->compile(code, size_binary, clusterBank, resourceUsage);
        
        // Put the code and cluster bank in the GALxCompiledProgram
        cProgramImp->setCode(compiledCode, size_binary);
        cProgramImp->setCompiledConstantBank(clusterBank);

        for(int tu = 0; tu < MAX_TEXTURE_UNITS_ARB; tu++)
            cProgramImp->setTextureUnitsUsage(tu, resourceUsage.textureUnitUsage[tu]);

        cProgramImp->setKillInstructions(resourceUsage.killInstructions);

        // Update last results structure
        lastResults.vpSource = code;
        lastResults.vpNumInstructions = resourceUsage.numberOfInstructions;
        lastResults.vpParameterRegisters = resourceUsage.numberOfParamRegs;

        delete[] compiledCode;
    }
    else if (!code.compare( 0, 10, "!!ARBfp1.0" ))
    {    
        // It�s an ARB fragment program 1.0

        // Compile
        ResourceUsage resourceUsage;

        const gal_ubyte* compiledCode = (const gal_ubyte*)fpee->compile(code, size_binary, clusterBank, resourceUsage);
        
        // Put the code and cluster bank in the GALxCompiledProgram
        cProgramImp->setCode(compiledCode, size_binary);
        cProgramImp->setCompiledConstantBank(clusterBank);

        for(int tu = 0; tu < MAX_TEXTURE_UNITS_ARB; tu++)
            cProgramImp->setTextureUnitsUsage(tu, resourceUsage.textureUnitUsage[tu]);

        cProgramImp->setKillInstructions(resourceUsage.killInstructions);


        // Update last results structure
        lastResults.fpSource = code;
        lastResults.fpNumInstructions = resourceUsage.numberOfInstructions;
        lastResults.fpParameterRegisters = resourceUsage.numberOfParamRegs;

        delete[] compiledCode;
    }
    else
        CG_ASSERT("Unexpected or not supported shader program model");
    
    GLOBAL_PROFILER_EXIT_REGION()
    return cProgramImp;
}

// resolves the compiled program updating a GALShaderProgram code and constant parameters accordingly to the the Fixed Pipeline state. 
GALxConstantBindingList* libGAL::GALxResolveProgram(GALxFixedPipelineState* fpState, const GALxCompiledProgram* cProgram, const GALxConstantBindingList* constantList, GALShaderProgram* program)
{
    GLOBAL_PROFILER_ENTER_REGION("GALxResolveProgram", "", "")
    // Get the pointer to the GALxFixedPipelineState implementation class 
    GALxFixedPipelineStateImp* fpStateImp = static_cast<GALxFixedPipelineStateImp*>(fpState);

    // Get the pointer to the GALxCompiledProgram implementation class 
    const GALxCompiledProgramImp* cProgramImp = static_cast<const GALxCompiledProgramImp*>(cProgram);

    // Copy the already compiled code 
    program->setCode(cProgramImp->getCode(), cProgramImp->getCodeSize());

    GALxRBank<float> clusterBank(200,"clusterBank");

    clusterBank = cProgramImp->getCompiledConstantBank();

    for(gal_uint tu = 0; tu < 16 /*TEX_UNITS */ ; tu++)
        program->setTextureUnitsUsage(tu,cProgramImp->getTextureUnitsUsage(tu));

    program->setKillInstructions(cProgramImp->getKillInstructions());

    // Merge both the input program resolve constant binding list with the
    // compiler generated constant binding list

    ClusterBankAdapter cbAdapter(&clusterBank);

    GALxConstantBindingList* finalConstantList = cbAdapter.getFinalConstantBindings(constantList);

    //////////////////////////////////////
    // Program constant resolving phase //
    //////////////////////////////////////

    // Synchronize GALxFixedPipelineState states to GALxGLState
    fpStateImp->sync();

    GALxConstantBindingList::iterator iter = finalConstantList->begin();

    while (iter != finalConstantList->end())
    {
        GALxFloatVector4 constant;
        
        // Resolve the constant
        (*iter)->resolveConstant(fpStateImp, constant);

        // Put the constant value in the GALShaderProgram object
        program->setConstant((*iter)->getConstantIndex(), constant.c_array());
        
        // Delete InternalConstantBinding object TO DO: CAUTION: Not all the cb are icb�s
        //InternalConstantBinding* icb = static_cast<InternalConstantBinding*>(*iter);

        //delete icb;

        iter++;
    }
    
    //finalConstantList->clear();
    //delete finalConstantList;
    
    GLOBAL_PROFILER_EXIT_REGION()

    return finalConstantList;
}

void libGAL::GALxDestroyCompiledProgram(GALxCompiledProgram* cProgram)
{
    // Get the pointer to the GALxCompiledProgram implementation class 
    const GALxCompiledProgramImp* cProgramImp = static_cast<const GALxCompiledProgramImp*>(cProgram);

    delete cProgramImp;
}

void libGAL::GALxGetCompilationLog(GALx_COMPILATION_LOG& compileLog)
{
    compileLog = lastResults;
}
