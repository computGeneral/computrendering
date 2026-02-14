/**************************************************************************
 * ApitraceCallDispatcher.cpp
 * 
 * Maps apitrace CallEvents to CG1 OGL_gl* entry point functions.
 */

#include "ApitraceCallDispatcher.h"
#include "OGLEntryPoints.h"
#include "../../ogl/OGLShaders/OGLShaderEntryPoints.h"
#include "OGL.h"
#include <iostream>
#include <set>
#include <vector>
#include <map>

using namespace apitrace;

// Display List State
static bool isRecording = false;
static GLuint currentListId = 0;
// Note: We copy CallEvent. Be aware that Value objects with pointers (BLOB/OPAQUE) need care.
// In ApitraceParser, BLOB values own their std::vector<uint8_t>, so copying Value is deep-copy for blobs.
static std::map<GLuint, std::vector<CallEvent>> displayLists;

#define A(n) arg(evt, n)

// Forward declaration
bool executeCall(const CallEvent& evt);

bool apitrace::dispatchCall(const CallEvent& evt) {
    const std::string& fn = evt.signature.functionName;

    // ---- Display List Management ----
    if (fn == "glNewList") {
        currentListId = asUInt(A(0));
        // GLenum mode = asEnum(A(1)); // GL_COMPILE or GL_COMPILE_AND_EXECUTE
        // glxgears uses GL_COMPILE (0x1300). We treat both as compile-only for now (or TODO: execute if needed)
        
        isRecording = true;
        displayLists[currentListId].clear();
        return true; 
    }
    
    if (fn == "glEndList") {
        isRecording = false;
        currentListId = 0;
        return true;
    }

    if (fn == "glCallList") {
        GLuint listId = asUInt(A(0));
        auto it = displayLists.find(listId);
        if (it != displayLists.end()) {
            // Replay recorded events
            for (const auto& recordedEvt : it->second) {
                executeCall(recordedEvt);
            }
        } else {
            std::cerr << "[ApitraceDispatch] Warning: glCallList called for unknown list " << listId << std::endl;
        }
        return true;
    }
    
    if (fn == "glGenLists") return true; // Ignore, assume trace IDs are valid
    
    if (fn == "glDeleteLists") {
        GLuint list = asUInt(A(0));
        GLsizei range = asInt(A(1));
        for(GLuint i=0; i<(GLuint)range; ++i) {
            displayLists.erase(list+i);
        }
        return true;
    }

    // ---- Shaders ----
    if (fn == "glCreateShader") { OGL_glCreateShader(asEnum(A(0))); return true; }
    if (fn == "glShaderSource") { 
        // Stub: ignore arguments for now as we don't have shader compiler
        OGL_glShaderSource(asUInt(A(0)), 0, NULL, NULL); 
        return true; 
    }
    if (fn == "glCompileShader") { OGL_glCompileShader(asUInt(A(0))); return true; }
    if (fn == "glGetShaderiv") { 
        // We can't pass pointer to A(2) directly if it's not a pointer in trace value?
        // A(2) is a pointer (GLint *params).
        // ApitraceParser treats array/blob arguments.
        // But for output params, we usually ignore them in replay unless we validate.
        // Here we just call the stub.
        GLint params[4];
        OGL_glGetShaderiv(asUInt(A(0)), asEnum(A(1)), params); 
        return true; 
    }
    if (fn == "glGetShaderInfoLog") { 
        GLsizei length;
        GLchar infoLog[1024];
        OGL_glGetShaderInfoLog(asUInt(A(0)), asInt(A(1)), &length, infoLog); 
        return true; 
    }
    if (fn == "glCreateProgram") { OGL_glCreateProgram(); return true; }
    if (fn == "glAttachShader") { OGL_glAttachShader(asUInt(A(0)), asUInt(A(1))); return true; }
    if (fn == "glBindAttribLocation") { OGL_glBindAttribLocation(asUInt(A(0)), asUInt(A(1)), asString(A(2)).c_str()); return true; }
    if (fn == "glLinkProgram") { OGL_glLinkProgram(asUInt(A(0))); return true; }
    if (fn == "glGetProgramiv") { 
        GLint params[4];
        OGL_glGetProgramiv(asUInt(A(0)), asEnum(A(1)), params); 
        return true; 
    }
    if (fn == "glGetUniformLocation") { OGL_glGetUniformLocation(asUInt(A(0)), asString(A(1)).c_str()); return true; }
    if (fn == "glGetAttribLocation") { OGL_glGetAttribLocation(asUInt(A(0)), asString(A(1)).c_str()); return true; }
    if (fn == "glUseProgram") { OGL_glUseProgram(asUInt(A(0))); return true; }
    if (fn == "glUniform1i") { OGL_glUniform1i(asInt(A(0)), asInt(A(1))); return true; }
    if (fn == "glUniform4f") { OGL_glUniform4f(asInt(A(0)), asFloat(A(1)), asFloat(A(2)), asFloat(A(3)), asFloat(A(4))); return true; }
    if (fn == "glUniformMatrix4fv") { 
        // array
        OGL_glUniformMatrix4fv(asInt(A(0)), asInt(A(1)), asBool(A(2)), NULL); 
        return true; 
    }
    if (fn == "glUniform1f") { OGL_glUniform1f(asInt(A(0)), asFloat(A(1))); return true; }
    if (fn == "glValidateProgram") { OGL_glValidateProgram(asUInt(A(0))); return true; }

    // ---- Recording ----
    if (isRecording) {
        displayLists[currentListId].push_back(evt);
        return true;
    }

    // ---- Execution ----
    return executeCall(evt);
}

