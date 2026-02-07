/**************************************************************************
 *
 * Validation Info definition file.
 *
 */

#ifndef _VALIDATION_INFO_

#define _VALIDATION_INFO_

#include "GPUType.h"
#include "GPUReg.h"
#include <map>

namespace cg1gpu
{

/**
 *
 *  Defines the identifier for a vertex.
 *
 */

struct VertexID
{
    U32 instance;
    U32 index;
    
    VertexID() : instance(0), index(0) {}
    VertexID(U32 instance, U32 index) : instance(instance), index(index) {}
    VertexID(U32 &index) : instance(0), index(index) {}
    
    bool operator==(const VertexID &in) const
    {
        return ((instance == in.instance) && (index == in.index));
    }
    
    bool operator<(const VertexID &in) const
    {
        return ((instance < in.instance) || ((instance == in.instance) && (index < in.index)));
    }
};

/**
 *
 *  Defines information associated with a vertex input.
 *
 */

struct VertexInputInfo
{
    VertexID vertexID;
    U32 timesRead;
    bool differencesBetweenReads;
    Vec4FP32 attributes[MAX_VERTEX_ATTRIBUTES];
};

typedef std::map<VertexID, VertexInputInfo> VertexInputMap;

/**
 *
 *  Defines information associated with shaded vertices.
 *
 */

struct ShadedVertexInfo
{
    VertexID vertexID;
    U32 timesShaded;
    bool differencesBetweenShading;
    Vec4FP32 attributes[MAX_VERTEX_ATTRIBUTES];
};

typedef std::map<VertexID, ShadedVertexInfo> ShadedVertexMap;

/**
 *
 *  Defines a fragment identifier and the operations required to
 *  use the class as a map key.
 *
 */
 
struct FragmentID
{
    U32 x;
    U32 y;
    U32 sample;
    U32 triangleID;
    
    FragmentID(U32 triangleID, U32 x, U32 y, U32 sample) : triangleID(triangleID), x(x), y(y), sample(sample) {}
    FragmentID() : triangleID(0), x(0), y(0), sample(0) {}
    
    bool operator==(const FragmentID &in) const
    {
        return ((triangleID == in.triangleID) && (x == in.x) && (y == in.y) && (sample == in.sample));
    };
    
    bool operator<(const FragmentID &in) const
    {
        return ((triangleID < in.triangleID) ||
                ((triangleID == in.triangleID) && ((x < in.x) ||
                                                   ((x == in.x) && ((y < in.y) || ((y == in.y) && (sample < in.sample)))))));
    }   
};
 

/**
 *
 *  Defines information associated with a fragment quad (2x2 fragments) that is updating memory.
 *
 */

struct FragmentQuadMemoryUpdateInfo
{
    static const U32 MAX_BYTES_PER_PIXEL = 16;
    
    FragmentID fragID;
    U08 inData[4 * MAX_BYTES_PER_PIXEL];
    U08 readData[4 * MAX_BYTES_PER_PIXEL];
    U08 writeData[4 * MAX_BYTES_PER_PIXEL];
    bool writeMask[4 * MAX_BYTES_PER_PIXEL];
    bool cullMask[4];
};

typedef std::map<FragmentID, FragmentQuadMemoryUpdateInfo> FragmentQuadMemoryUpdateMap;

};  // namespace cg1gpu

#endif