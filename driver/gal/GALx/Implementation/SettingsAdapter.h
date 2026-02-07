/**************************************************************************
 *
 */

#ifndef SETTINGS_ADAPTER_H
    #define SETTINGS_ADAPTER_H

#include "GALxFixedPipelineSettings.h"

#include "GALxTLState.h"
#include "GALxFPState.h"

namespace libGAL
{

class SettingsAdapter
{
public:
    
    SettingsAdapter(const libGAL::GALx_FIXED_PIPELINE_SETTINGS& fpSettings);

    GALxTLState& getTLState() { return tlState; }

    GALxFPState& getFPState() { return fpState; }

private:

    libGAL::GALx_FIXED_PIPELINE_SETTINGS settings;

    /**
     * Implements GALxTLState
     */
    class TLStateImplementation; // define an implementation

    friend class TLStateImplementation; // Allow internal access to parent class (SettingsAdapter)

    class TLStateImplementation : public GALxTLState
    {
    private:
        SettingsAdapter* parent; // The parent class

    public:
        TLStateImplementation(SettingsAdapter* parent);

        bool normalizeNormals();
        bool isLighting();
        bool isRescaling();
        bool infiniteViewer();
        bool localLightSource(Light light);
        void attenuationCoeficients(Light light, float& constantAtt, float& linearAtt, float& quadraticAtt);
        bool isSpotLight(Light light);
        bool isLightEnabled(Light light);
        int maxLights();
        bool separateSpecular();
        bool twoSidedLighting();
        bool anyLightEnabled();
        bool anyLocalLight();

        bool isCullingEnabled();
        bool isFrontFaceCulling();
        bool isBackFaceCulling();

        GALxTLState::ColorMaterialMode colorMaterialMode();
        bool colorMaterialFrontEnabled();
        bool colorMaterialBackEnabled();

        bool isTextureUnitEnabled(GLuint texUnit);
        bool anyTextureUnitEnabled();
        int maxTextureUnits();
        GALxTLState::TextureUnit getTextureUnit(GLuint unit);

        GALxTLState::FogCoordSrc fogCoordSrc();
        bool fogEnabled();

    } tlState; // declare a GALxTLState implementor object

    /**
     * Implements GALxFPState
     */
    class FPStateImplementation; // define an implementation

    friend class FPStateImplementation; // Allow internal access to parent class (SettingsAdapter)

    class FPStateImplementation : public GALxFPState
    {
    private:
        SettingsAdapter* parent; // The parent class
    
    public:

        FPStateImplementation(SettingsAdapter* parent);

        bool separateSpecular();
        bool anyTextureUnitEnabled();
        bool isTextureUnitEnabled(GLuint unit);
        int maxTextureUnits();
        GALxFPState::TextureUnit getTextureUnit(GLuint unit);
        bool fogEnabled();
        GALxFPState::FogMode fogMode();

        GALxFPState::AlphaTestFunc alphaTestFunc();
        bool alphaTestEnabled();

    } fpState; // declare a GALxFPState implementor object
};

} // namespace libGAL

#endif // SETTINGS_ADAPTER_H
