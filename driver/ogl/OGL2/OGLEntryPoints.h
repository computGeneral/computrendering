/**************************************************************************
 *
 */

#ifndef OGLENTRYPOINTS
    #define OGLENTRYPOINTS

#include "gl.h"
#include "glext.h"
#include "OGLContext.h"

/*************************************
 * OpenGL on OGL supported functions *
 *************************************/

GLAPI void GLAPIENTRY OGL_glBegin( GLenum );
GLAPI void GLAPIENTRY OGL_glBindBufferARB(GLenum target, GLuint buffer);
GLAPI void GLAPIENTRY OGL_glDeleteBuffersARB(GLsizei n, const GLuint *buffers);
GLAPI void GLAPIENTRY OGL_glBufferDataARB(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

GLAPI void GLAPIENTRY OGL_glBufferSubDataARB (GLenum target, GLintptrARB offset,
                                          GLsizeiptrARB size, const GLvoid *data);

GLAPI void GLAPIENTRY OGL_glClear( GLbitfield mask );

GLAPI void GLAPIENTRY OGL_glClearColor( GLclampf red, GLclampf green,
                                   GLclampf blue, GLclampf alpha );

GLAPI void GLAPIENTRY OGL_glColor3f( GLfloat red, GLfloat green, GLfloat blue );


GLAPI void GLAPIENTRY OGL_glLoadIdentity( void );

GLAPI void GLAPIENTRY OGL_glMatrixMode( GLenum mode );

GLAPI void GLAPIENTRY OGL_glOrtho( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_, GLdouble far_ );

GLAPI void GLAPIENTRY OGL_glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_, GLdouble far_ );

GLAPI void GLAPIENTRY OGL_glShadeModel( GLenum mode );

GLAPI void GLAPIENTRY OGL_glVertex2i( GLint x, GLint y );

GLAPI void GLAPIENTRY OGL_glVertex2f( GLfloat x, GLfloat y );

GLAPI void GLAPIENTRY OGL_glVertex3f( GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY OGL_glViewport( GLint x, GLint y, GLsizei width, GLsizei height );

GLAPI void GLAPIENTRY OGL_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz );

GLAPI void GLAPIENTRY OGL_glEnd( void );

GLAPI void GLAPIENTRY OGL_glFlush( void );

GLAPI void GLAPIENTRY OGL_glEnableClientState( GLenum cap );

GLAPI void GLAPIENTRY OGL_glDrawArrays( GLenum mode, GLint first, GLsizei count );

GLAPI void GLAPIENTRY OGL_glDisableClientState( GLenum cap );

GLAPI void GLAPIENTRY OGL_glEnableVertexAttribArrayARB (GLuint index);

GLAPI void GLAPIENTRY OGL_glDisableVertexAttribArrayARB (GLuint index);

GLAPI void GLAPIENTRY OGL_glVertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                                                GLboolean normalized, GLsizei stride, const GLvoid *pointer);

GLAPI void GLAPIENTRY OGL_glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glIndexPointer( GLenum type, GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glEdgeFlagPointer( GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY OGL_glArrayElement( GLint i );

GLAPI void GLAPIENTRY OGL_glInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer );

GLAPI void GLAPIENTRY OGL_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count,
                                           GLenum type, const GLvoid *indices );

GLAPI void GLAPIENTRY OGL_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

GLAPI void GLAPIENTRY OGL_glLoadMatrixf( const GLfloat *m );

GLAPI void GLAPIENTRY OGL_glMultMatrixd( const GLdouble *m );

GLAPI void GLAPIENTRY OGL_glMultMatrixf( const GLfloat *m );

GLAPI void GLAPIENTRY OGL_glTranslated( GLdouble x, GLdouble y, GLdouble z );

GLAPI void GLAPIENTRY OGL_glTranslatef( GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY OGL_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );

GLAPI void GLAPIENTRY OGL_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY OGL_glScalef( GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY OGL_glEnable( GLenum cap );

GLAPI void GLAPIENTRY OGL_glDisable( GLenum cap );

GLAPI void GLAPIENTRY OGL_glBindProgramARB (GLenum target, GLuint pid);

GLAPI const GLubyte* GLAPIENTRY OGL_glGetString( GLenum name );

GLAPI void GLAPIENTRY OGL_glProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat *params);

GLAPI void GLAPIENTRY OGL_glProgramEnvParameters4fvEXT (GLenum a,GLuint b,GLsizei c,const GLfloat* d);

GLAPI void GLAPIENTRY OGL_glProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat *v);

GLAPI void GLAPIENTRY OGL_glProgramLocalParameter4fARB (GLenum target, GLuint index,
                                                  GLfloat x, GLfloat y, GLfloat z, GLfloat w);

GLAPI void GLAPIENTRY OGL_glProgramLocalParameters4fvEXT (GLenum a,GLuint b,GLsizei c,const GLfloat* d);

GLAPI void GLAPIENTRY OGL_glProgramStringARB (GLenum target, GLenum format,
                                        GLsizei len, const GLvoid * str);

