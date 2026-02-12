/**************************************************************************
 *
 * Shader Architecture Parameters.
 * This file defines the ShaderArchParam class.
 *
 * The ShaderArchParam class defines a container for architecture parameters (instruction
 * execution latencies, repeation rates, etc) for a simulated shader architecture.
 *
 */
#ifndef __SHADER_ARCH_PARAM_H__
#define __SHADER_ARCH_PARAM_H__

#include "support.h"
#include "GPUType.h"
#include "ShaderInstr.h"

#include <string>

using namespace std;

namespace arch
{

/**
 *
 *  The ShaderArchParam class is a container for architecture parameters of the
 *  shader processor.
 *
 *  The class is a singleton used as a container for parameters.
 *
 */

class ShaderArchParam
{
private:

    static ShaderArchParam * shArchParams; //  Pointer to the singleton instance of the class.  
    
    static U32 SIMD4VarLatExecLatencyTable[];      //  Execution latency table for variable execution latency SIMD4 architecture.  
    static U32 SIMD4VarLatRepeatRateTable[];       //  Repeat rate table for variable execution latency SIMD4 architecture.  
    
    static U32 SIMD4FixLatExecLatencyTable[];    //  Execution latency table for fixed execution latency SIMD4 architecture.  
    static U32 SIMD4FixLatRepeatRateTable[];     //  Repeat rate table for fixed execution latency SIMD4 architecture.  
    
    static U32 ScalarVarLatExecLatencyTable[];      //  Execution latency table for variable execution latency Scalar architecture.  
    static U32 ScalarVarLatRepeatRateTable[];       //  Repeat rate table for variable exeuction latency Scalar architecture.      
    
    static U32 ScalarFixLatExecLatencyTable[];    //  Execution latency table for fixed execution latency Scalar architecture.  
    static U32 ScalarFixLatRepeatRateTable[];     //  Repeat rate table for fixed exeuction latency Scalar architecture.      
    
        
    U32 *execLatencyTable;    //  Pointer to the selected execution latency table.      
    U32 *repeatRateTable;     //  Pointer to the selected repeat rate table.  
    
    //  Constructor
    ShaderArchParam();
public:

    /**
     *  The function returns a pointer to the single instance of the ShaderArchParam class.
     *  If the instance was not yet created it is created at this point.
     *
     *  @return The pointer to the singleton instance of the ShaderArchParam class.
     */
    static ShaderArchParam *getShaderArchParam();     

    /**
     *  The function returns a list of available shader architecture configurations.
     *
     *  @param archList A reference to a string where to store a list with the names of the shader architecture
     *  configurations available for selection.
     */
    static void architectureList(string &archList); 
    
    /**
     *  The function is used to select the shader architecture configuration to use for
     *  queries.
     *
     *  @param archName Name of the shader architecture configuration to use for shader architecture
     *  parameter queries.
     */
    void selectArchitecture(string archName);

    /**
     *  The function is used to obtain the execution latency (pipeline depth) for a given shader instruction opcode.
     *
     *  @param opcode Shader Instruction opcode for which the execution latency is queried.
     *  @return The execution latency in cycles for the shader instruction opcode.
     */
    U32 getExecutionLatency(ShOpcode opcode);

    /**
     *  The function is used to obtain the repeat rate (cycles until next shader instruction can be issued) for
     *  a given shader instruction opcode.
     *
     *  @param opcode Shader Instruction opcode for which the repeat rate is queried.
     *  @return The repeat rate (cycles until next shader instruction can be issued) for the shader instruction opcode.
     */
    U32 getRepeatRate(ShOpcode opcode);
};

}  // namespace arch

#endif
