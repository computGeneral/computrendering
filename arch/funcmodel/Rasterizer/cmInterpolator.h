/**************************************************************************
 *
 * Interpolator mdu definition file.
 *
 */


/**
 *
 *  @file cmInterpolator.h
 *
 *  This file defines the Interpolator mdu.
 *
 *  The Interpolator mdu simulates the fragment attribute
 *  interpolators of a GPU.
 *
 */

#ifndef _INTERPOLATOR_

#define _INTERPOLATOR_

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmRasterizerCommand.h"
#include "bmRasterizer.h"
#include "cmRasterizerState.h"

namespace arch
{
//** Maximum interpolation latency.  
static const U32 MAX_INTERPOLATION_LATENCY = 8;

//** Default fragment attribute value.  
#define DEFAULT_FRAGMENT_ATTRIBUTE(qf) qf.setComponents(0.0, 0.0, 0.0, 0.0);

//**  Interpolation latency.  
static const U32 INTERPOLATION_LATENCY = 4;

/**
 *
 *  This class implements the Interpolator mdu.
 *
 *  The Interpolator mdu simulates the interpolation
 *  of fragment attributes.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class Interpolator : public cmoMduBase
{
private:

    //  Interpolator signals.  
    Signal *interpolatorCommand;    //  Command signal from the main rasterizer mdu.  
    Signal *interpolatorRastState;  //  State signal to the main rasterizer mdu.  
    Signal *newFragment;            //  New fragment signal from Triangle Traversal.  
    Signal *interpolationStart;     //  Interpolation start signal.  
    Signal *interpolationEnd;       //  Interpolation end signal.  
    Signal *interpolatorOutput;     //  Fragment output signal to the Fragment FIFO unit.  

    //  Interpolator registers.  
    U32 depthBitPrecission;      //  Depth buffer bit precission.  
    bool interpolation[MAX_FRAGMENT_ATTRIBUTES];        //  Flag storing if interpolation is enabled.  
    bool fragmentAttributes[MAX_FRAGMENT_ATTRIBUTES];   //  Stores the fragment input attributes that are enabled and must be calculated.  

    //  Interpolator parameters.  
    bmoRasterizer &bmRaster;    //  Reference to the Rasterizer Behavior Model used to interpolate the fragment attributes.  
    U32 interpolators;           //  Number of hardware interpolators.  
    U32 stampsCycle;             //  Number of stamps received per cycle.  
    U32 numStampUnits;           //  Number of stamps units.  

    //  Interpolator state.  
    RasterizerState state;          //  Current rasterization state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 triangleCounter;     //  Number of processed triangles.  
    U32 fragmentCounter;     //  Number of fragments processed in the current batch.  
    bool interpolationFinished; //  Stores if the interpolation of the current batch of fragments has finished.  
    U32 cyclesFragment;      //  Cycles needed before starting the interpolation of another group of fragments.  
    U32 remainingCycles;     //  Remaining cycles for the current group of fragments.  
    U32 currentTriangle;     //  Identifier of the current triangle being processed (used to count triangles).  
    U32 currentSetupTriangle;//  Identifier of the current setup triangle.  
    bool lastTriangle;          //  Stores if the current triangle is the last triangle.  
    bool firstTriangle;         //  First triangle already received flag.  
    U32 lastFragments;       //  Number of last fragments received for the current batch.  

    //  Private functions.  

    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param command The rasterizer command to process.
     *
     */

    void processCommand(RasterizerCommand *command);

    /**
     *
     *  Processes a register write.
     *
     *  @param reg The Interpolator register to write.
     *  @param subreg The register subregister to writo to.
     *  @param data The data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data);

public:

    /**
     *
     *  Interpolator mdu constructor.
     *
     *  Creates and initializes a new Interpolator mdu.
     *
     *  @param bmRaster The rasterizer behaviorModel to be used to interpolate
     *  the fragment attributes.
     *  @param interpolators The number of hardware interpolators supported
     *  (number of interpolated attributes calculated per cycle).
     *  @param stampsCycle Number of stamps received per cycle.
     *  @param stampUnits Number of stamp units in the GPU.
     *  @param name The mdu name.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new initialized Interpolator mdu.
     *
     */

    Interpolator(bmoRasterizer &bmRaster, U32 interpolators,
        U32 stampsCycle, U32 stampUnits, char *name, cmoMduBase *parent);

    /**
     *
     *  Interpolator mdu simulation function.
     *
     *  Simulates a cycle of the cmInterpolator.hardware.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void clock(U64 cycle);
};

} // namespace arch

#endif
