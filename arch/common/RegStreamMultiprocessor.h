#pragma once
#include "GPUType.h"
namespace cg1gpu
{
// Address definition for Stream Multiprocessor registers
enum cgeStreamMultiprocRegAddr
{
    CG_REG_SM_CONFIG = 1000, 
    CG_REG_SM_WORKGROUP_DIM,
    CG_REG_SM_WORKGROUP_SIZE,
};


class cgoStreamMultiprocConfig
{
public:
    union cguStreamMultiprocConfig
    {
        struct cgsStreamMultiprocConfig
        {
            U32 dimension : 2, // bit 0-1 work-item dimension // 2'b00 : work item dim one // 2'b01 : work item dim two // 2'b10 : work item dim three
                traverseMode : 3, // thread traverse mode // 3'b000: linear mode // 3'b001: linear mode
                initRegCount : 3, // initialized register count by meta stream.
                workGroupPack : 1, // multi workgroup packed in one wave
                octReissueMode : 1,
                dualIssueMode : 1,
                reserved : 21;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocConfig() { r.dat = 0; }

    enum cgeStreamMultiprocConfDim
    {
        CG_REG_SM_CONFIG_DIM_ONE = 0,
        CG_REG_SM_CONFIG_DIM_TWO,
        CG_REG_SM_CONFIG_DIM_THREE,
    };

    enum cgeStreamMultiprocConfTraverse
    {
        CG_REG_SM_CONFIG_TRAVERSE_LINEAR = 0,
        CG_REG_SM_CONFIG_TRAVERSE_1D,
        CG_REG_SM_CONFIG_TRAVERSE_2D,
        CG_REG_SM_CONFIG_TRAVERSE_3D,
    };
};

class cgoStreamMultiprocWorkGroupDim
{
public:
    union cguStreamMultiprocWorkGroupDim
    {
        struct cgsStreamMultiprocWorkGroupDim
        {
            U32 groupDimX: 8, 
                groupDimY: 8,
                groupDimZ: 8,
                reserved: 8;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocWorkGroupDim() { r.dat = 0; }
};

class cgoStreamMultiprocWorkGroupSize
{
public:
    union cguStreamMultiprocWorkGroupSize
    {
        struct cgsStreamMultiprocWorkGroupSize
        {
            U32 groupSizeX: 8, 
                groupSizeY: 8,
                groupSizeZ: 8,
                reserved: 8;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocWorkGroupSize() { r.dat = 0; }
};

class cgoStreamMultiprocWorkGroupConfig
{
public:
    union cguStreamMultiprocWorkGroupConfig
    {
        struct cgsStreamMultiprocWorkGroupConfig
        {
            U32 hasBarrier: 1, 
                localRegCount: 10,
                reserved: 21;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocWorkGroupConfig() { r.dat = 0; }
};

class cgoStreamMultiprocStartPc
{
public:
    union cguStreamMultiprocStartPc
    {
        struct cgsStreamMultiprocStartPc
        {
            U32 startPc: 20, 
                reserved: 12;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocStartPc() { r.dat = 0; }
};

class cgoStreamMultiprocEndPc
{
public:
    union cguStreamMultiprocEndPc
    {
        struct cgsStreamMultiprocEndPc
        {
            U32 endPc: 20, 
                reserved: 12;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocEndPc() { r.dat = 0; }
};

class cgoStreamMultiprocShaderCtrl
{
public:
    union cguStreamMultiprocShaderCtrl
    {
        struct cgsStreamMultiprocShaderCtrl
        {
            U32 gprCount : 9,
                lrCount : 9, // local register (storage) count
                reserved : 14;
        } dec;
        U32 dat;
    }r;
    cgoStreamMultiprocShaderCtrl() { r.dat = 0; }
};

class cgoStreamMultiprocReg
{
public:
    cgoStreamMultiprocReg(){};

    cgoStreamMultiprocConfig Config;
    cgoStreamMultiprocStartPc StartPc;
    cgoStreamMultiprocEndPc EndPc;
    cgoStreamMultiprocWorkGroupDim GroupDim;
    cgoStreamMultiprocWorkGroupSize GroupSize;
    cgoStreamMultiprocWorkGroupConfig GroupConf;
    cgoStreamMultiprocShaderCtrl ShaderCtrl;

};
}