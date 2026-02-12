/**************************************************************************
 *
 * cmoClipper mdu implementation file.
 *
 */

/**
 *
 *  @file cmoClipper.cpp
 *
 *  Implements the cmoClipper mdu class.
 *
 *  This class simulates the hardware clipper in a GPU.
 *
 *
 */

#include "cmClipper.h"
#include "bmClipper.h"
#include "support.h"
#include "cmClipperStateInfo.h"
#include "cmPrimitiveAssemblyRequest.h"
#include "cmTriangleSetupStateInfo.h"
#include "cmTriangleSetup.h"

#include <sstream>

using namespace arch;

//  cmoClipper constructor.  
cmoClipper::cmoClipper(U32 trCycle, U32 clipUnits, U32 startLat, U32 execLat, U32 bufferSize,
    U32 rastLat, U32 outputLat, char *name, cmoMduBase* parent) :

    trianglesCycle(trCycle), clipperUnits(clipUnits),
    startLatency(startLat), execLatency(execLat), clipBufferSize(bufferSize),
    rasterizerStartLat(rastLat), rasterizerOutputLat(outputLat), cmoMduBase(name, parent)
{
    DynamicObject *defaultState[1];

    //  Check parameters. 
    GPU_ASSERT(
        if (trianglesCycle != clipperUnits)
            CG_ASSERT("Number of clipper units must be the same that the triangles per cycle in current implementation.");
        if (clipBufferSize <= (3 * trianglesCycle))
            CG_ASSERT("cmoClipper triangle buffer too small for input latency and throughput.");
    )

    //  Create clipper signals.  

    //  Create command signal from the Command Processor.  
    clipperCommand = newInputSignal("ClipperCommand", 1, 1, NULL);

    //  Create state signal to the CommandProcessor.  
    clipperCommState = newOutputSignal("ClipperCommandState", 1, 1, NULL);

    //  Set default signal value.  
    defaultState[0] = new ClipperStateInfo(CLP_RESET);

    //  Set default primitive assembly state signal to the Command Processor.  
    clipperCommState->setData(defaultState);

    //  Create triangle input signal from Primitive Assembly.  
    clipperInput = newInputSignal("ClipperInput", trianglesCycle, 1, NULL);

    //  Create request signal to Primitive Assembly.  
    clipperRequest = newOutputSignal("ClipperRequest", 1, 1, NULL);

    //  Create new triangle signal to the Rasterizer unit.  
    rasterizerNewTriangle = newOutputSignal("RasterizerNewTriangle", trianglesCycle, rasterizerOutputLat, NULL);

    //  Create request signal from the Rasterizer unit.  
    rasterizerRequest = newInputSignal("ClipperRasterizerRequest", 1, 1, NULL);

    //  Create clipping start signal to the cmoClipper.  
    clipStart = newOutputSignal("ClipperExecution", clipperUnits, execLatency, NULL);

    //  Create clipping end signal from the cmoClipper.  
    clipEnd = newInputSignal("ClipperExecution", clipperUnits, execLatency, NULL);

    //  Create a dummy last clipper command.  
    lastClipperCommand = new ClipperCommand(CLPCOM_RESET);

    //  Allocate clip buffer.  
    clipBuffer = new TriangleSetupInput*[clipBufferSize];

   //  Check buffer allocation.  
    CG_ASSERT_COND(!(clipBuffer == NULL), "Error allocating the clip buffer.\n");
    //  Create statistics.  
    inputs = &getSM().getNumericStatistic("InputTriangles", U32(0), "cmoClipper", "CLP");
    outputs = &getSM().getNumericStatistic("OutputTriangles", U32(0), "cmoClipper", "CLP");
    clipped = &getSM().getNumericStatistic("ClippedTriangles", U32(0), "cmoClipper", "CLP");

    //  Initialize last triangle wait cycles.  
    lastTriangleCycles = 0;

    //  Start at reset state.  
    state = CLP_RESET;
}

