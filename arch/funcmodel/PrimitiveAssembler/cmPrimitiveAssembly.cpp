/**************************************************************************
 *
 * Primitive Assembly mdu implementation file.
 *
 */


/**
 *
 *  @file cmoPrimitiveAssembler.cpp
 *
 *  This file implements the Primitive Assembly mdu.
 *
 *
 */

#include "cmPrimitiveAssembly.h"
#include "cmPrimitiveAssemblyStateInfo.h"
#include "cmPrimitiveAssemblyRequest.h"
#include "cmPrimitiveAssemblyInput.h"
#include "cmTriangleSetupInput.h"
#include "support.h"
#include "GPUMath.h"

#include <sstream>

using namespace cg1gpu;
using namespace std;

//  Primitive Assembly mdu constructor.  
cmoPrimitiveAssembler::cmoPrimitiveAssembler(U32 vertCycle, U32 trCycle, U32 attrCycle, U32 vertBusLat,
    U32 paQueueSz, U32 clipStartLat, char *name, cmoMduBase *parent) :

    verticesCycle(vertCycle), trianglesCycle(trCycle), attributesCycle(attrCycle), vertexBusLat(vertBusLat),
    paQueueSize(paQueueSz), clipperStartLat(clipStartLat), cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];
    U32 i;

    //  Check parameters.  
    CG_ASSERT_COND(!(trianglesCycle > paQueueSize), "Triangles per cycle entries required for the primitive assembly queue.");
    //  Create command signal from the Command Processor.  
    assemblyCommand = newInputSignal("PrimitiveAssemblyCommand", 1, 1, NULL);

    //  Create state signal to the Command Processor.  
    commandProcessorState = newOutputSignal("PrimitiveAssemblyCommandState", 1, 1, NULL);

    //  Set default signal value.  
    defaultState[0] = new PrimitiveAssemblyStateInfo(PA_READY);

    /*  Set default primitive assembly state signal to the Command
        Processor.  */
    commandProcessorState->setData(defaultState);

    //  Create new vertex signal from the cmoStreamController Commit.  
    streamerNewOutput = newInputSignal("PrimitiveAssemblyInput", verticesCycle,
        vertexBusLat + MAX_VERTEX_ATTRIBUTES / attributesCycle, NULL);

    //  Create request signal to the cmoStreamController Commit.  
    assemblyRequest = newOutputSignal("PrimitiveAssemblyRequest", 1, 1, NULL);

    //  Create triangle output signal to the Clipper.  
    clipperInput = newOutputSignal("ClipperInput", trianglesCycle, 1, NULL);

    //  Create request signal from the Clipper.  
    clipperRequest = newInputSignal("ClipperRequest", 1, 1, NULL);

    //  Allocate the primitive assembly queue.  
    assemblyQueue = new AssemblyQueue[paQueueSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(assemblyQueue == NULL), "Error allocating the primitive assembly queue.");
    //  Reset stream count value.  
    streamCount = 0;
    
    //  Reset instance count.
    streamInstances = 1;

    //  Reset primitive mode.  
    primitiveMode = TRIANGLE;

    //  Reset received vertex counter.  
    receivedVertex = 0;

    //  Reset stored vertex counter.  
    storedVertex = 0;

    //  Reset triangle counter.  
    triangleCount = 0;

    //  Reset last vertex pointer.  
    lastVertex = 0;

    //  Set first triangle as odd.  
    oddTriangle = TRUE;

    //  Set default vertex output configuration.  
    for(i = 0; i < MAX_VERTEX_ATTRIBUTES; i++)
        activeOutput[i] = FALSE;

    //  Reset assembly queue.  
    for(i = 0; i < paQueueSize; i++)
    {
        assemblyQueue[i].index = 0;
        assemblyQueue[i].attributes = NULL;
    }

    //  Create statistics.  
    vertices = &getSM().getNumericStatistic("Vertices", U32(0), "cmoPrimitiveAssembler", "PA");
    requests = &getSM().getNumericStatistic("Requests", U32(0), "cmoPrimitiveAssembler", "PA");
    triangles = &getSM().getNumericStatistic("Triangles", U32(0), "cmoPrimitiveAssembler", "PA");
    degenerated = &getSM().getNumericStatistic("Degenerated", U32(0), "cmoPrimitiveAssembler", "PA");

    //  Reset active vertex outputs counter.  
    activeOutputs = 0;

    //  Set a dummy last command.  
    lastPACommand = new PrimitiveAssemblyCommand(PACOM_RESET);

    //  Set initial state to reset.  
    state = PAST_RESET;
}


