/**************************************************************************
 *
 * GPU behaviorModel definition file.
 *  This file contains definitions and includes for the CG1 GPU behaviorModel.
 *
 */
#ifndef __CG1_BHAVMODEL_HPP__
#define __CG1_BHAVMODEL_HPP__

//  Configuration file definitions.
#include "archParams.h"
#include "param_loader.hpp"

//  cgoTraceDriverBase definition.
#include "TraceDriverBase.h"

//  Behavior Model classes.
#include "bmGpuTop.h"
#include "modelbase.h"

#include <vector>
#include <map>

namespace arch
{

/** GPU Behavior Model class.
 *  The class defines structures and attributes to implement a GPU behaviorModel.*/
class BhavModel : public ModelBase
{
private:
    cgsArchConfig ArchConf;             //  Stores the behaviorModel configuration parameters.  
    cgoTraceDriverBase *TraceDriver;   //  Pointer to the objects used to obtain MetaStreams that drive the emulation.  
    
public:
    bool traceEnd;        //  Flag that stores if the trace to be emulated has been completely processed.  
    bool AbortSim;        //  Flag that stores if the emulation must be aborted due to an 'external' event.  
    bmoGpuTop   GpuBMdl;
    /** BhavModel constructor.
     *  Creates and initializes the different behaviorModel objects.  Allocates the emulated GPU memory.
     *  Initializes the caches for compressed textures.
     *  @param ArchConf Configuration parameters for the behaviorModel.
     *  @param TraceDriver Pointer to a cgoTraceDriverBase object from which to obtain MetaStreams.
     *  @return An initialized BhavModel object.
     */
    BhavModel(arch::cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver);
    void simulationLoop(cgeModelAbstractLevel MAL = CG_BEHV_MODEL);  // Implements a fire and forget emulation main loop. */
    void abortSimulation(); // Abort emulation. */ 
    void saveSnapshot();    // Save a snapshot of the current behaviorModel state. */
    void loadSnapshot();    // Load a snapshot of the behaviorModel state.*/
    // dummy function to share the ModelBase class
    void simulationLoopMultiClock() {};
    void debugLoop(bool validate) {};
    void createSnapshot() {};
    void getCycles(U64& gpuCycle, U64& shaderCycle, U64& memCycle) {};
    void getCounters(U32& frameCounter, U32& frameBatch, U32& batchCounter) {};
};  // class BhavModel
};  //  namespace arch

#endif
