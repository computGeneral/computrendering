1. project dependences not resolved automatically on windows:
    GALx depends on driver/utils/OGLApiCodeGen
    D3DTraceCore depends on driver/utils/D3DApiCodeGen
    and both ApiCodeGen depends on flex and bison. (need to be installed on windows)

    *solution*
    hand install these tools (flex, bison)
    hand rebuild for now

2. generated code needs "unistd.h"
    *solution*
    install mingw