#pragma once

#include "RegStreamMultiprocessor.h"

namespace arch
{
    class cgoGpuRegister
    {
    public:
        static cgoGpuRegister* GpuReg;
        static cgoGpuRegister& GetGpuReg();

        cgoStreamMultiprocReg RegSM;
    private:
        cgoGpuRegister();
    };

}