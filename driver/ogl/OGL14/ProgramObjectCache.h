/**************************************************************************
 *
 */

#ifndef PROGRAMOBJECTCACHE_H
    #define PROGRAMOBJECTCACHE_H

#include <map>
#include <list>
#include <vector>
#include "ProgramObject.h"
#include "gl.h"

namespace libgl
{

class ProgramObjectCache
{
public:

    ProgramObjectCache(U32 maxCacheablePrograms, U32 startPID, GLenum targetName);
    /**
     * Try to find a previous program with the same source code, if it is found the program
     * is returned, if not, a new program is created and added to the program cache
     */
    ProgramObject* get(const std::string& source, bool &cached);
    
    /**
     * Returns the  most recently used program
     */
    ProgramObject* getLastUsed() const;
    
    /**
     * Clear the program object from cache
     * the cleared programs are returned in a std::vector structure
     */
    std::vector<ProgramObject*> clear();
    
    bool full() const;
    
    // Returns the amount of programs stored currently in the cache
    U32 size() const;    
    
    void dumpStatistics() const;
    
private:

    typedef std::multimap<U32,ProgramObject*> ProgramCache;
    typedef ProgramCache::iterator PCIt;
    typedef std::pair<PCIt,PCIt> Range;
    
    ProgramObject* lastUsed;

    GLenum targetName;
    U32 maxPrograms;
    U32 startPID;
    U32 nextPID;
    ProgramCache cache;
    
    // STATISTICS
    U32 hits;
    U32 misses;
    U32 clears;
    
    U32 computeChecksum(const std::string& source) const;
};

} // namespace libgl

#endif // PROGRAMOBJECTCACHE_H
