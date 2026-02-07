/**************************************************************************
 *
 * Support definitions file.
 *
 */
#ifndef _GPU_SUPPORT_

#define _GPU_SUPPORT_

#include <string>
#include <iostream>
#include "GPUType.h"

// Using debug code, comment this line to avoid debug code
#define GPU_CHECK_ERRORS
//#define GPU_MESSAGES // messages on

#ifdef GPU_CHECK_ERRORS
   #define GPU_ERROR_CHECK(expr) { expr }
#else
   #define GPU_ERROR_CHECK(expr) { }
#endif

#ifdef GPU_MESSAGES
   #define GPU_MESSAGE(expr) { expr }
#else
   #define GPU_MESSAGE(expr) { }
#endif

//*  Macro for assertions.  

#define GPU_ASSERTS

#ifdef GPU_ASSERTS
#define GPU_ASSERT(expr) { expr }
#define GPU_ASSERT_COND(cond,expr) {if(cond) expr}
#else
#define GPU_ASSERT(expr) { }
#define GPU_ASSERT_COND(cond,expr) { }
#endif

//*  Macro for debug code and messages.  
//#define GPU_DEBUG_ON

#ifdef GPU_DEBUG_ON
    #define GPU_DEBUG(expr) { expr }
#else
    #define GPU_DEBUG(expr) { }
#endif


//#define GPU_TEX_TRACE_ENABLE 1

//  Macro for texture requests tracing
#ifdef GPU_TEX_TRACE_ENABLE
    #define GPU_TEX_TRACE(expr) { expr }
#else
    #define GPU_TEX_TRACE(expr) { }
#endif


/*
 * Macro functions ( decorated with GPU prefix )
 */

#define GPU_MOD(value,modulus) ((value)%(modulus))

#define COUNT_ARRAY(array) (int)(sizeof(array)/sizeof(array[0]))

#define EQ(a,b) (strcmp(a,b) == 0 ? true : false)


// Comment out this line to skip popup() calls
//#define SHOW_POPUPS

/*
 *   Panic Function.
 */
void panic(const char *className, const char* fnName, const char *message);

extern void (*panicCallback)();


//void panic_func(const char* className, const char* fName, const char* message,
//           const char* file, int line);

/**
 * macro for finding LINE & FILE when panic_func is called
 */
//#define panic(c,f,m) panic_func(c,f,m,__FILE__,__LINE__)

/*
 * Basic message function
 */
void popup(const char* title, const char* msg);

/**
 *
 * Create a new directory.
 *
 * @param dirName Name of the new directory to create.
 *
 * @return Returns 0 if the directory was created, or an error value if the directory couldn't be created.
 *
 */
 
int createDirectory(char *dirName);

/**
 *
 * Change working directory.
 *
 * @param dirName Name of the new working directory.
 *
 * @return Returns 0 if the working directory was changed.
 *
 */
 
int changeDirectory(char *dirName);

/**
 *
 *  Get the current directory.
 *
 *  @param dirName Pointer to a character array where to store the path for the current directory.
 *  @param arraySize Size of the character array in bytes.
 *
 */
 
int getCurrentDirectory(char *dirName, int arraySize);
 
#define DIRECTORY_ALREADY_EXISTS 1

void ModelPrint(const char* dbgLvl, const char* mdlTyp, const char* file, const char* func, const int line, const char* msg, ...);

#include "SupportMacro.h"

class DebugTracker
{
public:

    DebugTracker(const char* name);
    DebugTracker(const DebugTracker& dt);
    ~DebugTracker();
    void __showInfo() const; 

private:
    static unsigned int refs;
    std::string name;
};


#endif
