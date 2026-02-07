/**************************************************************************
 *
 */

#include "GALZStencilStageImp.h"
#include "GALMacros.h"
#include <sstream>

#include "GALStoredStateImp.h"

#include "GlobalProfiler.h"

using namespace libGAL;
using namespace std;

GALZStencilStageImp::GALZStencilStageImp(GALDeviceImp* device, HAL* driver) :
    _device(device), _driver(driver), _syncRequired(true),
    _zStencilBufferDefined(true),
    _zEnabled(false),
    _stencilEnabled(false),
    _zFunc(GAL_COMPARE_FUNCTION_LESS),
    _zMask(true),
    _depthRangeNear(0.0),
    _depthRangeFar(1.0),
    _d3d9DepthRange(false),
    _front(GAL_STENCIL_OP_KEEP, 
           GAL_STENCIL_OP_KEEP, 
           GAL_STENCIL_OP_KEEP, 
           GAL_COMPARE_FUNCTION_ALWAYS,
           0,
           0),
    _back( GAL_STENCIL_OP_KEEP,
           GAL_STENCIL_OP_KEEP,
           GAL_STENCIL_OP_KEEP,
           GAL_COMPARE_FUNCTION_ALWAYS,
           0,
           0),
    _depthSlopeFactor(0),
    _depthUnitOffset(0),
    _stencilUpdateMask(0)
{
    forceSync();
}


void GALZStencilStageImp::setZEnabled(gal_bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _zEnabled = enable;
    GLOBAL_PROFILER_EXIT_REGION()
}

gal_bool GALZStencilStageImp::isZEnabled() const
{
    return _zEnabled;
}

void GALZStencilStageImp::setZFunc(GAL_COMPARE_FUNCTION zFunc)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _zFunc = zFunc;
    GLOBAL_PROFILER_EXIT_REGION()
}

GAL_COMPARE_FUNCTION GALZStencilStageImp::getZFunc() const
{
    return _zFunc;
}

void GALZStencilStageImp::setZMask(gal_bool mask)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _zMask = mask;
    GLOBAL_PROFILER_EXIT_REGION()
}

gal_bool GALZStencilStageImp::getZMask() const
{
    return _zMask;
}

void GALZStencilStageImp::setStencilEnabled(gal_bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _stencilEnabled = enable;
    GLOBAL_PROFILER_EXIT_REGION()
}

gal_bool GALZStencilStageImp::isStencilEnabled() const
{
    return _stencilEnabled;
}

