/**************************************************************************
 *
 */

#ifndef GAL_BLENDINGSTAGE_IMP
    #define GAL_BLENDINGSTAGE_IMP

#include "GALTypes.h"
#include "GALVector.h"
#include "GALBlendingStage.h"
#include "HAL.h"
#include "StateItem.h"
#include "StateComponent.h"
#include "StateItemUtils.h"
#include "GALStoredItemID.h"
#include "GALStoredStateImp.h"
#include <string>

namespace libGAL
{

class GALDeviceImp;
class StoredStateItem;

class GALBlendingStageImp : public GALBlendingStage, public StateComponent
{
public:

    GALBlendingStageImp(GALDeviceImp* device, HAL* driver);

    virtual void setEnable(gal_uint renderTargetID, gal_bool enable);
    virtual void setSrcBlend(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlend);
    virtual void setDestBlend(gal_uint renderTargetID, GAL_BLEND_OPTION destBlend);
    virtual void setBlendFunc(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendOp);
    virtual void setSrcBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlendAlpha);
    virtual void setDestBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION destBlendAlpha);
    virtual void setBlendFuncAlpha(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendOpAlpha);

    virtual void setBlendColor(gal_uint renderTargetID, gal_float red, gal_float green, gal_float blue, gal_float alpha);
    virtual void setBlendColor(gal_uint renderTargetID, const gal_float* rgba);

    virtual void setColorMask(gal_uint renderTargetID, gal_bool red, gal_bool green, gal_bool blue, gal_bool alpha);
    virtual void disableColorWrite(gal_uint renderTargetID);
    virtual void enableColorWrite(gal_uint renderTargetID);
    virtual gal_bool getColorEnable(gal_uint renderTargetID);
    
    std::string getInternalState() const;

    void sync();

    void forceSync();

    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    GALStoredState* saveAllState() const;

    void restoreAllState(const GALStoredState* ssi);

private:

    static const gal_uint MAX_RENDER_TARGETS = GAL_MAX_RENDER_TARGETS;

    gal_bool _syncRequired;
    HAL* _driver;

    std::vector<StateItem<gal_bool> > _enabled;

    std::vector<StateItem<GAL_BLEND_FUNCTION> > _blendFunc;
    std::vector<StateItem<GAL_BLEND_OPTION> > _srcBlend;
    std::vector<StateItem<GAL_BLEND_OPTION> > _destBlend;

    std::vector<StateItem<GAL_BLEND_FUNCTION> > _blendFuncAlpha; // Not supported by CG1 Simulator
    std::vector<StateItem<GAL_BLEND_OPTION> > _srcBlendAlpha;
    std::vector<StateItem<GAL_BLEND_OPTION> > _destBlendAlpha;

    typedef GALVector<gal_float,4> GALFloatVector4;

    std::vector<StateItem<GALFloatVector4> > _blendColor;

    // Color Mask
    std::vector<StateItem<gal_bool> > _redMask;
    std::vector<StateItem<gal_bool> > _greenMask;
    std::vector<StateItem<gal_bool> > _blueMask;
    std::vector<StateItem<gal_bool> > _alphaMask;
    
    std::vector<StateItem<gal_bool> > _colorWriteEnabled;

    static void _getGPUBlendOption(GAL_BLEND_OPTION option, cg1gpu::GPURegData* data);
    static void _getGPUBlendFunction(GAL_BLEND_FUNCTION func, cg1gpu::GPURegData* data);
    
    class BlendOptionPrint: public PrintFunc<GAL_BLEND_OPTION>
    {
    public:

        virtual const char* print(const GAL_BLEND_OPTION& var) const;        

    } blendOptionPrint;

    class BlendFunctionPrint: public PrintFunc<GAL_BLEND_FUNCTION>
    {
    public:
    
        virtual const char* print(const GAL_BLEND_FUNCTION& var) const;        
    
    } blendFunctionPrint;

    BoolPrint boolPrint;
};

}


#endif // GAL_BLENDINGSTAGE_IMP
