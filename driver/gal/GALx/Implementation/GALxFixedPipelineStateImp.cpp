/**************************************************************************
 *
 */

#include "GALxFixedPipelineStateImp.h"
#include "GALxStoredFPStateImp.h"
#include "StateItemUtils.h"
#include <sstream>
#include <fstream>

using namespace libGAL;

/////////////////////////////////////////////////////
// GALxFixedPipelineState interface implementation //
/////////////////////////////////////////////////////

GALxTransformAndLightingStage& GALxFixedPipelineStateImp::tl()
{
    return *this;
}

GALxTextCoordGenerationStage& GALxFixedPipelineStateImp::txtcoord()
{
    return *this;
}

GALxFragmentShadingStage& GALxFixedPipelineStateImp::fshade()
{
    return *this;
}

GALxPostShadingStage& GALxFixedPipelineStateImp::postshade()
{
    return *this;
}

GALxStoredFPState* GALxFixedPipelineStateImp::saveState(GALxStoredFPItemIDList siIds) const
{
    GALxStoredFPStateImp* ret = new GALxStoredFPStateImp();

    GALxStoredFPItemIDList::const_iterator iter = siIds.begin();
    
    while( iter != siIds.end() )
    {
        ret->addStoredStateItem(createStoredStateItem((*iter)));        
        iter++;
    }

    return ret;
}

GALxStoredFPState* GALxFixedPipelineStateImp::saveState(GALx_STORED_FP_ITEM_ID stateId) const
{
    GALxStoredFPStateImp* ret = new GALxStoredFPStateImp();

    ret->addStoredStateItem(createStoredStateItem(stateId));        

    return ret;
}

GALxStoredFPState* GALxFixedPipelineStateImp::saveAllState() const
{
    GALxStoredFPStateImp* ret = new GALxStoredFPStateImp();

    for (gal_uint i=0; i < GALx_LAST_STATE; i++)
    {
        ret->addStoredStateItem(createStoredStateItem(GALx_STORED_FP_ITEM_ID(i)));        
    }

    return ret;
}

void GALxFixedPipelineStateImp::restoreState(const GALxStoredFPState* state)
{
    const GALxStoredFPStateImp* sFPsi = static_cast<const GALxStoredFPStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = sFPsi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();
    
    while ( iter != ssiList.end() )
    {
        restoreStoredStateItem((*iter));
        iter++;
    }
}

void GALxFixedPipelineStateImp::destroyState(GALxStoredFPState* state) const
{
    const GALxStoredFPStateImp* sFPsi = static_cast<const GALxStoredFPStateImp*>(state);

    std::list<const StoredStateItem*> stateList = sFPsi->getSSIList();

    std::list<const StoredStateItem*>::iterator iter = stateList.begin();

    while ( iter != stateList.end() )
    {
        delete (*iter);
        iter++;
    }

    delete sFPsi;
}

////////////////////////////////////////////////////////////
// GALxTransformAndLightingStage interface implementation //
////////////////////////////////////////////////////////////

GALxLight& GALxFixedPipelineStateImp::light(gal_uint light)
{
    if (light >= GALx_FP_MAX_LIGHTS_LIMIT)
        CG_ASSERT("Unexpected light number");

    return *(_light[light]);
}

GALxMaterial& GALxFixedPipelineStateImp::material(GALx_FACE face)
{
    switch(face)
    {
        case GALx_FRONT:
            return (*_material[0]);
        case GALx_BACK:
            return (*_material[1]);
        case GALx_FRONT_AND_BACK: // Can only ask for a single face material state
        default:
            CG_ASSERT("Unexpected material face");
            return (*_material[0]); // to avoid stupid compiler warnings

    }
}

void GALxFixedPipelineStateImp::setLightModelAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a)
{    
    GALxFloatVector4 color;
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;

    _lightModelAmbientColor = color;
}

void GALxFixedPipelineStateImp::setLightModelSceneColor(GALx_FACE face, gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 color;
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;

    switch(face)
    {
        case GALx_FRONT: 
        case GALx_BACK:
        case GALx_FRONT_AND_BACK: // Setting the two faces at the same time is allowed
            break;
        default:
            CG_ASSERT("Unexpected material face");
    }

    if (face == GALx_FRONT || face == GALx_FRONT_AND_BACK)
    {
        _lightModelSceneFrontColor = color;    
    }

    if (face == GALx_BACK || face == GALx_FRONT_AND_BACK)
    {
        _lightModelSceneBackColor = color;    
    }
}

void GALxFixedPipelineStateImp::setModelviewMatrix(gal_uint unit, const GALxFloatMatrix4x4& matrix)
{
    if (unit >= GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT)
        CG_ASSERT("Unexpected modelview matrix number");

    _modelViewMatrix[unit] = matrix;
}

GALxFloatMatrix4x4 GALxFixedPipelineStateImp::getModelviewMatrix(gal_uint unit) const
{
    if ( unit >= GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT )
        CG_ASSERT("Unexpected modelview matrix number");

    return _modelViewMatrix[unit];
}

void GALxFixedPipelineStateImp::setProjectionMatrix(const GALxFloatMatrix4x4& matrix)
{
    _projectionMatrix = matrix;
}

GALxFloatMatrix4x4 GALxFixedPipelineStateImp::getProjectionMatrix() const
{
    return _projectionMatrix;
}

///////////////////////////////////////////////////////////
// GALxTextCoordGenerationStage interface implementation //
///////////////////////////////////////////////////////////

void GALxFixedPipelineStateImp::setTextureCoordPlane(gal_uint textureStage, GALx_TEXTURE_COORD_PLANE plane, GALx_TEXTURE_COORD coord, gal_float a, gal_float b, gal_float c, gal_float d)
{
    if (textureStage >= GALx_FP_MAX_TEXTURE_STAGES_LIMIT)
        CG_ASSERT("Unexpected texture stage number");

    switch(plane)
    {
        case GALx_OBJECT_PLANE: 
        case GALx_EYE_PLANE:
            break;
        default:
            CG_ASSERT("Unexpected plane");
    }

    switch(coord)
    {
        case GALx_COORD_S: 
        case GALx_COORD_T:
        case GALx_COORD_R:
        case GALx_COORD_Q:
            break;
        default:
            CG_ASSERT("Unexpected coordinate");
    }

    GALxFloatVector4 planev;
    planev[0] = a; planev[1] = b; planev[2] = c; planev[3] = d;

    if ( coord == GALx_COORD_S )
    {
        if ( plane == GALx_OBJECT_PLANE )
            _textCoordSObjectPlane[textureStage] = planev;
        else // plane == GALx_EYE_PLANE
            _textCoordSEyePlane[textureStage] = planev;
    }
    else if ( coord == GALx_COORD_T )
    {
        if ( plane == GALx_OBJECT_PLANE )
            _textCoordTObjectPlane[textureStage] = planev;
        else // plane == GALx_EYE_PLANE
            _textCoordTEyePlane[textureStage] = planev;
    }
    else if ( coord == GALx_COORD_R )
    {
        if ( plane == GALx_OBJECT_PLANE )
            _textCoordRObjectPlane[textureStage] = planev;
        else // plane == GALx_EYE_PLANE
            _textCoordREyePlane[textureStage] = planev;
    }
    else if ( coord == GALx_COORD_Q )
    {
        if ( plane == GALx_OBJECT_PLANE )
            _textCoordQObjectPlane[textureStage] = planev;
        else // plane == GALx_EYE_PLANE
            _textCoordQEyePlane[textureStage] = planev;
    }
}

void GALxFixedPipelineStateImp::setTextureCoordMatrix(gal_uint textureStage, const GALxFloatMatrix4x4& matrix)
{
    _textureCoordMatrix[textureStage] = matrix;
}

GALxFloatMatrix4x4 GALxFixedPipelineStateImp::getTextureCoordMatrix(gal_uint textureStage) const
{
    return _textureCoordMatrix[textureStage];
}

///////////////////////////////////////////////////////
// GALxFragmentShadingStage interface implementation //
///////////////////////////////////////////////////////

void GALxFixedPipelineStateImp::setTextureEnvColor(gal_uint textureStage, gal_float r, gal_float g, gal_float b, gal_float a)
{
    if (textureStage >= GALx_FP_MAX_TEXTURE_STAGES_LIMIT)
        CG_ASSERT("Unexpected texture stage number");

    GALxFloatVector4 color;
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;

    _textEnvColor[textureStage] = color;
}

void GALxFixedPipelineStateImp::setDepthRange(gal_float nearValue, gal_float farValue)
{
    _near = nearValue;
    _far = farValue;
}

///////////////////////////////////////////////////
// GALxPostShadingStage interface implementation //
///////////////////////////////////////////////////

void GALxFixedPipelineStateImp::setFOGBlendColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 color;
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;

    _fogBlendColor = color;
}