//  cmoClipper simulation function.  
void cmoClipper::clock(U64 cycle)
{
    TriangleSetupInput *tsInput;
    ClipperCommand *clipComm;
    PrimitiveAssemblyRequest *paRequest;
    Vec4FP32 *v1, *v2, *v3;
    U32 triRequest;
    U32 i;

    GPU_DEBUG_BOX(
        printf("cmoClipper => Clock %lld.\n", cycle);
    )

    //  Receive triangle requests from Rasterizer.  
    if (rasterizerRequest->read(cycle, (DynamicObject *&) paRequest))
    {
        //  Update triangles requested.  
        requestedTriangles += paRequest->getRequest();

        //  Delete request object.  
        delete paRequest;
    }

    //  Update cycles remaining until last triangle signal reaches Rasterizer.  
    if (lastTriangleCycles > 0)
    {
        //  Update cycle counter.  
        lastTriangleCycles--;

        //  Ignore any triangle request while rasterizer hasn't received the last triangle signal.  
        requestedTriangles = 0;
    }

    //  Simulate current cycle.  
    switch(state)
    {
        case CLP_RESET:

            //  Reset state.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => RESET state.\n");
            )


            //  Set all the clip buffer entries to null.  
            for(i = 0; i < clipBufferSize; i++)
                clipBuffer[i] = NULL;

            //  Reset frustum clipping enable/disable flag.  
            frustumClip = TRUE;
            
            //  Reset D3D9 depth range mode register.
            d3d9DepthRange = false;

            //  Reset last triangle wait cycles counter.  
            lastTriangleCycles = 0;

            //  Change state to ready.  
            state = CLP_READY;

            break;

        case CLP_READY:

            //  Ready state.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => READY state.\n");
            )

            //  Receive clipper command from Command Processor.  
            if (clipperCommand->read(cycle, (DynamicObject *&) clipComm))
                processCommand(clipComm);

            break;

        case CLP_DRAW:

            //  Draw state.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => DRAW state.\n");
            )

            //  Check if there are available entries in the clip buffer.  
            if ((reservedEntries + clippedTriangles) < clipBufferSize)
            {
                //  Calculate number of triangles to request.  
                triRequest = GPU_MIN(clipBufferSize - reservedEntries - clippedTriangles, trianglesCycle);

                //  Request triangles to primitive assembly.  
                clipperRequest->write(cycle, new PrimitiveAssemblyRequest(triRequest));

                //  Update number of reserved entries.  
                reservedEntries += triRequest;
            }

            //  Receive triangle from Primitive Setup.  
            //  Check if there is a triangle being clipped.  
            if (clipCycles > 0)
            {
                GPU_DEBUG_BOX(
                    printf("cmoClipper => Remaining cycles for next triangle %d.\n",
                        clipCycles);
                )

                //  Update cycles remaining of the current triangle.  
                clipCycles--;
            }
            else
            {
                //  Receive a new triangle from Primitive Setup.  
                while (clipperInput->read(cycle, (DynamicObject *&) tsInput))
                {
                    CG_ASSERT_COND(!((reservedEntries + clippedTriangles) > clipBufferSize), "No clipper buffer entries available.");
                    //  Check if it is an already culled triangle.  
                    if (tsInput->isLast())
                    {
                        GPU_DEBUG_BOX(
                            printf("cmoClipper => Last triangle (ID %d) received.\n",
                                tsInput->getTriangleID());
                        )
                    }
                    else
                    {
                        GPU_DEBUG_BOX(
                            printf("cmoClipper => Triangle (ID %d) received from Primitive Assembly.\n",
                                tsInput->getTriangleID());
                        )
                    }

                    //  Start clipping.  Sent it to the clipping signal.  
                    clipStart->write(cycle, tsInput, execLatency);

                    //  Set clipping cycles to clip start latency.  
                    clipCycles = startLatency - 1;

                    //  Update statistics.  
                    inputs->inc();
                }
            }

            //  Receive clipped triangle.  
            while (clipEnd->read(cycle, (DynamicObject *&) tsInput))
            {

                CG_ASSERT_COND(!(clippedTriangles == clipBufferSize), "Clipped triangle buffer is full.");
                //  Check if it is the last triangle.  *
                if (tsInput->isLast())
                {
                    //  Store the triangle in the clipped triangle buffer.  
                    clipBuffer[nextFreeEntry] = tsInput;

                    //  Update clipped triangle counter.  
                    clippedTriangles++;

                    //  Update pointer to the next free entry in the clip  buffer.  
                    nextFreeEntry = GPU_MOD(nextFreeEntry + 1, clipBufferSize);
                }
                else
                {
                    //  Get triangle vertex attributes.  
                    v1 = tsInput->getVertexAttributes(0);
                    v2 = tsInput->getVertexAttributes(1);
                    v3 = tsInput->getVertexAttributes(2);

                    //  Perform trivial reject test with the triangle.  
                    if (bmoClipper::trivialReject(v1[0], v2[0], v3[0], d3d9DepthRange))
                    {
                        GPU_DEBUG_BOX(
                            printf("cmoClipper => Triangle (ID %d) culled.\n",
                            tsInput->getTriangleID());
                        )

                        //  Update statistics.  
                        clipped->inc();

                        //  Delete triangle vertex attributes.  
                        delete[] v1;
                        delete[] v2;
                        delete[] v3;

                        //  Drop clipped triangles.  
                        delete tsInput;
                    }
                    else
                    {
                        //  Store the triangle in the clipped triangle buffer.  
                        clipBuffer[nextFreeEntry] = tsInput;

                        //  Update clipped triangle counter.  
                        clippedTriangles++;

                        /*  Update pointer to the next free entry in the clip
                            buffer.  */
                        nextFreeEntry = GPU_MOD(nextFreeEntry + 1, clipBufferSize);
                    }
                }

                //  Update reserved entries counter.  
                reservedEntries--;
            }

            //  Update rasterizer cycles counter.  
            if (rasterizerCycles > 0)
                rasterizerCycles--;

            //  Check if rasterizer is ready and there are clipped triangles stored in the buffer.  
            if ((requestedTriangles > 0) && (rasterizerCycles == 0) &&
                ((clippedTriangles >= trianglesCycle) || ((clippedTriangles > 0) &&
                clipBuffer[GPU_PMOD(nextClipTriangle + clippedTriangles - 1, clipBufferSize)]->isLast())))
            {
                //  Send clipped triangles to Rasterizer.  
                for(i = 0; (i < trianglesCycle) && (clippedTriangles > 0) && (requestedTriangles > 0); i++)
                {
                    GPU_DEBUG_BOX(
                        printf("cmoClipper => Sending triangle (ID %d) to Rasterizer.\n",
                            clipBuffer[nextClipTriangle]->getTriangleID());
                    )

                    //  Send clipped triangle to Rasterizer.  
                    rasterizerNewTriangle->write(cycle, clipBuffer[nextClipTriangle]);

                    //  Update clipped triangle counter.  
                    clippedTriangles--;

                    //  Check if it is the last triangle.  
                    if (clipBuffer[nextClipTriangle]->isLast())
                    {
                        //  Check there are no remaining triangles in the buffer.  
                        CG_ASSERT_COND(!(clippedTriangles > 0), "Clipped triangles after the last triangle.");
                        //  Set last triangle cycles to the rasterizer signal latency and a little more.  
                        lastTriangleCycles = rasterizerOutputLat + 4;

                        //  End of batch, Change state to draw end.  
                        state = CLP_END;
                    }

                    //  Update pointer to the next clipped triangle.  
                    nextClipTriangle = GPU_MOD(nextClipTriangle + 1, clipBufferSize);

                    //  Update number of triangles requested by Rasterizer.  
                    requestedTriangles--;

                    //  Update processed triangles counter.  
                    triangleCount++;

                    //  Set cycles until next send to Rasterizer.  
                    rasterizerCycles = rasterizerStartLat;

                    //  Update statistics.  
                    outputs->inc();
                }
            }

            break;

        case CLP_END:

            //  End state.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => END state.\n");
            )

            //  Wait for end command.  
            if (clipperCommand->read(cycle, (DynamicObject *&) clipComm))
                processCommand(clipComm);

            break;

        default:
            CG_ASSERT("Unsupported state.");
            break;

    }

    //  Send state to Command Processor.  
    clipperCommState->write(cycle, new ClipperStateInfo(state));
}


