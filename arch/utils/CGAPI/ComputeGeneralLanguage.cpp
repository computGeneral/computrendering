
#include "GPULibInternals.h"
#include "DynamicMemoryOpt.h"
#include "ProgramManager.h"
#include "ProgramObject.h"
#include "glext.h"

#include "ComputeGeneralLanguage.h"

#include "HAL.h"
#include "ConfigLoader.h"
#include "MetaStreamTrace.h"
#include "ShaderInstruction.h"

#include "ShaderArchParam.h"

#include <fstream>
#include <sstream>
#include <vector>

using namespace libgl;

CGLInterface::CGLInterface()
{
}

void CGLInterface::cglInitialize(bool verboseExec)
{
    DynamicMemoryOpt::initialize(512, 1024, 1024, 1024, 4096, 1024);
    ConfigLoader* cl;
    char* configFile = "../../arch/config/CG1GPU.ini";
    cl = new ConfigLoader(configFile);
    arch = new cgsArchConfig;
    cl->getParameters(arch);
    delete cl;
    //  Check if the vector alu configuration is scalar (Scalar).
    string aluConf(arch->ush.vectorALUConfig);    
    bool vectorScalarALU = arch->ush.useVectorShader && (aluConf.compare("scalar") == 0);

    U32 instructionsToSchedulePerCycle = 0;
    
    //  Compute the number of shader instructions to schedule per cycle that is passed to the driver.
    //  Only the simd4+scalar architecture requires scheduling more than 1 instruction per cycle.
    instructionsToSchedulePerCycle = (arch->ush.useVectorShader) ? (aluConf.compare("simd4+scalar") == 0) ? 2 : 1
                                                                 : arch->ush.fetchRate;
    //  Use the 'FetchRate' (instructions fetched per cycle) parameter for the old shader model.
    
    //  Check configuration parameters.
    CG_ASSERT_COND(!(arch->ush.vAttrLoadFromShader && !arch->enableDriverShTrans), 
                 "Vertex attribute load from shader requires driver shader program translation to be enabled.");
    CG_ASSERT_COND(!(arch->ush.useVectorShader && vectorScalarALU && !arch->enableDriverShTrans), 
                 "Vector Shader Scalar architecture requires driver shader program translation to be enabled.");

    HAL::getHAL()->setGPUParameters(arch->mem.memSize * 1024,
                                    arch->mem.mappedMemSize * 1024,
                                    arch->ush.texBlockDim,
                                    arch->ush.texSuperBlockDim,
                                    arch->ras.scanWidth,
                                    arch->ras.scanHeight,
                                    arch->ras.overScanWidth,
                                    arch->ras.overScanHeight,
                                    arch->doubleBuffer,
                                    arch->forceMSAA,
                                    arch->msaaSamples,
                                    arch->forceFP16ColorBuffer,
                                    instructionsToSchedulePerCycle,
                                    arch->mem.memoryControllerV2,
                                    arch->mem.v2SecondInterleaving,
                                    arch->ush.vAttrLoadFromShader,
                                    vectorScalarALU,
                                    //arch->enableDriverShTrans  // Disabled when creating/processing MetaStreamTrans traces
                                    false,
                                    //(arch.ras.useMicroPolRast && arch.ras.microTrisAsFragments) // Disabled when creating/processing MetaStreamTrans traces
                                    false );
    //  Set the shader architecture to use.
    string shaderArch = (arch->ush.fixedLatencyALU) ?
                        (arch->ush.useVectorShader && vectorScalarALU) ? "ScalarFixLat" : "SIMD4FixLat" :
                        (arch->ush.useVectorShader && vectorScalarALU) ? "ScalarVarLat" : "SIMD4VarLat";
    ShaderArchParam::getShaderArchParam()->selectArchitecture(shaderArch);          

    //legacy code initLibOGL14(HAL::getHAL(), arch->ras.shadedSetup);

    verbose = verboseExec;
    
    /*TraceDriver = new TraceDriverOGL(arch->inputFile,
                                arch->useGAL, // False => Legacy, True => Uses GAL new implementation
                                HAL::getHAL(),
                                arch->ras.shadedSetup,
                                arch->startFrame);*/
}

