
#include "GAL.h"
#include "GALDeviceImp.h"
#include "HAL.h"

using namespace libGAL;

libGAL::GALDevice* libGAL::createDevice(HAL* driver)
{
    if ( driver == 0 ) // direct access to the single driver
        return new GALDeviceImp(HAL::getHAL());
    return new GALDeviceImp(driver);
}

void libGAL::destroyDevice(GALDevice* galDev)
{
    delete static_cast<GALDeviceImp*>(galDev);
}
