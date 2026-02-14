#ifndef OGL_SHADER_ENTRY_POINTS_H
#define OGL_SHADER_ENTRY_POINTS_H

#include "gl.h"
#include "glext.h"

#ifndef GLAPI
#define GLAPI extern
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

GLAPI GLuint GLAPIENTRY OGL_glCreateShader(GLenum type);
GLAPI void GLAPIENTRY OGL_glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
GLAPI void GLAPIENTRY OGL_glCompileShader(GLuint shader);
GLAPI void GLAPIENTRY OGL_glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLAPI void GLAPIENTRY OGL_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
GLAPI GLuint GLAPIENTRY OGL_glCreateProgram(void);
GLAPI void GLAPIENTRY OGL_glAttachShader(GLuint program, GLuint shader);
GLAPI void GLAPIENTRY OGL_glBindAttribLocation(GLuint program, GLuint index, const GLchar* name);
GLAPI void GLAPIENTRY OGL_glLinkProgram(GLuint program);
GLAPI void GLAPIENTRY OGL_glGetProgramiv(GLuint program, GLenum pname, GLint* params);
GLAPI GLint GLAPIENTRY OGL_glGetUniformLocation(GLuint program, const GLchar* name);
GLAPI GLint GLAPIENTRY OGL_glGetAttribLocation(GLuint program, const GLchar* name);
GLAPI void GLAPIENTRY OGL_glUseProgram(GLuint program);
GLAPI void GLAPIENTRY OGL_glUniform1i(GLint location, GLint v0);
GLAPI void GLAPIENTRY OGL_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLAPI void GLAPIENTRY OGL_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
GLAPI void GLAPIENTRY OGL_glUniform1f(GLint location, GLfloat v0);
GLAPI void GLAPIENTRY OGL_glValidateProgram(GLuint program);

#endif
