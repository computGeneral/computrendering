/**************************************************************************
 *
 */

#ifndef OGL_CONTEXT
    #define OGL_CONTEXT

#include "gl.h"
#include "glext.h"
#include "GALTypes.h"
#include "GALDevice.h"
#include "GALBuffer.h"
#include "GALSampler.h"
#include "GALShaderProgram.h"
#include "MatrixStack.h"
#include "GALx.h"
#include "OGLTextureUnit.h"
#include "GALRasterizationStage.h"
#include "GALxTextCoordGenerationStage.h"
#include "GALZStencilStage.h"

#include "ARBProgramManager.h"
#include "OGLBufferManager.h"
#include "OGLTextureManager.h"
#include "GALBlendingStage.h"

#include <stack>
#include <bitset>
#include <set>
#include <vector>
#include <math.h>

namespace ogl
{

enum OGL_RENDERSTATE
{
    RS_VERTEX_PROGRAM,
    RS_FRAGMENT_PROGRAM,
    RS_MAXRENDERSTATE
};

enum OGL_ARRAY
{
    VERTEX_ARRAY,
    NORMAL_ARRAY,
    COLOR_ARRAY,
    TEX_COORD_ARRAY_0,
    TEX_COORD_ARRAY_1,
    TEX_COORD_ARRAY_2,
    TEX_COORD_ARRAY_3,
    TEX_COORD_ARRAY_4,
    TEX_COORD_ARRAY_5,
    TEX_COORD_ARRAY_6,
    TEX_COORD_ARRAY_7,
    GENERIC_ARRAY_0,
    GENERIC_ARRAY_1,
    GENERIC_ARRAY_2,
    GENERIC_ARRAY_3,
    GENERIC_ARRAY_4,
    GENERIC_ARRAY_5,
    GENERIC_ARRAY_6,
    GENERIC_ARRAY_7,
    GENERIC_ARRAY_8,
    GENERIC_ARRAY_9,
    GENERIC_ARRAY_10,
    GENERIC_ARRAY_11,
    GENERIC_ARRAY_12,
    GENERIC_ARRAY_13,
    GENERIC_ARRAY_14,
    GENERIC_ARRAY_15,

};

//using namespace libGAL; // Open libGAL into namespace ogl

class GLContext
{
public:

    typedef libGAL::gal_int gal_int;
    typedef libGAL::gal_uint gal_uint;
    typedef libGAL::gal_float gal_float;
    typedef libGAL::gal_ubyte gal_ubyte;
    typedef libGAL::gal_bool gal_bool;

    struct VArray
    {
        bool inicialized;
        GLuint size; ///< # of components
        GLenum type; ///< type of each components
        GLsizei stride; ///< stride
        const GLvoid* userBuffer; ///< user data
        GLuint bufferID; ///< bound buffer (0 means unbound)
        bool normalized;

        /**
         * Default constructor (empty means no penalty creation)
         */
        VArray() : inicialized(false), bufferID(0), normalized(false), stride(0), userBuffer(0), size(0), type(0){}

        /**
         * Create an initialized VArray struct
         *
         * @param size number of components that has any "item"
         * @param type type of each component (GL_FLOAT, GL_INT, etc)
         * @stride Vertex array stride
         * @userBuffer pointer to data (user data)
         */
        VArray(GLuint size, GLenum type, GLsizei stride, const GLvoid* userBuffer, bool normalized = false) :
            size(size), type(type), stride(stride), userBuffer(userBuffer), bufferID(0), normalized(normalized), inicialized(true)
        {}

        GLuint bytes(GLuint elems = 1) const;


        void dump() const
        {
            std::cout << "size: " << size << std::endl;
            std::cout << "type: " << type << std::endl;
            std::cout << "stride: " << stride << std::endl;
            std::cout << "normalized: " << normalized << std::endl;
        }

    };

    ARBProgramManager& arbProgramManager();
    BufferManager& bufferManager();
    GLTextureManager& textureManager();

    GLContext(libGAL::GALDevice* galDev);

    void setLightParam(GLenum light, GLenum pname, const GLfloat* params);
    void setLightModel(GLenum pname, const GLfloat* params);
    void setMaterialParam(GLenum face, GLenum pname, const GLfloat* params);

    libGAL::GALDevice& gal();

    libGAL::GALxFixedPipelineState& fixedPipelineState();
    libGAL::GALx_FIXED_PIPELINE_SETTINGS& fixedPipelineSettings();

    void setShaders();

    // Put data into private buffers
    void addVertex(gal_float x, gal_float y, gal_float z = 0, gal_float w = 1);

