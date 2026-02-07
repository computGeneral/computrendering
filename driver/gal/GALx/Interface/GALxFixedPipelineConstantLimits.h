/**************************************************************************
 *
 */

#ifndef GALx_FIXED_PIPELINE_CONSTANT_LIMITS_H
    #define GALx_FIXED_PIPELINE_CONSTANT_LIMITS_H

namespace libGAL
{
/**
 *    The maximum number of lights supported by the implementation
 */
#define GALx_FP_MAX_LIGHTS_LIMIT 8                
/**
 *    The maximum number of texture stages supported by the implementation
 */
#define GALx_FP_MAX_TEXTURE_STAGES_LIMIT 16
/**
 *  The maximum number of modelview matrices supported by the implementation
 */
#define GALx_FP_MAX_MODELVIEW_MATRICES_LIMIT 4

} // namespace libGAL

#endif // GALx_FIXED_PIPELINE_CONSTANT_LIMITS_H
