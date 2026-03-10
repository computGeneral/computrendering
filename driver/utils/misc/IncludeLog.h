/**************************************************************************
 *
 */

#ifndef INCLUDELOG_H
    #define INCLUDELOG_H

#include "LogObject.h"

namespace includelog
{

enum LogType
{
    Init = 0x1,
    Panic = 0x2,
    Warning = 0x4,
    Debug = 0x8,
};

// Access to the global LogObject
LogObject& logfile();

} // namespace 

#endif // INCLUDELOG_H
