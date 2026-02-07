/**************************************************************************
 *
 */

#include "GLExecStats.h"

GLExecStats GLExecStats::stats;

void GLAPIENTRY end_USER()
{
    GLExecStats::instance().incBatchCount(1);
}

void GLAPIENTRY drawArrays_USER( GLenum mode, GLint first, GLsizei count )
{
    GLExecStats::instance().incBatchCount(1);
    GLExecStats::instance().incVertexCount(count);
}

void GLAPIENTRY drawElements_USER( GLenum, GLsizei count, GLenum , const GLvoid *)
{
    GLExecStats::instance().incBatchCount(1);
    GLExecStats::instance().incVertexCount(count);
}

void GLAPIENTRY drawRangeElements_USER( GLenum, GLuint, GLuint, GLsizei count, GLenum , const GLvoid *)
{
    GLExecStats::instance().incBatchCount(1);
    GLExecStats::instance().incVertexCount(count);
}
