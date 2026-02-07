/**************************************************************************
 *
 */

#ifndef GLITOOL_H
    #define GLITOOL_H

#include "GLInstrument.h"

struct GLITool
{
    /**
     * This method must have the code required to instrument OpenGL app
     * inserting handlers to GLInstrument object
     *
     * This method will be called just after retrieve the tool from GLInstrument
     */
    virtual bool init(GLInstrument& gli)=0;

};


#endif // GLITOOL_H
