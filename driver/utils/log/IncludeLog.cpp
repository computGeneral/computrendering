
#include "IncludeLog.h"
//#include "support.h"

LogObject& includelog::logfile()
{
    static LogObject* log = 0;
    if ( log == 0 )
    {
        //popup("includelog::logfile()", "Creating LogObject");
        log = new LogObject;
    }
    return *log;
}
