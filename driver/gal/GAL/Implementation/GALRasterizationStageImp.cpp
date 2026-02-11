/**************************************************************************
 *
 */

#include "GALRasterizationStageImp.h"
#include <iostream>
#include <sstream>

#include "Profiler.h"

#include "GALStoredStateImp.h"

using namespace libGAL;
using namespace std;

GALRasterizationStageImp::GALRasterizationStageImp(GALDeviceImp* device, HAL* driver) : 
    _device(device), _driver(driver), _syncRequired(true),
    _fillMode(GAL_FILL_SOLID),
    _cullMode(GAL_CULL_NONE),
    _faceMode(GAL_FACE_CCW),
    _xViewport(0),
    _yViewport(0),
    _widthViewport(0),
    _heightViewport(0),
    _interpolation(cg1gpu::MAX_FRAGMENT_ATTRIBUTES, GAL_INTERPOLATION_LINEAR),
    _scissorEnabled(false),
    _xScissor(0),
    _yScissor(0),
    _widthScissor(400),
    _heightScissor(400),
    _useD3D9RasterizationRules(false),
    _useD3D9PixelCoordConvention(false)
{
    forceSync();
}

void GALRasterizationStageImp::setInterpolationMode(gal_uint fshInputAttribute, GAL_INTERPOLATION_MODE mode)
{
    TRACING_ENTER_REGION("GAL", "", "")
    if ( fshInputAttribute >= _interpolation.size() )
        CG_ASSERT("Fragment shader input attribute out of range");

    _interpolation[fshInputAttribute] = mode;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::enableScissor(gal_bool enable)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _scissorEnabled = enable;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::setFillMode(GAL_FILL_MODE fillMode)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _fillMode = fillMode;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::setCullMode(GAL_CULL_MODE cullMode)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _cullMode = cullMode;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::setFaceMode(GAL_FACE_MODE faceMode)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _faceMode = faceMode;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::useD3D9RasterizationRules(gal_bool use)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _useD3D9RasterizationRules = use;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::useD3D9PixelCoordConvention(gal_bool use)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _useD3D9PixelCoordConvention = use;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::setViewport(gal_int x, gal_int y, gal_uint width, gal_uint height)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _xViewport = x;
    _yViewport = y;
    _widthViewport = width;
    _heightViewport = height;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::setScissor(gal_int x, gal_int y, gal_uint width, gal_uint height)
{
    TRACING_ENTER_REGION("GAL", "", "")
    _xScissor = x;
    _yScissor= y;
    _widthScissor = width;
    _heightScissor = height;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::getViewport(gal_int &x, gal_int &y, gal_uint &width, gal_uint &height)
{
    TRACING_ENTER_REGION("GAL", "", "")
    x = _xViewport;
    y = _yViewport;
    width = _widthViewport;
    height = _heightViewport;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::getScissor(gal_bool &enabled, gal_int &x, gal_int &y, gal_uint &width, gal_uint &height)
{
    TRACING_ENTER_REGION("GAL", "", "")
    enabled = _scissorEnabled;
    x = _xScissor;
    y = _yScissor;
    width = _widthScissor;
    height = _heightScissor;
    TRACING_EXIT_REGION()
}

void GALRasterizationStageImp::sync()
{
    cg1gpu::GPURegData data;
    
    // Fill mode not sync. CG1 GPU doesn�t support wireframe mode
    if ( _fillMode.changed() || _syncRequired ) {
        if (_fillMode == GAL_FILL_WIREFRAME)
            CG_ASSERT("Wireframe mode not supported natively");

        _fillMode.restart();
    }
    if ( _cullMode.changed() || _syncRequired ) {
        _getGPUCullMode(_cullMode, &data);
        _driver->writeGPURegister(cg1gpu::GPU_CULLING, data);
        _cullMode.restart();
    }
    if ( _faceMode.changed() || _syncRequired ) {
        _getGPUFaceMode(_faceMode, &data);
        _driver->writeGPURegister(cg1gpu::GPU_FACEMODE, data);
        _faceMode.restart();
    }
    if (_useD3D9RasterizationRules.changed() || _syncRequired)
    {
        data.booleanVal = _useD3D9RasterizationRules;
        _driver->writeGPURegister(cg1gpu::GPU_D3D9_RASTERIZATION_RULES, data);
        _useD3D9RasterizationRules.restart();
    }    
    if (_useD3D9PixelCoordConvention.changed() || _syncRequired)
    {
        data.booleanVal = _useD3D9PixelCoordConvention;
        _driver->writeGPURegister(cg1gpu::GPU_D3D9_PIXEL_COORDINATES, data);
        _useD3D9PixelCoordConvention.restart();
    }    
    if ( _xViewport.changed() || _syncRequired ) {
        data.intVal = _xViewport;
        _driver->writeGPURegister(cg1gpu::GPU_VIEWPORT_INI_X, data);
        _xViewport.restart();
    }
    if ( _yViewport.changed() || _syncRequired ) {
        data.intVal = _yViewport;
        _driver->writeGPURegister(cg1gpu::GPU_VIEWPORT_INI_Y, data);
        _yViewport.restart();
    }
    if ( _widthViewport.changed() || _syncRequired ) {
        data.uintVal = _widthViewport;
        _driver->writeGPURegister(cg1gpu::GPU_VIEWPORT_WIDTH, data);
        _widthViewport.restart();
    }
    if ( _heightViewport.changed() || _syncRequired ) {
        data.uintVal = _heightViewport;
        _driver->writeGPURegister(cg1gpu::GPU_VIEWPORT_HEIGHT, data);
        _heightViewport.restart();
    }

    const gal_uint iCount = _interpolation.size();
    for ( gal_uint i = 0; i < iCount; ++i ) {
        if ( _interpolation[i].changed() || _syncRequired ) {
            _getGPUInterpolationMode(_interpolation[i], &data);
            _driver->writeGPURegister(cg1gpu::GPU_INTERPOLATION, i, data);
            _interpolation[i].restart();
        }
    }

    if ( _scissorEnabled.changed() || _syncRequired ) {
        data.booleanVal = _scissorEnabled;
        _driver->writeGPURegister( cg1gpu::GPU_SCISSOR_TEST, data );
        _scissorEnabled.restart();
    }

    if ( _xScissor.changed() || _syncRequired ) {
        data.intVal = _xScissor;
        _driver->writeGPURegister( cg1gpu::GPU_SCISSOR_INI_X, data );
        _xScissor.restart();
    }

    if ( _yScissor.changed() || _syncRequired ) {
        data.intVal = _yScissor;
        _driver->writeGPURegister( cg1gpu::GPU_SCISSOR_INI_Y, data );
        _yScissor.restart();
    }

    if ( _widthScissor.changed() || _syncRequired ) {
        data.uintVal = _widthScissor;
        _driver->writeGPURegister( cg1gpu::GPU_SCISSOR_WIDTH, data );
        _widthScissor.restart();
    }
    
    if ( _heightScissor.changed() || _syncRequired ) {
        data.uintVal = _heightScissor;
        _driver->writeGPURegister( cg1gpu::GPU_SCISSOR_HEIGHT, data );
        _heightScissor.restart();
    }
}

void GALRasterizationStageImp::forceSync()
{
    _syncRequired = true;
    sync();
    _syncRequired = false;
}

void GALRasterizationStageImp::_getGPUCullMode(GAL_CULL_MODE mode, cg1gpu::GPURegData* data)
{
    switch ( mode )
    {
        case GAL_CULL_NONE:
            data->culling = cg1gpu::NONE;
            break;
        case GAL_CULL_FRONT:
            data->culling = cg1gpu::FRONT;
            break;
        case GAL_CULL_BACK:
            data->culling = cg1gpu::BACK;
            break;
        case GAL_CULL_FRONT_AND_BACK:
            data->culling = cg1gpu::FRONT_AND_BACK;
            break;
        default:
            CG_ASSERT("Unknown cull mode");
    }    
}

void GALRasterizationStageImp::_getGPUFaceMode(GAL_FACE_MODE mode, cg1gpu::GPURegData* data)
{
    switch ( mode )
    {
        case GAL_FACE_CW:
            data->faceMode = cg1gpu::GPU_CW;
            break;
        case GAL_FACE_CCW:
            data->faceMode = cg1gpu::GPU_CCW;
            break;
        default:
            CG_ASSERT("Unknown cull mode");
    }    
}

void GALRasterizationStageImp::_getGPUInterpolationMode(GAL_INTERPOLATION_MODE mode, cg1gpu::GPURegData* data)
{
    switch ( mode ) {
        case GAL_INTERPOLATION_NONE:
            data->booleanVal = false;
            break;
        case GAL_INTERPOLATION_LINEAR:
            data->booleanVal = true;
            break;
        case GAL_INTERPOLATION_CENTROID:
            CG_ASSERT("GAL_INTERPOLATION_CENTROID not supported yet");
            break;
        case GAL_INTERPOLATION_NOPERSPECTIVE:
            CG_ASSERT("GAL_INTERPOLATION_NOPERSPECTIVE supported yet");
            break;
        default:
            CG_ASSERT("Unknown interpolation mode");
    }
}

const char* GALRasterizationStageImp::CullModePrint::print(const GAL_CULL_MODE& cullMode) const
{
    switch ( cullMode )
    {
        case GAL_CULL_NONE:
            return "NONE";
        case GAL_CULL_FRONT:
            return "FRONT";
        case GAL_CULL_BACK:
            return "BACK";
        case GAL_CULL_FRONT_AND_BACK:
            return "FRONT_AND_BACK";
        default:
            CG_ASSERT("Unknown cull mode");
            return ""; 
    }

};

const char* GALRasterizationStageImp::InterpolationModePrint::print(const GAL_INTERPOLATION_MODE& iMode) const
{
    switch ( iMode ) {
        case GAL_INTERPOLATION_NONE:
            return "NONE";
        case GAL_INTERPOLATION_LINEAR:
            return "LINEAR";
        case GAL_INTERPOLATION_CENTROID:
            return "CENTROID";
        case GAL_INTERPOLATION_NOPERSPECTIVE:
            return "NOPERSPECTIVE";
        default:
            CG_ASSERT("Unknown interpolation mode");
            return "";
    }
}

string GALRasterizationStageImp::getInternalState() const
{
    stringstream ss;

    ss << stateItemString(_cullMode, "CULL_MODE", false, &cullModePrint);

    // Viewport
    ss << stateItemString(_xViewport, "VIEWPORT_X", false); 
    ss << stateItemString(_yViewport, "VIEWPORT_Y", false); 
    ss << stateItemString(_widthViewport, "VIEWPORT_WIDTH", false);
    ss << stateItemString(_heightViewport, "VIEWPORT_HEIGHT", false);

    const gal_uint iCount = _interpolation.size();
    for ( gal_uint i = 0; i < iCount; ++i ) {
        stringstream aux;
        aux << "RASTER_ATTRIBUTE" << i << "_INTERPOLATION";
        ss << stateItemString(_interpolation[i], aux.str().c_str(), false, &interpolationModePrint);
    }

    return ss.str();
}

const StoredStateItem* GALRasterizationStageImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    TRACING_ENTER_REGION("GAL", "", "")
    GALStoredStateItem* ret;
    gal_uint interpolation;

    if (stateId >= GAL_RASTER_FIRST_ID && stateId < GAL_RASTER_LAST)
    {  
        if ((stateId >= GAL_RASTER_INTERPOLATION) && (stateId < GAL_RASTER_INTERPOLATION + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            // It�s a blending enabled state
            interpolation = stateId - GAL_RASTER_INTERPOLATION;
            ret = new GALSingleBoolStoredStateItem(_interpolation[interpolation]);
        }
        else 
        {
            switch(stateId)
            {
                case GAL_RASTER_FILLMODE:       ret = new GALSingleEnumStoredStateItem((gal_enum)(_fillMode));          break;
                case GAL_RASTER_CULLMODE:       ret = new GALSingleEnumStoredStateItem((gal_enum)(_cullMode));          break;
                case GAL_RASTER_VIEWPORT:       ret = new GALViewportStoredStateItem(_xViewport, _yViewport, _widthViewport, _heightViewport);  break;
                case GAL_RASTER_FACEMODE:       ret = new GALSingleEnumStoredStateItem((gal_enum)(_faceMode));          break;
                case GAL_RASTER_SCISSOR_TEST:   ret = new GALSingleBoolStoredStateItem((gal_enum)(_scissorEnabled));    break;
                case GAL_RASTER_SCISSOR_X:      ret = new GALSingleUintStoredStateItem((gal_enum)(_xScissor));          break;
                case GAL_RASTER_SCISSOR_Y:      ret = new GALSingleUintStoredStateItem((gal_enum)(_yScissor));          break;
                case GAL_RASTER_SCISSOR_WIDTH:  ret = new GALSingleUintStoredStateItem((gal_enum)(_widthScissor));      break;
                case GAL_RASTER_SCISSOR_HEIGHT: ret = new GALSingleUintStoredStateItem((gal_enum)(_heightScissor));     break;

                // case GAL_RASTER_... <- add here future additional rasterization states.
                default:
                    CG_ASSERT("Unknown rasterization state");
                    ret = 0;
            }
        }
    }
    else
        CG_ASSERT("Unexpected raster state id");

    ret->setItemId(stateId);

    TRACING_EXIT_REGION()
    return ret;
}


#define CAST_TO_UINT(X)     *(static_cast<const GALSingleUintStoredStateItem*>(X))
#define CAST_TO_BOOL(X)     *(static_cast<const GALSingleBoolStoredStateItem*>(X))

void GALRasterizationStageImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    TRACING_ENTER_REGION("GAL", "", "")
    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);
    gal_uint interpolation;

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    if (stateId >= GAL_RASTER_FIRST_ID && stateId < GAL_RASTER_LAST)
    {  
        if ((stateId >= GAL_RASTER_INTERPOLATION) && (stateId < GAL_RASTER_INTERPOLATION + cg1gpu::MAX_FRAGMENT_ATTRIBUTES))
        {    
            // It�s a blending enabled state
            interpolation = stateId - GAL_RASTER_INTERPOLATION;
             gal_enum param = *(static_cast<const GALSingleEnumStoredStateItem*>(galssi));
            _interpolation[interpolation] = GAL_INTERPOLATION_MODE(param);
            delete(galssi);
        }
        else
        {
            switch(stateId)
            {
                case GAL_RASTER_FILLMODE:
                {
                    gal_enum param = *(static_cast<const GALSingleEnumStoredStateItem*>(galssi));
                    _fillMode = GAL_FILL_MODE(param);
                    delete(galssi);
                    break;
                }
                case GAL_RASTER_CULLMODE:
                {
                    gal_enum param = *(static_cast<const GALSingleEnumStoredStateItem*>(galssi));
                    _cullMode = GAL_CULL_MODE(param);
                    delete(galssi);
                    break;
                }
                case GAL_RASTER_VIEWPORT:
                {
                    const GALViewportStoredStateItem* params = static_cast<const GALViewportStoredStateItem*>(galssi);
                    _xViewport = params->getXViewport(); 
                    _yViewport = params->getYViewport(); 
                    _widthViewport = params->getWidthViewport(); 
                    _heightViewport = params->getHeightViewport(); 
                    delete(galssi);
                    break;
                }

                case GAL_RASTER_SCISSOR_TEST:   _scissorEnabled = CAST_TO_BOOL(galssi);     delete(galssi);     break;
                case GAL_RASTER_SCISSOR_X:      _xScissor = CAST_TO_UINT(galssi);           delete(galssi);     break;
                case GAL_RASTER_SCISSOR_Y:      _yScissor = CAST_TO_UINT(galssi);           delete(galssi);     break;
                case GAL_RASTER_SCISSOR_WIDTH:  _widthScissor = CAST_TO_UINT(galssi);       delete(galssi);     break;
                case GAL_RASTER_SCISSOR_HEIGHT: _heightScissor = CAST_TO_UINT(galssi);      delete(galssi);     break;
                // case GAL_RASTER_... <- add here future additional rasterization states.
                default:
                    CG_ASSERT("Unknown rasterization state");
            }
        }
    }
    else
        CG_ASSERT("Unexpected raster state id");

    TRACING_EXIT_REGION()
}

#undef CAST_TO_UINT
#undef CAST_TO_BOOL

GALStoredState* GALRasterizationStageImp::saveAllState() const
{
    TRACING_ENTER_REGION("GAL", "", "")    

    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i = 0; i < GAL_RASTER_LAST; i++)
        ret->addStoredStateItem(createStoredStateItem(GAL_STORED_ITEM_ID(i)));

    TRACING_EXIT_REGION()
    
    return ret;
}

void GALRasterizationStageImp::restoreAllState(const GALStoredState* state)
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

    TRACING_EXIT_REGION()

}