void GALxFixedPipelineStateImp::setFOGDensity(gal_float exponent)
{
    _fogDensity = exponent;
}

void GALxFixedPipelineStateImp::setFOGLinearDistances(gal_float start, gal_float end)
{
    _fogLinearStart = start;
    _fogLinearEnd = end;
}

void GALxFixedPipelineStateImp::setAlphaTestRefValue(gal_float refValue)
{
    _alphaTestRefValue = refValue;
}

//////////////////////////////////////////////
// GALxFixedPipelineStateImp implementation //
//////////////////////////////////////////////

GALxFixedPipelineStateImp::GALxFixedPipelineStateImp(GALxGLState *gls)
:   
    _gls(gls),
    _requiredSync(false),

    _light(GALx_FP_MAX_LIGHTS_LIMIT),
    _material(2),

    // Material state initialization

    _materialFrontAmbient(getVectorState4(GALx_MATERIAL_FRONT_AMBIENT)),
    _materialFrontDiffuse(getVectorState4(GALx_MATERIAL_FRONT_DIFFUSE)),
    _materialFrontSpecular(getVectorState4(GALx_MATERIAL_FRONT_SPECULAR)),
    _materialFrontEmission(getVectorState4(GALx_MATERIAL_FRONT_EMISSION)),
    _materialFrontShininess(getSingleState(GALx_MATERIAL_FRONT_SHININESS)),
        
    _materialBackAmbient(getVectorState4(GALx_MATERIAL_BACK_AMBIENT)),
    _materialBackDiffuse(getVectorState4(GALx_MATERIAL_BACK_DIFFUSE)),
    _materialBackSpecular(getVectorState4(GALx_MATERIAL_BACK_SPECULAR)),
    _materialBackEmission(getVectorState4(GALx_MATERIAL_BACK_EMISSION)),
    _materialBackShininess(getSingleState(GALx_MATERIAL_BACK_SHININESS)),

    // Light state initialization

    _lightAmbient(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState4(GALx_LIGHT_AMBIENT)),
    _lightDiffuse(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState4(GALx_LIGHT_DIFFUSE)),
    _lightSpecular(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState4(GALx_LIGHT_SPECULAR)),
    _lightPosition(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState4(GALx_LIGHT_POSITION)),
    _lightDirection(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState3(GALx_LIGHT_DIRECTION)),
    _lightAttenuation(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState3(GALx_LIGHT_ATTENUATION)),
    _lightSpotDirection(GALx_FP_MAX_LIGHTS_LIMIT, getVectorState3(GALx_LIGHT_SPOT_DIRECTION)),
    _lightSpotCutOffAngle(GALx_FP_MAX_LIGHTS_LIMIT, getSingleState(GALx_LIGHT_SPOT_CUTOFF)),
    _lightSpotExponent(GALx_FP_MAX_LIGHTS_LIMIT, getSingleState(GALx_LIGHT_SPOT_EXPONENT)),

    // Light model state initialization

    _lightModelAmbientColor(getVectorState4(GALx_LIGHTMODEL_AMBIENT)),
    _lightModelSceneFrontColor(getVectorState4(GALx_LIGHTMODEL_FRONT_SCENECOLOR)),
    _lightModelSceneBackColor(getVectorState4(GALx_LIGHTMODEL_BACK_SCENECOLOR)),

    // Transformation matrices initialization

    _modelViewMatrix(GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT, getMatrixState(GALx_M_MODELVIEW)),
    _projectionMatrix(getMatrixState(GALx_M_PROJECTION)),

    // Texture coordinate generation planes initialization

    _textCoordSObjectPlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_OBJECT_S)),
    _textCoordTObjectPlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_OBJECT_T)),
    _textCoordRObjectPlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_OBJECT_R)),
    _textCoordQObjectPlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_OBJECT_Q)),

    _textCoordSEyePlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_EYE_S)),
    _textCoordTEyePlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_EYE_T)),
    _textCoordREyePlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_EYE_R)),
    _textCoordQEyePlane(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXGEN_EYE_Q)),

    // Texture coordinate matrices

    _textureCoordMatrix(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getMatrixState(GALx_M_TEXTURE)),

    // Texture stage environment colors

    _textEnvColor(GALx_FP_MAX_TEXTURE_STAGES_LIMIT, getVectorState4(GALx_TEXENV_COLOR)),

    // Depth range

    _near(gal_float(0)),    // This state is not located in GALxGLState
    _far(gal_float(1)),        // This state is not located in GALxGLState

    // FOG params

    _fogBlendColor(getVectorState4(GALx_FOG_COLOR)),
    _fogDensity(getSingleState(GALx_FOG_DENSITY)),
    _fogLinearStart(getSingleState(GALx_FOG_LINEAR_START)),
    _fogLinearEnd(getSingleState(GALx_FOG_LINEAR_END)),

    // Alpha test params

    _alphaTestRefValue(gal_float(0)) // This state is not located in GALxGLState
{

    _fogBlendColor = getVectorState4(GALx_FOG_COLOR);

    if (!gls)
        CG_ASSERT("GALxGLState null pointer is not allowed");

    for (gal_uint i = 0; i < GALx_FP_MAX_LIGHTS_LIMIT; i++)
        _light[i] = new GALxLightImp(i, this);

    for (gal_uint i = 1; i < GALx_FP_MAX_LIGHTS_LIMIT; i++){
       _lightDiffuse[i] = getVectorState4((GALx_STORED_FP_ITEM_ID)(GALx_LIGHT_DIFFUSE+9));
       _lightDiffuse[i].restart();
    }

    for (gal_uint i = 1; i < GALx_FP_MAX_LIGHTS_LIMIT; i++){
        _lightSpecular[i] = getVectorState4((GALx_STORED_FP_ITEM_ID)(GALx_LIGHT_SPECULAR+9));
       _lightSpecular[i].restart();
    }

    _material[0] = new GALxMaterialImp(0, this);
    _material[1] = new GALxMaterialImp(1, this);
}

GALxFixedPipelineStateImp::~GALxFixedPipelineStateImp()
{
    for (gal_uint i = 0; i < GALx_FP_MAX_LIGHTS_LIMIT; i++)
        delete _light[i];

    delete _material[0];
    delete _material[1];
}

const GALxGLState* GALxFixedPipelineStateImp::getGLState() const
{
    return _gls;
}

gal_float GALxFixedPipelineStateImp::getSingleState(GALx_STORED_FP_ITEM_ID stateId) const
{
    gal_float ret = 1.0f;
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    switch(stateId)
    {
        // Put here the code for resolution of special Fixed Pipeline States
        // not present in GALxGLState
        case GALx_ALPHA_TEST_REF_VALUE:
            return _alphaTestRefValue;
    }

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Requesting a Matrix State. Use getMatrixState() instead");

    switch(componentMask)
    {
        case 0x08: ret = _gls->getVector(vId)[0]; break;
        case 0x04: ret = _gls->getVector(vId)[1]; break;
        case 0x02: ret = _gls->getVector(vId)[2]; break;
        case 0x01: ret = _gls->getVector(vId)[3]; break;
        default:
            CG_ASSERT("Not a single State. Use getVectorState4() or getVectorState3() instead");
    }

    return ret;
}

GALxFloatVector3 GALxFixedPipelineStateImp::getVectorState3(GALx_STORED_FP_ITEM_ID stateId) const
{
    GALxFloatVector3 ret(gal_float(1.0f));
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Requesting a Matrix State. Use getMatrixState() instead");

    switch(componentMask)
    {
        case 0x07: ret[0] = _gls->getVector(vId)[1]; 
                   ret[1] = _gls->getVector(vId)[2]; 
                   ret[2] = _gls->getVector(vId)[3]; 
                   break;
        case 0x0B: ret[0] = _gls->getVector(vId)[0]; 
                   ret[1] = _gls->getVector(vId)[2]; 
                   ret[2] = _gls->getVector(vId)[3]; 
                   break;
        case 0x0D: ret[0] = _gls->getVector(vId)[0]; 
                   ret[1] = _gls->getVector(vId)[1]; 
                   ret[2] = _gls->getVector(vId)[3]; 
                   break;
        case 0x0E: ret[0] = _gls->getVector(vId)[0]; 
                   ret[1] = _gls->getVector(vId)[1]; 
                   ret[2] = _gls->getVector(vId)[2]; 
                   break;
        default:
            CG_ASSERT("Not a vector3 State. Use getSingleState() or getVectorState4() instead");

    }
    
    return ret;
}

