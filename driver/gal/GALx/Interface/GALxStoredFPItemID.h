/**************************************************************************
 *
 */

#ifndef GALx_STORED_FP_ITEM_ID_H
    #define GALx_STORED_FP_ITEM_ID_H

#include "GALxFixedPipelineConstantLimits.h"

namespace libGAL
{

/**
 * Fixed Pipeline state identifiers
 */ 
enum GALx_STORED_FP_ITEM_ID
{
    //////////////////////
    // Surface Material //
    //////////////////////

    GALx_MATERIAL_FIRST_ID           = 0,

    GALx_MATERIAL_FRONT_AMBIENT    = GALx_MATERIAL_FIRST_ID,        ///< Material front ambient color
    GALx_MATERIAL_FRONT_DIFFUSE    = GALx_MATERIAL_FIRST_ID + 1,    ///< Material front diffuse color
    GALx_MATERIAL_FRONT_SPECULAR   = GALx_MATERIAL_FIRST_ID + 2,    ///< Material front specular color
    GALx_MATERIAL_FRONT_EMISSION   = GALx_MATERIAL_FIRST_ID + 3,    ///< Material front emission color
    GALx_MATERIAL_FRONT_SHININESS  = GALx_MATERIAL_FIRST_ID + 4,    ///< Material front shininess factor
    GALx_MATERIAL_BACK_AMBIENT     = GALx_MATERIAL_FIRST_ID + 5,    ///< Material back ambient color
    GALx_MATERIAL_BACK_DIFFUSE     = GALx_MATERIAL_FIRST_ID + 6,    ///< Material back diffuse color
    GALx_MATERIAL_BACK_SPECULAR    = GALx_MATERIAL_FIRST_ID + 7,    ///< Material back specular color
    GALx_MATERIAL_BACK_EMISSION    = GALx_MATERIAL_FIRST_ID + 8,    ///< Material back emission color
    GALx_MATERIAL_BACK_SHININESS   = GALx_MATERIAL_FIRST_ID + 9,    ///< Material back shininess factor
    
    GALx_MATERIAL_PROPERTIES_COUNT = 10,

    /////////////////////////////////////
    // Colors and properties of lights //
    /////////////////////////////////////

    GALx_LIGHT_FIRST_ID               = GALx_MATERIAL_FIRST_ID + GALx_MATERIAL_PROPERTIES_COUNT,

    GALx_LIGHT_AMBIENT             = GALx_LIGHT_FIRST_ID,        ///< Light ambient color
    GALx_LIGHT_DIFFUSE             = GALx_LIGHT_FIRST_ID + 1,    ///< Light diffuse color
    GALx_LIGHT_SPECULAR            = GALx_LIGHT_FIRST_ID + 2,    ///< Light specular color
    GALx_LIGHT_POSITION            = GALx_LIGHT_FIRST_ID + 3,    ///< Light position (point/spot lights only)
    GALx_LIGHT_DIRECTION           = GALx_LIGHT_FIRST_ID + 4,    ///< Light direction (directional lights only)
    GALx_LIGHT_ATTENUATION         = GALx_LIGHT_FIRST_ID + 5,    ///< Light attenuation (point/spot lights only)
    GALx_LIGHT_SPOT_DIRECTION      = GALx_LIGHT_FIRST_ID + 6,    ///< Light spot direction (spot lights only)
    GALx_LIGHT_SPOT_CUTOFF           = GALx_LIGHT_FIRST_ID + 7,    ///< Light spot cut-off cosine angle (Phi angle in D3D)
    GALx_LIGHT_SPOT_EXPONENT       = GALx_LIGHT_FIRST_ID + 8,    ///< Light spot attenuation exponent (Falloff factor in D3D)

    GALx_LIGHT_PROPERTIES_COUNT    = 9,
    // ... Implicit enums for lights 1, 2, ..., up to GALx_FP_MAX_LIGHTS_LIMIT - 1

    ////////////////////////////
    // Light model properties //
    ////////////////////////////

    GALx_LIGHTMODEL_FIRST_ID         = GALx_LIGHT_FIRST_ID + GALx_LIGHT_PROPERTIES_COUNT * GALx_FP_MAX_LIGHTS_LIMIT,
    
    GALx_LIGHTMODEL_AMBIENT             = GALx_LIGHTMODEL_FIRST_ID,        ///< Lightmodel ambient color
    GALx_LIGHTMODEL_FRONT_SCENECOLOR = GALx_LIGHTMODEL_FIRST_ID + 1,    ///< Lightmodel front face scence color
    GALx_LIGHTMODEL_BACK_SCENECOLOR  = GALx_LIGHTMODEL_FIRST_ID + 2,    ///< Lightmodel back face scene color

    GALx_LIGHTMODEL_PROPERTIES_COUNT = 3,

    //////////////////////////////////////////
    // Texture coordinate generation planes //
    //////////////////////////////////////////

    GALx_TEXGEN_FIRST_ID = GALx_LIGHTMODEL_FIRST_ID + GALx_LIGHTMODEL_PROPERTIES_COUNT,

