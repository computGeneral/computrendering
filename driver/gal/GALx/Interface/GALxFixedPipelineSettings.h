/**************************************************************************
 *
 */

#ifndef GALx_FIXED_PIPELINE_SETTINGS_H
    #define GALx_FIXED_PIPELINE_SETTINGS_H

#include "GALxGlobalTypeDefinitions.h"
#include "GALxFixedPipelineConstantLimits.h"

namespace libGAL
{
/**
 * Notes on Direct3D Fixed Pipeline compatibility:
 *
 * The following D3D Fixed Function Pipeline properties are not still supported
 * by the FF emulation engine:
 *
 *    1. The "Theta" angle in the spot/cone lights. Therefore, spot attenuation 
 *       starts at the cone center (Theta = 0.0).
 *
 *    2. The Falloff factor in D3D spot lights corresponds exactly to the 
 *       "Spot exponent" of Transform & Lighting State interface.
 *
 *    3. The "Range" distance of point lights. Therefore, there is not 
 *       cut-off distance in point lights.
 *
 *  4. The "Enable Specular lighting" property, that allows enabling/disabling
 *     the per-vertex specular computation in the vertex shader as
 *       long as the specular component of light won´t be used to form the final 
 *       color of the surface. The unalike "Separate Specular" property inherited 
 *       from the OpenGL Pipeline allows the specular contribution of lights to be
 *       added to the final fragment color after texturing.
 *
 * Some unknown parameters in the Direct3D world:
 *
 *    1. The "twoSidedLighting" parameter stands for a second/complementary 
 *       color computation for back faced polygons. The computation 
 *       is the same as front-faced polygons but using the opposite normal vector. 
 *       Thus, if two sided lighting is enabled, two surface colors are actually 
 *       computed for each vertex: the colors for either the front face and the 
 *     back faces that the vertex belongs to.
 *
 *    2. TO EXPLAIN: The difference in the Culling modes and OpenGL equivalence.
 *
 *  3. TO EXPLAIN: The texture stage combine functions and NVIDIA and ATI extensions supported.
 */

/**
 * The GALx_COMBINE_SETTINGS struct
 *
 * Defines the combine function settings
 */
struct GALx_COMBINE_SETTINGS
{
    /**
     * The combine function
     */
    enum GALx_COMBINE_FUNCTION 
    { 
        GALx_COMBINE_REPLACE, ///< Replace with the current stage sampled value
        GALx_COMBINE_MODULATE, ///< Modulate the previous stage color with the current sampled one
        GALx_COMBINE_ADD, ///< Add the previous stage and the current sampled color
        GALx_COMBINE_ADD_SIGNED, 
        GALx_COMBINE_INTERPOLATE, 
        GALx_COMBINE_SUBTRACT,
        GALx_COMBINE_DOT3_RGB, ///< Only applicable to Combine RGB function
        GALx_COMBINE_DOT3_RGBA,///< Only applicable to Combine RGB function
        
        GALx_COMBINE_MODULATE_ADD, ///< MODULATE_ADD function included in the "ATI_texture_env_combine3" extension.
        GALx_COMBINE_MODULATE_SIGNED_ADD, ///< MODULATED_SIGNED_ADD function included in the "ATI_texture_env_combine3" extension.
        GALx_COMBINE_MODULATE_SUBTRACT, ///< MODULATE_SUBTRACT function included in the "ATI_texture_env_combine3" extension.
    };

    /**
     * The combine sources
     */
    enum GALx_COMBINE_SOURCE
    {
        GALx_COMBINE_ZERO,  ///<  ZERO combine source included in the "NV_texture_env_combine4" extension.
        GALx_COMBINE_ONE,   ///<  ONE combine source included in the "ATI_texture_env_combine3" extension.
        GALx_COMBINE_TEXTURE,
        GALx_COMBINE_TEXTUREn,
        GALx_COMBINE_CONSTANT,
        GALx_COMBINE_PRIMARY_COLOR,
        GALx_COMBINE_PREVIOUS,
    };

