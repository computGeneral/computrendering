/**************************************************************************
 *
 */

#include "GALxCodeSnip.h"
#include <algorithm>
#include <iostream>
#include "support.h"
#include <typeinfo>

using namespace std;
using namespace libGAL;

GALxCodeSnip::~GALxCodeSnip()
{}

void GALxCodeSnip::dump() const
{
    cout << text;
}

string GALxCodeSnip::toString() const
{
    return text;
}

void GALxCodeSnip::setCode(const std::string& code)
{
    text = code;
}


void GALxCodeSnip::removeComments()
{
    string::size_type pos = text.find_first_of('#');
    string::size_type pos2;
    while ( pos != string::npos )
    {
        pos2 = text.find_first_of('\n', pos);
        if ( pos2 == string::npos )
            pos2 = text.length();

        text = text.erase(pos, pos2 - pos); // remove comment
        pos = text.find_first_of('#');
    }
}



GALxCodeSnip& GALxCodeSnip::append(const GALxCodeSnip& cn)
{
    if ( typeid(*this) != typeid(GALxCodeSnip) ) // Derivatives classes are read-only 
        CG_ASSERT("Only GALxCodeSnip base allows appending");

    text += cn.text;

    return *this;
}

GALxCodeSnip& GALxCodeSnip::append(std::string instr)
{
    if ( typeid(*this) != typeid(GALxCodeSnip) ) // Derivatives classes are read-only 
        CG_ASSERT("Only GALxCodeSnip base allows appending");

    text += instr;
    return *this;
}

GALxCodeSnip& GALxCodeSnip::clear()
{
    if ( typeid(*this) != typeid(GALxCodeSnip) )
        CG_ASSERT("Only GALxCodeSnip base allows clearing");

    text = "";
    return *this;
}
