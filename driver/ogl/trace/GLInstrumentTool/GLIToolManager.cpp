/**************************************************************************
 *
 */

#include "GLIToolManager.h"
/// INCLUDE YOUR TOOL DEFINITION HERE
#include "Tools.h"
#include "BatchesAsFrames.h"

using namespace std;

GLIToolManager::GLIToolManager()
{}

GLIToolManager& GLIToolManager::instance()
{
    static GLIToolManager toolManager;
    return toolManager;
}


GLITool* GLIToolManager::getTool()
{
    // examples:
    // PUT YOUR CODE HERE
    // create and return your tool here 
    //(replace the statement "return 0;" with your code)    
    //return 0;
    //return new EveryNInfoTool(50);
    return new BatchesAsFrames;
}

