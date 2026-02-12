/**************************************************************************
 *
 * ImageSaver definition file.
 *
 */


/**
 *
 *  @file ImageSaver.h
 *
 *  This file contains definitions and includes for the ImageSaver class.
 *
 */

#ifndef _IMAGESAVER_
#define _IMAGESAVER_

#include "GPUType.h"

namespace arch
{

/**
 *
 *  Saves image data into picture files.
 *
 */

class ImageSaver
{
private:

#ifdef WIN32
    pointer gdiplusToken;    // ULONG_PTR is a 64-bit pointer
    U08* encoderClsid;
#endif

    /**
     *
     *  Constructor.
     *
     */

    ImageSaver();
    
    /**
     *
     *  Destructor.
     *
     */

    ~ImageSaver();
    
public:

    /**
     *
     *  Get the single object instantation of the ImageSaver class.
     *
     *  @return Reference to the single instantation of the ImageSaver class.
     *
     */
     
    static ImageSaver &getInstance();
    
    /**
     *
     *  Save image data to PNG file.
     *
     *  @param filename Name/path of the destination PNG file.
     *  @param xRes Horizontal resolution in pixels of the image.
     *  @param yRes Vertical resolution in pixels of the image.
     *  @param data Pointer to the image data array.
     *
     */
     
    void savePNG(char *filename, U32 xRes, U32 yRes, U08 *data);
    
};

} // namespace arch

#endif  // _IMAGESAVER_

