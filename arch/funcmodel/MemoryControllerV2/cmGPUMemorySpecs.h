/**************************************************************************
 *
 */

#ifndef GPUMEMORYSPECS_H
    #define GPUMEMORYSPECS_H

#include "GPUType.h"

namespace cg1gpu
{
namespace memorycontroller
{

/**
 * All GPU memories supported by MCv2 must inherit from this GPU memory spec abstract class
 *
 * Ty�cal Usage
 *
 * GPUMemorySpecs* ms = obtaintFromX();
 * 
 * switch ( ms->memtype() ) {
 *     case GPUMemorySpecs::GDDR3:
 *         GDDR3Specs* gddr3specs = static_cast<GDDR3Specs*>(ms);
 *         // get GDDR3 params
 *         ...
 *     case GPUMemorySpecs::GDDR5:
 *         ...
 * }
 */
class GPUMemorySpecs
{
public:

    /**
     * Memories (protocols) supported by MCv2
     */
    enum GDDR_TYPE
    {
        GDDR3,
        GDDR4,
        GDDR5
    };

    virtual GDDR_TYPE memtype() const = 0;
    virtual const char* memstr() const = 0;

};


/**
 * Parameters supported by GDDR3 memory
 */
struct GDDR3Specs : public GPUMemorySpecs
{

    U32 tRRD;
    U32 tRCD;
    U32 tWTR;
    U32 tRTW;
    U32 tWR;
    U32 tRP;
    U32 CASLatency;
    U32 WriteLatency;

    GDDR_TYPE memtype() const { return GDDR3; }
    const char* memstr() const { return "GDDR3"; }

};

/**
 * Define your own GDDR3 specifications
 */
struct GDDR3SpecsCustom : public GDDR3Specs
{
    GDDR3SpecsCustom( U32 tRRD_,
                      U32 tRCD_,
                      U32 tWTR_,
                      U32 tRTW_,
                      U32 tWR_,
                      U32 tRP_,
                      U32 CASLatency_,
                      U32 WriteLatency_ )
    {
        tRRD = tRRD_;
        tRCD = tRCD_;
        tWTR = tWTR_;
        tRTW = tRTW_;
        tWR = tWR_;
        tRP = tRP_;
        CASLatency = CASLatency_;
        WriteLatency = WriteLatency_;
    }      
};

/**
 * Simulated model based on GDDR3 Hynix specification (HY5RS123235FP - Rev 1.3 / Feb 2006 )
 * Column used: speed = -16 (Aprox 600 Mhz)
 */
struct GDDR3Specs600MHz : public GDDR3Specs
{
    GDDR3Specs600MHz() 
    {
        // Based on GDDR3 Hynix specification (HY5RS123235FP - Rev 1.3 / Feb 2006 )
        // Column used: speed = -16 (Aprox 600 Mhz)
        tRRD = 9;
        tRCD = 13;
        tWTR = 5;
        tRTW = 2; 
        tWR = 10;
        tRP = 14;
        CASLatency = 10; 
        WriteLatency = 5;
    }
};

/**
 * GDDR3 without timing constraints
 */
struct GDDR3SpecsZeroDelay : public GDDR3Specs
{
    GDDR3SpecsZeroDelay() 
    {
        tRRD = 0;
        tRCD = 0;
        tWTR = 0;
        tRTW = 0; 
        tWR  = 0;
        tRP  = 0;
        CASLatency   = 0; 
        WriteLatency = 0;
    }
};

} // namespace memorycontroller
} // namespace cg1gpu


#endif // GPUMEMORYSPECS_H
