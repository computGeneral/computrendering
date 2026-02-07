/**************************************************************************
 *
 */

#ifndef BUFFEROBJECT_H
    #define BUFFEROBJECT_H

#include "BaseObject.h"
#include "GPUType.h"
#include "gl.h"

namespace libgl
{

class BufferTarget;

class BufferObject : public BaseObject
{
private:    
    
    GLsizei size;
    GLenum usage;
    GLubyte* data; // asociated data    
        
    static bool checkUsage(GLenum u);
    
public:    

    BufferObject(GLuint name, GLenum targetName);
    
    // Must be implemented (pure virtual in BaseObject)
    GLuint binarySize(GLuint portion = 0);
    const GLubyte* binaryData(GLuint portion = 0);
    
    GLenum getUsage() const;
    
    /**
     * Allows using this pointer to write into the buffer
     *
     * (allows to implement buffer map/unmap interface)
     */
    GLubyte* getData();
    
    /**
     * @see glBufferData in opengl spec
     */
    void setContents(GLsizei size, const GLubyte* data, GLenum usage);
    
    void setContents(GLsizei size, GLenum usage); // change just size & usage (discard previous data)
    
    /**
     * @see glBufferSubData in opengl spec
     */
    void setPartialContents(GLsizei offset, GLsizei size, const void* data);
    
    ~BufferObject();
    
    /**
     * 
     */
    void printfmt(GLsizei components, GLenum type, GLsizei start, GLsizei stride, GLsizei count );
    
    static void printfmt(const GLubyte* data, GLsizei components, GLenum type, GLsizei start, GLsizei stride, GLsizei count );

};

} // namespace libgl

#endif // BUFFEROBJECT_H
