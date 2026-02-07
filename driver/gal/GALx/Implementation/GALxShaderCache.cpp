/**************************************************************************
 *
 */

#include "GALxShaderCache.h"
#include "InternalConstantBinding.h"


using namespace std;
using namespace libGAL;

GALxShaderCache::GALxShaderCache()
{
    maxPrograms = 100;
}

gal_bool GALxShaderCache::isCached(const GALx_FIXED_PIPELINE_SETTINGS shaderSettings, GALShaderProgram*& cachedProgram, GALxConstantBindingList*& bindingList)
{

    gal_uint crc = computeChecksum(shaderSettings);    // CRC creation of the entry shader settings

    pair<multimap<gal_uint, cachedDataType* >::iterator, 
         multimap<gal_uint, cachedDataType* >::iterator> range = cache.equal_range(crc);    // check if the resultant CRC is inside the cache

    
    if ( range.first != range.second )  // For every match with the same CRC check if any one have the entry shader setting
    {
        for ( multimap<gal_uint, cachedDataType* >::iterator matching = range.first; matching != range.second; matching++ )
        {
			if ( compareShaderSettings(matching->second->_shaderSettings, shaderSettings) ) // Compares the input and the cache entry settings struct
            {
                hits++; // Hit stats
				delete cachedProgram;
                cachedProgram = matching->second->shaderProgram;
                bindingList = matching->second->constantBinding;
                return true; // Return a hit
            }
        }
    }

    return false; // Return false in case of a miss*/
}

void GALxShaderCache::addCacheEntry(GALx_FIXED_PIPELINE_SETTINGS shaderSettings, GALShaderProgram* cachedProgram, GALxConstantBindingList* bindingList)
{

    gal_uint crc = computeChecksum(shaderSettings);    // CRC creation of the entry shader settings

    if ( cache.size() == maxPrograms ) // Check if the cache is full, in that case clear it
    {
        cout << "Attention: shader cache full, clearing it \n";
        clears++; // Clear stats
        clear();
    }
        
    misses++; // Miss stats
	
	cachedDataType* newEntry = new cachedDataType(shaderSettings, cachedProgram, bindingList);

    cache.insert(make_pair(crc, newEntry)); // Insert the new cache entry

}

bool GALxShaderCache::compareShaderSettings(GALx_FIXED_PIPELINE_SETTINGS cachedShaderSet, GALx_FIXED_PIPELINE_SETTINGS checkingShaderSet)
{

    gal_bool equivalent = true;

    equivalent &= cachedShaderSet.alphaTestEnabled == checkingShaderSet.alphaTestEnabled;
    equivalent &= cachedShaderSet.alphaTestFunction == checkingShaderSet.alphaTestFunction;
    equivalent &= cachedShaderSet.colorMaterialEnabledFace == checkingShaderSet.colorMaterialEnabledFace;
    equivalent &= cachedShaderSet.colorMaterialMode == checkingShaderSet.colorMaterialMode;
    equivalent &= cachedShaderSet.cullEnabled == checkingShaderSet.cullEnabled;
    equivalent &= cachedShaderSet.cullMode == checkingShaderSet.cullMode;
    equivalent &= cachedShaderSet.fogCoordinateSource == checkingShaderSet.fogCoordinateSource;
    equivalent &= cachedShaderSet.fogEnabled == checkingShaderSet.fogEnabled;
    equivalent &= cachedShaderSet.fogMode == checkingShaderSet.fogMode;
    equivalent &= cachedShaderSet.lightingEnabled == checkingShaderSet.lightingEnabled;

    for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS; i++ )
    {
        equivalent &= cachedShaderSet.lights[i].enabled == checkingShaderSet.lights[i].enabled;
        equivalent &= cachedShaderSet.lights[i].lightType == checkingShaderSet.lights[i].lightType;
    }

    equivalent &= cachedShaderSet.localViewer == checkingShaderSet.localViewer;
    equivalent &= cachedShaderSet.normalizeNormals == checkingShaderSet.normalizeNormals;
    equivalent &= cachedShaderSet.separateSpecular == checkingShaderSet.separateSpecular;

    for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
    {
        equivalent &= cachedShaderSet.textureCoordinates[i].coordQ == checkingShaderSet.textureCoordinates[i].coordQ;
        equivalent &= cachedShaderSet.textureCoordinates[i].coordR == checkingShaderSet.textureCoordinates[i].coordR;
        equivalent &= cachedShaderSet.textureCoordinates[i].coordS == checkingShaderSet.textureCoordinates[i].coordS;
        equivalent &= cachedShaderSet.textureCoordinates[i].coordT == checkingShaderSet.textureCoordinates[i].coordR;
        equivalent &= cachedShaderSet.textureCoordinates[i].textureMatrixIsIdentity == checkingShaderSet.textureCoordinates[i].textureMatrixIsIdentity;
    }

    for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
    {
        equivalent &= cachedShaderSet.textureStages[i].activeTextureTarget == checkingShaderSet.textureStages[i].activeTextureTarget;
        equivalent &= cachedShaderSet.textureStages[i].baseInternalFormat == checkingShaderSet.textureStages[i].baseInternalFormat;
        //equivalent &= cachedShaderSet.textureStages[i].combineSettings == checkingShaderSet.textureStages[i].combineSettings;
        equivalent &= cachedShaderSet.textureStages[i].enabled == checkingShaderSet.textureStages[i].enabled;
        equivalent &= cachedShaderSet.textureStages[i].textureStageFunction == checkingShaderSet.textureStages[i].textureStageFunction;
    }

    equivalent &= cachedShaderSet.twoSidedLighting == checkingShaderSet.twoSidedLighting;

    return equivalent;

}