    GALx_TEXGEN_EYE_S     = GALx_TEXGEN_FIRST_ID,        ///< Texture coordinate S Eye plane coeffients
    GALx_TEXGEN_EYE_T     = GALx_TEXGEN_FIRST_ID + 1,    ///< Texture coordinate T Eye plane coeffients
    GALx_TEXGEN_EYE_R     = GALx_TEXGEN_FIRST_ID + 2,    ///< Texture coordinate R Eye plane coeffients
    GALx_TEXGEN_EYE_Q     = GALx_TEXGEN_FIRST_ID + 3,    ///< Texture coordinate Q Eye plane coeffients
    GALx_TEXGEN_OBJECT_S = GALx_TEXGEN_FIRST_ID + 4,    ///< Texture coordinate S Object plane coeffients
    GALx_TEXGEN_OBJECT_T = GALx_TEXGEN_FIRST_ID + 5,    ///< Texture coordinate T Object plane coeffients
    GALx_TEXGEN_OBJECT_R = GALx_TEXGEN_FIRST_ID + 6,    ///< Texture coordinate R Object plane coeffients
    GALx_TEXGEN_OBJECT_Q = GALx_TEXGEN_FIRST_ID + 7,    ///< Texture coordinate Q Object plane coeffients
    // ... implicit enums for texgen 1, 2, ... up to GALx_FP_MAX_TEXTURE_STAGES_LIMIT - 1
    
    GALx_TEXGEN_PROPERTIES_COUNT = 8,

    ////////////////////
    // Fog properties //
    ////////////////////

    GALx_FOG_FIRST_ID     = GALx_TEXGEN_FIRST_ID + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXGEN_PROPERTIES_COUNT,

    GALx_FOG_COLOR        = GALx_FOG_FIRST_ID,        ///< FOG combine color
    GALx_FOG_DENSITY      = GALx_FOG_FIRST_ID + 1,    ///< FOG exponent (only for non-linear modes)
    GALx_FOG_LINEAR_START = GALx_FOG_FIRST_ID + 2,    ///< FOG start position (only for linear mode)
    GALx_FOG_LINEAR_END   = GALx_FOG_FIRST_ID + 3,    ///< FOG end position (only for linear mode)

    GALx_FOG_PROPERTIES_COUNT = 4,

    //////////////////////////////
    // Texture stage properties //
    //////////////////////////////

    GALx_TEXENV_FIRST_ID = GALx_FOG_FIRST_ID + GALx_FOG_PROPERTIES_COUNT,

    GALx_TEXENV_COLOR = GALx_TEXENV_FIRST_ID, ///< Texture stage environment color
    // ... Implicit enums for texenv 1, 2, ..., up to GALx_FP_MAX_TEXTURE_STAGES_LIMIT - 1

    GALx_TEXENV_PROPERTIES_COUNT = 1,

    ////////////////////////////
    // Depth range properties //
    ////////////////////////////

    GALx_DEPTH_FIRST_ID = GALx_TEXENV_FIRST_ID + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXENV_PROPERTIES_COUNT,

    GALx_DEPTH_RANGE_NEAR = GALx_DEPTH_FIRST_ID,    ///< The depth range near distance
    GALx_DEPTH_RANGE_FAR = GALx_DEPTH_FIRST_ID + 1,    ///< The depth range far distance

    GALx_DEPTH_RANGE_PROPERTIES_COUNT = 2,

    ///////////////////////////
    // Alpha test properties //
    ///////////////////////////

    GALx_ALPHA_TEST_FIRST_ID = GALx_DEPTH_FIRST_ID + GALx_DEPTH_RANGE_PROPERTIES_COUNT,
    
    GALx_ALPHA_TEST_REF_VALUE = GALx_ALPHA_TEST_FIRST_ID,    ///< The alpha test reference value

    GALx_ALPHA_TEST_PROPERTIES_COUNT = 1,

    /////////////////////////////
    // Transformation matrices //
    /////////////////////////////

    GALx_MATRIX_FIRST_ID = GALx_MATERIAL_PROPERTIES_COUNT + GALx_FP_MAX_LIGHTS_LIMIT * GALx_LIGHT_PROPERTIES_COUNT + GALx_LIGHTMODEL_PROPERTIES_COUNT + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXGEN_PROPERTIES_COUNT + GALx_FOG_PROPERTIES_COUNT + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXENV_PROPERTIES_COUNT + GALx_DEPTH_RANGE_PROPERTIES_COUNT + GALx_ALPHA_TEST_PROPERTIES_COUNT,

    GALx_M_MODELVIEW  = GALx_MATRIX_FIRST_ID,    ///< The modelview transformation matrix
    // ... Implicit enums for modelview 1, 2, ..., up to GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT - 1
    GALx_M_PROJECTION = GALx_M_MODELVIEW + GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT, ///< The projection matrix
    GALx_M_TEXTURE    = GALx_M_PROJECTION + 1,  ///< The texture coordinate generation matrix

    GALx_MATRIX_COUNT = GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT + 1 + GALx_FP_MAX_TEXTURE_STAGES_LIMIT,
    
    //////////////////////
    // Last Dummy state //
    //////////////////////

    GALx_LAST_STATE = GALx_MATERIAL_PROPERTIES_COUNT + GALx_FP_MAX_LIGHTS_LIMIT * GALx_LIGHT_PROPERTIES_COUNT + GALx_LIGHTMODEL_PROPERTIES_COUNT + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXGEN_PROPERTIES_COUNT + GALx_FOG_PROPERTIES_COUNT + GALx_FP_MAX_TEXTURE_STAGES_LIMIT * GALx_TEXENV_PROPERTIES_COUNT + GALx_DEPTH_RANGE_PROPERTIES_COUNT + GALx_ALPHA_TEST_PROPERTIES_COUNT + GALx_MATRIX_COUNT,
};

} // namespace libGAL

#endif // GALx_STORED_FP_ITEM_ID_H
