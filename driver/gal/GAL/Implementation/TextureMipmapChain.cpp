/**************************************************************************
 *
 */

#include "TextureMipmapChain.h"
#include <utility>

using namespace std;
using namespace libGAL;

vector<const TextureMipmap*> TextureMipmapChain::getMipmaps(gal_uint min, gal_uint max) const
{
    vector<const TextureMipmap*> mips;

    map<gal_uint, TextureMipmap*>::const_iterator it = _mips.lower_bound(min);
    map<gal_uint, TextureMipmap*>::const_iterator upperBound = _mips.upper_bound(max);
    for ( ; it != upperBound; ++it )
        mips.push_back(it->second);

    return mips;
}

TextureMipmap* TextureMipmapChain::create( gal_uint mipLevel )
{
    TextureMipmap* mip = find(mipLevel);
    
    if ( mip != 0 ) // previos mipmap definition exists, delete it
    {
        delete mip;
        _mips.erase(mipLevel);
    }

    mip = new TextureMipmap();
    _mips.insert(make_pair(mipLevel, mip));
    return mip;
}

void TextureMipmapChain::destroy( gal_uint mipLevel )
{
    map<gal_uint, TextureMipmap*>::iterator it = _mips.find(mipLevel);
    if ( it != _mips.end() )
        delete it->second;
}

void TextureMipmapChain::destroyMipmaps()
{
    map<gal_uint, TextureMipmap*>::iterator it = _mips.begin();
    for ( ; it != _mips.end(); ++it )
        delete it->second;
}

TextureMipmap* TextureMipmapChain::find(gal_uint mipLevel)
{
    map<gal_uint, TextureMipmap*>::iterator it = _mips.find(mipLevel);
    if ( it != _mips.end() )
        return it->second;
    return 0;
}

const TextureMipmap* TextureMipmapChain::find(gal_uint mipLevel) const
{
    map<gal_uint, TextureMipmap*>::const_iterator it = _mips.find(mipLevel);
    if ( it != _mips.end() )
        return it->second;
    return 0;
}

gal_uint TextureMipmapChain::size()
{
	return _mips.size();
}
