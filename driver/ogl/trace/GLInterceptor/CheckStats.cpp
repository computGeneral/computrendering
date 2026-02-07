/**************************************************************************
 *
 */

#include "CheckStats.h"

using namespace std;

FPAndAlphaStat::FPAndAlphaStat(const string& name) : GLIStat(name), 
    fp(false), alpha(false),count(0)
{}

void FPAndAlphaStat::setFPFlag(bool enable)
{
    fp = enable;
}

void FPAndAlphaStat::setAlphaFlag(bool enable)
{
    alpha = enable;
}

int FPAndAlphaStat::getBatchCount()
{
    if ( fp && alpha )
    {
        count++;
        return 1;
    }
    return 0;
}

int FPAndAlphaStat::getFrameCount()
{
    int temp = count;
    count = 0;
    return temp;
}
