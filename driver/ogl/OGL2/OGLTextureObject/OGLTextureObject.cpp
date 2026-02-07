/**************************************************************************
 *
 */

#include "OGLTextureObject.h"
#include "OGLTextureTarget.h"
#include "GPUMath.h"
#include "OGLContext.h"
#include "glext.h"
#include "GL2GAL.h"
#include "GL2GALx.h"
#include <iostream>
#include <stdio.h>

using namespace ogl;
using namespace libGAL;

GLTextureObject::GLTextureObject(GLuint name, GLenum targetName) : 
                                    BaseObject(name, targetName),
                                    _galDev(0), _texture2D(0), _textureCM(0), _textureRECT(0),
                                    _wrapS(GL_REPEAT), _wrapT(GL_REPEAT), _wrapR(GL_REPEAT),
                                    _magFilter(GL_LINEAR), _minFilter(GL_NEAREST_MIPMAP_LINEAR), 
                                    _maxAnisotropy(1.0f), _minLOD(-1000), _maxLOD(1000), _biasLOD(0.0f),
                                    _baseLevel(0), _maxLevel(mipmapMaxSize-1)
                                
{
}


void GLTextureObject::attachDevice(libGAL::GALDevice* device, GLuint target)
{
    if (device == 0)
        CG_ASSERT("Selected device not correct");

    if (_galDev != 0)
        return;

    _galDev = device;

    switch (target)
    {
        case GL_TEXTURE_2D:
            _texture2D = _galDev->createTexture2D();
            break;

        case GL_TEXTURE_3D:
            _texture3D = _galDev->createTexture3D();
            break;

        case GL_TEXTURE_RECTANGLE:
            _textureRECT = _galDev->createTexture2D();
            break;

        case GL_TEXTURE_CUBE_MAP:
            _textureCM = _galDev->createTextureCubeMap();
            break;

        case GL_TEXTURE_1D:
        default:
            char error[128];
            sprintf(error, "Selected target not implemented yet: %d", target);
            CG_ASSERT(error);
            break;
    }

}

GALTexture* GLTextureObject::getTexture ()
{
    if (_texture2D)
        return _texture2D;
    else if (_textureCM)
        return _textureCM;
    else if (_textureRECT)
        return _textureRECT;
    else if (_texture3D)
        return _texture3D;
    else
        CG_ASSERT("Texture object doesn't have a texture");
        return 0;
}

void GLTextureObject::setContents( GLenum targetFace, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                    GLsizei depth, GLint border, GLenum inputFormat, GLenum type,
                                    const GLubyte* data, GLsizei compressedSize)
{
    GLubyte* convertedData = 0;
    GLenum adaptedFormat;

    switch(targetFace)
    {
        case GL_TEXTURE_2D:
            adaptedFormat = set2D(width, height, border, inputFormat, internalFormat, type, data, convertedData, compressedSize);
            _texture2D->setData(level, width, height,ogl::getGALTextureFormat(adaptedFormat), 0, convertedData, compressedSize);
            break;

        case GL_TEXTURE_3D:
        {
            gal_uint offset = 0;
            for (gal_uint i = 0; i < depth; i++)
            {
                adaptedFormat = set2D(width, height, border, inputFormat, internalFormat, type, data + offset, convertedData, compressedSize);
                offset = ogl::getGALTexelSize(adaptedFormat) * width * height;
            }
            _texture3D->setData(level, width, height, depth, ogl::getGALTextureFormat(adaptedFormat), 0, convertedData, compressedSize);
        }
            break;

        case GL_TEXTURE_RECTANGLE:
            adaptedFormat = set2D(width, height, border, inputFormat, internalFormat, type, data, convertedData, compressedSize);
            _textureRECT->setData(level, width, height,ogl::getGALTextureFormat(adaptedFormat), 0, convertedData, compressedSize);
            break;

        case GL_TEXTURE_CUBE_MAP:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            adaptedFormat = set2D(width, height, border, inputFormat, internalFormat, type, data, convertedData, compressedSize);
            _textureCM->setData(ogl::getGALCubeMapFace(targetFace), level, width, height, ogl::getGALTextureFormat(adaptedFormat), 0, convertedData, compressedSize);
            break;

        default:
            char error[128];
            sprintf(error, "Selected texture type not implemented: %d", targetFace);
            CG_ASSERT(error);
            break;

    }
}

void GLTextureObject::setPartialContents(GLenum targetFace, GLint level, GLint xoffset, GLint yoffset,
                                         GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                         GLenum format, GLenum type, const GLubyte* subData, GLint compressedSize)
{
    GLubyte* dataTrans = 0;

    switch(targetFace)
    {
        case GL_TEXTURE_2D:
            setPartial2D(width, height, xoffset, yoffset, format, getGLTextureFormat(_texture2D->getFormat(level)), type, subData, dataTrans, compressedSize);
            _texture2D->updateData(level, xoffset, yoffset, width, height, _texture2D->getFormat(level), width * ogl::getGALTexelSize(format), subData);
            break;

        case GL_TEXTURE_3D:
            {
                gal_uint slideOffset = getGLTextureFormat(_texture2D->getFormat(level)) * width * height;
                gal_uint offset = slideOffset * zoffset;

                for (gal_uint i = zoffset; i < depth; i++)
                {
                    setPartial2D(width, height, xoffset, yoffset, format, getGLTextureFormat(_texture2D->getFormat(level)), type, subData + offset*i, dataTrans, compressedSize);
                }

                _texture3D->updateData(level, xoffset, yoffset, zoffset, width, height, depth, _texture2D->getFormat(level), width * ogl::getGALTexelSize(format), subData);
                
            }
            break;

        case GL_TEXTURE_RECTANGLE:
            setPartial2D(width, height, xoffset, yoffset, format, getGLTextureFormat(_textureRECT->getFormat(level)), type, subData, dataTrans, compressedSize);
            _textureRECT->updateData(level, xoffset, yoffset, width, height, _textureRECT->getFormat(level), width * ogl::getGALTexelSize(format), subData);
            break;

        case GL_TEXTURE_CUBE_MAP:
            {
                GAL_CUBEMAP_FACE face = ogl::getGALCubeMapFace(targetFace);
                setPartial2D(width, height, xoffset, yoffset, format, getGLTextureFormat(_textureCM->getFormat(ogl::getGALCubeMapFace(targetFace),level)), type, subData, dataTrans, compressedSize);
                _textureCM->updateData(face, level, xoffset, yoffset, width, height, _textureCM->getFormat(face,level), width * ogl::getGALTexelSize(format), subData);
            }
            break;

        default:
            char error[128];
            sprintf(error, "Texture type not implemented: %d", targetFace);
            CG_ASSERT(error);
            break;

    }
}

