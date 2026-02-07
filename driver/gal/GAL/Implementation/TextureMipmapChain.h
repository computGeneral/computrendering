/**************************************************************************
 *
 */

#ifndef TEXTUREMIPMAPCHAIN
    #define TEXTUREMIPMAPCHAIN

#include "TextureMipmap.h"
#include <map>
#include <vector>

class HAL;

namespace libGAL
{

class TextureMipmapChain
{
public:

    TextureMipmap* create( gal_uint mipLevel );
    
    void destroy( gal_uint mipLevel );
    
    void destroyMipmaps();

    TextureMipmap* find(gal_uint mipmap);
    
    const TextureMipmap* find(gal_uint mipmap) const;

    std::vector<const TextureMipmap*> getMipmaps(gal_uint min, gal_uint max) const;
	
	gal_uint size();

private:

    std::map<gal_uint,TextureMipmap*> _mips;

};

}

#endif // TEXTUREMIPMAPCHAIN
