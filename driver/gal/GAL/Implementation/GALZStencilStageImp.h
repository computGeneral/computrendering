/**************************************************************************
 *
 */

#ifndef GAL_ZSTENCILSTAGE_IMP
    #define GAL_ZSTENCILSTAGE_IMP

#include "GALTypes.h"
#include "GALZStencilStage.h"
#include "HAL.h"
#include "StateItem.h"
#include "StateComponent.h"
#include "StateItemUtils.h"
#include "GALStoredItemID.h"
#include "GALStoredStateImp.h"

namespace libGAL
{

class GALDeviceImp;
class StoredStateItem;

class GALZStencilStageImp : public GALZStencilStage, public StateComponent
{
public:

    GALZStencilStageImp(GALDeviceImp* device, HAL* driver);

    virtual void setZEnabled(gal_bool enable);

    virtual gal_bool isZEnabled() const;

    virtual void setZFunc(GAL_COMPARE_FUNCTION zFunc);

    virtual GAL_COMPARE_FUNCTION getZFunc() const;

    virtual void setZMask(gal_bool mask);

    virtual gal_bool getZMask() const;

    virtual void setStencilEnabled(gal_bool enable);

    virtual gal_bool isStencilEnabled() const;

    virtual void setStencilOp( GAL_FACE face, 
                               GAL_STENCIL_OP onStencilFail,
                               GAL_STENCIL_OP onStencilPassZFail,
                               GAL_STENCIL_OP onStencilPassZPass);
    virtual void getStencilOp( GAL_FACE face,
                               GAL_STENCIL_OP& onStencilFail,
                               GAL_STENCIL_OP& onStencilPassZFail,
                               GAL_STENCIL_OP& onStencilPassZPass) const;

    virtual void setStencilFunc( GAL_FACE face, GAL_COMPARE_FUNCTION func, gal_uint stencilRef, gal_uint mask );

    virtual void setDepthRange (gal_float near, gal_float far);

    virtual void setD3D9DepthRangeMode(gal_bool mode);
    
    virtual void setPolygonOffset (gal_float factor, gal_float units);

    virtual void setStencilUpdateMask (gal_uint mask);

    virtual void setZStencilBufferDefined(gal_bool enable);
    
    void sync();

    void forceSync();

    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    GALStoredState* saveAllState() const;

    void restoreAllState(const GALStoredState* ssi);

    std::string getInternalState() const;

private:

    GALDeviceImp* _device;
    HAL* _driver;
    gal_bool _syncRequired;

    StateItem<gal_bool> _zEnabled;
    StateItem<gal_bool> _stencilEnabled;
    StateItem<gal_float> _depthRangeNear;
    StateItem<gal_float> _depthRangeFar;
    StateItem<gal_bool> _d3d9DepthRange;
    
    StateItem<GAL_COMPARE_FUNCTION> _zFunc;
    StateItem<gal_bool> _zMask;

    StateItem<gal_float> _depthSlopeFactor;
    StateItem<gal_float> _depthUnitOffset;

    StateItem<gal_uint> _stencilUpdateMask;

    StateItem<gal_uint> _zStencilBufferDefined;
    
    // Per face state
    struct _FaceInfo
    {
        StateItem<GAL_STENCIL_OP> onStencilFail;
        StateItem<GAL_STENCIL_OP> onStencilPassZFails;
        StateItem<GAL_STENCIL_OP> onStencilPassZPass;

        StateItem<GAL_COMPARE_FUNCTION> stencilFunc;
        StateItem<gal_uint> stencilRef;
        StateItem<gal_uint> stencilMask;

        _FaceInfo(GAL_STENCIL_OP onStencilFail, GAL_STENCIL_OP onStencilPassZFails, GAL_STENCIL_OP onStencilPassZPass,
                  GAL_COMPARE_FUNCTION stencilFunc, gal_uint stencilRef, gal_uint stencilMask) :
                onStencilFail(onStencilFail),
                onStencilPassZFails(onStencilPassZFails),
                onStencilPassZPass(onStencilPassZPass),
                stencilFunc(stencilFunc),
                stencilRef(stencilRef),
                stencilMask(stencilMask)
        {}
    };

    _FaceInfo _front;
    _FaceInfo _back;

    static void _getGPUCompareFunction(GAL_COMPARE_FUNCTION comp, arch::GPURegData* data);
    static void _getGPUStencilOperation(GAL_STENCIL_OP op, arch::GPURegData* data);
    
    class CompareFunctionPrint: public PrintFunc<GAL_COMPARE_FUNCTION>
    {
    public:

        virtual const char* print(const GAL_COMPARE_FUNCTION& var) const;
    } compareFunctionPrint;

    class StencilOperationPrint: public PrintFunc<GAL_STENCIL_OP>
    {
    public:

        virtual const char* print(const GAL_STENCIL_OP& var) const;
    } stencilOperationPrint;

    //static const char* _getCompareFunctionString(GAL_COMPARE_FUNCTION comp);
    //static const char* _getStencilOperationString(GAL_STENCIL_OP op);

    BoolPrint boolPrint;
};

}

#endif // GAL_ZSTENCILSTAGE_IMP
