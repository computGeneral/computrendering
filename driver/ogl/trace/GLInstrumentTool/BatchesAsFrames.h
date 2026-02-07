/**************************************************************************
 *
 */

#ifndef BATCHESASFRAMES_H
    #define BATCHESASFRAMES_H

#include "GLITool.h"

class DoSwap : public EventHandler
{
    void action(GLInstrument& gli);
};

class BatchesAsFrames : public GLITool
{
private:

    DoSwap* doSwap;
    GLInstrument* gliPtr;

public:

    bool init(GLInstrument& gli);

    ~BatchesAsFrames();
};

#endif // BATCHESASFRAMES_H
