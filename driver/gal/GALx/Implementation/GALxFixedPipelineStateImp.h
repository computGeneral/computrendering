/**************************************************************************
 *
 */

#ifndef GALx_FIXED_PIPELINE_STATE_IMP_H
    #define GALx_FIXED_PIPELINE_STATE_IMP_H

#include "GALxGlobalTypeDefinitions.h"

#include "GALxFixedPipelineState.h"
#include "StateItem.h"

#include "GALxGLState.h"

#include "GALxLightImp.h"
#include "GALxMaterialImp.h"

#include "GALxStoredFPStateImp.h"

#include <vector>
#include <string>

namespace libGAL
{

class GALxFixedPipelineStateImp: public GALxFixedPipelineState, public GALxTransformAndLightingStage, public GALxTextCoordGenerationStage, public GALxFragmentShadingStage, public GALxPostShadingStage 
{
public:

    //////////////////////////////////////
    // GALxFixedPipelineState interface //
    //////////////////////////////////////

    /**
     * Gets an interface to the transform & lighting stages.
     */
    virtual GALxTransformAndLightingStage& tl();

    /**
     * Gets an interface to the texture coordinate generation stage.
     */
    virtual GALxTextCoordGenerationStage& txtcoord();

    /**
     * Gets an interface to the fragment shading and texturing stages.
     */
    virtual GALxFragmentShadingStage& fshade();

    /**
     * Gets an interface to the post fragment shading stages (FOG and Alpha).
     */
    virtual GALxPostShadingStage& postshade();

    /**
     * Save a group of Fixed Pipeline states
     *
     * @param siIds List of state identifiers to save
     * @returns The group of states saved
     */
    virtual GALxStoredFPState* saveState(GALxStoredFPItemIDList siIds) const;

/**
     * Save a single Fixed Pipeline state
     *
     * @param    stateId the identifier of the state to save
     * @returns The group of states saved
     */
    virtual GALxStoredFPState* saveState(GALx_STORED_FP_ITEM_ID stateId) const;

    /**
     * Save the whole set of Fixed Pipeline states
     *
     * @returns The whole states group saved
     */
    virtual GALxStoredFPState* saveAllState() const;

    /**
     * Restores the value of a group of Fixed Pipeline states
     *
     * @param state a previously saved state group to be restored
     */
    virtual void restoreState(const GALxStoredFPState* state);

    /**
     * Releases a GALxStoredFPState interface object
     *
     * @param state The saved state object to be released
     */
    virtual void destroyState(GALxStoredFPState* state) const;

    /**
     * Dumps the GALxFixedPipelineState state
     */
    virtual void DBG_dump(const gal_char* file, gal_enum flags) const;

    /**
     * Saves a file with an image of the current GALxFixedPipelineState state
     */
    virtual gal_bool DBG_save(const gal_char* file) const;

    /**
     * Restores the GALxFixedPipelineState state from a image file previously saved
     */
    virtual gal_bool DBG_load(const gal_char* file);

    /////////////////////////////////////////////
    // GALxTransformAndLightingStage interface //
    /////////////////////////////////////////////

    /**
     * Select an light object to be configured
     *
     * @param light The light number.
     * @return        The corresponding GALxLight object.
     */
    virtual GALxLight& light(gal_uint light);

    /**
     * Select a face material object to be configured
     *
     * @param face  The material properties face.
     * @return        The corresponding GALxMaterial object.
     */
    virtual GALxMaterial& material(GALx_FACE face);

    /**
     * Sets the light model ambient color
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightModelAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a);
        
    /**
     * Sets the light model scene color
     *
     * @param face  The face where to set the scene color.
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightModelSceneColor(GALx_FACE face, gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the Modelview Matrix.
     *
     * @param unit     The vertex unit matrix(Vertex blending support).
     * @param matrix The input matrix.
     */
    virtual void setModelviewMatrix(gal_uint unit, const GALxFloatMatrix4x4& matrix);

    /**
     * Gets the Modelview Matrix
     *
     * @param unit The vertex unit matrix
     */
    virtual GALxFloatMatrix4x4 getModelviewMatrix(gal_uint unit) const;

