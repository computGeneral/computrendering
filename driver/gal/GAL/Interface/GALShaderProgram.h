/**************************************************************************
 *
 */

#ifndef GAL_SHADERPROGRAM
    #define GAL_SHADERPROGRAM

#include "GALTypes.h"
#include <ostream>

namespace libGAL
{

/**
 * Common interface for shader programas (vertex,fragment and geometry shader programs)
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @date 02/27/2007
 */
class GALShaderProgram
{
public:

    /**
     * Sets the CG1 bytecode for this program
     */
    virtual void setCode(const gal_ubyte* CG1ByteCode, gal_uint sizeInBytes) = 0;

    /**
     *
     *  Set the Shader Program using a shader program written in CG1 Shader Assembly.
     *
     *  @param CG1ASM Pointer to the shader program assembly code.
     *
     */

    virtual void setProgram(gal_ubyte *CG1ASM) = 0;
         
    /**
     * Gets the bytecode of this program (previously set with setCode)
     */
    virtual const gal_ubyte* getCode() const = 0;

    /**
     * Gets the size in bytes of the program
     */
    virtual gal_uint getSize() const = 0;

    /**
     * Sets the value of a constant register exposed by the shader architecture
     *
     * @param index constant register index
     * @param vect4 vector of 4 floats to write into a constant register
     */
    virtual void setConstant(gal_uint index, const gal_float* vect4) = 0;

    /**
     * Gets the current value of a constant register exposed by the shader architecture
     *
     * @param index constant register index
     * @retval vect4 4-float out with the values of the constant register
     */
    virtual void getConstant(gal_uint index, gal_float* vect4) const = 0;

    /**
     * Sets if a texture unit is being used in that shader.
     *
     * @param tu Texture Unit number
     * @param usage == 0 Texture Unit not used // != 0 Type of Texture used in that Texture Unit
     */
    virtual void setTextureUnitsUsage(gal_uint tu, gal_enum usage) = 0;

    /**
     * Gets if a texture unit is being used in that shader.
     *
     * @param tu Texture Unit number
     * @retval usage == 0 Texture Unit not used // != 0 Type of Texture used in that Texture Unit
     */
    virtual gal_enum getTextureUnitsUsage(gal_uint tu) = 0;

    /**
     * Sets if a shader have Kil instructions.
     *
     * @param kill?
     */
    virtual void setKillInstructions(gal_bool kill) = 0;

    /**
     * Gets if a shader have Kil instructions.
     *
     * @retval kill?
     */
    virtual gal_bool getKillInstructions() = 0;

    /**
     * Prints the disassembled CG1 bytecode.
     *
     * @retval    os    The output stream.
     */
    virtual void printASM(std::ostream& os) const = 0;

    /**
     * Prints the CG1 bytecode in hex format.
     *
     * @retval    os    The output stream.
     */
    virtual void printBinary(std::ostream& os) const = 0;

    /**
     * Prints the program constants
     *
     * @retval os The output stream.
     */
    virtual void printConstants(std::ostream& os) const = 0;

};

}

#endif // GAL_SHADERPROGRAM
