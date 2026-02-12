/**************************************************************************
 *
 */

#include "Common.h"
#include "D3D9State.h"
#include "AIDeviceImp_9.h"
#include "AITextureImp_9.h"
#include "AICubeTextureImp_9.h"
#include "AISurfaceImp_9.h"
#include "../D3DControllers/Utils.h"
#include <stdio.h>

#include "GPUMath.h"

AISurfaceImp9::AISurfaceImp9(AIDeviceImp9* _i_parent, UINT _Width, UINT _Height,
                             DWORD _Usage , D3DFORMAT _Format, D3DPOOL _Pool) : 
                             i_parent(_i_parent), Width(_Width), Height(_Height),
                             Usage(_Usage) , Format(_Format), Pool(_Pool)
{
    //data = new unsigned char[getSurfaceSize(Width, Height, Format)];

    lockedData = NULL;

    ref_count = 0;

    //numModif = 0;
    
    surfaceType = D3D9_NONDEFINED;

    texture2d = NULL;
    cubeTexture = NULL;
    renderTarget = NULL;

}

AISurfaceImp9::AISurfaceImp9() 
{
    ///@note used to differentiate when creating singleton
    i_parent = 0;
}

AISurfaceImp9& AISurfaceImp9::getInstance() 
{
    static AISurfaceImp9 instance;
    return instance;
}

void AISurfaceImp9::setAsTexture2D(AITextureImp9* tex) 
{

    surfaceType = D3D9_TEXTURE2D;
    texture2d = tex;

}

void AISurfaceImp9::setAsCubeTexture(AICubeTextureImp9* tex) 
{

    surfaceType = D3D9_CUBETEXTURE;
    cubeTexture = tex;

}

void AISurfaceImp9::setAsRenderTarget(libGAL::GALRenderTarget* rt) 
{

    surfaceType = D3D9_RENDERTARGET;
    renderTarget = rt;

}

void AISurfaceImp9::setMipMapLevel(UINT m) 
{

    mipmap = m;

}

void AISurfaceImp9::setFace(D3DCUBEMAP_FACES f) 
{

    face = f;
    
}

TextureType AISurfaceImp9::getD3D9Type() 
{

    return surfaceType;

}

libGAL::GALTexture2D* AISurfaceImp9::getGALTexture2D() 
{

    if (surfaceType != D3D9_TEXTURE2D) CG_ASSERT("This surface isn't a member of a texture 2d.");

    return texture2d->getGALTexture2D();

}

libGAL::GALRenderTarget* AISurfaceImp9::getGALRenderTarget() 
{

    if (surfaceType == D3D9_RENDERTARGET)
        return renderTarget;

    if (renderTarget != NULL)
        return renderTarget; // The render target exist, so is returned.

    // If render target doesn't exist, create it.

    //  Create the new render target.
    /*libGAL::GAL_RT_DESC rtDesc;
    rtDesc.format = texture2d->getFormat(mipmap);
    rtDesc.dimension = libGAL::GAL_RT_DIMENSION_TEXTURE2D;
    rtDesc.texture2D.mipmap = mipmap;*/ // Is really needed????

    switch (surfaceType)
    {
        case D3D9_TEXTURE2D:
            renderTarget = D3D9State::instance().createRenderTarget(texture2d->getGALTexture2D(), libGAL::GAL_RT_DIMENSION_TEXTURE2D, (D3DCUBEMAP_FACES)0, mipmap);
            break;    

        case D3D9_CUBETEXTURE:
            renderTarget = D3D9State::instance().createRenderTarget(cubeTexture->getGALCubeTexture(), libGAL::GAL_RT_DIMENSION_TEXTURECM, face, mipmap);
            break;

        default:
            CG_ASSERT("Render targets form cube maps or volumes isn't supported.");
            break;
    }
    return renderTarget;

}

UINT AISurfaceImp9::getMipMapLevel() 
{

    return mipmap;

}


