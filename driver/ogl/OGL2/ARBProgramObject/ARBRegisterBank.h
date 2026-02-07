/**************************************************************************
 *
 */

#ifndef ARB_REGISTERBANK
    #define ARB_REGISTERBANK

#include "gl.h"

#include <vector>
#include <set>

namespace ogl
{

class ARBRegisterBank
{
public:

    ARBRegisterBank(GLuint registerCount);

    void restartTracking();

    std::vector<GLuint> getModified() const;

    void set(GLuint reg, GLfloat coord1, GLfloat coord2, GLfloat coord3, GLfloat coord4, GLubyte mask = 0xF);
    void set(GLuint reg, const GLfloat* coords, GLubyte mask = 0xF);

    void get(GLuint reg, GLfloat& coord1, GLfloat& coord2, GLfloat& coord3, GLfloat& coord4) const;
    void get(GLuint reg, GLfloat* coords) const;
    const GLfloat* get(GLuint reg) const;

private:

    const GLuint _RegisterCount;

    typedef GLfloat _Register[4];

    _Register* _registers; // register bank

    std::set<GLuint> _modified; // register tracking
};

} // namespace ogl

#endif // ARB_REGISTERBANK