GLenum GLTextureObject::set2D(GLint &width, GLint &height, GLint border, GLenum inputFormat, GLint internalFormat, 
                                  GLenum type, const GLvoid* data, GLubyte* &convertedData, GLint &compressedSize)
{
    if ( border != 0 )
        CG_ASSERT("border different of 0 is not supported for now");

    if ( internalFormat == 4 ) internalFormat = GL_RGBA; // Patch for backward compatibility
    if ( internalFormat == 3 ) internalFormat = GL_RGB; // Patch for backward compatibility

    gal_uint compressedFormat = false;

    switch ( internalFormat )
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            compressedFormat = true;
            break;

        default:
            break;
    }

    if ( compressedSize != 0 || compressedFormat) // Compressed texture (glCompressedTexImage2D)
    {
        if (!compressedFormat)
        {
                char error[128];
                sprintf(error, "Unexpected compressed internal format: %d", internalFormat);
                CG_ASSERT(error);
        }

        if (compressedSize == 0)
        {
            convertedData = _galDev->compressTexture(ogl::getGALTextureFormat(inputFormat), ogl::getGALTextureFormat(internalFormat), width, height, (gal_ubyte* )data, 0);
            compressedSize = (width * height * 4)/8;
        }
        else
            convertedData = (GLubyte *)data;

        return internalFormat;
    }

    if ( type == GL_FLOAT )
        CG_ASSERT("GL_FLOAT type not yet supported");

    switch ( inputFormat )
    {
        case GL_RGBA8:
        case GL_RGBA:
        case GL_RGB:
        case GL_RGB8:
        case GL_BGR:
        case GL_BGRA:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT24:
            break;
        default:
            char error[128];
            sprintf(error, "Texture input format not supported: %d", inputFormat);
            CG_ASSERT(error);
    }

    switch ( internalFormat )
    {
        case GL_RGB:
        case GL_RGBA:
        case GL_INTENSITY8:
        case GL_LUMINANCE:
        case GL_ALPHA:
        case GL_ALPHA8:
        case GL_DEPTH_COMPONENT24:
            break;

        case GL_RGB8:
        case GL_COMPRESSED_RGB:
            internalFormat = GL_RGB;
            break;

        case GL_RGBA8:
        case GL_COMPRESSED_RGBA:
            internalFormat = GL_RGBA;
            break;

        case GL_LUMINANCE8:
            internalFormat = GL_LUMINANCE;
            break;

        default:
            char error[128];
            sprintf(error, "Unexpected internal format: %d", internalFormat);
            CG_ASSERT("Unexpected internal format");
    }

    GLsizei bytesPerComponent = getSize(type);
    GLsizei texelsSize = 4; // RGBA size

    if ( bytesPerComponent != 1 && bytesPerComponent != 2 && bytesPerComponent != 4 )
        CG_ASSERT("Bytes per component must be 1, 2 or 4");

    if ( inputFormat == GL_RGB && (internalFormat == GL_RGB || internalFormat == GL_RGBA)) // Convert to RGBA
    {
        GLubyte* rgba = new GLubyte[width*height*4]; // Assumes 1 byte per component

        convertRGBtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width,height ); // Support RGB coverting RGB to RGBA

        convertedData = rgba;
        internalFormat = GL_RGBA;
    }
    else if ( inputFormat == GL_RGBA && internalFormat == GL_RGB )
    {
        GLubyte* rgba = new GLubyte[width*height*4];

        if ( bytesPerComponent != getSize(GL_UNSIGNED_BYTE) )
            convertRGBAtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);
        else
            memcpy(rgba, data, width*height*4); // Assumes UNSIGNED_BYTE

        convertedData = rgba;
        internalFormat = GL_RGBA;
    }
    else if ( (inputFormat == GL_BGR || inputFormat == GL_BGRA) && (internalFormat == GL_RGB || internalFormat == GL_RGBA) )
    {
        GLubyte* rgba = new GLubyte[width*height*4];

        if ( inputFormat == GL_BGR )
            convertBGRtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);
        else
            convertBGRAtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);

        convertedData = rgba;
        internalFormat = GL_RGBA;
    }
    else if ( inputFormat == GL_RGBA && internalFormat == GL_INTENSITY8 )
    {
        GLubyte* intensity = new GLubyte[width*height*1]; // GL_INTENSITY8 = 1 byte

        convertRGBAtoINTENSITY((const GLubyte*)data, intensity, bytesPerComponent,1, width, height);

        convertedData = intensity;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_ALPHA && (internalFormat == GL_ALPHA || internalFormat == GL_ALPHA8 ))
    {
        GLubyte* alpha = new GLubyte[width*height];

        if ( bytesPerComponent != getSize(GL_UNSIGNED_BYTE) )
            convertALPHAtoALPHA((const GLubyte*)data, alpha, bytesPerComponent,1, width, height);
        else
            memcpy(alpha, data, width * height);

        convertedData = alpha;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_LUMINANCE && internalFormat == GL_LUMINANCE )
    {
        GLubyte* luminance = new GLubyte[width*height*1];

        memcpy(luminance, data, width*height*1); // Assumes UNSIGNED_BYTE

        convertedData = luminance;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_DEPTH_COMPONENT && internalFormat == GL_DEPTH_COMPONENT24 )
    {

        GLubyte* depth24 = new GLubyte[width*height*4]; // Assumes 1 byte per component

        if(data)
            convertDEPTHtoDEPTH24((const GLubyte*)data, depth24, bytesPerComponent, 1, width,height );

        convertedData = depth24;
        internalFormat = GL_DEPTH_COMPONENT24;
    }
    else
        convertedData = (GLubyte *)data;

    //  Special check for type with different endianess.
    if (type == GL_UNSIGNED_INT_8_8_8_8)
        return GL_UNSIGNED_INT_8_8_8_8;
        
    return internalFormat;
}