void GALZStencilStageImp::setStencilOp( GAL_FACE face, 
                                        GAL_STENCIL_OP onStencilFail,
                                        GAL_STENCIL_OP onStencilPassZFails,
                                        GAL_STENCIL_OP onStencilPassZPass)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT
    (
        if ( face != GAL_FACE_FRONT && face != GAL_FACE_BACK && face != GAL_FACE_FRONT_AND_BACK )
            CG_ASSERT("Only FRONT, BACK or FRONT_AND_BACK faces accepted");
    )

    if ( face == GAL_FACE_FRONT || face == GAL_FACE_FRONT_AND_BACK ) {
        _front.onStencilFail = onStencilFail;
        _front.onStencilPassZFails = onStencilPassZFails;
        _front.onStencilPassZPass = onStencilPassZPass;
    }
    if ( face == GAL_FACE_BACK || face == GAL_FACE_FRONT_AND_BACK ) {
        _back.onStencilFail = onStencilFail;
        _back.onStencilPassZFails = onStencilPassZFails;
        _back.onStencilPassZPass = onStencilPassZPass;
    }
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::getStencilOp( GAL_FACE face,
                                        GAL_STENCIL_OP& onStencilFail,
                                        GAL_STENCIL_OP& onStencilPassZFail,
                                        GAL_STENCIL_OP& onStencilPassZPass) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT
    (
        if ( face != GAL_FACE_FRONT && face != GAL_FACE_BACK )
            CG_ASSERT("Only FRONT or BACK faces accepted");
    )
    
    const _FaceInfo& faceInfo = ( face == GAL_FACE_FRONT ? _front : _back );

    onStencilFail = faceInfo.onStencilFail;
    onStencilPassZFail = faceInfo.onStencilPassZFails;
    onStencilPassZPass = faceInfo.onStencilPassZPass;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setStencilFunc( GAL_FACE face, GAL_COMPARE_FUNCTION func, gal_uint stencilRef, gal_uint stencilMask ) 
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GAL_ASSERT
    (
        if ( face != GAL_FACE_FRONT && face != GAL_FACE_BACK && face != GAL_FACE_FRONT_AND_BACK ) 
            CG_ASSERT("Only FRONT, BACK or FRONT_AND_BACK faces accepted");
    )

    if ( face == GAL_FACE_FRONT || face == GAL_FACE_FRONT_AND_BACK ) {
        _front.stencilFunc = func;
        _front.stencilRef = stencilRef;
        _front.stencilMask = stencilMask;
    }
    else if ( face == GAL_FACE_BACK || face == GAL_FACE_FRONT_AND_BACK ) {
        _back.stencilFunc = func;
        _back.stencilRef = stencilRef;
        _back.stencilMask = stencilMask;
    }
    else
        CG_ASSERT("Stencil Function not defined neither front nor back");
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setDepthRange (gal_float near, gal_float far)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _depthRangeNear = near;
    _depthRangeFar = far;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setD3D9DepthRangeMode(gal_bool mode)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _d3d9DepthRange = mode;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setPolygonOffset (gal_float factor, gal_float units)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _depthSlopeFactor = factor;
    _depthUnitOffset = units;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setStencilUpdateMask (gal_uint mask)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _stencilUpdateMask = mask;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::setZStencilBufferDefined(gal_bool present)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    _zStencilBufferDefined = present;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALZStencilStageImp::sync()
{
    cg1gpu::GPURegData data;

    if ( _zEnabled.changed() || _zStencilBufferDefined.changed() || _syncRequired ) {
        data.booleanVal = _zEnabled && _zStencilBufferDefined;
        _driver->writeGPURegister(cg1gpu::GPU_DEPTH_TEST, data);
        _zEnabled.restart();
        _zStencilBufferDefined.restart();
    }

    if ( (_zEnabled && _zFunc.changed()) || _syncRequired ) { // only updates if _zEnabled == true
        _getGPUCompareFunction(_zFunc, &data);
        _driver->writeGPURegister(cg1gpu::GPU_DEPTH_FUNCTION, data);
        _zFunc.restart();
    }

    if ( (_zEnabled && _zMask.changed()) || _syncRequired ) {
        data.booleanVal = _zMask;
        _driver->writeGPURegister(cg1gpu::GPU_DEPTH_MASK, data);
        _zMask.restart();
    }

    if ( _stencilEnabled.changed() || _zStencilBufferDefined.changed() || _syncRequired ) {
        data.booleanVal = _stencilEnabled && _zStencilBufferDefined;
        _driver->writeGPURegister(cg1gpu::GPU_STENCIL_TEST, data);
        _stencilEnabled.restart();    
        _zStencilBufferDefined.restart();
    }
    
    if ( _stencilEnabled || _syncRequired ) {
        // Updates FRONT_FACE related state
        if ( _front.stencilFunc.changed() || _syncRequired ) {
            _getGPUCompareFunction(_front.stencilFunc, &data);
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_FUNCTION, data);
            _front.stencilFunc.restart();
        }
        if ( _front.stencilRef.changed() || _syncRequired ) {
            data.uintVal = _front.stencilRef;
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_REFERENCE, data);
            _front.stencilRef.restart();
        }
        if ( _front.stencilMask.changed() || _syncRequired ) {
            data.uintVal = _front.stencilMask;
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_COMPARE_MASK, data);
            _front.stencilMask.restart();
        }
        if ( _front.onStencilFail.changed() || _syncRequired ) {
            _getGPUStencilOperation(_front.onStencilFail, &data);
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_FAIL_UPDATE, data);
            _front.onStencilFail.restart();
        }
        if ( _front.onStencilPassZFails.changed() || _syncRequired ) {
            _getGPUStencilOperation(_front.onStencilPassZFails, &data);
            _driver->writeGPURegister(cg1gpu::GPU_DEPTH_FAIL_UPDATE, data);
            _front.onStencilPassZFails.restart();
        }
        if ( _front.onStencilPassZPass.changed() || _syncRequired ) {
            _getGPUStencilOperation(_front.onStencilPassZPass, &data);
            _driver->writeGPURegister(cg1gpu::GPU_DEPTH_PASS_UPDATE, data);
            _front.onStencilPassZPass.restart();
        }
        // Updates BACK_FACE related state
        if ( _back.stencilFunc.changed() || _syncRequired ) {
            _getGPUCompareFunction(_back.stencilFunc, &data);
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_FUNCTION, data);
            _back.stencilFunc.restart();
        }
        if ( _back.stencilRef.changed() || _syncRequired ) {
            data.uintVal = _back.stencilRef;
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_REFERENCE, data);
            _back.stencilRef.restart();
        }
        if ( _back.stencilMask.changed() || _syncRequired ) {
            data.uintVal = _back.stencilMask;
            _driver->writeGPURegister(cg1gpu::GPU_STENCIL_COMPARE_MASK, data);
            _back.stencilMask.restart();
        }
        if ( _back.onStencilFail.changed() || _syncRequired ) {
            _getGPUStencilOperation(_back.onStencilFail, &data);
            // Separate per faces stencil not supported by CG1
            // Remove the comment and fix the register name when the support is available
            //_driver->writeGPURegister(cg1gpu::GPU_STENCIL_FAIL_UPDATE_BACK, data);
            _back.onStencilFail.restart();
        }
        if ( _back.onStencilPassZFails.changed() || _syncRequired ) {
            _getGPUStencilOperation(_back.onStencilPassZFails, &data);
            // Separate per faces stencil not supported by CG1
            // Remove the comment and fix the register name when the support is available
            //_driver->writeGPURegister(cg1gpu::GPU_DEPTH_FAIL_UPDATE_BACK, data);
            _back.onStencilPassZFails.restart();
        }
        if ( _back.onStencilPassZPass.changed() || _syncRequired ) {
            _getGPUStencilOperation(_back.onStencilPassZPass, &data);
            // Separate per faces stencil not supported by CG1
            // Remove the comment and fix the register name when the support is available
            //_driver->writeGPURegister(cg1gpu::GPU_DEPTH_PASS_UPDATE_BACK, data);
            _back.onStencilPassZPass.restart();
        }
    }

    if (_depthRangeNear.changed() || _syncRequired) {
        data.f32Val = _depthRangeNear;
        _driver->writeGPURegister( cg1gpu::GPU_DEPTH_RANGE_NEAR, data );
        _depthRangeNear.restart();
    }

    if (_depthRangeFar.changed() || _syncRequired) {
        data.f32Val = _depthRangeFar;
        _driver->writeGPURegister( cg1gpu::GPU_DEPTH_RANGE_FAR, data );
        _depthRangeFar.restart();
    }
    
    if (_d3d9DepthRange.changed() || _syncRequired)
    {
        data.booleanVal = _d3d9DepthRange;
        _driver->writeGPURegister(cg1gpu::GPU_D3D9_DEPTH_RANGE, data);
        _d3d9DepthRange.restart();
    }

    if (_depthSlopeFactor.changed() || _syncRequired) {
        data.f32Val = _depthSlopeFactor;
        _driver->writeGPURegister( cg1gpu::GPU_DEPTH_SLOPE_FACTOR, data );
        _depthSlopeFactor.restart();
    }

    if (_depthUnitOffset.changed() || _syncRequired) {
        data.f32Val = _depthUnitOffset;
        data.f32Val = 0;
        _driver->writeGPURegister( cg1gpu::GPU_DEPTH_UNIT_OFFSET, data );
        _depthUnitOffset.restart();
    }

    if (_stencilUpdateMask.changed() || _syncRequired) {
        data.uintVal = _stencilUpdateMask;
        _driver->writeGPURegister( cg1gpu::GPU_STENCIL_UPDATE_MASK, data );
        _stencilUpdateMask.restart();
    }

}