    /**
     * The combine source operand/modifier
     */
    enum GALx_COMBINE_OPERAND
    {
        GALx_COMBINE_SRC_COLOR,
        GALx_COMBINE_ONE_MINUS_SRC_COLOR,
        GALx_COMBINE_SRC_ALPHA,
        GALx_COMBINE_ONE_MINUS_SRC_ALPHA
    };

    GALx_COMBINE_FUNCTION RGBFunction, ALPHAFunction; ///< The combine functions for RGB and Alpha components

    GALx_COMBINE_SOURCE srcRGB[4], srcALPHA[4]; ///< 4th source extended for "NV_texture_env_combine4" extension support.
    GALx_COMBINE_OPERAND operandRGB[4], operandALPHA[4]; ///< 4th source extended for "NV_texture_env_combine4" extension support.

    gal_int srcRGB_texCrossbarReference[4]; ///< The RGB crossbar reference (What the stage source number when specifying GALx_COMBINE_TEXTUREn as RGB combine source).
    gal_int srcALPHA_texCrossbarReference[4]; ///< The ALPHA crossbar reference (What the stage source number when specifying GALx_COMBINE_TEXTUREn as ALPHA combine source).

    gal_int RGBScale; ///< The Final RGB component scale factor.
    gal_int ALPHAScale; ///< The Final Alpha component scale factor.

};

/**
 * The GALx_FIXED_PIPELINE_SETTINGS struct.
 * 
 * This struct is used as input of the GALxGeneratePrograms() function
 * to generate the shader programs that emulate the Fixed Pipeline. 
 * This struct gathers all the fixed pipeline settings concerning states
 * for which a single feature change requires rebuilding of both generated programs.
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @version 0.8
 * @date 03/09/2007
 */
struct GALx_FIXED_PIPELINE_SETTINGS
{
    ///////////////////////
    // Lighting settings //
    ///////////////////////

    gal_bool lightingEnabled;    ///< Vertex Lighting is enabled (if disabled the vertex color attribute is copied as vertex color output)
    gal_bool localViewer;        ///< The view point is not located at an infinite distance (enabling this feature increases the vertex computation cost)
    
    enum GALx_NORMALIZATION_MODE
    { 
        GALx_NO_NORMALIZE,    ///< The vertex normal is used directly for lighting computations
        GALx_RESCALE,        ///< The vertex normal is rescaled (this is a cheaper normalization mode but is only applicable to modelview transformation matrices not distortioning objects to any dimension)
        GALx_NORMALIZE,        ///< The full cost vertex normal normalization.
    
    } normalizeNormals; ///< The normal normalization mode

    gal_bool separateSpecular;    ///< Computes separately the specular contribution of lights in order to be added after surface fragment texturing.
    gal_bool twoSidedLighting;  ///< Computes colors for each front and back faces that the vertex belongs to

    static const gal_uint GALx_MAX_LIGHTS = GALx_FP_MAX_LIGHTS_LIMIT;    ///< Max number of lights supported by the implementation

    /**
     * The GALx_LIGHT_SETTINGS struct defines the settings for a light source
     */
    struct GALx_LIGHT_SETTINGS
    {
        gal_bool enabled;    ///< Light enabled/disabled.

        enum GALx_LIGHT_TYPE 
        { 
            GALx_DIRECTIONAL,    ///< The light is at an infinite distance from the scene, so, the light rays incide all the surface points from the same direction.
            GALx_POINT,            ///< The light is at a not infinite concrete coordinate in the scene, so, the light rays incide from different directions to each surface point.
            GALx_SPOT,            ///< A point light with the light rays constrained to the cut-off angle of a cone pointing in a concrete direction (the spot direction).
        
        } lightType;    ///< The light type
    };

    GALx_LIGHT_SETTINGS lights[GALx_MAX_LIGHTS];    ///< The array of lights

