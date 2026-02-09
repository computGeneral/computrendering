/**************************************************************************
 * ApitraceCallDispatcher.cpp
 * 
 * Maps apitrace CallEvents to CG1 OGL_gl* entry point functions.
 */

#include "ApitraceCallDispatcher.h"
#include "OGLEntryPoints.h"
#include "OGL.h"
#include <iostream>
#include <set>

using namespace apitrace;

#define A(n) arg(evt, n)

bool apitrace::dispatchCall(const CallEvent& evt) {
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
    if (fn == "glViewport")         { OGL_glViewport(asInt(A(0)), asInt(A(1)), asInt(A(2)), asInt(A(3))); return true; }
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
        fn == "glGenBuffers" || fn == "glGenBuffersARB" || fn == "glLineWidth" || fn == "glPointSize") {
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

