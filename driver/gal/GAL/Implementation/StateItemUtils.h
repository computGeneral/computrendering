/**************************************************************************
 *
 */

#ifndef STATEITEMUTILS
    #define STATEITEMUTILS

#include "StateItem.h"
#include <sstream>
#include <string>

namespace libGAL
{
/**
 * PrintFunc class
 * 
 * Allows defining print methods to be used in the stateItemString function
 * just deriving and specializing the class. 
 *
 * @note The print() method returns a const char * (pointer), so the
 * char array in memory must be available after the function return.
 * This can only be ensured by the standard C++ if internaly the print
 * function returns something in the form:
 *
 *            return "VALUE"; or
 *            return stringObject.str().c_str(); (where stringObject is
 *                                                a dynamically 
 *                                                allocated C++ string).
 *
 * In the second case, however, the dynamically allocated C++ string must
 * be properly deleted externally.
 *
 */

template<class T>
class PrintFunc
{
public:
    virtual const char *print(const T& var) const = 0;
};

class BoolPrint: public PrintFunc<gal_bool>
{
public:
    virtual const char* print(const gal_bool& var) const {  return (var? "TRUE" : "FALSE"); }

};

template<class T>
std::string stateItemString(const StateItem<T>& si, const gal_char* siName, gal_bool notSync, const PrintFunc<T>* pFunc = 0)
{
    std::stringstream ss;
    
    if (!pFunc)
        ss << siName << " = " << si;
    else
        ss << siName << " = " << pFunc->print(si);
    
    if ( notSync )
        ss << " NotSync = ?\n";
    else {
        if ( si.changed() )
        {
            ss << " NotSync = ";
            if (!pFunc)
                ss << si.initial() << "\n";
            else
                ss << pFunc->print(si.initial()) << "\n";
        }
        else
            ss << " Sync\n";
    }

    return ss.str();
}

}

#endif // STATEITEMUTILS

