/**************************************************************************
 *
 * Setup Triangle definition file.
 *
 */

/**
 *
 *  @file SetupTriangle.h
 *
 *  Defines the Setup Triangle class.
 *  This class defines a Triangle and its associated data
 *  in the Rasterization and Triangle Setups stages of the
 *  3D graphic pipeline.
 *
 *
 */

#include "GPUType.h"
#include "support.h"
#include "DynamicMemoryOpt.h"
#include "FixedPoint.h"

#ifndef _SETUPTRIANGLE_

#define _SETUPTRIANGLE_

namespace cg1gpu
{

//*  Maximum saved positions per triangle.  
static const U32 MAX_SAVED_POSITIONS = 8;


//*  Defines the rasterization directions.  
//enum RasterDirection
//{
//   CENTER_DIR,     //  Rasterize from a center fragment of the triangle.  
//    UP_DIR,         //  Rastarize triangle up.  
//    DOWN_DIR,       //  Rasterize triangle down.  
//    DOWN_LEFT_DIR,  //  Rasterize triangle to the left and down.  
//    DOWN_RIGHT_DIR, //  Rasterize triangle to the right and down.  
//    UP_LEFT_DIR,    //  Rasterize triangle to the left and up.  
//    UP_RIGHT_DIR,   //  Rasterize triangle to the right and up.  
//    CENTER_LEFT_DIR,    //  Rasterize triangle to the left up and down.  
//    CENTER_RIGHT_DIR    //  Rasterize triangle to the right up and down.  
//};

enum RasterDirection
{
    //  Basic directions.  
    UP_DIR      = 0x01,         //  Rastarize triangle up.  
    DOWN_DIR    = 0x02,         //  Rasterize triangle down.  
    LEFT_DIR    = 0x04,         //  Rasterize triangle to the left.  
    RIGHT_DIR   = 0x08,         //  Rasterize triangle to the right.  

    //  Composite directions.  
    CENTER_DIR          = 0x03, //  Rasterize in the center (up and down).  
    CENTER_LEFT_DIR     = 0x0f, //  Rasterize to the center left (up and down, left and right).  
    CENTER_RIGHT_DIR    = 0x0b, //  Rasterize to the center (up, down, right).  
    UP_LEFT_DIR         = 0x0d, //  Rasterize up and left (up, left, right).  
    UP_RIGHT_DIR        = 0x09, //  Rasterize up and right (up, right).  
    DOWN_LEFT_DIR       = 0x0e, //  Rasterize down and left (down, left, right).  
    DOWN_RIGHT_DIR      = 0x0a, //  Rasterize down and right (down, right).  

    //  Border traversal.  
    BORDER_DIR          = 0x10, //  Traverse the bounding mdu border.  
    TOP_BORDER_DIR      = 0x11, //  Traverse the bounding mdu top border.  
    BOTTOM_BORDER_DIR   = 0x12, //  Traverse the bounding mdu bottom border.  
    LEFT_BORDER_DIR     = 0x13, //  Traverse the bounding mdu left border.  
    RIGHT_BORDER_DIR    = 0x14  //  Traverse the bounding mdu right border.  
};


/**
 *
 *  Defines the saved position identifiers.
 *
 */
enum SavedPosition
{
    RIGHT_SAVE = 0,     //  Right save position.  
    UP_SAVE,            //  Up save position.  
    DOWN_SAVE,          //  Down save position.  
    TILE_LEFT_SAVE,     //  Tile left save position.  
    TILE_RIGHT_SAVE,    //  Tile right save position.  
    TILE_DOWN_SAVE,     //  Tile down save position.  
    TILE_UP_SAVE,       //  Tile up save position.  
    RASTER_START_SAVE   //  Rasterization start position.  
};



/**
 *
 *  Defines a Setup Triangle and its associated data.
 *
 *  This class stores the data associated with a triangle
 *  in the setup and rasterization stages of a hardware
 *  3D graphic pipeline.
 *
 *  This class inherits from the DynamicMemoryOpt object that provides custom optimized
 *  memory allocation and deallocation support
 *
 */

class SetupTriangle : public DynamicMemoryOpt
{

private:

    Vec4FP32 *vertex1;     //  First triangle vertex attribute array.  
    Vec4FP32 *vertex2;     //  Second triangle vertex attribute array.  
    Vec4FP32 *vertex3;     //  Third triangle vertex attribute array.  
    F64 area;            //  The triangle signed area (or an approximation to the area).  
    F64 edge1[3];        //  First edge coefficients.  
    F64 edge2[3];        //  Second edge coefficients.  
    F64 edge3[3];        //  Third edge coefficients.  
    F64 zEq[3];          //  Z interpolation equation coefficients.  
    F64 savedEdge[MAX_SAVED_POSITIONS][4];   //  Saved edge and z interpolation position.  
    S32 savedRaster[MAX_SAVED_POSITIONS][2]; //  Saved (x,y) raster position.  
    bool isSaved[MAX_SAVED_POSITIONS];      //  Stores if a saved position has valid data.  
    bool lastFragmentFlag;  //  Flag storing if the last triangle fragment has been generated.  
    S32 x;               //  Current x raster position inside the triangle.  
    S32 y;               //  Current y raster position inside the triangle.  
    U08 direction;        //  The current rasterization direction for the triangle.  
    U08 tileDirection;    //  The current rasterization direction for tiles inside the triangle.  
    U32 references;      //  Setup Triangle references counter.  Used for 'garbage collection'/object destruction.  
    bool firstStamp;        //  Flag signaling if the first stamp of the triangle hasn't been generated yet.  
    S32 minX;            //  Defines the triangle bounding mdu (minimum x) inside the viewport.  
    S32 minY;            //  Defines the triangle bounding mdu (minimum y) inside the viewport.  
    S32 maxX;            //  Defines the triangle bounding mdu (maximum x) inside the viewport.  
    S32 maxY;            //  Defines the triangle bounding mdu (maximum y) inside the viewport.  
    F64 screenArea;      //  The triangle size (in percentage of total screen area). 
    Vec4FP32 nHVtxPos[3];  //  The non-homogeneous position coordinates for the three vertices (fourth component is each vertex 1/w).  
    bool preBoundTriangle;  //  Triangle is pre-bound.  
    FixedPoint subXMin;     //  The subpixel bounding mdu (minimum x) stored as a fixed-point fractional value. 
    FixedPoint subYMin;     //  The subpixel bounding mdu (minimum y) stored as a fixed-point fractional value. 
    FixedPoint subXMax;     //  The subpixel bounding mdu (maximum x) stored as a fixed-point fractional value. 
    FixedPoint subYMax;     //  The subpixel bounding mdu (maximum y) stored as a fixed-point fractional value. 
    
public:

    /**
     *
     *  Setup Triangle constructor.
     *
     *  Creates and initializes a new setup triangle from the
     *  three given vertices and their attributes.  The vertices
     *  are passed in CCW for front facing triangles (default face mode).
     *
     *  @param vert1 The triangle first vertex attributes.
     *  @param vert2 The triangle second vertex attributes.
     *  @param vert3 The triangle third vertex attributes.
     *
     */

    SetupTriangle(Vec4FP32 *vert1, Vec4FP32 *vert2, Vec4FP32 *vert3);

    /**
     *
     *  Setup Triangle destructor.
     *
     */

    ~SetupTriangle();

    /**
     *
     *  Sets the triangle three edge equations.
     *
     *  @param edgeEq1 The first edge equation coefficients.
     *  @param edgeEq2 The second edge equation coefficients.
     *  @param edgeEq3 The third edge equation coefficients.
     *
     */

    void setEdgeEquations(F64 *edgeEq1, F64 *edgeEq2, F64 *edgeEq3);

    /**
     *
     *  Sets the triangle z interpolation edge equation.
     *
     *  @param zeq The z interpolation egde equation coefficients.
     *
     */

    void setZEquation(F64 *zeq);

    /**
     *
     *  Gets the triangle three edge equations.
     *
     *  @param edgeEq1 Pointer to the 64 bit float array where to store
     *  the first edge equation coefficients.
     *  @param edgeEq2 Pointer to the 64 bit float array where to store
     *  the second edge equation coefficients.
     *  @param edgeEq1 Pointer to the 64 bit float array where to store
     *  the third edge equation coefficients.
     *
     */

    void getEdgeEquations(F64 *edgeEq1, F64 *edgeEq2, F64 *edgeEq3);

    /**
     *
     *  Gets the triangle z interpolation equation.
     *
     *  @param zeq A pointer to array where to stored the z intepolation
     *  equation coefficients.
     *
     */

    void getZEquation(F64 *zeq);


    /**
     *
     *  Updates the current fragment generation value.
     *
     *  @param updateVal The value with which to update the current position.
     *  @param xP New x raster position.
     *  @param yP New y raster position.
     *
     */

    void updatePosition(F64 *update, S32 xP, S32 yP);

    /**
     *
     *  Saves a rasterization position.
     *
     *  @param save Pointer to the edge and z interpolation
     *  equation values to save.
     *  @param x Horizontal raster position to save.
     *  @param y Vertical raster position to save.
     *  @param savePos Save position where to save the data.
     *
     */

    void save(F64 *save, S32 x, S32 y, SavedPosition savePos);

    /**
     *
     *  Saves the raster start position.
     *
     */

    void saveRasterStart();

    /**
     *
     *  Saves right fragment position state.
     *
     *  @param right Right save position rasterization state.
     *
     */