void GLTextureObject::setPartial2D(GLint width, GLint height, GLint xoffset, GLint yoffset, GLenum inputFormat, 
                                   GLenum internalFormat, GLenum type, const GLvoid* data, GLubyte* &convertedData, GLint &compressedSize)
{

    if ( type != type )
        CG_ASSERT("type previously defined was diferent (maybe is not a panic)");
    
    if (xoffset < 0)
        CG_ASSERT("negative xoffset (maybe is not a panic)");
        
    if (yoffset < 0)
        CG_ASSERT("negative yoffset (maybe is not a panic)");    
        
    /*if (xoffset >= _width || yoffset >= _height)
        CG_ASSERT("No texture image area is updated. xoffset or yoffset lay outside the texture dimensions. (maybe is not a panic)");    

    if (width > _width)
        CG_ASSERT("previously defined width was smaller (maybe is not a panic)");
    
    if (height > _height)
        CG_ASSERT("previously defined height was smaller (maybe is not a panic)");

    if ( type == GL_FLOAT )
        CG_ASSERT("GL_FLOAT type not yet supported");*/
     

    switch ( inputFormat ) // Check format
    {
        case GL_RGBA8:
        case GL_RGBA:
        case GL_RGB:
        case GL_RGB8:
        case GL_BGR:
        case GL_BGRA:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            break;

        default:
            char error[128];
            sprintf(error, "Texture input format not supported: %d", inputFormat);
            CG_ASSERT(error);
    }

    switch ( internalFormat )
    {
        case GL_COMPRESSED_RGBA:
            popup("GLTextureObject", "Warning. defining a Mipmap to be compressed and compression is not supported. This texture will not be compressed (used as regular RGBA)");
            internalFormat = GL_RGBA;
            break;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:            
            convertedData = (GLubyte*)(data);
            return;
            break;

        case GL_RGBA:
        case GL_RGB:
        case GL_INTENSITY8:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
        case GL_ALPHA:
        case GL_ALPHA8:
        case GL_DEPTH_COMPONENT24:
            break;

        case GL_RGB8:
        case GL_COMPRESSED_RGB:
            internalFormat = GL_RGB;
            break;

        case GL_RGBA8:
            internalFormat = GL_RGBA;
            break;

        default:
            char error[128];
            sprintf(error, "Unexpected internal format: %d", internalFormat);
            CG_ASSERT("Unexpected internal format");
    }

    GLsizei bytesPerComponent = getSize(type);
    GLsizei texelsSize = 4; // RGBA size

    if ( bytesPerComponent != 1 && bytesPerComponent != 2 && bytesPerComponent != 4 )
        CG_ASSERT("Bytes per component must be 1, 2 or 4");

        
    if ( inputFormat == GL_RGB && (internalFormat == GL_RGB || internalFormat == GL_RGBA)) // Convert to RGBA
    {
        GLubyte* rgba = new GLubyte[width*height*4]; // Assumes 1 byte per component

        convertRGBtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height); // Support RGB coverting RGB to RGBA

        convertedData = rgba;
    }
    else if ( inputFormat == GAL_FORMAT_RGBA_8888 && internalFormat == GL_RGB )
    {
        GLubyte* rgba = new GLubyte[width*height*4];

        if ( bytesPerComponent != getSize(GL_UNSIGNED_BYTE) )
            convertRGBAtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);
        else
            memcpy(rgba, data, width*height*4); // Assumes UNSIGNED_BYTE

        convertedData = rgba;
    }
    else if ( (inputFormat == GL_BGR || inputFormat == GL_BGRA) && (internalFormat == GL_RGB || internalFormat == GL_RGBA) )
    {
        GLubyte* rgba = new GLubyte[width*height*4];

        if ( inputFormat == GL_BGR )
            convertBGRtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);
        else
            convertBGRAtoRGBA((const GLubyte*)data, rgba, bytesPerComponent, 1, width, height);

        convertedData = rgba;
    }
    else if ( inputFormat == GL_RGBA && internalFormat == GL_INTENSITY8 )
    {
        // GL_INTENSITY8 = 1 byte
        GLubyte* intensity = new GLubyte[width*height*1];

        convertRGBAtoINTENSITY((const GLubyte*)data, intensity, bytesPerComponent,1, width, height);

        convertedData = intensity;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_ALPHA && (internalFormat == GL_ALPHA || internalFormat == GL_ALPHA8 ))
    {
        GLubyte* alpha = new GLubyte[width*height];

        if ( bytesPerComponent != getSize(GL_UNSIGNED_BYTE) )
            convertALPHAtoALPHA((const GLubyte*)data, alpha, bytesPerComponent,1, width, height);
        else
            memcpy(alpha, data, width * height);

        convertedData = alpha;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_LUMINANCE && internalFormat == GL_LUMINANCE )
    {
        GLubyte* luminance = new GLubyte[width*height*1];

        memcpy(luminance, data, width*height*1); // Assumes UNSIGNED_BYTE

        convertedData = luminance;
        texelsSize = 1;
    }
    else if ( inputFormat == GL_DEPTH_COMPONENT && internalFormat == GL_DEPTH_COMPONENT24 )
    {

        GLubyte* depth24 = new GLubyte[width*height*4]; // Assumes 1 byte per component

        if(data)
            convertDEPTHtoDEPTH24((const GLubyte*)data, depth24, bytesPerComponent, 1, width,height ); // Support RGB coverting RGB to RGBA

        convertedData = depth24;
        internalFormat = GL_RGBA;
    }
    else
        convertedData = (GLubyte *)data;

}

