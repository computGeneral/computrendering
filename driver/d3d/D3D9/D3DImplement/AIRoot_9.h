/**************************************************************************
 *
 */

#ifndef AIROOT_9_H
#define AIROOT_9_H

#include "d3d9_port.h"
#include <set>

class AIDirect3DImp9;

/**
    Root interface for D3D9:
        Owner of IDirect3DImp9 objects.
        Implements some D3D9 functions.
*/
class AIRoot9 {
public:
    AIRoot9();
    ~AIRoot9();

    IDirect3D9* Direct3DCreate9(UINT SDKVersion);
private:
    std::set<AIDirect3DImp9*> i_childs;
};

#endif

