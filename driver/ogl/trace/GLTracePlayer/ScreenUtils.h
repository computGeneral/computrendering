/**************************************************************************
 *
 */

#ifndef SCREENUTILS_H
    #define SCREENUTILS_H

#include <windows.h>

// From msdn

PBITMAPINFO createBitmapInfoStruct(HBITMAP hBmp);

void createBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, 
                  HBITMAP hBMP, HDC hDC);


#endif // SCREENUTILS_H