GALxFloatVector4 GALxFixedPipelineStateImp::getVectorState4(GALx_STORED_FP_ITEM_ID stateId) const
{
    GALxFloatVector4 ret(gal_float(1.0f));
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Requesting a Matrix State. Use getMatrixState() instead");

    if (componentMask != 0x0F)
        CG_ASSERT("Not a vector4 State. Use getSingleState() or getVectorState3() instead");

    ret[0] = _gls->getVector(vId)[0];
    ret[1] = _gls->getVector(vId)[1];
    ret[2] = _gls->getVector(vId)[2];
    ret[3] = _gls->getVector(vId)[3];

    return ret;
}
    
GALxFloatMatrix4x4 GALxFixedPipelineStateImp::getMatrixState(GALx_STORED_FP_ITEM_ID stateId) const
{
    GALxFloatMatrix4x4 ret(gal_float(0));
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (!isMatrix)
        CG_ASSERT("Requesting a Vector State. Use getSingleState(), getVectorState3() or getVectorState4()");

    const GALxMatrixf& mat = _gls->getMatrix(mId, type, unit);

    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
            ret[i][j] = mat.getRows()[i*4 + j];

    return ret;
}

void GALxFixedPipelineStateImp::setSingleState(GALx_STORED_FP_ITEM_ID stateId, gal_float value)
{
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    switch(stateId)
    {
        // Put here the code for resolution of special Fixed Pipeline States
        // not present in GALxGLState
        case GALx_ALPHA_TEST_REF_VALUE:
            _alphaTestRefValue = value;
            return;
    }

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Setting a Matrix State. Use getMatrixState() instead");

    Quadf glsvalue = _gls->getVector(vId);

    switch(componentMask)
    {
        case 0x08:  glsvalue[0] = value; break;
        case 0x04:  glsvalue[1] = value; break;
        case 0x02:  glsvalue[2] = value; break;
        case 0x01:  glsvalue[3] = value; break;
        default:
            CG_ASSERT("Not a single State. Use setVectorState4() or setVectorState3() instead");
    }

    _gls->setVector(glsvalue, vId);
}

void GALxFixedPipelineStateImp::setVectorState3(GALx_STORED_FP_ITEM_ID stateId, GALxFloatVector3 value)
{
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Setting a Matrix State. Use setMatrixState() instead");

    Quadf glsvalue = _gls->getVector(vId);

    switch(componentMask)
    {
        case 0x07: glsvalue[1] = value[0]; 
                   glsvalue[2] = value[1]; 
                   glsvalue[3] = value[2]; 
                   break;
        case 0x0B: glsvalue[0] = value[0]; 
                   glsvalue[2] = value[1]; 
                   glsvalue[3] = value[2]; 
                   break;
        case 0x0D: glsvalue[0] = value[0]; 
                   glsvalue[1] = value[1]; 
                   glsvalue[3] = value[2]; 
                   break;
        case 0x0E: glsvalue[0] = value[0]; 
                   glsvalue[1] = value[1]; 
                   glsvalue[2] = value[2]; 
                   break;
        default:
            CG_ASSERT("Not a vector3 State. Use setSingleState() or setVectorState4() instead");

    }
    
    _gls->setVector(glsvalue, vId);
}

void GALxFixedPipelineStateImp::setVectorState4(GALx_STORED_FP_ITEM_ID stateId, GALxFloatVector4 value)
{
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (isMatrix)
        CG_ASSERT("Setting a Matrix State. Use setMatrixState() instead");

    if (componentMask != 0x0F)
        CG_ASSERT("Not a vector4 State. Use setSingleState() or setVectorState3() instead");

    Quadf glsvalue = _gls->getVector(vId);

    glsvalue[0] = value[0];
    glsvalue[1] = value[1];
    glsvalue[2] = value[2];
    glsvalue[3] = value[3];

    _gls->setVector(glsvalue, vId);
}

void GALxFixedPipelineStateImp::setMatrixState(GALx_STORED_FP_ITEM_ID stateId, GALxFloatMatrix4x4 value)
{
    VectorId vId;
    MatrixId mId;
    gal_uint componentMask;
    gal_uint unit;
    MatrixType type;
    gal_bool isMatrix;

    getGLStateId(stateId, vId, componentMask, mId, unit, type, isMatrix);

    if (!isMatrix)
        CG_ASSERT("Setting a Vector State. Use setSingleState(), setVectorState3() or setVectorState4()");

    const GALxMatrixf& glsvalue = _gls->getMatrix(mId, type, unit);

    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
            glsvalue.getRows()[i*4 + j] = value[i][j];
}

