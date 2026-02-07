/**************************************************************************
 *
 */

#ifndef GLISTAT_H
    #define GLISTAT_H

#include <string>

/**
 * Common interface for all user-defined stats
 */
class GLIStat
{
private:

    std::string name;

protected:

    /**
     * Must be called in each subclass
     */
    GLIStat::GLIStat(const std::string& name) : name(name) {}

public:

    /**
     * Returns the name of the statistic instance
     */
    const std::string& getName() { return name; }

    /**
     * This method is invoked every time a batch finishes
     *
     * @note It should return the count of this statistic during last batch, 
     *       afterwards it should reset its internal batch counter.
     */
    virtual int getBatchCount() = 0;

    /**
     * This method is invoked every time a frame finishes
     *
     * @note It should return the count of this statistic during last frame,
     *       afterwards it should reset its internal frame counter
     */
    virtual int getFrameCount() = 0;

};




#endif