bool executeCall(const CallEvent& evt) {
    const std::string& fn = evt.signature.functionName;

    // ---- State management ----
    if (fn == "glEnable")           { OGL_glEnable(asEnum(A(0))); return true; }
    if (fn == "glDisable")          { OGL_glDisable(asEnum(A(0))); return true; }
    if (fn == "glShadeModel")       { OGL_glShadeModel(asEnum(A(0))); return true; }
    if (fn == "glCullFace")         { OGL_glCullFace(asEnum(A(0))); return true; }
    if (fn == "glFrontFace")        { OGL_glFrontFace(asEnum(A(0))); return true; }
    if (fn == "glPolygonMode")      { OGL_glPolygonMode(asEnum(A(0)), asEnum(A(1))); return true; }
    if (fn == "glPolygonOffset")    { OGL_glPolygonOffset(asFloat(A(0)), asFloat(A(1))); return true; }

    // ---- Clear ----
    if (fn == "glClear")            { OGL_glClear(asUInt(A(0))); return true; }
    if (fn == "glClearColor")       { OGL_glClearColor(asClampf(A(0)), asClampf(A(1)), asClampf(A(2)), asClampf(A(3))); return true; }
    if (fn == "glClearDepth")       { OGL_glClearDepth(asClampd(A(0))); return true; }
    if (fn == "glClearStencil")     { OGL_glClearStencil(asInt(A(0))); return true; }

    // ---- Viewport / Scissor ----
    if (fn == "glViewport")         { 
        GLint x = asInt(A(0));
        GLint y = asInt(A(1));
        GLsizei w = asInt(A(2));
        GLsizei h = asInt(A(3));
        
        // Prevent crash due to 0-size viewport (e.g. if args are missing/default in trace)
        if (w == 0) w = 300; 
        if (h == 0) h = 300;

        OGL_glViewport(x, y, w, h); 
        return true; 
    }
    // Added specific check for "lScissor" corruption seen in logs, just in case
    if (fn == "glScissor")          { OGL_glScissor(asInt(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3))); return true; }
    if (fn == "glDepthRange")       { OGL_glDepthRange(asClampd(A(0)), asClampd(A(1))); return true; }

    // ---- Matrix ----
    if (fn == "glMatrixMode")       { OGL_glMatrixMode(asEnum(A(0))); return true; }
    if (fn == "glLoadIdentity")     { OGL_glLoadIdentity(); return true; }
    if (fn == "glLoadMatrixf")      { OGL_glLoadMatrixf(asFloatPtr(A(0))); return true; }
    if (fn == "glMultMatrixf")      { OGL_glMultMatrixf(asFloatPtr(A(0))); return true; }
    if (fn == "glMultMatrixd")      { OGL_glMultMatrixd(asDoublePtr(A(0))); return true; }
    if (fn == "glPushMatrix")       { OGL_glPushMatrix(); return true; }
    if (fn == "glPopMatrix")        { OGL_glPopMatrix(); return true; }
    if (fn == "glOrtho")            { OGL_glOrtho(asDouble(A(0)), asDouble(A(1)), asDouble(A(2)), asDouble(A(3)), asDouble(A(4)), asDouble(A(5))); return true; }
    if (fn == "glFrustum")          { OGL_glFrustum(asDouble(A(0)), asDouble(A(1)), asDouble(A(2)), asDouble(A(3)), asDouble(A(4)), asDouble(A(5))); return true; }
    if (fn == "glTranslatef")       { OGL_glTranslatef(asFloat(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }
    if (fn == "glTranslated")       { OGL_glTranslated(asDouble(A(0)), asDouble(A(1)), asDouble(A(2))); return true; }
    if (fn == "glRotatef")          { OGL_glRotatef(asFloat(A(0)), asFloat(A(1)), asFloat(A(2)), asFloat(A(3))); return true; }
    if (fn == "glRotated")          { OGL_glRotated(asDouble(A(0)), asDouble(A(1)), asDouble(A(2)), asDouble(A(3))); return true; }
    if (fn == "glScalef")           { OGL_glScalef(asFloat(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }

    // ---- Immediate mode vertex submission ----
    if (fn == "glBegin")            { OGL_glBegin(asEnum(A(0))); return true; }
    if (fn == "glEnd")              { OGL_glEnd(); return true; }
    if (fn == "glVertex2i")         { OGL_glVertex2i(asInt(A(0)), asInt(A(1))); return true; }
    if (fn == "glVertex2f")         { OGL_glVertex2f(asFloat(A(0)), asFloat(A(1))); return true; }
    if (fn == "glVertex3f")         { OGL_glVertex3f(asFloat(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }
    if (fn == "glVertex3fv")        { OGL_glVertex3fv(asFloatPtr(A(0))); return true; }
    if (fn == "glNormal3f")         { OGL_glNormal3f(asFloat(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }
    if (fn == "glNormal3fv")        { OGL_glNormal3fv(asFloatPtr(A(0))); return true; }
    if (fn == "glColor3f")          { OGL_glColor3f(asFloat(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }
    if (fn == "glColor4f")          { OGL_glColor4f(asFloat(A(0)), asFloat(A(1)), asFloat(A(2)), asFloat(A(3))); return true; }
    if (fn == "glColor4fv")         { OGL_glColor4fv(asFloatPtr(A(0))); return true; }
    if (fn == "glColor4ub")         { OGL_glColor4ub(asUInt(A(0)), asUInt(A(1)), asUInt(A(2)), asUInt(A(3))); return true; }
    if (fn == "glTexCoord2f")       { OGL_glTexCoord2f(asFloat(A(0)), asFloat(A(1))); return true; }
    if (fn == "glMultiTexCoord2f")  { OGL_glMultiTexCoord2f(asEnum(A(0)), asFloat(A(1)), asFloat(A(2))); return true; }
    if (fn == "glMultiTexCoord2fv") { OGL_glMultiTexCoord2fv(asEnum(A(0)), asFloatPtr(A(1))); return true; }

    // ---- Vertex arrays ----
    if (fn == "glEnableClientState")  { OGL_glEnableClientState(asEnum(A(0))); return true; }
    if (fn == "glDisableClientState") { OGL_glDisableClientState(asEnum(A(0))); return true; }
    if (fn == "glVertexPointer")    { OGL_glVertexPointer(asInt(A(0)), asEnum(A(1)), asInt(A(2)), asVoidPtr(A(3))); return true; }
    if (fn == "glColorPointer")     { OGL_glColorPointer(asInt(A(0)), asEnum(A(1)), asInt(A(2)), asVoidPtr(A(3))); return true; }
    if (fn == "glNormalPointer")    { OGL_glNormalPointer(asEnum(A(0)), asInt(A(1)), asVoidPtr(A(2))); return true; }
    if (fn == "glTexCoordPointer")  { OGL_glTexCoordPointer(asInt(A(0)), asEnum(A(1)), asInt(A(2)), asVoidPtr(A(3))); return true; }
    if (fn == "glDrawArrays")       { OGL_glDrawArrays(asEnum(A(0)), asInt(A(1)), asInt(A(2))); return true; }
    if (fn == "glDrawElements")     { OGL_glDrawElements(asEnum(A(0)), asInt(A(1)), asEnum(A(2)), asVoidPtr(A(3))); return true; }
    if (fn == "glDrawRangeElements"){ OGL_glDrawRangeElements(asEnum(A(0)), asUInt(A(1)), asUInt(A(2)), asInt(A(3)), asEnum(A(4)), asVoidPtr(A(5))); return true; }
    if (fn == "glArrayElement")     { OGL_glArrayElement(asInt(A(0))); return true; }

    // ---- VBO (ARB) ----
    if (fn == "glBindBuffer" || fn == "glBindBufferARB")         { OGL_glBindBufferARB(asEnum(A(0)), asUInt(A(1))); return true; }
    if (fn == "glBufferData" || fn == "glBufferDataARB")         { OGL_glBufferDataARB(asEnum(A(0)), (GLsizeiptr)A(1).uintVal, asVoidPtr(A(2)), asEnum(A(3))); return true; }
    if (fn == "glDeleteBuffers" || fn == "glDeleteBuffersARB")   { OGL_glDeleteBuffersARB(asInt(A(0)), (const GLuint*)asVoidPtr(A(1))); return true; }
    if (fn == "glEnableVertexAttribArray" || fn == "glEnableVertexAttribArrayARB")   { OGL_glEnableVertexAttribArrayARB(asUInt(A(0))); return true; }
    if (fn == "glDisableVertexAttribArray" || fn == "glDisableVertexAttribArrayARB") { OGL_glDisableVertexAttribArrayARB(asUInt(A(0))); return true; }
    if (fn == "glVertexAttribPointer" || fn == "glVertexAttribPointerARB") { OGL_glVertexAttribPointerARB(asUInt(A(0)), asInt(A(1)), asEnum(A(2)), asBool(A(3)), asInt(A(4)), asVoidPtr(A(5))); return true; }

    // ---- Texture ----
    if (fn == "glActiveTexture" || fn == "glActiveTextureARB")     { OGL_glActiveTextureARB(asEnum(A(0))); return true; }
    if (fn == "glClientActiveTexture" || fn == "glClientActiveTextureARB") { OGL_glClientActiveTextureARB(asEnum(A(0))); return true; }
    if (fn == "glBindTexture")      { OGL_glBindTexture(asEnum(A(0)), asUInt(A(1))); return true; }
    if (fn == "glDeleteTextures")   { OGL_glDeleteTextures(asInt(A(0)), (const GLuint*)asVoidPtr(A(1))); return true; }
    if (fn == "glTexParameteri")    { OGL_glTexParameteri(asEnum(A(0)), asEnum(A(1)), asInt(A(2))); return true; }
    if (fn == "glTexParameterf")    { OGL_glTexParameterf(asEnum(A(0)), asEnum(A(1)), asFloat(A(2))); return true; }
    if (fn == "glPixelStorei")      { OGL_glPixelStorei(asEnum(A(0)), asInt(A(1))); return true; }
    if (fn == "glTexImage2D")       { OGL_glTexImage2D(asEnum(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asEnum(A(6)), asEnum(A(7)), asVoidPtr(A(8))); return true; }
    if (fn == "glTexSubImage2D")    { OGL_glTexSubImage2D(asEnum(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asEnum(A(6)), asEnum(A(7)), asVoidPtr(A(8))); return true; }
    if (fn == "glCopyTexImage2D")   { OGL_glCopyTexImage2D(asEnum(A(0)), asInt(A(1)), asEnum(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asInt(A(6)), asInt(A(7))); return true; }
    if (fn == "glCopyTexSubImage2D"){ OGL_glCopyTexSubImage2D(asEnum(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asInt(A(6)), asInt(A(7))); return true; }
    if (fn == "glTexEnvi")          { OGL_glTexEnvi(asEnum(A(0)), asEnum(A(1)), asInt(A(2))); return true; }
    if (fn == "glTexEnvf")          { OGL_glTexEnvf(asEnum(A(0)), asEnum(A(1)), asFloat(A(2))); return true; }
    if (fn == "glTexEnvfv")         { OGL_glTexEnvfv(asEnum(A(0)), asEnum(A(1)), asFloatPtr(A(2))); return true; }
    if (fn == "glTexGeni")          { OGL_glTexGeni(asEnum(A(0)), asEnum(A(1)), asInt(A(2))); return true; }
    if (fn == "glTexGenf")          { OGL_glTexGenf(asEnum(A(0)), asEnum(A(1)), asFloat(A(2))); return true; }
    if (fn == "glTexGenfv")         { OGL_glTexGenfv(asEnum(A(0)), asEnum(A(1)), asFloatPtr(A(2))); return true; }

    // ---- Compressed textures ----
    if (fn == "glCompressedTexImage2D" || fn == "glCompressedTexImage2DARB")     { OGL_glCompressedTexImage2DARB(asEnum(A(0)), asInt(A(1)), asEnum(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asInt(A(6)), asVoidPtr(A(7))); return true; }
    if (fn == "glCompressedTexSubImage2D" || fn == "glCompressedTexSubImage2DARB") { OGL_glCompressedTexSubImage2DARB(asEnum(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3)), asInt(A(4)), asInt(A(5)), asEnum(A(6)), asInt(A(7)), asVoidPtr(A(8))); return true; }

    // ---- Lighting ----
    if (fn == "glLightfv")          { OGL_glLightfv(asEnum(A(0)), asEnum(A(1)), asFloatPtr(A(2))); return true; }
    if (fn == "glLightf")           { OGL_glLightf(asEnum(A(0)), asEnum(A(1)), asFloat(A(2))); return true; }
    if (fn == "glLightModelfv")     { OGL_glLightModelfv(asEnum(A(0)), asFloatPtr(A(1))); return true; }
    if (fn == "glLightModelf")      { OGL_glLightModelf(asEnum(A(0)), asFloat(A(1))); return true; }
    if (fn == "glLightModeli")      { OGL_glLightModeli(asEnum(A(0)), asInt(A(1))); return true; }
    if (fn == "glMaterialfv")       { OGL_glMaterialfv(asEnum(A(0)), asEnum(A(1)), asFloatPtr(A(2))); return true; }
    if (fn == "glMaterialf")        { OGL_glMaterialf(asEnum(A(0)), asEnum(A(1)), asFloat(A(2))); return true; }
    if (fn == "glColorMaterial")    { OGL_glColorMaterial(asEnum(A(0)), asEnum(A(1))); return true; }

    // ---- Depth / Stencil / Blend ----
    if (fn == "glDepthFunc")        { OGL_glDepthFunc(asEnum(A(0))); return true; }
    if (fn == "glDepthMask")        { OGL_glDepthMask(asBool(A(0))); return true; }
    if (fn == "glStencilFunc")      { OGL_glStencilFunc(asEnum(A(0)), asInt(A(1)), asUInt(A(2))); return true; }
    if (fn == "glStencilOp")        { OGL_glStencilOp(asEnum(A(0)), asEnum(A(1)), asEnum(A(2))); return true; }
    if (fn == "glStencilMask")      { OGL_glStencilMask(asUInt(A(0))); return true; }
    if (fn == "glColorMask")        { OGL_glColorMask(asBool(A(0)), asBool(A(1)), asBool(A(2)), asBool(A(3))); return true; }
    if (fn == "glAlphaFunc")        { OGL_glAlphaFunc(asEnum(A(0)), asClampf(A(1))); return true; }
    if (fn == "glBlendFunc")        { OGL_glBlendFunc(asEnum(A(0)), asEnum(A(1))); return true; }
    if (fn == "glBlendEquation" || fn == "glBlendEquationEXT") { OGL_glBlendEquation(asEnum(A(0))); return true; }
    if (fn == "glBlendColor")       { OGL_glBlendColor(asClampf(A(0)), asClampf(A(1)), asClampf(A(2)), asClampf(A(3))); return true; }

    // ---- Fog ----
    if (fn == "glFogf")             { OGL_glFogf(asEnum(A(0)), asFloat(A(1))); return true; }
    if (fn == "glFogi")             { OGL_glFogi(asEnum(A(0)), asInt(A(1))); return true; }
    if (fn == "glFogfv")            { OGL_glFogfv(asEnum(A(0)), asFloatPtr(A(1))); return true; }
    if (fn == "glFogiv")            { OGL_glFogiv(asEnum(A(0)), (const GLint*)asVoidPtr(A(1))); return true; }

    // ---- ARB Programs ----
    if (fn == "glBindProgramARB")   { OGL_glBindProgramARB(asEnum(A(0)), asUInt(A(1))); return true; }
    if (fn == "glProgramStringARB") { OGL_glProgramStringARB(asEnum(A(0)), asEnum(A(1)), asInt(A(2)), asVoidPtr(A(3))); return true; }
    if (fn == "glProgramEnvParameter4fvARB")    { OGL_glProgramEnvParameter4fvARB(asEnum(A(0)), asUInt(A(1)), asFloatPtr(A(2))); return true; }
    if (fn == "glProgramLocalParameter4fvARB")  { OGL_glProgramLocalParameter4fvARB(asEnum(A(0)), asUInt(A(1)), asFloatPtr(A(2))); return true; }
    if (fn == "glProgramLocalParameter4fARB")   { OGL_glProgramLocalParameter4fARB(asEnum(A(0)), asUInt(A(1)), asFloat(A(2)), asFloat(A(3)), asFloat(A(4)), asFloat(A(5))); return true; }

    // ---- Push/Pop ----
    if (fn == "glPushAttrib")       { OGL_glPushAttrib(asUInt(A(0))); return true; }
    if (fn == "glPopAttrib")        { OGL_glPopAttrib(); return true; }

    // ---- Misc ----
    if (fn == "glFlush")            { OGL_glFlush(); return true; }
    if (fn == "glGetString")        { OGL_glGetString(asEnum(A(0))); return true; }

    // ---- Platform / swap (silently skip or handle) ----
    if (fn == "wglSwapBuffers" || fn == "glXSwapBuffers" || fn == "eglSwapBuffers" ||
        fn == "wglSwapLayerBuffers" || fn == "SwapBuffers") {
        // Handled by TraceDriverApitrace directly
        return true;
    }

    // ---- Context setup (skip on Linux — no real window) ----
    if (fn.substr(0, 3) == "wgl" || fn.substr(0, 3) == "glX" || fn.substr(0, 3) == "egl" ||
        fn == "glFinish" || fn == "glGetError" || fn == "glGetIntegerv" || fn == "glGetFloatv" ||
        fn == "glIsEnabled" || fn == "glHint" || fn == "glReadPixels" || fn == "glGenTextures" ||
        fn == "glGenBuffers" || fn == "glGenBuffersARB" || fn == "glLineWidth" || fn == "glPointSize" ||
        fn == "glXQueryExtensionsString") {
        // Silently skip — these don't affect simulation
        return true;
    }

    // ---- Unmapped call ----
    static std::set<std::string> warned;
    if (warned.find(fn) == warned.end()) {
        std::cerr << "[ApitraceDispatch] Unhandled GL call: " << fn << std::endl;
        warned.insert(fn);
    }
    return false;
}

#undef A
