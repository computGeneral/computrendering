/**************************************************************************
 *
 */

#ifndef TOOLS_H
    #define TOOLS_H

#include "GLITool.h"

/////////////////////////////////////////////////////////
//// TESTEAR DOSWAPBUFFERS
//////////////////////////////////////////////////////



class FrameHandlerWakeup : public EventHandler
{
    void action(GLInstrument& gli);
};

class FrameHandlerInfo : public EventHandler
{
    void action(GLInstrument& gli);
};

// Tool Every N Frames Info tool
class EveryNInfoTool : public GLITool
{
private:

    int every;

public:
    
    EveryNInfoTool(int every);
    bool init(GLInstrument& gli);
};

#endif // TOOLS_H

