/**************************************************************************
 *
 */

#ifndef GAL_ZSTENCILSTAGE
    #define GAL_ZSTENCILSTAGE

#include "GALTypes.h"

namespace libGAL
{

/**
 * Comparison functions for Z and Stencil tests
 */
enum GAL_COMPARE_FUNCTION
{
    GAL_COMPARE_FUNCTION_NEVER, ///< Returns: 0 (false)
    GAL_COMPARE_FUNCTION_LESS, ///< Returns: Source < Destination
    GAL_COMPARE_FUNCTION_EQUAL, ///< Returns: Source == Destination
    GAL_COMPARE_FUNCTION_LESS_EQUAL, ///< Returns: Source <= Destination
    GAL_COMPARE_FUNCTION_GREATER, ///< Returns: Destination < Source
    GAL_COMPARE_FUNCTION_NOT_EQUAL, ///< Returns:  Source != Destination
    GAL_COMPARE_FUNCTION_GREATER_EQUAL, ///< Returns: Destination <= Source
    GAL_COMPARE_FUNCTION_ALWAYS ///< Returns: 1 (true)
};

/**
 * Stencil operations for the stencil test
 */
enum GAL_STENCIL_OP
{
    GAL_STENCIL_OP_KEEP, ///< Keep the existing stencil data
    GAL_STENCIL_OP_ZERO, ///< Set the stencil data to 0
    GAL_STENCIL_OP_REPLACE, ///< Set the stencil data to the reference value
    GAL_STENCIL_OP_INCR_SAT, ///< Increment the stencil value by 1, and clamp the result
    GAL_STENCIL_OP_DECR_SAT, ///< Decrement the stencil value by 1, and clamp the result
    GAL_STENCIL_OP_INVERT, ///< Invert the stencil data
    GAL_STENCIL_OP_INCR, ///< Increment the stencil value by 1, and wrap the result if necessary
    GAL_STENCIL_OP_DECR ///< Increment the stencil value by 1, and wrap the result if necessary
};

/**
 * Face identifiers
 */
enum GAL_FACE
{
    GAL_FACE_NONE, ///< undefined face
    GAL_FACE_FRONT, ///< Front face idenfier
    GAL_FACE_BACK, ///< Back face identifier
    GAL_FACE_FRONT_AND_BACK ///< front and back face identifier
};

/**
 * Interface to configure the CG1 ZStencil stage
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @date 02/07/2007
 */
class GALZStencilStage
{
public:

    /**
     * Enables or disables the Z test
     *
     * @param enable If true the Z test is enabled, otherwise the Z test is disabled
     */
    virtual void setZEnabled(gal_bool enable) = 0;

    virtual gal_bool isZEnabled() const = 0;

    /**
     * Sets the Z function used in the Z test
     *
     * @param zFunc Z function to use in the Z test
     */
    virtual void setZFunc(GAL_COMPARE_FUNCTION zFunc) = 0;

    /**
     * Gets the Z function used in the Z test
     *
     * @returns The function used to perform the Z test
     */
    virtual GAL_COMPARE_FUNCTION getZFunc() const = 0;

    /**
     * Enables or disables the Z mask
     */
    virtual void setZMask(gal_bool mask) = 0;

    /**
     * Gets if the current Z mask is enabled or disabled
     */
    virtual gal_bool getZMask() const = 0;


    /**
     * Enables or disables the Stencil test
     *
     * @param enable If true the stencil test is performed, otherwise the stencil test is omited
     */
    virtual void setStencilEnabled(gal_bool enable) = 0;

    virtual gal_bool isStencilEnabled() const = 0;

    /**
     * Sets the stencil test actions in a per-face basis
     *
     * @param face Face to apply this stencil test actions
     * @param onStencilFail Specifies the action to take when the stencil test fails
     * @param onStencilPassZFail Specifies the stencil action when the stencil test passes, 
     *                               but the depth test fails
     * @param onStencilPassZPass Specifies the stencil action when both    the stencil test and 
     *                       the depth test pass, or when the stencil test passes and either
     *                       there is no depth buffer or depth testing is not enabled
     */
    virtual void setStencilOp( GAL_FACE face, 
                               GAL_STENCIL_OP onStencilFail,
                               GAL_STENCIL_OP onStencilPassZFail,
                               GAL_STENCIL_OP onStencilPassZPass) = 0;

    virtual void getStencilOp( GAL_FACE face,
                               GAL_STENCIL_OP& onStencilFail,
                               GAL_STENCIL_OP& onStencilPassZFail,
                               GAL_STENCIL_OP& onStencilPassZPass) const = 0;

    /**
     * Sets the stencil test reference value
     *
     * @param face Face to apply this stencil function definition
     * @param func The stencil comparison function
     * @param stencilRef The stencil test reference value
     */
    virtual void setStencilFunc( GAL_FACE face, GAL_COMPARE_FUNCTION func, gal_uint stencilRef, gal_uint stencilMask ) = 0;

    /**
     * Sets the stencil range value
     *
     * @param near Specifies the mapping of the near clipping plane to window	coordinates.  The initial value	is 0
     * @param far Specifies the mapping of the far clipping plane to window	coordinates.  The initial value	is 1.
     */
    virtual void setDepthRange (gal_float near, gal_float far) = 0;

    /**
     *
     *  Sets the depth range for depth values in clip space that will be used:
     *    + [0, 1] for D3D9
     *    + [-1, 1] for OpenGL
     *
     *  @param mode Set to TRUE for D3D9 range and FALSE for OpenGL range.
     *
     */     
    virtual void setD3D9DepthRangeMode(gal_bool mode) = 0;

    /**
     * Sets the Polygon offset
     *
     * @param factor
     * @param units
     */
    virtual void setPolygonOffset (gal_float factor, gal_float units) = 0;

    /**
     * Sets the stencil update mask
     *
     * @param mask
     */
    virtual void setStencilUpdateMask (gal_uint mask) = 0;

    /**
     *
     *  Sets if the z stencil buffer has been defined.
     *
     *  @param enable Defines if the z stencil buffer is defined.
     *
     */
     
    virtual void setZStencilBufferDefined(gal_bool enable) = 0;
     
}; // class GALZStencil

} // namespace libGAL

#endif // GAL_ZSTENCILSTAGE