    void saveRight(F64 *right);

    /**
     *
     *  Saves down fragment position state.
     *
     *  @param down Down save position rasterization state.
     *
     */

    void saveDown(F64 *down);

    /**
     *
     *  Saves up fragment position state.
     *
     *  @param up Up save position rasterization state.
     *
     */

    void saveUp(F64 *up);

    /**
     *
     *  Restores a saved rasterization position.
     *
     *  @param save The saved position identifier.
     *
     */

    void restore(SavedPosition save);


    /**
     *
     *  Restores right saved position state.
     *
     *
     */

    void restoreRight();

    /**
     *
     *  Restores down saved position state.
     *
     *
     */

    void restoreDown();


    /**
     *
     *  Restores up saved position state.
     *
     *
     */

    void restoreUp();


    /**
     *
     *  Restores left tile saved position state.
     *
     *
     */

    void restoreLeftTile();

    /**
     *
     *  Restores right tile saved position state.
     *
     *
     */

    void restoreRightTile();

    /**
     *
     *  Restores down tile saved position state.
     *
     *
     */

    void restoreDownTile();


    /**
     *
     *  Restores up tile saved position state.
     *
     *
     */

    void restoreUpTile();

    /**
     *
     *  Asks if the right position has been already saved.
     *
     *  @return If the right position is saved.
     *
     */

    bool isRightSaved();

    /**
     *
     *  Asks if the down position has been already saved.
     *
     *  @return If the down position is saved.
     *
     */

    bool isDownSaved();

    /**
     *
     *  Asks if the up position has been already saved.
     *
     *  @return If the up position is saved.
     *
     */

    bool isUpSaved();

    /**
     *
     *  Asks if the left tile position has been already saved.
     *
     *  @return If the left tile position is saved.
     *
     */

    bool isLeftTileSaved();

    /**
     *
     *  Asks if the right tile position has been already saved.
     *
     *  @return If the right tile position is saved.
     *
     */

    bool isRightTileSaved();

    /**
     *
     *  Asks if the down tile position has been already saved.
     *
     *  @return If the down tile position is saved.
     *
     */

    bool isDownTileSaved();

    /**
     *
     *  Asks if the up tile position has been already saved.
     *
     *  @return If the up tile position is saved.
     *
     */

    bool isUpTileSaved();

    /**
     *
     *  Sets triangle last fragment flag.
     *
     *
     */

    void lastFragment();

    /**
     *
     *  Asks if the triangle last fragment has been already generated.
     *
     *  @return If the triangle last fragment has been generated.
     *
     */

    bool isLastFragment();

    /**
     *
     *  Sets the triangle as pre-bound triangle.
     *
     */
    void setPreBound();

    /**  
     *
     *  Returns if the triangle is a pre-bound triangle.
     *
     */
    bool isPreBoundTriangle();

    /**
     *
     *  Sets the triangle current raster position.
     *
     *  @param xP The x coordinate of the raster position.
     *  @param yP The y coordinate of the raster position.
     *
     */

    void setRasterPosition(S32 x, S32 y);

    /**
     *
     *  Gets the triangle current raster position.
     *
     *  @param xP Reference to the 32 bit integer where to store the triangle
     *  current rasterization position.
     *  @param yP Reference to the 32bit integer where to store the triangle
     *  current rasterization position.
     *
     */

    void getRasterPosition(S32 &x, S32 &y);

    /**
     *
     *  Changes the triangle rasterization direction.
     *
     *  @param dir The triangle new rasterization direction.
     *
     */

    void setDirection(RasterDirection dir);

    /**
     *
     *  Gets the triangle current rasterization direction.
     *
     *  @return The triangle current rasterization direction.
     *
     */

    U08 getDirection();

    /**
     *
     *  Changes the triangle tile rasterization direction.
     *
     *  @param dir The triangle new tile rasterization direction.
     *
     */

    void setTileDirection(RasterDirection dir);

    /**
     *
     *  Returns if a tile rasterization direction is 'active' (must
     *  save positions).
     *
     *  @param dir The rasterization direction to check.
     *
     *  @return If the rasterization algorithm must save tile positions
     *  in the given rasterization direction.
     *
     */

    bool getTileDirection(RasterDirection dir);

    /**
     *
     *  Gets a vertex attribute from the setup triangle.
     *
     *  @param vertex The triangle vertex from which to get
     *  the attribute.
     *  @param attribute The vertex attribute to get.
     *
     *  @return A pointer to the vertex attribute requested.
     *
     */

    Vec4FP32 *getAttribute(U32 vertex, U32 attribute);


