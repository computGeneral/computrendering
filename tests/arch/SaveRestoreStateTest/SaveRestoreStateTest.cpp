
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

char testVertexProgram2[] =
"!!ARBvp1.0"
""
"CG1_ISA_OPCODE_MOV result.position, vertex.position;"
"CG1_ISA_OPCODE_MOV result.texcoord[0], vertex.attrib[8];"
"END";

char testFragmentProgram[] =
"!!ARBfp1.0"
""
"CG1_ISA_OPCODE_MOV result.color, fragment.color;"
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
        printf("  SaveRestoreStateTest <output file>\n");
        return -1;
    }

    U32 nextBufferID = 0;

    CGLInterface CGLAPI;

    CGLAPI.initialize();

    CGLAPI.createTraceFile(argv[1]);

    CGLAPI.miscReset();
    CGLAPI.resetVertexOutputAttributes();
    CGLAPI.resetFragmentInputAttributes();
    CGLAPI.resetStreamer();
    CGLAPI.resetTextures();

    CGLAPI.setResolution(100, 100);

    U08 vpCode[8192];
    U32 vpCodeSize = 8192;
    CGLAPI.compileARBProgram(GL_VERTEX_PROGRAM_ARB, testVertexProgram, strlen(testVertexProgram), vpCode, vpCodeSize);

    U08 fpCode[8192];
    U32 fpCodeSize = 8192;
    CGLAPI.compileARBProgram(GL_FRAGMENT_PROGRAM_ARB, testFragmentProgram, strlen(testFragmentProgram), fpCode, fpCodeSize);

    CGLAPI.loadVertexProgram(0x00000000, 0, vpCodeSize, vpCode, 1, nextBufferID++);
    CGLAPI.loadFragmentProgram(0x00002000, 0x100, fpCodeSize, fpCode, 1, nextBufferID++);

    CGLAPI.writeGPURegister(GPU_COLOR_BUFFER_FORMAT, 0, GPU_RGBA8888);
    CGLAPI.writeGPURegister(GPU_FRONTBUFFER_ADDR, 0, 0x00400000U);
    CGLAPI.writeGPURegister(GPU_BACKBUFFER_ADDR, 0, 0x00400000U);
    CGLAPI.writeGPURegister(GPU_ZSTENCILBUFFER_ADDR, 0, 0x00C00000U);

    CGLAPI.writeGPURegister(GPU_VERTEX_OUTPUT_ATTRIBUTE, POSITION_ATTRIBUTE, true);
    CGLAPI.writeGPURegister(GPU_VERTEX_OUTPUT_ATTRIBUTE, COLOR_ATTRIBUTE, true);

    CGLAPI.writeGPURegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, POSITION_ATTRIBUTE, true);
    CGLAPI.writeGPURegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, COLOR_ATTRIBUTE, true);

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

    CGLAPI.writeBuffer(0x00004000, sizeof(vertexBuffer), (U08 *) vertexBuffer, nextBufferID++);

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

    CGLAPI.writeBuffer(0x00006000, sizeof(colorBuffer), (U08 *) colorBuffer, nextBufferID++);

    CGLAPI.setAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
    CGLAPI.setAttributeStream(1,    COLOR_ATTRIBUTE, 0x00006000, 12, SD_FLOAT, 3);

    CGLAPI.setViewport(0, 0, 100, 100);

    CGLAPI.writeGPURegister(GPU_INTERPOLATION, COLOR_ATTRIBUTE, true);

    CGLAPI.clearColorBuffer();

    CGLAPI.writeGPURegister(GPU_COLOR_COMPRESSION, 0, false);

    CGLAPI.draw(TRIANGLE, 0, 9);

    CGLAPI.swap();

    //  End of first frame.

    CGLAPI.writeGPURegister(GPU_FRONTBUFFER_ADDR, 0, 0x00800000U);
    CGLAPI.writeGPURegister(GPU_BACKBUFFER_ADDR, 0, 0x00800000U);
    
    CGLAPI.setResolution(512, 512);

    vpCodeSize = 8192;
    CGLAPI.compileARBProgram(GL_VERTEX_PROGRAM_ARB, testVertexProgram2, strlen(testVertexProgram2), vpCode, vpCodeSize);

    fpCodeSize = 8192;
    CGLAPI.compileARBProgram(GL_FRAGMENT_PROGRAM_ARB, testFragmentProgram2, strlen(testFragmentProgram2), fpCode, fpCodeSize);

    CGLAPI.loadVertexProgram(0x00008000, 0, vpCodeSize, vpCode, 1, nextBufferID++);
    CGLAPI.loadFragmentProgram(0x0000A000, 0x100, fpCodeSize, fpCode, 1, nextBufferID++);

    CGLAPI.setViewport(0, 0, 512, 512);

    F32 texBuffer[] =
    {
        0.0f, 0.0f,
        0.0f, 4.0f,
        4.0f, 4.0f,

        4.0f, 4.0f,
        4.0f, 0.0f,
        0.0f, 0.0f
    };

    CGLAPI.writeBuffer(0x0000C000, sizeof(texBuffer), (U08 *) texBuffer, nextBufferID++);

    CGLAPI.resetStreamer();

    CGLAPI.setAttributeStream(0, POSITION_ATTRIBUTE, 0x00004000, 12, SD_FLOAT, 3);
    CGLAPI.setAttributeStream(1,                  8, 0x0000C000,  8, SD_FLOAT, 2);

    CGLAPI.writeGPURegister(GPU_INTERPOLATION, COLOR_ATTRIBUTE, false);
    CGLAPI.writeGPURegister(GPU_INTERPOLATION, 6, true);
    CGLAPI.writeGPURegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, COLOR_ATTRIBUTE, false);
    CGLAPI.writeGPURegister(GPU_FRAGMENT_INPUT_ATTRIBUTES, 6, true);

    CGLAPI.setTexture(0, GPU_TEXTURE2D,  0x00400000, 100, 100, 7, 7, GPU_RGBA8888, GPU_TXBLOCK_FRAMEBUFFER,
               GPU_TEXT_REPEAT, GPU_TEXT_REPEAT, GPU_LINEAR, GPU_LINEAR, true);

    CGLAPI.enableTexture(0);

    CGLAPI.writeGPURegister(GPU_COLOR_COMPRESSION, 0, true);

    CGLAPI.draw(TRIANGLE, 0, 6);

    CGLAPI.swap();

    CGLAPI.writeGPUAddrRegister(GPU_COLOR_STATE_BUFFER_MEM_ADDR, 0, 0x01000000, 0);

    CGLAPI.saveColorState();

    F32 clearColor[] = {0.0f, 1.0f, 0.0f, 0.0f};

    CGLAPI.writeGPURegister(GPU_COLOR_BUFFER_CLEAR, 0, clearColor);

    CGLAPI.clearColorBuffer();
    
    CGLAPI.swap();

    CGLAPI.resetColorState();

    CGLAPI.swap();

    CGLAPI.restoreColorState();

    CGLAPI.swap();
}