void CGLInterface::cglCreateTraceFile(char *traceName)
{
    outFile.open(traceName, ios::out | ios::binary);    //  Open output file
    CG_ASSERT_COND((outFile.is_open()), "Error opening output MetaStream trace file.");    //  Check if output file was correctly created.
    MetaStreamHeader metaTraceHeader;    //  Create header for the MetaStream Trace file.
    //  Write file type stamp
    for(U32 i = 0; i < sizeof(MetaStreamTRACEFILE_SIGNATURE); i++)
        metaTraceHeader.signature[i] = MetaStreamTRACEFILE_SIGNATURE[i];

    metaTraceHeader.version = MetaStreamTRACEFILE_VERSION;
    metaTraceHeader.parameters.startFrame         = arch->startFrame;
    metaTraceHeader.parameters.traceFrames        = arch->simFrames;
    metaTraceHeader.parameters.memSize            = arch->mem.memSize;
    metaTraceHeader.parameters.mappedMemSize      = arch->mem.mappedMemSize;
    metaTraceHeader.parameters.texBlockDim        = arch->ush.texBlockDim;
    metaTraceHeader.parameters.texSuperBlockDim   = arch->ush.texSuperBlockDim;
    metaTraceHeader.parameters.scanWidth          = arch->ras.scanWidth;
    metaTraceHeader.parameters.scanHeight         = arch->ras.scanHeight;
    metaTraceHeader.parameters.overScanWidth      = arch->ras.overScanWidth;
    metaTraceHeader.parameters.overScanHeight     = arch->ras.overScanHeight;
    metaTraceHeader.parameters.doubleBuffer       = arch->doubleBuffer;
    metaTraceHeader.parameters.fetchRate          = arch->ush.fetchRate;
    metaTraceHeader.parameters.memoryControllerV2 = arch->mem.memoryControllerV2;
    metaTraceHeader.parameters.v2SecondInterleaving = arch->mem.v2SecondInterleaving;
    //  Write header (configuration parameters related with the OpenGL to CG1 MetaStream commands translation).
    outFile.write((char *) &metaTraceHeader, sizeof(metaTraceHeader));
}

void CGLInterface::cglCloseTraceFile()
{
    outFile.close();    //  Close output file.
}


