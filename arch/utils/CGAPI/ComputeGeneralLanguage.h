#include "GPUReg.h"
#include "zfstream.h"

#include <string>

using namespace std;
using namespace cg1gpu;

namespace cg1gpu
{
struct cgsArchConfig;
class cgoMetaStream;
}

#ifndef GL_VERTEX_PROGRAM_ARB
    #define GL_VERTEX_PROGRAM_ARB 0x8620
#endif

#ifndef GL_FRAGMENT_PROGRAM_ARB
    #define GL_FRAGMENT_PROGRAM_ARB 0x8804
#endif

class CGLInterface
{
public:
    CGLInterface();

    void cglInitialize(bool verbose = false);
    void cglCreateTraceFile(char *traceName);
    void cglCloseTraceFile();

    void cglWriteRegister(GPURegister reg, U32 subreg, bool data);
    void cglWriteRegister(GPURegister reg, U32 subreg, U32 data);
    void cglWriteRegister(GPURegister reg, U32 subreg, S32 data);
    void cglWriteRegister(GPURegister reg, U32 subreg, F32 data);
    void cglWriteRegister(GPURegister reg, U32 subreg, F32 *data);
    void cglWriteRegister(GPURegister reg, U32 subreg, TextureFormat data);

    void cglWriteAddrRegister(GPURegister reg, U32 subreg, U32 data, U32 mdID);

    void cglWriteBuffer(U32 address, U32 size, U08 *data, U32 mdID);

    void cglCompileARBProgram(U32 target, const char *program, U32 size, U08 *code, U32 &codeSize);
    bool cglAssembleKernelFromText(char *program, U08 *code, U32 &codeSize, string &errorString);
    bool cglAssembleKernelFromFile(char *filename, U08 *code, U32 &codeSize, string &errorString);
    void cglLoadVertexShader(U32 address, U32 pc, U32 size, U08 *code, U32 resources, U32 mdID);
    void cglLoadFragmentShader(U32 address, U32 pc, U32 size, U08 *code, U32 resources, U32 mdID);

    void cglResetVertexOutputAttributes();
    void cglResetFragmentInputAttributes();

    void cglSetResolution(U32 w, U32 h);
    void cglSetViewport(S32 x, S32 y, U32 w, U32 h);
    void cglSetScissor(S32 x, S32 y, U32 w, U32 h);

    void cglResetStreamer();
    void cglSetAttributeStream(U32 stream, U32 attr, U32 address, U32 stride, StreamData format, U32 elements);

    void cglMiscReset();
    void cglResetTextures();
    void cglEnableTexture(U32 textureID);
    void cglDisableTexture(U32 textureID);
    void cglSetTexture(U32 texID, TextureMode texMode, U32 address, U32 width, U32 height,
                    U32 widthlog2, U32 heightlog2, TextureFormat texFormat, TextureBlocking texBlockMode,
                    ClampMode texClampS, ClampMode texClampT, FilterMode texMinFilter, FilterMode texMagFilter,
                    bool invertV);

    void cglDraw(PrimitiveMode primitive, U32 start, U32 count);

    void cglSwap();
    void cglReset();

    void cglClearColorBuffer();
    void cglClearZStencilBuffer();
    void cglFlushColorBuffer();
    void cglFlushZStencilBuffer();

    void cglSaveColorState();
    void cglResetColorState();
    void cglRestoreColorState();

    void cglSaveZStencilState();
    void cglResetZStencilState();
    void cglRestoreZStencilState();

private:
    gzofstream outFile;
    cgsArchConfig *arch;
    bool verbose;
    void cglWritecgoMetaStream(cgoMetaStream *metaStreamt);

};
