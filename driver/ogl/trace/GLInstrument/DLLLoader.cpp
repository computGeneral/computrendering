/**************************************************************************
 *
 */

#include "DLLLoader.h"

#if defined WIN32 || defined WIn64

#include <windows.h>

struct DLLLoader::_IRep
{
    HMODULE handle;    
};

bool DLLLoader::open(std::string path)
{
    _iRep->handle = LoadLibrary(path.c_str());
    if ( _iRep->handle )
        return true;
    return false;
}

bool DLLLoader::operator!() const
{
    if ( _iRep->handle )
        return false;
    return true;
}

FPointer DLLLoader::getFunction(std::string functionName)
{
    return (FPointer)GetProcAddress(_iRep->handle, functionName.c_str());
}

#else // Assume Linux/Unix

struct DLLLoader::_IRep
{
    void* handle;    
};

bool DLLLoader::open(std::string path)
{
    _iRep->handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if ( _iRep->handle )
        return true;
    return false;

}

FPointer DLLLoader::getFunction(std::string functionName)
{
    return dlsym(_iRep->handle, functionName.c_str());
}

#endif


/**
 * Common code
 */

DLLLoader::DLLLoader()
{
    _iRep = new _IRep;
#if defined WIN32 || defined WIn64
    _iRep->handle = 0;
#endif
}

DLLLoader::DLLLoader(std::string path)
{
    _iRep = new _IRep;
#if defined WIN32 || defined WIn64
    _iRep->handle = 0;
#endif
    open(path);
}

DLLLoader::~DLLLoader()
{
    delete _iRep;
}
