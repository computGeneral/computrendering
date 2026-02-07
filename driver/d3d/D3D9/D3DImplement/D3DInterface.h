/**************************************************************************
 *
 */

#ifndef D3DINTERFACE_H_INCLUDED
#define D3DINTERFACE_H_INCLUDED

#include "AIRoot_9.h"

class D3DTrace;

class D3DInterface
{
public:

    static void initialize(D3DTrace *trace);
    static void finalize();
    static AIRoot9 *get_gal_root_9();
    static D3DTrace *getD3DTrace();
    
private:

    static AIRoot9 *ai_root_9;
    static D3DTrace *trace;
    
    D3DInterface();
};

#endif

