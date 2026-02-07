#include "Drawer.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

//  Vertex position and color arrays.  
Vec4FP32* Drawer::vertex[MAX_VERTEXS];
Vec4FP32* Drawer::color[MAX_VERTEXS];

//  Vertex position and color arrays for the previous frame (double buffered).  
Vec4FP32 *Drawer::oldVertex[MAX_VERTEXS];
Vec4FP32 *Drawer::oldColor[MAX_VERTEXS];

//  Current frame batch array.  
Batch Drawer::batch[MAX_BATCHES];

//  Previous frame batch array.  
Batch Drawer::oldBatch[MAX_BATCHES];

//  Vertex counter.  
U32 Drawer::vertexCounter;

//  Vertex counter for the previous frame (double buffered).  
U32 Drawer::oldVertexCounter;

//  Batch counter.  
U32 Drawer::batchCounter;

//  Batch counter for the previous frame (double buffered).  
U32 Drawer::oldBatchCounter;

//  Window ID.  
U32 Drawer::windowID;

//  Initialize the renderer.  
void Drawer::init()
{
    //  Initialize counters.  
    vertexCounter = 0;
    oldBatchCounter = (U32) -1;
    batchCounter = (U32) -1;    

}

//  Set GLUT idle function.  
void Drawer::setIdleFunction(void (*func)(void))
{
    //  Set GLUT idle function.  
    glutIdleFunc(func);    
}

//  Start the renderer.  
void Drawer::start()
{
    //  Initialize GLUT: double buffer and RGBA frame buffer.  
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    //  Set display function.  
    glutDisplayFunc(display);

    //  Start GLUT main loop.  
    glutMainLoop();
}

//  Create a GLUT window.  
void Drawer::createWindow(U32 w, U32 h)
{
    //  Set initial window position.  
    glutInitWindowPosition(DEFAULT_WIN_X_POSITION, DEFAULT_WIN_Y_POSITION);
    
    //  Set window initial size.  
    glutInitWindowSize(w, h);

    //  Create window.  
    windowID = glutCreateWindow("CG1GPU simulator 0.1");

    //  Set display function.  
    glutDisplayFunc(display);

}

//  Destroy current window.  
void Drawer::destroyWindow()
{
    //  Destroy current GLUT window.  
    glutDestroyWindow(windowID);
}

//  Sets GL clear color.  
void Drawer::setClearColor(U32 color)
{
    float r, g, b, a;
    
    //  Convert RGBA values.  
    r = (double) ((color >> 24) & 0xff) / 255.0;
    g = (double) ((color >> 16) & 0xff) / 255.0;
    b = (double) ((color >> 8) & 0xff)/ 255.0;
    a = (double) (color & 0xff) / 255.0;
    
    //  Set current GL clear color.  
    glClearColor(r, g, b, a);
}

//  Sets the GL Z clear value.  
void Drawer::setClearZ(U32 zval)
{
    //  TO IMPLEMENT.  
}

//  Sets the GL stencil clear value.  
void Drawer::setClearStencil(S08 stencil)
{
    //  TO IMPLEMENT.  
}

//  Sets GL Viewport.  
void Drawer::viewport(S32 iniX, S32 iniY, U32 width, U32 height)
{
    //  Set GL viewport.  
    glViewport(iniX, iniY, width, height);
}

//  Sets orthogonal projection matrix.  
void Drawer::setOrthogonal(F32 left, F32 right, F32 bottom, F32 top,
        F32 near, F32 far)
{
    //  Select GL projection matrix.  
    glMatrixMode(GL_PROJECTION);
    
    //  Load identity matrix.  
    glLoadIdentity();
    
    //  Create orthogonal projection matrix.  
    //glOrtho(left, right, bottom, top, near, far);
}

//  Sets perspective projection matrix.  
void Drawer::setPerspective(F32 left, F32 right, F32 bottom, F32 top,
        F32 near, F32 far)
{
    //  Select GL projection matrix.  
    glMatrixMode(GL_PROJECTION);
    
    //  Load identity matrix.  
    glLoadIdentity();
    
    //  Create perspective projection matrix.  
    //glFrustum(left, right, bottom, top, near, far);
}

//  Sets GL face culling mode.  
void Drawer::setCulling(CullingMode cullMode)
{
    //  Enable face culling.  
    glEnable(GL_CULL_FACE);
    
    //  Select culling mode.  
    switch(cullMode)
    {
        case FRONT:
            //  Set GL cull face mode to front faces.  
            glCullFace(GL_FRONT);
            break;
        case BACK:
            //  Set GL cull face mode to back faces.  
            glCullFace(GL_BACK);
            break;
        case FRONT_AND_BACK:
            //  Set GL cull face mode to both faces.  
            glCullFace(GL_FRONT_AND_BACK);
            break;
    }
}

//  Sets GL shading mode.  
void Drawer::setShading(bool shading)
{
    //  Smooth shading must be enabled?  
    if (shading)
        glShadeModel(GL_SMOOTH);    //  Set smooth shading.  
    else
        glShadeModel(GL_FLAT);      //  Set flat shading.  
}

//  Clear color buffer.  
void Drawer::clearColorBuffer()
{
    //  Clear the GL color buffer.  
    glClear(GL_COLOR_BUFFER_BIT);
}

