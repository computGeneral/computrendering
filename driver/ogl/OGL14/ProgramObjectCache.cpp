/**************************************************************************
 *
 */

#include "ProgramObjectCache.h"
#include "gl.h"
#include "glext.h"

using namespace std;
using namespace libgl;

ProgramObjectCache::ProgramObjectCache(U32 maxCacheablePrograms, U32 startPID, GLenum targetName) : 
    maxPrograms(maxCacheablePrograms), startPID(startPID), nextPID(startPID), targetName(targetName), lastUsed(0),
    // Statistics
    hits(0), misses(0), clears(0)
{}

U32 ProgramObjectCache::computeChecksum(const std::string& source) const
{
    U32 sz = source.length();
    const char* sourceAscii = source.c_str();
    
    U32 cs = 0;
    for ( U32 i = 0; i < sz; i++ ) // trivial checksum
        cs += U32(sourceAscii[i]);
    
    return cs;
}

ProgramObject* ProgramObjectCache::getLastUsed() const
{
    return lastUsed;    
}


ProgramObject* ProgramObjectCache::get(const std::string& source, bool &cached)
{
    U32 cs = computeChecksum(source);
    Range range = cache.equal_range(cs);

    
    if ( range.first != range.second )
    {
        for ( PCIt matching = range.first; matching != range.second; ++matching )
        {
            if ( matching->second->getSource() == source )
            {
                hits++;
                cached = true;
                return (lastUsed = matching->second);
            }
        }
    }
    
    if ( cache.size() == maxPrograms )
        return 0;
        
    misses++;
    
    lastUsed = new ProgramObject(nextPID, targetName);
    nextPID++;
    cache.insert(make_pair(cs, lastUsed));
    cached = false;
    return lastUsed;
}

vector<ProgramObject*> ProgramObjectCache::clear()
{
    clears++;

    vector<ProgramObject*> v;
    v.reserve(cache.size()); // Simple optimization
    
    PCIt it = cache.begin();
    for ( ; it != cache.end(); it++ )
        v.push_back(it->second);

    // Reset internal state 
    lastUsed = 0;
    nextPID = startPID;
    cache.clear();
    return v;
}

bool ProgramObjectCache::full() const
{
    return cache.size() == maxPrograms;
}

U32 ProgramObjectCache::size() const
{
    return cache.size();
}

void ProgramObjectCache::dumpStatistics() const
{
    if ( targetName == GL_VERTEX_PROGRAM_ARB )
        cout << "ProgramObjectCache - VERTEX programs\n";
    else
        cout << "ProgramObjectCache - FARGMENT programs\n";
    
    cout << "hits: " << hits << " (" << (U32)(100.0f * ((F32)hits/(F32)(hits + misses))) << "%)\n";
    cout << "misses: " << misses << " (" << (U32)(100.0f * ((F32)misses/(F32)(hits + misses))) << "%)\n";
    cout << "clears: " << clears << "\n";
    cout << "programs in cache: " << cache.size() << endl;
}
