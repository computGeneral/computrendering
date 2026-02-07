
// Code for write a bitmap
#ifndef __WRITE_BMP
#define __WRITE_BMP

PBITMAPINFO createBitmapInfoStruct(HBITMAP hBmp);
void createBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);

#endif