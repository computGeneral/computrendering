/**************************************************************************
 *
 */

#ifndef D3DINTERFACE_H_INCLUDED
#define D3DINTERFACE_H_INCLUDED

#include "AIRoot_9.h"

class D3DInterface
{
public:

    static void initialize();
    static void finalize();
    static AIRoot9 *get_gal_root_9();
    
private:

    static AIRoot9 *ai_root_9;
    
    D3DInterface();
};

#endif

