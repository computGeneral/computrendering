/**************************************************************************
 *
 */

#ifndef BASICSTATS_H
    #define BASICSTATS_H

#include "GLIStat.h"

class VertexStat : public GLIStat
{
private:

    int batchCount;
    int frameCount;

public:

    VertexStat(const std::string& name);

    /**
     * Returns the count in the vertex-per-batch counter, add the vertex-per-batch 
     * counter value to the vertex-per-frame counter and finally resets the 
     * vertex-per-batch counter
     */
    virtual int getBatchCount();

    /**
     * Returns the count in the vertex-per-frame counter and afterwards resets this
     * internal counter
     */
    virtual int getFrameCount();

    /**
     * Increase the current vertex count in the vertex-per-batch counter
     */
    void addVertexes(int vertexCount);

};

class TriangleStat : public GLIStat
{
private:

    int batchCount;
    int frameCount;

public:

    TriangleStat(const std::string& name);

    virtual int getBatchCount();

    virtual int getFrameCount();

    void addTriangles(unsigned int primitive, int vertexCount);

};


#endif