//  Primitive Assembly simulation rutine.  
void cmoPrimitiveAssembler::clock(U64 cycle)
{
    PrimitiveAssemblyCommand *paCommand;
    PrimitiveAssemblyInput *paInput;
    PrimitiveAssemblyRequest *paRequest;
    TriangleSetupInput *setupInput;
    U32 requestV;
    U32 i;

    CG_DEBUG("cmoPrimitiveAssembler => clock %lld\n", cycle);

    //  Read request from the Clipper unit.  
    if(clipperRequest->read(cycle, (DynamicObject *&) paRequest))
    {
        //  Get clipper request of triangles..  
        clipperRequests += paRequest->getRequest();

        //  Delete request object.  
        delete paRequest;
    }

//if (GPU_MOD(cycle, 1000) == 0)
//printf("PA %lld => clipperRequests %d storedVertex %d requestVertex %d clipCycles %d\n", cycle,
//clipperRequests, storedVertex, requestVertex, clipCycles);

    //  Process current state.  
    switch(state)
    {
        case PAST_RESET: //  Reset state.  
            CG_DEBUG( "cmoPrimitiveAssembler => RESET state.\n"); 
            //  Reset Primitive Assembly state.  
            streamCount = 0; //  Reset stream count value.  
            streamInstances = 1; //  Reset instance count;
            primitiveMode = TRIANGLE; //  Reset primitive mode.  
            receivedVertex = 0; //  Reset received vertex counter.  
            storedVertex = 0; //  Reset stored vertex counter.  
            triangleCount = 0; //  Reset triangle counter.  
            lastVertex = 0; //  Reset last vertex pointer.  
            oddTriangle = TRUE; //  Set first triangle as odd.  
            clipCycles = 0; //  Reset clipper cycle counter.  
            //  Set default vertex output configuration.  
            for(i = 0; i < MAX_VERTEX_ATTRIBUTES; i++)
                activeOutput[i] = FALSE;
            //  Reset assembly queue.  
            for(i = 0; i < paQueueSize; i++)
            {
                assemblyQueue[i].index = 0;
                if (assemblyQueue[i].attributes != NULL)
                {
                    delete[] assemblyQueue[i].attributes;
                    assemblyQueue[i].attributes = NULL;
                }
            }
            activeOutputs = 0; //  Reset active vertex outputs counter.  
            state = PAST_READY; //  Change state to ready.  
            break;

        case PAST_READY: //  Ready state.  
            CG_DEBUG("cmoPrimitiveAssembler => READY state.\n");
            //  Read a new command from the Command Processor. 
            if (assemblyCommand->read(cycle, (DynamicObject *&) paCommand))
                processCommand(paCommand);
            break;

        case PAST_DRAWING: //  Drawing state.  
            CG_DEBUG("cmoPrimitiveAssembler => DRAWING state.\n");
            CG_DEBUG("cmoPrimitiveAssembler => Stored vertexes %d  Requested vertexes %d.\n", storedVertex, requestVertex);

            //  Update clip cycle counter.  
            if (clipCycles > 0)
                clipCycles--;

            //  Check if all the vertices in the batch have been received or requested.  
            if ((receivedVertex + requestVertex) < (streamCount * streamInstances))
            {
                //  Check number of stored vertices.  
                if ((storedVertex + requestVertex) < paQueueSize)
                {
                    //  Calculate the number of triangles to request.  
                    requestV = GPU_MIN(paQueueSize - storedVertex - requestVertex, verticesCycle);

                    //  There is free space.  Send request.  
                    assemblyRequest->write(cycle, new PrimitiveAssemblyRequest(requestV));

                    //  Update requested vertexes counter.  
                    requestVertex += requestV;

                    //  Update statistics.  
                    requests->inc();
                }
            }

            //  Issue new triangles to the Clipper unit.  

            //  Issue last triangle signal.  
            if ((clipperRequests > 0) && (clipCycles == 0) && lastTriangle)
            {
                //  Send the last triangle signal down the pipeline.  
                setupInput = new TriangleSetupInput(triangleCount, NULL, NULL, NULL, TRUE);

                //  Copy cookies from parent DRAW command.  
                setupInput->copyParentCookies(*lastPACommand);

                //  Add a new cookie.  
                setupInput->addCookie();

                //  Send triangle to Clipper.  
                clipperInput->write(cycle, setupInput);

                //  Set clip cycle counter.  
                clipCycles = clipperStartLat;

                CG_DEBUG("cmoPrimitiveAssembler => Last triangle ID %d.\n", triangleCount);

                //  Update number of requested triangles by the clipper.  
                clipperRequests--;

                //  Update statistics.  
                triangles->inc();

                //  Change state to draw end.  
                state = PAST_DRAW_END;
            }

            //  Check if the Clipper is ready to receive new triangles.  
            if (clipCycles == 0)
            {
                for(i = 0; (i < trianglesCycle) && (clipperRequests > 0); i++)
                {
                    //  Assemble a new triangle and send it to the Clipper.  
                    assembleTriangle(cycle);
                }
            }

            //  Receive new vertex from the cmoStreamController Commit.  
            while (streamerNewOutput->read(cycle, (DynamicObject *&) paInput))
            {
                //  Check there is a free queue entry.  
                CG_ASSERT_COND(!(storedVertex == paQueueSize), "No free assembly queue entry.");
//if (cycle > 83000000)
//printf("PA %lld => Deleting assQ entry %d attributes %p\n", cycle, nextFreeEntry, assemblyQueue[nextFreeEntry].attributes);

                //  Delete old vertex attributes.  
                delete[] assemblyQueue[nextFreeEntry].attributes;

                //  Store input in the next assembly queue entry.  
                assemblyQueue[nextFreeEntry].index = paInput->getID();
                assemblyQueue[nextFreeEntry].attributes = paInput->getAttributes();

//if (cycle > 83000000)
//printf("PA %lld => Inserting vertex in assQ entry %d attributes %p\n", cycle, nextFreeEntry,
//assemblyQueue[nextFreeEntry].attributes);

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => Vertex with index %d received from cmoStreamController Commit.\n",
                        paInput->getID());
                )

                //  Update stored vertex counter.  
                storedVertex++;

                //  Update requested vertex counter.  
                requestVertex--;

                //  Update received vertex counter.  
                receivedVertex++;

                //  Update last vertex pointer.  
                //lastVertex = nextFreeEntry;

                //  Update pointer to next free queue entry.  

                //  Check if it is a FAN primitive mode.  
                if ((primitiveMode == TRIANGLE_FAN) || (primitiveMode == LINE_FAN))
                {
                    /*  A fan must always keep the first vertex.  Point to the second older vertex position.  Skip the
                        first vertex (0) when doing the module.  */
                    nextFreeEntry = (nextFreeEntry == (paQueueSize - 1))?1:GPU_MOD(nextFreeEntry + 1, paQueueSize);
                }
                else
                {
                    /*  For other primitive modes just point to the next
                        position in circular order.  */
                    nextFreeEntry = GPU_MOD(nextFreeEntry + 1, paQueueSize);
                }

                //  Update statistics.  
                vertices->inc();

                //  Delete primitive assembly input object.  
                delete paInput;
            }


            /*  Check if the last vertex was received and the last
                triangle was issued.  */
            if ((receivedVertex == (streamCount * streamInstances)) && (((storedVertex < 3) && ((primitiveMode == TRIANGLE) ||
                (primitiveMode == TRIANGLE_STRIP) || (primitiveMode == TRIANGLE_FAN))) ||
                ((storedVertex < 4) && ((primitiveMode == QUAD) || (primitiveMode == QUAD_STRIP)))))
            {
                //  Set last triangle assembled and issued to TRUE.  
                lastTriangle = TRUE;
            }

            break;

        case PAST_DRAW_END:
            //  End of drawing state.  

            GPU_DEBUG_BOX( printf("cmoPrimitiveAssembler => DRAW_END state.\n"); )

            //  Wait for END command from the Command Processor.  
            if (assemblyCommand->read(cycle, (DynamicObject *&) paCommand))
                processCommand(paCommand);

            break;

        default:
            CG_ASSERT("Unsupported Primitive Assembly state.");
            break;
    }

    //  Check current state.  
    if (state == PAST_READY)
    {
        //  Send current state to the Command Processor.  
        commandProcessorState->write(cycle, new PrimitiveAssemblyStateInfo(PA_READY));
    }
    else if (state == PAST_DRAW_END)
    {
        //  Send current state to the Command Processor.  
        commandProcessorState->write(cycle, new PrimitiveAssemblyStateInfo(PA_END));
    }
    else    //  It shouldn't matter what we send.  
    {
        //  Send current state to the Command Processor.  
        commandProcessorState->write(cycle, new PrimitiveAssemblyStateInfo(PA_READY));
    }
}


