/**************************************************************************
 *
 */

#ifndef GALx_FIXED_PIPELINE_STATE_H
    #define GALx_FIXED_PIPELINE_STATE_H

#include <list>
#include <string>

#include "GALxGlobalTypeDefinitions.h"

// Include the interfaces of the Fixed Pipeline stages 
#include "GALxTransformAndLightingStage.h"
#include "GALxTextCoordGenerationStage.h"
#include "GALxFragmentShadingStage.h"
#include "GALxPostShadingStage.h"

#include "GALxStoredFPState.h"
#include "GALxStoredFPItemID.h"


namespace libGAL
{
/**
 * Stored FP item list definition
 */
typedef std::list<GALx_STORED_FP_ITEM_ID> GALxStoredFPItemIDList;

/**
 * The GALxFixedPipelineState interface gives access
 * to the state related with the Fixed Pipeline.
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @version 0.8
 * @date 03/12/2007
 */
class GALxFixedPipelineState
{
public:
    
    /**
     * Gets an interface to the transform & lighting stages.
     */
    virtual GALxTransformAndLightingStage& tl() = 0;

    /**
     * Gets an interface to the texture coordinate generation stage.
     */
    virtual GALxTextCoordGenerationStage& txtcoord() = 0;

    /**
     * Gets an interface to the fragment shading and texturing stages.
     */
    virtual GALxFragmentShadingStage& fshade() = 0;

    /**
     * Gets an interface to the post fragment shading stages (FOG and Alpha).
     */
    virtual GALxPostShadingStage& postshade() = 0;

    /**
     * Save a single Fixed Pipeline state
     *
     * @param    stateId the identifier of the state to save
     * @returns The group of states saved
     */
    virtual GALxStoredFPState* saveState(GALx_STORED_FP_ITEM_ID stateId) const = 0;

    /**
     * Save a group of Fixed Pipeline states
     *
     * @param siIds List of state identifiers to save
     * @returns The group of states saved
     */
    virtual GALxStoredFPState* saveState(GALxStoredFPItemIDList siIds) const = 0;

    /**
     * Save the whole set of Fixed Pipeline states
     *
     * @returns The whole states group saved
     */
    virtual GALxStoredFPState* saveAllState() const = 0;

    /**
     * Restores the value of a group of Fixed Pipeline states
     *
     * @param state a previously saved state group to be restored
     */
    virtual void restoreState(const GALxStoredFPState* state) = 0;

    /**
     * Releases a GALxStoredFPState interface object
     *
     * @param state The saved state object to be released
     */
    virtual void destroyState(GALxStoredFPState* state) const = 0;

    //////////////////////////////////////////
    //      Debug/Persistence methods       //
    //////////////////////////////////////////

    /**
     * Dumps the GALxFixedPipelineState state
     */
    virtual void DBG_dump(const gal_char* file, gal_enum flags) const = 0;

    /**
     * Saves a file with an image of the current GALxFixedPipelineState state
     */
    virtual gal_bool DBG_save(const gal_char* file) const = 0;

    /**
     * Restores the GALxFixedPipelineState state from a image file previously saved
     */
    virtual gal_bool DBG_load(const gal_char* file) = 0;

};

} // namespace libGAL

#endif // GALx_FIXED_PIPELINE_STATE_H
