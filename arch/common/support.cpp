
/**************************************************************************
 * Support functions file. 
 */


#include <iostream>
#include <cstdio>
#include <cstdarg>
#include "support.h"
//#include "Log.h"
#ifdef WIN32
    #include <windows.h>
    #include <sstream>
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif
#include <cstdlib>
#include <cerrno>

using namespace std;

void (*panicCallback)() = 0;

void panic(const char* className, const char* fnName, const char* message)
{
    #ifdef WIN32_MESSAGES
        std::stringstream ss;
        ss << className << ":" << fnName << " => " << message;
        MessageBox(0, ss.str().c_str(), "panic", MB_OK);
    #else        
        cout << endl;
        cout << className << ":" << fnName << " => " << message << endl;
    #endif
    
    if (panicCallback != NULL)
    {    
        void (*definedPanicCallback)() = panicCallback;
        
        //  Disable panic call back inside a panic.
        panicCallback = NULL;
        
        (*definedPanicCallback)();
    }
    
    exit(-1);
}

/*void panic_func(const char *className, const char *fnName, const char *message, const char* file, int line)
//{
    //CG1_ISA_OPCODE_LOG(0, Log::log() << "PANIC: " << "class: " << className << " method: " << fnName << " Msg: " << message; )
    cout << "File: " << file << "  Line: " << line << "\n ";
    cout << className << ":" << fnName << " => " << message << endl;
    exit(-1);
}*/

void popup( const char* title, const char* msg )
{
    //#define SHOW_POPUPS
    #ifdef WIN32_MESSAGES
        MessageBox(0, msg, title, MB_OK);
    #else
        #ifdef SHOW_POPUPS    
            cout << title << ": " << msg << endl;
        #endif
    #endif    
}

int createDirectory(char *dirName)
{
#ifdef WIN32
    if (_mkdir(dirName) == 0)
        return 0;
    else
    {
        if (errno == EEXIST)
            return DIRECTORY_ALREADY_EXISTS;
        else
            printf("createDirectory => errno = %d\n", errno);
            return -1;
    }
#else
    if (mkdir(dirName, S_IRWXU) == 0)
        return 0;
    else
        if (errno == EEXIST)
            return DIRECTORY_ALREADY_EXISTS;
        else
            return -1;
#endif
}

int changeDirectory(char *dirName)
{
#ifdef WIN32
    return _chdir(dirName);
#else
    return chdir(dirName);
#endif
}

int getCurrentDirectory(char *dirName, int arraySize)
{
#ifdef WIN32
    if (_getcwd(dirName, arraySize) != NULL)
        return 0;
    else
        return -1;
#else
    if(getcwd(dirName, arraySize) != NULL)
        return 0;
    else
        return -1;
#endif
}

void ModelPrint(const char* dbgLvl, const char* mdlTyp, const char* file, const char* func, const int line, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);

    char bufMsg[256]   = {0};
    char bufTotal[1024] = {0};

    vsnprintf(bufMsg, sizeof(bufMsg), msg, args);
    va_end(args);

    int n = snprintf(bufTotal, sizeof(bufTotal),
                     "[%s|%s] [MSG]:%-48s [FILE]:%s [FUNC]:%s [LINE]:%05d\n",
                     dbgLvl, mdlTyp, bufMsg, file, func, line);
    (void)n;

    fputs(bufTotal, stdout);
#ifdef _WIN32
    OutputDebugStringA(bufTotal);
#endif
}



unsigned int DebugTracker::refs = 0;


DebugTracker::DebugTracker(const char* name) : name(name) 
{
    cout << "Create DEBUGTRACKER address: " << reinterpret_cast<unsigned int*>(this) << endl;
    ++refs;
}

DebugTracker::~DebugTracker() 
{ 
    --refs; 
}

void DebugTracker::__showInfo() const
{
    std::cout << "Name: " << name << ". References => " << refs << std::endl;
}


DebugTracker::DebugTracker(const DebugTracker& dt) 
{ 
    ++refs; 
}