//  Processes a primitive assembly command.  
void cmoPrimitiveAssembler::processCommand(PrimitiveAssemblyCommand *command)
{
    U32 i;

    //  Delete last command.  
    delete lastPACommand;

    //  Set current command as last received command.  
    lastPACommand = command;

    //  Process current command.  
    switch(command->getCommand())
    {
        case PACOM_RESET:

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => RESET command received.\n");
            )

            //  Change to reset state.  
            state = PAST_RESET;

            break;

        case PACOM_DRAW:

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => DRAW command received.\n");
            )

            //  Check if the current state supports the command.  
            CG_ASSERT_COND(!(state != PAST_READY), "Command not supported in current state.");
            //  Check active vertex outputs.  Position (0) and diffuse color (1) required.  
            GPU_ASSERT(
                /*
                cout << "Active outputs: " << activeOutputs << endl;
                for ( int i = 0; i < MAX_VERTEX_ATTRIBUTES; i++ )
                {
                    cout << "activeOutput[" << i << "] = " << activeOutput[i] << endl;
                }
                */
                /*
                if (activeOutputs < 2)
                    CG_ASSERT("At least two vertex outputs must be active.");

                */
                if (!activeOutput[0])
                    CG_ASSERT("Vertex position (0) required for rasterization.");

                //if (!activeOutput[1])
                //    CG_ASSERT("Vertex color (1) required for rasterization.");
            )

            //  Change to draw state.  
            state = PAST_DRAWING;

            //  Initialize primitive assembly structures.  

            //  Set received vertex counter to 0.  
            receivedVertex = 0;

            //  Reset stored vertex counter.  
            storedVertex = 0;

            //  Reset the requested vertexes counter.  
            requestVertex = 0;

            //  Reset next queue entry pointer.  
            nextFreeEntry = 0;

            //  Reset triangle counter.  
            triangleCount = 0;

            //  Reset number of triangles requested by the Clipper.  
            clipperRequests = 0;

            // Initialize last vertex pointer.  
            switch(primitiveMode)
            {
                case TRIANGLE:
                case TRIANGLE_STRIP:
                case TRIANGLE_FAN:

                    lastVertex = 2;
                    break;

                case QUAD:
                case QUAD_STRIP:

                    lastVertex = 3;
                    break;

                case LINE:
                    CG_ASSERT("Line primitive not supported yet.");
                    break;

                case LINE_FAN:
                    CG_ASSERT("Line Fan primitive not supported yet.");
                    break;

                case LINE_STRIP:
                    CG_ASSERT("Line Strip primitive not supported yet.");
                    break;

                case POINT:
                    CG_ASSERT("Point primitive not supported yet.");
                    break;

                default:
                    CG_ASSERT("Primitive mode not supported.");
                    break;
            }

            //  Reset assembly queue.  
            for(i = 0; i < paQueueSize; i++)
            {
                assemblyQueue[i].index = 0;
                if (assemblyQueue[i].attributes != NULL)
                {
//printf("PA => Deleting assQ entry %d attributes %p\n", i, assemblyQueue[i].attributes);
//fdatasync(1);
//fflush(stdout);
                    delete[] assemblyQueue[i].attributes;
                    assemblyQueue[i].attributes = NULL;
                }
            }

            //  Reset clipper cycle counter.  
            clipCycles = 0;

            //  Set first triangle as odd.  
            oddTriangle = TRUE;

            //  Set last triangle flag to false.  
            lastTriangle = FALSE;

            break;

        case PACOM_END:

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => END command received.\n");
            )

            CG_ASSERT_COND(!(state != PAST_DRAW_END), "Command not supported in current state.");
            //  Change state to ready.  
            state = PAST_READY;

            break;

        case PACOM_REG_WRITE:

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => REGISTER WRITE command received.\n");
            )

            CG_ASSERT_COND(!(state != PAST_READY), "Command not supported in current state.");
            //  Process register write.  
            processRegisterWrite(command->getRegister(),
                command->getSubRegister(), command->getRegisterData());


            break;

        default:
            CG_ASSERT("Unsupported Primitive Assembly command.");
            break;
    }
}

