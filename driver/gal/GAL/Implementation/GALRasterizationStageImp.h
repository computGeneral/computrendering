/**************************************************************************
 *
 */

#ifndef GAL_RASTERIZATIONSTAGE_IMP
    #define GAL_RASTERIZATIONSTAGE_IMP

#include "GALTypes.h"
#include "GALRasterizationStage.h"
#include "HAL.h"
#include <string>
#include "StateItem.h"
#include "StateComponent.h"
#include "StateItemUtils.h"
#include "GALStoredStateImp.h"
#include "GALStoredItemID.h"

namespace libGAL
{

class GALDeviceImp;
class StoredStateItem;

class GALRasterizationStageImp : public GALRasterizationStage, public StateComponent
{
public:

    GALRasterizationStageImp(GALDeviceImp* device, HAL* driver);

    virtual void setInterpolationMode(gal_uint fshInputAttribute, GAL_INTERPOLATION_MODE mode);

    virtual void enableScissor(gal_bool enable);

    virtual void setFillMode(GAL_FILL_MODE fillMode);

    virtual void setCullMode(GAL_CULL_MODE cullMode);

    virtual void setFaceMode(GAL_FACE_MODE faceMode);

    virtual void setViewport(gal_int xMin, gal_int yMin, gal_uint xMax, gal_uint yMax);

    virtual void setScissor(gal_int xMin, gal_int yMin, gal_uint xMax, gal_uint yMax);

    virtual void getViewport(gal_int &xMin, gal_int &yMin, gal_uint &xMax, gal_uint &yMax);

    virtual void getScissor(gal_bool &enabled, gal_int &xMin, gal_int &yMin, gal_uint &xMax, gal_uint &yMax);

    virtual void useD3D9RasterizationRules(gal_bool use);
    
    virtual void useD3D9PixelCoordConvention(gal_bool use);

    // StateComponent interface

    /**
     * Writes all the rasterization state that requires update
     */
    void sync();

    /**
     * Writes all the rasterization state without checking if updating is required
     */
    void forceSync();

    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    GALStoredState* saveAllState() const;

    void restoreAllState(const GALStoredState* ssi);

    std::string getInternalState() const;

private:

    std::vector<StateItem<GAL_INTERPOLATION_MODE> > _interpolation;

    HAL* _driver;

    GALDeviceImp* _device;

    gal_bool _syncRequired;

    // Fill mode state
    StateItem<GAL_FILL_MODE> _fillMode;

    // Culling state
    StateItem<GAL_CULL_MODE> _cullMode;

    // Facing state
    StateItem<GAL_FACE_MODE> _faceMode;

    // Viewport state
    StateItem<gal_uint> _xViewport;
    StateItem<gal_uint> _yViewport;
    StateItem<gal_uint> _widthViewport;
    StateItem<gal_uint> _heightViewport;

    // Scissor state
    StateItem<gal_bool> _scissorEnabled;
    StateItem<gal_uint> _xScissor;
    StateItem<gal_uint> _yScissor;
    StateItem<gal_uint> _widthScissor;
    StateItem<gal_uint> _heightScissor;

    //  Rasterization rules.
    StateItem<gal_bool> _useD3D9RasterizationRules;
    
    //  Pixel coordinates convention
    StateItem<gal_bool> _useD3D9PixelCoordConvention;
    
    static void _getGPUCullMode(GAL_CULL_MODE mode, arch::GPURegData* data);
    static void _getGPUFaceMode(GAL_FACE_MODE mode, arch::GPURegData* data);
    static void _getGPUInterpolationMode(GAL_INTERPOLATION_MODE mode, arch::GPURegData* data);

    //  Allow the device implementation class to access private attributes.
    friend class GALDeviceImp;

    class CullModePrint: public PrintFunc<GAL_CULL_MODE>
    {
    public:

        virtual const char* print(const GAL_CULL_MODE& cullMode) const;
    } cullModePrint;

    class InterpolationModePrint : public PrintFunc<GAL_INTERPOLATION_MODE>
    {
        virtual const char* print(const GAL_INTERPOLATION_MODE& iMode) const;
    } interpolationModePrint;

};

}

#endif // GAL_RASTERIZATIONSTAGE_IMP
