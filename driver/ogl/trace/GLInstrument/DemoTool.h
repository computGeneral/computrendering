/**************************************************************************
 *
 */

#ifndef DEMOTOOL_H
    #define DEMOTOOL_H

#include "GLITool.h"

class DemoTool : public GLITool
{
public:
    
    bool init(GLInstrument& gli);  
    bool release();
    
    DemoTool();
    ~DemoTool();
};

#endif DEMOTOOL_H