    /**
     * Sets the Projection Matrix.
     *
     * @param matrix The input matrix.
     */
    virtual void setProjectionMatrix(const GALxFloatMatrix4x4& matrix);

    /**
     * Gets the Projection Matrix
     *
     * @return A copy of the current Projection Matrix
     */
    virtual GALxFloatMatrix4x4 getProjectionMatrix() const;

    ////////////////////////////////////////////
    // GALxTextCoordGenerationStage interface //
    ////////////////////////////////////////////

    /**
     * Sets the texture coordinate generation object and eye planes.
     *
     * @param textureStage The texture stage affected.
     * @param plane           The Eye or Object plane.
     * @param coord           The texture coordinate affected.
     * @param a               The a coefficient of the plane equation.
     * @param b               The b coefficient of the plane equation.
     * @param c               The c coefficient of the plane equation.
     * @param d               The d coefficient of the plane equation.
     */
    virtual void setTextureCoordPlane(gal_uint textureStage, GALx_TEXTURE_COORD_PLANE plane, GALx_TEXTURE_COORD coord, gal_float a, gal_float b, gal_float c, gal_float d);

    /**
     * Sets the texture coordinate matrix
     *
     * @param textureStage The texture stage affected.
     * @param matrix       The texture coordinate matrix.
     */
    virtual void setTextureCoordMatrix(gal_uint textureStage, const GALxFloatMatrix4x4& matrix);

    /**
     * Gets the texture coordinate matrix
     *
     * @param textureStage The texture stage matrix selected
     */
    virtual GALxFloatMatrix4x4 getTextureCoordMatrix(gal_uint textureStage) const;

    ////////////////////////////////////////
    // GALxFragmentShadingStage interface //
    ////////////////////////////////////////

    /**
     * Sets the texture environment color used for each texture stage.
     *
     * @param textureStage    The corresponding texture stage.
     * @param r                The red component of the color.
     * @param g                The green component of the color.
     * @param b                The    blue component of the color.
     * @param a                The alpha component of the color.
     */
    virtual void setTextureEnvColor(gal_uint textureStage, gal_float r, gal_float g, gal_float b, gal_float a);

   /**
    * Sets the depth range (near and far plane distances)
    *
    * @param near           The distance of the near plane.
    * @param far           The distance of the far plane.
    */
    virtual void setDepthRange(gal_float near, gal_float far);

    ////////////////////////////////////
    // GALxPostShadingStage interface //
    ////////////////////////////////////

   /**
    * Sets the FOG blending color.
    *
    * @param r        The red component of the color.
    * @param g        The green component of the color.
    * @param b        The blue component of the color.
    * @param a        The alpha component of the color.
    */
    virtual void setFOGBlendColor(gal_float r, gal_float g, gal_float b, gal_float a); 

    /**
     * Sets the FOG density exponent
     *
     * @param exponent The density exponent
     */
     virtual void setFOGDensity(gal_float exponent);

    /**
     * Sets the FOG linear mode start and end distances.
     *
     * @param start The FOG linear start distance.
     * @param end   The FOG linear end distance.
     */
     virtual void setFOGLinearDistances(gal_float start, gal_float end);

    /**
     * Sets the Alpha Test Reference Value.
     *
     * @param refValue The reference value.
     */
     virtual void setAlphaTestRefValue(gal_float refValue);

//////////////////////////
//  interface extension //
//////////////////////////
public:

    GALxFixedPipelineStateImp(GALxGLState *gls);

    const GALxGLState *getGLState() const;

    void sync();

    void forceSync();

    std::string getInternalState() const;

    gal_float getSingleState(GALx_STORED_FP_ITEM_ID stateId) const;

    GALxFloatVector3 getVectorState3(GALx_STORED_FP_ITEM_ID stateId) const;

    GALxFloatVector4 getVectorState4(GALx_STORED_FP_ITEM_ID stateId) const;
    
    GALxFloatMatrix4x4 getMatrixState(GALx_STORED_FP_ITEM_ID stateId) const;