void GLTextureObject::setParameter(GLenum pname, const GLint* params)
{
    int parameter = params[0];
    switch ( pname )
    {
        case GL_TEXTURE_WRAP_S:
            _wrapS = parameter;
            break;

        case GL_TEXTURE_WRAP_T:
            _wrapT = parameter;
            break;

        case GL_TEXTURE_WRAP_R:
            _wrapR = parameter;
            break;

        case GL_TEXTURE_MAG_FILTER:
            _magFilter = parameter;
            break;

        case GL_TEXTURE_MIN_FILTER:
            switch (parameter)
            {
                case GL_NEAREST: break;
                case GL_LINEAR:  break;
                case GL_NEAREST_MIPMAP_NEAREST: break;
                case GL_NEAREST_MIPMAP_LINEAR: break;
                case GL_LINEAR_MIPMAP_NEAREST: break;
                case GL_LINEAR_MIPMAP_LINEAR: break;
                default:
                    CG_ASSERT("Unknown texture min filter");
            }

            _minFilter = parameter;
            break;

        case GL_TEXTURE_MAX_ANISOTROPY_EXT:

            //
            // Patch to force max aniso 16 for all the traces that were captured in the old NVidia GeForce5950XT
            // that only supported max aniso 8.
            //
            _maxAnisotropy = (parameter == 8) ? 16.0f : GLfloat(parameter);

            break;

        case GL_TEXTURE_BASE_LEVEL:
            if ( _baseLevel < 0 )
                CG_ASSERT("GL_TEXTURE_BASE_LEVEL value must be non-negative");

            _baseLevel = parameter;
            break;

        case GL_TEXTURE_MAX_LEVEL:
            if ( _maxLevel < 0 )
                CG_ASSERT("GL_TEXTURE_MAX_LEVEL value must be non-negative");

            _maxLevel = parameter;
            break;

        case GL_TEXTURE_MIN_LOD:
            _minLOD = static_cast<GLfloat>(parameter);
            break;

        case GL_TEXTURE_MAX_LOD:
            _maxLOD = static_cast<GLfloat>(parameter);
            break;

        default:
            CG_ASSERT("Parameter name not supported");
    }
}


void GLTextureObject::setParameter(GLenum pname, const GLfloat* params)
{
    switch ( pname )
    {
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_TEXTURE_WRAP_R_EXT:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_BASE_LEVEL:
        case GL_TEXTURE_MAX_LEVEL:
            {
                GLint param = GLint(params[0]);
                setParameter(pname, &param);
            }
            break;

        case GL_TEXTURE_MIN_LOD:
            _minLOD = params[0];
            break;

        case GL_TEXTURE_MAX_LOD:
            _maxLOD = params[0];
            break;

        case GL_TEXTURE_MAX_ANISOTROPY_EXT:

            //
            // Patch to force max aniso 16 for all the traces that were captured in the old NVidia GeForce5950XT
            // that only supported max aniso 8.
            //
            _maxAnisotropy = (params[0] == 8.0f) ? 16.0f : params[0];

            break;
        case GL_TEXTURE_LOD_BIAS:
            _biasLOD = params[0];
            break;

        default:
            {
                char msg[128];
                sprintf(msg, "Parameter not supported: 0x%x", pname);
                CG_ASSERT(msg);
            }
    }
}


GLuint GLTextureObject::getImageSize(GLenum targetFace, GLenum mipmap)
{
    switch (targetFace)
    {
        case GL_TEXTURE_2D:
            return _texture2D->getWidth(mipmap) * _texture2D->getHeight(mipmap) * getSize(_texture2D->getFormat(mipmap));
            break;

        case GL_TEXTURE_3D:
            return _texture3D->getDepth(mipmap) * _texture3D->getHeight(mipmap) * _texture3D->getWidth(mipmap) * getSize(_texture3D->getFormat(mipmap));
            break;

        case GL_TEXTURE_RECTANGLE:
            return _textureRECT->getWidth(mipmap) * _textureRECT->getHeight(mipmap) * getSize(_textureRECT->getFormat(mipmap));
            break;

        case GL_TEXTURE_CUBE_MAP:
            {
                GAL_CUBEMAP_FACE cubeFace = ogl::getGALCubeMapFace(targetFace);
                return _textureCM->getWidth(cubeFace, mipmap) * _textureCM->getHeight(cubeFace, mipmap) * getSize(_textureCM->getFormat(cubeFace,mipmap)) * 6;
            }
            break;
        default:
            CG_ASSERT("Target not supported yet");
            return 0;
            break;
    }
}