//  Process a clipper command.  
void cmoClipper::processCommand(ClipperCommand *clipComm)
{
    //  Delete last clipper command.  
    delete lastClipperCommand;

    //  Set as last clipper command.  
    lastClipperCommand = clipComm;

    //  Process clipper command.  
    switch(clipComm->getCommand())
    {
        case CLPCOM_RESET:
            //  Reset command.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => CLPCOM_RESET command received.\n");
            )

            //  Change to reset state.  
            state = CLP_RESET;

            break;

        case CLPCOM_START:

            //  Start command.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => CLPCOM_START command received.\n");
            )

            //  Check if current state is CLP_READY.  
            CG_ASSERT_COND(!(state != CLP_READY), "Command not supported in current state.");
            //  Next triangle can be processed right now.  
            clipCycles = 0;

            /*  Reset cycles remaining until a triangle can be sent to
                Rasterizer.  */
            rasterizerCycles = 0;

            //  Reset processed triangle counter.  
            triangleCount = 0;

            //  Reset clipped triangles counter.  
            clippedTriangles = 0;

            //  Reset reserved clip buffer entries counter.  
            reservedEntries = 0;

            //  Reset next clipped triangle counter.  
            nextClipTriangle = 0;

            //  Reset next free clip buffer entry pointer.  
            nextFreeEntry = 0;

            //  Reset number of triangles requested by Rasterizer.  
            requestedTriangles = 0;

            //  Change to draw state.  
            state = CLP_DRAW;

            break;

        case CLPCOM_END:

            //  End command.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => CLPCOM_END command received.\n");
            )

            //  Check if current state is CLP_END.  
            CG_ASSERT_COND(!(state != CLP_END), "Command not supported in current state.");
            //  Change to ready state.  
            state = CLP_READY;

            break;

        case CLPCOM_REG_WRITE:

            //  Register write command.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => CLPCOM_REG_WRITE command received.\n");
            )

            //  Process register write.  
            processRegisterWrite(clipComm->getRegister(),
                clipComm->getSubRegister(), clipComm->getRegisterData());

            break;

        case CLPCOM_REG_READ:

            //  Register read command.  

            GPU_DEBUG_BOX(
                printf("cmoClipper => CLPCOM_REG_READ command received.\n");
            )

            CG_ASSERT("Register Read not supported.");

            break;

        default:
            CG_ASSERT("Unsupported clipper command.");
            break;
    }

}

