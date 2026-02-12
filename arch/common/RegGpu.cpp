#pragma once

#include "RegGpu.hpp"

namespace arch
{
    cgoGpuRegister::cgoGpuRegister()
    {
            
    }

    cgoGpuRegister& cgoGpuRegister::GetGpuReg()
    {
        static cgoGpuRegister GpuReg;
        return GpuReg;
    }
}