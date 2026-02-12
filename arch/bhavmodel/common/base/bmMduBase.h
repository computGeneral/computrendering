#ifndef __BM_MDUBASE_H__
#define __BM_MDUBASE_H__
#include "GPUType.h"

namespace arch
{
    class bmoTextureProcessor;

    // Class that forms the base of any class that requires MC connection.
    class bmoMduBase
    {
    public:
        bmoMduBase() :
            CurMdlType(CG_BEHV_MODEL),
            bmTextureProcessor(nullptr)
        {
        }

        virtual ~bmoMduBase()
        {
        }


        virtual void SetTP(bmoTextureProcessor* TP) { bmTextureProcessor = TP; }


    protected:
        bmoTextureProcessor* GetTP(void) const { return bmTextureProcessor; }

        bmoTextureProcessor* bmTextureProcessor;
        cgeModelAbstractLevel CurMdlType; // Current Caller's Model Type
    };

}
#endif // __BM_MDUBASE_H__