void GALZStencilStageImp::forceSync()
{
    _syncRequired = true;
    sync();
    _syncRequired = false;
}

string GALZStencilStageImp::getInternalState() const
{
    stringstream ss;
    
    // Depth state
    ss << stateItemString<gal_bool>(_zEnabled, "Z_ENABLED", false, &boolPrint);
    ss << stateItemString<GAL_COMPARE_FUNCTION>(_zFunc, "Z_FUNCTION", false, &compareFunctionPrint);
    ss << stateItemString<gal_bool>(_zMask, "Z_MASK", false, &boolPrint);

    // Stencil state
    ss << stateItemString<gal_bool>(_stencilEnabled, "STENCIL_ENABLED", false, &boolPrint);
    // Front face stencil state
    ss << stateItemString<GAL_STENCIL_OP>(_front.onStencilFail, "STENCIL_FRONT_ON_STENCIL_FAIL", false, &stencilOperationPrint);
    ss << stateItemString<GAL_STENCIL_OP>(_front.onStencilPassZFails, "STENCIL_FRONT_ON_STENCIL_PASS_Z_FAILS", false, &stencilOperationPrint);
    ss << stateItemString<GAL_STENCIL_OP>(_front.onStencilPassZPass, "STENCIL_FRONT_ON_STENCIL_PASS_Z_PASS", false, &stencilOperationPrint);
    ss << stateItemString<GAL_COMPARE_FUNCTION>(_front.stencilFunc, "STENCIL_FRONT_STENCIL_FUNC", false, &compareFunctionPrint);
    ss << stateItemString(_front.stencilRef, "STENCIL_FRONT_STENCIL_REF", false);
    // Back face stencil state
    ss << stateItemString<GAL_STENCIL_OP>(_back.onStencilFail, "STENCIL_BACK_ON_STENCIL_FAIL", false, &stencilOperationPrint);
    ss << stateItemString<GAL_STENCIL_OP>(_back.onStencilPassZFails, "STENCIL_BACK_ON_STENCIL_PASS_Z_FAILS", false, &stencilOperationPrint);
    ss << stateItemString<GAL_STENCIL_OP>(_back.onStencilPassZPass, "STENCIL_BACK_ON_STENCIL_PASS_Z_PASS", false, &stencilOperationPrint);
    ss << stateItemString<GAL_COMPARE_FUNCTION>(_back.stencilFunc, "STENCIL_BACK_STENCIL_FUNC", false, &compareFunctionPrint);
    ss << stateItemString(_back.stencilRef, "STENCIL_BACK_STENCIL_REF", false);

    return ss.str();
}

