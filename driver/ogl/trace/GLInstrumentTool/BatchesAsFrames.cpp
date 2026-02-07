/**************************************************************************
 *
 */

#include "BatchesAsFrames.h"

void DoSwap::action(GLInstrument& gli)
{
    char msg[512];
    //sprintf(msg, "Frame: %d  Batch: %d", gli.currentFrame(), gli.currentBatch());
    //MessageBox(NULL, msg, "BatchesAsFrames message", MB_OK);
    gli.doSwapBuffers();
}

bool BatchesAsFrames::init(GLInstrument& gli)
{
    gliPtr = &gli;
    doSwap = new DoSwap;
    gli.registerBatchHandler(gli.After, *doSwap);
    return true;
}

BatchesAsFrames::~BatchesAsFrames()
{
    gliPtr->unregisterBatchHandler(GLInstrument::After);
    delete doSwap;
}
