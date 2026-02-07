
#ifndef __D3D_PIXREADER_H__
#define __D3D_PIXREADER_H__
//PIXRun file format reader.

#include <iostream>
#include <fstream>
#include <stdio.h>

//#ifdef WIN32
//    #ifndef ZLIB_WINAPI
//        #define ZLIB_WINAPI
//    #endif
//#endif

#include "zlib.h"

#include "GPUType.h"

// A D3DPixRunReader implements the reading operations for a Pixrun(pix) file
class D3D9PixRunReader
{
public:
    typedef U32 CID; // CID = Command IDentifier

    D3D9PixRunReader();

    void open(std::string filename);    // Open a PixRun file
    void close(); // Closes opened PixRun file or does nothing
    bool fetch(); // Reads next call, return false if there aren't more calls.
    bool isPreloadCall();
    CID  getFetchedCID(); // Returns fetched call id.  Pre: Last fetch returned true
    U32  getFetchedEID(); // Returns event id Pre: Last fetch returned true

    void readParameter(D3DBuffer *param);//     Specialization for reading the locked memory region that follows unlock operations.
    template < typename T > void readParameter(T *param);//    Templated method that reads a 'fixed size' parameter, this includes primitive types, enums, structs ...

private:
    // PIXRun specific structures and id's , see "PixRunFileFormat.pdf" in doc
    struct Chunk
    {
        U32 size;
        U32 type;
    };

    struct EventData
    {
        U32 EDID;
        U32 EID;
    };
    
    struct AsyncAttribute
    {
        U32 EID;
        U32 ADID;
    };

    struct PackedCallPackage
    {
        U32 size;
        U32 CID;
        U32 one;
    };

    // Attributes
    PackedCallPackage fetchedPCP;
    U32 fetchedEID;
    bool isD3DCALL_SYNC;
    //FILE *input;
    gzFile input;
    // std::ifstream *input;

    //std::ofstream log;
    
    //  Some statistics.
    //U64 readData;
    //U64 usedData;
    //U64 skippedData;

    bool readCID();           // Try to read next CID, return true if success
    void skipParameters();    // Skips the rest of parameters
    template < typename T > void readPixRunStructure(T *s);    // Read template method for structures of the PixRun files, it doesn't increment parameter offset.
    void skip(U32 bytes);     // Skip bytes

    bool mustSkipParameters; // Controls if we are reading parameters and have to skip before reading next CID
    DWORD parameterOffset;   // Control of the current offset
};

template < typename T > void D3D9PixRunReader::readParameter(T *param)
{
    //fread((void *)param, sizeof(T), 1, input);
    gzread(input, (void *)param, sizeof(T));
    // Advance parameter offset
    parameterOffset += sizeof(T);
    //readData +=sizeof(T);
    //usedData +=sizeof(T);
    //log << "readParameter -> read " << sizeof(T) << " bytes | current file position is " << gztell(input);
    //log << " parameterOffset = " << parameterOffset;
    //if (sizeof(T) >= 4)
    //    log << " | value = " << (*(int *) param);
    //log << std::endl;
}

template < typename T > void D3D9PixRunReader::readPixRunStructure(T *param)
{
    //fread((void *)param, sizeof(T), 1, input);
    gzread(input, (void *)param, sizeof(T));
    //readData +=sizeof(T);
    //log << "readPixStructure -> read " << sizeof(T) << " bytes.  Current file position is " << gztell(input) << std::endl;
}

#endif
