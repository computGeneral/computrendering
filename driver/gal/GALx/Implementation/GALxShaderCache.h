/**************************************************************************
 *
 */

#ifndef GALxSHADERCACHE_H
    #define GALxSHADERCACHE_H

#include <map>
#include <list>
#include <vector>
#include "GALTypes.h"
#include "GALxTLShader.h"
#include "GALxTLState.h"
#include "GALxFixedPipelineSettings.h"

namespace libGAL
{

class GALxShaderCache
{
public:

    GALxShaderCache();

    /*
     * Try to find if a previous program is in the cache, if it is found the program is returned,
     * otherwise, it is return a false
     */
    gal_bool isCached(const GALx_FIXED_PIPELINE_SETTINGS shaderSettings, GALShaderProgram*& cachedProgram, GALxConstantBindingList*& bindingList);

    /*
     * Add a new entry to the cache. In case the cache is full, it is clear first
     */
    void addCacheEntry(GALx_FIXED_PIPELINE_SETTINGS shaderSettings, GALShaderProgram* cachedProgram, GALxConstantBindingList* bindingList);
    
    /**
     * Clear the program object from cache
     * the cleared programs are returned in a std::vector structure
     */
    void clear();
     
    /**
     * Returns if the cache is full
     */
    gal_bool full() const;
    
    /**
     * Returns the amount of programs stored currently in the cache
     */
    gal_uint size() const;    
    
    /**
     * Dump stadistics obtained from the use of the shader cache
     *
     */
    void dumpStatistics() const;
    
private:

    gal_uint computeChecksum(const GALx_FIXED_PIPELINE_SETTINGS fpSettings) const;

	bool compareShaderSettings(GALx_FIXED_PIPELINE_SETTINGS cachedShaderSet, GALx_FIXED_PIPELINE_SETTINGS checkingShaderSet);

    gal_uint maxPrograms;

    struct cachedDataType
	{
		GALx_FIXED_PIPELINE_SETTINGS _shaderSettings;
		GALShaderProgram* shaderProgram;
        GALxConstantBindingList* constantBinding;

		cachedDataType(GALx_FIXED_PIPELINE_SETTINGS shaderSettings, GALShaderProgram* cachedProgram, GALxConstantBindingList* bindingList)
		{
            shaderProgram = cachedProgram;
            constantBinding = bindingList;

            _shaderSettings.alphaTestEnabled = shaderSettings.alphaTestEnabled;
            _shaderSettings.alphaTestFunction = shaderSettings.alphaTestFunction;
            _shaderSettings.colorMaterialEnabledFace = shaderSettings.colorMaterialEnabledFace;
            _shaderSettings.colorMaterialMode = shaderSettings.colorMaterialMode;
            _shaderSettings.cullEnabled = shaderSettings.cullEnabled;
            _shaderSettings.cullMode = shaderSettings.cullMode;
            _shaderSettings.fogCoordinateSource = shaderSettings.fogCoordinateSource;
            _shaderSettings.fogEnabled = shaderSettings.fogEnabled;
            _shaderSettings.fogMode = shaderSettings.fogMode;
            _shaderSettings.lightingEnabled = shaderSettings.lightingEnabled;

			for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS; i++ )
			{
                _shaderSettings.lights[i].enabled = shaderSettings.lights[i].enabled;
                _shaderSettings.lights[i].lightType = shaderSettings.lights[i].lightType;
			}

            _shaderSettings.localViewer = shaderSettings.localViewer;
            _shaderSettings.normalizeNormals = shaderSettings.normalizeNormals;
            _shaderSettings.separateSpecular = shaderSettings.separateSpecular;

			for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
			{
                _shaderSettings.textureCoordinates[i].coordQ = shaderSettings.textureCoordinates[i].coordQ;
                _shaderSettings.textureCoordinates[i].coordR = shaderSettings.textureCoordinates[i].coordR;
                _shaderSettings.textureCoordinates[i].coordS = shaderSettings.textureCoordinates[i].coordS;
                _shaderSettings.textureCoordinates[i].coordT = shaderSettings.textureCoordinates[i].coordR;
                _shaderSettings.textureCoordinates[i].textureMatrixIsIdentity = shaderSettings.textureCoordinates[i].textureMatrixIsIdentity;
			}

			for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
			{
                _shaderSettings.textureStages[i].activeTextureTarget = shaderSettings.textureStages[i].activeTextureTarget;
                _shaderSettings.textureStages[i].baseInternalFormat = shaderSettings.textureStages[i].baseInternalFormat;
                // shaderSettings.textureStages[i].combineSettings = shaderSettings.textureStages[i].combineSettings;
                _shaderSettings.textureStages[i].enabled = shaderSettings.textureStages[i].enabled;
                _shaderSettings.textureStages[i].textureStageFunction = shaderSettings.textureStages[i].textureStageFunction;
			}

			_shaderSettings.twoSidedLighting = shaderSettings.twoSidedLighting;

		}
	};


    multimap<gal_uint, cachedDataType* >  cache;
    
    gal_uint hits;
    gal_uint misses;
    gal_uint clears;
    
};

} // namespace libGAL

#endif // GALxSHADERCACHE_H
