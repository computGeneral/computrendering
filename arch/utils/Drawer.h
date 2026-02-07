/**************************************************************************
 *
 * Drawer class definition file. 
 *
 */

#ifndef _DRAWER_

#define _DRAWER_

#include "GPUType.h"
#include "Vec4FP32.h"
#include "GPUReg.h"
#include "glut.h"

//**  Default initial window x position.  
#define DEFAULT_WIN_X_POSITION 100

//**  Default initial window y position.  
#define DEFAULT_WIN_Y_POSITION 100

//**  Max number of batches in a frame.  
#define MAX_BATCHES 500

//** Max number of vertexs in a frame.  
#define MAX_VERTEXS 500000

//**  This structure defines a batch of vertexs.  
typedef struct Batch
{
    PrimitiveMode primitive;
    U32 start;
    U32 count;
};

/**
 *
 *  OpenGL wrapper for the fake rasterizer mdu.
 *
 *  Implements functions to rasterizer polygons using
 *  OpenGL.
 *
 */
 
class Drawer {

private:

	static Vec4FP32* vertex[];     //  A buffer for the current frame vertex positions.  
	static Vec4FP32* color[];      //  A buffer for the current frame vertex colors.  
	static Vec4FP32* oldVertex[];  //  A buffer for the previous frame vertex positions.  
    static Vec4FP32* oldColor[];   //  A buffer for the previous frame vertex colors.  
    
    static Batch batch[];           //  A buffer for the current frame vertex batches.  
    static Batch oldBatch[];        //  A buffer for the previous frame vertex batches.  
    
	static U32 vertexCounter;    //  Number of vertexs in the current frame.  
    static U32 batchCounter;     //  Number of batches in the current frame.  
    static U32 oldBatchCounter;  //  Number of batches for the previous frame.  
    static U32 oldVertexCounter; //  Number of vertexes fro the previous frame.  
    static U32 windowID;         //  Current window identifier.  
    
    /**
     *
     *  Drawer constructor.
     *
     *  Drawer is an static class so the constructor function is
     *  hidden.
     *
     */
    
	Drawer();

    /**
     *
     *  Drawer copy.
     *
     *  Drawer is an static class so the copy function is
     *  hidden.
     *
     */

	Drawer( const Drawer& );

    /**
     *
     *  Display function.
     *
     */
     
    static void display();
    
public:

    /**
     *
     *  Initializes the GLUT rasterizer.
     *
     */
     
    static void init();
    
    /**
     *
     *  Sets the GLUT idle function.
     *
     *  @param func The function to call from the idle loop.
     *
     */
     
    static void setIdleFunction(void (*func)(void));

    /**
     *
     *  Starts the Drawer main loop.
     *
     */
     
    static void start();

    /**
     *
     *  Creates a new GLUT window.
     *
     *  @param w Window width.
     *  @param h Window height.
     *
     */
         
    static void createWindow(U32 w, U32 h);
    
    /**
     *
     *  Destroys the current GLUT window.
     *
     */
     
    static void destroyWindow();
    
    /**
     *
     *  Sets the current clear color.
     *
     *  @param color The clear color for the color buffer.
     *
     */
     
    static void setClearColor(U32 color);
    
    /**
     *
     *  Sets the current clear z value.
     *
     *  @param zvalue Clear z value for the z buffer.
     *
     */
     
    static void setClearZ(U32 zvalue);
    
    /**
     *
     *  Sets the current clear stencil value.
     *
     *  @param val Clear stencil value for the stencil buffer.
     *
     */
     
    static void setClearStencil(S08 val);
    
    /**
     *
     *  Sets the viewport.
     *
     *  @param iniX Initial viewport x position.
     *  @param iniY Initial viewport y position.
     *  @param width Viewport width.
     *  @param height Viewport height.
     *
     */
     
    static void viewport(S32 iniX, S32 iniY, U32 width, U32 height);
    
    /**
     *
     *  Sets orthogonal projection matrix and mode.
     *
     *  @param left Left clip plane.
     *  @param right Right clip plane.
     *  @param bottom Bottom clip plane.
     *  @param top Top clip plane.
     *  @param near Near clip plane.
     *  @param far Far clip plane.
     *
     */
     
    static void setOrthogonal(F32 left, F32 right, F32 bottom, F32 top,
        F32 near, F32 far);

    /**
     *
     *  Sets the perspective projection frustum matrix and mode.
     *
     *  @param left Left frustum plane.
     *  @param right Right frustum plane.
     *  @param bottom Bottom frustum plane.
     *  @param top Top frustum plane.
     *  @param near Near frustum plane.
     *  @param far frustum plane.
     *
     */
     
    static void setPerspective(F32 left, F32 right, F32 bottom, F32 top,
        F32 near, F32 far);
      
    
    /**
     *
     *  Sets the face culling mode.
     *
     *  @param cullMode The face culling mode.
     *
     */
     
    static void setCulling(CullingMode cullMode);
    
    /**
     *
     *  Sets the shading mode.
     *
     *  @param shading Smooth shading mode enabled?
     *
     */
     
    static void setShading(bool shading);
    
    /**
     *
     *  Clear the color buffer.
     *
     */
     
    static void clearColorBuffer();
    
    /**
     *
     *  Swap front and back buffers.
     *
     */

    static void swapBuffers();

    /**
     *
     *  Set current primitive.
     *
     *  @param primitive The current primitive.
     *
     */
         
    static void setPrimitive(PrimitiveMode primitive);
   
    /**
     *
     *  Draw a vertex.
     *
     *  @param position The vertex position.
     *  @param color The vertex color.
     *
     */
      
    static void drawVertex(Vec4FP32 *position, Vec4FP32 *color);
   
};


#endif.