GLsizei GLTextureObject::getWidth(GLenum targetFace, GLenum mipmap) const
{
    switch (targetFace)
    {
        case GL_TEXTURE_2D:
            return _texture2D->getWidth(mipmap);            
            break;

        case GL_TEXTURE_3D:
            return _texture3D->getWidth(mipmap);
            break;

        case GL_TEXTURE_RECTANGLE:
            return _textureRECT->getWidth(mipmap);            
            break;

        case GL_TEXTURE_CUBE_MAP:
            {
                GAL_CUBEMAP_FACE cubeFace = ogl::getGALCubeMapFace(targetFace);
                return _textureCM->getWidth(cubeFace, mipmap);
            }
            break;

        default:
            CG_ASSERT("Target not supported yet");
            return 0;
            break;
    }
}


GLsizei GLTextureObject::getHeight(GLenum targetFace, GLenum mipmap) const
{
    switch (targetFace)
    {
        case GL_TEXTURE_2D:
            return _texture2D->getHeight(mipmap);            
            break;

        case GL_TEXTURE_3D:
            return _texture3D->getHeight(mipmap);
            break;

        case GL_TEXTURE_RECTANGLE:
            return _textureRECT->getHeight(mipmap);            
            break;

        case GL_TEXTURE_CUBE_MAP:
            {
                GAL_CUBEMAP_FACE cubeFace = ogl::getGALCubeMapFace(targetFace);
                return _textureCM->getHeight(cubeFace, mipmap);
            }
            break;

        default:
            CG_ASSERT("Target not supported yet");
            return 0;
            break;
    }
}


GLsizei GLTextureObject::getDepth(GLenum targetFace, GLenum mipmap) const
{
    CG_ASSERT("Function not implemented yet");
    return 0;
}


GLenum GLTextureObject::getFormat(GLenum targetFace, GLenum mipmap) const
{
    switch (targetFace)
    {
        case GL_TEXTURE_2D:
            return _texture2D->getFormat(mipmap);            
            break;

        case GL_TEXTURE_3D:
            return _texture3D->getFormat(mipmap);
            break;

        case GL_TEXTURE_RECTANGLE:
            return _textureRECT->getFormat(mipmap);            
            break;

        case GL_TEXTURE_CUBE_MAP:
            {
                GAL_CUBEMAP_FACE cubeFace = ogl::getGALCubeMapFace(targetFace);
                return _textureCM->getFormat(cubeFace,mipmap);
            }
            break;
        default:
            cout << targetFace << endl;
            CG_ASSERT("Target not supported yet");
            return 0;
            break;
    }
}


GLenum GLTextureObject::getBaseInternalFormat() const
{
    GLint ifmt;

    if (_texture2D)
        ifmt = _texture2D->getFormat(_texture2D->getBaseLevel());
    else if (_textureRECT)
        ifmt = _textureRECT->getFormat(_textureRECT->getBaseLevel());
    else
        ifmt = _textureCM->getFormat((GAL_CUBEMAP_FACE)0,_textureCM->getBaseLevel());

    switch ( ifmt )
    {
        case GAL_FORMAT_ALPHA_8:
        case GAL_FORMAT_ALPHA_12:
        case GAL_FORMAT_ALPHA_16:
            return GL_ALPHA;
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            return GL_DEPTH_COMPONENT;
        case GAL_FORMAT_LUMINANCE_8:
        case GAL_FORMAT_LUMINANCE_12:
        case GAL_FORMAT_LUMINANCE_16:
            return GL_LUMINANCE;
        /*case GL_LUMINANCE_ALPHA:
        case GL_LUMINANCE4_ALPHA4:
        case GL_LUMINANCE6_ALPHA2:
        case GL_LUMINANCE8_ALPHA8:
        case GL_LUMINANCE12_ALPHA4:
        case GL_LUMINANCE12_ALPHA12:
        case GL_LUMINANCE16_ALPHA16:
            return GL_LUMINANCE_ALPHA;*/
        case GAL_FORMAT_INTENSITY_8:
        case GAL_FORMAT_INTENSITY_12:
        case GAL_FORMAT_INTENSITY_16:
            return GL_INTENSITY;
        case GAL_FORMAT_RGB_888:
        /*case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB8:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGB16:*/
        case GAL_COMPRESSED_S3TC_DXT1_RGB:
            return GL_RGB;
        case GAL_FORMAT_RGBA_8888:
        case GAL_FORMAT_UNSIGNED_INT_8888:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB5_A1:
        case GL_RGBA8:
        case GL_RGB10_A2:
        case GL_RGBA12:
        case GL_RGBA16:
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
            return GL_RGBA;
        default:
            CG_ASSERT("Cannot find a base ifmt for this format");
            return 0;
    }
}

GLuint GLTextureObject::getSize(GLenum openGLType)
{
    switch ( openGLType )
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_INT_8_8_8_8:
            return sizeof(GLubyte);
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_INT:
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_DOUBLE:
            return sizeof(GLdouble);
        case GL_2_BYTES:
            return 2*sizeof(GLubyte);
        case GL_3_BYTES:
            return 3*sizeof(GLubyte);
        case GL_4_BYTES:
            return 4*sizeof(GLubyte);
        default:
            char error[128];
            sprintf(error, "Unknown openGL type size: %d", openGLType);
            CG_ASSERT(error);
            return 0;
    }
}