HRESULT D3D_CALL AISurfaceImp9::QueryInterface(REFIID riid, void** ppvObj) 
{
    D3D9_CALL(false, "AISurfaceImp9::QueryInterface")
    * ppvObj = cover_buffer_9;
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

ULONG D3D_CALL AISurfaceImp9::AddRef() 
{
    D3D9_CALL(true, "AISurfaceImp9::AddRef")
    if(i_parent != 0)
    {
        ref_count++;
        return ref_count;
    }
    else
    {
        // Object is used as singleton "cover"
        return 0;
    }
}

ULONG D3D_CALL AISurfaceImp9::Release() 
{
    D3D9_CALL(true, "AISurfaceImp9::Release")
    if(i_parent != 0) {
        ref_count--;
        /*if(ref_count == 0) {
            // Remove state
            StateDataNode* parent = state->get_parent();
            parent->remove_child(state);
            delete state;
            state = 0;
        }*/
        return ref_count;
    }
    else {
        // Object is used as singleton "cover"
        return 0;
    }
}

HRESULT D3D_CALL AISurfaceImp9::GetDevice(IDirect3DDevice9** ppDevice) 
{
    ///@note Depending on the constructor used, parent can be null.
    D3D9_CALL(false, "AISurfaceImp9::GetDevice")
    *ppDevice = (i_parent != 0)? i_parent : &AIDeviceImp9::getInstance();
    return D3D_OK;
}

HRESULT D3D_CALL AISurfaceImp9::SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) 
{
    D3D9_CALL(false, "AISurfaceImp9::SetPrivateData")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL AISurfaceImp9::GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) 
{
    D3D9_CALL(false, "AISurfaceImp9::GetPrivateData")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL AISurfaceImp9::FreePrivateData(REFGUID refguid) 
{
    D3D9_CALL(false, "AISurfaceImp9::FreePrivateData")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

DWORD D3D_CALL AISurfaceImp9::SetPriority (DWORD PriorityNew) 
{
    D3D9_CALL(false, "AISurfaceImp9::SetPriority")
    DWORD ret = static_cast< DWORD >(0);
    return ret;
}

DWORD D3D_CALL AISurfaceImp9::GetPriority() 
{
    D3D9_CALL(false, "AISurfaceImp9::GetPriority")
    DWORD ret = static_cast< DWORD >(0);
    return ret;
}

void D3D_CALL AISurfaceImp9::PreLoad () 
{
    D3D9_CALL(false, "AISurfaceImp9::PreLoad")
}

D3DRESOURCETYPE D3D_CALL AISurfaceImp9::GetType() 
{
    D3D9_CALL(false, "AISurfaceImp9::GetType")
    D3DRESOURCETYPE ret = static_cast< D3DRESOURCETYPE >(0);
    return ret;
}

HRESULT D3D_CALL AISurfaceImp9::GetContainer(REFIID riid, void** ppContainer) 
{
    D3D9_CALL(false, "AISurfaceImp9::GetContainer")
    * ppContainer = cover_buffer_9;
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL AISurfaceImp9::GetDesc(D3DSURFACE_DESC * pDesc) 
{
    // Surface is being used as a cover, so do nothing
    if(i_parent == 0) return D3D_OK;

    D3D9_CALL(true, "AISurfaceImp9::GetDesc")
    /*state->get_child(StateId("FORMAT"))->read_data(&pDesc->Format);
    state->get_child(StateId("TYPE"))->read_data(&pDesc->Type);
    state->get_child(StateId("USAGE"))->read_data(&pDesc->Usage);
    state->get_child(StateId("POOL"))->read_data(&pDesc->Pool);
    /// @note  Multisample info is not available
    state->get_child(StateId("WIDTH"))->read_data(&pDesc->Width);
    state->get_child(StateId("HEIGHT"))->read_data(&pDesc->Height);*/

    pDesc->Format = Format;
    pDesc->Width = Width;
    pDesc->Height = Height;
    pDesc->Usage = Usage;
    pDesc->Pool = Pool;

    return D3D_OK;
}

HRESULT D3D_CALL AISurfaceImp9::LockRect(D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) 
{
    D3D9_CALL(true, "AISurfaceImp9::LockRect")

    // Surface is being used as a cover, so do nothing
    if(i_parent == 0) return D3D_OK;

    LONG w, h;

    if (pRect == NULL) {
        leftLock = 0;
        topLock = 0;
        rightLock = Width;
        bottomLock= Height;
        w = Width;
        h = Height;
    }
    else {
        leftLock = pRect->left;
        topLock = pRect->top;
        rightLock = pRect->right;
        bottomLock= pRect->bottom;
        w = rightLock - leftLock;
        h = bottomLock - topLock;
    }

    if (lockedData != NULL) 
        delete[] lockedData;
    
    UINT actual_lock_size;

    actual_lock_size = getSurfaceSize(w, h, Format);

    lockedData = new unsigned char[actual_lock_size];

    UINT mipPitch;
    
    mipPitch = Width * texel_size(Format);

    lockedPitch = getSurfacePitch(w, Format);

    pLockedRect->pBits = (void*)lockedData;
    pLockedRect->Pitch = lockedPitch;

    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Width: " << Width << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Height: " << Height << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: lockedPitch: " << lockedPitch << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: actual_lock_size: " << actual_lock_size << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: mipPitch: " << mipPitch << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Left: " << leftLock << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Top: " << topLock << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Right: " << rightLock << endl; )
    D3D_DEBUG( cout << "ISURFACE9<" << this << ">: LockRect: Bottom: " << bottomLock << endl; )

    /*if (is_compressed(Format))
        memcpy(lockedData, data, actual_lock_size);
    else {
        for (UINT i = 0; i < (bottomLock - topLock); i++) {
                memcpy((lockedData + lockedPitch * i), (data + topLock * mipPitch + i * mipPitch + leftLock * texel_size(Format)), lockedPitch);
        }
    }*/

    return D3D_OK;

}

HRESULT D3D_CALL AISurfaceImp9::UnlockRect() 
{
    D3D9_CALL(true, "AISurfaceImp9::UnlockRect")

     // Surface is being used as a cover, so do nothing
    if(i_parent == 0) return D3D_OK;

    /*UINT actual_lock_size;
    
    actual_lock_size = getSurfaceSize(rightLock - leftLock, bottomLock - topLock, Format);

    UINT mipPitch;

    mipPitch = Width * texel_size(Format);*/

    RECT lockRect;

    lockRect.top = topLock;
    lockRect.bottom = bottomLock;
    lockRect.left = leftLock;
    lockRect.right = rightLock;

    //D3D9State::instance().updateSurface(this, &lockRect, Format, lockedData);


    switch (surfaceType)
    {

        case D3D9_TEXTURE2D:
            // Update the mipmap that corresponts to the passed surface with the passed data
            texture2d->getGALTexture2D()->updateData(mipmap, leftLock, topLock, (rightLock - leftLock), (bottomLock - topLock),
                                                     D3D9State::instance().getGALFormat(Format),
                                                     getSurfacePitchGAL((rightLock - leftLock), Format),
                                                     (libGAL::gal_ubyte*) lockedData, D3D9State::instance().isPreloadCall());
            break;

        case D3D9_CUBETEXTURE:
            cubeTexture->getGALCubeTexture()->updateData(D3D9State::instance().getGALCubeMapFace(face), mipmap, leftLock, topLock,
                                                         (rightLock - leftLock), (bottomLock - topLock),
                                                         D3D9State::instance().getGALFormat(Format),
                                                         getSurfacePitchGAL((rightLock - leftLock), Format),
                                                         (libGAL::gal_ubyte*) lockedData, D3D9State::instance().isPreloadCall());
            break;

        case D3D9_RENDERTARGET:
            CG_ASSERT("A render target can't be updated.");
            //renderTargetSurface[surf]->updateData(0, rect->left, rect->top, (rect->right - rect->left),
            //                                      (rect->bottom - rect->top), getGALFormat(format),
            //                                      getSurfacePitchGAL((rect->right - rect->left), format),
            //                                      (libGAL::gal_ubyte*) data, D3D9State::instance().isPreloadCall());
            break;

        default:
            CG_ASSERT("Unsupported texture type");
            break;
    }



/*if (Format == D3DFMT_ATI2)
{
    FILE *fout;
    char filename[50];
    sprintf(filename, "ATI2-surface-%016p.dat", this);

    fout = fopen((const char*) filename, "wb");

    int width = lockRect.right - lockRect.left;
    int height = lockRect.bottom - lockRect.top;
    
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
           fputc(char(lockedData[y * width + x]), fout);
        }
    }
    fclose(fout);
}*/

    /*if (is_compressed(Format))
        memcpy(data, lockedData, actual_lock_size);
    else {
        for (UINT i = 0; i < (bottomLock - topLock); i++) {
                memcpy((data + topLock * mipPitch + i * mipPitch + leftLock * texel_size(Format)), (lockedData + lockedPitch * i), lockedPitch);
        }
    }*/

    /*

    memcpy(data, lockedData, actual_lock_size);

    FILE *fout;
    char filename[50];
    sprintf(filename, "ad3d9-sampler-%d-%d.ppm", data, numModif);

    fout = fopen((const char*)filename, "wb");
    libGAL::gal_ubyte* index;
    index = data;

    for (int i = 0; i < actual_lock_size; i++) {
    }*/

    delete[] lockedData;
    lockedData = NULL;

    return D3D_OK;


    if (Format != D3DFMT_A8R8G8B8) {
        delete[] lockedData;
        lockedData = NULL;
        return D3D_OK;
    }

    /*if (Format != D3DFMT_G16R16F) {
        delete[] lockedData;
        lockedData = NULL;
        return D3D_OK;
    }*/

    FILE *fout;
    char filename[50];

    /*sprintf(filename, "ad3d9-sampler-%d.ppm", data);

    fout = fopen((const char*)filename, "wb");*/

    //  Write magic number.  
    //fprintf(fout, "P6\n");

    //  Write frame size.  
    //fprintf(fout, "%d %d\n", Width, Height);

    //  Write color component maximum value.  
    //fprintf(fout, "255\n");

    // Supose unsigned byte

    /*libGAL::gal_ubyte* index;
    index = data;
 
    for (  int i = 0 ; i < Height; i++ )
    {
        for ( int j = 0; j < Width; j++ )
        {
            fputc( char(*index), fout ); index++;
            fputc( char(*index), fout ); index++;
            fputc( char(*index), fout ); index++;
            index++;
        }
    }

    fclose(fout);*/


    //sprintf(filename, "ad3d9-sampler-unlock-%d-%d.ppm", lockedData, numModif);
    sprintf(filename, "ad3d9-sampler-unlock-%p.ppm", lockedData);

    fout = fopen((const char*)filename, "wb");

    //  Write magic number.  
    fprintf(fout, "P6\n");

    //  Write frame size.  
    fprintf(fout, "%d %d\n", (rightLock - leftLock), (bottomLock - topLock));

    //  Write color component maximum value.  
    fprintf(fout, "255\n");

    // Supose unsigned byte

    libGAL::gal_ubyte* index;
    index = lockedData;
 
    for (  int i = 0 ; i < (bottomLock - topLock); i++ )
    {
        for ( int j = 0; j < (rightLock - leftLock); j++ )
        {
            fputc( char(*index), fout ); index++;
            fputc( char(*index), fout ); index++;
            fputc( char(*index), fout ); index++;
            index++;
        }
    }

 
    /*for (  int i = 0 ; i < (bottomLock - topLock); i++ )
    {
        for ( int j = 0; j < (rightLock - leftLock); j++ )
        {
            fputc( U08(arch::GPUMath::convertFP16ToFP32(*((U16 *) &index[0])) * 255.0f), fout ); index++; index++;
            fputc( U08(arch::GPUMath::convertFP16ToFP32(*((U16 *) &index[0])) * 255.0f), fout ); index++; index++;
            fputc( char(0), fout );
            //index++;
        }
    }*/

    fclose(fout);

    delete[] lockedData;
    lockedData = NULL;



    //numModif++;

    return D3D_OK;

}

HRESULT D3D_CALL AISurfaceImp9::GetDC(HDC * phdc) 
{
    D3D9_CALL(false, "AISurfaceImp9::GetDC")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}

HRESULT D3D_CALL AISurfaceImp9::ReleaseDC(HDC hdc) 
{
    D3D9_CALL(false, "AISurfaceImp9::ReleaseDC")
    HRESULT ret = static_cast< HRESULT >(0);
    return ret;
}
