/**************************************************************************
 *
 */


#ifndef DLLLOADER_H
    #define DLLLOADER_H

#include <string>

typedef void *FPointer;

/**
 * Class used to dynamically load functions contained within a dynamic library
 *
 * @code 
 *
 * // Example of use 1
 *
 * DLLLoader dll("myDll.dll");
 * if ( !dll )
 * {
 *     // open was not successful
 * }
 * 
 *
 * @endcode
 */
class DLLLoader
{
public:

    DLLLoader();
    DLLLoader(std::string path);
    ~DLLLoader();

    bool open(std::string path);

    /**
     * Checks if the library could be found (opened)
     */
    bool operator!() const;
    
    FPointer getFunction(std::string functionName);
    
private:

    struct _IRep;
    _IRep* _iRep;

};


#endif // DLLLOADER