    //////////////////////
    // Culling Settings //
    //////////////////////

    gal_bool cullEnabled;    ///< Face culling is enabled/disabled.

    enum GALx_CULL_MODE 
    { 
        GALx_CULL_NONE,                ///< Do not cull any face. The same as culling disabled
        GALx_CULL_FRONT,            ///< Front face culling. Front faces are those specified in the counter-clock wise order by default.
        GALx_CULL_BACK,                ///< Back face culling. Back faces are those specified in the clock wise order by default.
        GALx_CULL_FRONT_AND_BACK,    ///< Cull both faces.
    
    } cullMode;

    /////////////////////////////
    // Color Material Settings //
    /////////////////////////////

    enum GALx_COLOR_MATERIAL_MODE 
    { 
        GALx_CM_EMISSION,                ///< Color material emission mode
        GALx_CM_AMBIENT,                ///< Color material ambient mode
        GALx_CM_DIFFUSE,                ///< Color material diffuse mode
        GALx_CM_SPECULAR,                ///< Color material specular mode
        GALx_CM_AMBIENT_AND_DIFFUSE,    ///< Color material ambient and diffuse mode
    
    } colorMaterialMode; ///< The color material mode. Only emission is currently implemented.

    enum GALx_COLOR_MATERIAL_ENABLED_FACE 
    { 
        GALx_CM_NONE,            ///< Color material any face enabled
        GALx_CM_FRONT,            ///< Color material front face enabled
        GALx_CM_BACK,            ///< Color material back face enabled
        GALx_CM_FRONT_AND_BACK, ///< Color material front and back faces enabled
    
    } colorMaterialEnabledFace;  ///< The color material enabled face

    //////////////////
    // FOG Settings //
    //////////////////

    gal_bool fogEnabled;    ///< FOG enabled/disabled

    enum GALx_FOG_COORDINATE_SOURCE
    { 
        GALx_FRAGMENT_DEPTH, ///< The FOG coordinate is taken from the fragment depth.
        GALx_FOG_COORD,         ///< The FOG coordinate is taken from the vertex fog attribute.
    
    } fogCoordinateSource; ///< The FOG coordinate source
    
    enum GALx_FOG_MODE 
    { 
        GALx_FOG_LINEAR, ///< The FOG factor is computed linearly across the segment between the start and the end distances.
        GALx_FOG_EXP,     ///< The FOG factor is an exponential function of the FOG coordinate.
        GALx_FOG_EXP2,     ///< The FOG factor is a second order exponential function of the FOG coordinate.
    
    } fogMode;  ///< The FOG mode

    ////////////////////////////////////////////
    // Texture Coordinate Generation Settings //
    ////////////////////////////////////////////

    static const gal_uint GALx_MAX_TEXTURE_STAGES = GALx_FP_MAX_TEXTURE_STAGES_LIMIT; ///< Max number of texture stages supported by the implementation

    /**
     * The GALx_TEXTURE_COORDINATE_SETTINGS struct defines the settings of the texture coordinate generation
     */
    struct GALx_TEXTURE_COORDINATE_SETTINGS
    {
        enum GALx_TEXTURE_COORDINATE_MODE 
        { 
            GALx_VERTEX_ATTRIB,        ///< The texture coordinate is taken/copied directly from the vertex input texture coordinate
            GALx_OBJECT_LINEAR,        ///< The texture coordinate is computed as the normalized distance to the object linear plane
            GALx_EYE_LINEAR,        ///< The texture coordinate is computed as the normalized distance to the eye linear plane
            GALx_SPHERE_MAP,        ///< The texture coordinate is computed using the sphere map reflection technique (environmental mapping)
            GALx_REFLECTION_MAP,    ///< The texture coordinate is computed using the reflection map technique (environmental mapping)
            GALx_NORMAL_MAP            ///< The texture coordinate is computed using the normal map technique (environmental mapping)
        };