//  Process a register write.  
void cmoPrimitiveAssembler::processRegisterWrite(GPURegister reg, U32 subReg,
    GPURegData data)
{
    //  Process primitive assembly register write.  
    switch(reg)
    {
        case GPU_VERTEX_OUTPUT_ATTRIBUTE:

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Write GPU_VERTEX_OUTPUT_ATTRIBUTE[%d] = %s.\n",
                    subReg, data.booleanVal?"ACTIVE":"DISABLED");
            )

            //  Check if it is changing current state.  
            if (activeOutput[subReg] != data.booleanVal)
            {
                //  Update active vertex outputs counter.  
                if (data.booleanVal)
                    activeOutputs++;
                else
                    activeOutputs--;
            }

            //  Set vertex output configuration.  
            activeOutput[subReg] = data.booleanVal;

            break;

        case GPU_STREAM_COUNT:

            //  Store batch/stream vertex count.  
            streamCount = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Write GPU_STREAM_COUNT = %d\n",
                    data.uintVal);
            )

            break;

        case GPU_STREAM_INSTANCES:

            //  Store batch/stream instance count.
            streamInstances = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Write GPU_STREAM_INSTANCES = %d\n",
                    data.uintVal);
            )

            break;

        case GPU_PRIMITIVE:

            //  Change the current primitive mode.  
            primitiveMode = data.primitive;

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Write GPU_PRIMITIVE = ");
                switch(data.primitive)
                {
                    case TRIANGLE:
                        printf("TRIANGLE.\n");
                        break;
                    case TRIANGLE_STRIP:
                        printf("TRIANGLE_STRIP.\n");
                        break;
                    case TRIANGLE_FAN:
                        printf("TRIANGLE_FAN.\n");
                        break;
                    case QUAD:
                        printf("QUAD.\n");
                        break;
                    case QUAD_STRIP:
                        printf("QUAD_STRIP.\n");
                        break;
                    case LINE:
                        printf("LINE.\n");
                        break;
                    case LINE_STRIP:
                        printf("LINE_STRIP.\n");
                        break;
                    case LINE_FAN:
                        printf("LINE_FAN.\n");
                        break;
                    case POINT:
                        printf("POINT.\n");
                        break;
                    default:
                        CG_ASSERT("Unsupported primitive mode.");
                        break;
                }
            )

            break;

        default:
            CG_ASSERT("Unsupported Primitive Assembly register.");
            break;
    }
}

