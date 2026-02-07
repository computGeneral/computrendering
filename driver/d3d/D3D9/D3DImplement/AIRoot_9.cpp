/**************************************************************************
 *
 */

#include "Common.h"
#include "AIDirect3DImp_9.h"
#include "AIRoot_9.h"


using namespace std;

AIRoot9::AIRoot9(/*StateDataNode* s_root*/)/*:state(s_root)*/ {
}

AIRoot9::~AIRoot9() {
    set< AIDirect3DImp9* > :: iterator it;
    for(it = i_childs.begin(); it != i_childs.end(); it ++)
        delete *it;
}

IDirect3D9* AIRoot9::Direct3DCreate9(UINT SDKVersion) 
{
    D3D9_CALL(true, "AIRoot9::Direct3DCreate9")

    /// @note SDKVersion is ignored from here on
    AIDirect3DImp9* d3d = new AIDirect3DImp9(/*state*/);
    i_childs.insert(d3d);
    return d3d;
}






