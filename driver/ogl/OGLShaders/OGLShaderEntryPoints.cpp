#include "OGLShaderEntryPoints.h"
#include <iostream>

using namespace std;

GLAPI GLuint GLAPIENTRY OGL_glCreateShader(GLenum type) {
    // cout << "OGL_glCreateShader stub" << endl;
    return 1;
}

GLAPI void GLAPIENTRY OGL_glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glCompileShader(GLuint shader) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    // 0x8B81 = GL_COMPILE_STATUS
    if (pname == 0x8B81) *params = 1; // GL_TRUE
    else *params = 0;
}

GLAPI void GLAPIENTRY OGL_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    if (length) *length = 0;
    if (infoLog && bufSize > 0) infoLog[0] = '\0';
}

GLAPI GLuint GLAPIENTRY OGL_glCreateProgram(void) {
    return 2;
}

GLAPI void GLAPIENTRY OGL_glAttachShader(GLuint program, GLuint shader) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glLinkProgram(GLuint program) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    // 0x8B82 = GL_LINK_STATUS
    if (pname == 0x8B82) *params = 1; // GL_TRUE
    else *params = 0;
}

GLAPI GLint GLAPIENTRY OGL_glGetUniformLocation(GLuint program, const GLchar* name) {
    return -1; // Not found
}

GLAPI GLint GLAPIENTRY OGL_glGetAttribLocation(GLuint program, const GLchar* name) {
    return -1;
}

GLAPI void GLAPIENTRY OGL_glUseProgram(GLuint program) {
    // Stub
}

GLAPI void GLAPIENTRY OGL_glUniform1i(GLint location, GLint v0) {
}

GLAPI void GLAPIENTRY OGL_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
}

GLAPI void GLAPIENTRY OGL_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
}

GLAPI void GLAPIENTRY OGL_glUniform1f(GLint location, GLfloat v0) {
}

GLAPI void GLAPIENTRY OGL_glValidateProgram(GLuint program) {
}
