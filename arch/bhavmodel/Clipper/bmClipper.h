/**************************************************************************
 *
 * Clipper Behavior Model definition file. 
 *
 */
 
/**
 * 
 *  @file bmClipper.h
 *
 *  Defines the Clipper Behavior Model class.
 *
 *  This class provides triangle clipping functions to the
 *  clipper simulator.
 * 
 *
 */
 
#ifndef _CLIPPEREMULATOR_

#define _CLIPPEREMULATOR_ 

#include "GPUType.h"
#include "GPUReg.h"
 
namespace cg1gpu
{

/**
 *
 *  Maximum number of user clip planes supported.
 *  @note (carlos) it was defined previously in GPUReg.h
 */
//static const U32 MAX_USER_CLIP_PLANES = 6;

/**
 * 
 *  Maximum number of clip vertices that can be stored in the
 *  clipper behaviorModel.
 *
 */
static const U32 MAX_CLIP_VERTICES = 12;


/**
 *
 *  Implements triangle clipping functions. 
 *
 *  This class implements clipping functions for the clipper unit
 *  in the GPU simulator.
 *
 */
 
class bmoClipper
{

private:

    Vec4FP32 userClipPlanes[MAX_USER_CLIP_PLANES]; //  User clip planes.  
    bool userPlanes[MAX_USER_CLIP_PLANES];          //  Active user clip planes.  
    Vec4FP32 *clipVertex[MAX_CLIP_VERTICES];       //  Table with the generated clip vertices.  
    U32 numClipVertex;                           //  Number of clip vertices produced by the last clip operation. 
    
public:

    /**
     *
     *  Clipper behaviorModel constructor.
     *
     *  Creates and initializes a new clipper behaviorModel object.
     *
     *  @return A new initializec clipper behaviorModel object.
     *
     */
     
    bmoClipper();

    /**
     *
     *  Tests the triangle three vertices against the frustum clip
     *  volume and performs a trivial reject test
     *
     *  @param v1 The triangle first vertex coordinates in homogeneous space.
     *  @param v2 The triangle second vertex coordinates in homogeneous space.
     *  @param v3 The triangle third vertex coordinates in homogeneous space.
     *  @param d3d9DepthRange A boolean value that defines the range in clip space
     *  for the depth component, if TRUE uses [0, 1] to emulate D3D9, if FALSE
     *  used [-1, 1] to emulate OpenGL.
     *     
     *  @return If the triangle can be trivially rejected (completely
     *  outside of the frustum clip volume).
     *
     */
     
    static bool trivialReject(Vec4FP32 v1, Vec4FP32 v2, Vec4FP32 v3, bool d3d9DepthRange);
    
    /**
     *
     *  Clips a triangle against the frustum clip volume.
     *
     *  @param v1 The triangle first vertex attributes.
     *  @param v2 The triangle second vertex attributes.
     *  @param v3 The triangle third vertex attributes.
     *
     *  @return If the clip operation has produced new vertices.
     *
     */
     
    bool frustumClip(Vec4FP32 *v1, Vec4FP32 *v2, Vec4FP32 *v3);
    
    /**
     *
     *  Clips a triangle against the user defined clip planes.
     *
     *  @param v1 The triangle first vertex attributes.
     *  @param v2 The triangle second vertex attriutes.
     *  @param v3 The triangle third vertex attributes.
     *
     *  @return If the clip operation has produced new vertices.
     *
     */
     
    bool userClip(Vec4FP32 *v1, Vec4FP32 *v2, Vec4FP32 *v3);
    
    /**
     *
     *  Returns the next clipped vertex produced 
     *
     *  @return The next clip vertex and attributes produced by
     *  the previous clip operation.
     *
     */
     
    Vec4FP32 *getNextClipVertex();

    /**
     *
     *  Defines a new user clip plane.
     *
     *  @param planeID The user clip plane to be defined.
     *  @param plane The user clip plane.
     *
     */
     
    void defineClipPlane(U32 planeID, Vec4FP32 plane);
    
    /**
     *
     *  Undefines an user clip plane.
     *
     *  @param planeID The user clip plane to undefine.
     *
     */
    
    void undefineClipPlane(U32 planeID);
    
        
};

} // namespace cg1gpu

#endif