GLenum GLTextureObject::getGLTextureFormat(GAL_FORMAT format)
{

    switch(format)
    {
        case GAL_FORMAT_RGB_888:
            return GL_RGB8;
            break;
        case GAL_FORMAT_RGBA_8888:
            return GL_RGBA;
            break;

        case GAL_FORMAT_INTENSITY_8:
            return GL_INTENSITY8;
            break;
        case GAL_FORMAT_INTENSITY_12:
            return GL_INTENSITY12;
            break;
        case GAL_FORMAT_INTENSITY_16:
            return GL_INTENSITY16;
            break;

        case GAL_FORMAT_ALPHA_8:
            return GL_ALPHA8;
            break;
        case GAL_FORMAT_ALPHA_12:
            return GL_ALPHA12;
            break;
        case GAL_FORMAT_ALPHA_16:
            return GL_ALPHA16;
            break;


        case GAL_FORMAT_LUMINANCE_8:
            return GL_LUMINANCE8;
            break;
        case GAL_FORMAT_LUMINANCE_12:
            return GL_LUMINANCE12;
            break;
        case GAL_FORMAT_LUMINANCE_16:
            return GL_LUMINANCE16;
            break;

        case GAL_FORMAT_DEPTH_COMPONENT_16:
            return GL_DEPTH_COMPONENT16;
            break;
        case GAL_FORMAT_DEPTH_COMPONENT_24:
            return GL_DEPTH_COMPONENT24;
            break;
        case GAL_FORMAT_DEPTH_COMPONENT_32:
            return GL_DEPTH_COMPONENT32;
            break;

        case GAL_COMPRESSED_S3TC_DXT1_RGB:
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            break;
        case GAL_COMPRESSED_S3TC_DXT1_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case GAL_COMPRESSED_S3TC_DXT3_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case GAL_COMPRESSED_S3TC_DXT5_RGBA:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;


        default:
            char error[128];
            sprintf(error, "GAL format unknown: %d", format);
            CG_ASSERT(error);
            return 0;
    }
}

///////////////
///////////////

void GLTextureObject::convertALPHAtoALPHA(const GLubyte* input, GLubyte* output,
                                          GLsizei ics, GLsizei ocs,
                                          GLsizei width, GLint height)
{
    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height;

    GLdouble a;

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            a = GLdouble(ub_iptr[i*4]) / 0xFF;
        }
        else if ( ics == 2 )
            a = GLdouble(us_iptr[i*4]) / 0xFFFF;
        else if ( ics == 4 )
            a = GLdouble(ui_iptr[i*4]) / 0xFFFFFFFF;

        if ( ocs == 1 )
            ub_optr[i] = GLubyte(a * 0xFF);
        else if ( ocs == 2 )
            us_optr[i] = GLushort(a * 0xFFFF);
        else if ( ocs == 4 )
            ui_optr[i] = GLuint(a * 0xFFFFFFFF);
    }
}

void GLTextureObject::convertLUMINANCEtoLUMINANCE(const GLubyte* input, GLubyte* output,
                                                        GLsizei ics, GLsizei ocs,
                                                        GLsizei width, GLint height)
{
    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height;

    GLdouble l;

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            //cout << "(" << GLint(ub_iptr[i*4]) << "," << GLint(ub_iptr[i*4+1]) << "," << GLint(ub_iptr[i*4+2]) << "," << GLint(ub_iptr[i*4+3]) << ")" << endl;
            l = GLdouble(ub_iptr[i*4]) / 0xFF;
        }
        else if ( ics == 2 )
            l = GLdouble(us_iptr[i*4]) / 0xFFFF;
        else if ( ics == 4 )
            l = GLdouble(ui_iptr[i*4]) / 0xFFFFFFFF;

        if ( ocs == 1 )
            ub_optr[i] = GLubyte(l * 0xFF);
        else if ( ocs == 2 )
            us_optr[i] = GLushort(l * 0xFFFF);
        else if ( ocs == 4 )
            ui_optr[i] = GLuint(l * 0xFFFFFFFF);
    }
}

void GLTextureObject::convertRGBAtoINTENSITY(const GLubyte* input, GLubyte* output,
                                         GLsizei ics, GLsizei ocs,
                                         GLsizei width, GLint height)
{
    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height;

    GLdouble r;

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            //cout << "(" << GLint(ub_iptr[i*4]) << "," << GLint(ub_iptr[i*4+1]) << "," << GLint(ub_iptr[i*4+2]) << "," << GLint(ub_iptr[i*4+3]) << ")" << endl;
            r = GLdouble(ub_iptr[i*4]) / 0xFF;
        }
        else if ( ics == 2 )
            r = GLdouble(us_iptr[i*4]) / 0xFFFF;
        else if ( ics == 4 )
            r = GLdouble(ui_iptr[i*4]) / 0xFFFFFFFF;

        if ( ocs == 1 )
            ub_optr[i] = GLubyte(r * 0xFF);
        else if ( ocs == 2 )
            us_optr[i] = GLushort(r * 0xFFFF);
        else if ( ocs == 4 )
            ui_optr[i] = GLuint(r * 0xFFFFFFFF);
    }
}

void GLTextureObject::convertRGBtoRGB(const GLubyte* input, GLubyte* output,
                                    GLsizei ics, GLsizei ocs,
                                    GLsizei width, GLsizei height)
{
    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLdouble r, g, b; // temporary values

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            r = GLdouble(ub_iptr[i*3]) / 0xFF;
            g = GLdouble(ub_iptr[i*3+1]) / 0xFF;
            b = GLdouble(ub_iptr[i*3+2]) / 0xFF;
        }
        else if ( ics == 2 )
        {
            r = GLdouble(us_iptr[i*3]) / 0xFFFF;
            g = GLdouble(us_iptr[i*3+1]) / 0xFFFF;
            b = GLdouble(us_iptr[i*3+2]) / 0xFFFF;
        }
        else if ( ics == 4 )
        {
            r = GLdouble(ui_iptr[i*3]) / 0xFFFFFFFF;
            g = GLdouble(ui_iptr[i*3+1]) / 0xFFFFFFFF;
            b = GLdouble(ui_iptr[i*3+2]) / 0xFFFFFFFF;
        }

        // r, g and b contains values in the range [0..1]

        if ( ocs == 1 )
        {
            ub_optr[i*3] = GLubyte(r * 0xFF);
            ub_optr[i*3+1] = GLubyte(g * 0xFF);
            ub_optr[i*3+2] = GLubyte(b * 0xFF);

        }
        else if ( ocs == 2 )
        {
            us_optr[i*3] = GLushort(r * 0xFFFF);
            us_optr[i*3+1] = GLushort(g * 0xFFFF);
            us_optr[i*3+2] = GLushort(b * 0xFFFF);
        }
        else if ( ocs == 4 )
        {
            ui_optr[i*3] = GLuint(r * 0xFFFFFFFF);
            ui_optr[i*3+1] = GLuint(g * 0xFFFFFFFF);
            ui_optr[i*3+2] = GLuint(b * 0xFFFFFFFF);
        }
    }
}


