
#include "ComputeGeneralLanguage.h"

#include <iostream>
#include <string.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage : \n");
        printf("  isa_test_lda <output MetaStream Trace file>\n");
        return -1;
    }
    U32          nextBufferID = 0;
    CGLInterface CGLAPI;

    CGLAPI.cglInitialize();
    CGLAPI.cglCreateTraceFile(argv[1]);
    CGLAPI.cglMiscReset();
    CGLAPI.cglResetVertexOutputAttributes();
    CGLAPI.cglResetFragmentInputAttributes();
    CGLAPI.cglResetStreamer();
    CGLAPI.cglResetTextures();

    CGLAPI.cglSetResolution(100, 100);
    CGLAPI.cglSetViewport(0, 0, 100, 100);

    string errorString;
    bool result;
    
    U08 vpCode[8192];
    U32 vpCodeSize = 8192;
    
    result = CGLAPI.cglAssembleKernelFromFile("vprogram.asm", vpCode, vpCodeSize, errorString);
    
    if (!result)
    {
        printf("Error assembling vertex program.\n");
        printf("%s\n", errorString.c_str());
        return -1;
    }
    
    U08 fpCode[8192];
    U32 fpCodeSize = 8192;

    result = CGLAPI.cglAssembleKernelFromFile("fprogram.asm", fpCode, fpCodeSize, errorString);
    
    if (!result)
    {
        printf("Error assembling fragment program.\n");
        printf("%s\n", errorString.c_str());
        return -1;
    }

    CGLAPI.cglLoadVertexShader(0x00000000, 0, vpCodeSize, vpCode, 1, nextBufferID++);
    CGLAPI.cglLoadFragmentShader(0x00002000, 0, fpCodeSize, fpCode, 1, nextBufferID++);

    CGLAPI.cglWriteRegister(GPU_COLOR_BUFFER_FORMAT, 0, GPU_RGBA8888);
    CGLAPI.cglWriteRegister(GPU_FRONTBUFFER_ADDR, 0, 0x00400000U);
    CGLAPI.cglWriteRegister(GPU_BACKBUFFER_ADDR, 0, 0x00800000U);
    CGLAPI.cglWriteRegister(GPU_ZSTENCILBUFFER_ADDR, 0, 0x00C00000U);

    CGLAPI.cglWriteRegister(GPU_VERTEX_OUTPUT_ATTRIBUTE, POSITION_ATTRIBUTE, true);
    CGLAPI.cglWriteRegister(GPU_VERTEX_OUTPUT_ATTRIBUTE, COLOR_ATTRIBUTE, true);

    CGLAPI.cglWriteRegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, POSITION_ATTRIBUTE, true);
    CGLAPI.cglWriteRegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, COLOR_ATTRIBUTE, true);

    F32 vertexBuffer[] =
    {
        -1.0f, -1.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f,  1.0f, 0.1f,

         1.0f,  1.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
        -1.0f, -1.0f, 0.1f,

        -0.5f, -0.5f, 0.1f,
         0.5f, -0.5f, 0.1f,
         0.0f,  0.5f, 0.1f
    };

    CGLAPI.cglWriteBuffer(0x00004000, sizeof(vertexBuffer), (U08 *) vertexBuffer, nextBufferID++);

    F32 colorBuffer[] =
    {
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };

    CGLAPI.cglWriteBuffer(0x00006000, sizeof(colorBuffer), (U08 *) colorBuffer, nextBufferID++);

    F32 indexBuffer[16];
    for(U32 i = 0; i < 16; i++)
        ((U32 *) indexBuffer)[i] = i;
        
    CGLAPI.cglWriteBuffer(0x00008000, sizeof(indexBuffer), (U08 *) indexBuffer, nextBufferID++);            
    
    CGLAPI.cglSetAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
    CGLAPI.cglSetAttributeStream(1,    COLOR_ATTRIBUTE, 0x00006000, 12, SD_FLOAT, 3);
    CGLAPI.cglSetAttributeStream(2,                  2, 0x00008000,  4, SD_FLOAT, 1);
    CGLAPI.cglWriteRegister(GPU_INTERPOLATION, COLOR_ATTRIBUTE, true);
    CGLAPI.cglClearColorBuffer();
    CGLAPI.cglDraw(TRIANGLE, 0, 9);
    CGLAPI.cglSwap();
    
//  Second frame.  Test attribute load bypass mode.

//    CGLAPI.cglClearColorBuffer();
//    vpCodeSize = 8192;
//    result = CGLAPI.cglAssembleKernelFromFile("vprogram2.asm", vpCode, vpCodeSize, errorString);
//    if (!result)
//    {
//        printf("Error assembling vertex program.\n");
//        printf("%s\n", errorString.c_str());
//        return -1;
//    }
//
//    CGLAPI.cglLoadVertexShader(0x00000000, 0, vpCodeSize, vpCode, 1, nextBufferID++);
//    CGLAPI.cglResetStreamer();
//    CGLAPI.cglSetAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
//    CGLAPI.cglSetAttributeStream(1,    COLOR_ATTRIBUTE, 0x00006000, 12, SD_FLOAT, 3);
//    CGLAPI.cglWriteRegister(GPU_ATTRIBUTE_LOAD_BYPASS, 0, true);
//    CGLAPI.cglDraw(TRIANGLE, 0, 9);
//    CGLAPI.cglSwap();
}