//  Swap buffers.  
void Drawer::swapBuffers()
{
    int i;
    
    //  Free previous frame data.  
//    for (i = 0; i < oldVertexCounter; i++)
//   {
//        //  Delete vertex position and color.  
//        delete oldVertex[i];
//        delete oldColor[i];
//    }

//    printf("!!SWAP BUFFERS!!\n");
      
    //  Copy current frame data into previous frame storage.  
    oldBatchCounter = batchCounter;
    oldVertexCounter = vertexCounter;
    memcpy(&oldBatch, &batch, sizeof(Batch)*(batchCounter + 1));
    memcpy(&oldVertex, &vertex, sizeof(Vec4FP32 *)*vertexCounter);
    memcpy(&oldColor, &color, sizeof(Vec4FP32 *)*vertexCounter);  

    //  Reset next frame batch counter.  
    batchCounter = (U32) -1;
    //  Reset next frame vertex counter.  
    vertexCounter = 0;

    //  Swap front and back buffer.  
    glutSwapBuffers();
    glutPostRedisplay();

//    printf("!!SWAP BUFFERS END!!\n");

}

//  Sets current primitive.  
void Drawer::setPrimitive(PrimitiveMode primitive)
{

    //  Update batch counter.  
    batchCounter++;

    //  Check overflow.  
    GPU_ASSERT(
        if (batchCounter == MAX_BATCHES)
        {
            CG_ASSERT("Batch buffer overflow.");
        }
    )

    //  Add a new vertex batch for this primitive.  
    batch[batchCounter].primitive = primitive;
    batch[batchCounter].start = vertexCounter;
    batch[batchCounter].count = 0;
}

//  Draw a vertex.  
void Drawer::drawVertex(Vec4FP32 *position, Vec4FP32 *vcolor)
{
    //  Check overflow.  
    CG_ASSERT_COND(!(vertexCounter == MAX_VERTEXS), "Vertex buffer overflow.");
    //  Store vertex position in the vertex position array.  
    vertex[vertexCounter] = position;
    
    //  Clamp vertex color to 0.0 - 1.0.  
    (*vcolor)[0] = (*vcolor)[0] > 1.0?1.0:(*vcolor)[0];
    (*vcolor)[0] = (*vcolor)[0] < 0.0?0.0:(*vcolor)[0];
    (*vcolor)[1] = (*vcolor)[1] > 1.0?1.0:(*vcolor)[1];
    (*vcolor)[1] = (*vcolor)[1] < 0.0?0.0:(*vcolor)[1];
    (*vcolor)[2] = (*vcolor)[2] > 1.0?1.0:(*vcolor)[2];
    (*vcolor)[2] = (*vcolor)[2] < 0.0?0.0:(*vcolor)[2];
    
    //  Store vertex color in the vertex color array.  
    color[vertexCounter] = vcolor;

    //  Update current batch vertex counter.  
    batch[batchCounter].count++;
    
    //  Update vertex counter.  
    vertexCounter++;
}

//  Display function.  
void Drawer::display(void)
{
    int i, j;

    //  Just clear the window if there are no batches available.  
    if (oldBatchCounter == -1)
    {
        glClearColor(0.000000,0.000000,0.000000,0.000000);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();
    }
    else
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

//    printf("DISPLAYING %d BATCHES %d VERTEXES\n", oldBatchCounter, 
//        oldVertexCounter);
        
    //  Process all batches in the frame.  
    for(i = 0; i < (oldBatchCounter + 1); i++)
    {
        switch(oldBatch[i].primitive)
        {
            case TRIANGLE:
                //  Draw triangles.  

                glBegin(GL_TRIANGLES);
                
                break;
                
            case TRIANGLE_STRIP:
                
                //  Draw triangle strips.  
                
                glBegin(GL_TRIANGLE_STRIP);
                 
                break;
                
            case TRIANGLE_FAN:
                
                //  Draw a triangle fan.  
                glBegin(GL_TRIANGLE_FAN);

                break;
                
            case QUAD:
                CG_ASSERT("Primitive not supported.");
                break;
                
            case QUAD_STRIP:
                
                //  Draw a quad strip.  
                glBegin(GL_QUAD_STRIP);
                
                break;
                
            case LINE:
                CG_ASSERT("Primitive not supported.");
                break;
                
            case LINE_STRIP:
                CG_ASSERT("Primitive not supported.");
                break;
                
            case LINE_FAN:
                CG_ASSERT("Primitive not supported.");
                break;
                
            case POINT:
                CG_ASSERT("Primitive not supported.");
                break;
                
            default:
                CG_ASSERT("Primitive not supported.");
                break;
        }

//        printf("BATCH %d FROM VERTEX %d TO VERTEX %d\n", 
//            i, oldBatch[i].start, oldBatch[i].start + oldBatch[i].count - 1);
            
        //  Send triangles data.  
        for(j = 0; j < oldBatch[i].count; j++)
        {
            //  Set vertex color.  
            glColor3f((*oldColor[oldBatch[i].start + j])[0],
                (*oldColor[oldBatch[i].start + j])[1],
                (*oldColor[oldBatch[i].start + j])[2]);
                        
            //  Send vertex position.  
            glVertex3f((*oldVertex[oldBatch[i].start + j])[0],
                (*oldVertex[oldBatch[i].start + j])[1],
                (*oldVertex[oldBatch[i].start + j])[2]);
        
            //printf("V = {%f, %f, %f}, C = {%f, %f, %f, %f}\n", 
            //    (*oldVertex[oldBatch[i].start + j])[0],
            //    (*oldVertex[oldBatch[i].start + j])[1],
            //    (*oldVertex[oldBatch[i].start + j])[2],
            //    (*oldColor[oldBatch[i].start + j])[0],
            //    (*oldColor[oldBatch[i].start + j])[1],
            //    (*oldColor[oldBatch[i].start + j])[2]);
        }

        //  End batch
        glEnd();

//        printf("END BATCH %d\n", i);
    }

//    printf("END DISPLAY\n");    
}