void GLTextureObject::convertBGRtoRGBA(const GLubyte* input, GLubyte* output,
                                                    GLsizei ics, GLsizei ocs,
                                                    GLsizei width, GLint height)
{

    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLdouble r, g, b; // temporary values

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            b = GLdouble(ub_iptr[i*3]) / 0xFF;
            g = GLdouble(ub_iptr[i*3+1]) / 0xFF;
            r = GLdouble(ub_iptr[i*3+2]) / 0xFF;
        }
        else if ( ics == 2 )
        {
            b = GLdouble(us_iptr[i*3]) / 0xFFFF;
            g = GLdouble(us_iptr[i*3+1]) / 0xFFFF;
            r = GLdouble(us_iptr[i*3+2]) / 0xFFFF;
        }
        else if ( ics == 4 )
        {
            b = GLdouble(ui_iptr[i*3]) / 0xFFFFFFFF;
            g = GLdouble(ui_iptr[i*3+1]) / 0xFFFFFFFF;
            r = GLdouble(ui_iptr[i*3+2]) / 0xFFFFFFFF;
        }

        // r, g and b values contain values in the range [0..1]

        if ( ocs == 1 )
        {
            ub_optr[i*4] = GLubyte(r * 0xFF);
            ub_optr[i*4+1] = GLubyte(g * 0xFF);
            ub_optr[i*4+2] = GLubyte(b * 0xFF);
            ub_optr[i*4+3] = 0xFF;
        }
        else if ( ocs == 2 )
        {
            us_optr[i*4] = GLushort(r * 0xFFFF);
            us_optr[i*4+1] = GLushort(g * 0xFFFF);
            us_optr[i*4+2] = GLushort(b * 0xFFFF);
            us_optr[i*4+3] = 0xFFFF;
        }
        else if ( ocs == 4 )
        {
            ui_optr[i*4] = GLuint(r * 0xFFFFFFFF);
            ui_optr[i*4+1] = GLuint(g * 0xFFFFFFFF);
            ui_optr[i*4+2] = GLuint(b * 0xFFFFFFFF);
            ui_optr[i*4+3] = 0xFFFFFFFF;
        }
    }
}

void GLTextureObject::convertBGRAtoRGBA(const GLubyte* input, GLubyte* output,
                                                     GLsizei ics, GLsizei ocs,
                                                     GLsizei width, GLint height)
{
    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLdouble r, g, b, a; // temporary values

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            b = GLdouble(ub_iptr[i*4]) / 0xFF;
            g = GLdouble(ub_iptr[i*4+1]) / 0xFF;
            r = GLdouble(ub_iptr[i*4+2]) / 0xFF;
            a = GLdouble(ub_iptr[i*4+3]) / 0xFF;
        }
        else if ( ics == 2 )
        {
            b = GLdouble(us_iptr[i*4]) / 0xFFFF;
            g = GLdouble(us_iptr[i*4+1]) / 0xFFFF;
            r = GLdouble(us_iptr[i*4+2]) / 0xFFFF;
            a = GLdouble(us_iptr[i*4+3]) / 0xFFFF;
        }
        else if ( ics == 4 )
        {
            b = GLdouble(ui_iptr[i*4]) / 0xFFFFFFFF;
            g = GLdouble(ui_iptr[i*4+1]) / 0xFFFFFFFF;
            r = GLdouble(ui_iptr[i*4+2]) / 0xFFFFFFFF;
            a = GLdouble(ui_iptr[i*4+3]) / 0xFFFFFFFF;
        }

        // r, g, b and a contains values in the range [0..1]

        if ( ocs == 1 )
        {
            ub_optr[i*4] = GLubyte(r * 0xFF);
            ub_optr[i*4+1] = GLubyte(g * 0xFF);
            ub_optr[i*4+2] = GLubyte(b * 0xFF);
            ub_optr[i*4+3] = GLubyte(a * 0xFF);
        }
        else if ( ocs == 2 )
        {
            us_optr[i*4] = GLushort(r * 0xFFFF);
            us_optr[i*4+1] = GLushort(g * 0xFFFF);
            us_optr[i*4+2] = GLushort(b * 0xFFFF);
            us_optr[i*4+3] = GLushort(a * 0xFFFF);
        }
        else if ( ocs == 4 )
        {
            ui_optr[i*4] = GLuint(r * 0xFFFFFFFF);
            ui_optr[i*4+1] = GLuint(g * 0xFFFFFFFF);
            ui_optr[i*4+2] = GLuint(b * 0xFFFFFFFF);
            ui_optr[i*4+3] = GLuint(a * 0xFFFFFFFF);
        }
    }
}


