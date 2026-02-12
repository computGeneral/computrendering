/**************************************************************************
 *
 */

#include "GALBlendingStageImp.h"
#include "GALMacros.h"
#include "support.h"
#include "GALSupport.h"
#include <sstream>

#include "Profiler.h"

using namespace std;
using namespace libGAL;

GALBlendingStageImp::GALBlendingStageImp(GALDeviceImp*, HAL* driver) :
    _driver(driver), _enabled(MAX_RENDER_TARGETS, false), _syncRequired(true),
    _srcBlend(MAX_RENDER_TARGETS, GAL_BLEND_ONE),
    _destBlend(MAX_RENDER_TARGETS, GAL_BLEND_ZERO),
    _srcBlendAlpha(MAX_RENDER_TARGETS, GAL_BLEND_ONE),
    _destBlendAlpha(MAX_RENDER_TARGETS, GAL_BLEND_ZERO),
    _blendFunc(MAX_RENDER_TARGETS, GAL_BLEND_ADD),
    _blendFuncAlpha(MAX_RENDER_TARGETS, GAL_BLEND_ADD),
    _blendColor(MAX_RENDER_TARGETS, GALFloatVector4(gal_float(0.0f))),
    _redMask(MAX_RENDER_TARGETS, true),
    _greenMask(MAX_RENDER_TARGETS, true),
    _blueMask(MAX_RENDER_TARGETS, true),
    _alphaMask(MAX_RENDER_TARGETS, true),
    _colorWriteEnabled(MAX_RENDER_TARGETS, false)
{
    //  The backbuffer (render target 0) is enabled by default.
    _colorWriteEnabled[0] = true;
    
    forceSync();
}

void GALBlendingStageImp::setEnable(gal_uint renderTargetID, gal_bool enable)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _enabled[renderTargetID] = enable;
    TRACING_EXIT_REGION()
    
}