//  Assembles a new triangle and sends it to the Clipper.  
void cmoPrimitiveAssembler::assembleTriangle(U64 cycle)
{
    TriangleSetupInput *setupInput;
    Vec4FP32 *vertexAttr1, *vertexAttr2, *vertexAttr3;
    U32 vertex1, vertex2, vertex3;

    // Check if there are enough vertices in the cache.  
    if(!lastTriangle && (((storedVertex >= 3) && ((primitiveMode == TRIANGLE) ||
        (primitiveMode == TRIANGLE_STRIP) || (primitiveMode == TRIANGLE_FAN))) ||
        ((storedVertex >= 4) && ((primitiveMode == QUAD) || (primitiveMode == QUAD_STRIP)))))
    {
        //  Process current primitive mode.  
        switch(primitiveMode)
        {
            case TRIANGLE:

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => New TRIANGLE Primitive.\n");
                )

                //  Update stored vertex counter.  
                storedVertex = storedVertex - 3;

                //  Get the three vertex that form the next triangle.  

                //  First vertex.  
                vertex1 = GPU_PMOD(lastVertex - 2, paQueueSize);

                //  Second vertex.  
                vertex2 = GPU_PMOD(lastVertex - 1, paQueueSize);

                //  Third vertex.  
                vertex3 = lastVertex;

                //  Update last vertex pointer.  
                lastVertex = GPU_MOD(lastVertex + 3, paQueueSize);

                break;

            case TRIANGLE_STRIP:

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => New TRIANGLE_STRIP Primitive.\n");
                )

                //  Free older triangle strip vertex.  
                storedVertex = storedVertex - 1;

                //  Get the three vertex that form the next triangle.  

                /*  Check if it is an odd or even triangle to keep
                    the correct vertex ordering.  */
                if (oddTriangle)
                {
                    //  First vertex.  
                    vertex1 = GPU_PMOD(lastVertex - 2, paQueueSize);

                    //  Second vertex.  
                    vertex2 = GPU_PMOD(lastVertex - 1, paQueueSize);

                    //  Change to even triangle.  
                    oddTriangle = FALSE;

                }
                else
                {
                    //  First vertex (invert order).  
                    vertex1 = GPU_PMOD(lastVertex - 1, paQueueSize);

                    //  Second vertex (invert order).  
                    vertex2 = GPU_PMOD(lastVertex - 2, paQueueSize);

                    //  Change to odd triangle.  
                    oddTriangle = TRUE;
                }

                //  Third vertex.  
                vertex3 = lastVertex;

                //  Update last vertex pointer.  
                lastVertex = GPU_MOD(lastVertex + 1, paQueueSize);

                break;

            case TRIANGLE_FAN:

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => New TRIANGLE_FAN Primitive.\n");
                )

                //  Free older triangle strip vertex.  
                storedVertex = storedVertex - 1;

                //  First vertex.  Always the first vertex received.  
                vertex1 = 0;

                //  Second vertex.  Must skip always the '0' vertex when doing the module.  
                vertex2 = (lastVertex == 1)?(paQueueSize - 1):GPU_PMOD(lastVertex - 1, paQueueSize);

                //  Third vertex.  
                vertex3 = lastVertex;

                //  Update last vertex pointer.  
                lastVertex = (lastVertex == (paQueueSize - 1))?1:GPU_MOD(lastVertex + 1, paQueueSize);

                break;

            case QUAD:

                /*  Convert QUAD into two triangles.

                     B--C       B--C  C
                     |  |   ==> | /  /|
                     |  |       |/  / |
                     A--D       A  A--D

                 */

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => New QUAD Primitive.\n");
                )

                //  Check if if is the first triangle.  
                if(oddTriangle)
                {
                    //  First triangle.  Use the first three vertex of the quad for the first triangle.  

                    //  First vertex.  
                    vertex1 = GPU_PMOD(lastVertex - 3, paQueueSize);

                    //  Second vertex.  
                    vertex2 = GPU_PMOD(lastVertex - 2, paQueueSize);

                    //  Third vertex.  
                    vertex3 = lastVertex;

                    //  First quad triangle assembled.  
                    oddTriangle = FALSE;
                }
                else
                {
                    /*  Second triangle.  Use the first, third and fourth vertexes of the quad for the triangle
                        (like in a triangle fan).  */

                    //  Free the quad vertexes.  
                    storedVertex = storedVertex - 4;

                    //  First vertex (invert order).  
                    vertex1 = GPU_PMOD(lastVertex - 2, paQueueSize);

                    //  Second vertex (invert order).  
                    vertex2 = GPU_PMOD(lastVertex - 1, paQueueSize);

                    //  Third vertex.  
                    vertex3 = lastVertex;

                    //  First triangle in the quad not assembled.  
                    oddTriangle = TRUE;

                    //  Update last vertex pointer.  
                    lastVertex = GPU_MOD(lastVertex + 4, paQueueSize);
                }


                break;

            case QUAD_STRIP:

                /*  Works as a triangle strip.

                    A---C---E
                    |\  |\  |
                    | \ | \ |
                    |  \|  \|
                    B---D---F

                 */

                GPU_DEBUG_BOX(
                    printf("cmoPrimitiveAssembler => New QUAD_STRIP Primitive.\n");
                )

                //  Get the three vertex that form the next triangle.  

                /*  Check if it is the first or the second triangle
                    in the current quad.  */
                if (oddTriangle)
                {
                    //  First vertex.  
                    vertex1 = GPU_PMOD(lastVertex - 3, paQueueSize);

                    //  Second vertex.  
                    vertex2 = GPU_PMOD(lastVertex - 2, paQueueSize);

                    //  Third vertex.  
                    vertex3 = lastVertex;

                    //  First triangle in the quad assembled.  
                    oddTriangle = FALSE;
                }
                else
                {
                    //  Free the quad vertexes.  
                    storedVertex = storedVertex - 2;

                    //  First vertex (invert order).  
                    vertex1 = GPU_PMOD(lastVertex - 1, paQueueSize);

                    //  Second vertex (invert order).  
                    vertex2 = GPU_PMOD(lastVertex - 3, paQueueSize);

                    //  Third vertex.  
                    vertex3 = lastVertex;

                    //  First triangle in the quad not assembled.  
                    oddTriangle = TRUE;

                    //  Update last vertex pointer.  
                    lastVertex = GPU_MOD(lastVertex + 2, paQueueSize);
                }

                break;

            case LINE:
                CG_ASSERT("Line primitive not supported yet.");
                break;

            case LINE_FAN:
                CG_ASSERT("Line Fan primitive not supported yet.");
                break;

            case LINE_STRIP:
                CG_ASSERT("Line Strip primitive not supported yet.");
                break;

            case POINT:
                CG_ASSERT("Point primitive not supported yet.");
                break;

            default:
                CG_ASSERT("Primitive mode not supported.");
                break;

        }


        /*  Perform trivial triangle reject of degenerate
            triangles using the triangle vertex indexes.  */

        //  Compare the three triangle vertex indexes.  
        if ((assemblyQueue[vertex1].index == assemblyQueue[vertex2].index) ||
            (assemblyQueue[vertex2].index == assemblyQueue[vertex3].index) ||
            (assemblyQueue[vertex3].index == assemblyQueue[vertex1].index))
        {
            //  Drop the triangle as at least two vertex are shared creating a degenerate triangle.  

            /*  No need to send the culled triangle down the pipeline.
                Using last triangle to mark end of batch and synchronization.  */

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Degenerated triangle (ID %d) culled.\n",
                    triangleCount);
            )

            //  Update the assembled triangle counter.  
            triangleCount++;

            //  Update the degenerated triangle counter.  
            degenerateTriangles++;

            //  Update statistics.  
            degenerated->inc();
        }
        else
        {
            //  Allocate the triangle vertex attributes.  
            vertexAttr1 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];
            vertexAttr2 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];
            vertexAttr3 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];

            //  Check memory allocation.  
            CG_ASSERT_COND(!((vertexAttr1 == NULL) || (vertexAttr2 == NULL) || (vertexAttr3 == NULL)), "Error allocating triangle vertex attributes.");
            //  Copy the triangle vertex attributes.  
            memcpy(vertexAttr1, assemblyQueue[vertex1].attributes, sizeof(Vec4FP32) * MAX_VERTEX_ATTRIBUTES);
            memcpy(vertexAttr2, assemblyQueue[vertex2].attributes, sizeof(Vec4FP32) * MAX_VERTEX_ATTRIBUTES);
            memcpy(vertexAttr3, assemblyQueue[vertex3].attributes, sizeof(Vec4FP32) * MAX_VERTEX_ATTRIBUTES);

            //  Create triangle setup input.  
            setupInput = new TriangleSetupInput(triangleCount, vertexAttr1, vertexAttr2, vertexAttr3, FALSE);

            //  Copy cookies from parent DRAW command.  
            setupInput->copyParentCookies(*lastPACommand);

            //  Add a new cookie.  
            setupInput->addCookie();

            //  Send triangle to clipper.  
            clipperInput->write(cycle, setupInput);

            GPU_DEBUG_BOX(
                printf("cmoPrimitiveAssembler => Triangle (ID %d) sent to the Clipper.\n",
                    triangleCount);
            )

            //  Set clip cycles counter.  
            clipCycles = clipperStartLat;

            //  Update number of pending triangle requests from the clipper.  
            clipperRequests--;

            //  Update the assembled triangle counter.  
            triangleCount++;

            //  Update statistics.  
            triangles->inc();
        }
    }
}

//  Get state string
void cmoPrimitiveAssembler::getState(string &stateString)
{
    stringstream stateStream;

    stateStream.clear();

    stateStream << " state = ";

    switch(state)
    {
        case PAST_RESET:
            stateStream << "PAST_RESET";
            break;
        case PAST_READY:
            stateStream << "PAST_READY";
            break;
        case PAST_DRAWING:
            stateStream << "PAST_DRAWING";
            break;
        case PAST_DRAW_END:
            stateStream << "PAST_DRAW_END";
            break;
        default:
            stateStream << "undefined";
            break;
    }

    stateStream << " | Received Vertices = " << receivedVertex;
    stateStream << " | Requested Vertices = " << requestVertex;
    stateStream << " | Stored Vertices = " << storedVertex;

    stateString.assign(stateStream.str());
}
