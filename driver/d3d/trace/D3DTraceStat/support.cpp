/**************************************************************************
 *
 * Support functions file. 
 *
 */


#include <iostream>
#include "support.h"

using namespace std;

void panic(const char* className, const char* fName, const char* message)
{
    panic_func(className,fName,message,__FILE__,__LINE__);
}

void panic_func(const char *className, const char *fnName, const char *message, const char* file, int line)
{
    cout << "File: " << file << "  Line: " << line << "\n ";
	cout << className << ":" << fnName << " => " << message << endl;
	exit(-1);
}

void popup( const char* title, const char* msg )
{
    //#define SHOW_POPUPS
    #ifdef SHOW_POPUPS    
    cout << title << ": " << msg << endl;
    #endif
}