void CGLInterface::cglWritecgoMetaStream(cgoMetaStream *metaStream)
{
    if (verbose)
        metaStream->dump();
    metaStream->save(&outFile);
    delete metaStream;
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, U32 data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.uintVal = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, S32 data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.intVal = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, F32 data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.f32Val = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, bool data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.booleanVal = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, F32 *data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.qfVal[0] = data[0];
    regData.qfVal[1] = data[1];
    regData.qfVal[2] = data[2];
    regData.qfVal[3] = data[3];
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteRegister(GPURegister reg, U32 subReg, TextureFormat data)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.txFormat = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteAddrRegister(GPURegister reg, U32 subReg, U32 data, U32 mdID)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.uintVal = data;
    metaStream = new cgoMetaStream(reg, subReg, regData, mdID);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglWriteBuffer(U32 address, U32 size, U08 *data, U32 mdID)
{
    cgoMetaStream *metaStream;

    metaStream = new cgoMetaStream(address, size, data, mdID, true, true);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglCompileARBProgram(U32 target, const char *program, U32 size, U08 *code, U32 &codeSize)
{
    ProgramManager &pm = ProgramManager::instance();

    U32 objID = (target == GL_VERTEX_PROGRAM_ARB) ? 1 : 2;

    ProgramObject &po = dynamic_cast<ProgramObject&> (pm.bindObject(target, objID));

    po.setSource(program, size);
    po.setFormat(GL_PROGRAM_FORMAT_ASCII_ARB);
    po.compile();

    GLsizei codeSz = codeSize;

    po.getBinary(code, codeSz);

    codeSize = codeSz;

    //char disassem[32*1024];
    //GLsizei disassemsize = 32*1024;
    //po.getASMCode((GLubyte *) disassem, disassemsize);
    //cout << "----------" << endl;
    //cout << "Disassembled code : " << endl;
    //cout << disassem << endl;
}

void CGLInterface::cglLoadVertexShader(U32 address, U32 pc, U32 size, U08 *code, U32 resources, U32 mdID)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    metaStream = new cgoMetaStream(address, size, code, mdID, true, true);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = address;
    metaStream = new cgoMetaStream(GPU_VERTEX_PROGRAM, 0, regData, 1);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = pc;
    metaStream = new cgoMetaStream(GPU_VERTEX_PROGRAM_PC, 0, regData, 1);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = size;
    metaStream = new cgoMetaStream(GPU_VERTEX_PROGRAM_SIZE, 0, regData, 1);
    cglWritecgoMetaStream(metaStream);

    metaStream = new cgoMetaStream(GPU_LOAD_VERTEX_PROGRAM);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = resources;
    metaStream = new cgoMetaStream(GPU_VERTEX_THREAD_RESOURCES, 0, regData, 1);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglLoadFragmentShader(U32 address, U32 pc, U32 size, U08 *code, U32 resources, U32 mdID)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    metaStream = new cgoMetaStream(address, size, code, mdID, true, true);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = address;
    metaStream = new cgoMetaStream(GPU_FRAGMENT_PROGRAM, 0, regData, 1);

    cglWritecgoMetaStream(metaStream);

    regData.uintVal = pc;
    metaStream = new cgoMetaStream(GPU_FRAGMENT_PROGRAM_PC, 0, regData, 1);

    cglWritecgoMetaStream(metaStream);

    regData.uintVal = size;
    metaStream = new cgoMetaStream(GPU_FRAGMENT_PROGRAM_SIZE, 0, regData, 1);

    cglWritecgoMetaStream(metaStream);

    metaStream = new cgoMetaStream(GPU_LOAD_FRAGMENT_PROGRAM);

    cglWritecgoMetaStream(metaStream);

    regData.uintVal = resources;
    metaStream = new cgoMetaStream(GPU_FRAGMENT_THREAD_RESOURCES, 0, regData, 1);

    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSetResolution(U32 w, U32 h)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.uintVal = w;
    metaStream = new cgoMetaStream(GPU_DISPLAY_X_RES, 0, regData, 0);

    cglWritecgoMetaStream(metaStream);

    regData.uintVal = h;
    metaStream = new cgoMetaStream(GPU_DISPLAY_Y_RES, 0, regData, 0);

    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglResetVertexOutputAttributes()
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    for(U32 i = 0; i < MAX_VERTEX_ATTRIBUTES; i++)
    {
        regData.booleanVal = false;
        metaStream = new cgoMetaStream(GPU_VERTEX_OUTPUT_ATTRIBUTE, i, regData, 0);
        cglWritecgoMetaStream(metaStream);
    }
}

void CGLInterface::cglResetFragmentInputAttributes()
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    for(U32 i = 0; i < MAX_FRAGMENT_ATTRIBUTES; i++)
    {
        regData.booleanVal = false;
        metaStream = new cgoMetaStream(GPU_FRAGMENT_INPUT_ATTRIBUTES, i, regData, 0);
        cglWritecgoMetaStream(metaStream);
    }

}