void GALxFixedPipelineStateImp::sync()
{
    if ( _materialFrontAmbient.changed() || _requiredSync )
    {
        _materialFrontAmbient.restart();
        setVectorState4(GALx_MATERIAL_FRONT_AMBIENT, _materialFrontAmbient);
    }

    if ( _materialFrontDiffuse.changed() || _requiredSync )
    {
        _materialFrontDiffuse.restart();
        setVectorState4(GALx_MATERIAL_FRONT_DIFFUSE, _materialFrontDiffuse);
    }

    if ( _materialFrontSpecular.changed() || _requiredSync )
    {
        _materialFrontSpecular.restart();
        setVectorState4(GALx_MATERIAL_FRONT_SPECULAR, _materialFrontSpecular);
    }

    if ( _materialFrontEmission.changed() || _requiredSync )
    {
        _materialFrontEmission.restart();
        setVectorState4(GALx_MATERIAL_FRONT_EMISSION, _materialFrontEmission);
    }

    if ( _materialFrontShininess.changed() || _requiredSync )
    {
        _materialFrontShininess.restart();
        setSingleState(GALx_MATERIAL_FRONT_SHININESS, _materialFrontShininess);
    }

    if ( _materialBackAmbient.changed() || _requiredSync )
    {
        _materialBackAmbient.restart();
        setVectorState4(GALx_MATERIAL_BACK_AMBIENT, _materialBackAmbient);
    }

    if ( _materialBackDiffuse.changed() || _requiredSync )
    {
        _materialBackDiffuse.restart();
        setVectorState4(GALx_MATERIAL_BACK_DIFFUSE, _materialBackDiffuse);
    }

    if ( _materialBackSpecular.changed() || _requiredSync )
    {
        _materialBackSpecular.restart();
        setVectorState4(GALx_MATERIAL_BACK_DIFFUSE, _materialBackDiffuse);
    }

    if ( _materialBackEmission.changed() || _requiredSync )
    {
        _materialBackEmission.restart();
        setVectorState4(GALx_MATERIAL_BACK_EMISSION, _materialBackDiffuse);
    
    }

    if ( _materialBackShininess.changed() || _requiredSync )
    {
        _materialBackShininess.restart();
        setSingleState(GALx_MATERIAL_BACK_SHININESS, _materialBackShininess);
    }

    for (gal_uint i=0; i < GALx_FP_MAX_LIGHTS_LIMIT; i++)
    {
        if ( _lightAmbient[i].changed() || _requiredSync )
        {
            _lightAmbient[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_AMBIENT + GALx_LIGHT_PROPERTIES_COUNT * i), _lightAmbient[i]);
        }

        if ( _lightDiffuse[i].changed() || _requiredSync )
        {
            _lightDiffuse[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_DIFFUSE + GALx_LIGHT_PROPERTIES_COUNT * i), _lightDiffuse[i]);
        }

        if ( _lightSpecular[i].changed() || _requiredSync )
        {
            _lightSpecular[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_SPECULAR + GALx_LIGHT_PROPERTIES_COUNT * i), _lightSpecular[i]);
        }
        
        if ( _lightPosition[i].changed() || _requiredSync )
        {
            _lightPosition[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_POSITION + GALx_LIGHT_PROPERTIES_COUNT * i), _lightPosition[i]);
        }
        
        if ( _lightDirection[i].changed() || _requiredSync )
        {
            _lightDirection[i].restart();
            setVectorState3(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_DIRECTION + GALx_LIGHT_PROPERTIES_COUNT * i), _lightDirection[i]);
        }
        
        if ( _lightAttenuation[i].changed() || _requiredSync )
        {
            _lightAttenuation[i].restart();
            setVectorState3(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_ATTENUATION + GALx_LIGHT_PROPERTIES_COUNT * i), _lightAttenuation[i]);
        }
        
        if ( _lightSpotDirection[i].changed() || _requiredSync )
        {
            _lightSpotDirection[i].restart();
            setVectorState3(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_SPOT_DIRECTION + GALx_LIGHT_PROPERTIES_COUNT * i), _lightSpotDirection[i]);
        }
        
        if ( _lightSpotCutOffAngle[i].changed() || _requiredSync )
        {
            _lightSpotCutOffAngle[i].restart();
            setSingleState(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_SPOT_CUTOFF + GALx_LIGHT_PROPERTIES_COUNT * i), _lightSpotCutOffAngle[i]);
        }
        
        if ( _lightSpotExponent[i].changed() || _requiredSync )
        {
            _lightSpotExponent[i].restart();
            setSingleState(GALx_STORED_FP_ITEM_ID(GALx_LIGHT_SPOT_EXPONENT + GALx_LIGHT_PROPERTIES_COUNT * i), _lightSpotExponent[i]);
        }
    }

    if (_lightModelAmbientColor.changed() || _requiredSync )
    {
        _lightModelAmbientColor.restart();
        setVectorState4(GALx_LIGHTMODEL_AMBIENT, _lightModelAmbientColor);
    }

    if (_lightModelSceneFrontColor.changed() || _requiredSync )
    {
        _lightModelSceneFrontColor.restart();
        setVectorState4(GALx_LIGHTMODEL_FRONT_SCENECOLOR, _lightModelSceneFrontColor);
    }

    if (_lightModelSceneBackColor.changed() || _requiredSync )
    {
        _lightModelSceneBackColor.restart();
        setVectorState4(GALx_LIGHTMODEL_BACK_SCENECOLOR, _lightModelSceneBackColor);
    }

    for (gal_uint i=0; i < GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT ; i++)
    {
        if (_modelViewMatrix[i].changed() || _requiredSync )
        {
            _modelViewMatrix[i].restart();
            setMatrixState(GALx_STORED_FP_ITEM_ID(GALx_M_MODELVIEW + i), _modelViewMatrix[i]);
        }
    }

    if (_projectionMatrix.changed() || _requiredSync )
    {
        _projectionMatrix.restart();
        setMatrixState(GALx_M_PROJECTION, _projectionMatrix);
    }


    for (gal_uint i=0; i < GALx_FP_MAX_TEXTURE_STAGES_LIMIT ; i++)
    {
        if (_textCoordSObjectPlane[i].changed() || _requiredSync )
        {
            _textCoordSObjectPlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_OBJECT_S + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordSObjectPlane[i]);
        }

        if (_textCoordTObjectPlane[i].changed() || _requiredSync )
        {
            _textCoordTObjectPlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_OBJECT_T + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordTObjectPlane[i]);
        }

        if (_textCoordRObjectPlane[i].changed() || _requiredSync )
        {
            _textCoordRObjectPlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_OBJECT_R + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordRObjectPlane[i]);
        }

        if (_textCoordQObjectPlane[i].changed() || _requiredSync )
        {
            _textCoordQObjectPlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_OBJECT_Q + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordQObjectPlane[i]);
        }

        if (_textCoordSEyePlane[i].changed() || _requiredSync )
        {
            _textCoordSEyePlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_EYE_S + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordSEyePlane[i]);
        }

        if (_textCoordTEyePlane[i].changed() || _requiredSync )
        {
            _textCoordTEyePlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_EYE_T + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordTEyePlane[i]);
        }

        if (_textCoordREyePlane[i].changed() || _requiredSync )
        {
            _textCoordREyePlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_EYE_R + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordREyePlane[i]);
        }

        if (_textCoordQEyePlane[i].changed() || _requiredSync )
        {
            _textCoordQEyePlane[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXGEN_EYE_Q + GALx_TEXGEN_PROPERTIES_COUNT * i), _textCoordQEyePlane[i]);
        }

        if (_textureCoordMatrix[i].changed() || _requiredSync )
        {
            _textureCoordMatrix[i].restart();
            setMatrixState(GALx_STORED_FP_ITEM_ID(GALx_M_TEXTURE + i),_textureCoordMatrix[i]);
        }

        if (_textEnvColor[i].changed() || _requiredSync )
        {
            _textEnvColor[i].restart();
            setVectorState4(GALx_STORED_FP_ITEM_ID(GALx_TEXENV_COLOR + i), _textEnvColor[i]);
        }

    }
    
    if (_near.changed() || _requiredSync )
    {
        _near.restart();
        // Do nothing. Already synchronized in GALxFixedPipelineState
        //setSingleState(GALx_DEPTH_RANGE_NEAR, _near);
    }

    if (_far.changed() || _requiredSync )
    {
        _far.restart();
        // Do nothing. Already synchronized in GALxFixedPipelineState
        //setSingleState(GALx_DEPTH_RANGE_FAR, _far);
    }

    if (_fogBlendColor.changed() || _requiredSync )
    {
        _fogBlendColor.restart();
        setVectorState4(GALx_FOG_COLOR, _fogBlendColor);
    }

    if (_fogDensity.changed() || _requiredSync )
    {
        _fogDensity.restart();
        setSingleState(GALx_FOG_DENSITY, _fogDensity);
    }

    if (_fogLinearStart.changed() || _requiredSync )
    {
        _fogLinearStart.restart();
        setSingleState(GALx_FOG_LINEAR_START, _fogLinearStart);
    }

    if (_fogLinearEnd.changed() || _requiredSync )
    {
        _fogLinearEnd.restart();
        setSingleState(GALx_FOG_LINEAR_END, _fogLinearEnd);
    }

    if (_alphaTestRefValue.changed() || _requiredSync )
    {
        _alphaTestRefValue.restart();
        // Do nothing. Already synchronized in GALxFixedPipelineState
        //setSingleState(GALx_ALPHA_TEST_REF_VALUE, _alphaTestRefValue);
    }
}


void GALxFixedPipelineStateImp::forceSync()
{
    _requiredSync = true;
    sync();
    _requiredSync = false;
}

