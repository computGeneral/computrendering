#ifndef GPUPROXY_H
#define GPUPROXY_H

enum GPUProxyRegisterType {
        RT_U32BIT,
        RT_S32BIT,
        RT_F32BIT,
        RT_U8BIT,
        RT_BOOL,
        RT_QUADFLOAT,
        RT_GPU_STATUS,
        RT_FACE_MODE,
        RT_CULLING_MODE,
        RT_PRIMITIVE_MODE,
        RT_STREAM_DATA,
        RT_COMPARE_MODE,
        RT_STENCIL_UPDATE_FUNCTION,
        RT_BLEND_EQUATION,
        RT_BLEND_FUNCTION,
        RT_LOGIC_OP_MODE,
        RT_TEXTURE_MODE,
        RT_TEXTURE_FORMAT,
        RT_TEXTURE_COMPRESSION,
        RT_TEXTURE_BLOCKING,
        RT_CLAMP_MODE,
        RT_FILTER_MODE,
        RT_ADDRESS
};

struct GPUProxyRegId {
    cg1gpu::GPURegister gpu_reg;
    U32 index;
    bool operator()(const GPUProxyRegId &a, const GPUProxyRegId &b) const;
    friend bool operator<(const GPUProxyRegId &a, const GPUProxyRegId &b)
    {
        if((unsigned int)(a.gpu_reg) < (unsigned int)(b.gpu_reg))
            return true;
        else if((unsigned int)(a.gpu_reg) > (unsigned int)(b.gpu_reg))
            return false;
        else {
            return (a.index < b.index);
        }
    }

    friend bool operator==(const GPUProxyRegId &a, const GPUProxyRegId &b)
    {
        return ((a.gpu_reg == b.gpu_reg) && (a.index == b.index));
    }

    GPUProxyRegId();
    GPUProxyRegId(cg1gpu::GPURegister _gpu_reg, U32 _index = 0);
};

struct GPUProxyAddress {
    U32 md;
    U32 offset;
    GPUProxyAddress();
    GPUProxyAddress(U32 md, U32 offset = 0);
};


/** @note You must read/write address registers with read/writeGPUAddrRegister except for setting them to 0.
		  Simetrically address registers must be read/written with read/writeGPURegister.
		  This ensures the values cached for reading registers are consistent */
class GPUProxy {
    public:
        void getGPUParameters(U32& gpuMemSz, U32& sysMemSz, U32& blocksz, U32& sblocksz,
        U32& scanWidth, U32& scanHeight, U32& overScanWidth, U32& overScanHeight,
        bool& doubleBuffer, U32& fetchRate) const;
        void writeGPURegister( cg1gpu::GPURegister regId, cg1gpu::GPURegData data);
        void writeGPURegister( cg1gpu::GPURegister regId, U32 index, cg1gpu::GPURegData data);
        void writeGPUAddrRegister( cg1gpu::GPURegister regId, U32 index, U32 md, U32 offset = 0);
        void readGPURegister( cg1gpu::GPURegister regId, cg1gpu::GPURegData* data);
        void readGPURegister( cg1gpu::GPURegister regId, U32 index, cg1gpu::GPURegData* data);
        void readGPUAddrRegister( cg1gpu::GPURegister regId, U32 index, U32* md, U32* offset);
        void sendCommand(cg1gpu::GPUCommand com);
        void initBuffers(U32* mdFront = 0, U32* mdBack = 0, U32* mdZS = 0);
        void setResolution(U32 width, U32 height);
        /**@todo Add parameter MemoryRequestPolicy memRequestPolicy = GPUMemoryFirst,
                 now is ommited because it gives a build error. */
        U32 obtainMemory( U32 sizeBytes);
        void releaseMemory( U32 md );
        bool writeMemory( U32 md, const U08* data, U32 dataSize, bool isLocked = false);
        bool writeMemory( U32 md, U32 offset, const U08* data, U32 dataSize, bool isLocked = false );

        bool commitVertexProgram( U32 memDesc, U32 programSize, U32 startPC );
        bool commitFragmentProgram( U32 memDesc, U32 programSize, U32 startPC );

        void debug_print_registers();

        static GPUProxy* get_instance();
    private:
        map<GPUProxyRegId, GPURegData, GPUProxyRegId> values;
        map<GPUProxyRegId, GPUProxyAddress, GPUProxyRegId> addresses;
        void debug_print_register(cg1gpu::GPURegister reg, U32 index = 0);

        map<GPUProxyRegId, GPUProxyRegisterType, GPUProxyRegId> register_types;

        map<cg1gpu::GPURegister, string> register_names;

        GPUProxy();
        ~GPUProxy();
};



#endif // GPUPROXY_H
