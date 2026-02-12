/**************************************************************************
 * Primitive Assembly mdu definition file.
 */

#ifndef _PRIMITIVEASSEMBLY_
#define _PRIMITIVEASSEMBLY_

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmPrimitiveAssemblyCommand.h"

namespace arch
{

//*  Number of shaded vertices in the assembly vertex queue.  
//#define NUM_ASSEMBLY_VERTEX 4

/**  Defines the different Primitive Assembly unit status states signaled
     to the other units.   */
enum AssemblyState
{
    PA_READY,       //  Ready to receive new vertices from the cmoStreamController Commit.  
    PA_FULL,        //  Can not receive new vertices from the cmoStreamController Commit.  
    PA_END          //  All the vertex in the current stream/batch have been procesed.  
};

//*  Defines the different Primitive Assembly unit functional states.  
enum PrimitiveAssemblyState
{
    PAST_RESET,     //  Reset state.  
    PAST_READY,     //  Ready State.  
    PAST_DRAWING,   //  Drawing state.  
    PAST_DRAW_END,  //  End of drawing state.  
};

/**
 *
 *  Defines a Primitive Assembly Queue entry.
 *
 */

struct AssemblyQueue
{
    U32 index;           //  Vertex index.  
    Vec4FP32 *attributes;  //  Vertex attributes.  
};


/**
 *
 *  This class implements the simulation mdu for the Primitive Assembly
 *  unit for a GPU.
 *
 *  Primitive Assembly receives vertex outputs from the cmoStreamController Commit
 *  in the original stream order and groups them in primitives (triangles)
 *  based in the active primitive mode.
 *
 *  This class inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class cmoPrimitiveAssembler : public cmoMduBase
{
private:

    //  Primitive Assembly signals.  
    Signal *assemblyCommand;        //  Command signal from the Command Processor.  
    Signal *commandProcessorState;  //  State signal to the Command Processor.  
    Signal *streamerNewOutput;      //  New vertex output signal from the StreamController Commit.  
    Signal *assemblyRequest;        //  Request signal from the Primitive Assembly unit to the cmoStreamController Commit unit.  
    Signal *clipperInput;           //  Triangle input signal to the Clipper unit.  
    Signal *clipperRequest;         //  Request signal from the Clipper unit.  

    //  Primitive assembly registers.  
    U32 verticesCycle;           //  Number of vertices received from cmoStreamController Commit per cycle.  
    U32 trianglesCycle;          //  Number of triangles sent to the Clipper per cycle.  
    U32 attributesCycle;         //  Vertex attributes sent from cmoStreamController Commit per cycle.  
    U32 vertexBusLat;            //  Latency of the vertex bus from cmoStreamController Commit.  
    U32 paQueueSize;             //  Size in vertices of the primitive assembly queue.  
    U32 clipperStartLat;         //  Start latency of the clipper test units.  


    //  Primitive assembly registers.  
    bool activeOutput[MAX_VERTEX_ATTRIBUTES]; //  Stores if a vertex output attribute is active.  
    U32 activeOutputs;           //  Number of vertex active output attributes.  
    PrimitiveMode primitiveMode;    //  Current primitive mode.  
    U32 streamCount;             //  Number of vertex in the current stream/batch.  
    U32 streamInstances;         //  Number of instances of the current stream to process.  

    //  Primitive Assembly state.  
    PrimitiveAssemblyState state;   //  Current state of the Primitive Assembly unit.  
    U32 triangleCount;           //  Number (and ID) of the assembled triangles in the current stream/batch.  
    bool oddTriangle;               //  Indicates if the current triangle is or odd or even (used for triangle strips).  
    U32 degenerateTriangles;     //  Degenerated triangles counter.  
    PrimitiveAssemblyCommand *lastPACommand;    //  Last primitive assembly command received (for signal tracing).  
    bool lastTriangle;              //  If the last batch triangle has been already sent.  
    U32 clipCycles;              //  Cycles since last time triangles were sent to the clipper.  
    U32 clipperRequests;         //  Number of triangles requested by the clipper.  

    //  Primitive Assembly Queue.  
    AssemblyQueue *assemblyQueue;   //  Assembly vertex queue.  
    U32 receivedVertex;          //  Number of received vertices from the cmoStreamController.  
    U32 storedVertex;            //  Number of vertices currently stored in the assembly queue.  
    U32 requestVertex;           //  Number of vertices currenty requested to the cmoStreamController unit.  
    U32 nextFreeEntry;           //  Position of the next free assembly queue entry.  
    U32 lastVertex;              //  Pointer to the last vertex stored in the assembly queue.  

    //  Statistics.  
    gpuStatistics::Statistic *vertices; //  Number of received vertices received from cmoStreamController Commit.  
    gpuStatistics::Statistic *requests; //  Number of requests send to cmoStreamController Commit.  
    gpuStatistics::Statistic *degenerated;  //  Number of degenerated triangles found.  
    gpuStatistics::Statistic *triangles;    //  Number of triangles sent to Clipper.  

    //  Private functions.  

    /**
     *
     *  Processes a new Primitive Assembly Command.
     *
     *  @param command The primitive assembly command to process.
     *
     */

    void processCommand(PrimitiveAssemblyCommand *command);

    /**
     *
     *  Processes a register write to the Primitive Assembly.
     *
     *  @param reg Primitive Assembly register to write.
     *  @param subreg Primitive Assembly subregister to write.
     *  @param data Data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data);

    /**
     *
     *  Assembles a new triangle and sends it to the Clipper.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void assembleTriangle(U64 cycle);

public:

    /**
     *
     *  Creates and initializes a new Primitive Assembly mdu.
     *
     *  @param vertCycle Vertices per cycle received from cmoStreamController Commit.
     *  @param trCycle Triangles per cycle sent to Clipper.
     *  @param attrCycle Vertex attributes received from cmoStreamController Commit per cycle.
     *  @param vertBusLat Latency of the vertex bus from cmoStreamController Commit.
     *  @param paQueueSize Size in vertices of the assembly queue.
     *  @param clipStartLat Start latency of the Clipper test units.
     *  @param name The mdu name.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new initialized cmoPrimitiveAssembler mdu.
     *
     */

    cmoPrimitiveAssembler(U32 vertCycle, U32 trCycle, U32 attrCycle, U32 vertBustLat, U32 paQueueSize,
        U32 clipStartLat, char *name, cmoMduBase *parent);

    /**
     *
     *  Primitive Assembly simulation function.
     *
     *  Performs the simulation of a cycle of the Primitive Assembly
     *  GPU unit.
     *
     *  @param cycle The cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Primitive Assembly mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(std::string &stateString);

};

} // namespace arch

#endif
