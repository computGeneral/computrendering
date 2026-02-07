/**************************************************************************
 *
 */

#ifndef GALx_CODESNIP_H
    #define GALx_CODESNIP_H

#include <vector>
#include <string>

namespace libGAL
{

class GALxCodeSnip
{
private:

    std::string text;

protected:

    void setCode(const std::string& code);

public:    
    
    // final (no virtual)
    GALxCodeSnip& append(const GALxCodeSnip& cn);
    GALxCodeSnip& append(std::string instr);
    GALxCodeSnip& clear();
    void removeComments();
    
    virtual void dump() const;
    virtual std::string toString() const;
    virtual ~GALxCodeSnip();
};

} // namespace libGAL


#endif // GALx_CODESNIP_H