        GALx_TEXTURE_COORDINATE_MODE coordS; ///< The texture S coordinate mode
        GALx_TEXTURE_COORDINATE_MODE coordT; ///< The texture T coordinate mode
        GALx_TEXTURE_COORDINATE_MODE coordR; ///< The texture R coordinate mode
        GALx_TEXTURE_COORDINATE_MODE coordQ; ///< The texture Q coordinate mode

        gal_bool textureMatrixIsIdentity; ///< The texture coordinate matrix is the identity matrix (no coordinate transformation is needed)
    };

    GALx_TEXTURE_COORDINATE_SETTINGS textureCoordinates[GALx_MAX_TEXTURE_STAGES]; ///< The array of texture coordinate generation settings

    ////////////////////////////
    // Texture Stage Settings //
    ////////////////////////////

    /**
     * The GALx_TEXTURE_STAGE_SETTINGS struct defines the settings of a texture stage
     */
    struct GALx_TEXTURE_STAGE_SETTINGS
    {
        gal_bool enabled;    ///< Texture stage enabled/disabled
        
        enum GALx_TEXTURE_TARGET 
        { 
            GALx_TEXTURE_1D,        ///< 1D Texture target
            GALx_TEXTURE_2D,        ///< 2D Texture target
            GALx_TEXTURE_3D,        ///< 3D Texture target
            GALx_TEXTURE_CUBE_MAP,    ///< Cube Map texture target
            GALx_TEXTURE_RECT        ///< Rectangular texture target
        
        } activeTextureTarget;    /// The active texture target

        enum GALx_TEXTURE_STAGE_FUNCTION 
        { 
            GALx_REPLACE,    ///< Replace texture combine function 
            GALx_MODULATE,  ///< Modulate texture combine function 
            GALx_DECAL,        ///< Decal texture combine function 
            GALx_BLEND,        ///< Blend texture combine function 
            GALx_ADD,        ///< Add texture combine function 
            GALx_COMBINE,    ///< Combine texture combine function 
            GALx_COMBINE_NV  ///< Combine texture combine function (For "NV_texture_env_combine4")
        
        } textureStageFunction;    ///< The texture combine function

        GALx_COMBINE_SETTINGS combineSettings;

        enum GALx_BASE_INTERNAL_FORMAT 
        { 
            GALx_ALPHA,                ///<  The alpha internal format
            GALx_LUMINANCE,            ///<  The luminance internal format
            GALx_LUMINANCE_ALPHA,    ///<  The luminance alpha internal format
            GALx_DEPTH,                ///<  The depth internal format
             GALx_INTENSITY,            ///<  The intensity internal format
            GALx_RGB,                ///<  The RGB internal format
            GALx_RGBA,                ///<  The RGBA internal format
        
        } baseInternalFormat;    ///< The base internal format
    };

    GALx_TEXTURE_STAGE_SETTINGS textureStages[GALx_MAX_TEXTURE_STAGES];  ///< The array of texture stage settings

    /////////////////////////
    // Alpha Test Settings //
    /////////////////////////

    gal_bool alphaTestEnabled;    ///< Alpha test enabled/disabled

    enum GALx_ALPHA_TEST_FUNCTION
    { 
        GALx_ALPHA_NEVER,        ///< Alpha test never function
        GALx_ALPHA_ALWAYS,        ///< Alpha test always function
        GALx_ALPHA_LESS,        ///< Alpha test less function
        GALx_ALPHA_LEQUAL,        ///< Alpha test lequal function
        GALx_ALPHA_EQUAL,        ///< Alpha test equal function
        GALx_ALPHA_GEQUAL,        ///< Alpha test gequal function
        GALx_ALPHA_GREATER,        ///< Alpha test greater function
        GALx_ALPHA_NOTEQUAL        ///< Alpha test notequal function
    
    } alphaTestFunction;    ///< The alpha test function
};

} // namespace libGAL

#endif // GALx_FIXED_PIPELINE_SETTINGS_H