string GALxFixedPipelineStateImp::getInternalState() const
{
    stringstream ss;
    stringstream ssAux;

    ss << stateItemString(_materialFrontAmbient,"MATERIAL_FRONT_AMBIENT", false);
    ss << stateItemString(_materialFrontDiffuse,"MATERIAL_FRONT_DIFFUSE", false);
    ss << stateItemString(_materialFrontSpecular,"MATERIAL_FRONT_SPECULAR", false);
    ss << stateItemString(_materialFrontEmission,"MATERIAL_FRONT_EMISSION", false);
    ss << stateItemString(_materialFrontShininess,"MATERIAL_FRONT_SHININESS", false);

    ss << stateItemString(_materialBackAmbient,"MATERIAL_BACK_AMBIENT", false);
    ss << stateItemString(_materialBackDiffuse,"MATERIAL_BACK_DIFFUSE", false);
    ss << stateItemString(_materialBackSpecular,"MATERIAL_BACK_SPECULAR", false);
    ss << stateItemString(_materialBackEmission,"MATERIAL_BACK_EMISSION", false);
    ss << stateItemString(_materialBackShininess,"MATERIAL_BACK_SHININESS", false);
    
    ssAux.str("");
    for (gal_uint i=0; i < GALx_FP_MAX_LIGHTS_LIMIT; i++)
    {
        ssAux << "LIGHT" << i << "_AMBIENT_COLOR"; ss << stateItemString(_lightAmbient[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_DIFFUSE_COLOR"; ss << stateItemString(_lightDiffuse[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_SPECULAR_COLOR"; ss << stateItemString(_lightSpecular[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_POSITION"; ss << stateItemString(_lightPosition[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_DIRECTION"; ss << stateItemString(_lightDirection[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_ATTENUATION_COEFFS"; ss << stateItemString(_lightAttenuation[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_SPOT_DIRECTION"; ss << stateItemString(_lightSpotDirection[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_SPOT_CUT_OFF"; ss << stateItemString(_lightSpotCutOffAngle[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "LIGHT" << i << "_SPOT_EXPONENT"; ss << stateItemString(_lightSpotExponent[i], ssAux.str().c_str(), false); ssAux.str("");
    }

    ss << stateItemString(_lightModelAmbientColor, "LIGHTMODEL_AMBIENT_COLOR", false);
    ss << stateItemString(_lightModelSceneFrontColor, "LIGHTMODEL_FRONT_SCENE_COLOR", false);
    ss << stateItemString(_lightModelSceneBackColor, "LIGHTMODEL_BACK_SCENE_COLOR", false);

    ssAux.str("");
    for (gal_uint i=0; i < GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT ; i++)
    {
        ssAux << "MODELVIEW" << i << "_MATRIX"; ss << stateItemString(_modelViewMatrix[i], ssAux.str().c_str(), false); ssAux.str("");
    }

    ss << stateItemString(_projectionMatrix, "PROJECTION_MATRIX", false);

    ssAux.str("");
    for (gal_uint i=0; i < GALx_FP_MAX_TEXTURE_STAGES_LIMIT ; i++)
    {
        ssAux << "TEXTURE_STAGE" << i << "_COORD_S_OBJECT_PLANE"; ss << stateItemString(_textCoordSObjectPlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_T_OBJECT_PLANE"; ss << stateItemString(_textCoordTObjectPlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_R_OBJECT_PLANE"; ss << stateItemString(_textCoordRObjectPlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_Q_OBJECT_PLANE"; ss << stateItemString(_textCoordQObjectPlane[i], ssAux.str().c_str(), false); ssAux.str("");

        ssAux << "TEXTURE_STAGE" << i << "_COORD_S_EYE_PLANE"; ss << stateItemString(_textCoordSEyePlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_T_EYE_PLANE"; ss << stateItemString(_textCoordTEyePlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_R_EYE_PLANE"; ss << stateItemString(_textCoordREyePlane[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_COORD_Q_EYE_PLANE"; ss << stateItemString(_textCoordQEyePlane[i], ssAux.str().c_str(), false); ssAux.str("");

        ssAux << "TEXTURE_STAGE" << i << "_COORD_MATRIX"; ss << stateItemString(_textureCoordMatrix[i], ssAux.str().c_str(), false); ssAux.str("");
        ssAux << "TEXTURE_STAGE" << i << "_ENV_COLOR"; ss << stateItemString(_textEnvColor[i], ssAux.str().c_str(), false); ssAux.str("");
    }

    ss << stateItemString(_near, "DEPTH_NEAR_VALUE", false);
    ss << stateItemString(_far, "DEPTH_FAR_VALUE", false);
    ss << stateItemString(_fogBlendColor, "FOG_BLENDING_COLOR", false);
    ss << stateItemString(_fogDensity, "FOG_DENSITY", false);
    ss << stateItemString(_fogLinearStart, "FOG_LINEAR_START", false);
    ss << stateItemString(_fogLinearEnd, "FOG_LINEAR_END", false);
    ss << stateItemString(_alphaTestRefValue, "ALPHA_TEST_REF_VALUE", false);

    return ss.str();
}

void GALxFixedPipelineStateImp::getGLStateId(GALx_STORED_FP_ITEM_ID stateId, VectorId& vId, gal_uint& componentMask, MatrixId& mId, gal_uint& unit, MatrixType& type, gal_bool& isMatrix) const
{
    if (stateId >= GALx_MATRIX_FIRST_ID)
    {    
        // It큦 a matrix state
        isMatrix = true;
        
        if ((stateId >= GALx_M_MODELVIEW) && (stateId < GALx_M_MODELVIEW + GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT))
        {    
            // It큦 a modelview matrix
            unit = stateId - GALx_M_MODELVIEW;
            mId = M_MODELVIEW;
            type = MT_NONE;
        }
        else if ( (stateId >= GALx_M_TEXTURE) && (stateId < GALx_M_TEXTURE + GALx_FP_MAX_TEXTURE_STAGES_LIMIT))
        {
            // It큦 a texture matrix
            unit = stateId - GALx_M_TEXTURE;
            mId = M_TEXTURE;
            type = MT_NONE;
        }
        else if ( stateId == GALx_M_PROJECTION )
        {
            // It큦 a projection matrix
            unit = 0;
            mId = M_PROJECTION;
            type = MT_NONE;
        }
        // else if (... <- write here for future additional defined matrix type.
    }
    else // Vector state
    {
        isMatrix = false;
        componentMask = 0x0F;
        
        if ( stateId < GALx_LIGHT_FIRST_ID )
        {
            // It큦 a material state
            unit = 0;

            switch(stateId)
            {
                case GALx_MATERIAL_FRONT_AMBIENT: vId = VectorId(V_MATERIAL_FRONT_AMBIENT); break;
                case GALx_MATERIAL_FRONT_DIFFUSE: vId = VectorId(V_MATERIAL_FRONT_DIFUSSE); break;
                case GALx_MATERIAL_FRONT_SPECULAR: vId = VectorId(V_MATERIAL_FRONT_SPECULAR); break;
                case GALx_MATERIAL_FRONT_EMISSION: vId = VectorId(V_MATERIAL_FRONT_EMISSION); break;
                case GALx_MATERIAL_FRONT_SHININESS: vId = VectorId(V_MATERIAL_FRONT_SHININESS); componentMask = 0x08; break;
                case GALx_MATERIAL_BACK_AMBIENT: vId = VectorId(V_MATERIAL_BACK_AMBIENT); break;
                case GALx_MATERIAL_BACK_DIFFUSE: vId = VectorId(V_MATERIAL_BACK_DIFUSSE); break;
                case GALx_MATERIAL_BACK_SPECULAR: vId = VectorId(V_MATERIAL_BACK_SPECULAR); break;
                case GALx_MATERIAL_BACK_EMISSION: vId = VectorId(V_MATERIAL_BACK_EMISSION); break;
                case GALx_MATERIAL_BACK_SHININESS: vId = VectorId(V_MATERIAL_BACK_SHININESS); componentMask = 0x08; break;
            }
        }      
        else if ( stateId < GALx_LIGHTMODEL_FIRST_ID )
        {
            // It큦 a light state
            gal_uint aux = stateId - GALx_LIGHT_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_LIGHT_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseLight = GALx_STORED_FP_ITEM_ID( (aux % GALx_LIGHT_PROPERTIES_COUNT) + GALx_LIGHT_FIRST_ID);
            switch(baseLight)
            {
                case GALx_LIGHT_AMBIENT: vId = VectorId(V_LIGHT_AMBIENT + 7 * unit); break;
                case GALx_LIGHT_DIFFUSE: vId = VectorId(V_LIGHT_DIFFUSE + 7 * unit); break;
                case GALx_LIGHT_SPECULAR: vId = VectorId(V_LIGHT_SPECULAR + 7 * unit); break;
                case GALx_LIGHT_POSITION: vId = VectorId(V_LIGHT_POSITION + 7 * unit); componentMask = 0x0F; break;
                case GALx_LIGHT_DIRECTION: vId = VectorId(V_LIGHT_POSITION + 7 * unit); componentMask = 0x0E; break;
                case GALx_LIGHT_ATTENUATION: vId = VectorId(V_LIGHT_ATTENUATION + 7 * unit); componentMask = 0x0E; break;
                case GALx_LIGHT_SPOT_DIRECTION: vId = VectorId(V_LIGHT_SPOT_DIRECTION + 7 * unit); componentMask = 0x0E; break;
                case GALx_LIGHT_SPOT_CUTOFF: vId = VectorId(V_LIGHT_SPOT_DIRECTION + 7 * unit); componentMask = 0x01; break;
                case GALx_LIGHT_SPOT_EXPONENT: vId = VectorId(V_LIGHT_ATTENUATION + 7 * unit); componentMask = 0x01; break;
            }
        }
        else if ( stateId < GALx_TEXGEN_FIRST_ID )
        {
            // It큦 a lightmodel state
            switch( stateId )
            {
                case GALx_LIGHTMODEL_AMBIENT: vId = VectorId(V_LIGHTMODEL_AMBIENT); break;
                case GALx_LIGHTMODEL_FRONT_SCENECOLOR: vId = VectorId(V_LIGHTMODEL_FRONT_SCENECOLOR); break;
                case GALx_LIGHTMODEL_BACK_SCENECOLOR: vId = VectorId(V_LIGHTMODEL_BACK_SCENECOLOR); break;
            }
        }
        else if ( stateId < GALx_FOG_FIRST_ID )
        {
            // It큦 a texture generation state
            
            gal_uint aux = stateId - GALx_TEXGEN_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXGEN_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXGEN_PROPERTIES_COUNT) + GALx_TEXGEN_FIRST_ID);

            switch(baseTexStage)
            {
                case GALx_TEXGEN_EYE_S: vId = VectorId(V_TEXGEN_EYE_S + 8 * unit); break;
                case GALx_TEXGEN_EYE_T: vId = VectorId(V_TEXGEN_EYE_T + 8 * unit); break;
                case GALx_TEXGEN_EYE_R: vId = VectorId(V_TEXGEN_EYE_R + 8 * unit); break;
                case GALx_TEXGEN_EYE_Q: vId = VectorId(V_TEXGEN_EYE_Q + 8 * unit); break;
                case GALx_TEXGEN_OBJECT_S: vId = VectorId(V_TEXGEN_OBJECT_S + 8 * unit); break;
                case GALx_TEXGEN_OBJECT_T: vId = VectorId(V_TEXGEN_OBJECT_T + 8 * unit); break;
                case GALx_TEXGEN_OBJECT_R: vId = VectorId(V_TEXGEN_OBJECT_R + 8 * unit); break;
                case GALx_TEXGEN_OBJECT_Q: vId = VectorId(V_TEXGEN_OBJECT_Q + 8 * unit); break;
            }
        }
        else if ( stateId < GALx_TEXENV_FIRST_ID )
        {
            // It큦 a FOG state
            switch(stateId)
            {
                case GALx_FOG_COLOR: vId = VectorId(V_FOG_COLOR); break;
                case GALx_FOG_DENSITY: vId = VectorId(V_FOG_PARAMS); componentMask = 0x08; break;
                case GALx_FOG_LINEAR_START: vId = VectorId(V_FOG_PARAMS); componentMask = 0x04; break;
                case GALx_FOG_LINEAR_END: vId = VectorId(V_FOG_PARAMS); componentMask = 0x02; break;
            }
        }
        else if ( stateId < GALx_DEPTH_FIRST_ID )
        {
            // It큦 a texture environment state
            gal_uint aux = stateId - GALx_TEXENV_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXENV_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXENV_PROPERTIES_COUNT) + GALx_TEXENV_FIRST_ID);
        
            switch( baseTexStage )
            {
                case GALx_TEXENV_COLOR: vId = VectorId(V_TEXENV_COLOR + unit); break;
            }
        }
        else if ( stateId < GALx_ALPHA_TEST_FIRST_ID )
        {
            // It큦 a depth range state
            switch( stateId )
            {
                case GALx_DEPTH_RANGE_NEAR: 
                case GALx_DEPTH_RANGE_FAR: CG_ASSERT("Depth range not in GALxGLState! Shouldn큧 reach here! "); break;
            }
        }
        else if ( stateId < GALx_MATRIX_FIRST_ID )
        {
            // It큦 a alpha test state. 
            switch( stateId )
            {
                case GALx_ALPHA_TEST_REF_VALUE: CG_ASSERT("Alpha test ref value not in GALxGLState! Shouldn큧 reach here!"); break;
            }
        }
    }

}

void GALxFixedPipelineStateImp::printStateEnum(std::ostream& os, GALx_STORED_FP_ITEM_ID stateId)
{
    gal_uint unit = 0;

    if (stateId >= GALx_MATRIX_FIRST_ID)
    {    
        // It큦 a matrix state
        
        if ((stateId >= GALx_M_MODELVIEW) && (stateId < GALx_M_MODELVIEW + GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT))
        {    
            // It큦 a modelview matrix
            unit = stateId - GALx_M_MODELVIEW;
            os << "GALx_M_MODELVIEW(" << unit << ")";
        }
        else if ( (stateId >= GALx_M_TEXTURE) && (stateId < GALx_M_TEXTURE + GALx_FP_MAX_TEXTURE_STAGES_LIMIT))
        {
            // It큦 a texture matrix
            unit = stateId - GALx_M_TEXTURE;
            os << "GALx_M_TEXTURE(" << unit << ")";
        }
        else if ( stateId == GALx_M_PROJECTION )
        {
            // It큦 a projection matrix
            os << "GALx_M_PROJECTION";
        }
        // else if (... <- write here for future additional defined matrix type.
    }
    else // Vector state
    {
        if ( stateId < GALx_LIGHT_FIRST_ID )
        {
            // It큦 a material state
            switch(stateId)
            {
                case GALx_MATERIAL_FRONT_AMBIENT: os << "GALx_MATERIAL_FRONT_AMBIENT"; break;
                case GALx_MATERIAL_FRONT_DIFFUSE: os << "GALx_MATERIAL_FRONT_DIFFUSE"; break;
                case GALx_MATERIAL_FRONT_SPECULAR: os << "GALx_MATERIAL_FRONT_SPECULAR"; break;
                case GALx_MATERIAL_FRONT_EMISSION: os << "GALx_MATERIAL_FRONT_EMISSION"; break;
                case GALx_MATERIAL_FRONT_SHININESS: os << "GALx_MATERIAL_FRONT_SHININESS"; break;
                case GALx_MATERIAL_BACK_AMBIENT: os << "GALx_MATERIAL_BACK_AMBIENT"; break;
                case GALx_MATERIAL_BACK_DIFFUSE: os << "GALx_MATERIAL_BACK_DIFFUSE"; break;
                case GALx_MATERIAL_BACK_SPECULAR: os << "GALx_MATERIAL_BACK_SPECULAR"; break;
                case GALx_MATERIAL_BACK_EMISSION: os << "GALx_MATERIAL_BACK_EMISSION"; break;
                case GALx_MATERIAL_BACK_SHININESS: os << "GALx_MATERIAL_BACK_SHININESS"; break;
            }
        }
        else if ( stateId < GALx_LIGHTMODEL_FIRST_ID )
        {
            // It큦 a light state
            gal_uint aux = stateId - GALx_LIGHT_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_LIGHT_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseLight = GALx_STORED_FP_ITEM_ID( (aux % GALx_LIGHT_PROPERTIES_COUNT) + GALx_LIGHT_FIRST_ID);
            
            switch(baseLight)
            {
                case GALx_LIGHT_AMBIENT: os << "GALx_LIGHT_AMBIENT(" << unit << ")"; break;
                case GALx_LIGHT_DIFFUSE: os << "GALx_LIGHT_DIFFUSE(" << unit << ")"; break;
                case GALx_LIGHT_SPECULAR: os << "GALx_LIGHT_SPECULAR(" << unit << ")"; break;
                case GALx_LIGHT_POSITION: os << "GALx_LIGHT_POSITION(" << unit << ")"; break;
                case GALx_LIGHT_DIRECTION: os << "GALx_LIGHT_DIRECTION(" << unit << ")"; break;
                case GALx_LIGHT_ATTENUATION: os << "GALx_LIGHT_ATTENUATION(" << unit << ")"; break;
                case GALx_LIGHT_SPOT_DIRECTION: os << "GALx_LIGHT_SPOT_DIRECTION(" << unit << ")"; break;
                case GALx_LIGHT_SPOT_CUTOFF: os << "GALx_LIGHT_SPOT_CUTOFF(" << unit << ")"; break;
                case GALx_LIGHT_SPOT_EXPONENT: os << "GALx_LIGHT_SPOT_EXPONENT(" << unit << ")"; break;
            }
        }
        else if ( stateId < GALx_TEXGEN_FIRST_ID )
        {
            // It큦 a lightmodel state
            switch( stateId )
            {
                case GALx_LIGHTMODEL_AMBIENT: os << "GALx_LIGHTMODEL_AMBIENT";  break;
                case GALx_LIGHTMODEL_FRONT_SCENECOLOR: os << "GALx_LIGHTMODEL_FRONT_SCENECOLOR"; break;
                case GALx_LIGHTMODEL_BACK_SCENECOLOR: os << "GALx_LIGHTMODEL_BACK_SCENECOLOR"; break;
            }
        }
        else if ( stateId < GALx_FOG_FIRST_ID )
        {
            // It큦 a texture generation state
            
            gal_uint aux = stateId - GALx_TEXGEN_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXGEN_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXGEN_PROPERTIES_COUNT) + GALx_TEXGEN_FIRST_ID);

            switch(baseTexStage)
            {
                case GALx_TEXGEN_EYE_S: os << "GALx_TEXGEN_EYE_S(" << unit << ")"; break;
                case GALx_TEXGEN_EYE_T: os << "GALx_TEXGEN_EYE_T(" << unit << ")"; break;
                case GALx_TEXGEN_EYE_R: os << "GALx_TEXGEN_EYE_R(" << unit << ")"; break;
                case GALx_TEXGEN_EYE_Q: os << "GALx_TEXGEN_EYE_Q(" << unit << ")"; break;
                case GALx_TEXGEN_OBJECT_S: os << "GALx_TEXGEN_OBJECT_S(" << unit << ")"; break;
                case GALx_TEXGEN_OBJECT_T: os << "GALx_TEXGEN_OBJECT_T(" << unit << ")"; break;
                case GALx_TEXGEN_OBJECT_R: os << "GALx_TEXGEN_OBJECT_R(" << unit << ")"; break;
                case GALx_TEXGEN_OBJECT_Q: os << "GALx_TEXGEN_OBJECT_Q(" << unit << ")"; break;
            }
        }
        else if ( stateId < GALx_TEXENV_FIRST_ID )
        {
            // It큦 a FOG state
            switch(stateId)
            {
                case GALx_FOG_COLOR: os << "GALx_FOG_COLOR"; break;
                case GALx_FOG_DENSITY: os << "GALx_FOG_DENSITY"; break;
                case GALx_FOG_LINEAR_START: os << "GALx_FOG_LINEAR_START"; break;
                case GALx_FOG_LINEAR_END: os << "GALx_FOG_LINEAR_END"; break;
            }
        }
        else if ( stateId < GALx_DEPTH_FIRST_ID )
        {
            // It큦 a texture environment state
            gal_uint aux = stateId - GALx_TEXENV_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXENV_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXENV_PROPERTIES_COUNT) + GALx_TEXENV_FIRST_ID);
        
            switch( baseTexStage )
            {
                case GALx_TEXENV_COLOR: os << "GALx_TEXENV_COLOR(" << unit << ")"; break;
            }
        }
        else if ( stateId < GALx_ALPHA_TEST_FIRST_ID )
        {
            // It큦 a depth range state
            switch( stateId )
            {
                case GALx_DEPTH_RANGE_NEAR: os << "GALx_DEPTH_RANGE_NEAR"; break;
                case GALx_DEPTH_RANGE_FAR: os << "GALx_DEPTH_RANGE_FAR"; break;
            }
        }
        else if ( stateId < GALx_MATRIX_FIRST_ID )
        {
            // It큦 a alpha test state. 
            switch( stateId )
            {
                case GALx_ALPHA_TEST_REF_VALUE: os << "GALx_ALPHA_TEST_REF_VALUE"; break;
            }
        }
    }

}

const StoredStateItem* GALxFixedPipelineStateImp::createStoredStateItem(GALx_STORED_FP_ITEM_ID stateId) const
{
    gal_uint unit = 0;
    
    GALxStoredFPStateItem* ret;

    if (stateId >= GALx_MATRIX_FIRST_ID)
    {    
        // It큦 a matrix state
        
        if ((stateId >= GALx_M_MODELVIEW) && (stateId < GALx_M_MODELVIEW + GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT))
        {    
            // It큦 a modelview matrix
            unit = stateId - GALx_M_MODELVIEW;
            ret = new GALxFloatMatrix4x4StoredStateItem(_modelViewMatrix[unit]);
        }
        else if ( (stateId >= GALx_M_TEXTURE) && (stateId < GALx_M_TEXTURE + GALx_FP_MAX_TEXTURE_STAGES_LIMIT))
        {
            // It큦 a texture matrix
            unit = stateId - GALx_M_TEXTURE;
            ret = new GALxFloatMatrix4x4StoredStateItem(_textureCoordMatrix[unit]);
        }
        else if ( stateId == GALx_M_PROJECTION )
        {
            // It큦 a projection matrix
            ret = new GALxFloatMatrix4x4StoredStateItem(_projectionMatrix);
        }
        // else if (... <- write here for future additional defined matrix type.
        else
        {
            CG_ASSERT("Unknown Matrix state");
            ret = 0;
        }
    }
    else // Vector state
    {
        if ( stateId < GALx_LIGHT_FIRST_ID )
        {
            // It큦 a material state
            switch(stateId)
            {
                case GALx_MATERIAL_FRONT_AMBIENT:  ret = new GALxFloatVector4StoredStateItem(_materialFrontAmbient); break;
                case GALx_MATERIAL_FRONT_DIFFUSE:  ret = new GALxFloatVector4StoredStateItem(_materialFrontDiffuse); break;
                case GALx_MATERIAL_FRONT_SPECULAR:  ret = new GALxFloatVector4StoredStateItem(_materialFrontSpecular); break;
                case GALx_MATERIAL_FRONT_EMISSION:  ret = new GALxFloatVector4StoredStateItem(_materialFrontEmission); break;
                case GALx_MATERIAL_FRONT_SHININESS:  ret = new GALxSingleFloatStoredStateItem(_materialFrontShininess); break;
                case GALx_MATERIAL_BACK_AMBIENT:  ret = new GALxFloatVector4StoredStateItem(_materialBackAmbient); break;
                case GALx_MATERIAL_BACK_DIFFUSE:  ret = new GALxFloatVector4StoredStateItem(_materialBackDiffuse); break;
                case GALx_MATERIAL_BACK_SPECULAR:  ret = new GALxFloatVector4StoredStateItem(_materialBackSpecular); break;
                case GALx_MATERIAL_BACK_EMISSION:  ret = new GALxFloatVector4StoredStateItem(_materialBackEmission); break;
                case GALx_MATERIAL_BACK_SHININESS:  ret = new GALxSingleFloatStoredStateItem(_materialBackShininess); break;
                // case GALx_MATERIAL_... <- add here future additional material states.
                default:
                    CG_ASSERT("Unknown material state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_LIGHTMODEL_FIRST_ID )
        {
            // It큦 a light state
            gal_uint aux = stateId - GALx_LIGHT_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_LIGHT_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseLight = GALx_STORED_FP_ITEM_ID( (aux % GALx_LIGHT_PROPERTIES_COUNT) + GALx_LIGHT_FIRST_ID);
            
            switch(baseLight)
            {
                case GALx_LIGHT_AMBIENT: ret = new GALxFloatVector4StoredStateItem(_lightAmbient[unit]); break;
                case GALx_LIGHT_DIFFUSE: ret = new GALxFloatVector4StoredStateItem(_lightDiffuse[unit]); break;
                case GALx_LIGHT_SPECULAR: ret = new GALxFloatVector4StoredStateItem(_lightSpecular[unit]); break;
                case GALx_LIGHT_POSITION: ret = new GALxFloatVector4StoredStateItem(_lightPosition[unit]); break;
                case GALx_LIGHT_DIRECTION: ret = new GALxFloatVector3StoredStateItem(_lightDirection[unit]); break;
                case GALx_LIGHT_ATTENUATION: ret = new GALxFloatVector3StoredStateItem(_lightAttenuation[unit]); break;
                case GALx_LIGHT_SPOT_DIRECTION: ret = new GALxFloatVector3StoredStateItem(_lightSpotDirection[unit]); break;
                case GALx_LIGHT_SPOT_CUTOFF: ret = new GALxSingleFloatStoredStateItem(_lightSpotCutOffAngle[unit]); break;
                case GALx_LIGHT_SPOT_EXPONENT: ret = new GALxSingleFloatStoredStateItem(_lightSpotExponent[unit]); break;
                // case GALx_LIGHT_... <- add here future additional light states.
                default:
                    CG_ASSERT("Unknown light state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_TEXGEN_FIRST_ID )
        {
            // It큦 a lightmodel state
            switch( stateId )
            {
                case GALx_LIGHTMODEL_AMBIENT: ret = new GALxFloatVector4StoredStateItem(_lightModelAmbientColor); break;
                case GALx_LIGHTMODEL_FRONT_SCENECOLOR: ret = new GALxFloatVector4StoredStateItem(_lightModelSceneFrontColor); break;
                case GALx_LIGHTMODEL_BACK_SCENECOLOR: ret = new GALxFloatVector4StoredStateItem(_lightModelSceneBackColor); break;
                // case GALx_LIGHTMODEL_... <- add here future additional lightmodel states.
                default:
                    CG_ASSERT("Unknown lightmodel state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_FOG_FIRST_ID )
        {
            // It큦 a texture generation state
            
            gal_uint aux = stateId - GALx_TEXGEN_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXGEN_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXGEN_PROPERTIES_COUNT) + GALx_TEXGEN_FIRST_ID);

            switch(baseTexStage)
            {
                case GALx_TEXGEN_EYE_S: ret = new GALxFloatVector4StoredStateItem(_textCoordSEyePlane[unit]); break;
                case GALx_TEXGEN_EYE_T: ret = new GALxFloatVector4StoredStateItem(_textCoordTEyePlane[unit]); break;
                case GALx_TEXGEN_EYE_R: ret = new GALxFloatVector4StoredStateItem(_textCoordREyePlane[unit]); break;
                case GALx_TEXGEN_EYE_Q: ret = new GALxFloatVector4StoredStateItem(_textCoordQEyePlane[unit]); break;
                case GALx_TEXGEN_OBJECT_S: ret = new GALxFloatVector4StoredStateItem(_textCoordSObjectPlane[unit]); break;
                case GALx_TEXGEN_OBJECT_T: ret = new GALxFloatVector4StoredStateItem(_textCoordTObjectPlane[unit]); break;
                case GALx_TEXGEN_OBJECT_R: ret = new GALxFloatVector4StoredStateItem(_textCoordRObjectPlane[unit]); break;
                case GALx_TEXGEN_OBJECT_Q: ret = new GALxFloatVector4StoredStateItem(_textCoordQObjectPlane[unit]); break;
                // case GALx_TEXGEN_... <- add here future additional texgen states.
                default:
                    CG_ASSERT("Unknown texgen state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_TEXENV_FIRST_ID )
        {
            // It큦 a FOG state
            switch(stateId)
            {
                case GALx_FOG_COLOR: ret = new GALxFloatVector4StoredStateItem(_fogBlendColor); break;
                case GALx_FOG_DENSITY: ret = new GALxSingleFloatStoredStateItem(_fogDensity); break;
                case GALx_FOG_LINEAR_START: ret = new GALxSingleFloatStoredStateItem(_fogLinearStart); break;
                case GALx_FOG_LINEAR_END: ret = new GALxSingleFloatStoredStateItem(_fogLinearEnd); break;
                // case GALx_FOG_... <- add here future additional FOG states.
                default:
                    CG_ASSERT("Unknown FOG state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_DEPTH_FIRST_ID )
        {
            // It큦 a texture environment state
            gal_uint aux = stateId - GALx_TEXENV_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXENV_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXENV_PROPERTIES_COUNT) + GALx_TEXENV_FIRST_ID);
        
            switch( baseTexStage )
            {
                case GALx_TEXENV_COLOR: ret = new GALxFloatVector4StoredStateItem(_textEnvColor[unit]); break;
                // case GALx_TEXENV_... <- add here future additional texenv states.
                default:
                    CG_ASSERT("Unknown texenv state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_ALPHA_TEST_FIRST_ID )
        {
            // It큦 a depth range state
            switch( stateId )
            {
                case GALx_DEPTH_RANGE_NEAR: ret = new GALxSingleFloatStoredStateItem(_near); break;
                case GALx_DEPTH_RANGE_FAR: ret = new GALxSingleFloatStoredStateItem(_far); break;
                // case GALx_DEPTH_... <- add here future additional depth range states.
                default:
                    CG_ASSERT("Unknown depth range state");
                    ret = 0;
            }
        }
        else if ( stateId < GALx_MATRIX_FIRST_ID )
        {
            // It큦 a alpha test state. 
            switch( stateId )
            {
                case GALx_ALPHA_TEST_REF_VALUE: ret = new GALxSingleFloatStoredStateItem(_alphaTestRefValue); break;
                // case GALx_ALPHA_TEST_... <- add here future additional alpha test states.
                default:
                    CG_ASSERT("Unknown alpha test state");
                    ret = 0;

            }
        }
        // else if (... <- write here for future additional defined vector state groups.
        else
        {
            CG_ASSERT("Unknown Vector state");
            ret = 0;
        }
    }
    
    ret->setItemId(stateId);

    return ret;
}

#define CAST_TO_MATRIX(X) static_cast<const GALxFloatMatrix4x4StoredStateItem*>(X)
#define CAST_TO_VECT4(X) static_cast<const GALxFloatVector4StoredStateItem*>(X)
#define CAST_TO_VECT3(X) static_cast<const GALxFloatVector3StoredStateItem*>(X)
#define CAST_TO_SINGLE(X) static_cast<const GALxSingleFloatStoredStateItem*>(X)

void GALxFixedPipelineStateImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    gal_uint unit = 0;
    
    const GALxStoredFPStateItem* sFPsi = static_cast<const GALxStoredFPStateItem*>(ssi);

    GALx_STORED_FP_ITEM_ID stateId = sFPsi->getItemId();

    if (stateId >= GALx_MATRIX_FIRST_ID)
    {    
        // It큦 a matrix state
        
        if ((stateId >= GALx_M_MODELVIEW) && (stateId < GALx_M_MODELVIEW + GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT))
        {    
            // It큦 a modelview matrix
            unit = stateId - GALx_M_MODELVIEW;
            _modelViewMatrix[unit] = *(CAST_TO_MATRIX(sFPsi));
        }
        else if ( (stateId >= GALx_M_TEXTURE) && (stateId < GALx_M_TEXTURE + GALx_FP_MAX_TEXTURE_STAGES_LIMIT))
        {
            // It큦 a texture matrix
            unit = stateId - GALx_M_TEXTURE;
            _textureCoordMatrix[unit] = *(CAST_TO_MATRIX(sFPsi));
        }
        else if ( stateId == GALx_M_PROJECTION )
        {
            // It큦 a projection matrix
            _projectionMatrix = *(CAST_TO_MATRIX(sFPsi));
        }
        // else if (... <- write here for future additional defined matrix type.
    }
    else // Vector state
    {
        if ( stateId < GALx_LIGHT_FIRST_ID )
        {
            // It큦 a material state
            switch(stateId)
            {
                case GALx_MATERIAL_FRONT_AMBIENT: _materialFrontAmbient = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_FRONT_DIFFUSE:  _materialFrontDiffuse = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_FRONT_SPECULAR:  _materialFrontSpecular = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_FRONT_EMISSION:  _materialFrontEmission = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_FRONT_SHININESS:  _materialFrontShininess = *(CAST_TO_SINGLE(sFPsi)); break;
                case GALx_MATERIAL_BACK_AMBIENT:  _materialBackAmbient = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_BACK_DIFFUSE:  _materialBackDiffuse = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_BACK_SPECULAR:  _materialBackSpecular = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_BACK_EMISSION:  _materialBackEmission = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_MATERIAL_BACK_SHININESS:  _materialBackShininess = *(CAST_TO_SINGLE(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_LIGHTMODEL_FIRST_ID )
        {
            // It큦 a light state
            gal_uint aux = stateId - GALx_LIGHT_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_LIGHT_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseLight = GALx_STORED_FP_ITEM_ID( (aux % GALx_LIGHT_PROPERTIES_COUNT) + GALx_LIGHT_FIRST_ID);
            
            switch(baseLight)
            {
                case GALx_LIGHT_AMBIENT: _lightAmbient[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHT_DIFFUSE: _lightDiffuse[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHT_SPECULAR: _lightSpecular[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHT_POSITION: _lightPosition[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHT_DIRECTION: _lightDirection[unit] = *(CAST_TO_VECT3(sFPsi)); break;
                case GALx_LIGHT_ATTENUATION: _lightAttenuation[unit] = *(CAST_TO_VECT3(sFPsi)); break;
                case GALx_LIGHT_SPOT_DIRECTION: _lightSpotDirection[unit] = *(CAST_TO_VECT3(sFPsi)); break;
                case GALx_LIGHT_SPOT_CUTOFF: _lightSpotCutOffAngle[unit] = *(CAST_TO_SINGLE(sFPsi)); break;
                case GALx_LIGHT_SPOT_EXPONENT: _lightSpotExponent[unit] = *(CAST_TO_SINGLE(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_TEXGEN_FIRST_ID )
        {
            // It큦 a lightmodel state
            switch( stateId )
            {
                case GALx_LIGHTMODEL_AMBIENT: _lightModelAmbientColor = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHTMODEL_FRONT_SCENECOLOR: _lightModelSceneFrontColor = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_LIGHTMODEL_BACK_SCENECOLOR: _lightModelSceneBackColor = *(CAST_TO_VECT4(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_FOG_FIRST_ID )
        {
            // It큦 a texture generation state
            
            gal_uint aux = stateId - GALx_TEXGEN_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXGEN_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXGEN_PROPERTIES_COUNT) + GALx_TEXGEN_FIRST_ID);

            switch(baseTexStage)
            {
                case GALx_TEXGEN_EYE_S: _textCoordSEyePlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_EYE_T: _textCoordTEyePlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_EYE_R: _textCoordREyePlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_EYE_Q: _textCoordQEyePlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_OBJECT_S: _textCoordSObjectPlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_OBJECT_T: _textCoordTObjectPlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_OBJECT_R: _textCoordRObjectPlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_TEXGEN_OBJECT_Q: _textCoordQObjectPlane[unit] = *(CAST_TO_VECT4(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_TEXENV_FIRST_ID )
        {
            // It큦 a FOG state
            switch(stateId)
            {
                case GALx_FOG_COLOR: _fogBlendColor = *(CAST_TO_VECT4(sFPsi)); break;
                case GALx_FOG_DENSITY: _fogDensity = *(CAST_TO_SINGLE(sFPsi)); break;
                case GALx_FOG_LINEAR_START: _fogLinearStart = *(CAST_TO_SINGLE(sFPsi)); break;
                case GALx_FOG_LINEAR_END: _fogLinearEnd = *(CAST_TO_SINGLE(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_DEPTH_FIRST_ID )
        {
            // It큦 a texture environment state
            gal_uint aux = stateId - GALx_TEXENV_FIRST_ID;
            unit = gal_uint(gal_float(aux) / gal_float(GALx_TEXENV_PROPERTIES_COUNT));
            
            GALx_STORED_FP_ITEM_ID baseTexStage = GALx_STORED_FP_ITEM_ID((aux % GALx_TEXENV_PROPERTIES_COUNT) + GALx_TEXENV_FIRST_ID);
        
            switch( baseTexStage )
            {
                case GALx_TEXENV_COLOR: _textEnvColor[unit] = *(CAST_TO_VECT4(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_ALPHA_TEST_FIRST_ID )
        {
            // It큦 a depth range state
            switch( stateId )
            {
                case GALx_DEPTH_RANGE_NEAR: _near = *(CAST_TO_SINGLE(sFPsi)); break;
                case GALx_DEPTH_RANGE_FAR: _far = *(CAST_TO_SINGLE(sFPsi)); break;
            }
        }
        else if ( stateId < GALx_MATRIX_FIRST_ID )
        {
            // It큦 a alpha test state. 
            switch( stateId )
            {
                case GALx_ALPHA_TEST_REF_VALUE: _alphaTestRefValue = *(CAST_TO_SINGLE(sFPsi)); break;
            }
        }
    }

}

void GALxFixedPipelineStateImp::DBG_dump(const gal_char* file, gal_enum) const
{
    fstream f(file,ios_base::out);

    f << getInternalState();

    f.close();
}

gal_bool GALxFixedPipelineStateImp::DBG_save(const gal_char*) const
{
    return true;
}

gal_bool GALxFixedPipelineStateImp::DBG_load(const gal_char*)
{
    return true;
}