void GALZStencilStageImp::_getGPUCompareFunction(GAL_COMPARE_FUNCTION comp, cg1gpu::GPURegData* data)
{
    using namespace cg1gpu;
    switch ( comp ) {
        case GAL_COMPARE_FUNCTION_NEVER:
            data->compare = GPU_NEVER; break;
        case GAL_COMPARE_FUNCTION_LESS:
            data->compare = GPU_LESS; break;
        case GAL_COMPARE_FUNCTION_EQUAL:
            data->compare = GPU_EQUAL; break;
        case GAL_COMPARE_FUNCTION_LESS_EQUAL:
            data->compare = GPU_LEQUAL; break;
        case GAL_COMPARE_FUNCTION_GREATER:
            data->compare = GPU_GREATER; break;
        case GAL_COMPARE_FUNCTION_NOT_EQUAL:
            data->compare = GPU_NOTEQUAL; break;
        case GAL_COMPARE_FUNCTION_GREATER_EQUAL:
            data->compare = GPU_GEQUAL; break;
        case GAL_COMPARE_FUNCTION_ALWAYS:
            data->compare = GPU_ALWAYS; break;
        default:
            CG_ASSERT("Unknown compare function"); 
    }
}

void GALZStencilStageImp::_getGPUStencilOperation(GAL_STENCIL_OP op, cg1gpu::GPURegData* data)
{
    switch ( op ) {
        case GAL_STENCIL_OP_KEEP:
            data->stencilUpdate = cg1gpu::STENCIL_KEEP; break;
        case GAL_STENCIL_OP_ZERO:
            data->stencilUpdate = cg1gpu::STENCIL_ZERO; break;
        case GAL_STENCIL_OP_REPLACE:
            data->stencilUpdate = cg1gpu::STENCIL_REPLACE; break;
        case GAL_STENCIL_OP_INCR_SAT:
            data->stencilUpdate = cg1gpu::STENCIL_INCR; break;            
        case GAL_STENCIL_OP_DECR_SAT:
            data->stencilUpdate = cg1gpu::STENCIL_DECR; break;
        case GAL_STENCIL_OP_INVERT:
            data->stencilUpdate = cg1gpu::STENCIL_INVERT; break;
        case GAL_STENCIL_OP_INCR:
            data->stencilUpdate = cg1gpu::STENCIL_INCR_WRAP; break;
        case GAL_STENCIL_OP_DECR:
            data->stencilUpdate = cg1gpu::STENCIL_DECR_WRAP; break;
        default:
            CG_ASSERT("Unknown stencil operation");
    }
}

