/**************************************************************************
 *
 */

#ifndef GALx_FPFACTORY_H
    #define GALx_FPFACTORY_H

#include "GALxFPState.h"
#include "GALxTLShader.h"
#include "GALxCodeSnip.h"
#include "GALxTransformations.h"
#include <string>

namespace libGAL
{

class GALxFPFactory 
{
private:

    static GALxTLShader* tlshader;
    
    static GALxFPState* fps;
    static GALxCodeSnip code;
    
    static void constructInitialization();
    
    static void computeColorSum();
    
    static void computeFogApplication();
    static void computeFogFactor();
    static void computeFogCombination();

    static void computeTextureApplication(GALxFPState::TextureUnit textureUnit, 
                                          std::string inputFragmentColorTemp, 
                                          std::string outputFragmentColorTemp,
                                          std::string primaryColorRegister);
    
    static void computeTextureApplicationTemporaries();
    
    static void constructTexturing();
    
    static void constructFragmentProcessing();
    
    static void constructPostProcessingTests();
    static void constructAlphaTest();
    
    static void computeDepthBypass();

    GALxFPFactory();
    GALxFPFactory(const GALxFPFactory&);
    GALxFPFactory& operator=(const GALxFPFactory&);
    
public:
    
    static GALxTLShader* constructFragmentProgram(GALxFPState& fps);
    
    /*
     * Constructs a small ARB fragment program containing isolated alpha test implementation.
     * The code is a well-formed fragment program that can be compiled by itself and uses
     * a first temporary that will contain the final color and second temporary test register 
     * for kill condition.
     */
    static GALxTLShader* constructSeparatedAlphaTest(GALxFPState& fps);

};

} // namespace libGAL

#endif // GALx_FPFACTORY_H