void GALBlendingStageImp::setSrcBlend(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlend)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    // Update object state
    _srcBlend[renderTargetID] = srcBlend;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setDestBlend(gal_uint renderTargetID, GAL_BLEND_OPTION destBlend)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    // Update object state
    _destBlend[renderTargetID] = destBlend;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setSrcBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlendAlpha)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    // update object state
    _srcBlendAlpha[renderTargetID] = srcBlendAlpha;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setDestBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION destBlendAlpha)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    // update object state
    _destBlendAlpha[renderTargetID] = destBlendAlpha;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setBlendFunc(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendFunc)
{
    TRACING_ENTER_REGION("GAL", "", "")
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _blendFunc[renderTargetID] = blendFunc;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setBlendFuncAlpha(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendFuncAlpha)
{
    CG_ASSERT(
          "Separate ALPHA blending (function/equation) is not supported yet");

    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _blendFuncAlpha[renderTargetID] = blendFuncAlpha;
}

void GALBlendingStageImp::setBlendColor(gal_uint renderTargetID, gal_float red, gal_float green, gal_float blue, gal_float alpha)
{
    TRACING_ENTER_REGION("GAL", "", "")

    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    GALFloatVector4 color;
    color[0] = galsupport::clamp(red); 
    color[1] = galsupport::clamp(green); 
    color[2] = galsupport::clamp(blue);
    color[3] = galsupport::clamp(alpha);

    _blendColor[renderTargetID] = color;
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setBlendColor(gal_uint renderTargetID, const gal_float* rgba)
{
    TRACING_ENTER_REGION("GAL", "", "")
    setBlendColor(renderTargetID, rgba[0], rgba[1], rgba[2], rgba[3]);
    TRACING_EXIT_REGION()
}

void GALBlendingStageImp::setColorMask(gal_uint renderTargetID, gal_bool red, gal_bool green, gal_bool blue, gal_bool alpha)
{
    TRACING_ENTER_REGION("GAL", "", "")

    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _redMask[renderTargetID] = red;
    _greenMask[renderTargetID] = green;
    _blueMask[renderTargetID] = blue;
    _alphaMask[renderTargetID] = alpha;
    TRACING_EXIT_REGION()
}

gal_bool GALBlendingStageImp::getColorEnable(gal_uint renderTargetID)
{
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    return (_redMask[renderTargetID] || _greenMask[renderTargetID] || _blueMask[renderTargetID] || _alphaMask[renderTargetID]) &&
           _colorWriteEnabled[renderTargetID];
}

void GALBlendingStageImp::enableColorWrite(gal_uint renderTargetID)
{
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _colorWriteEnabled[renderTargetID] = true;
}

void GALBlendingStageImp::disableColorWrite(gal_uint renderTargetID)
{
    GAL_ASSERT(
        if ( renderTargetID >= MAX_RENDER_TARGETS )
            CG_ASSERT("Render target value greater than the maximum allowed id");
    )

    _colorWriteEnabled[renderTargetID] = false;
}

void GALBlendingStageImp::forceSync()
{
    _syncRequired = true;
    sync();
    _syncRequired = false;
}

void GALBlendingStageImp::sync()
{
    arch::GPURegData data;

    for ( gal_uint rt = 0; rt < MAX_RENDER_TARGETS; ++rt )
    {
        if ( _enabled[rt].changed() || _syncRequired )
        {
            data.booleanVal = _enabled[rt];
            _driver->writeGPURegister(arch::GPU_COLOR_BLEND, rt, data);
            _enabled[rt].restart();
        }
        
        if ( _blendFunc[rt].changed() || _syncRequired ) {
            _getGPUBlendFunction(_blendFunc[rt], &data);
            _driver->writeGPURegister(arch::GPU_BLEND_EQUATION, rt, data);
            _blendFunc[rt].restart();
        }

        if ( _blendFuncAlpha[rt].changed() || _syncRequired ) {
            _getGPUBlendFunction(_blendFuncAlpha[rt], &data);
            // Remove this comment and fix register name to support independent ALPHA BLEND equation/function 
            // _driver->writeGPURegister(arch::GPU_BLEND_ALPHA_EQUATION, rt, data);
            _blendFuncAlpha[rt].restart();
        }

        if ( _srcBlend[rt].changed() || _syncRequired ) {
            _getGPUBlendOption(_srcBlend[rt], &data);
            _driver->writeGPURegister(arch::GPU_BLEND_SRC_RGB, rt, data);
            _srcBlend[rt].restart();
        }

        if ( _destBlend[rt].changed() || _syncRequired ) {
            _getGPUBlendOption(_destBlend[rt], &data);
            _driver->writeGPURegister(arch::GPU_BLEND_DST_RGB, rt, data);
            _destBlend[rt].restart();
        }

        if ( _srcBlendAlpha[rt].changed() || _syncRequired ) {
            _getGPUBlendOption(_srcBlendAlpha[rt], &data);
            _driver->writeGPURegister(arch::GPU_BLEND_SRC_ALPHA, rt, data);
            _srcBlendAlpha[rt].restart();
        }

        if ( _destBlendAlpha[rt].changed() || _syncRequired ) {
            _getGPUBlendOption(_destBlendAlpha[rt], &data);
            _driver->writeGPURegister(arch::GPU_BLEND_DST_ALPHA, rt, data);
            _destBlendAlpha[rt].restart();
        }
        
        if ( _blendColor[rt].changed() || _syncRequired ) {
            const GALFloatVector4& blendColor = _blendColor[rt]; // Perform a explicit cast using a const reference
            data.qfVal[0] = blendColor[0];
            data.qfVal[1] = blendColor[1];
            data.qfVal[2] = blendColor[2];
            data.qfVal[3] = blendColor[3];
            _driver->writeGPURegister(arch::GPU_BLEND_COLOR, rt, data);
            _blendColor[rt].restart();
        }

        if ( _redMask[rt].changed() || _colorWriteEnabled[rt].changed() || _syncRequired )
        {
            //data.booleanVal = (_redMask != 0 ? true : false);
            data.booleanVal = _redMask[rt] && _colorWriteEnabled[rt];
            _driver->writeGPURegister(arch::GPU_COLOR_MASK_R, rt, data);  //blending
            _redMask[rt].restart();
        }

        if ( _greenMask[rt].changed() || _colorWriteEnabled[rt].changed() || _syncRequired )
        {
            //data.booleanVal = (_greenMask != 0 ? true : false);
            data.booleanVal = _greenMask[rt] && _colorWriteEnabled[rt];
            _driver->writeGPURegister(arch::GPU_COLOR_MASK_G, rt, data);
            _greenMask[rt].restart();
        }

        if ( _blueMask[rt].changed() || _colorWriteEnabled[rt].changed() || _syncRequired )
        {
            //data.booleanVal = (_blueMask != 0 ? true : false);
            data.booleanVal = _blueMask[rt] && _colorWriteEnabled[rt];
            _driver->writeGPURegister(arch::GPU_COLOR_MASK_B, rt, data);
            _blueMask[rt].restart();
        }

        if ( _alphaMask[rt].changed() || _colorWriteEnabled[rt].changed() || _syncRequired )
        {
            //data.booleanVal = (_alphaMask != 0 ? true : false);
            data.booleanVal = _alphaMask[rt] && _colorWriteEnabled[rt];
            _driver->writeGPURegister(arch::GPU_COLOR_MASK_A, rt, data);
            _alphaMask[rt].restart();
        }
    }
}

void GALBlendingStageImp::_getGPUBlendOption(GAL_BLEND_OPTION option, arch::GPURegData* data)
{
    // Note:
    // BLEND_FUNCTION in CG1 GPU is equivalent to BLEND_OPTION in CG1 GAL
    // BLEND_EQUATION in CG1 GPU is equivalent to BLEND_FUNCTION in CG1 GAL

    using namespace arch;
    switch ( option )
    {
        case GAL_BLEND_ZERO:
            data->blendFunction = BLEND_ZERO;
            break;
        case GAL_BLEND_ONE:
            data->blendFunction = BLEND_ONE;
            break;
        case GAL_BLEND_SRC_COLOR:
            data->blendFunction = BLEND_SRC_COLOR;
            break;
        case GAL_BLEND_INV_SRC_COLOR:
            data->blendFunction = BLEND_ONE_MINUS_SRC_COLOR;
            break;
        case GAL_BLEND_DEST_COLOR:
            data->blendFunction = BLEND_DST_COLOR;
            break;
        case GAL_BLEND_INV_DEST_COLOR:
            data->blendFunction = BLEND_ONE_MINUS_DST_COLOR;
            break;
        case GAL_BLEND_SRC_ALPHA:
            data->blendFunction = BLEND_SRC_ALPHA;
            break;
        case GAL_BLEND_INV_SRC_ALPHA:
            data->blendFunction = BLEND_ONE_MINUS_SRC_ALPHA;
            break;
        case GAL_BLEND_DEST_ALPHA:
            data->blendFunction = BLEND_DST_ALPHA;
            break;
        case GAL_BLEND_INV_DEST_ALPHA:
            data->blendFunction = BLEND_ONE_MINUS_DST_ALPHA;
            break;
        case GAL_BLEND_CONSTANT_COLOR:
            data->blendFunction = BLEND_CONSTANT_COLOR;
            break;
        case GAL_BLEND_INV_CONSTANT_COLOR:
            data->blendFunction = BLEND_ONE_MINUS_CONSTANT_COLOR;
            break;
        case GAL_BLEND_CONSTANT_ALPHA:
            data->blendFunction = BLEND_CONSTANT_ALPHA;
            break;
        case GAL_BLEND_INV_CONSTANT_ALPHA:
            data->blendFunction = BLEND_ONE_MINUS_CONSTANT_ALPHA;
            break;
        case GAL_BLEND_SRC_ALPHA_SAT:
            data->blendFunction = BLEND_SRC_ALPHA_SATURATE;
            break;
        case GAL_BLEND_BLEND_FACTOR:
            CG_ASSERT("GAL_BLEND_BLEND_FACTOR not supported");
            break;
        case GAL_BLEND_INV_BLEND_FACTOR:
            CG_ASSERT("GAL_BLEND_INV_BLEND_FACTOR not supported");
            break;
        default:
            CG_ASSERT("Unexpected blending function");
    }
}

void GALBlendingStageImp::_getGPUBlendFunction(GAL_BLEND_FUNCTION func, arch::GPURegData* eq)
{
    // Note:
    // BLEND_FUNCTION in CG1 GPU is equivalent to BLEND_OPTION in CG1 GAL
    // BLEND_EQUATION in CG1 GPU is equivalent to BLEND_FUNCTION in CG1 GAL

    using namespace arch;
    
    switch ( func )
    {
        case GAL_BLEND_ADD:
            eq->blendEquation = BLEND_FUNC_ADD;
            break;
        case GAL_BLEND_SUBTRACT:
            eq->blendEquation = BLEND_FUNC_SUBTRACT;
            break;
        case GAL_BLEND_REVERSE_SUBTRACT:
            eq->blendEquation = BLEND_FUNC_REVERSE_SUBTRACT;
            break;
        case GAL_BLEND_MIN:
            eq->blendEquation = BLEND_MIN;
            break;
        case GAL_BLEND_MAX:
            eq->blendEquation = BLEND_MAX;
            break;
        default:
            CG_ASSERT("Unexpected blend mode equation");
    }
}

const char* GALBlendingStageImp::BlendOptionPrint::print(const GAL_BLEND_OPTION& option) const
{
    using namespace arch;
    switch ( option )
    {
        case GAL_BLEND_ZERO:
            return "ZERO";
        case GAL_BLEND_ONE:
            return "ONE";
        case GAL_BLEND_SRC_COLOR:
            return "SRC_COLOR";
        case GAL_BLEND_INV_SRC_COLOR:
            return "INC_SRC_COLOR";
        case GAL_BLEND_DEST_COLOR:
            return "DEST_COLOR";
        case GAL_BLEND_INV_DEST_COLOR:
            return "INV_DEST_COLOR";
        case GAL_BLEND_SRC_ALPHA:
            return "SRC_ALPHA";
        case GAL_BLEND_INV_SRC_ALPHA:
            return "INV_SRC_ALPHA";
        case GAL_BLEND_DEST_ALPHA:
            return "DEST_ALPHA";
        case GAL_BLEND_INV_DEST_ALPHA:
            return "INV_DEST_ALPHA";
        case GAL_BLEND_CONSTANT_COLOR:
            return "CONSTANT_COLOR";
        case GAL_BLEND_INV_CONSTANT_COLOR:
            return "INV_CONSTANT_COLOR";
        case GAL_BLEND_CONSTANT_ALPHA:
            return "CONSTANT_ALPHA";
        case GAL_BLEND_INV_CONSTANT_ALPHA:
            return "INV_CONSTANT_ALPHA";
        case GAL_BLEND_SRC_ALPHA_SAT:
            return "SRC_ALPHA_SAT";
        case GAL_BLEND_BLEND_FACTOR:
            return "BLEND_FACTOR";
        case GAL_BLEND_INV_BLEND_FACTOR:
            return "INV_BLEND_FACTOR";
        default:
            CG_ASSERT("Unexpected blending function");
    }
    return 0;
}

const char* GALBlendingStageImp::BlendFunctionPrint::print(const GAL_BLEND_FUNCTION& func) const
{
    using namespace arch;
    
    switch ( func )
    {
        case GAL_BLEND_ADD:
            return "ADD";
        case GAL_BLEND_SUBTRACT:
            return "SUBTRACT";
        case GAL_BLEND_REVERSE_SUBTRACT:
            return "REVERSE_SUBTRACT";
        case GAL_BLEND_MIN:
            return "MIN";
        case GAL_BLEND_MAX:
            return "MAX";
        default:
            CG_ASSERT("Unexpected blend mode equation");
    }
    return 0;
}

string GALBlendingStageImp::getInternalState() const
{
    stringstream ss, ssAux;

    ssAux.str("");

    for ( gal_uint i = 0; i < MAX_RENDER_TARGETS; ++i )
    {
        ssAux << "RENDER_TARGET" << i << "_ENABLED"; ss << stateItemString<gal_bool>(_enabled[i], ssAux.str().c_str(), false, &boolPrint); ssAux.str("");
        ss << stateItemString<GAL_BLEND_FUNCTION>(_blendFunc[i], "BLEND_FUNCTION_RGB", false, &blendFunctionPrint); 
        ss << stateItemString<GAL_BLEND_OPTION>(_srcBlend[i], "BLEND_SRC_RGB", false, &blendOptionPrint);
        ss << stateItemString<GAL_BLEND_OPTION>(_destBlend[i], "BLEND_DST_RGB", false, &blendOptionPrint);

        ss << stateItemString<GAL_BLEND_FUNCTION>(_blendFuncAlpha[i], "BLEND_FUNCTION_ALPHA", false, &blendFunctionPrint);
        ss << stateItemString<GAL_BLEND_OPTION>(_srcBlendAlpha[i], "BLEND_SRC_ALPHA", false, &blendOptionPrint);
        ss << stateItemString<GAL_BLEND_OPTION>(_destBlendAlpha[i], "BLEND_DST_ALPHA", false, &blendOptionPrint);

        const GALFloatVector4& color = _blendColor[i];
        ss << stateItemString<GALFloatVector4>(color, "BLEND_COLOR", false);
    }


    return ss.str();

}

const StoredStateItem* GALBlendingStageImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    TRACING_ENTER_REGION("GAL", "", "")
    GALStoredStateItem* ret;
    gal_uint rTarget;

    if (stateId >= GAL_BLENDING_FIRST_ID && stateId < GAL_BLENDING_LAST)
    {    
        // It�s a blending state
        
        if ((stateId >= GAL_BLENDING_ENABLED) && (stateId < GAL_BLENDING_ENABLED + GAL_MAX_RENDER_TARGETS))
        {    
            // It�s a blending enabled state
            rTarget = stateId - GAL_BLENDING_ENABLED;
            ret = new GALSingleBoolStoredStateItem(_enabled[rTarget]);
        }
        else 
        {
            switch(stateId)
            {
                case GAL_BLENDING_SRC_RGB:              ret = new GALSingleEnumStoredStateItem((gal_enum)_srcBlend[0]);            break;
                case GAL_BLENDING_DST_RGB:              ret = new GALSingleEnumStoredStateItem((gal_enum)_destBlend[0]);           break;
                case GAL_BLENDING_FUNC_RGB:             ret = new GALSingleEnumStoredStateItem((gal_enum)_blendFunc[0]);           break;
                case GAL_BLENDING_SRC_ALPHA:            ret = new GALSingleEnumStoredStateItem((gal_enum)_srcBlendAlpha[0]);       break;
                case GAL_BLENDING_DST_ALPHA:            ret = new GALSingleEnumStoredStateItem((gal_enum)_destBlendAlpha[0]);      break;
                case GAL_BLENDING_FUNC_ALPHA:           ret = new GALSingleEnumStoredStateItem((gal_enum)_blendFuncAlpha[0]);      break;
                case GAL_BLENDING_COLOR:                ret = new GALFloatVector4StoredStateItem(_blendColor[0]);                  break;
                case GAL_BLENDING_COLOR_WRITE_ENABLED:  ret = new GALSingleBoolStoredStateItem((gal_enum)_colorWriteEnabled[0]);   break;
                case GAL_BLENDING_COLOR_MASK_R:         ret = new GALSingleBoolStoredStateItem((gal_enum)_redMask[0]);             break;
                case GAL_BLENDING_COLOR_MASK_G:         ret = new GALSingleBoolStoredStateItem((gal_enum)_greenMask[0]);           break;
                case GAL_BLENDING_COLOR_MASK_B:         ret = new GALSingleBoolStoredStateItem((gal_enum)_blueMask[0]);            break;
                case GAL_BLENDING_COLOR_MASK_A:         ret = new GALSingleBoolStoredStateItem((gal_enum)_alphaMask[0]);           break;
                // case GALx_BLENDING_... <- add here future additional blending states.

                default:
                    CG_ASSERT("Unexpected blending state id");
            }
        }            
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    ret->setItemId(stateId);

    TRACING_EXIT_REGION()

    return ret;
}

#define CAST_TO_BOOL(X)             *(static_cast<const GALSingleBoolStoredStateItem*>(X))
#define BLEND_FUNCTION(DST,X)       { const GALSingleEnumStoredStateItem* aux = static_cast<const GALSingleEnumStoredStateItem*>(X); gal_enum value = *aux; DST = static_cast<GAL_BLEND_FUNCTION>(value); }
#define BLEND_OPTION(DST,X)         { const GALSingleEnumStoredStateItem* aux = static_cast<const GALSingleEnumStoredStateItem*>(X); gal_enum value = *aux; DST = static_cast<GAL_BLEND_OPTION>(value); }
#define CAST_TO_VECT4(X)            *(static_cast<const GALFloatVector4StoredStateItem*>(X))

void GALBlendingStageImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    TRACING_ENTER_REGION("GAL", "", "")
    gal_uint rTarget;

    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    if (stateId >= GAL_BLENDING_FIRST_ID && stateId < GAL_BLENDING_LAST)
    {    
        // It�s a blending state
        
        if ((stateId >= GAL_BLENDING_ENABLED) && (stateId < GAL_BLENDING_ENABLED + GAL_MAX_RENDER_TARGETS))
        {    
            // It�s a blending enabled state
            rTarget = stateId - GAL_BLENDING_ENABLED;
            _enabled[rTarget] = CAST_TO_BOOL(galssi);
            delete(galssi);
        }
        else 
        {
            switch(stateId)
            {
                case GAL_BLENDING_SRC_RGB:              BLEND_OPTION(_srcBlend[0], galssi);            delete(galssi);     break;
                case GAL_BLENDING_DST_RGB:              BLEND_OPTION(_destBlend[0], galssi);           delete(galssi);     break;
                case GAL_BLENDING_FUNC_RGB:             BLEND_FUNCTION(_blendFunc[0], galssi);         delete(galssi);     break;
                case GAL_BLENDING_SRC_ALPHA:            BLEND_OPTION(_srcBlendAlpha[0], galssi);       delete(galssi);     break;
                case GAL_BLENDING_DST_ALPHA:            BLEND_OPTION(_destBlendAlpha[0], galssi);      delete(galssi);     break;
                case GAL_BLENDING_FUNC_ALPHA:           BLEND_FUNCTION(_blendFuncAlpha[0], galssi);    delete(galssi);     break;
                case GAL_BLENDING_COLOR_WRITE_ENABLED:  _colorWriteEnabled[0] = CAST_TO_BOOL(galssi);  delete(galssi);     break;
                case GAL_BLENDING_COLOR:                _blendColor[0] = CAST_TO_VECT4(galssi);        delete(galssi);     break;
                case GAL_BLENDING_COLOR_MASK_R:         _redMask[0] = CAST_TO_BOOL(galssi);            delete(galssi);     break;
                case GAL_BLENDING_COLOR_MASK_G:         _greenMask[0] = CAST_TO_BOOL(galssi);          delete(galssi);     break;
                case GAL_BLENDING_COLOR_MASK_B:         _blueMask[0] = CAST_TO_BOOL(galssi);           delete(galssi);     break;
                case GAL_BLENDING_COLOR_MASK_A:         _alphaMask[0] = CAST_TO_BOOL(galssi);          delete(galssi);     break;

                // case GALx_BLENDING_... <- add here future additional blending states.
                default:
                    CG_ASSERT("Unexpected blending state id");
            }
        }            
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    TRACING_EXIT_REGION()
}

#undef CAST_TO_BOOL             
#undef BLEND_FUNCTION
#undef BLEND_OPTION
#undef CAST_TO_VECT4

GALStoredState* GALBlendingStageImp::saveAllState() const
{
    TRACING_ENTER_REGION("GAL", "", "")    
    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i = 0; i < GAL_BLENDING_LAST; i++)
         ret->addStoredStateItem(createStoredStateItem(GAL_STORED_ITEM_ID(i)));

    TRACING_EXIT_REGION()
    
    return ret;
}

void GALBlendingStageImp::restoreAllState(const GALStoredState* state)
{
    TRACING_ENTER_REGION("GAL", "", "")    

    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = ssi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();

    while ( iter != ssiList.end() )
    {
        const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(*iter);
        restoreStoredStateItem(galssi);
        iter++;
    }

    delete(state);

    TRACING_EXIT_REGION()
}
