#include "ComputeGeneralLanguage.h"

#include <iostream>
#include <string.h>

using namespace std;

char testVertexProgram[] =
"!!ARBvp1.0"
""
"CG1_ISA_OPCODE_MOV result.position, vertex.position;"
"CG1_ISA_OPCODE_MOV result.color, vertex.attrib[1];"
"END";

char testFragmentProgram[] =
"!!ARBfp1.0"
""
"CG1_ISA_OPCODE_MOV result.color, fragment.color;"
"END";

char testVertexProgram2[] =
"!!ARBvp1.0"
""
"CG1_ISA_OPCODE_MOV result.position, vertex.position;"
"CG1_ISA_OPCODE_MOV result.texcoord[0], vertex.attrib[8];"
"END";

char testFragmentProgram2[] =
"!!ARBfp1.0"
""
"TEX result.color, fragment.texcoord[0], texture[0], 2D;"
"END";


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage : \n");
        printf("  CGLTest.exe <output metaStream trace file>\n");
        return -1;        
    }
    
    CGLInterface CGLAPI;
    U32          nextBufferID = 0;

    CGLAPI.cglInitialize();
    CGLAPI.cglCreateTraceFile(argv[1]);
    CGLAPI.cglMiscReset();
    CGLAPI.cglResetVertexOutputAttributes();
    CGLAPI.cglResetFragmentInputAttributes();
    CGLAPI.cglResetStreamer();
    CGLAPI.cglResetTextures();
    CGLAPI.cglSetResolution(100, 100);

    U08 vpCode[8192];
    U32 vpCodeSize = 8192;
    CGLAPI.cglCompileARBProgram(GL_VERTEX_PROGRAM_ARB, testVertexProgram, strlen(testVertexProgram), vpCode, vpCodeSize);

    U08 fpCode[8192];
    U32 fpCodeSize = 8192;
    CGLAPI.cglCompileARBProgram(GL_FRAGMENT_PROGRAM_ARB, testFragmentProgram, strlen(testFragmentProgram), fpCode, fpCodeSize);

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
    CGLAPI.cglSetAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
    CGLAPI.cglSetAttributeStream(1,    COLOR_ATTRIBUTE, 0x00006000, 12, SD_FLOAT, 3);
    CGLAPI.cglSetViewport(0, 0, 100, 100);
    CGLAPI.cglWriteRegister(GPU_INTERPOLATION, COLOR_ATTRIBUTE, true);
    CGLAPI.cglClearColorBuffer();
    CGLAPI.cglWriteRegister(GPU_COLOR_COMPRESSION, 0, false);
    CGLAPI.cglDraw(TRIANGLE, 0, 9);
    CGLAPI.cglSwap();

    //  End of first frame.
    CGLAPI.cglSetResolution(512, 512);
    vpCodeSize = 8192;
    CGLAPI.cglCompileARBProgram(GL_VERTEX_PROGRAM_ARB, testVertexProgram2, strlen(testVertexProgram2), vpCode, vpCodeSize);
    fpCodeSize = 8192;
    CGLAPI.cglCompileARBProgram(GL_FRAGMENT_PROGRAM_ARB, testFragmentProgram2, strlen(testFragmentProgram2), fpCode, fpCodeSize);
    CGLAPI.cglLoadVertexShader(0x00008000, 0, vpCodeSize, vpCode, 1, nextBufferID++);
    CGLAPI.cglLoadFragmentShader(0x0000A000, 0, fpCodeSize, fpCode, 1, nextBufferID++);
    CGLAPI.cglSetViewport(0, 0, 512, 512);

    F32 texBuffer[] =
    {
        0.0f, 0.0f,
        0.0f, 4.0f,
        4.0f, 4.0f,

        4.0f, 4.0f,
        4.0f, 0.0f,
        0.0f, 0.0f
    };
    CGLAPI.cglWriteBuffer(0x0000C000, sizeof(texBuffer), (U08 *) texBuffer, nextBufferID++);
    CGLAPI.cglResetStreamer();
    CGLAPI.cglSetAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
    CGLAPI.cglSetAttributeStream(1,                  8, 0x0000C000,  8, SD_FLOAT, 2);
    CGLAPI.cglWriteRegister(GPU_INTERPOLATION, COLOR_ATTRIBUTE, false);
    CGLAPI.cglWriteRegister(GPU_INTERPOLATION, 6, true);
    CGLAPI.cglWriteRegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, COLOR_ATTRIBUTE, false);
    CGLAPI.cglWriteRegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, 6, true);
    CGLAPI.cglSetTexture(0, GPU_TEXTURE2D,  0x00800000, 100, 100, 7, 7, GPU_RGBA8888, GPU_TXBLOCK_FRAMEBUFFER,
                         GPU_TEXT_REPEAT, GPU_TEXT_REPEAT, GPU_LINEAR, GPU_LINEAR, true);
    CGLAPI.cglEnableTexture(0);
    CGLAPI.cglWriteRegister(GPU_COLOR_COMPRESSION, 0, true);
    CGLAPI.cglDraw(TRIANGLE, 0, 6);
    CGLAPI.cglSwap();
    CGLAPI.cglWriteAddrRegister(GPU_COLOR_STATE_BUFFER_MEM_ADDR, 0, 0x01000000, 0);
    CGLAPI.cglSaveColorState();
    F32 clearColor[] = {0.0f, 1.0f, 0.0f, 0.0f};
    CGLAPI.cglWriteRegister(GPU_COLOR_BUFFER_CLEAR, 0, clearColor);
    CGLAPI.cglClearColorBuffer();
    CGLAPI.cglSwap();
    CGLAPI.cglRestoreColorState();
    CGLAPI.cglSwap();
}


