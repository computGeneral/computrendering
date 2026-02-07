#pragma once

#include "RegStreamMultiprocessor.h"

namespace cg1gpu
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