void CGLInterface::cglResetStreamer()
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    for(U32 i = 0; i < MAX_VERTEX_ATTRIBUTES; i++)
    {
        regData.uintVal = ST_INACTIVE_ATTRIBUTE;
        metaStream = new cgoMetaStream(GPU_VERTEX_ATTRIBUTE_MAP, i, regData, 0);
        cglWritecgoMetaStream(metaStream);

        regData.qfVal[0] = 0.0f;
        regData.qfVal[1] = 0.0f;
        regData.qfVal[2] = 0.0f;
        if ((i == COLOR_ATTRIBUTE) || (i == POSITION_ATTRIBUTE))
            regData.qfVal[3] = 1.0f;
        else
            regData.qfVal[3] = 0.0f;
        metaStream = new cgoMetaStream(GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE, i, regData, 0);
        cglWritecgoMetaStream(metaStream);
    }

    for(U32 i = 0; i < MAX_STREAM_BUFFERS; i++)
    {
        regData.uintVal = 0;
        metaStream = new cgoMetaStream(GPU_STREAM_ADDRESS, i, regData, 0);
        cglWritecgoMetaStream(metaStream);

        regData.uintVal = 0;
        metaStream = new cgoMetaStream(GPU_STREAM_STRIDE, i, regData, 0);
        cglWritecgoMetaStream(metaStream);

        regData.streamData = SD_FLOAT;
        metaStream = new cgoMetaStream(GPU_STREAM_DATA, i, regData, 0);
        cglWritecgoMetaStream(metaStream);

        regData.uintVal = 0;
        metaStream = new cgoMetaStream(GPU_STREAM_ELEMENTS, i, regData, 0);
        cglWritecgoMetaStream(metaStream);

        regData.booleanVal = 0;
        metaStream = new cgoMetaStream(GPU_D3D9_COLOR_STREAM, i, regData, 0);
        cglWritecgoMetaStream(metaStream);
    }

    regData.uintVal = 0;
    metaStream = new cgoMetaStream(GPU_STREAM_START, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = 0;
    metaStream = new cgoMetaStream(GPU_STREAM_COUNT, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_INDEX_MODE, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = 0;
    metaStream = new cgoMetaStream(GPU_INDEX_STREAM, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSetAttributeStream(U32 stream, U32 attr, U32 address, U32 stride, StreamData format, U32 elements)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.uintVal = stream;
    metaStream = new cgoMetaStream(GPU_VERTEX_ATTRIBUTE_MAP, attr, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = address;
    metaStream = new cgoMetaStream(GPU_STREAM_ADDRESS, stream, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = stride;
    metaStream = new cgoMetaStream(GPU_STREAM_STRIDE, stream, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.streamData = format;
    metaStream = new cgoMetaStream(GPU_STREAM_DATA, stream, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = elements;
    metaStream = new cgoMetaStream(GPU_STREAM_ELEMENTS, stream, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSetViewport(S32 x, S32 y, U32 w, U32 h)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.intVal = x;
    metaStream = new cgoMetaStream(GPU_VIEWPORT_INI_X, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.intVal = y;
    metaStream = new cgoMetaStream(GPU_VIEWPORT_INI_Y, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = w;
    metaStream = new cgoMetaStream(GPU_VIEWPORT_WIDTH, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = h;
    metaStream = new cgoMetaStream(GPU_VIEWPORT_HEIGHT, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSetScissor(S32 x, S32 y, U32 w, U32 h)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    cglWriteRegister(GPU_PROGRAM_MEM_ADDR, 0, 0);
    cglWriteRegister(GPU_TEXTURE_MEM_ADDR, 0, 0);

    regData.intVal = x;
    metaStream = new cgoMetaStream(GPU_SCISSOR_INI_X, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.intVal = y;
    metaStream = new cgoMetaStream(GPU_SCISSOR_INI_Y, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = w;
    metaStream = new cgoMetaStream(GPU_SCISSOR_WIDTH, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = h;
    metaStream = new cgoMetaStream(GPU_SCISSOR_HEIGHT, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglMiscReset()
{
    cgoMetaStream* metaStream;
    GPURegData      regData;

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_FRUSTUM_CLIPPING, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.faceMode = GPU_CW;
    metaStream = new cgoMetaStream(GPU_FACEMODE, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.culling = NONE;
    metaStream = new cgoMetaStream(GPU_CULLING, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_HIERARCHICALZ, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_EARLYZ, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_SCISSOR_TEST, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.f32Val = 0.0f;
    metaStream = new cgoMetaStream(GPU_DEPTH_RANGE_NEAR, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.f32Val = 0.0f;
    metaStream = new cgoMetaStream(GPU_DEPTH_RANGE_FAR, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_TWOSIDED_LIGHTING, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_MULTISAMPLING, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_MODIFY_FRAGMENT_DEPTH, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_DEPTH_TEST, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_STENCIL_TEST, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_COLOR_MASK_R, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_COLOR_MASK_G, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_COLOR_MASK_B, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_COLOR_MASK_A, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_COLOR_BLEND, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_LOGICAL_OPERATION, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = 0;
    metaStream = new cgoMetaStream(GPU_MCV2_2ND_INTERLEAVING_START_ADDR, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglResetTextures()
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    for(U32 i = 0; i < MAX_TEXTURES; i++)
    {
        regData.booleanVal = false;
        metaStream = new cgoMetaStream(GPU_TEXTURE_ENABLE, i, regData, 0);
        cglWritecgoMetaStream(metaStream);
    }
}

void CGLInterface::cglEnableTexture(U32 textureID)
{
    GPURegData regData;
    cgoMetaStream *metaStream;

    regData.booleanVal = true;
    metaStream = new cgoMetaStream(GPU_TEXTURE_ENABLE, textureID, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglDisableTexture(U32 textureID)
{
    GPURegData regData;
    cgoMetaStream *metaStream;

    regData.booleanVal = false;
    metaStream = new cgoMetaStream(GPU_TEXTURE_ENABLE, textureID, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSetTexture(U32 texID, TextureMode texMode, U32 address, U32 width, U32 height,
                U32 widthlog2, U32 heightlog2, TextureFormat texFormat, TextureBlocking texBlockMode,
                ClampMode texClampS, ClampMode texClampT, FilterMode texMinFilter, FilterMode texMagFilter,
                bool invertV)

{
    GPURegData regData;
    cgoMetaStream *metaStream;

    regData.txMode = texMode;
    metaStream = new cgoMetaStream(GPU_TEXTURE_MODE, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = address;
    metaStream = new cgoMetaStream(GPU_TEXTURE_ADDRESS, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = width;
    metaStream = new cgoMetaStream(GPU_TEXTURE_WIDTH, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = height;
    metaStream = new cgoMetaStream(GPU_TEXTURE_HEIGHT, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = widthlog2;
    metaStream = new cgoMetaStream(GPU_TEXTURE_WIDTH2, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = heightlog2;
    metaStream = new cgoMetaStream(GPU_TEXTURE_HEIGHT2, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txFormat = texFormat;
    metaStream = new cgoMetaStream(GPU_TEXTURE_FORMAT, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txBlocking = texBlockMode;
    metaStream = new cgoMetaStream(GPU_TEXTURE_BLOCKING, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txClamp = texClampS;
    metaStream = new cgoMetaStream(GPU_TEXTURE_WRAP_S, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txClamp = texClampT;
    metaStream = new cgoMetaStream(GPU_TEXTURE_WRAP_T, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txFilter = texMinFilter;
    metaStream = new cgoMetaStream(GPU_TEXTURE_MIN_FILTER, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.txFilter = texMagFilter;
    metaStream = new cgoMetaStream(GPU_TEXTURE_MAG_FILTER, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.booleanVal = invertV;
    metaStream = new cgoMetaStream(GPU_TEXTURE_D3D9_V_INV, texID, regData, 0);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglDraw(PrimitiveMode primitive, U32 start, U32 count)
{
    cgoMetaStream *metaStream;
    GPURegData regData;

    regData.primitive = primitive;
    metaStream = new cgoMetaStream(GPU_PRIMITIVE, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = start;
    metaStream = new cgoMetaStream(GPU_STREAM_START, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    regData.uintVal = count;
    metaStream = new cgoMetaStream(GPU_STREAM_COUNT, 0, regData, 0);
    cglWritecgoMetaStream(metaStream);

    metaStream = new cgoMetaStream(GPU_DRAW);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSwap()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_SWAPBUFFERS);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglReset()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_RESET);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglClearColorBuffer()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_CLEARCOLORBUFFER);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglClearZStencilBuffer()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_CLEARZSTENCILBUFFER);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglFlushColorBuffer()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_FLUSHCOLOR);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglFlushZStencilBuffer()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_FLUSHZSTENCIL);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSaveColorState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_SAVE_COLOR_STATE);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglRestoreColorState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_RESTORE_COLOR_STATE);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglSaveZStencilState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_SAVE_ZSTENCIL_STATE);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglRestoreZStencilState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_RESTORE_ZSTENCIL_STATE);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglResetColorState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_RESET_COLOR_STATE);
    cglWritecgoMetaStream(metaStream);
}

void CGLInterface::cglResetZStencilState()
{
    cgoMetaStream *metaStream;
    metaStream = new cgoMetaStream(GPU_RESET_ZSTENCIL_STATE);
    cglWritecgoMetaStream(metaStream);
}

bool CGLInterface::cglAssembleKernelFromText(char *programString, U08 *code, U32 &codeSize, string &errorString)
{
    istringstream inputStream;
    
    inputStream.clear();
    inputStream.str(string(programString));

    vector<ShaderInstruction*> program;

    program.clear();

    U32 line = 1;

    bool error = false;

    //  Read until the end of the input file or until there is an error assembling the instruction.
    while(!error && !inputStream.eof())
    {
        ShaderInstruction *shInstr;

        string currentLine;
        string errorString;

        //  Read a line from the input file.
        getline(inputStream, currentLine);

        if ((currentLine.length() == 0) && inputStream.eof())
        {
            //  End loop.
        }
        else
        {
            //  Assemble the line.
            shInstr = ShaderInstruction::assemble((char *) currentLine.c_str(), errorString);

            //  Check if the instruction was assembled.
            if (shInstr == NULL)
            {
                char buff[256];
                sprintf(buff, "Line %d.  ERROR : %s\n", line, errorString.c_str());
                errorString = buff;
                error = true;
            }
            else
            {
                //  Add instruction to the program.
                program.push_back(shInstr);
            }

            //  Update line number.
            line++;
        }
    }

    //  Check if an error occurred.
    if (!error)
    {
        //  Check if there is space in the buffer for the whole program.
        if (codeSize <= (program.size() * 16))
        {
            errorString = "ERROR: program too large for provided buffer";            
            codeSize = U32(program.size()) * 16;
            return false;
        }
        
        //  Set final code size.
        codeSize = U32(program.size()) * 16;
                
        //  Encode the instructions and write to the output file.
        for(U32 instr = 0; instr < program.size(); instr++)
        {
            //  Check if this is the last instruction.  If so mark the instruction with the end flag.
            if (instr == (program.size() - 1))
                program[instr]->setEndFlag(true);

            //  Encode instruction.
            program[instr]->getCode(&code[instr * 16]);                       
        }        
    }        
        
    return !error;
}

bool CGLInterface::cglAssembleKernelFromFile(char *filename, U08 *code, U32 &codeSize, string &errorString)
{
    string   inputFileName = filename;
    ifstream inputFile;
    inputFile.open(inputFileName.c_str());
    if (!inputFile.is_open())
    {
        errorString = "ERROR: could not open input file";
        return false;
    }
    vector<ShaderInstruction*> program;
    program.clear();
    U32  line = 1;
    bool error = false;
    //  Read until the end of the input file or until there is an error assembling the instruction.
    while(!error && !inputFile.eof())
    {
        ShaderInstruction *shInstr;
        string             currentLine;
        string             errorString;
        //  Read a line from the input file.
        getline(inputFile, currentLine);
        if ((currentLine.length() == 0) && inputFile.eof())
        {
            //  End loop.
        }
        else
        {
            //  Assemble the line.
            shInstr = ShaderInstruction::assemble((char *) currentLine.c_str(), errorString);

            //  Check if the instruction was assembled.
            if (shInstr == NULL)
            {
                char buff[256];
                sprintf(buff, "Line %d.  ERROR : %s\n", line, errorString.c_str());
                errorString = string(buff);
                error = true;
            }
            else
            {
                //  Add instruction to the program.
                program.push_back(shInstr);
            }

            //  Update line number.
            line++;
        }
    }

    //  Check if an error occurred.
    if (!error)
    {
        //  Check if there is space in the buffer for the whole program.
        if (codeSize <= (program.size() * 16))
        {
            errorString = "ERROR: program too large for provided buffer";            
            codeSize = U32(program.size()) * 16;
            inputFile.close();
            return false;
        }
        //  Set final code size.
        codeSize = U32(program.size()) * 16;
        //  Encode the instructions and write to the output file.
        for(U32 instr = 0; instr < program.size(); instr++)
        {
            //  Check if this is the last instruction.  If so mark the instruction with the end flag.
            if (instr == (program.size() - 1))
                program[instr]->setEndFlag(true);
            //  Encode instruction.
            program[instr]->getCode(&code[instr * 16]);                       
        }        
    }
    inputFile.close();
    return !error;
}