    /**
     *
     *  Gets the triangle three vertex attribute arrays.
     *
     *  @param vAttr1 Reference to a Vec4FP32 pointer where to
     *  store the address of the first vertex attribute array.
     *  @param vAttr2 Reference to a Vec4FP32 pointer where to
     *  store the address of the second vertex attribute array.
     *  @param vAttr2 Reference to a Vec4FP32 pointer where to
     *  store the address of the third vertex attribute array.
     *
     */

    void getVertexAttributes(Vec4FP32 *&vAttr1, Vec4FP32 *&vAttr2,
        Vec4FP32 *&vAttr3);


    /**
     *
     *  Sets the triangle signed area.
     *
     *  @param area The triangle signed area (or an approximation).
     *
     */

    void setArea(F64 area);

     /**
     *  Sets the triangle size in percentage of screen area
     *
     *  @param size The triangle size. 
     *
     */
     void setScreenPercent(F64 size);

    /**
     *
     *  Gets the triangle signed area.
     *
     *  @return The triangle signed area (or an approximation to that area).
     *
     */

    F64 getArea();

   /**
    *
    *  Gets the triangle size in percentage of screen area.
    *
    *  @return The triangle size. 
    *
    */

    F64 getScreenPercent();

    /**
     *
     *  Sets the vertex non-homogeneous position coordinates (divided by W).
     *
     *  @param vtx  The vertex identifier [0-2].
     *  @param nHPos The vertex's non-homogeneous position coordinates.
     *
     */

    void setNHVtxPosition(U32 vtx, Vec4FP32 nHPos);

    /**
     *
     *  Gets the vertex non-homogeneous position coordinates (divided by W).
     *
     *  @param vtx  The vertex identifier [0-2].
     *  @param nHPosOut The returned vertex's non-homogeneous position coordinates.
     *
     */

    void getNHVtxPosition(U32 vtx, Vec4FP32& nHPosOut);

    /**
     *
     *  Inverts the sign of the triangle edge equations.
     *
     */

    void invertEdgeEquations();

    /**
     *
     *  Increments the setup triangle reference counter.
     *
     *
     */

    void newReference();


    /**
     *
     *  Decrements the setup triangle reference counter.
     *
     *  If the number of objects referencing the setup triangle object
     *  is 0 the object is deleted.
     *
     */

    void deleteReference();

    /**
     *
     *  Sets the first stamp generated flag.
     *
     *  @param flag The value that the first stamp flag will take.
     *
     */

    void setFirstStamp(bool flag);

    /**
     *
     *  Returns the value of the first stamp flag.
     *
     *  @return The current value of the first stamp flag.
     *
     */

   bool isFirstStamp();

    /**
     *
     *  Sets the triangle bounding mdu.
     *
     *  @param x0 Minimum x for the bounding mdu.
     *  @param y0 Minimum y for the bounding mdu.
     *  @param x1 Maximum x for the bounding mdu.
     *  @param y1 Maximum y for the bounding mdu.
     *
     */

    void setBoundingBox(S32 x0, S32 y0, S32 x1, S32 y1);

    /**
     *
     *  Sets the subpixel triangle bounding mdu in fixed-point fractional values.
     *
     *  @param subXMin Minimum x for the bounding mdu (fixed-point).
     *  @param subYMin Minimum y for the bounding mdu (fixed-point).
     *  @param subXMax Maximum x for the bounding mdu (fixed-point).
     *  @param subYMax Maximum y for the bounding mdu (fixed-point).
     *
     */

    void setSubPixelBoundingBox(FixedPoint subXMin, FixedPoint subYMin, FixedPoint subXMax, FixedPoint subYMax);

    /**
     *
     *  Gets the triangle bounding mdu.
     *
     *  @param x0 Reference to a variable where to store the minimum x for the bounding mdu.
     *  @param y0 Reference to a variable where to store the minimum y for the bounding mdu.
     *  @param x1 Reference to a variable where to store the maximum x for the bounding mdu.
     *  @param y1 Reference to a variable where to store the maximum y for the bounding mdu.
     *
     */

    void getBoundingBox(S32 &x0, S32 &y0, S32 &x1, S32 &y1);

    /**
     *
     *  Gets the subpixel triangle bounding mdu in fixed-point fractional values.
     *
     *  @param subXMin Reference to a variable where to store the minimum x for the bounding mdu (fixed-point).
     *  @param subYMin Reference to a variable where to store the minimum y for the bounding mdu (fixed-point).
     *  @param subXMax Reference to a variable where to store the maximum x for the bounding mdu (fixed-point).
     *  @param subYMax Reference to a variable where to store the maximum y for the bounding mdu (fixed-point).
     *
     */

    void getSubPixelBoundingBox(FixedPoint& subXmin, FixedPoint& subYmin, FixedPoint& subXmax, FixedPoint& subYmax);
        
}; // SetupTriangle

} // namespace cg1gpu


#endif