GLAPI void GLAPIENTRY OGL_glLightfv( GLenum light, GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY OGL_glMaterialfv( GLenum face, GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY OGL_glLightModelfv( GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY OGL_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

GLAPI void GLAPIENTRY OGL_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );

GLAPI void GLAPIENTRY OGL_glColor4fv( const GLfloat *v );

GLAPI void GLAPIENTRY OGL_glMaterialf( GLenum face, GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glLightModelf( GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glLightModeli( GLenum pname, GLint param );

GLAPI void GLAPIENTRY OGL_glLightf( GLenum light, GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glNormal3fv( const GLfloat *v );

GLAPI void GLAPIENTRY OGL_glVertex3fv( const GLfloat *v );

GLAPI void GLAPIENTRY OGL_glDepthFunc( GLenum func );

GLAPI void GLAPIENTRY OGL_glDepthMask( GLboolean flag );

GLAPI void GLAPIENTRY OGL_glClearDepth( GLclampd depth );

GLAPI void GLAPIENTRY OGL_glCullFace( GLenum mode );

GLAPI void GLAPIENTRY OGL_glFrontFace( GLenum mode );

GLAPI void GLAPIENTRY OGL_glPushMatrix( void );

GLAPI void GLAPIENTRY OGL_glPopMatrix( void );

GLAPI void GLAPIENTRY OGL_glActiveTextureARB(GLenum texture);

GLAPI void GLAPIENTRY OGL_glBindTexture( GLenum target, GLuint texture );

GLAPI void GLAPIENTRY OGL_glDeleteTextures(GLsizei n, const GLuint *textures);

GLAPI void GLAPIENTRY OGL_glTexParameteri( GLenum target, GLenum pname, GLint param );

GLAPI void GLAPIENTRY OGL_glTexParameterf( GLenum target, GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glPixelStorei( GLenum pname, GLint param );

GLAPI void GLAPIENTRY OGL_glCompressedTexImage2DARB( GLenum target, GLint level,
                                                 GLenum internalFormat,
                                                 GLsizei width, GLsizei height,
                                                 GLint border, GLsizei imageSize,
                                                 const GLvoid *data );

GLAPI void GLAPIENTRY OGL_glCompressedTexImage2D( GLenum target, GLint level,
                                              GLenum internalFormat,
                                              GLsizei width, GLsizei height,
                                              GLint border, GLsizei imageSize,
                                              const GLvoid *data );

GLAPI void GLAPIENTRY OGL_glCompressedTexSubImage2DARB (    GLenum target,
                                                            GLint level,
                                                            GLint xoffset,
                                                            GLint yoffset,
                                                            GLsizei width,
                                                            GLsizei height,
                                                            GLenum format,
                                                            GLsizei imageSize,
                                                            const GLvoid *data);

GLAPI void GLAPIENTRY OGL_glTexImage2D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels );

GLAPI void GLAPIENTRY OGL_glTexImage3D( GLenum target, GLint level,
                                        GLint internalFormat,
                                        GLsizei width, GLsizei height,
                                        GLsizei depth, GLint border,
                                        GLenum format, GLenum type,
                                        const GLvoid *pixels );


GLAPI void GLAPIENTRY OGL_glTexSubImage2D( GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels );

GLAPI void GLAPIENTRY OGL_glCopyTexImage2D( GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLsizei height,
                                        GLint border );


GLAPI void GLAPIENTRY OGL_glCopyTexSubImage2D( GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height );

GLAPI void GLAPIENTRY OGL_glTexCoord2f( GLfloat s, GLfloat t );

GLAPI void GLAPIENTRY OGL_glMultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t );

GLAPI void GLAPIENTRY OGL_glMultiTexCoord2fv( GLenum texture, const GLfloat *v );

GLAPI void GLAPIENTRY OGL_glTexGeni(GLenum coord, GLenum pname, GLint param);

GLAPI void GLAPIENTRY OGL_glTexGenfv(GLenum coord, GLenum pname, const GLfloat* param);

GLAPI void GLAPIENTRY OGL_glTexGenf( GLenum coord, GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glClientActiveTextureARB(GLenum texture);

GLAPI void GLAPIENTRY OGL_glTexEnvf( GLenum target, GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glTexEnvi( GLenum target, GLenum pname, GLint param );

GLAPI void GLAPIENTRY OGL_glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY OGL_glFogf( GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY OGL_glFogi( GLenum pname, GLint param );

GLAPI void GLAPIENTRY OGL_glFogfv( GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY OGL_glFogiv( GLenum pname, const GLint *params );

GLAPI void GLAPIENTRY OGL_glStencilFunc( GLenum func, GLint ref, GLuint mask );

GLAPI void GLAPIENTRY OGL_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass );

GLAPI void GLAPIENTRY OGL_glStencilOpSeparateATI(GLenum face, GLenum fail, GLenum zfail, GLenum zpass );

GLAPI void GLAPIENTRY OGL_glClearStencil( GLint s );

GLAPI void GLAPIENTRY OGL_glStencilMask( GLuint mask );

GLAPI void GLAPIENTRY OGL_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );

GLAPI void GLAPIENTRY OGL_glAlphaFunc( GLenum func, GLclampf ref );

GLAPI void GLAPIENTRY OGL_glBlendFunc( GLenum sfactor, GLenum dfactor );

GLAPI void GLAPIENTRY OGL_glBlendEquation( GLenum mode );

GLAPI void GLAPIENTRY OGL_glBlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );

GLAPI void GLAPIENTRY OGL_glColorMaterial( GLenum face, GLenum mode );

GLAPI void GLAPIENTRY OGL_glScissor( GLint x, GLint y, GLsizei width, GLsizei height);

GLAPI void GLAPIENTRY OGL_glDepthRange( GLclampd near_val, GLclampd far_val );

GLAPI void GLAPIENTRY OGL_glPolygonOffset( GLfloat factor, GLfloat units );

GLAPI void GLAPIENTRY OGL_glPushAttrib( GLbitfield mask );

GLAPI void GLAPIENTRY OGL_glPopAttrib( void );

GLAPI void GLAPIENTRY OGL_glPolygonMode (GLenum face, GLenum mode);

GLAPI void GLAPIENTRY OGL_glBlendEquationEXT (GLenum mode);

#endif // OGLENTRYPOINTS