void GLTextureObject::convertRGBtoRGBA(const GLubyte* input, GLubyte* output,
                                    GLsizei ics, GLsizei ocs,  // inputComponentSize and outputComponentSize 
                                    GLsizei width, GLsizei height)
{

    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLdouble r, g, b; // temporary values

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            r = GLdouble(ub_iptr[i*3]) / 0xFF;
            g = GLdouble(ub_iptr[i*3+1]) / 0xFF;
            b = GLdouble(ub_iptr[i*3+2]) / 0xFF;
        }
        else if ( ics == 2 )
        {
            r = GLdouble(us_iptr[i*3]) / 0xFFFF;
            g = GLdouble(us_iptr[i*3+1]) / 0xFFFF;
            b = GLdouble(us_iptr[i*3+2]) / 0xFFFF;
        }
        else if ( ics == 4 )
        {
            r = GLdouble(ui_iptr[i*3]) / 0xFFFFFFFF;
            g = GLdouble(ui_iptr[i*3+1]) / 0xFFFFFFFF;
            b = GLdouble(ui_iptr[i*3+2]) / 0xFFFFFFFF;
        }

        // r, g and b contains values in the range [0..1]

        if ( ocs == 1 )
        {
            ub_optr[i*4] = GLubyte(r * 0xFF);
            ub_optr[i*4+1] = GLubyte(g * 0xFF);
            ub_optr[i*4+2] = GLubyte(b * 0xFF);
            ub_optr[i*4+3] = 0xFF;
        }
        else if ( ocs == 2 )
        {
            us_optr[i*4] = GLushort(r * 0xFFFF);
            us_optr[i*4+1] = GLushort(g * 0xFFFF);
            us_optr[i*4+2] = GLushort(b * 0xFFFF);
            us_optr[i*4+3] = 0xFFFF;
        }
        else if ( ocs == 4 )
        {
            ui_optr[i*4] = GLuint(r * 0xFFFFFFFF);
            ui_optr[i*4+1] = GLuint(g * 0xFFFFFFFF);
            ui_optr[i*4+2] = GLuint(b * 0xFFFFFFFF);
            ui_optr[i*4+3] = 0xFFFFFFFF;
        }
    }
}


void GLTextureObject::convertDEPTHtoDEPTH24(const GLubyte* input, GLubyte* output,
                                    GLsizei ics, GLsizei ocs,  // inputComponentSize and outputComponentSize 
                                    GLsizei width, GLsizei height)
{

    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 3 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLuint d; // temporary value

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
            d = GLuint(ub_iptr[i]);
        else if ( ics == 2 )
            d = GLuint(us_iptr[i]);
        else if ( ics == 4 )
            d = GLuint(ui_iptr[i]);

        // r, g and b contains values in the range [0..1]

        if ( ocs == 1 )
        {
            //F32(*((U32 *) data) & 0x00FFFFFF) / 16777215.0f;

            ui_optr[i] = (16777215 * d)/255; 
        }
    }
}


void GLTextureObject::convertRGBAtoRGBA(const GLubyte* input, GLubyte* output,
                                    GLsizei ics, GLsizei ocs,  // inputComponentSize and outputComponentSize 
                                    GLsizei width, GLsizei height)
{

    if ( ics != 1 && ics != 2 && ics != 4 )
        CG_ASSERT("inputComponent size must be 1, 2 or 4");

    if ( ocs != 1 && ocs != 2 && ocs != 4 )
        CG_ASSERT("outputComponent size must be 1, 2 or 4");

    const GLubyte* ub_iptr = input;
    const GLushort* us_iptr = (GLushort*)input;
    const GLuint* ui_iptr = (GLuint*)input;

    GLubyte* ub_optr = output;
    GLushort* us_optr = (GLushort*)output;
    GLuint* ui_optr = (GLuint*)output;

    GLsizei texels = width*height; // number of texels

    GLdouble r, g, b, a; // temporary values

    for ( GLint i = 0; i < texels; i++ )
    {
        if ( ics == 1 )
        {
            r = GLdouble(ub_iptr[i*3]) / 0xFF;
            g = GLdouble(ub_iptr[i*3+1]) / 0xFF;
            b = GLdouble(ub_iptr[i*3+2]) / 0xFF;
            a = GLdouble(ub_iptr[i*3+3]) / 0xFF;
        }
        else if ( ics == 2 )
        {
            r = GLdouble(us_iptr[i*3]) / 0xFFFF;
            g = GLdouble(us_iptr[i*3+1]) / 0xFFFF;
            b = GLdouble(us_iptr[i*3+2]) / 0xFFFF;
            a = GLdouble(us_iptr[i*3+3]) / 0xFFFF;
        }
        else if ( ics == 4 )
        {
            r = GLdouble(ui_iptr[i*3]) / 0xFFFFFFFF;
            g = GLdouble(ui_iptr[i*3+1]) / 0xFFFFFFFF;
            b = GLdouble(ui_iptr[i*3+2]) / 0xFFFFFFFF;
            a = GLdouble(ui_iptr[i*3+3]) / 0xFFFFFFFF;
        }

        // r, g, b and a contains values in the range [0..1]

        if ( ocs == 1 )
        {
            ub_optr[i*4] = GLubyte(r * 0xFF);
            ub_optr[i*4+1] = GLubyte(g * 0xFF);
            ub_optr[i*4+2] = GLubyte(b * 0xFF);
            ub_optr[i*4+3] = GLubyte(a * 0xFF);
        }
        else if ( ocs == 2 )
        {
            us_optr[i*4] = GLushort(r * 0xFFFF);
            us_optr[i*4+1] = GLushort(g * 0xFFFF);
            us_optr[i*4+2] = GLushort(b * 0xFFFF);
            us_optr[i*4+3] = GLushort(a * 0xFFFF);
        }
        else if ( ocs == 4 )
        {
            ui_optr[i*4] = GLuint(r * 0xFFFFFFFF);
            ui_optr[i*4+1] = GLuint(g * 0xFFFFFFFF);
            ui_optr[i*4+2] = GLuint(b * 0xFFFFFFFF);
            ui_optr[i*4+3] = GLuint(a * 0xFFFFFFFF);
        }
    }
}