    void drawElements ( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

    void setColor(gal_float red, gal_float green, gal_float blue, gal_float alpha = 1);
    void setColor(gal_uint red, gal_uint green, gal_uint blue);
    void setNormal(gal_float x, gal_float y, gal_float z);

    void setClearColor(gal_ubyte red, gal_ubyte green, gal_ubyte blue, gal_ubyte alpha);
    void setClearColor(gal_float red, gal_float green, gal_float blue, gal_float alpha);
    void getClearColor(gal_ubyte& red, gal_ubyte& green, gal_ubyte& blue, gal_ubyte& alpha) const;

    void setDepthClearValue(gal_float depthValue);
    gal_float getDepthClearValue() const;

    void setStencilClearValue(gal_int stencilValue);
    gal_int getStencilClearValue() const;

    gal_uint countInternalVertexes() const;
    void initInternalBuffers(gal_bool createBuffers);
    void attachInternalBuffers();
    void deattachInternalBuffers();

    void attachInternalSamplers();
    void deattachInternalSamplers();
    
    /**
     * Accepts GL_PROJECTION_MATRIX, GL_MODELVIEW_MATRIX, GL_TEXTURE
     */
    void setMatrixMode(GLenum mode);

    void setActiveTextureUnit(gal_uint textureStage);
    void setActiveModelview(gal_uint modelviewStack);

    // Three flavors to access matrix stacks
    MatrixStack& matrixStack();
    MatrixStack& matrixStack(GLenum mode);
    MatrixStack& matrixStack(GLenum mode, gal_uint unit);

    void setRenderState(OGL_RENDERSTATE state, gal_bool enabled);
    gal_bool getRenderState(OGL_RENDERSTATE state) const;

    void dump() const;

    gal_uint obtainStream();
    void releaseStream(gal_uint streamID);

    gal_uint obtainSampler();
    void releaseSampler(gal_uint streamID);

    GLContext::VArray& getPosVArray();
    GLContext::VArray& getColorVArray();
    GLContext::VArray& getNormalVArray(); 
    GLContext::VArray& getIndexVArray(); 
    GLContext::VArray& getEdgeVArray();
    GLContext::VArray& getTextureVArray(GLuint unit);
    GLContext::VArray& getGenericVArray(GLuint unit);

    void setDescriptor ( OGL_ARRAY descr, GLint offset, GLint components, libGAL::GAL_STREAM_DATA type, GLint stride );
    void setVertexBuffer (libGAL::GALBuffer *ptr);
    void setNormalBuffer (libGAL::GALBuffer *ptr);
    void setColorBuffer (libGAL::GALBuffer *ptr);
    void setTexBuffer (libGAL::GALBuffer *ptr);
    
    void enableBufferArray (GLenum cap);
    void disableBufferArray (GLenum cap);
    void getIndicesSet(std::set<GLuint>& iSet, GLenum type, const GLvoid* indices, GLsizei count);
    libGAL::GALBuffer* createIndexBuffer(const GLvoid* indices, GLenum indicesType, GLsizei count, const std::set<GLuint>& usedIndices);
    void setVertexArraysBuffers(GLuint start, GLuint count, std::set<GLuint>& iSet);
    libGAL::GALBuffer* configureStream (VArray *array, bool colorAttrib, GLuint start, GLuint count, set<GLuint>& iSet);

    libGAL::GALBuffer* fillVA_ind (GLenum type, const GLvoid* buf, GLsizei size, GLsizei stride, const std::set<GLuint>& iSet);
    libGAL::GALBuffer* fillVAColor_ind (GLenum type, const GLvoid* buf, GLsizei size, GLsizei stride, const std::set<GLuint>& iSet);
    template<typename T> libGAL::GALBuffer* fillVA_ind(const T* buf, GLsizei size, GLsizei stride, const std::set<GLuint>& iSet);
    template<typename A> libGAL::GALBuffer* fillVAColor_ind(const A* buf, GLsizei size, GLsizei stride, const std::set<GLuint>& iSet);

    libGAL::GALBuffer* fillVAColor(GLenum type, const GLvoid* buf, GLsizei size, GLsizei stride, GLsizei start, GLsizei count);
    libGAL::GALBuffer* fillVA(GLenum type, const GLvoid* buf, GLsizei size, GLsizei stride, GLsizei start, GLsizei count);
    template<typename T> libGAL::GALBuffer* fillVA(const T* buf, GLsizei size, GLsizei stride, GLsizei start, GLsizei count);
    template<typename A> libGAL::GALBuffer* fillVAColor(const A* buf, GLsizei size, GLsizei stride, GLsizei start, GLsizei count);

    bool allBuffersBound() const;
    bool allBuffersUnbound() const;
    libGAL::GALBuffer* rawIndices(const GLvoid* indices, GLenum type, GLsizei count);
    void resetDescriptors ();

    GLuint getSize(GLenum openGLType);

    void fillVertexArrayBuffer (OGL_ARRAY stream, GLint count);

    void setTexCoord(GLuint unit, GLfloat r, GLfloat s, GLfloat t = 0, GLfloat q = 1);

    TextureUnit& getTextureUnit(GLuint unit);
    TextureUnit& getActiveTextureUnit();
    GLuint getActiveTextureUnitNumber();
    void setFog(GLenum pname, const GLfloat* params);
    void setFog(GLenum pname, const GLint* params);

    void setCurrentClientTextureUnit(GLenum tex);
    GLenum getCurrentClientTextureUnit() const;
    void setEnabledCurrentClientTextureUnit(bool enabled);
    bool isEnabledCurrentClientTextureUnit() const;
    bool isEnabledClientTextureUnit(GLuint tex);


    GLContext::VArray& getTexVArray(GLuint unit);

    void setTexGen(GLenum coord, GLenum pname, GLint param);
    void setTexGen(GLenum coord, GLenum pname, const GLfloat* params);


    void setEnabledGenericAttribArray(GLuint index, bool enabled);
    bool isGenericAttribArrayEnabled(GLuint index) const;
    GLuint countGenericVArrays() const;

    void pushAttrib (GLbitfield mask);
    void popAttrib (void);

private:

    int _numTU;

    std::set<gal_uint> _freeStream;
    std::set<gal_uint> _freeSampler;

    // Managers
    ARBProgramManager _arbProgramManager;
    ogl::BufferManager _bufferManager;
    ogl::GLTextureManager _textureManager;

    gal_bool _renderStates[RS_MAXRENDERSTATE];

    // Fixed function emulation
    libGAL::GALxFixedPipelineState* _fpState;
    libGAL::GALx_FIXED_PIPELINE_SETTINGS _fpSettings;

    // Stream descriptors used with internal buffers
    libGAL::GAL_STREAM_DESC _vertexDesc;
    libGAL::GAL_STREAM_DESC _colorDesc;
    libGAL::GAL_STREAM_DESC _normalDesc;
    libGAL::GAL_STREAM_DESC _textureDesc[16];

    gal_uint _vertexStreamMap;
    gal_uint _colorStreamMap;
    gal_uint _normalStreamMap;
    gal_uint _textureStreamMap[16];

    gal_uint _textureSamplerMap[16];

    libGAL::GALDevice* _galDev;

    // Buffers to emulate glBegin/glEnd paradigm
    libGAL::GALBuffer* _vertexBuffer;
    libGAL::GALBuffer* _colorBuffer;
    libGAL::GALBuffer* _normalBuffer;
    libGAL::GALBuffer* _textureBuffer[16];

    gal_bool _vertexVBO;
    gal_bool _colorVBO;
    gal_bool _normalVBO;
    gal_bool _textureVBO[16];

    gal_uint _bufferVertexes;   // Number of vertices added (glBegin/glEng)

    gal_float _currentColor[4];
    gal_float _currentNormal[3];
    gal_float _currentTexCoord[8][4];

    // Vertex arrays
    VArray _posVArray;
    VArray _colVArray;
    VArray _norVArray;
    VArray _indVArray;
    VArray _edgeVArray;
    std::vector<VArray> _texVArray;
    std::vector<VArray> _genericVArray;
    std::vector<libGAL::GAL_STREAM_DESC> _genericDesc;
    std::vector<libGAL::GALBuffer*> _genericBuffer;
    std::vector<gal_uint> _genericStreamMap;
    GLboolean _genericAttribArrayEnabled;
    std::vector<bool> _genericAttribArrayFlags;

    gal_ubyte _clearColorR;
    gal_ubyte _clearColorG;
    gal_ubyte _clearColorB;
    gal_ubyte _clearColorA;
    
    gal_float _depthClearValue;
    gal_int _stencilClearValue;

    libGAL::GALShaderProgram* _emulVertexProgram;
    libGAL::GALShaderProgram* _emulFragmentProgram;
    libGAL::GALShaderProgram* _currentFragmentProgram;

    std::vector<MatrixStack*> _stackGroup[3];
    gal_uint _currentStackGroup;

    gal_uint _activeTextureUnit;
    gal_uint _currentModelview;

    static gal_ubyte _clamp(gal_float v);

    gal_int _activeBuffer; /*        GL_VERTEX_ARRAY: bit 0
                                    GL_COLOR_ARRAY: bit 1
                                    GL_NORMAL_ARRAY: bit 2
                                    GL_INDEX_ARRAY: bit 3
                                    GL_TEXTURE_COORD_ARRAY: bit 4
                                    GL_EDGE_FLAG_ARRAY: bit 5*/

    /**
     * Texture units state
     */
    vector<TextureUnit> _textureUnits;
    float _fogStart;
    float _fogEnd;
    GLuint _currentClientTextureUnit;
	vector<bool> _clientTextureUnits;

    void _setSamplerConfiguration();

};

}

#endif // OGL_CONTEXT