const char* GALZStencilStageImp::CompareFunctionPrint::print(const GAL_COMPARE_FUNCTION& comp) const
{
    using namespace cg1gpu;
    switch ( comp ) {
        case GAL_COMPARE_FUNCTION_NEVER:
            return "NEVER";
        case GAL_COMPARE_FUNCTION_LESS:
            return "LESS";
        case GAL_COMPARE_FUNCTION_EQUAL:
            return "EQUAL";
        case GAL_COMPARE_FUNCTION_LESS_EQUAL:
            return "LESS_EQUAL";
        case GAL_COMPARE_FUNCTION_GREATER:
            return "GREATER";
        case GAL_COMPARE_FUNCTION_NOT_EQUAL:
            return "NOT_EQUAL";
        case GAL_COMPARE_FUNCTION_GREATER_EQUAL:
            return "GREATER_EQUAL";
        case GAL_COMPARE_FUNCTION_ALWAYS:
            return "ALWAYS";
        default:
            CG_ASSERT("Unknown compare function"); 
    }
    return 0;
}

const char* GALZStencilStageImp::StencilOperationPrint::print(const GAL_STENCIL_OP& op) const
{
    switch ( op ) {
        case GAL_STENCIL_OP_KEEP:
            return "KEEP";
        case GAL_STENCIL_OP_ZERO:
            return "ZERO";
        case GAL_STENCIL_OP_REPLACE:
            return "REPLACE";
        case GAL_STENCIL_OP_INCR_SAT:
            return "INCR_SAT";
        case GAL_STENCIL_OP_DECR_SAT:
            return "DECR_SAT";
        case GAL_STENCIL_OP_INVERT:
            return "INVERT";
        case GAL_STENCIL_OP_INCR:
            return "INCR";
        case GAL_STENCIL_OP_DECR:
            return "DECR";
        default:
            CG_ASSERT("Unknown stencil operation");
    }
    return 0;
}

