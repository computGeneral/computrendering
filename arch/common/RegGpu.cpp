#pragma once

#include "RegGpu.hpp"

namespace cg1gpu
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