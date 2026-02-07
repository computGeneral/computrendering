/**************************************************************************
 *
 */

#ifndef CHECKSTATS_H
    #define CHECKSTATS_H

#include "GLIStat.h"

/**
 * This statistic is used to check if a GL_ALPHA_TEST and GL_FRAGMENT_PROGRAM_ARB
 * are enabled at the same time
 */
class FPAndAlphaStat : public GLIStat
{
private:

    bool fp;
    bool alpha;
    int count; // times that fp && alpha where true during this frame

public:

    FPAndAlphaStat(const std::string& name);

    virtual int getBatchCount();

    virtual int getFrameCount();

    void setFPFlag(bool enable);

    void setAlphaFlag(bool enable);

};


#endif