//  Process register write.  
void cmoClipper::processRegisterWrite(GPURegister reg, U32 subReg,
    GPURegData data)
{
    //  Process register write.  
    switch(reg)
    {
        case GPU_FRUSTUM_CLIPPING:

            //  Frustum clipping enable/disable register.  

            frustumClip = data.booleanVal;

            GPU_DEBUG_BOX(
                printf("cmoClipper => Write GPU_FRUSTUM_CLIPPING = %s.\n",
                    frustumClip?"ENABLED":"DISABLED");
            )

            break;

        case GPU_USER_CLIP:

            //  User clip register.  

            CG_ASSERT("GPU_USER_CLIP not supported yet.");

            break;

        case GPU_USER_CLIP_PLANE:

            //  User clip plane register.  

            CG_ASSERT("GPU_USER_CLIP_PLANE not supported yet.");

            break;

        case GPU_D3D9_DEPTH_RANGE:
        
            //  Set D3D9 depth range in clip space register.
            
            d3d9DepthRange = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("cmoClipper => Write GPU_D3D9_DEPTH_RANGE = %s\n", d3d9DepthRange ? "T" : "F");
            )
            
            break;

        default:
            CG_ASSERT("Unsupported register write.");
            break;
    }
}


void cmoClipper::getState(string &stateString)
{
    stringstream stateStream;

    stateStream.clear();

    stateStream << " state = ";

    switch(state)
    {
        case CLP_RESET:
            stateStream << "CLP_RESET";
            break;
        case CLP_READY:
            stateStream << "CLP_READY";
            break;
        case CLP_DRAW:
            stateStream << "CLP_DRAW";
            break;
        case CLP_END:
            stateStream << "CLP_END";
            break;
        default:
            stateStream << "undefined";
            break;
    }

    stateStream << " | Clipped Triangles = " << clippedTriangles;
    stateStream << " | Requested Triangles = " << requestedTriangles;
    stateStream << " | Triangle Count = " << triangleCount;

    stateString.assign(stateStream.str());
}

