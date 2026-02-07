/**************************************************************************
 *
 */

#include "MainGLInstrument.h"

GLInstrumentImp& gli()
{
    static GLInstrumentImp glInstrumentImp;
    return glInstrumentImp;
}

GLJumpTable& glCalls()
{
    static GLJumpTable& glJT = gli().glCall();
    return glJT;
}

GLJumpTable& prevUserCall()
{
    static GLJumpTable& prevUserJT = gli().registerBeforeFunc();
    return prevUserJT;
}

GLJumpTable& postUserCall()
{
    static GLJumpTable& postUserJT = gli().registerAfterFunc();
    return postUserJT;
}


