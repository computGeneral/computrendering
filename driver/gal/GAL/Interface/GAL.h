/**************************************************************************
 *
 */

#ifndef GAL
    #define GAL

#include "GALTypes.h"
#include "GALDevice.h"


class HAL;

namespace libGAL
{

    /**
     * Creayes the CG1 Common Driver object
     *
     * @retuns A ready to use GALDevice interface
     */
    GALDevice* createDevice(HAL* driver);


    void destroyDevice(GALDevice* galDev);

}

#endif // GAL