const StoredStateItem* GALZStencilStageImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GALStoredStateItem* ret;

    switch(stateId)
    {
        case GAL_ZST_Z_ENABLED:                     ret = new GALSingleBoolStoredStateItem(_zEnabled);                      break;
        case GAL_ZST_Z_FUNC:                        ret = new GALSingleEnumStoredStateItem((gal_enum)_zFunc);               break;
        case GAL_ZST_Z_MASK:                            ret = new GALSingleBoolStoredStateItem(_zMask);                     break;
        case GAL_ZST_STENCIL_ENABLED:               ret = new GALSingleBoolStoredStateItem(_stencilEnabled);                break;
        case GAL_ZST_STENCIL_BUFFER_DEF:            ret = new GALSingleUintStoredStateItem(_zStencilBufferDefined);         break;
        case GAL_ZST_FRONT_COMPARE_FUNC:            ret = new GALSingleEnumStoredStateItem((gal_enum)_front.stencilFunc);   break;
        case GAL_ZST_FRONT_STENCIL_REF_VALUE:       ret = new GALSingleUintStoredStateItem(_front.stencilRef);              break;
        case GAL_ZST_FRONT_STENCIL_COMPARE_MASK:    ret = new GALSingleUintStoredStateItem(_front.stencilMask);             break;
        case GAL_ZST_FRONT_STENCIL_OPS: 
            { 
                GALEnumVector3 v; 
                v[0] = (gal_enum)_front.onStencilFail; 
                v[1] = (gal_enum)_front.onStencilPassZFails; 
                v[2] = (gal_enum)_front.onStencilPassZPass; 
                ret = new GALEnumVector3StoredStateItem(v); 
                break; 
            }
        case GAL_ZST_BACK_COMPARE_FUNC:             ret = new GALSingleEnumStoredStateItem((gal_enum)_back.stencilFunc);    break;
        case GAL_ZST_BACK_STENCIL_REF_VALUE:        ret = new GALSingleUintStoredStateItem((gal_enum)_back.stencilRef);     break;
        case GAL_ZST_BACK_STENCIL_COMPARE_MASK:     ret = new GALSingleUintStoredStateItem((gal_enum)_back.stencilMask);    break;
        case GAL_ZST_BACK_STENCIL_OPS: 
            { 
                GALEnumVector3 v; 
                v[0] = (gal_enum)_back.onStencilFail; 
                v[1] = (gal_enum)_back.onStencilPassZFails; 
                v[2] = (gal_enum)_back.onStencilPassZPass; 
                ret = new GALEnumVector3StoredStateItem(v); 
                break; 
            }

        case GAL_ZST_RANGE_NEAR:            ret = new GALSingleFloatStoredStateItem((gal_enum)_depthRangeNear);     break;
        case GAL_ZST_RANGE_FAR:             ret = new GALSingleFloatStoredStateItem((gal_enum)_depthRangeFar);      break;
        case GAL_ZST_D3D9_DEPTH_RANGE:      ret = new GALSingleBoolStoredStateItem((gal_enum) _d3d9DepthRange);     break;
        case GAL_ZST_SLOPE_FACTOR:          ret = new GALSingleFloatStoredStateItem((gal_enum)_depthSlopeFactor);   break;
        case GAL_ZST_UNIT_OFFSET:           ret = new GALSingleFloatStoredStateItem((gal_enum)_depthUnitOffset);    break;
        case GAL_ZST_DEPTH_UPDATE_MASK:     ret = new GALSingleUintStoredStateItem((gal_enum)_stencilUpdateMask);   break;


        // case GAL_ZST_... <- add here future additional z stencil states.
        default:
            CG_ASSERT("Unknown z stencil state");
            ret = 0;
    }

    ret->setItemId(stateId);

    GLOBAL_PROFILER_EXIT_REGION()

    return ret;
}

#define CAST_TO_BOOL(X)         *(static_cast<const GALSingleBoolStoredStateItem*>(X))
#define COMP_FUNC(DST,X)        { const GALSingleEnumStoredStateItem* aux = static_cast<const GALSingleEnumStoredStateItem*>(X); gal_enum value = *aux; DST = static_cast<GAL_COMPARE_FUNCTION>(value); }
#define STENCIL_OP(DST,X,i)     { const GALEnumVector3StoredStateItem* aux = static_cast<const GALEnumVector3StoredStateItem*>(X); GALEnumVector3 v = (*aux); gal_enum value = v[i]; DST = static_cast<GAL_STENCIL_OP>(value); }
#define CAST_TO_UINT(X)         *(static_cast<const GALSingleUintStoredStateItem*>(X))
#define CAST_TO_FLOAT(X)        *(static_cast<const GALSingleFloatStoredStateItem*>(X))

void GALZStencilStageImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    switch(stateId)
    {
        case GAL_ZST_Z_ENABLED:                     _zEnabled = CAST_TO_BOOL(galssi);               delete(galssi);     break;
        case GAL_ZST_Z_FUNC:                        COMP_FUNC(_zFunc, galssi);                      delete(galssi);     break;
        case GAL_ZST_Z_MASK:                        _zMask = CAST_TO_BOOL(galssi);                  delete(galssi);     break;
        case GAL_ZST_STENCIL_ENABLED:               _stencilEnabled = CAST_TO_BOOL(galssi);         delete(galssi);     break;
        case GAL_ZST_STENCIL_BUFFER_DEF:            _zStencilBufferDefined = CAST_TO_UINT(galssi);  delete(galssi);     break;
        case GAL_ZST_FRONT_STENCIL_REF_VALUE:       _front.stencilRef = CAST_TO_UINT(galssi);       delete(galssi);     break;
        case GAL_ZST_FRONT_COMPARE_FUNC:            COMP_FUNC(_front.stencilFunc, galssi);          delete(galssi);     break;
        case GAL_ZST_FRONT_STENCIL_COMPARE_MASK:    _front.stencilMask = CAST_TO_UINT(galssi);      delete(galssi);     break;
        case GAL_ZST_FRONT_STENCIL_OPS:         
            STENCIL_OP(_front.onStencilFail, galssi, 0); 
            STENCIL_OP(_front.onStencilPassZFails, galssi, 1); 
            STENCIL_OP(_front.onStencilPassZPass, galssi, 2); 
            delete(galssi);
            break;
        case GAL_ZST_BACK_COMPARE_FUNC:             COMP_FUNC(_back.stencilFunc, galssi);           delete(galssi);     break;
        case GAL_ZST_BACK_STENCIL_REF_VALUE:        _back.stencilRef = CAST_TO_UINT(galssi);        delete(galssi);     break;
        case GAL_ZST_BACK_STENCIL_COMPARE_MASK:     _back.stencilMask = CAST_TO_UINT(galssi);       delete(galssi);     break;
        case GAL_ZST_BACK_STENCIL_OPS:          
            STENCIL_OP(_back.onStencilFail, galssi, 0); 
            STENCIL_OP(_back.onStencilPassZFails, galssi, 1); 
            STENCIL_OP(_back.onStencilPassZPass, galssi, 2); 
            delete(galssi);
            break;
        case GAL_ZST_RANGE_NEAR:                    _depthRangeNear = CAST_TO_FLOAT(galssi);        delete(galssi);     break;
        case GAL_ZST_RANGE_FAR:                     _depthRangeFar = CAST_TO_FLOAT(galssi);         delete(galssi);     break;
        case GAL_ZST_D3D9_DEPTH_RANGE:              _d3d9DepthRange = CAST_TO_BOOL(galssi);         delete(galssi);     break;
        case GAL_ZST_SLOPE_FACTOR:                  _depthSlopeFactor = CAST_TO_FLOAT(galssi);      delete(galssi);     break;
        case GAL_ZST_UNIT_OFFSET:                   _depthUnitOffset = CAST_TO_FLOAT(galssi);       delete(galssi);     break;
        case GAL_ZST_DEPTH_UPDATE_MASK:             _stencilUpdateMask = CAST_TO_UINT(galssi);      delete(galssi);     break;


        // case GAL_ZST_... <- add here future additional z stencil states.
        default:
            CG_ASSERT("Unknown z stencil state");
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

#undef CAST_TO_BOOL
#undef COMP_FUNC
#undef STENCIL_OP
#undef CAST_TO_UINT
#undef CAST_TO_FLOAT

GALStoredState* GALZStencilStageImp::saveAllState() const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    
    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i = 0; i < GAL_ZST_LAST; i++)
        ret->addStoredStateItem(createStoredStateItem(GAL_STORED_ITEM_ID(i)));

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

void GALZStencilStageImp::restoreAllState(const GALStoredState* state)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = ssi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();

    while ( iter != ssiList.end() )
    {
        const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(*iter);
        restoreStoredStateItem(galssi);
        iter++;
    }

    GLOBAL_PROFILER_EXIT_REGION()
}