    void getGLStateId(GALx_STORED_FP_ITEM_ID stateId, VectorId& vId, gal_uint& componentMask, MatrixId& mId, gal_uint& unit, MatrixType& type, gal_bool& isMatrix) const;

    void setSingleState(GALx_STORED_FP_ITEM_ID stateId, gal_float value);

    void setVectorState3(GALx_STORED_FP_ITEM_ID stateId, GALxFloatVector3 value);

    void setVectorState4(GALx_STORED_FP_ITEM_ID stateId, GALxFloatVector4 value);

    void setMatrixState(GALx_STORED_FP_ITEM_ID stateId, GALxFloatMatrix4x4 value);

    static void printStateEnum(std::ostream& os, GALx_STORED_FP_ITEM_ID stateId);

    const StoredStateItem* createStoredStateItem(GALx_STORED_FP_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    ~GALxFixedPipelineStateImp();

private:

    GALxGLState* _gls;

    friend class GALxLightImp;
    friend class GALxMaterialImp;

    std::vector<GALxLightImp*> _light;
    std::vector<GALxMaterialImp*> _material;

    gal_bool _requiredSync;

    //////////////////////////
    // Internal State Items //
    //////////////////////////

    // Surface Material

    StateItem<GALxFloatVector4> _materialFrontAmbient;
    StateItem<GALxFloatVector4> _materialFrontDiffuse;
    StateItem<GALxFloatVector4> _materialFrontSpecular;
    StateItem<GALxFloatVector4> _materialFrontEmission;
    StateItem<gal_float>        _materialFrontShininess;
    
    StateItem<GALxFloatVector4> _materialBackAmbient;
    StateItem<GALxFloatVector4> _materialBackDiffuse;
    StateItem<GALxFloatVector4> _materialBackSpecular;
    StateItem<GALxFloatVector4> _materialBackEmission;
    StateItem<gal_float>        _materialBackShininess;

    // Light properties

    std::vector<StateItem<GALxFloatVector4> > _lightAmbient;
    std::vector<StateItem<GALxFloatVector4> > _lightDiffuse;
    std::vector<StateItem<GALxFloatVector4> > _lightSpecular;
    std::vector<StateItem<GALxFloatVector4> > _lightPosition;
    std::vector<StateItem<GALxFloatVector3> > _lightDirection;
    std::vector<StateItem<GALxFloatVector3> > _lightAttenuation;
    std::vector<StateItem<GALxFloatVector3> > _lightSpotDirection;
    std::vector<StateItem<gal_float> >          _lightSpotCutOffAngle;
    std::vector<StateItem<gal_float> >          _lightSpotExponent;

    // Light model properties

    StateItem<GALxFloatVector4> _lightModelAmbientColor;
    StateItem<GALxFloatVector4> _lightModelSceneFrontColor;
    StateItem<GALxFloatVector4> _lightModelSceneBackColor;
    
    // Transformation matrices

    std::vector<StateItem<GALxFloatMatrix4x4> > _modelViewMatrix;
    StateItem<GALxFloatMatrix4x4>               _projectionMatrix;

    // Texture coordinate generation planes

    std::vector<StateItem<GALxFloatVector4> > _textCoordSObjectPlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordTObjectPlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordRObjectPlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordQObjectPlane;

    std::vector<StateItem<GALxFloatVector4> > _textCoordSEyePlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordTEyePlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordREyePlane;
    std::vector<StateItem<GALxFloatVector4> > _textCoordQEyePlane;

    std::vector<StateItem<GALxFloatMatrix4x4> > _textureCoordMatrix;

    // Texture stage environment colors

    std::vector<StateItem<GALxFloatVector4> > _textEnvColor;

    // Depth range

    StateItem<gal_float> _near;
    StateItem<gal_float> _far;

    // FOG params

    StateItem<GALxFloatVector4> _fogBlendColor;
    StateItem<gal_float>        _fogDensity;
    StateItem<gal_float>        _fogLinearStart;
    StateItem<gal_float>        _fogLinearEnd;

    // Alpha test params

    StateItem<gal_float>        _alphaTestRefValue;
};

} // namespace libGAL

#endif // GALx_FIXED_PIPELINE_STATE_IMP_H
