/**************************************************************************
 *
 */

#include "Common.h"
#include "D3DInterface.h"

AIRoot9* D3DInterface::ai_root_9 = NULL;

void D3DInterface::initialize()
{
    D3D_DEBUG( cout << "D3DInterface: Initializing" << endl; )

    ai_root_9 = new AIRoot9();
}

void D3DInterface::finalize()
{
    D3D_DEBUG( cout << "D3DInterface: Finalizing" << endl; )

    delete ai_root_9;
}

AIRoot9* D3DInterface::get_gal_root_9()
{
    return ai_root_9;
}

D3DInterface::D3DInterface() {}

