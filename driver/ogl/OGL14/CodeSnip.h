/**************************************************************************
 *
 */

#ifndef CODESNIP_H
    #define CODESNIP_H

#include <vector>
#include <string>

class CodeSnip
{
private:

    std::string text;

protected:

    void setCode(const std::string& code);

public:    
    
    // final (no virtual)
    CodeSnip& append(const CodeSnip& cn);
    CodeSnip& append(std::string instr);
    CodeSnip& clear();
    void removeComments();
    
    virtual void dump() const;
    virtual std::string toString() const;
    virtual ~CodeSnip();
};


#endif