gal_uint GALxShaderCache::computeChecksum(const GALx_FIXED_PIPELINE_SETTINGS fpSettings) const
{

    gal_uint crc = 0;

    crc += gal_uint(fpSettings.alphaTestEnabled);
    crc += gal_uint(fpSettings.alphaTestFunction);
    crc += gal_uint(fpSettings.colorMaterialEnabledFace);
    crc += gal_uint(fpSettings.colorMaterialMode);
    crc += gal_uint(fpSettings.cullEnabled);
    crc += gal_uint(fpSettings.cullMode);
    crc += gal_uint(fpSettings.fogCoordinateSource);
    crc += gal_uint(fpSettings.fogEnabled);
    crc += gal_uint(fpSettings.fogMode);
    crc += gal_uint(fpSettings.lightingEnabled);

	for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS; i++ )
    {
        crc += gal_uint(fpSettings.lights[i].enabled);
        crc += gal_uint(fpSettings.lights[i].lightType);
    }

    crc += gal_uint(fpSettings.localViewer);
    crc += gal_uint(fpSettings.normalizeNormals);
    crc += gal_uint(fpSettings.separateSpecular);

    for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
    {
        crc += gal_uint(fpSettings.textureCoordinates[i].coordQ);
        crc += gal_uint(fpSettings.textureCoordinates[i].coordR);
        crc += gal_uint(fpSettings.textureCoordinates[i].coordS);
        crc += gal_uint(fpSettings.textureCoordinates[i].coordT);
        crc += gal_uint(fpSettings.textureCoordinates[i].textureMatrixIsIdentity);
    }

    for (gal_uint i = 0; i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES; i++ )
    {
        crc += gal_uint(fpSettings.textureStages[i].activeTextureTarget);
        crc += gal_uint(fpSettings.textureStages[i].baseInternalFormat);
        //crc += gal_uint(fpSettings.textureStages[i].combineSettings);
        crc += gal_uint(fpSettings.textureStages[i].enabled);
        crc += gal_uint(fpSettings.textureStages[i].textureStageFunction);
    }

    crc += gal_uint(fpSettings.twoSidedLighting);

    return crc;
}

void GALxShaderCache::clear()
{    
    for (multimap<gal_uint, cachedDataType* >::iterator it = cache.begin() ; it != cache.end(); it++ )
    {
        GALxConstantBindingList::iterator iter = it->second->constantBinding->begin();

        while (iter != it->second->constantBinding->end())
        {
            //Delete InternalConstantBinding object TO DO: CAUTION: Not all the cb are icb´s
            InternalConstantBinding* icb = static_cast<InternalConstantBinding*>(*iter);

            delete icb;

            iter++;
        }

        delete (it->second->constantBinding);

        delete (it->second);

    }

    cache.clear();

    clears++;
}

bool GALxShaderCache::full() const
{
	return cache.size() == maxPrograms;
}

gal_uint GALxShaderCache::size() const
{
	return (U32) cache.size();
}

void GALxShaderCache::dumpStatistics() const
{ 
    cout << "hits: " << hits << " (" << (gal_float)(100.0f * ((gal_float)hits/(gal_float)(hits + misses))) << "%)\n";
    cout << "misses: " << misses << " (" << (gal_float)(100.0f * ((gal_float)misses/(gal_float)(hits + misses))) << "%)\n";
    cout << "clears: " << clears << "\n";
    cout << "programs in cache: " << cache.size() << endl;
}
